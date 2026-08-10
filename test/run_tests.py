#!/usr/bin/env python3
"""
End-to-end HOMFLY invariance tests for knoodle simplification.

Verifies that simplification preserves the HOMFLY polynomial, which is a
knot/link invariant. Any mismatch means the simplifier has a bug.

The HOMFLY-invariance tiers (1a, 1c, 1d, 2) use the C++ driver
`homfly_check --invariance` as their oracle: it exercises the core library's
PlanarDiagramComplex::Simplify directly and checks invariance with a vendored,
dependency-free libhomfly. No Python/Regina is needed for the default run.

Pass --cross-check to additionally run the legacy Regina oracle (simplifying
via the knoodlesimplify CLI) and require both to agree — useful for validating
the libhomfly path. That mode needs `regina` and the knoodlesimplify binary.

Tiers:
  1a: Hardcoded small diagrams (trefoil, Hopf link, figure-eight, unknot)
  1b: Hard unknots (simplification-only: must reduce to 0 crossings; uses
      knoodlesimplify, no HOMFLY — before-diagrams are too big to compute)
  1c: Link table from data/diagrams/linktable/
  1d: Children's-game knots (3D embeddings)
  2:  Exhaustive plantri-generated diagrams (requires plantri)

Usage:
  python run_tests.py --smoke-only
  python run_tests.py --hard-unknots-only
  python run_tests.py --linktable-only
  python run_tests.py                        # all Tier 1
  python run_tests.py --up-to-crossing-number=8 --crossing-assignments=all
  python run_tests.py --cross-check          # also validate against Regina
"""

import argparse
import glob
import os
import random
import struct
import subprocess
import sys
import tempfile
import time
from collections.abc import Iterator
from dataclasses import dataclass, field
from pathlib import Path


# ---------------------------------------------------------------------------
# Regina import (optional — only needed for --cross-check)
# ---------------------------------------------------------------------------

try:
    import regina
    REGINA_AVAILABLE = True
except ImportError:
    regina = None
    REGINA_AVAILABLE = False


# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent
DEFAULT_BINARY = PROJECT_DIR / "tools" / "knoodlesimplify"
DEFAULT_HOMFLY_CHECK = SCRIPT_DIR / "homfly_check"
LINKTABLE_DIR = PROJECT_DIR / "data" / "diagrams" / "linktable"
FAILURES_DIR = SCRIPT_DIR / "failures"
SIMPLIFICATION_LEVELS = [3, 4, 5, 6]
_failure_counter = 0


# ---------------------------------------------------------------------------
# Data structures
# ---------------------------------------------------------------------------

@dataclass
class TestResult:
    name: str
    passed: bool
    input_crossings: int = 0
    output_crossings: int = 0
    homfly_before: str = ""
    homfly_after: str = ""
    error: str = ""
    elapsed: float = 0.0  # seconds


class TestSuite:
    def __init__(self, verbose: bool = False, stop_on_failure: bool = False):
        self.results: list[TestResult] = []
        self.verbose = verbose
        self.stop_on_failure = stop_on_failure
        self._section = ""
        self._section_start: float = 0.0

    def section(self, name: str):
        self._section = name
        self._section_start = time.time()
        self._section_result_start = len(self.results)
        print(f"\n{'='*60}")
        print(f"  {name}")
        print(f"{'='*60}")

    def end_section(self):
        """Print per-section timing summary."""
        elapsed = time.time() - self._section_start
        n = len(self.results) - self._section_result_start
        passed = sum(1 for r in self.results[self._section_result_start:] if r.passed)
        print(f"\n  {self._section}: {passed}/{n} passed [{elapsed:.1f}s]")

    def record(self, result: TestResult):
        self.results.append(result)
        if result.passed:
            if self.verbose:
                timing = f"  {result.elapsed:.2f}s" if result.elapsed else ""
                print(f"  PASS  {result.name}"
                      f"  ({result.input_crossings} -> {result.output_crossings} crossings)"
                      f"{timing}")
        else:
            print(f"  FAIL  {result.name}")
            if result.error:
                print(f"        Error: {result.error}")
            else:
                print(f"        HOMFLY before: {result.homfly_before}")
                print(f"        HOMFLY after:  {result.homfly_after}")
            if self.stop_on_failure:
                self.summary()
                sys.exit(1)

    def record_batch(self, results: list[TestResult]):
        for r in results:
            self.record(r)

    def summary(self):
        passed = sum(1 for r in self.results if r.passed)
        failed = sum(1 for r in self.results if not r.passed)
        total = len(self.results)
        print(f"\n{'='*60}")
        print(f"  SUMMARY: {passed}/{total} passed, {failed} failed")
        print(f"{'='*60}")
        if failed > 0:
            print("\nFailed tests:")
            for r in self.results:
                if not r.passed:
                    print(f"  - {r.name}: {r.error or 'HOMFLY mismatch'}")
        if FAILURES_DIR.exists() and any(FAILURES_DIR.iterdir()):
            print(f"\nFailure artifacts saved to: {FAILURES_DIR}")
        return failed == 0


# ---------------------------------------------------------------------------
# PD code I/O
# ---------------------------------------------------------------------------

def detect_file_format(path: str | Path) -> str:
    """Detect whether a TSV file contains PD codes or 3D embedding coordinates.

    Returns "pd" for 5-column integer PD codes, "embedding" for 3-column floats.
    """
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) >= 5:
                try:
                    [int(x) for x in parts[:5]]
                    return "pd"
                except ValueError:
                    return "embedding"
            elif len(parts) == 3:
                return "embedding"
            break
    return "unknown"


def read_tsv_file(path: str | Path) -> list[list[int]]:
    """Read a 5-column TSV file into a list of [X0, X1, X2, X3, sign] rows."""
    crossings = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split('\t')
            if len(parts) >= 5:
                crossings.append([int(x) for x in parts[:5]])
    return crossings


def crossings_to_knoodle_tsv(crossings: list[list[int]]) -> str:
    """Format crossing rows as 5-column TSV string."""
    return '\n'.join('\t'.join(str(x) for x in row) for row in crossings)


def crossings_to_regina_pd(crossings: list[list[int]]) -> str:
    """Convert Knoodle 0-based crossings to Regina 1-based PD string.

    Regina.Link.fromPD() expects a string like "[[1,5,2,4],[3,1,4,6],[5,3,6,2]]"
    with 1-based arc indices.
    """
    if not crossings:
        return "[]"
    rows = []
    for row in crossings:
        # Take columns 0-3 (arcs), add 1 for Regina's 1-based convention
        rows.append(f"[{row[0]+1},{row[1]+1},{row[2]+1},{row[3]+1}]")
    return "[" + ",".join(rows) + "]"


# ---------------------------------------------------------------------------
# HOMFLY computation
# ---------------------------------------------------------------------------

def compute_homfly(crossings: list[list[int]]):
    """Compute HOMFLY polynomial for a single diagram.

    Returns a regina.Laurent2 object representing the HOMFLY-PT polynomial.
    For the unknot (empty crossings), returns the trivial polynomial (1).
    """
    if not crossings:
        # Unknot: HOMFLY = 1
        return regina.Link(1).homflyAZ()

    pd_str = crossings_to_regina_pd(crossings)
    try:
        link = regina.Link.fromPD(pd_str)
    except Exception as e:
        raise ValueError(f"Regina failed to parse PD code {pd_str}: {e}")
    return link.homflyAZ()


