#!/usr/bin/env python3
"""klut_table_check -- knoodleidentify refuses bad KLUT tables instead of answering from them.

knoodleidentify names knots by looking their keys up in data/Klut. Until
November 2026 it trusted those files completely, and a damaged table gave a
real-looking answer: a one-digit change to one count misnamed thousands of
knots with exit status 0, and a missing or truncated file showed up as
NotFound, which reads as a gap in the table. docs/klut-table-failure-audit.md
has the whole list. This test holds the tool to what it does now:

  * the tables are checked before any input is read. Missing, empty,
    unreadable or truncated files, a keys file whose size does not match its
    values file, and a values file that does not parse strictly all stop it
    with exit 1, empty stdout, and a message naming the file;
  * a table that fails or changes while it runs (the subtables load lazily)
    stops it at that knot, with an error, never NotFound or a wrong name;
  * a NotFound is a nonzero exit, with --quiet too;
  * --data-dir and $KNOODLE_KLUT_DIR are the only candidate when given, so a
    wrong one is an error rather than a quiet switch to other tables; an
    unreadable directory is an error, not a crash; tables found only in the
    current directory come with a warning; the Homebrew layout (a symlinked
    bin/ next to share/knoodle/Klut) works; and the summary says which tables
    answered.

Damage that keeps the structure intact (a flipped digit in a knot's index, a
flipped bit inside a key, two names swapped at startup) is NOT caught: only a
checksum could see it. The audit lists those cases.

The test carries its own copy of the 3- to 7-crossing tables (MINI below,
~800 bytes) and runs with --max-crossings=7, so it works in a checkout made
without Git LFS, as CI's is. When this checkout has the real tables it also
checks that MINI still matches them, and that the full set names a 13-crossing
knot.

Run: python3 klut_table_check.py   (needs ../tools/knoodleidentify built)
"""

import os
import shutil
import stat
import subprocess
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
IDENTIFY = os.path.join(REPO, "tools", "knoodleidentify")
KLUT = os.path.join(REPO, "data", "Klut")

LFS_HEADER = b"version https://git-lfs.github.com/spec/"

TREFOIL = "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n"
KNOT_5_1 = "0 6 1 5 1\n2 8 3 7 1\n4 0 5 9 1\n6 2 7 1 1\n8 4 9 3 1\n"
KNOT_13A1 = ("22 0 23 25 1\n0 6 1 5 1\n12 1 13 2 -1\n2 11 3 12 -1\n20 3 21 4 -1\n"
             "4 19 5 20 -1\n6 14 7 13 1\n10 8 11 7 1\n8 15 9 16 -1\n14 9 15 10 -1\n"
             "16 22 17 21 1\n24 17 25 18 -1\n18 23 19 24 -1\n")
TWO_KNOTS = TREFOIL + "k\n" + KNOT_5_1
TWO_NAMES = ['K[3,1,1,"e/r"]', 'K[5,1,1,"e/r"]']

