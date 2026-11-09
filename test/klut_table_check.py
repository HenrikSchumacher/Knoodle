#!/usr/bin/env python3
"""klut_table_check -- knoodleidentify refuses bad KLUT tables instead of answering from them.

knoodleidentify names knots by looking their keys up in data/Klut. Until
November 2026 it trusted those files completely, and a damaged table gave a
real-looking answer: a one-digit change to one count misnamed thousands of
knots with exit status 0, and a missing or truncated file showed up as
NotFound, which reads as a gap in the table. docs/klut-table-failure-audit.md
has the whole list. This test holds the tool to what it does now:

  * the tables are checked, and loaded in full, before any input is read.
    Missing, empty, unreadable or truncated files, a keys file whose size does
    not match its values file, a values file that does not parse strictly, and
    a key listed twice (Klut keeps only the first) all stop it with exit 1,
    empty stdout, and a message naming the file;
  * Klut_Manifest.txt, when present, must agree with the tables: sizes, key
    counts and the values files' SHA-256 on every run, the keys files' too with
    --verify-tables, and the table set must be the one the build expects unless
    --allow-other-tables. That catches what keeps the structure intact (an
    index or alternating flag changed, names reordered, a key bit flipped);
  * since everything is loaded at startup, a table changed while the tool runs
    changes nothing;
  * a NotFound is a nonzero exit, with --quiet too; a knot above
    --max-crossings is Unidentified, not NotFound;
  * --data-dir and $KNOODLE_KLUT_DIR are the only candidate when given, so a
    wrong one is an error rather than a quiet switch to other tables; an
    unreadable directory is an error, not a crash; tables found only in the
    current directory come with a warning; the Homebrew layout (a symlinked
    bin/ next to share/knoodle/Klut) works; and the summary says which tables
    answered.

The test carries its own copy of the 3- to 7-crossing tables (MINI below,
~800 bytes) and runs with --max-crossings=7, so it works in a checkout made
without Git LFS, as CI's is; it writes its own manifests for them. When this
checkout has the real tables it also checks that MINI still matches them, that
the full set verifies and names a 13-crossing knot, and that damage to a copy
of it is caught.

Run: python3 klut_table_check.py   (needs ../tools/knoodleidentify built)
"""

import hashlib
import os
import re
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
MANIFEST = "Klut_Manifest.txt"
SET_HEADER = os.path.join(REPO, "tools", "klut_table_set.hpp")

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


def manifest_text(d, crossings, version="1"):
    """A Klut_Manifest.txt for the tables in d, written from the format's
    description, independently of scripts/klut-manifest.py."""
    lines = []
    for c in crossings:
        for name in (keys_name(c), values_name(c)):
            with open(os.path.join(d, name), "rb") as f:
                data = f.read()
            if name.endswith(".bin"):
                nkeys = len(data) // c
            else:
                nkeys = sum(int(l.split(b"\t")[1]) for l in data.splitlines())
            lines.append(f"{name} {len(data)} {nkeys} {hashlib.sha256(data).hexdigest()}")
    set_id = hashlib.sha256("".join(l + "\n" for l in lines).encode()).hexdigest()[:16]
    return set_id, (f"# written by klut_table_check\nklut-manifest {version}\nset {set_id}\n"
                    + "".join(l + "\n" for l in lines))


def write_manifest(d, crossings=tuple(MINI)):
    set_id, text = manifest_text(d, crossings)
    with open(os.path.join(d, MANIFEST), "w", newline="") as f:
        f.write(text)
    return set_id


def built_set():
    with open(SET_HEADER) as f:
        m = re.search(r'kKlutTableSet = "([0-9a-f]{16})"', f.read())
    return m.group(1) if m else None


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
    ("a key listed twice, under two knots", 7,
     lambda v, k: (v, k[:7] + k[:7] + k[14:]),
     [keys_name(7), 'Key 1 of', '(for K[7,1,1,"m/mr"]) repeats a key of K[7,1,1,"e/r"]']),
    ("the trefoil's mirror key replaced by its own", 3,
     lambda v, k: (v, k[:3] + k[:3]),
     [keys_name(3), '(for K[3,1,1,"m/mr"]) repeats a key of K[3,1,1,"e/r"]']),
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
        assert_refused(what, rc, out, err, d, "are damaged", *must_say)

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

    # --max-crossings sets the table range: 5_1 is above 4, so Unidentified.
    # It used to come back as NotFound, a "table gap", with exit status 1.
    rc, out, err = run([f"--data-dir={mini}", "--max-crossings=4"], stdin=KNOT_5_1)
    no_crash("5_1 with --max-crossings=4", rc, err)
    check(rc == 0, "5_1 with --max-crossings=4: exits 0", err)
    check(out.startswith("<| Unidentified[5,"), "5_1 with --max-crossings=4: Unidentified", out)
    check("NotFound" not in out + err, "5_1 with --max-crossings=4: no NotFound", out + err)

    # Asking for fewer subtables is fine: a trefoil only needs the 3s.
    rc, out, err = run([f"--data-dir={only3}", "--max-crossings=3", "--expanded"],
                       stdin=TREFOIL)
    assert_answers("only the 3-crossing pair, --max-crossings=3", rc, out, err,
                   [TWO_NAMES[0]])