def compute_composite_homfly(summands: list[list[list[int]]]):
    """Compute HOMFLY for a composite knot/link (product of summand HOMFLYs).

    Non-empty summands are connected sum components: HOMFLY(K1 # K2) = H(K1)*H(K2).
    Empty summands are split unknot components (unlinked from the rest).

    The link decomposes as: (K1 # K2 # ... # Km) ⊔ O1 ⊔ ... ⊔ Op
    where Ki are connected sum factors and Oj are split unknot components.
    Total "pieces" = (1 if m>0 else 0) + p.
    HOMFLY-AZ = delta^(pieces-1) * product(HOMFLY-AZ(Ki))
    where delta = HOMFLY-AZ(2-component unlink) = (a^-1 - a)/z.
    """
    unknot_homfly = regina.Link(1).homflyAZ()
    delta = regina.Link(2).homflyAZ()  # (a^-1 - a)/z

    if not summands:
        return unknot_homfly

    # Separate non-empty (connected sum) and empty (split unknot) summands
    non_empty = [s for s in summands if s]
    num_empty = len(summands) - len(non_empty)

    # Product of connected sum component HOMFLYs
    conn_homfly = unknot_homfly
    for s in non_empty:
        conn_homfly = conn_homfly * compute_homfly(s)

    # Total split pieces: the connected sum (if any) + each split unknot
    pieces = (1 if non_empty else 0) + num_empty
    if pieces <= 1:
        return conn_homfly

    # Multiply by delta^(pieces-1) for the split unions
    result = conn_homfly
    for _ in range(pieces - 1):
        result = result * delta
    return result


# ---------------------------------------------------------------------------
# Knoodlesimplify I/O
# ---------------------------------------------------------------------------

def run_knoodlesimplify(diagrams: list[list[list[int]]],
                        binary: str | Path,
                        simplify_level: int = 6,
                        timeout: int = 300) -> list[list[list[list[int]]]]:
    """Run knoodlesimplify in streaming mode on a batch of diagrams.

    Args:
        diagrams: List of diagrams, each a list of [X0,X1,X2,X3,sign] rows.
        binary: Path to knoodlesimplify binary.
        simplify_level: Simplification level (1-6).
        timeout: Timeout in seconds per batch.

    Returns:
        List of results, one per input diagram. Each result is a list of
        summands, where each summand is a list of [X0,X1,X2,X3,sign] rows.
        An unknot result is [[]] (one empty summand) or [] (no summands).
    """
    # Build input: first diagram raw, subsequent preceded by "k\n"
    parts = []
    for i, diagram in enumerate(diagrams):
        if i > 0:
            parts.append("k")
        parts.append(crossings_to_knoodle_tsv(diagram))
    input_text = '\n'.join(parts) + '\n'

    cmd = [str(binary), "--streaming-mode", f"-s={simplify_level}"]
    try:
        proc = subprocess.run(
            cmd, input=input_text, capture_output=True, text=True,
            timeout=timeout
        )
    except subprocess.TimeoutExpired:
        raise RuntimeError(f"knoodlesimplify timed out after {timeout}s")

    if proc.returncode != 0:
        raise RuntimeError(
            f"knoodlesimplify failed (exit {proc.returncode}): {proc.stderr[:500]}")

    return parse_knoodlesimplify_output(proc.stdout, len(diagrams))


def run_knoodlesimplify_raw(raw_input: str,
                             binary: str | Path,
                             simplify_level: int = 6,
                             timeout: int = 300) -> list[list[list[int]]]:
    """Run knoodlesimplify on raw file content (e.g. 3D embedding).

    Returns a single result: list of summands, each a list of crossing rows.
    For 3D embeddings, knoodlesimplify auto-detects the format and projects.
    """
    cmd = [str(binary), "--streaming-mode", f"-s={simplify_level}"]
    try:
        proc = subprocess.run(
            cmd, input=raw_input, capture_output=True, text=True,
            timeout=timeout
        )
    except subprocess.TimeoutExpired:
        raise RuntimeError(f"knoodlesimplify timed out after {timeout}s")

    if proc.returncode != 0:
        raise RuntimeError(
            f"knoodlesimplify failed (exit {proc.returncode}): {proc.stderr[:500]}")

    results = parse_knoodlesimplify_output(proc.stdout, 1)
    return results[0]


def parse_knoodlesimplify_output(output: str,
                                  expected_count: int) -> list[list[list[list[int]]]]:
    """Parse knoodlesimplify streaming output into per-diagram summand lists.

    Output protocol:
      - 'k' on a line by itself separates diagrams (first diagram has no prefix)
      - 's' on a line by itself starts a new summand
      - Other lines are 5-column TSV crossing data

    Returns:
        List of diagram results. Each is a list of summands.
        Each summand is a list of [X0,X1,X2,X3,sign] rows.
    """
    lines = output.split('\n')

    # Split into per-diagram blocks
    diagram_blocks: list[list[str]] = [[]]
    for line in lines:
        if line == 'k':
            diagram_blocks.append([])
        else:
            diagram_blocks[-1].append(line)

    if len(diagram_blocks) != expected_count:
        raise RuntimeError(
            f"Expected {expected_count} diagram outputs, got {len(diagram_blocks)}")

    results = []
    for block in diagram_blocks:
        summands: list[list[list[int]]] = []
        current_summand: list[list[int]] | None = None

        for line in block:
            line = line.strip()
            if not line:
                continue
            if line == 's':
                current_summand = []
                summands.append(current_summand)
                continue
            parts = line.split('\t')
            if len(parts) >= 5 and current_summand is not None:
                current_summand.append([int(x) for x in parts[:5]])

        results.append(summands)

    return results


# ---------------------------------------------------------------------------
# Failure diagnosis helpers
# ---------------------------------------------------------------------------

def _next_failure_id() -> int:
    global _failure_counter
    _failure_counter += 1
    return _failure_counter


def _ensure_failures_dir():
    FAILURES_DIR.mkdir(exist_ok=True)


def summands_to_tsv(summands: list[list[list[int]]]) -> str:
    """Format summands as streaming TSV with 's' prefix per summand."""
    parts = []
    for summand in summands:
        parts.append("s")
        for row in summand:
            parts.append('\t'.join(str(x) for x in row))
    return '\n'.join(parts) + '\n'


def run_knoodledraw(tsv_input: str, knoodledraw_binary: str | Path) -> str | None:
    """Pipe TSV input to knoodledraw, return ASCII string or None on error."""
    try:
        proc = subprocess.run(
            [str(knoodledraw_binary), "--quality=best"],
            input=tsv_input, capture_output=True, text=True, timeout=30
        )
        if proc.returncode == 0 and proc.stdout.strip():
            return proc.stdout
    except Exception:
        pass
    return None


def find_minimal_failure_level(crossings: list[list[int]],
                                homfly_before,
                                binary: str | Path,
                                original_level: int) -> tuple[int, list[list[list[int]]] | None]:
    """Find the lowest simplification level that still produces a HOMFLY mismatch.

    Returns (min_level, summands_at_min_level). If no lower level fails,
    returns (original_level, None).
    """
    for level in SIMPLIFICATION_LEVELS:
        if level >= original_level:
            continue
        try:
            results = run_knoodlesimplify([crossings], binary, level)
            summands = results[0]
            homfly_after = compute_composite_homfly(summands)
            if homfly_before != homfly_after:
                return (level, summands)
        except Exception:
            continue
    return (original_level, None)


