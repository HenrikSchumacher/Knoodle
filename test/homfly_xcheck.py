#!/usr/bin/env python3
"""
Cross-validate the libhomfly oracle against Regina.

The new C++ correctness test (homfly_check) uses a vendored, dependency-free
libhomfly as its HOMFLY oracle, fed by Knoodle's own Jenkins code. This script
proves that pipeline agrees *exactly* with Regina before we rely on it: for a
panel of diagrams it compares

    Knoodle PD --[ToJenkinsCodeString]--> libhomfly  --(L,M) terms-->
    Knoodle PD --[crossings_to_regina_pd]--> Regina.homflyLM(l,m)

libhomfly's (L, M) variables are term-for-term identical to Regina's
homflyLM(l, m) (no substitution), so equality is checked directly by
reconstructing libhomfly's result as a regina.Laurent2 and comparing.

This is the *optional* Regina cross-check (it needs Python+Regina). The
production test, homfly_check, needs neither.

Usage:
    venv/bin/python homfly_xcheck.py [--samples N]
"""

import argparse
import pathlib
import subprocess
import sys

import regina

HERE = pathlib.Path(__file__).resolve().parent
HOMFLY_CHECK = HERE / "homfly_check"
LINKTABLE_DIR = HERE.parent / "data" / "diagrams" / "linktable"

# Hardcoded small knots (0-based [X0,X1,X2,X3,sign]), from run_tests.py.
# (The trefoil is chiral — its non-palindromic HOMFLY exercises chirality
# agreement; the figure-eight is amphichiral. A "mirror" cannot be made by
# flipping the sign column alone, so it is not in the panel.)
TREFOIL = [[0, 4, 1, 3, 1], [2, 0, 3, 5, 1], [4, 2, 5, 1, 1]]
FIGURE_EIGHT = [[3, 1, 4, 0, 1], [7, 5, 0, 4, 1], [5, 2, 6, 3, -1], [1, 6, 2, 7, -1]]


def disjoint_union(*diagrams):
    """Combine PD diagrams into a SPLIT (disjoint) PD by shifting arc labels so
    the pieces share no arcs. Exercises homfly_check's split-union delta rule."""
    result, shift = [], 0
    for d in diagrams:
        hi = max(max(r[:4]) for r in d)
        result += [[r[0] + shift, r[1] + shift, r[2] + shift, r[3] + shift, r[4]] for r in d]
        shift += hi + 1
    return result


def regina_pd(crossings):
    """0-based Knoodle crossings -> Regina 1-based PD string (cols 0-3)."""
    rows = [f"[{r[0] + 1},{r[1] + 1},{r[2] + 1},{r[3] + 1}]" for r in crossings]
    return "[" + ",".join(rows) + "]"


def regina_homflyLM(crossings):
    return regina.Link.fromPD(regina_pd(crossings)).homflyLM()


def libhomfly_laurent2(crossings):
    """Run homfly_check --emit-poly and rebuild the result as a Laurent2."""
    flat = " ".join(str(x) for row in crossings for x in row[:5])
    proc = subprocess.run(
        [str(HOMFLY_CHECK), "--emit-poly"],
        input=flat, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"homfly_check failed: {proc.stderr.strip()}")
    poly = regina.Laurent2()
    for line in proc.stdout.splitlines():
        if not line.strip():
            continue
        l_exp, m_exp, coef = (int(x) for x in line.split())
        poly.set(l_exp, m_exp, coef)
    return poly


def read_pd_file(path):
    crossings = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        crossings.append([int(x) for x in line.split()])
    return crossings


def check(name, crossings):
    """Return (passed, detail)."""
    try:
        lib = libhomfly_laurent2(crossings)
        reg = regina_homflyLM(crossings)
    except Exception as e:  # noqa: BLE001 — report any failure as a test failure
        return False, f"error: {e}"
    if lib == reg:
        return True, reg.str()
    return False, f"libhomfly={lib.str()!r}  regina={reg.str()!r}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--samples", type=int, default=40,
                    help="number of linktable diagrams to sample (default 40)")
    args = ap.parse_args()

    if not HOMFLY_CHECK.exists():
        sys.exit(f"build homfly_check first (make -C {HERE}); not found at {HOMFLY_CHECK}")

    panel = [
        ("trefoil (3_1)", TREFOIL),
        ("figure-eight (4_1)", FIGURE_EIGHT),
        # Split links exercise the delta rule (H = delta^(k-1) * prod H(piece)).
        ("trefoil # split # figure-eight", disjoint_union(TREFOIL, FIGURE_EIGHT)),
        ("trefoil # split # trefoil", disjoint_union(TREFOIL, TREFOIL)),
        ("trefoil # split # trefoil # split # figure-eight",
         disjoint_union(TREFOIL, TREFOIL, FIGURE_EIGHT)),
    ]
    n_knots = len(panel)

    if LINKTABLE_DIR.is_dir():
        files = sorted(LINKTABLE_DIR.glob("*.tsv"))[: args.samples]
        for f in files:
            panel.append((f.stem, read_pd_file(f)))

    passed = failed = 0
    for name, crossings in panel:
        ok, detail = check(name, crossings)
        if ok:
            passed += 1
        else:
            failed += 1
            print(f"  FAIL  {name}: {detail}")

    print(f"\nhomfly_xcheck: {passed} agreed, {failed} disagreed "
          f"({len(panel)} diagrams: {n_knots} knots + {len(panel) - n_knots} linktable)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