def run_midway(d, between):
    """Identify a trefoil, call between(), then identify 5_1, in one run."""
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
    section("a table changed while the tool runs changes nothing")

    # Klut reads a subtable on first use. knoodleidentify now loads them all at
    # startup, after checking them, so the answers come from the checked
    # tables whatever happens to the files later. (Before that, a keys file
    # deleted mid-run came back as NotFound[5, ...], and a values file swapped
    # mid-run renamed 5_1 as its mirror.)
    d = write_mini(os.path.join(scratch, "midway-keys"))
    rc, first, rest, err = run_midway(d, lambda: os.remove(os.path.join(d, keys_name(5))))
    what = "keys file deleted mid-run"
    no_crash(what, rc, err)
    check(rc == 0, f"{what}: exits 0", err)
    check((first + rest).split() == TWO_NAMES, f"{what}: both knots named as before",
          first + rest)

    d = write_mini(os.path.join(scratch, "midway-values"))
    lines = MINI[5][0].splitlines(keepends=True)
    swapped = "".join([lines[1], lines[0]] + lines[2:])

    def swap():
        with open(os.path.join(d, values_name(5)), "w", newline="") as f:
            f.write(swapped)

    rc, first, rest, err = run_midway(d, swap)
    what = "values file replaced mid-run"
    no_crash(what, rc, err)
    check(rc == 0, f"{what}: exits 0", err)
    check((first + rest).split() == TWO_NAMES, f"{what}: 5_1 is still 5_1, not its mirror",
          first + rest)


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
    check("from the current directory" not in err,
          "Homebrew layout: no current-directory warning", err)

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


