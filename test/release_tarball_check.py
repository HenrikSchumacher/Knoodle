#!/usr/bin/env python3
"""release_tarball_check -- the vendored release tarball can be built, and builds.

WHY THIS EXISTS. scripts/make-source-tarball.sh makes the asset every release
ships: the Homebrew formula and offline cluster builds use it instead of a
git + LFS + submodule clone. Nothing ran it except the release workflow itself,
from the tag. At v1.1.0 its allowlist still named PolyFold/ and Reapr_Legacy.hpp
-- one moved to devel/PolyFold, the other deleted, both months earlier -- and
`cp -a` of a missing path would have failed the release workflow after the tag
was public. (PolyFold also did not compile; see polyfold_check.)

Two depths, because the light tier reduces rather than skips:

  light (no arguments; also what CI runs)  -- static, no network, under a second:
    * every entry of the script's SHIP list exists in the checkout and is
      tracked by git (or is a submodule);
    * the formula's build entry points (tools/Makefile, devel/PolyFold/makefile)
      and the KLUT are inside SHIP;
    * every quoted #include in the shipped C++ that resolves to a file in this
      checkout resolves to one inside SHIP -- so a new include of, say,
      experimental/ cannot ship a tarball that does not compile.

  heavy (--build)  -- what the release workflow and the formula do, end to end:
    * run the script, unpack the tarball into an empty directory, and check
      its VERSION file and that the KLUT is real data, not LFS pointers;
    * build the four tools and PolyFold FROM THE UNPACKED TREE, with
      KNOODLE_VERSION unset so the makefiles must find the tarball's VERSION;
    * run the formula's checks against those binaries: every tool's --help,
      knoodleidentify finding the KLUT next to itself from a scratch directory
      and naming a 3- and a 13-crossing knot from it (an empty input, which is
      what the formula runs, loads no subtable), and the PolyFold smoke run;
      and each binary reports the tarball's version.

The heavy build compiles every tool from scratch (minutes) and needs git-lfs,
Boost and SuiteSparse. That is the heavy tier's job; the light tier and CI get
the static half.

Run: python3 release_tarball_check.py [--build]
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SCRIPT = os.path.join(REPO, "scripts", "make-source-tarball.sh")
TOOLS = ["knoodlesimplify", "knoodledraw", "knoodleidentify", "knoodleprove"]
CHECK_VERSION = "v0.0.0-tarball-check"

# Signed PD codes of the right-handed trefoil, K[3,1,1,"e/r"], and of 13a1,
# K[13,1,1,"e/r"] (the first key of Klut_Keys_13.bin, rebuilt as a diagram).
TREFOIL = "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n"
KNOT_13A1 = ("22 0 23 25 1\n0 6 1 5 1\n12 1 13 2 -1\n2 11 3 12 -1\n20 3 21 4 -1\n"
             "4 19 5 20 -1\n6 14 7 13 1\n10 8 11 7 1\n8 15 9 16 -1\n14 9 15 10 -1\n"
             "16 22 17 21 1\n24 17 25 18 -1\n18 23 19 24 -1\n")

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
        for line in str(detail).strip().splitlines()[-20:]:
            print(f"          | {line}")


def section(s):
    print(f"\n=== {s} ===")


def git(*args):
    return subprocess.run(["git", "-C", REPO, *args], capture_output=True, text=True,
                          check=True).stdout


def ship_list():
    """The SHIP=( ... ) array in the script: the single allowlist it uses for both
    the copy and the tracked-file filter."""
    text = open(SCRIPT).read()
    m = re.search(r"^SHIP=\((.*?)\)", text, re.S | re.M)
    return m.group(1).split() if m else []


def inside(rel, ship):
    return any(rel == s or rel.startswith(s.rstrip("/") + "/") for s in ship)


# ---------------------------------------------------------------------------
# light: static
# ---------------------------------------------------------------------------

def static_checks(ship):
    section("the allowlist names what exists")
    check(ship != [], "the script has a SHIP=( ... ) allowlist", SCRIPT)
    tracked = set(git("ls-files").splitlines())
    submodules = set(git("config", "--file", ".gitmodules", "--get-regexp", "path")
                     .split()[1::2]) if os.path.exists(os.path.join(REPO, ".gitmodules")) else set()
    for s in ship:
        exists = os.path.exists(os.path.join(REPO, s))
        check(exists, f"SHIP entry '{s}' exists (was it moved or deleted?)")
        is_tracked = s in tracked or any(t.startswith(s + "/") for t in tracked) \
            or any(m == s or m.startswith(s + "/") for m in submodules)
        check(is_tracked, f"SHIP entry '{s}' is tracked by git")

    section("the formula's build entry points ship")
    for need in ["tools/Makefile", "devel/PolyFold/makefile", "data/Klut", "Knoodle.hpp"]:
        check(inside(need, ship), f"'{need}' is inside SHIP")

    section("shipped C++ includes nothing outside SHIP")
    roots = [s for s in ship if s not in ("submodules", "deps", "data/Klut")]
    sources = sorted(t for t in tracked
                     if inside(t, roots) and t.endswith((".hpp", ".cpp", ".h", ".cc")))
    include = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
    escaped = []
    for rel in sources:
        try:
            text = open(os.path.join(REPO, rel), errors="replace").read()
        except OSError:
            continue
        for inc in include.findall(text):
            target = os.path.normpath(os.path.join(os.path.dirname(rel), inc))
            if target.startswith(".."):
                escaped.append(f"{rel}: \"{inc}\" leaves the source tree")
            elif os.path.isfile(os.path.join(REPO, target)) and not inside(target, ship):
                escaped.append(f"{rel}: \"{inc}\" -> {target}")
    check(len(sources) > 100, f"scanned the shipped sources ({len(sources)} files)")
    check(escaped == [], f"{len(escaped)} include(s) reach outside SHIP", "\n".join(escaped))


# ---------------------------------------------------------------------------
# heavy: build the tarball and build from it
# ---------------------------------------------------------------------------

def build_checks(ship):
    section("the script builds a tarball")
    work = tempfile.mkdtemp(prefix="knoodle-tarball-check-")
    try:
        p = subprocess.run(["bash", SCRIPT, CHECK_VERSION, work], cwd=REPO,
                           capture_output=True, text=True, timeout=1800)
        check(p.returncode == 0, "make-source-tarball.sh exits 0", p.stdout + p.stderr)
        name = f"knoodle-{CHECK_VERSION}-vendored"
        tgz = os.path.join(work, name + ".tar.gz")
        check(os.path.exists(tgz), f"it wrote {name}.tar.gz")
        if not os.path.exists(tgz):
            return

        unpack = os.path.join(work, "unpack")
        os.mkdir(unpack)
        with tarfile.open(tgz) as tf:
            members = tf.getnames()
            check(all(m == name or m.startswith(name + "/") for m in members),
                  f"every entry is under the single top-level {name}/")
            tf.extractall(unpack, filter="data") if sys.version_info >= (3, 12) else tf.extractall(unpack)
        root = os.path.join(unpack, name)

        section("the tarball's contents")
        version = open(os.path.join(root, "VERSION")).read().strip() \
            if os.path.exists(os.path.join(root, "VERSION")) else ""
        check(version == CHECK_VERSION, f"VERSION is {CHECK_VERSION}", version)
        for s in ship:
            check(os.path.exists(os.path.join(root, s)), f"'{s}' is in the tarball")
        for banned in ["test", "docs", ".git", "experimental", "examples"]:
            check(not os.path.exists(os.path.join(root, banned)), f"'{banned}' is NOT in the tarball")
        keys = os.path.join(root, "data", "Klut", "Klut_Keys_13.bin")
        size = os.path.getsize(keys) if os.path.exists(keys) else 0
        check(size > 1_000_000, f"the KLUT keys are real data, not LFS pointers ({size} bytes)")

        section("build from the unpacked tree")
        env = dict(os.environ)
        env.pop("KNOODLE_VERSION", None)   # make the makefiles find ../VERSION
        # Build as a fresh shell would, as Homebrew does. Under `make check-full`
        # this script inherits the outer make's MAKEFLAGS, which names jobserver
        # file descriptors that Python does not pass on to children: the inner
        # make then dies with "write jobserver: Bad file descriptor". (It passed
        # run by hand and failed in the tier, which is how this was found.)
        for var in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL", "MAKEOVERRIDES", "GNUMAKEFLAGS"):
            env.pop(var, None)
        for d, target in [("tools", "all"), (os.path.join("devel", "PolyFold"), "polyfold")]:
            p = subprocess.run(["make", "-C", os.path.join(root, d), target], env=env,
                               capture_output=True, text=True, timeout=3600)
            check(p.returncode == 0, f"make -C {d} {target} succeeds in the unpacked tree",
                  p.stdout + p.stderr)

        section("the formula's checks, against the tarball's binaries")
        scratch = os.path.join(work, "scratch")   # no data/Klut here, no env override
        os.mkdir(scratch)
        env.pop("KNOODLE_KLUT_DIR", None)
        for t in TOOLS:
            exe = os.path.join(root, "tools", t)
            if not os.path.exists(exe):
                check(False, f"{t} was built")
                continue
            p = subprocess.run([exe, "--help"], cwd=scratch, env=env,
                               capture_output=True, text=True, timeout=120)
            check(p.returncode == 0, f"{t} --help exits 0", p.stdout + p.stderr)
            # Only knoodleprove has --version; the others compile the same
            # GIT_VERSION into their failure reports (BuildIdentity), so look
            # for it in the binary. Either way it proves the makefile read the
            # tarball's VERSION rather than falling through to "unknown".
            check(CHECK_VERSION.encode() in open(exe, "rb").read(),
                  f"{t} has the tarball's VERSION compiled in")
        p = subprocess.run([os.path.join(root, "tools", "knoodleprove"), "--version"],
                           cwd=scratch, env=env, capture_output=True, text=True, timeout=120)
        check(CHECK_VERSION in p.stdout + p.stderr,
              "knoodleprove --version reports the tarball's VERSION", p.stdout + p.stderr)

        # An empty input loads no subtable, so it proves only that the tool found
        # a directory (and, since the startup table check, that every file in it
        # is structurally sound). Identify a 3- and a 13-crossing knot as well, so
        # the smallest and the largest subtable are actually read and the names
        # that come back are the right ones.
        knots = os.path.join(scratch, "knots.tsv")
        with open(knots, "w") as f:
            f.write(TREFOIL + "k\n" + KNOT_13A1)
        p = subprocess.run([os.path.join(root, "tools", "knoodleidentify"), "--expanded", knots],
                           cwd=scratch, env=env, capture_output=True, text=True, timeout=300)
        check(p.returncode == 0,
              "knoodleidentify finds the KLUT next to itself, from a scratch directory",
              p.stdout + p.stderr)
        check(p.stdout.split() == ['K[3,1,1,"e/r"]', 'K[13,1,1,"e/r"]'],
              "knoodleidentify names the trefoil and 13a1 from the tarball's KLUT",
              p.stdout + p.stderr)

        polyfold = os.path.join(root, "devel", "PolyFold", "polyfold")
        out = os.path.join(scratch, "pf")
        p = subprocess.run([polyfold, "-n", "8", "-N", "2", "-b", "2", "-s", "1", "-g", "-o", out],
                           cwd=scratch, env=env, capture_output=True, text=True, timeout=300)
        check(p.returncode == 0, "the formula's PolyFold smoke run exits 0", p.stdout + p.stderr)
        info = os.path.join(out, "Info.m")
        text = open(info).read() if os.path.exists(info) else ""
        check('"Sample Count" -> 2,' in text and text.count('"Squared Gyradius"') == 2,
              "PolyFold's Info.m records 2 samples, each with a squared gyradius")
        check(CHECK_VERSION in text, "PolyFold's Info.m reports the tarball's VERSION")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", action="store_true",
                    help="build the tarball, unpack it, and build everything from it (heavy)")
    args = ap.parse_args()

    print("release_tarball_check -- the vendored release tarball"
          + (" (static + build)" if args.build else " (static)"))
    ship = ship_list()
    static_checks(ship)
    if args.build:
        build_checks(ship)

    print(f"\nrelease_tarball_check: ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
