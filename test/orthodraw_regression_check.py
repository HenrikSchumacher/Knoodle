#!/usr/bin/env python3
"""orthodraw_regression_check: the by-hand tool test/orthodraw_regression.py works.

The tool itself is in no tier (it compares OrthoDraw's drawings with a golden
file, over Git LFS data, and a changed drawing is not a failure). This checks
its machinery so that it does not rot: a made-up corpus of three files and a
made-up knoodledraw, whose drawings change when told to.
"""

import os
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOL = os.path.join(HERE, "orthodraw_regression.py")

checks = fails = 0


def check(okQ, what, detail=""):
    global checks, fails
    checks += 1
    if not okQ:
        fails += 1
    print(f"  {'ok  ' if okQ else 'FAIL'}  {what}")
    if not okQ and detail:
        print("      " + detail.replace("\n", "\n      "))


FAKE_DRAW = """#!/usr/bin/env python3
import os, sys
path = sys.argv[-1]
name = os.path.basename(path)
rows = open(path).read().split()
mark = "#" if name in os.environ.get("FAKE_CHANGED", "").split(",") else "+"
for r in rows:
    print(mark + "--" + r + "--" + mark + "  ")
sys.exit(3 if name in os.environ.get("FAKE_FAILS", "").split(",") else 0)
"""

with tempfile.TemporaryDirectory() as tmp:
    d = os.path.join(tmp, "data", "diagrams", "hardunknots")
    os.makedirs(d)
    for name, rows in (("a.tsv", "1 2 3"), ("b.tsv", "4 5"), ("c.tsv", "6")):
        with open(os.path.join(d, name), "w") as f:
            f.write(rows + "\n")
    fake = os.path.join(tmp, "fakedraw")
    with open(fake, "w") as f:
        f.write(FAKE_DRAW)
    os.chmod(fake, os.stat(fake).st_mode | stat.S_IXUSR)
    golden = os.path.join(tmp, "golden.txt")

    def run(args=(), keys=None, build="build-one", **fake_env):
        env = dict(os.environ, ORTHODRAW_REGRESSION_ROOT=tmp, ORTHODRAW_REGRESSION_DRAW=fake,
                   ORTHODRAW_REGRESSION_GOLDEN=golden, ORTHODRAW_REGRESSION_BUILD=build, **fake_env)
        env.pop("ORTHODRAW_REGRESSION_KEYS", None)
        if keys is not None:
            env["ORTHODRAW_REGRESSION_KEYS"] = keys
        p = subprocess.run([sys.executable, TOOL, *args], capture_output=True, text=True,
                           stdin=subprocess.DEVNULL, env=env, timeout=120)
        return p.returncode, p.stdout + p.stderr

    def golden_text():
        return open(golden).read() if os.path.exists(golden) else None

    print("=== no golden file, then one ===")
    rc, out = run()
    check(rc == 2 and "no golden file" in out, "without a golden file: exit 2, and it says so", out)
    rc, out = run(["--accept"])
    first = golden_text()
    check(rc == 0 and first is not None and "# recorded\tbuild-one\t" in first
          and first.count("=== ") == 3 and "+--1--+  \n" in first,
          "--accept writes it: the build's name, three drawings, trailing spaces kept", out)
    rc, out = run(build="build-two")
    check(rc == 0 and "3 drawings unchanged from build-one" in out and "this build (build-two)" in out,
          "the same drawings from another build: exit 0, both builds named", out)

    print("=== drawings that changed ===")
    rc, out = run(["--check"], build="build-two", FAKE_CHANGED="b.tsv,c.tsv")
    check(rc == 1 and "on 2 of 3 test diagrams has changed from build-one" in out
          and "b.tsv" in out and "c.tsv" in out and "a.tsv" not in out,
          "--check: exit 1, the two changed files named and the other not", out)
    check("Press v" not in out and golden_text() == first, "--check asks nothing and changes nothing", out)
    rc, out = run(build="build-two", FAKE_CHANGED="b.tsv")
    check(rc == 1 and "Press v" not in out, "not at a terminal: the same as --check", out)
    rc, out = run(["--show"], build="build-two", FAKE_CHANGED="b.tsv")
    check(rc == 1 and "+--4--+" in out and "#--4--#" in out and "old: build-one" in out
          and "new: build-two" in out and "--1--" not in out,
          "--show: the old and the new drawing of the changed file, and only that file", out)
    rc, out = run(["--check"], build="build-two", FAKE_FAILS="a.tsv")
    check(rc == 1 and "on 1 of 3" in out and "a.tsv" in out,
          "the same drawing with another exit status counts as changed", out)

    print("=== answering the question ===")
    rc, out = run(keys="v space space", build="build-two", FAKE_CHANGED="b.tsv,c.tsv")
    check(rc == 1 and "[1/2]" in out and "[2/2]" in out and "#--6--#" in out
          and "Ignored" in out and golden_text() == first,
          "v, space, then space: both files shown, then ignored; nothing changed", out)
    rc, out = run(keys="v q space", build="build-two", FAKE_CHANGED="b.tsv,c.tsv")
    check(rc == 1 and "[1/2]" in out and "[2/2]" not in out and golden_text() == first,
          "v then q: the viewer stops after the first file", out)
    rc, out = run(keys="acept space", build="build-two", FAKE_CHANGED="b.tsv")
    check(rc == 1 and "'acept' is not accept" in out and golden_text() == first,
          "a mistyped accept accepts nothing", out)
    rc, out = run(keys="accept", build="build-two", FAKE_CHANGED="b.tsv")
    second = golden_text()
    check(rc == 0 and second != first and "# recorded\tbuild-two\t" in second and "#--4--#  \n" in second,
          "accept: exit 0, the golden file now holds build-two's drawings", out)
    rc, out = run(build="build-three", FAKE_CHANGED="b.tsv")
    check(rc == 0 and "3 drawings unchanged from build-two" in out, "and the next run finds nothing changed", out)

    print("=== a corpus that is not checked out ===")
    with open(os.path.join(d, "c.tsv"), "w") as f:
        f.write("version https://git-lfs.github.com/spec/v1\noid sha256:0\nsize 1\n")
    rc, out = run(["--check"])
    check(rc == 2 and "c.tsv is a Git LFS pointer" in out and golden_text() == second,
          "a Git LFS pointer in the corpus: exit 2, named, nothing compared", out)

print(f"orthodraw_regression_check: ({checks} checks, {fails} failed)")
sys.exit(1 if fails else 0)
