#!/usr/bin/env python3
"""polyfold_check -- PolyFold builds, and the Homebrew formula's smoke run passes.

WHY THIS EXISTS. Until v1.1.0 nothing in test/ or CI built PolyFold. Only the
Homebrew formula did, and it builds from a release tag -- so a broken PolyFold
reached the release before anyone saw it, twice:

  * v1.0.0: PolyFold's makefile lacked the Min-Cost-Flow include paths that
    src/OrthoDraw.hpp needs (it #includes MCFSimplex.C). The brew build failed
    and the release was redone.
  * v1.1.0, caught during release prep: the Grand cleanup (61b585f9) moved
    PolyFold/ to devel/PolyFold/ without updating the makefile's relative
    paths, and a later commit (6c782c38) left one use of the removed member
    hierarchicalQ in src/PolyFold/HandleOptions.hpp. Either alone stopped it
    compiling, and neither was noticed for months.

`make -C test tools` (and so `make check`, the pre-push hook, and
`make check-full`) now builds devel/PolyFold, with header dependency tracking
so that an edit in src/ rebuilds it. A compile error therefore fails the tier
before any test runs. This script then checks the binary actually works:

  * `polyfold --help` exits 0 (it did not before v1.0.1);
  * the formula's own `test do` run, `polyfold -n 8 -N 2 -b 2 -s 1 -g -o DIR`,
    exits 0 and writes an Info.m that records the samples it was asked for,
    each with a squared gyradius (that is what -g asks for) -- so a run that
    exits 0 having sampled nothing fails;
  * the --extend output path (the line 6c782c38 broke) runs and names its
    output directory from the run's parameters.

NEEDS boost_program_options and UMFPACK at link time, hence needs=boost_umfpack
in the manifest: CI drops that link shape (its images lack one or the other)
and run_tier.py lists the skip, so a CI pass never claims to cover PolyFold.

Run: python3 polyfold_check.py
"""

import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
POLYFOLD = os.path.join(REPO, "devel", "PolyFold", "polyfold")

checks = 0
fails = 0


def check(ok, what, detail=""):
    global checks, fails
    checks += 1
    if ok:
        return
    fails += 1
    print(f"  FAILED  {what}")
    if detail:
        for line in detail.strip().splitlines()[-15:]:
            print(f"          | {line}")


def run(args, cwd):
    p = subprocess.run([POLYFOLD, *args], cwd=cwd, capture_output=True, text=True,
                       timeout=300)
    return p.returncode, p.stdout + p.stderr


def main():
    print("polyfold_check -- PolyFold builds and passes the Homebrew smoke run")

    if not os.path.exists(POLYFOLD):
        print(f"  FAILED  {POLYFOLD} is not built. `make -C test tools` builds it;")
        print("          so do `make check` and `make check-full`.")
        print("\npolyfold_check: (1 checks, 1 failed)")
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        # --help, from a scratch directory so nothing lands in the tree.
        rc, out = run(["--help"], tmp)
        check(rc == 0, "polyfold --help exits 0", out)
        check("--edge-count" in out, "polyfold --help lists its options (--edge-count)", out)

        # Exactly the formula's `test do` invocation.
        n, N = 8, 2
        outdir = os.path.join(tmp, "smoke")
        rc, out = run(["-n", str(n), "-N", str(N), "-b", "2", "-s", "1", "-g",
                       "-o", outdir], tmp)
        check(rc == 0, "the formula's smoke run exits 0", out)
        info_path = os.path.join(outdir, "Info.m")
        info = open(info_path).read() if os.path.exists(info_path) else ""
        check(info != "", "the smoke run writes a non-empty Info.m", out)
        check(f'"Edge Count" -> {n},' in info, f"Info.m records Edge Count {n}")
        check(f'"Sample Count" -> {N},' in info, f"Info.m records Sample Count {N}")
        sampled = re.findall(r'"Sample" -> (\d+),', info)
        check(sampled == [str(i) for i in range(1, N + 1)],
              f"Info.m reports samples 1..{N}", f"found: {sampled}")
        gyradii = re.findall(r'"Squared Gyradius" ->\s*([0-9.eE*^+-]+)', info)
        check(len(gyradii) == N, f"every sample has a squared gyradius (-g): {len(gyradii)} of {N}")

        # --extend builds the output name from the parameters. 6c782c38 broke
        # exactly this line (a removed member, hierarchicalQ, was still read).
        base = os.path.join(tmp, "ext")
        rc, out = run(["-n", str(n), "-N", str(N), "-b", "2", "-s", "1", "-g",
                       "-o", base, "--extend"], tmp)
        check(rc == 0, "polyfold --extend exits 0", out)
        made = [d for d in os.listdir(tmp) if d.startswith("ext")]
        check(len(made) == 1 and f"-n{n}" in made[0] and f"-N{N}" in made[0],
              "--extend names its output directory from the parameters", f"made: {made}")
        if len(made) == 1:
            check(os.path.exists(os.path.join(tmp, made[0], "Info.m")),
                  "--extend writes Info.m into that directory")

    print(f"\npolyfold_check: ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