# data/Klut's 3- to 7-crossing subtables, byte for byte: crossing number ->
# (Klut_Values_NN.tsv text, Klut_Keys_NN.bin bytes). test_mini_matches_tree
# checks them against data/Klut whenever the real tables are present.
MINI = {
    3: (
        'K[3,1,1,"e/r"]\t1\n'
        'K[3,1,1,"m/mr"]\t1\n',
        bytes.fromhex('0f0d0f0e0c0e')),
    4: (
        'K[4,1,1,"e/m/r/mr"]\t2\n',
        bytes.fromhex('170d160c150f140e')),
    5: (
        'K[5,1,1,"e/r"]\t1\n'
        'K[5,1,1,"m/mr"]\t1\n'
        'K[5,2,1,"e/r"]\t1\n'
        'K[5,2,1,"m/mr"]\t1\n',
        bytes.fromhex('171517151716141614161f150f15171e140e1416')),
    6: (
        'K[6,1,1,"e/r"]\t2\n'
        'K[6,1,1,"m/mr"]\t2\n'
        'K[6,2,1,"e/r"]\t2\n'
        'K[6,2,1,"m/mr"]\t2\n'
        'K[6,3,1,"e/m/r/mr"]\t4\n',
        bytes.fromhex('251f150f1c16271d170d1e14241e140e1d17261c160c1f15271517151e142517'
                      '15171c16261416141f15241614161d172516141f1d0e250e141617152714161d'
                      '1f0c270c16141517')),
    7: (
        'K[7,1,1,"e/r"]\t1\n'
        'K[7,1,1,"m/mr"]\t1\n'
        'K[7,2,1,"e/r"]\t1\n'
        'K[7,2,1,"m/mr"]\t1\n'
        'K[7,3,1,"e/r"]\t1\n'
        'K[7,3,1,"m/mr"]\t1\n'
        'K[7,4,1,"e/r"]\t1\n'
        'K[7,4,1,"m/mr"]\t1\n'
        'K[7,5,1,"e/r"]\t3\n'
        'K[7,5,1,"m/mr"]\t3\n'
        'K[7,6,1,"e/r"]\t6\n'
        'K[7,6,1,"m/mr"]\t6\n'
        'K[7,7,1,"e/r"]\t4\n'
        'K[7,7,1,"m/mr"]\t4\n',
        bytes.fromhex('1f1d1f1d1f1d1f1e1c1e1c1e1c1e2f251f150f1d1f2e241e140e1c1e271d171d'
                      '1f1d1f261c161c1e1c1e271d17251f151f261c16241e141e2f251715171d1f2f'
                      '1d1f1d0f1d1f2d271517151f1d2e241614161c1e2c261416141e1c2e1c1e1c0e'
                      '1c1e2f0c1f151d1f0c2e241715271d0f2e240f15171f152f1d172527140e2d0e'
                      '1d171f1d0e2f140e1d1715172f250e14161e142d1e1c0f241e0e2e150f1c1614'
                      '162e1c162426150f2f1c1e0d261c0c2f251614261c0e2f1d0f1c161e142f151e'
                      '14261c0d2d171c16241e0f2f1c161d261c0f2c161d17251f0e2e141f15271d0c'
                      '2e1c0e1d171f152e1d171c271d0e')),
}
MINI_MAX = max(MINI)

# Permission checks mean nothing to root (a CI container, say).
AS_ROOT = hasattr(os, "geteuid") and os.geteuid() == 0

checks = 0
fails = 0


def check(ok, what, detail=""):
    global checks, fails
    checks += 1
    if ok:
        return
    print(f"  FAILED  {what}")
    if detail:
        for line in detail.rstrip().splitlines()[:8]:
            print(f"          | {line}")
    fails += 1


def section(s):
    print(f"\n=== {s} ===")


def values_name(c):
    return f"Klut_Values_{c:02d}.tsv"


def keys_name(c):
    return f"Klut_Keys_{c:02d}.bin"


def write_mini(dest):
    """Fill dest (created if need be) with the MINI tables."""
    os.makedirs(dest, exist_ok=True)
    for c, (values, keys) in MINI.items():
        with open(os.path.join(dest, values_name(c)), "w", newline="") as f:
            f.write(values)
        with open(os.path.join(dest, keys_name(c)), "wb") as f:
            f.write(keys)
    return dest


def env_without_klut_dir(extra=None):
    env = {k: v for k, v in os.environ.items() if k != "KNOODLE_KLUT_DIR"}
    env.update(extra or {})
    return env


def run(args, stdin=TWO_KNOTS, env_extra=None, exe=IDENTIFY, cwd=None):
    """Run knoodleidentify with $KNOODLE_KLUT_DIR unset unless env_extra sets it."""
    p = subprocess.run([exe] + args, input=stdin, capture_output=True, text=True,
                       env=env_without_klut_dir(env_extra), cwd=cwd, timeout=300)
    return p.returncode, p.stdout, p.stderr


def no_crash(what, rc, err):
    # A crash is a negative return code (killed by a signal), or 128+signal from
    # a shell or a crash-catching shim. A refusal is exit 1.
    check(0 <= rc < 128, f"{what}: exits normally, not by a signal (rc={rc})", err)