def test_manifest(scratch):
    section("Klut_Manifest.txt: contents and table set")

    mine = built_set()
    check(mine is not None, "tools/klut_table_set.hpp names a table set")
    allow = [f"--max-crossings={MINI_MAX}", "--allow-other-tables"]

    # No manifest: a warning, not an error (tables from before the manifest).
    d = write_mini(os.path.join(scratch, "no-manifest"))
    rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}", "--expanded"])
    assert_answers("no manifest", rc, out, err)
    check(f"there is no {MANIFEST}" in err, "no manifest: warns", err)
    check(", no manifest" in err, "no manifest: the summary says so", err)
    rc, out, err = run([f"--data-dir={d}", "--verify-tables", f"--max-crossings={MINI_MAX}"])
    assert_refused("--verify-tables without a manifest", rc, out, err,
                   "Cannot verify the KLUT tables", MANIFEST)

    # A manifest for MINI names MINI's own table set, which is not the one this
    # build expects.
    d = write_mini(os.path.join(scratch, "manifest-ok"))
    set_id = write_manifest(d)
    check(set_id != mine, "MINI's table set differs from the built one (else the next check is moot)")
    rc, out, err = run([f"--data-dir={d}", f"--max-crossings={MINI_MAX}"])
    assert_refused("another table set", rc, out, err, f"are table set {set_id}",
                   f"built for table set {mine}", "--allow-other-tables")

    rc, out, err = run([f"--data-dir={d}", "--expanded"] + allow)
    assert_answers("another table set, --allow-other-tables", rc, out, err)
    check(f"using KLUT table set {set_id}" in err, "--allow-other-tables: warns", err)
    check(f"table set {set_id}" in err.splitlines()[-1], "the summary names the table set", err)
    rc, out, err = run([f"--data-dir={d}", "--expanded", "--quiet"] + allow)
    assert_answers("--allow-other-tables --quiet", rc, out, err)
    check(err == "", "--allow-other-tables --quiet: nothing on stderr", err)

    rc, out, err = run([f"--data-dir={d}", "--verify-tables"] + allow, stdin="")
    no_crash("--verify-tables", rc, err)
    check(rc == 0, "--verify-tables on intact tables: exits 0", err)
    check(f"verified: table set {set_id}, 10 files, 54 keys, 3 to 7 crossings" in out,
          "--verify-tables: says what it verified", out)

    rc, out, err = run([f"--data-dir={d}", "--verify-tables", "input.txt"] + allow)
    assert_refused("--verify-tables with an input file", rc, out, err, "reads no input")

    # Damage that keeps the structure intact. The structural check passes all
    # of these; the values files' SHA-256, checked on every run, does not.
    def values_edit(old, new):
        return lambda v, k: (bump_count(v, old, new), k)

    lines7 = MINI[7][0].splitlines(keepends=True)
    same_count_swap = "".join(lines7[:2] + [lines7[3], lines7[2]] + lines7[4:])
    cases = [
        ("alternating flag flipped", values_edit('K[7,1,1,"e/r"]', 'K[7,1,0,"e/r"]')),
        ("index changed to an unused one", values_edit('K[7,7,1,"e/r"]', 'K[7,8,1,"e/r"]')),
        ("two names with equal counts swapped", lambda v, k: (same_count_swap, k)),
    ]
    for i, (what, damage) in enumerate(cases):
        d = write_mini(os.path.join(scratch, f"manifest-values{i}"))
        write_manifest(d)
        apply_damage(d, 7, damage)
        rc, out, err = run([f"--data-dir={d}"] + allow)
        assert_refused(what, rc, out, err, values_name(7), "SHA-256 differs",
                       f"do not match their manifest, {MANIFEST}")

    # A flipped bit in a key: the keys files are hashed only by
    # --verify-tables, so an ordinary run does not see it (a flip that made a
    # duplicate key would be caught at load). The flip below makes no
    # duplicate.
    d = write_mini(os.path.join(scratch, "manifest-keybit"))
    write_manifest(d)
    k7 = bytearray(MINI[7][1])
    k7[-1] ^= 1
    keys7 = [bytes(k7[i:i + 7]) for i in range(0, len(k7), 7)]
    check(len(set(keys7)) == len(keys7), "the flipped key duplicates no other (setup)")
    with open(os.path.join(d, keys_name(7)), "wb") as f:
        f.write(k7)
    rc, out, err = run([f"--data-dir={d}", "--quiet"] + allow)
    check(rc == 0, "flipped key bit: an ordinary run does not notice (known limit)", err)
    rc, out, err = run([f"--data-dir={d}", "--verify-tables"] + allow)
    assert_refused("flipped key bit, --verify-tables", rc, out, err, keys_name(7),
                   "SHA-256 differs")

    # The manifest itself damaged, or disagreeing with the files.
    def manifest_case(name, edit, *must_say):
        d = write_mini(os.path.join(scratch, f"manifest-{name}"))
        _, text = manifest_text(d, tuple(MINI))
        with open(os.path.join(d, MANIFEST), "w", newline="") as f:
            f.write(edit(text))
        rc, out, err = run([f"--data-dir={d}"] + allow)
        assert_refused(f"manifest {name.replace('-', ' ')}", rc, out, err, *must_say)

    def sub(old, new):
        return lambda t: re.sub(old, new, t, count=1, flags=re.M)

    manifest_case("set-line-edited", sub(r"^set [0-9a-f]{16}$", "set 0123456789abcdef"),
                  "is damaged", "does not match its file lines")
    manifest_case("newer-format", sub(r"^klut-manifest 1$", "klut-manifest 2"),
                  "is damaged", "format 2", "reads format 1")
    manifest_case("cut-short", lambda t: t[:-5], "is damaged", "does not end with a newline")
    manifest_case("missing-a-file", sub(r"^Klut_Values_06\.tsv .*\n", ""),
                  "is damaged", "does not match its file lines")
    manifest_case("size-off-by-one", lambda t: re.sub(
        r"^(Klut_Keys_05\.bin) 20 ", r"\1 21 ", t, count=1, flags=re.M),
                  "is damaged")  # the set line no longer matches either

    # A self-consistent manifest that lists a different size: rebuild the set
    # line too, so only the size disagrees.
    d = write_mini(os.path.join(scratch, "manifest-wrong-size"))
    _, text = manifest_text(d, tuple(MINI))
    body = [l for l in text.splitlines() if l and not l.startswith(("#", "klut-manifest", "set "))]
    body = [re.sub(r"^(Klut_Keys_05\.bin) 20 4 ", r"\1 25 5 ", l) for l in body]
    sid = hashlib.sha256("".join(l + "\n" for l in body).encode()).hexdigest()[:16]
    with open(os.path.join(d, MANIFEST), "w", newline="") as f:
        f.write(f"klut-manifest 1\nset {sid}\n" + "".join(l + "\n" for l in body))
    rc, out, err = run([f"--data-dir={d}"] + allow)
    assert_refused("manifest lists another size", rc, out, err, keys_name(5),
                   "20 bytes, but the manifest says 25")

    # A manifest that does not list a subtable the run needs.
    d = write_mini(os.path.join(scratch, "manifest-short"))
    write_manifest(d, crossings=(3, 4, 5, 6))
    rc, out, err = run([f"--data-dir={d}"] + allow)
    assert_refused("manifest without the 7s", rc, out, err,
                   keys_name(7) + ": not listed in the manifest")

    # The tree's manifest is not in LFS, so this runs in every checkout: it
    # must be self-consistent and name the set the build expects.
    with open(os.path.join(KLUT, MANIFEST)) as f:
        tree = f.read()
    body = [l for l in tree.splitlines() if l and not l.startswith(("#", "klut-manifest", "set "))]
    m = re.search(r"^set ([0-9a-f]{16})$", tree, flags=re.M)
    check(m is not None and len(body) == 22, f"data/Klut/{MANIFEST} has a set line and 22 files")
    if m:
        sid = hashlib.sha256("".join(l + "\n" for l in body).encode()).hexdigest()[:16]
        check(sid == m.group(1), f"data/Klut/{MANIFEST}: its set line matches its file lines")
        check(m.group(1) == mine,
              "tools/klut_table_set.hpp names data/Klut's table set "
              "(run scripts/klut-manifest.py after changing the tables)")


