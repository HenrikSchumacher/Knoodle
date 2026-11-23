#!/usr/bin/env python3
"""run_tests_plantri_check -- run_tests.py's Tier 2 finds plantri, or fails.

Tier 2 of run_tests.py tests every diagram plantri generates up to
--up-to-crossing-number. Until 2026-11-22 it looked for plantri only at
test/plantri58/plantri, where setup.sh --with-plantri downloaded it, and not
at the vendored test/vendor/plantri/plantri that `make -C test` builds. On a
checkout where setup.sh was never run it printed "Skipping: plantri not
found", reported "Tier 2: 0/0 passed" and exited 0. The generator was no
stricter: plantri failing for some V, or a graph that would not trace, was a
printed warning, and the tier passed having tested less.

This checks the fixes:
  * find_plantri prefers the vendored binary, falls back to plantri58/, and
    takes an explicit --plantri path as the only candidate;
  * asking for Tier 2 with no plantri is an error naming the build command,
    whether by --plantri-only or a default run, and so is asking for it where
    it does not run (--smoke-only --up-to-crossing-number=8);
  * everything that keeps diagrams from being generated is a failure: plantri
    exiting nonzero, output that is not an edge code or is truncated, a
    graph count that differs from PLANTRI_GRAPH_COUNTS, a graph or crossing
    assignment that does not trace, and no diagrams at all;
  * with the vendored plantri, the default Tier 2 up to 8 crossings tests
    exactly 320 diagrams, the number docs/homfly-oracle.md records.

homfly_check is stubbed out (every diagram passes), so what is under test is
how Tier 2 finds plantri and counts its diagrams, not the oracle. The last
section runs the vendored plantri, which every tier builds for plantri_check
(hence `needs homfly` in the manifest row).

Run: python3 run_tests_plantri_check.py
"""

from __future__ import annotations

import contextlib
import io
import os
import subprocess
import sys
import tempfile
import types
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import run_tests as rt  # noqa: E402

VENDORED = HERE / "vendor" / "plantri" / "plantri"

checks = 0
fails = 0


def check(ok, what, detail=""):
    global checks, fails
    checks += 1
    if ok:
        return
    print(f"  FAILED  {what}")
    if detail:
        for line in str(detail).rstrip().splitlines()[:8]:
            print(f"          | {line}")
    fails += 1


def section(s):
    print(f"\n=== {s} ===")


def make_exe(path: Path, text: str = "#!/bin/sh\nexit 0\n", mode: int = 0o755):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(mode)
    return path


@contextlib.contextmanager
def patched(obj, name, value):
    old = getattr(obj, name)
    setattr(obj, name, value)
    try:
        yield
    finally:
        setattr(obj, name, old)


def stub_verify_invariance(suite, args, items, chunk=400):
    """homfly_check stand-in: every diagram passes."""
    for name, _path, _data in items:
        suite.record(rt.TestResult(name=name, passed=True))


def run_tier2(plantri, max_crossings=8, assignments="all", mode="simple",
              knots_only=False):
    """Run rt.run_tier2 with the oracle stubbed. Returns (results, stdout)."""
    suite = rt.TestSuite()
    out = io.StringIO()
    with patched(rt, "verify_invariance", stub_verify_invariance), \
            contextlib.redirect_stdout(out):
        rt.run_tier2(suite, types.SimpleNamespace(), Path(plantri),
                     max_crossings, assignments, mode, knots_only, 42)
    return suite.results, out.getvalue()


def failures(results):
    return [r for r in results if not r.passed]


def failed_with(results, text):
    return any(text in r.error for r in failures(results))


def run_main(argv, script_dir):
    """rt.main() with test/ moved to `script_dir`. Returns (exit code,
    stdout, stderr). main() must stop before any tier runs: any tier would
    need binaries this check does not have."""
    out, err = io.StringIO(), io.StringIO()
    code = None
    with patched(rt, "SCRIPT_DIR", Path(script_dir)), \
            patched(sys, "argv", ["run_tests.py", *argv]), \
            contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        try:
            rt.main()
        except SystemExit as e:
            code = e.code
    return code, out.getvalue(), err.getvalue()


# ---------------------------------------------------------------------------