def assert_refused(what, rc, out, err, *must_say):
    """Stopped before identifying anything, saying what and where."""
    no_crash(what, rc, err)
    check(rc != 0, f"{what}: exits nonzero", err)
    check(out == "", f"{what}: identifies nothing (stdout empty)", out)
    for s in must_say:
        check(s in err, f"{what}: stderr says {s!r}", err)
    check("NotFound" not in err and "table gap" not in err,
          f"{what}: no misleading NotFound / table-gap message", err)


def assert_answers(what, rc, out, err, names=TWO_NAMES):
    no_crash(what, rc, err)
    check(rc == 0, f"{what}: exits 0", err)
    check(out.split() == names, f"{what}: names {names}", out)


# ---------------------------------------------------------------------------


def test_intact(scratch):
    section("intact tables answer, and the summary says where from")

    mini = write_mini(os.path.join(scratch, "intact"))
    rc, out, err = run([f"--data-dir={mini}", f"--max-crossings={MINI_MAX}", "--expanded"])
    assert_answers("--data-dir", rc, out, err)
    check(f"KLUT tables from {mini} (--data-dir)" in err,
          "the summary names the tables' directory and how it was chosen", err)

    rc, out, err = run([f"--max-crossings={MINI_MAX}", "--expanded"],
                       env_extra={"KNOODLE_KLUT_DIR": mini})
    assert_answers("$KNOODLE_KLUT_DIR", rc, out, err)
    check(f"KLUT tables from {mini} ($KNOODLE_KLUT_DIR)" in err,
          "the summary credits $KNOODLE_KLUT_DIR", err)

    rc, out, err = run([f"--data-dir={mini}", f"--max-crossings={MINI_MAX}",
                        "--expanded", "--quiet"])
    assert_answers("--quiet", rc, out, err)
    check(err == "", "--quiet with nothing wrong prints nothing to stderr", err)


# Each damage case: (what, crossing number, how to damage, what stderr must say).
# The damage functions take the MINI (values text, keys bytes) of that subtable
# and return the files to write: a str/bytes, None for "delete it", or "DIR"
# for "put a directory there".

def bump_count(values, old, new):
    assert old in values
    return values.replace(old, new, 1)


DAMAGE = [
    ("values file missing", 5,
     lambda v, k: (None, k), [values_name(5), "missing"]),
    ("keys file missing", 5,
     lambda v, k: (v, None), [keys_name(5), "missing"]),
    ("values file empty", 5,
     lambda v, k: ("", k), [values_name(5), "empty"]),
    ("keys file empty", 5,
     lambda v, k: (v, b""), [keys_name(5), "holds 0 bytes"]),
    ("values file a directory", 5,
     lambda v, k: ("DIR", k), [values_name(5), "not a regular file"]),
    ("keys file one byte short", 7,
     lambda v, k: (v, k[:-1]), [keys_name(7), "truncated or does not belong together"]),
    ("keys file one key too long", 7,
     lambda v, k: (v, k + k[:7]), [keys_name(7), "truncated or does not belong together"]),
    ("a count one too high", 7,
     lambda v, k: (bump_count(v, '"e/r"]\t6\n', '"e/r"]\t7\n'), k),
     [keys_name(7), values_name(7), "truncated or does not belong together"]),
    ("a count one too low", 7,
     lambda v, k: (bump_count(v, '"e/r"]\t6\n', '"e/r"]\t5\n'), k),
     [keys_name(7), "truncated or does not belong together"]),
    ("a count that is not a number", 7,
     lambda v, k: (bump_count(v, '"e/r"]\t6\n', '"e/r"]\t6x\n'), k),
     [values_name(7) + " line 11", "is not a key count"]),
    ("a count of zero", 7,
     lambda v, k: (bump_count(v, '"e/r"]\t6\n', '"e/r"]\t0\n'), k),
     [values_name(7) + " line 11", "is not a key count"]),
    ("values file cut off mid-line", 7,
     lambda v, k: (v[:-3], k), [values_name(7), "does not end with a newline"]),
    ("a zero-filled block in the values file", 7,
     lambda v, k: (v[:40] + "\0" * 16 + v[56:], k),
     [values_name(7), "unexpected byte 0x00"]),
    ("CRLF line endings", 7,
     lambda v, k: (v.replace("\n", "\r\n"), k), [values_name(7), "unexpected byte 0x0d"]),
    ("a name twice", 7,
     lambda v, k: (bump_count(v, 'K[7,2,1,"e/r"]', 'K[7,1,1,"e/r"]'), k),
     [values_name(7) + " line 3", "also appears on line 1"]),
    ("a name for the wrong crossing number", 7,
     lambda v, k: (bump_count(v, 'K[7,4,1,"m/mr"]', 'K[8,4,1,"m/mr"]'), k),
     [values_name(7) + " line 8", 'is not a K[7,i,j,"coset"] name']),
    ("an index with a leading zero", 7,
     lambda v, k: (bump_count(v, 'K[7,3,1,"e/r"]', 'K[7,03,1,"e/r"]'), k),
     [values_name(7) + " line 5", "is not a K[7"]),
    ("a coset that repeats a symmetry", 7,
     lambda v, k: (bump_count(v, 'K[7,5,1,"m/mr"]', 'K[7,5,1,"m/m"]'), k),
     [values_name(7) + " line 10", "is not a K[7"]),
    ("a line with no tab", 7,
     lambda v, k: (bump_count(v, '"e/r"]\t4\n', '"e/r"]4\n'), k),
     [values_name(7) + " line 13", "expected `name<TAB>count`"]),
    ("a space instead of a tab", 7,
     lambda v, k: (bump_count(v, '"e/r"]\t4\n', '"e/r"] 4\n'), k),
     [values_name(7) + " line 13", "unexpected byte 0x20"]),
]


