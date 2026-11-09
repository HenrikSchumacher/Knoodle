#!/usr/bin/env python3
"""klut_lfs_pointer_check -- knoodleidentify refuses KLUT tables that are LFS pointers.

data/Klut is stored with Git LFS. A checkout made without it -- CI checks out
with `lfs: false` -- has ~130-byte pointer stubs where the tables should be,
each starting "version https://git-lfs.github.com/spec/v1". knoodleidentify
used to load those without complaint and miss every lookup: a plain trefoil
came back NotFound[3, ...] with "table gap or a hard simplification case?" and
exit 1, which reads as a hole in the table rather than a broken checkout. It
now looks at the files before loading them and stops, naming the directory and
saying to run `git lfs pull`.

The pointer files are made the way a no-LFS checkout makes them: `git show
HEAD:data/Klut/<file>` prints the blob git stores, which is the pointer whether
or not this checkout has the real tables. So the test runs the same in CI and
on a laptop. Only the in-tree control at the end depends on which of the two
this checkout is, and it asserts the right behaviour for each.

Run: python3 klut_lfs_pointer_check.py   (needs ../tools/knoodleidentify built)
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
IDENTIFY = os.path.join(REPO, "tools", "knoodleidentify")
KLUT = os.path.join(REPO, "data", "Klut")

LFS_HEADER = b"version https://git-lfs.github.com/spec/"
TREFOIL = "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n"

checks = 0
fails = 0


def check(ok, what, detail=""):
    global checks, fails
    checks += 1
    if ok:
        return
    print(f"  FAILED  {what}")
    if detail:
        for line in detail.rstrip().splitlines()[:6]:
            print(f"          | {line}")
    fails += 1


def section(s):
    print(f"\n=== {s} ===")


def run(args, env_extra=None):
    """Run knoodleidentify on a trefoil, with $KNOODLE_KLUT_DIR unset unless given."""
    env = {k: v for k, v in os.environ.items() if k != "KNOODLE_KLUT_DIR"}
    env.update(env_extra or {})
    p = subprocess.run([IDENTIFY] + args, input=TREFOIL, capture_output=True,
                       text=True, env=env)
    return p.returncode, p.stdout, p.stderr


def git(*args):
    return subprocess.run(["git", "-C", REPO] + list(args), capture_output=True,
                          check=True).stdout


def is_pointer(path):
    with open(path, "rb") as f:
        return f.read(len(LFS_HEADER)) == LFS_HEADER


def make_pointer_dir(dest):
    """Fill dest with the LFS pointers of every tracked data/Klut table file.

    Klut_Manifest.txt is tracked too, but deliberately not in LFS (it must be
    readable without it), so it is left out.
    """
    names = []
    for rel in git("ls-files", "data/Klut").decode().split():
        name = os.path.basename(rel)
        if not name.endswith((".bin", ".tsv")):
            continue
        with open(os.path.join(dest, name), "wb") as f:
            f.write(git("show", f"HEAD:{rel}"))
        names.append(name)
    return names


def assert_refused(what, rc, out, err, dir_shown):
    check(rc != 0, f"{what}: exits nonzero", err)
    check(out == "", f"{what}: identifies nothing (stdout empty)", out)
    check(dir_shown in err, f"{what}: names the directory {dir_shown}", err)
    check("git lfs pull" in err, f"{what}: says to run `git lfs pull`", err)
    check("--data-dir" in err, f"{what}: mentions --data-dir", err)
    check("NotFound" not in err and "table gap" not in err,
          f"{what}: no misleading table-gap message", err)


# ---------------------------------------------------------------------------


def test_pointer_dir(ptr, names):
    section("a directory of LFS pointers is refused")

    check(len(names) >= 2 * 11, f"data/Klut tracks 22 table files; found {len(names)}")
    check(all(is_pointer(os.path.join(ptr, n)) for n in names),
          "every `git show HEAD:data/Klut/<file>` blob is an LFS pointer "
          "(else this test proves nothing)")

    rc, out, err = run([f"--data-dir={ptr}"])
    assert_refused("--data-dir", rc, out, err, ptr)

    rc, out, err = run([], {"KNOODLE_KLUT_DIR": ptr})
    assert_refused("$KNOODLE_KLUT_DIR", rc, out, err, ptr)

    # Only the 3-crossing subtables are in play here, and they are refused too.
    rc, out, err = run([f"--data-dir={ptr}", "--max-crossings=3"])
    assert_refused("--max-crossings=3", rc, out, err, ptr)

    # --quiet hides the summary and warnings, never this error.
    rc, out, err = run([f"--data-dir={ptr}", "--quiet"])
    assert_refused("--quiet", rc, out, err, ptr)


def test_one_pointer_is_enough(ptr, names, scratch):
    section("one pointer among other files is refused")

    # A partial `git lfs pull --include=...` leaves some files real and some
    # stubs. Model it with every file but the 13-crossing values table replaced
    # by non-pointer bytes. The tool must stop before it loads any of them, so
    # their contents never matter.
    mixed = os.path.join(scratch, "mixed")
    os.mkdir(mixed)
    for n in names:
        src = os.path.join(ptr, n)
        if n == "Klut_Values_13.tsv":
            shutil.copy(src, os.path.join(mixed, n))
        else:
            with open(os.path.join(mixed, n), "wb") as f:
                f.write(b"not a pointer\n")

    rc, out, err = run([f"--data-dir={mixed}"])
    assert_refused("only Klut_Values_13.tsv a pointer", rc, out, err, mixed)
    check("Klut_Values_13.tsv" in err and "1 of 22" in err,
          "the message counts the one pointer and names it", err)


def test_in_tree_tables():
    section("the in-tree tables, whichever kind this checkout has")

    values03 = os.path.join(KLUT, "Klut_Values_03.tsv")
    if is_pointer(values03):
        # A no-LFS checkout (CI): the default resolution must refuse too.
        rc, out, err = run([])
        assert_refused("in-tree data/Klut (pointers)", rc, out, err,
                       os.path.realpath(KLUT))
    else:
        # Real tables: the check must not get in the way.
        for what, args in (("in-tree data/Klut", []),
                           ("--data-dir=data/Klut", [f"--data-dir={KLUT}"])):
            rc, out, err = run(args)
            check(rc == 0, f"{what} (real tables): exits 0", err)
            check("KnotSymbol[3,1," in out, f"{what}: names the trefoil", out)
            check("pointer" not in err, f"{what}: no pointer complaint", err)


def main():
    print("klut_lfs_pointer_check -- knoodleidentify vs KLUT tables that are LFS pointers")

    if not os.path.exists(IDENTIFY):
        print(f"error: {IDENTIFY} is not built. Run `make all` in ../tools.")
        return 2

    with tempfile.TemporaryDirectory(prefix="klut-lfs-") as scratch:
        ptr = os.path.join(scratch, "pointers")
        os.mkdir(ptr)
        names = make_pointer_dir(ptr)
        test_pointer_dir(ptr, names)
        test_one_pointer_is_enough(ptr, names, scratch)
    test_in_tree_tables()

    print(f"\n{'KLUT LFS POINTER CHECK OK' if fails == 0 else 'KLUT LFS POINTER CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