def diagnose_failure(name: str,
                     crossings: list[list[int]],
                     summands: list[list[list[int]]],
                     homfly_before,
                     homfly_after,
                     binary: str | Path,
                     simplify_level: int,
                     knoodledraw_binary: Path | None):
    """Print structured diagnostic report for a HOMFLY mismatch."""
    fid = _next_failure_id()
    _ensure_failures_dir()

    min_level, min_summands = find_minimal_failure_level(
        crossings, homfly_before, binary, simplify_level)

    diag_summands = min_summands if min_summands is not None else summands

    # Write before/after TSV files
    before_path = FAILURES_DIR / f"failure_{fid}_before.tsv"
    after_path = FAILURES_DIR / f"failure_{fid}_after.tsv"

    with open(before_path, 'w') as f:
        f.write(crossings_to_knoodle_tsv(crossings) + '\n')
    with open(after_path, 'w') as f:
        f.write(summands_to_tsv(diag_summands))

    # ASCII art (best-effort)
    before_art = None
    after_art = None
    if knoodledraw_binary:
        before_art = run_knoodledraw(
            crossings_to_knoodle_tsv(crossings) + '\n', knoodledraw_binary)
        after_art = run_knoodledraw(
            summands_to_tsv(diag_summands), knoodledraw_binary)

    output_crossings = sum(len(s) for s in diag_summands)
    n_summands = len(diag_summands)

    print(f"\n        --- Failure Diagnosis #{fid}: {name} ---")
    print(f"        Minimal failing level: {min_level} (original: {simplify_level})")
    print(f"        Files: {before_path}")
    print(f"               {after_path}")

    if before_art:
        print(f"\n        Before (input, {len(crossings)} crossings):")
        for line in before_art.splitlines():
            print(f"          {line}")

    if after_art:
        print(f"\n        After (-s={min_level}, {output_crossings} crossings,"
              f" {n_summands} summands):")
        for line in after_art.splitlines():
            print(f"          {line}")

    print(f"\n        --- End Diagnosis #{fid} ---")


def diagnose_embedding_failure(name: str,
                                raw_content: str,
                                before_summands: list[list[list[int]]],
                                after_summands: list[list[list[int]]],
                                homfly_before,
                                homfly_after,
                                binary: str | Path,
                                simplify_level: int,
                                knoodledraw_binary: Path | None):
    """Print structured diagnostic report for a 3D embedding HOMFLY mismatch."""
    fid = _next_failure_id()
    _ensure_failures_dir()

    # Find minimal failure level using raw input
    min_level = simplify_level
    min_summands = after_summands

    for level in SIMPLIFICATION_LEVELS:
        if level >= simplify_level:
            continue
        try:
            summands = run_knoodlesimplify_raw(raw_content, binary, level)
            ha = compute_composite_homfly(summands)
            if homfly_before != ha:
                min_level = level
                min_summands = summands
                break
        except Exception:
            continue

    # Write before/after TSV files
    before_path = FAILURES_DIR / f"failure_{fid}_before.tsv"
    after_path = FAILURES_DIR / f"failure_{fid}_after.tsv"

    with open(before_path, 'w') as f:
        f.write(summands_to_tsv(before_summands))
    with open(after_path, 'w') as f:
        f.write(summands_to_tsv(min_summands))

    # ASCII art (best-effort)
    before_art = None
    after_art = None
    if knoodledraw_binary:
        before_art = run_knoodledraw(
            summands_to_tsv(before_summands), knoodledraw_binary)
        after_art = run_knoodledraw(
            summands_to_tsv(min_summands), knoodledraw_binary)

    before_crossings = sum(len(s) for s in before_summands)
    output_crossings = sum(len(s) for s in min_summands)
    n_summands = len(min_summands)

    print(f"\n        --- Failure Diagnosis #{fid}: {name} ---")
    print(f"        Minimal failing level: {min_level} (original: {simplify_level})")
    print(f"        Files: {before_path}")
    print(f"               {after_path}")

    if before_art:
        print(f"\n        Before (projected, {before_crossings} crossings):")
        for line in before_art.splitlines():
            print(f"          {line}")

    if after_art:
        print(f"\n        After (-s={min_level}, {output_crossings} crossings,"
              f" {n_summands} summands):")
        for line in after_art.splitlines():
            print(f"          {line}")

    print(f"\n        --- End Diagnosis #{fid} ---")


# ---------------------------------------------------------------------------
# Test helpers
# ---------------------------------------------------------------------------

def test_homfly_preserved(name: str,
                          crossings: list[list[int]],
                          binary: str | Path,
                          simplify_level: int = 6,
                          expect_unknot: bool = False,
                          diagnose: bool = False,
                          knoodledraw: Path | None = None) -> TestResult:
    """Test that simplification preserves the HOMFLY polynomial for one diagram."""
    t0 = time.time()
    try:
        homfly_before = compute_homfly(crossings)
    except Exception as e:
        return TestResult(name=name, passed=False, input_crossings=len(crossings),
                          error=f"HOMFLY before failed: {e}",
                          elapsed=time.time() - t0)

    try:
        results = run_knoodlesimplify([crossings], binary, simplify_level)
    except Exception as e:
        return TestResult(name=name, passed=False, input_crossings=len(crossings),
                          error=f"knoodlesimplify failed: {e}",
                          elapsed=time.time() - t0)

    summands = results[0]

    try:
        homfly_after = compute_composite_homfly(summands)
    except Exception as e:
        return TestResult(name=name, passed=False, input_crossings=len(crossings),
                          error=f"HOMFLY after failed: {e}",
                          elapsed=time.time() - t0)

    output_crossings = sum(len(s) for s in summands)

    passed = (homfly_before == homfly_after)

    if not passed and diagnose:
        diagnose_failure(name, crossings, summands, homfly_before, homfly_after,
                         binary, simplify_level, knoodledraw)

    if expect_unknot:
        unknot_homfly = regina.Link(1).homflyAZ()
        if homfly_before != unknot_homfly:
            return TestResult(
                name=name, passed=False,
                input_crossings=len(crossings),
                output_crossings=output_crossings,
                error=f"Expected unknot HOMFLY but got: {homfly_before}",
                elapsed=time.time() - t0)

    return TestResult(
        name=name, passed=passed,
        input_crossings=len(crossings),
        output_crossings=output_crossings,
        homfly_before=str(homfly_before),
        homfly_after=str(homfly_after),
        elapsed=time.time() - t0,
    )