def apply_damage(d, c, damage):
    v_path = os.path.join(d, values_name(c))
    k_path = os.path.join(d, keys_name(c))
    new_v, new_k = damage(*MINI[c])
    for path, new in ((v_path, new_v), (k_path, new_k)):
        if new is None:
            os.remove(path)
        elif new == "DIR":
            os.remove(path)
            os.mkdir(path)
        elif isinstance(new, str):
            with open(path, "w", newline="") as f:
                f.write(new)
        else:
            with open(path, "wb") as f:
                f.write(new)


def test_damage(scratch):
    section("damaged tables are refused before any input is read")

    for i, (what, c, damage, must_say) in enumerate(DAMAGE):
        d = write_mini(os.path.join(scratch, f"damage{i:02d}"))
        apply_damage(d, c, damage)
        rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}"])
        assert_refused(what, rc, out, err, d, "damaged or incomplete", *must_say)

    # --quiet hides the summary, never this.
    d = os.path.join(scratch, "damage00")
    rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}", "--quiet"])
    assert_refused("values file missing, --quiet", rc, out, err, d, values_name(5))

    # Every problem is reported, not just the first.
    d = write_mini(os.path.join(scratch, "two-problems"))
    os.remove(os.path.join(d, values_name(4)))
    with open(os.path.join(d, keys_name(6)), "ab") as f:
        f.write(b"\0")
    rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}"])
    assert_refused("two damaged subtables", rc, out, err, "(2 problems)",
                   values_name(4), keys_name(6))

    if AS_ROOT:
        print("  (running as root: skipping the unreadable-file case)")
    else:
        d = write_mini(os.path.join(scratch, "unreadable-file"))
        os.chmod(os.path.join(d, keys_name(5)), 0)
        rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}"])
        assert_refused("keys file unreadable", rc, out, err, keys_name(5),
                       "cannot be opened for reading")