def check_find_plantri(tmp: Path):
    section("find_plantri: vendored first, then plantri58/, or --plantri alone")
    d = tmp / "find"
    vendored = d / "vendor" / "plantri" / "plantri"
    setup = d / "plantri58" / "plantri"

    d.mkdir()
    check(rt.find_plantri(d) is None, "neither binary: None",
          rt.find_plantri(d))
    make_exe(setup)
    check(rt.find_plantri(d) == setup, "only plantri58/: finds it",
          rt.find_plantri(d))
    make_exe(vendored)
    check(rt.find_plantri(d) == vendored, "both: the vendored binary wins",
          rt.find_plantri(d))
    vendored.chmod(0o644)
    check(rt.find_plantri(d) == setup,
          "vendored not executable: falls back to plantri58/", rt.find_plantri(d))
    vendored.chmod(0o755)
    setup.unlink()
    check(rt.find_plantri(d) == vendored, "only vendored: finds it",
          rt.find_plantri(d))

    other = make_exe(tmp / "elsewhere" / "plantri")
    check(rt.find_plantri(d, str(other)) == other, "--plantri PATH: that path",
          rt.find_plantri(d, str(other)))
    missing = tmp / "elsewhere" / "nope"
    check(rt.find_plantri(d, str(missing)) is None,
          "--plantri to a missing file: None, not the vendored binary",
          rt.find_plantri(d, str(missing)))

    dir_only = tmp / "dironly"
    (dir_only / "vendor" / "plantri" / "plantri").mkdir(parents=True)
    check(rt.find_plantri(dir_only) is None,
          "a directory named plantri is not a binary", rt.find_plantri(dir_only))


def check_main_refuses(tmp: Path):
    section("main: Tier 2 asked for with no plantri is an error, never 0/0")
    empty = tmp / "empty-test-dir"
    empty.mkdir()

    for argv, what in [
            (["--plantri-only", "--up-to-crossing-number=8"], "--plantri-only"),
            (["--up-to-crossing-number=8"], "a default run with Tier 2")]:
        code, out, err = run_main(argv, empty)
        check(code == 2, f"{what}, no plantri: exit 2", f"exit {code}\n{err}")
        check("plantri not found" in err and rt.PLANTRI_BUILD in err,
              f"{what}, no plantri: says so and how to build it", err)
        check("0/0" not in out and "Tier 2" not in out,
              f"{what}, no plantri: stops before any tier runs", out)

    code, out, err = run_main(["--plantri-only", "--up-to-crossing-number=8",
                               f"--plantri={tmp / 'no-such-plantri'}"], empty)
    check(code == 2 and "no-such-plantri" in err,
          "--plantri to a missing file: exit 2, naming it", f"exit {code}\n{err}")
    check("Build it with" not in err,
          "--plantri to a missing file: no build hint for a path we didn't pick",
          err)

    # A plantri is there, so the only thing wrong is asking for Tier 2 where
    # it does not run, and that alone must stop the run (one Error line).
    has_plantri = tmp / "has-plantri"
    make_exe(has_plantri / "vendor" / "plantri" / "plantri")
    code, out, err = run_main(["--smoke-only", "--up-to-crossing-number=8"],
                              has_plantri)
    check(code == 2 and "--up-to-crossing-number" in err
          and err.count("Error:") == 1,
          "--smoke-only --up-to-crossing-number: exit 2 at once, not a quiet "
          "no-op", f"exit {code}\n{err}")

    code, out, err = run_main(["--plantri-only"], empty)
    check(code == 2 and "--up-to-crossing-number required" in err,
          "--plantri-only alone: exit 2", f"exit {code}\n{err}")


def check_generator_failures(tmp: Path):
    section("Tier 2: whatever keeps diagrams from being generated fails")

    exits = make_exe(tmp / "fake" / "exits1",
                     "#!/bin/sh\necho 'plantri: out of luck' >&2\nexit 1\n")
    results, out = run_tier2(exits)
    check(failed_with(results, "exited 1") and failed_with(results,
                                                           "out of luck"),
          "plantri exits 1: a failure quoting its stderr", out)
    check(failed_with(results, "no diagrams generated"),
          "plantri exits 1 for every V: 'no diagrams generated' fails", out)

    junk = make_exe(tmp / "fake" / "junk", "#!/bin/sh\necho hello\n")
    results, out = run_tier2(junk)
    check(failed_with(results, "Missing plantri edge code header"),
          "output that is not an edge code: a failure", out)

    header_only = make_exe(tmp / "fake" / "header",
                           "#!/bin/sh\nprintf '>>edge_code<<'\n")
    results, out = run_tier2(header_only)
    check(failed_with(results, "gave 0 graphs; plantri 5.8 gives 1"),
          "zero graphs where plantri 5.8 gives one: a failure", out)

    truncated = make_exe(tmp / "fake" / "truncated",
                         "#!/bin/sh\nprintf '>>edge_code<<\\050\\001\\002'\n")
    results, out = run_tier2(truncated)
    check(failed_with(results, "truncated"),
          "a truncated edge code: a failure, not a short graph list", out)

    results, out = run_tier2(exits, max_crossings=5)
    check(failed_with(results, "simple mode starts at 6 crossings"),
          "simple mode below 6 crossings: fails, saying why", out)