def test_batch_homfly_preserved(names: list[str],
                                 diagrams: list[list[list[int]]],
                                 binary: str | Path,
                                 simplify_level: int = 6,
                                 batch_size: int = 100,
                                 diagnose: bool = False,
                                 knoodledraw: Path | None = None) -> list[TestResult]:
    """Test HOMFLY preservation for a batch of diagrams, sent in groups."""
    # Pre-compute all HOMFLY-before values
    t_homfly_start = time.time()
    homfly_befores = []
    for i, (name, crossings) in enumerate(zip(names, diagrams)):
        try:
            homfly_befores.append(compute_homfly(crossings))
        except Exception as e:
            homfly_befores.append(None)
    t_homfly = time.time() - t_homfly_start

    results_out = []
    t_simplify_total = 0.0
    t_verify_total = 0.0

    for batch_start in range(0, len(diagrams), batch_size):
        batch_end = min(batch_start + batch_size, len(diagrams))
        batch_diagrams = diagrams[batch_start:batch_end]
        batch_names = names[batch_start:batch_end]
        batch_homflys = homfly_befores[batch_start:batch_end]

        t_simp = time.time()
        try:
            batch_results = run_knoodlesimplify(batch_diagrams, binary, simplify_level)
        except Exception as e:
            t_simplify_total += time.time() - t_simp
            for name in batch_names:
                results_out.append(TestResult(
                    name=name, passed=False,
                    error=f"knoodlesimplify batch failed: {e}"))
            continue
        t_simplify_total += time.time() - t_simp

        for i, (name, crossings, hb, summands) in enumerate(
                zip(batch_names, batch_diagrams, batch_homflys, batch_results)):
            if hb is None:
                results_out.append(TestResult(
                    name=name, passed=False,
                    input_crossings=len(crossings),
                    error="HOMFLY before computation failed"))
                continue

            t_ver = time.time()
            try:
                ha = compute_composite_homfly(summands)
            except Exception as e:
                t_verify_total += time.time() - t_ver
                results_out.append(TestResult(
                    name=name, passed=False,
                    input_crossings=len(crossings),
                    error=f"HOMFLY after failed: {e}"))
                continue
            t_verify_total += time.time() - t_ver

            output_crossings = sum(len(s) for s in summands)
            passed = (hb == ha)

            if not passed and diagnose:
                diagnose_failure(name, crossings, summands, hb, ha,
                                 binary, simplify_level, knoodledraw)

            results_out.append(TestResult(
                name=name, passed=passed,
                input_crossings=len(crossings),
                output_crossings=output_crossings,
                homfly_before=str(hb),
                homfly_after=str(ha),
            ))

    print(f"  Timing: HOMFLY(before) {t_homfly:.1f}s, "
          f"simplify {t_simplify_total:.1f}s, "
          f"HOMFLY(after) {t_verify_total:.1f}s")

    return results_out


# ---------------------------------------------------------------------------
# HOMFLY oracle: homfly_check (libhomfly) by default; Regina under --cross-check
# ---------------------------------------------------------------------------

def run_homfly_invariance(homfly_check_bin, paths):
    """Run `homfly_check --invariance` on file paths. Returns (results, proc),
    where results maps each path string to (status, crossings) from the
    machine-readable RESULT lines."""
    cmd = [str(homfly_check_bin), "--invariance", *[str(p) for p in paths]]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    results = {}
    for line in proc.stdout.splitlines():
        if line.startswith("RESULT\t"):
            _, status, crossings, label = line.split("\t", 3)
            results[label] = (status, int(crossings))
    return results, proc


def regina_invariance(args, regina_input) -> bool:
    """Legacy Regina oracle: simplify via the knoodlesimplify CLI, compare
    HOMFLY before/after. Returns True if preserved."""
    kind, data = regina_input
    if kind == "pd":
        before = compute_homfly(data)
        summands = run_knoodlesimplify([data], args.knoodlesimplify,
                                       args.simplify_level)[0]
        after = compute_composite_homfly(summands)
    elif kind == "embedding":
        before = compute_composite_homfly(
            run_knoodlesimplify_raw(data, args.knoodlesimplify, 0))
        after = compute_composite_homfly(
            run_knoodlesimplify_raw(data, args.knoodlesimplify, args.simplify_level))
    else:
        raise ValueError(f"unknown regina_input kind: {kind}")
    return before == after


def write_pd_tmp(crossings, tmpdir, stem) -> Path:
    """Write a 5-col PD code to a temp .tsv (for diagrams held in memory)."""
    p = Path(tmpdir) / f"{stem}.tsv"
    p.write_text(crossings_to_knoodle_tsv(crossings) + "\n")
    return p


def verify_invariance(suite, args, items, chunk: int = 400):
    """Check HOMFLY invariance for `items`, recording one TestResult each.

    items: list of (name, path, regina_input). `path` is a PD or 3D-embedding
    file fed to homfly_check --invariance. `regina_input` is ('pd', crossings),
    ('embedding', raw_content), or None — used only under --cross-check.
    """
    status_by_path = {}

    def run_chunk(chunk_items):
        results, proc = run_homfly_invariance(
            args.homfly_check, [p for (_n, p, _r) in chunk_items])
        if proc.returncode not in (0, 1):
            # A crash somewhere in the chunk: isolate it so one bad diagram
            # doesn't poison the rest. Re-run each item alone.
            if len(chunk_items) == 1:
                status_by_path[str(chunk_items[0][1])] = ("crash", 0)
            else:
                for it in chunk_items:
                    run_chunk([it])
        else:
            status_by_path.update(results)

    for i in range(0, len(items), chunk):
        run_chunk(items[i:i + chunk])

    n_skip = 0
    for (name, path, ri) in items:
        status, cr = status_by_path.get(str(path), ("missing", 0))

        if not args.cross_check:
            if status == "pass":
                suite.record(TestResult(name=name, passed=True, input_crossings=cr))
            elif status == "fail":
                suite.record(TestResult(name=name, passed=False, input_crossings=cr,
                                        error="homfly_check: HOMFLY changed"))
            elif status == "skip":
                # Split link: homfly_check can't verify it (needs the delta
                # rule). Not a failure — flag it; --cross-check verifies via Regina.
                n_skip += 1
                suite.record(TestResult(name=name, passed=True, input_crossings=cr))
            else:  # crash / missing
                suite.record(TestResult(name=name, passed=False, input_crossings=cr,
                                        error=f"homfly_check {status}"))
            continue

        # --cross-check: bring in the Regina verdict.
        try:
            reg_pass = regina_invariance(args, ri) if ri is not None else None
        except Exception as e:
            suite.record(TestResult(name=name, passed=False, input_crossings=cr,
                                    error=f"regina cross-check failed: {e}"))
            continue

        if status in ("pass", "fail"):
            hc_pass = (status == "pass")
            if reg_pass is None:
                suite.record(TestResult(name=name, passed=hc_pass, input_crossings=cr,
                    error="" if hc_pass else "homfly_check: HOMFLY changed"))
            elif hc_pass != reg_pass:
                suite.record(TestResult(name=name, passed=False, input_crossings=cr,
                    error=(f"ORACLE DISAGREEMENT: homfly_check="
                           f"{'preserved' if hc_pass else 'changed'}, "
                           f"regina={'preserved' if reg_pass else 'changed'}")))
            else:
                suite.record(TestResult(name=name, passed=hc_pass, input_crossings=cr,
                    error="" if hc_pass else "both oracles: HOMFLY changed"))
        elif status == "skip":
            # homfly_check skipped a split link — rely on Regina (which handles
            # the split-union delta rule).
            if reg_pass is None:
                n_skip += 1
                suite.record(TestResult(name=name, passed=True, input_crossings=cr))
            else:
                suite.record(TestResult(name=name, passed=reg_pass, input_crossings=cr,
                    error="" if reg_pass else "regina: HOMFLY changed (split link)"))
        else:  # crash / missing
            suite.record(TestResult(name=name, passed=False, input_crossings=cr,
                                    error=f"homfly_check {status}"))

    if n_skip:
        extra = "" if args.cross_check else " — run --cross-check to verify via Regina"
        print(f"  ({n_skip} split-link diagram(s) skipped by homfly_check{extra})")