def test_partial(scratch):
    section("a partial set of tables is refused")

    # The tool reads every subtable up to --max-crossings (13 by default), so
    # MINI's 3..7 is a partial install unless the run asks for 7 or fewer.
    mini = write_mini(os.path.join(scratch, "partial"))
    rc, out, err = run([f"--data-dir={mini}"])
    assert_refused("subtables 8-13 absent", rc, out, err, mini,
                   values_name(8) + ": missing", keys_name(13) + ": missing",
                   "(12 problems)")

    only3 = os.path.join(scratch, "only3")
    os.mkdir(only3)
    for name in (values_name(3), keys_name(3)):
        shutil.copy(os.path.join(mini, name), only3)
    rc, out, err = run([f"--data-dir={only3}", f"--max-crossings={MINI_MAX}"])
    assert_refused("only the 3-crossing pair", rc, out, err, values_name(4) + ": missing",
                   keys_name(7) + ": missing")

    # Asking for fewer subtables is fine: a trefoil only needs the 3s.
    rc, out, err = run([f"--data-dir={only3}", "--max-crossings=3", "--expanded"],
                       stdin=TREFOIL)
    assert_answers("only the 3-crossing pair, --max-crossings=3", rc, out, err,
                   [TWO_NAMES[0]])


def run_midway(d, between):
    """Identify a trefoil, call between(), then identify 5_1, in one run.

    The 5-crossing subtable is not read until 5_1 needs it, so between() can
    change it after the startup check has passed.
    """
    p = subprocess.Popen([IDENTIFY, f"--data-dir={d}", f"--max-crossings={MINI_MAX}",
                          "--expanded"],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, text=True, env=env_without_klut_dir())
    watchdog = threading.Timer(120, p.kill)
    watchdog.start()
    try:
        p.stdin.write(TREFOIL + "k\n")
        p.stdin.flush()
        first = p.stdout.readline()
        between()
        p.stdin.write(KNOT_5_1)
        p.stdin.close()
        rest = p.stdout.read()
        err = p.stderr.read()
        p.wait()
    finally:
        watchdog.cancel()
    return p.returncode, first, rest, err


def test_midway(scratch):
    section("a table that fails or changes while the tool runs")

    # The keys file disappears: Klut's lazy read of it fails. That used to come
    # back as NotFound[5, ...] after a pointless Reapr escalation.
    d = write_mini(os.path.join(scratch, "midway-keys"))
    rc, first, rest, err = run_midway(d, lambda: os.remove(os.path.join(d, keys_name(5))))
    what = "keys file deleted mid-run"
    no_crash(what, rc, err)
    check(first.split() == [TWO_NAMES[0]], f"{what}: the trefoil, before it, is named", first)
    check(rc != 0, f"{what}: exits nonzero", err)
    check(rest == "", f"{what}: nothing printed for 5_1", rest)
    check("5-crossing KLUT table" in err and "failed to load" in err,
          f"{what}: says the 5-crossing table failed to load", err)
    check("stopped at knot 2" in err, f"{what}: says where it stopped", err)
    check("NotFound" not in err and "NotFound" not in rest,
          f"{what}: no NotFound", err + rest)

    # The values file is replaced by one with two names swapped. Its structure
    # is fine, so Klut loads it -- and would name 5_1 "5_1 mirror" by the new
    # order. The names checked at startup no longer match, which is an error.
    d = write_mini(os.path.join(scratch, "midway-values"))
    lines = MINI[5][0].splitlines(keepends=True)
    swapped = "".join([lines[1], lines[0]] + lines[2:])

    def swap():
        with open(os.path.join(d, values_name(5)), "w", newline="") as f:
            f.write(swapped)

    rc, first, rest, err = run_midway(d, swap)
    what = "values file replaced mid-run"
    no_crash(what, rc, err)
    check(first.split() == [TWO_NAMES[0]], f"{what}: the trefoil, before it, is named", first)
    check(rc != 0, f"{what}: exits nonzero", err)
    check(rest == "", f"{what}: no name, right or wrong, for 5_1", rest)
    check("changed after knoodleidentify started" in err,
          f"{what}: says the table changed after startup", err)