def check_vendored(tmp: Path):
    section("Tier 2 with the vendored plantri")
    if not (VENDORED.is_file() and os.access(VENDORED, os.X_OK)):
        check(False, f"vendored plantri is built: {rt.display_plantri(VENDORED)}",
              f"build it with: {rt.PLANTRI_BUILD}")
        return

    results, out = run_tier2(VENDORED)
    check(len(results) == 320 and not failures(results),
          "default Tier 2 to 8 crossings: 320 diagrams, all recorded",
          f"{len(results)} results, {len(failures(results))} failed\n{out}")
    check("(of 320 expected)" in out, "says how many it expected", out)
    check(rt.expected_tier2_total(8, "all", "simple", False) == 320,
          "expected_tier2_total(8, all, simple) == 320",
          rt.expected_tier2_total(8, "all", "simple", False))

    results, out = run_tier2(VENDORED, max_crossings=6, mode="no-r1")
    want = rt.expected_tier2_total(6, "all", "no-r1", False)
    check(len(results) == want and not failures(results),
          f"no-r1 to 6 crossings: {want} diagrams",
          f"{len(results)} results\n{out}")

    results, out = run_tier2(VENDORED, max_crossings=5, assignments="3",
                             mode="everything")
    want = rt.expected_tier2_total(5, "3", "everything", False)
    check(len(results) == want and not failures(results),
          f"everything to 5 crossings, 3 assignments each: {want} diagrams",
          f"{len(results)} results\n{out}")

    # The graph counts are the vendored plantri's own: ask it with -u.
    for mode, flags in rt.PLANTRI_FLAGS.items():
        for v, n in sorted(rt.PLANTRI_GRAPH_COUNTS[mode].items()):
            if v > 10:   # keep this check fast; the rest were counted the same way
                break
            proc = subprocess.run([str(VENDORED), *flags, "-u", str(v)],
                                  capture_output=True, text=True)
            said = (proc.stderr.strip().splitlines() or [""])[-1]
            check(said.startswith(f"{n} "),
                  f"PLANTRI_GRAPH_COUNTS[{mode}][{v}] == {n} matches plantri -u",
                  said)

    wrong = {m: dict(c) for m, c in rt.PLANTRI_GRAPH_COUNTS.items()}
    wrong["simple"][10] = 2
    with patched(rt, "PLANTRI_GRAPH_COUNTS", wrong):
        results, out = run_tier2(VENDORED)
    check(failed_with(results, "gave 1 graphs; plantri 5.8 gives 2"),
          "a graph count other than the table's: a failure", out)
    check(failed_with(results, "tested 320 diagrams; there are 576"),
          "a diagram total other than the table's: a failure", out)

    def no_shadow(graph):
        raise ValueError("stub refuses this graph")
    with patched(rt, "quad_to_shadow", no_shadow):
        results, out = run_tier2(VENDORED)
    check(len(failures(results)) == 3 and failed_with(results, "not a knot shadow"),
          "graphs that do not trace: one failure each, plus the total",
          f"{[r.error for r in failures(results)]}\n{out}")

    real_trace = rt.assign_crossings_and_trace

    def trace_fails_mask_5(shadow, partner, mask):
        if mask == 5:
            raise ValueError("stub refuses mask 5")
        return real_trace(shadow, partner, mask)
    with patched(rt, "assign_crossings_and_trace", trace_fails_mask_5):
        results, out = run_tier2(VENDORED)
    check(failed_with(results, "trace failed") and
          failed_with(results, "tested 318 diagrams; there are 320"),
          "an assignment that does not trace: a failure, and the total is short",
          f"{[r.error for r in failures(results)]}\n{out}")

    results, out = run_tier2(VENDORED, knots_only=True)
    check(results and not failures(results) and "expected" not in out,
          "--knots-only: no expected total claimed", out)


def main():
    with tempfile.TemporaryDirectory(prefix="run_tests_plantri_check.") as td:
        tmp = Path(td)
        check_find_plantri(tmp)
        check_main_refuses(tmp)
        check_generator_failures(tmp)
        check_vendored(tmp)

    print(f"\nrun_tests_plantri_check: ({checks} checks, {fails} failed)")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