# ---------------------------------------------------------------------------
# Tier 1a: Hardcoded known-input tests
# ---------------------------------------------------------------------------

# Standard right-hand trefoil (3_1), 0-based arcs
TREFOIL = [
    [0, 4, 1, 3, 1],
    [2, 0, 3, 5, 1],
    [4, 2, 5, 1, 1],
]

# Figure-eight knot (4_1), 0-based arcs
# 1-based PD: [[4,2,5,1],[8,6,1,5],[6,3,7,4],[2,7,3,8]], signs: +1,+1,-1,-1
FIGURE_EIGHT = [
    [3, 1, 4, 0, 1],
    [7, 5, 0, 4, 1],
    [5, 2, 6, 3, -1],
    [1, 6, 2, 7, -1],
]

# Unknot with R1 curl (1 crossing, simplifies to 0)
# 1-based PD: [[1,1,2,2]], 0-based: [0,0,1,1]
UNKNOT_R1 = [
    [0, 0, 1, 1, 1],
]


def run_tier1a(suite: TestSuite, args):
    """Tier 1a: Hardcoded small diagrams."""
    suite.section("Tier 1a: Hardcoded Diagrams")

    # Framework self-test: homfly_check's own built-in checks — encoding
    # (vs libhomfly reference values), invariance, and discrimination (distinct
    # knots get distinct HOMFLY, so the equality test isn't vacuous).
    selftest = subprocess.run([str(args.homfly_check)],
                              capture_output=True, text=True)
    suite.record(TestResult(
        name="homfly_check self-test (encoding/invariance/discrimination)",
        passed=(selftest.returncode == 0),
        error="" if selftest.returncode == 0
              else selftest.stdout.strip()[-300:] or "self-test failed"))

    with tempfile.TemporaryDirectory() as td:
        items = [
            ("Right-hand trefoil (3_1)",
             write_pd_tmp(TREFOIL, td, "trefoil"), ("pd", TREFOIL)),
            ("Figure-eight (4_1)",
             write_pd_tmp(FIGURE_EIGHT, td, "figure_eight"), ("pd", FIGURE_EIGHT)),
            ("Unknot (R1 curl, 1 crossing)",
             write_pd_tmp(UNKNOT_R1, td, "unknot_r1"), ("pd", UNKNOT_R1)),
        ]
        hopf_path = LINKTABLE_DIR / "L2a1_0.tsv"
        if hopf_path.exists():
            items.append(("Hopf link (L2a1_0)", hopf_path,
                          ("pd", read_tsv_file(hopf_path))))
        verify_invariance(suite, args, items)

    suite.end_section()


# ---------------------------------------------------------------------------
# Tier 1b: Known unknots (simplification-only, no HOMFLY on input)
# ---------------------------------------------------------------------------

# Directories whose diagrams are all known to be unknots.
# We verify knoodlesimplify reduces them to 0 crossings.
UNKNOT_DIRS = [
    ("hardunknots", PROJECT_DIR / "data" / "diagrams" / "hardunknots"),
    ("hardgoeritz", PROJECT_DIR / "data" / "diagrams" / "hardgoeritz"),
    ("randomunknots", PROJECT_DIR / "data" / "diagrams" / "randomunknots"),
    ("tsunknots", PROJECT_DIR / "data" / "diagrams" / "tsunknots"),
    ("veryhardunknots", PROJECT_DIR / "data" / "diagrams" / "veryhardunknots"),
]


def test_file_simplifies_to_unknot(name: str,
                                    file_path: Path,
                                    binary: str | Path,
                                    simplify_level: int = 6) -> TestResult:
    """Test that a known-unknot diagram simplifies to 0 crossings.

    Handles both PD code and 3D embedding file formats. Does NOT compute
    HOMFLY on the input (too expensive for large diagrams).
    """
    t0 = time.time()
    fmt = detect_file_format(file_path)
    with open(file_path) as f:
        raw_content = f.read()
    input_lines = sum(1 for line in raw_content.strip().split('\n') if line.strip())

    try:
        if fmt == "pd":
            crossings = read_tsv_file(file_path)
            if not crossings:
                return TestResult(name=name, passed=False, error="Empty PD file",
                                  elapsed=time.time() - t0)
            results = run_knoodlesimplify([crossings], binary, simplify_level)
            summands = results[0]
            n_input = len(crossings)
        else:
            # 3D embedding or unknown: pass raw content to knoodlesimplify
            summands = run_knoodlesimplify_raw(raw_content, binary, simplify_level)
            n_input = input_lines
    except Exception as e:
        return TestResult(name=name, passed=False, input_crossings=input_lines,
                          error=f"knoodlesimplify failed: {e}",
                          elapsed=time.time() - t0)

    output_crossings = sum(len(s) for s in summands)
    elapsed = time.time() - t0

    if output_crossings != 0:
        return TestResult(
            name=name, passed=False,
            input_crossings=n_input,
            output_crossings=output_crossings,
            error=f"Expected 0 crossings (unknot) but got {output_crossings}",
            elapsed=elapsed)

    return TestResult(
        name=name, passed=True,
        input_crossings=n_input,
        output_crossings=0,
        elapsed=elapsed,
    )


def run_tier1b(suite: TestSuite, binary: Path, simplify_level: int):
    """Tier 1b: Known unknots — verify simplification to 0 crossings."""
    suite.section("Tier 1b: Known Unknots")

    for dir_label, dir_path in UNKNOT_DIRS:
        if not dir_path.exists():
            print(f"  Skipping {dir_label}: directory not found")
            continue

        files = sorted(dir_path.glob("*.tsv"))
        if not files:
            print(f"  Skipping {dir_label}: no .tsv files")
            continue

        print(f"  Testing {len(files)} unknots from {dir_label}/...")

        for f in files:
            result = test_file_simplifies_to_unknot(
                f"{dir_label}/{f.stem}",
                f, binary, simplify_level)
            suite.record(result)

    suite.end_section()


# ---------------------------------------------------------------------------
# Tier 1c: Link table
# ---------------------------------------------------------------------------

def run_tier1c(suite: TestSuite, args):
    """Tier 1c: Link table from data/diagrams/linktable/."""
    suite.section("Tier 1c: Link Table")

    if not LINKTABLE_DIR.exists():
        print(f"  Skipping: {LINKTABLE_DIR} not found")
        suite.end_section()
        return

    files = sorted(LINKTABLE_DIR.glob("*.tsv"))
    if not files:
        print("  No .tsv files found")
        suite.end_section()
        return

    print(f"  Testing {len(files)} link table entries via homfly_check...")

    items = []
    for f in files:
        crossings = read_tsv_file(f)
        if crossings:
            items.append((f"{f.stem} ({len(crossings)} cr)", f, ("pd", crossings)))

    verify_invariance(suite, args, items)
    suite.end_section()


# ---------------------------------------------------------------------------
# Tier 1d: Children's game knots (3D embeddings, HOMFLY on output)
# ---------------------------------------------------------------------------

CHILDRENSGAME_DIR = PROJECT_DIR / "data" / "diagrams" / "childrensgame"