def test_notfound_exit(scratch):
    section("NotFound is a nonzero exit, with --quiet too")

    # A table without the trefoil's line and key: structurally sound, so it
    # passes the startup check (only a checksum could tell), and the trefoil
    # comes back NotFound.
    d = write_mini(os.path.join(scratch, "no-trefoil"))
    with open(os.path.join(d, values_name(3)), "w", newline="") as f:
        f.write(MINI[3][0].splitlines(keepends=True)[1])
    with open(os.path.join(d, keys_name(3)), "wb") as f:
        f.write(MINI[3][1][3:])

    for args in ([], ["--quiet"]):
        what = "trefoil missing from the table" + (", --quiet" if args else "")
        rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}"] + args,
                           stdin=TREFOIL)
        no_crash(what, rc, err)
        check(rc != 0, f"{what}: exits nonzero", err)
        check("NotFound[3," in out, f"{what}: prints the NotFound", out)
        if args:
            check("table gap" not in err, f"{what}: --quiet keeps stderr quiet", err)
        else:
            check("came back NotFound" in err, f"{what}: says why the exit is nonzero", err)


def test_where(scratch):
    section("where the tables are looked for")

    mini = write_mini(os.path.join(scratch, "where-tables"))
    missing = os.path.join(scratch, "no-such-dir")
    empty_cwd = os.path.join(scratch, "empty-cwd")
    os.mkdir(empty_cwd)

    rc, out, err = run([f"--data-dir={missing}"])
    assert_refused("--data-dir names a missing directory", rc, out, err, missing,
                   "given by --data-dir")

    # A wrong $KNOODLE_KLUT_DIR used to fall through silently to the tables next
    # to the executable -- here data/Klut, which may well be real.
    rc, out, err = run([], env_extra={"KNOODLE_KLUT_DIR": missing})
    assert_refused("$KNOODLE_KLUT_DIR names a missing directory", rc, out, err, missing,
                   "given by $KNOODLE_KLUT_DIR")

    # A copy of the binary with no tables next to it: a moved binary, or a
    # broken install.
    moved_bin = os.path.join(scratch, "moved", "bin")
    os.makedirs(moved_bin)
    moved = os.path.join(moved_bin, "knoodleidentify")
    shutil.copy2(IDENTIFY, moved)

    rc, out, err = run([], exe=moved, cwd=empty_cwd)
    assert_refused("moved binary, no tables anywhere", rc, out, err,
                   "Could not find KLUT data directory")

    rc, out, err = run([], exe=moved, cwd=empty_cwd, env_extra={"KNOODLE_KLUT_DIR": ""})
    assert_refused("$KNOODLE_KLUT_DIR empty is the same as unset", rc, out, err,
                   "Could not find KLUT data directory")
    check("given by $KNOODLE_KLUT_DIR" not in err,
          "an empty $KNOODLE_KLUT_DIR is not treated as a directory", err)

    # Tables in the current directory only: used, with a warning that names
    # them, even under --quiet.
    cwd_with_tables = os.path.join(scratch, "cwd-with-tables")
    write_mini(os.path.join(cwd_with_tables, "data", "Klut"))
    for args in ([], ["--quiet"]):
        what = "tables only in the current directory" + (", --quiet" if args else "")
        rc, out, err = run([f"--max-crossings={MINI_MAX}", "--expanded"] + args,
                           exe=moved, cwd=cwd_with_tables)
        assert_answers(what, rc, out, err)
        check("warning: no KLUT next to this executable" in err
              and "from the current directory" in err, f"{what}: warns", err)
        check(os.path.join("cwd-with-tables", "data", "Klut") in err,
              f"{what}: the warning gives the tables' path", err)

    # The Homebrew layout: the keg holds bin/knoodleidentify and
    # share/knoodle/Klut, and $(brew --prefix)/bin/knoodleidentify is a symlink
    # into it. The tool must look next to the real file, not the symlink.
    keg = os.path.join(scratch, "Cellar", "knoodle", "9.9.9")
    os.makedirs(os.path.join(keg, "bin"))
    shutil.copy2(IDENTIFY, os.path.join(keg, "bin", "knoodleidentify"))
    write_mini(os.path.join(keg, "share", "knoodle", "Klut"))
    linked_bin = os.path.join(scratch, "homebrew", "bin")
    os.makedirs(linked_bin)
    link = os.path.join(linked_bin, "knoodleidentify")
    os.symlink(os.path.join(keg, "bin", "knoodleidentify"), link)
    rc, out, err = run([f"--max-crossings={MINI_MAX}", "--expanded"], exe=link, cwd=empty_cwd)
    assert_answers("Homebrew layout through a symlink", rc, out, err)
    check(os.path.join("9.9.9", "share", "knoodle", "Klut") in err
          and "(next to the executable)" in err,
          "Homebrew layout: the summary names the keg's share/knoodle/Klut", err)
    check("warning" not in err, "Homebrew layout: no warning", err)

    if AS_ROOT:
        print("  (running as root: skipping the unreadable-directory cases)")
        return

    # An unreadable directory used to be an uncaught filesystem_error: abort(),
    # and a crash report on macOS.
    locked = write_mini(os.path.join(scratch, "locked"))
    os.chmod(locked, 0)
    try:
        rc, out, err = run([f"--data-dir={locked}"])
        assert_refused("--data-dir unreadable", rc, out, err, locked,
                       "Cannot read the KLUT directory")
    finally:
        os.chmod(locked, stat.S_IRWXU)

    # The same, met during the automatic search: a keg whose share/knoodle
    # cannot be read is a broken install, reported as one.
    keg2 = os.path.join(scratch, "Cellar2", "knoodle", "9.9.9")
    os.makedirs(os.path.join(keg2, "bin"))
    shutil.copy2(IDENTIFY, os.path.join(keg2, "bin", "knoodleidentify"))
    share = os.path.join(keg2, "share", "knoodle")
    write_mini(os.path.join(share, "Klut"))
    os.chmod(share, 0)
    try:
        rc, out, err = run([f"--max-crossings={MINI_MAX}"],
                           exe=os.path.join(keg2, "bin", "knoodleidentify"), cwd=empty_cwd)
        assert_refused("unreadable share/knoodle next to the executable", rc, out, err,
                       "Cannot read the KLUT directory", "(next to the executable)")
    finally:
        os.chmod(share, stat.S_IRWXU)