def real_tables_present():
    p = os.path.join(KLUT, values_name(3))
    if not os.path.exists(p):
        return False
    with open(p, "rb") as f:
        return f.read(len(LFS_HEADER)) != LFS_HEADER


def test_tree(scratch):
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
    check("warning" not in err, "data/Klut: no warning", err)

    rc, out, err = run(["--verify-tables"], stdin="")
    no_crash("data/Klut --verify-tables", rc, err)
    check(rc == 0, "data/Klut --verify-tables: exits 0", err)
    check(f"verified: table set {built_set()}, 22 files, 1816748 keys, 3 to 13 crossings" in out,
          "data/Klut --verify-tables: says what it verified", out)

    p = subprocess.run([sys.executable, os.path.join(REPO, "scripts", "klut-manifest.py"),
                        "--check"], capture_output=True, text=True)
    check(p.returncode == 0, "scripts/klut-manifest.py --check agrees with data/Klut",
          p.stdout + p.stderr)

    # Two of the audit's silent wrong-name cases, on a copy of the real tables.
    copy = os.path.join(scratch, "real-copy")
    shutil.copytree(KLUT, copy)
    v13 = os.path.join(copy, values_name(13))
    with open(v13, newline="") as f:
        original = f.read()
    with open(v13, "w", newline="") as f:
        f.write(original.replace('K[13,1,1,"e/r"]', 'K[13,1,0,"e/r"]', 1))
    rc, out, err = run([f"--data-dir={copy}"], stdin=KNOT_13A1)
    assert_refused("real tables, alternating flag flipped in 13a1's name", rc, out, err,
                   values_name(13), "SHA-256 differs")
    with open(v13, "w", newline="") as f:
        f.write(original)

    k13 = os.path.join(copy, keys_name(13))
    with open(k13, "r+b") as f:
        f.seek(13 * 1000000)
        b = f.read(1)
        f.seek(13 * 1000000)
        f.write(bytes([b[0] ^ 1]))
    rc, out, err = run([f"--data-dir={copy}", "--verify-tables"], stdin="")
    assert_refused("real tables, one key bit flipped, --verify-tables", rc, out, err,
                   keys_name(13), "SHA-256 differs")


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
        test_manifest(scratch)
        test_tree(scratch)

    print(f"\n{'KLUT TABLE CHECK OK' if fails == 0 else 'KLUT TABLE CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