def test_embedding_homfly_preserved(name: str,
                                     raw_content: str,
                                     binary: str | Path,
                                     simplify_level: int = 6,
                                     diagnose: bool = False,
                                     knoodledraw: Path | None = None) -> TestResult:
    """Test HOMFLY invariance for a 3D embedding file.

    Uses -s=0 to get the projected PD code (HOMFLY before), then runs at the
    requested simplification level (HOMFLY after), and compares.
    """
    t0 = time.time()
    # Step 1: Get the unsimplified PD code via -s=0
    try:
        before_summands = run_knoodlesimplify_raw(raw_content, binary, simplify_level=0)
    except Exception as e:
        return TestResult(name=name, passed=False,
                          error=f"knoodlesimplify -s=0 failed: {e}",
                          elapsed=time.time() - t0)

    before_crossings = sum(len(s) for s in before_summands)

    try:
        homfly_before = compute_composite_homfly(before_summands)
    except Exception as e:
        return TestResult(name=name, passed=False,
                          input_crossings=before_crossings,
                          error=f"HOMFLY before failed: {e}",
                          elapsed=time.time() - t0)

    # Step 2: Get the simplified PD code
    try:
        after_summands = run_knoodlesimplify_raw(raw_content, binary, simplify_level)
    except Exception as e:
        return TestResult(name=name, passed=False,
                          input_crossings=before_crossings,
                          error=f"knoodlesimplify -s={simplify_level} failed: {e}",
                          elapsed=time.time() - t0)

    after_crossings = sum(len(s) for s in after_summands)

    try:
        homfly_after = compute_composite_homfly(after_summands)
    except Exception as e:
        return TestResult(name=name, passed=False,
                          input_crossings=before_crossings,
                          output_crossings=after_crossings,
                          error=f"HOMFLY after failed: {e}",
                          elapsed=time.time() - t0)

    passed = (homfly_before == homfly_after)

    if not passed and diagnose:
        diagnose_embedding_failure(name, raw_content, before_summands, after_summands,
                                    homfly_before, homfly_after,
                                    binary, simplify_level, knoodledraw)

    return TestResult(
        name=name, passed=passed,
        input_crossings=before_crossings,
        output_crossings=after_crossings,
        homfly_before=str(homfly_before),
        homfly_after=str(homfly_after),
        elapsed=time.time() - t0,
    )


def run_tier1d(suite: TestSuite, args):
    """Tier 1d: Children's game knots — 3D embeddings, HOMFLY invariance.

    homfly_check reads the 3-column embedding directly (via FromKnotEmbedding),
    projects it, simplifies, and compares HOMFLY before/after.
    """
    suite.section("Tier 1d: Children's Game Knots")

    if not CHILDRENSGAME_DIR.exists():
        print(f"  Skipping: {CHILDRENSGAME_DIR} not found")
        suite.end_section()
        return

    files = sorted(CHILDRENSGAME_DIR.glob("*.tsv"))
    if not files:
        print("  No .tsv files found")
        suite.end_section()
        return

    print(f"  Testing {len(files)} children's game knots via homfly_check...")

    items = [(f.stem, f, ("embedding", f.read_text())) for f in files]
    verify_invariance(suite, args, items)
    suite.end_section()


# ---------------------------------------------------------------------------
# Tier 2: Plantri-generated tests
# ---------------------------------------------------------------------------

def find_plantri(script_dir: Path) -> Path | None:
    """Locate the plantri binary."""
    candidate = script_dir / "plantri58" / "plantri"
    if candidate.exists() and os.access(candidate, os.X_OK):
        return candidate
    return None


def parse_plantri_edge_codes(raw: bytes) -> list[list[list[int]]]:
    """Parse plantri binary edge code output (-E flag).

    Returns list of graphs. Each graph is a list of vertex neighborhoods
    (CW-ordered lists of edge indices).
    """
    HEADER = b">>edge_code<<"
    if not raw.startswith(HEADER):
        raise ValueError("Missing plantri edge code header")

    pos = len(HEADER)
    graphs = []

    while pos < len(raw):
        body_size = raw[pos]
        pos += 1

        if body_size == 0:
            # Extended header for large graphs: next 4 bytes are little-endian size
            if pos + 4 > len(raw):
                break
            body_size = struct.unpack_from('<I', raw, pos)[0]
            pos += 4

        if pos + body_size > len(raw):
            break

        body = raw[pos:pos + body_size]
        pos += body_size

        # Parse vertex sections separated by 0xFF
        vertices = []
        current = []
        for b in body:
            if b == 0xFF:
                if current:
                    vertices.append(current)
                current = []
            else:
                current.append(b)
        if current:
            vertices.append(current)

        graphs.append(vertices)

    return graphs


def quad_to_shadow(vertices: list[list[int]]) -> tuple[
        list[tuple[int, int, int, int]], dict[int, int]]:
    """Convert quadrangulation rotation system to knot diagram shadow.

    The dual of a quadrangulation is a 4-valent graph (knot shadow).

    Args:
        vertices: CW-ordered edge-index lists per vertex (from plantri).

    Returns:
        (crossings, partner): where crossings[i] = (h0, h1, h2, h3) are
        half-edge IDs in CW order, and partner[h] gives the partner half-edge.
    """
    n_vertices = len(vertices)

    # Build edge endpoint mapping
    # Each edge index appears twice; find the two (vertex, position) pairs
    edge_to_darts: dict[int, list[tuple[int, int]]] = {}
    for v, neighborhood in enumerate(vertices):
        for pos, e in enumerate(neighborhood):
            edge_to_darts.setdefault(e, []).append((v, pos))

    # Find faces using dart traversal
    # A dart is (vertex, position_in_cw_neighborhood)
    # From dart (v, p), the edge is vertices[v][p], leading to some other vertex w.
    # At w, that edge appears at some position q. The next dart in the face
    # traversal is (w, (q-1) mod deg(w)) — the CW predecessor at w.

    # Build dart-to-next mapping
    dart_next: dict[tuple[int, int], tuple[int, int]] = {}
    for e, darts in edge_to_darts.items():
        if len(darts) != 2:
            raise ValueError(f"Edge {e} has {len(darts)} endpoints, expected 2")
        (v0, p0), (v1, p1) = darts
        # Dart (v0, p0) goes from v0 to v1 via edge e.
        # At v1, edge e is at position p1. Next dart is (v1, (p1-1) mod deg)
        deg1 = len(vertices[v1])
        dart_next[(v0, p0)] = (v1, (p1 - 1) % deg1)
        deg0 = len(vertices[v0])
        dart_next[(v1, p1)] = (v0, (p0 - 1) % deg0)

    # Trace faces
    visited_darts: set[tuple[int, int]] = set()
    faces: list[list[tuple[int, int]]] = []

    for v in range(n_vertices):
        for p in range(len(vertices[v])):
            dart = (v, p)
            if dart in visited_darts:
                continue
            face = []
            d = dart
            while d not in visited_darts:
                visited_darts.add(d)
                face.append(d)
                d = dart_next[d]
            if len(face) != 4:
                raise ValueError(
                    f"Found face with {len(face)} edges, expected 4 (quadrangulation)")
            faces.append(face)

    n_crossings = len(faces)

    # Build dual graph
    # Each face becomes a crossing vertex.
    # Half-edges: face f, side i (0-3) → half-edge ID = 4*f + i
    # The primal edge at face[f][i] is vertices[face[f][i][0]][face[f][i][1]].
    # Partner: the half-edge in the OTHER face that shares the same primal edge.

    # Map primal edge → half-edges
    edge_to_halfedges: dict[int, list[int]] = {}
    crossings = []
    for f, face in enumerate(faces):
        he_tuple = tuple(4 * f + i for i in range(4))
        crossings.append(he_tuple)
        for i, (v, p) in enumerate(face):
            primal_edge = vertices[v][p]
            edge_to_halfedges.setdefault(primal_edge, []).append(4 * f + i)

    partner: dict[int, int] = {}
    for e, hes in edge_to_halfedges.items():
        if len(hes) != 2:
            raise ValueError(
                f"Primal edge {e} borders {len(hes)} faces, expected 2")
        partner[hes[0]] = hes[1]
        partner[hes[1]] = hes[0]

    return crossings, partner