def real_tables_present():
    p = os.path.join(KLUT, values_name(3))
    if not os.path.exists(p):
        return False
    with open(p, "rb") as f:
        return f.read(len(LFS_HEADER)) != LFS_HEADER


def test_tree():
    section("the tables in data/Klut")

    if not real_tables_present():
        # A checkout made without Git LFS. klut_lfs_pointer_check covers it.
        print("  (data/Klut holds LFS pointers here: skipping; klut_lfs_pointer_check covers them)")
        return

    for c, (values, keys) in MINI.items():
        with open(os.path.join(KLUT, values_name(c)), newline="") as f:
            same_v = f.read() == values
        with open(os.path.join(KLUT, keys_name(c)), "rb") as f:
            same_k = f.read() == keys
        check(same_v and same_k,
              f"MINI's {c}-crossing tables match data/Klut (regenerate MINI if the tables changed)")

    rc, out, err = run(["--expanded"], stdin=TREFOIL + "k\n" + KNOT_5_1 + "k\n" + KNOT_13A1)
    assert_answers("data/Klut, all subtables", rc, out, err,
                   TWO_NAMES + ['K[13,1,1,"e/r"]'])
    check("(next to the executable)" in err, "data/Klut is found next to the executable", err)


def main():
    print("klut_table_check -- knoodleidentify vs damaged, partial and misplaced KLUT tables")

    if not os.path.exists(IDENTIFY):
        print(f"error: {IDENTIFY} is not built. Run `make all` in ../tools.")
        return 2

    with tempfile.TemporaryDirectory(prefix="klut-table-") as scratch:
        # Resolve /var -> /private/var and the like, so paths compare equal to
        # the ones the tool prints.
        scratch = os.path.realpath(scratch)
        # Runs that end in an error write a diagnostic report to the temp
        # directory; keep those in the scratch directory too.
        reports = os.path.join(scratch, "reports")
        os.mkdir(reports)
        os.environ["TMPDIR"] = reports
        test_intact(scratch)
        test_damage(scratch)
        test_partial(scratch)
        test_midway(scratch)
        test_notfound_exit(scratch)
        test_where(scratch)
    test_tree()

    print(f"\n{'KLUT TABLE CHECK OK' if fails == 0 else 'KLUT TABLE CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