def assign_crossings_and_trace(
        crossings: list[tuple[int, int, int, int]],
        partner: dict[int, int],
        mask: int) -> tuple[list[list[int]], int]:
    """Assign crossing types and trace arcs to build PD code.

    The algorithm:
    1. Trace link components through the shadow, assigning consecutive arc
       numbers along each strand. This ensures Knoodle's PD code constraint
       X[2] = X[0]+1 (mod component length) is satisfied.
    2. Record each crossing position's arc number and whether it's an arrival
       or departure for the strand passing through it.
    3. Determine the PD code sign from the embedding geometry (relative CW
       positions of the two strands' arrivals), not from the bitmask alone.

    Args:
        crossings: CW half-edge tuples per crossing.
        partner: Half-edge partner mapping.
        mask: Bitmask for crossing assignments (bit i → crossing i type).

    Returns:
        (pd_rows, n_components): 5-col PD code rows and component count.
    """
    n = len(crossings)

    # Build traversal: at crossing c, entering on half-edge at position p,
    # exit on position (p+2) mod 4, then follow partner.
    he_to_crossing: dict[int, tuple[int, int]] = {}  # halfedge → (crossing_idx, position)
    for c, (h0, h1, h2, h3) in enumerate(crossings):
        for p, h in enumerate((h0, h1, h2, h3)):
            he_to_crossing[h] = (c, p)

    def next_halfedge(h: int) -> int:
        c, p = he_to_crossing[h]
        exit_pos = (p + 2) % 4
        exit_he = crossings[c][exit_pos]
        return partner[exit_he]

    all_halfedges = set()
    for c, (h0, h1, h2, h3) in enumerate(crossings):
        all_halfedges.update([h0, h1, h2, h3])

    # Trace strands, assign consecutive arc numbers, track arrival positions.
    # Each strand visits a sequence of crossings. Between consecutive crossings
    # is one arc. Arc i connects the exit of path[i]'s crossing to the arrival
    # of path[(i+1)%k]'s crossing.
    #
    # At crossing c_i (visited at index i in the strand):
    #   arriving arc = base + (i-1) % k
    #   departing arc = base + i
    # This ensures departing = arriving + 1 (with wrap-around at component end).
    arc_at: dict[tuple[int, int], int] = {}     # (crossing, position) → arc number
    is_arrival: dict[tuple[int, int], bool] = {}  # (crossing, position) → True if arrival

    visited: set[int] = set()
    arc_count = 0
    n_components = 0

    for start_he in sorted(all_halfedges):
        if start_he in visited:
            continue
        n_components += 1

        # Collect the strand path (one direction only: mark partner as visited)
        path = []
        h = start_he
        while h not in visited:
            visited.add(h)
            visited.add(partner[h])
            path.append(h)
            h = next_halfedge(h)

        k = len(path)
        base = arc_count

        for i, h in enumerate(path):
            c_idx, p = he_to_crossing[h]
            exit_pos = (p + 2) % 4

            # Arriving arc at this position
            arc_at[(c_idx, p)] = base + ((i - 1) % k)
            is_arrival[(c_idx, p)] = True

            # Departing arc at the exit position
            arc_at[(c_idx, exit_pos)] = base + i
            is_arrival[(c_idx, exit_pos)] = False

        arc_count += k

    # Build PD code.
    # For each crossing, determine which positions are arrivals for each strand
    # pair, then construct X in CCW order starting from the under-strand arrival.
    # The sign (right/left-handed) is determined by geometry: whether the
    # over-strand arrives CW-next (+1) or CW-prev (-1) from the under arrival.
    pd_rows = []
    for c in range(n):
        # Strand A uses positions 0,2; Strand B uses positions 1,3
        p_a = 0 if is_arrival.get((c, 0)) else 2  # strand A arrival position
        p_b = 1 if is_arrival.get((c, 1)) else 3  # strand B arrival position

        bit = (mask >> c) & 1
        if bit == 0:
            p_u, p_o = p_a, p_b  # A is under, B is over
        else:
            p_u, p_o = p_b, p_a  # B is under, A is over

        # Sign from geometry: over arrives CW-next → right-handed
        if p_o == (p_u + 1) % 4:
            sign = 1
        else:
            sign = -1

        # CCW order from under arrival: p_u, (p_u+3)%4, (p_u+2)%4, (p_u+1)%4
        ccw = [p_u, (p_u + 3) % 4, (p_u + 2) % 4, (p_u + 1) % 4]
        X = [arc_at[(c, q)] for q in ccw]
        pd_rows.append([X[0], X[1], X[2], X[3], sign])

    return pd_rows, n_components


def plantri_vertex_count(n_crossings: int) -> int:
    """Crossing count → plantri vertex count (V = n + 2)."""
    return n_crossings + 2


def generate_plantri_diagrams(plantri_path: Path,
                               max_crossings: int,
                               crossing_assignments: str,
                               plantri_mode: str = "simple",
                               knots_only: bool = False,
                               seed: int = 42) -> Iterator[tuple[str, list[list[int]]]]:
    """Generate all test diagrams from plantri.

    Yields (name, pd_rows) pairs one at a time (generator, not list).
    """
    rng = random.Random(seed)

    # Determine vertex counts to test
    if plantri_mode == "simple":
        # Simple mode: V must be even, min 8
        v_values = [v for v in range(8, max_crossings + 3)
                    if v % 2 == 0 and v - 2 <= max_crossings]
    else:
        v_values = [v for v in range(4, max_crossings + 3)
                    if v - 2 <= max_crossings]

    for v in v_values:
        n_crossings = v - 2

        # Build plantri command
        if plantri_mode == "simple":
            cmd = [str(plantri_path), "-q", "-E", str(v)]
        elif plantri_mode == "no-r1":
            cmd = [str(plantri_path), "-Q", "-c2", "-E", str(v)]
        else:  # everything
            cmd = [str(plantri_path), "-Q", "-E", str(v)]

        try:
            proc = subprocess.run(cmd, capture_output=True, timeout=60)
        except subprocess.TimeoutExpired:
            print(f"  Warning: plantri timed out for V={v}")
            continue

        if proc.returncode != 0:
            print(f"  Warning: plantri failed for V={v}: {proc.stderr[:200]}")
            continue

        graphs = parse_plantri_edge_codes(proc.stdout)
        print(f"  V={v} ({n_crossings} crossings): {len(graphs)} graphs")

        for g_idx, graph in enumerate(graphs):
            try:
                shadow_crossings, partner_map = quad_to_shadow(graph)
            except ValueError as e:
                print(f"  Warning: skipping graph {g_idx}: {e}")
                continue

            # Determine which masks to test
            total_masks = 1 << n_crossings
            if crossing_assignments == "all":
                masks = range(total_masks)
            else:
                k = int(crossing_assignments)
                if k >= total_masks:
                    masks = range(total_masks)
                else:
                    masks = rng.sample(range(total_masks), k)

            for mask in masks:
                try:
                    pd_rows, n_components = assign_crossings_and_trace(
                        shadow_crossings, partner_map, mask)
                except ValueError as e:
                    print(f"  Warning: trace failed g={g_idx} m={mask}: {e}")
                    continue

                if knots_only and n_components > 1:
                    continue

                name = (f"plantri_V{v}_g{g_idx}_m{mask}"
                        f"_{n_components}comp")
                yield (name, pd_rows)


def run_tier2(suite: TestSuite, args,
              max_crossings: int, crossing_assignments: str,
              plantri_mode: str, knots_only: bool, seed: int,
              batch_size: int = 400):
    """Tier 2: Plantri-generated exhaustive tests (streaming via homfly_check)."""
    suite.section("Tier 2: Plantri-Generated Tests")

    plantri_path = find_plantri(SCRIPT_DIR)
    if plantri_path is None:
        print("  Skipping: plantri not found. Run setup.sh --with-plantri")
        suite.end_section()
        return

    print(f"  Generating and testing diagrams up to {max_crossings} crossings (batched)...")
    print(f"  Crossing assignments: {crossing_assignments}")
    print(f"  Plantri mode: {plantri_mode}")

    total = 0
    with tempfile.TemporaryDirectory() as td:
        batch: list = []

        def flush():
            if batch:
                verify_invariance(suite, args, batch)
                batch.clear()

        for name, pd_rows in generate_plantri_diagrams(
                plantri_path, max_crossings, crossing_assignments,
                plantri_mode, knots_only, seed):
            # Reuse a small set of temp filenames per batch (each batch is fully
            # processed before the next overwrites them).
            p = Path(td) / f"d{len(batch)}.tsv"
            p.write_text(crossings_to_knoodle_tsv(pd_rows) + "\n")
            batch.append((name, p, ("pd", pd_rows)))
            total += 1
            if len(batch) >= batch_size:
                flush()
        flush()

    if total == 0:
        print("  No diagrams generated")
    else:
        print(f"\n  Tested {total} diagrams total")

    suite.end_section()


# ---------------------------------------------------------------------------
# CLI and main
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="End-to-end HOMFLY invariance tests for knoodlesimplify")

    # Tier selection
    tier_group = parser.add_mutually_exclusive_group()
    tier_group.add_argument("--smoke-only", action="store_true",
                            help="Run only Tier 1a hardcoded tests")
    tier_group.add_argument("--hard-unknots-only", action="store_true",
                            help="Run only Tier 1b hard unknots")
    tier_group.add_argument("--linktable-only", action="store_true",
                            help="Run only Tier 1c link table")
    tier_group.add_argument("--childrensgame-only", action="store_true",
                            help="Run only Tier 1d children's game knots")
    tier_group.add_argument("--plantri-only", action="store_true",
                            help="Run only Tier 2 plantri tests")

    # Simplification
    parser.add_argument("--simplify-level", type=int, default=6,
                        help="Simplification level 1-6 (default: 6)")

    # Plantri options
    parser.add_argument("--up-to-crossing-number", type=int, default=0,
                        help="Max crossing number for plantri tier")
    parser.add_argument("--crossing-assignments", type=str, default="all",
                        help="'all' or integer K for random sample")
    parser.add_argument("--plantri-mode", type=str, default="simple",
                        choices=["simple", "no-r1", "everything"],
                        help="Plantri quadrangulation mode")
    parser.add_argument("--knots-only", action="store_true",
                        help="Skip multi-component diagrams in plantri tier")
    parser.add_argument("--seed", type=int, default=42,
                        help="RNG seed for random sampling")

    # Oracles / binaries
    parser.add_argument("--homfly-check", type=str, default=str(DEFAULT_HOMFLY_CHECK),
                        help="Path to the homfly_check binary (default: test/homfly_check)")
    parser.add_argument("--knoodlesimplify", type=str, default=str(DEFAULT_BINARY),
                        help="Path to knoodlesimplify (tier 1b and --cross-check)")
    parser.add_argument("--cross-check", action="store_true",
                        help="Also run the Regina oracle and require both to "
                             "agree (needs regina + knoodlesimplify)")

    # General
    parser.add_argument("--verbose", action="store_true",
                        help="Print details for passing tests")
    parser.add_argument("--stop-on-failure", action="store_true",
                        help="Stop at first failure")
    parser.add_argument("--no-diagnose", action="store_true",
                        help="(Accepted for compatibility; on failure homfly_check "
                             "prints the before/after HOMFLY polynomials to stderr.)")

    return parser.parse_args()


def main():
    args = parse_args()

    homfly_check = Path(args.homfly_check)
    knoodlesimplify = Path(args.knoodlesimplify)

    is_default = not (args.smoke_only or args.hard_unknots_only or
                      args.linktable_only or args.childrensgame_only or
                      args.plantri_only)
    runs_1b = args.hard_unknots_only or is_default
    runs_homfly_tier = not args.hard_unknots_only  # 1a/1c/1d/2 all use homfly_check

    # homfly_check is the oracle for the HOMFLY-invariance tiers.
    if runs_homfly_tier and (not homfly_check.exists()
                             or not os.access(homfly_check, os.X_OK)):
        print(f"Error: homfly_check not found at {homfly_check}", file=sys.stderr)
        print("Build it with: make -C test homfly_check", file=sys.stderr)
        sys.exit(2)

    # knoodlesimplify is needed only for tier 1b and for --cross-check.
    if (runs_1b or args.cross_check) and (not knoodlesimplify.exists()
                                          or not os.access(knoodlesimplify, os.X_OK)):
        print(f"Error: knoodlesimplify not found at {knoodlesimplify}", file=sys.stderr)
        print("Build it with: make -C tools", file=sys.stderr)
        sys.exit(2)

    if args.cross_check and not REGINA_AVAILABLE:
        print("Error: --cross-check needs regina. "
              "Run: source venv/bin/activate && pip install regina", file=sys.stderr)
        sys.exit(2)

    suite = TestSuite(verbose=args.verbose, stop_on_failure=args.stop_on_failure)

    start_time = time.time()

    if args.smoke_only:
        run_tier1a(suite, args)
    elif args.hard_unknots_only:
        run_tier1b(suite, knoodlesimplify, args.simplify_level)
    elif args.linktable_only:
        run_tier1c(suite, args)
    elif args.childrensgame_only:
        run_tier1d(suite, args)
    elif args.plantri_only:
        if args.up_to_crossing_number <= 0:
            print("Error: --up-to-crossing-number required for plantri tests",
                  file=sys.stderr)
            sys.exit(2)
        run_tier2(suite, args, args.up_to_crossing_number,
                  args.crossing_assignments, args.plantri_mode,
                  args.knots_only, args.seed)
    else:
        # Default: all Tier 1 tests
        run_tier1a(suite, args)
        run_tier1b(suite, knoodlesimplify, args.simplify_level)
        run_tier1c(suite, args)
        run_tier1d(suite, args)

        # Tier 2 if plantri args given
        if args.up_to_crossing_number > 0:
            run_tier2(suite, args, args.up_to_crossing_number,
                      args.crossing_assignments, args.plantri_mode,
                      args.knots_only, args.seed)

    elapsed = time.time() - start_time
    all_passed = suite.summary()
    print(f"\nElapsed: {elapsed:.1f}s")

    sys.exit(0 if all_passed else 1)


if __name__ == "__main__":
    main()
