#!/usr/bin/env python3
"""knoodledraw --format=wl: the pass-move and trace paths.

Build: a kind=script row in test/manifest.tsv (runs the real binary).

WHY THIS EXISTS. --format=wl used to be silently incompatible with every
pass-move flag. The wl branch in DrawKnot returned before the overlay code ran,
so `--move` and `--trace` died on the fail-loud guard for a move that had never
been *tried* ("no drawable summands"), and `--find-pass` was a silent no-op --
plain geometry, exit 0, no corridor and no warning. Nothing caught any of it,
because no test anywhere asserted on a single byte of wl output: the only
--format=wl coverage was a spelling check in cli_contract_check.py that runs
`--format=WL --help`, and --help short-circuits before any drawing happens.

So this asserts on the OUTPUT, and specifically on the two properties a
geometry consumer depends on:

  * stdout is pure WL -- one self-contained association per line, every line
    starting with "<|". Commentary (echoed headers, --verify reports) goes to
    stderr in wl mode. The paclet's reader already assumes this: runGeometry
    keeps only lines matching StringStartsQ["<|"].
  * a routed pass move is present as geometry, under "Pass".

These are CLI-level properties -- argument wiring and stream routing -- so they
cannot be reached from an in-process test the way pass_view_check reaches the
two-deletions contract.
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DRAW = os.path.join(HERE, "..", "tools", "knoodledraw")
TRACE_FIXTURE = os.path.join(HERE, "trace_example.txt")

# The trefoil carried by trace_example.txt's first record, as 5-column signed
# PD, plus the pass move that record prescribes.
TREFOIL = "0\t4\t1\t3\t1\n2\t0\t3\t5\t1\n4\t2\t5\t1\t1\n"
TREFOIL_MOVE = "kind=pass strand=1,3 depart=1 cross=6:u land=3"

checks = 0
fails = 0


def check(condQ, label, detail=""):
    global checks, fails
    checks += 1
    if not condQ:
        fails += 1
        print(f"FAIL: {label}")
        if detail:
            for line in str(detail).rstrip("\n").split("\n")[:12]:
                print(f"      {line}")
    return condQ


def run(args, stdin_text=None):
    """Run knoodledraw; return (rc, stdout, stderr)."""
    p = subprocess.run(
        [DRAW] + args,
        input=stdin_text,
        capture_output=True,
        text=True,
        timeout=120,
    )
    return p.returncode, p.stdout, p.stderr


def wl_lines(out):
    return [ln for ln in out.split("\n") if ln.strip()]


def assoc_balancedQ(line):
    """A cheap structural sanity check: <| ... |> nest and close."""
    if not (line.startswith("<|") and line.endswith("|>")):
        return False
    depth = 0
    i = 0
    while i < len(line) - 1:
        two = line[i:i + 2]
        if two == "<|":
            depth += 1
            i += 2
            continue
        if two == "|>":
            depth -= 1
            if depth < 0:
                return False
            i += 2
            continue
        i += 1
    return depth == 0


def test_plain_wl_still_works():
    """The ordinary per-summand emit, unchanged. Guards the paclet's input."""
    rc, out, err = run(["--format=wl"], TREFOIL)
    if not check(rc == 0, "plain --format=wl exits 0", err):
        return
    lines = wl_lines(out)
    if not check(len(lines) == 1, f"plain wl emits one association, got {len(lines)}", out):
        return
    check(assoc_balancedQ(lines[0]), "plain wl association is balanced", lines[0])
    for key in ("BoundingBox", "Arcs", "Crossings", "Faces"):
        check(f'"{key}"' in lines[0], f"plain wl carries {key}", lines[0])
    # No pass move was asked for, so the key must be absent -- a consumer tests
    # for "Pass" rather than for a flag.
    check('"Pass"' not in lines[0], "plain wl carries no Pass", lines[0])


def test_move_wl_emits_pass():
    """--move + --format=wl: used to exit 1 having emitted nothing at all."""
    rc, out, err = run(["--format=wl", f"--move={TREFOIL_MOVE}"], TREFOIL)
    if not check(rc == 0, "--move --format=wl exits 0", err):
        return
    lines = wl_lines(out)
    if not check(len(lines) == 1, f"--move wl emits one association, got {len(lines)}", out):
        return
    line = lines[0]
    check(assoc_balancedQ(line), "--move wl association is balanced", line)
    if not check('"Pass"' in line, "--move wl carries a Pass member", line):
        return
    for key in ("Kind", "View", "Strand", "Depart", "Land",
                "Corridor", "Points", "Dots", "Anchors"):
        check(f'"{key}"' in line, f"Pass carries {key}", line)
    # The corridor must actually be a routed polyline, not an empty stub.
    check('"Points"->{{' in line, "Pass corridor has points", line)

    # "Disk" is opt-in: absent by default, present under --pass-disk.
    check('"Disk"' not in line, "no Disk without --pass-disk", line)

    rc, out, err = run(["--format=wl", "--pass-disk", f"--move={TREFOIL_MOVE}"], TREFOIL)
    dlines = wl_lines(out)
    if check(rc == 0 and len(dlines) == 1,
             "--pass-disk --format=wl emits one association", err or out):
        check('"Disk"->{{' in dlines[0], "--pass-disk adds a shaded Disk", dlines[0])
        check(assoc_balancedQ(dlines[0]), "--pass-disk association is balanced", dlines[0])


def test_find_pass_wl_is_not_silent():
    """--find-pass + --format=wl was a silent no-op: the flag never ran.

    Either outcome is legitimate -- a corridor is found, or none exists -- but
    the run must say which on stderr rather than emitting bare geometry and
    exiting 0 with nothing to show for the flag.
    """
    rc, out, err = run(["--format=wl", "--find-pass=1,3"], TREFOIL)
    check(rc == 0, "--find-pass --format=wl exits 0", err)
    lines = wl_lines(out)
    if not check(len(lines) == 1, f"--find-pass wl emits one association, got {len(lines)}", out):
        return
    check(assoc_balancedQ(lines[0]), "--find-pass wl association is balanced", lines[0])
    saidQ = ("--find-pass" in err) or ("no reducing pass" in err)
    check(saidQ, "--find-pass reports what it found", err or "(no stderr)")
    # And it agrees with itself: a reported corridor means a Pass member.
    if "no reducing pass" not in err:
        check('"Pass"' in lines[0], "a found corridor appears as Pass", lines[0])


def test_trace_wl_is_pure_wl():
    """--trace + --format=wl: used to die on the first pass record."""
    if not check(os.path.exists(TRACE_FIXTURE), "trace fixture exists", TRACE_FIXTURE):
        return

    with open(TRACE_FIXTURE, encoding="utf-8") as f:
        want_records = sum(1 for ln in f if ln.startswith("#step"))

    rc, out, err = run(["--trace", "--format=wl", TRACE_FIXTURE])
    if not check(rc == 0, "--trace --format=wl exits 0", err):
        return

    lines = wl_lines(out)
    check(len(lines) == want_records,
          f"one association per record ({want_records} expected, {len(lines)} found)", out)

    # The property the whole mode rests on: nothing but geometry on stdout.
    for i, line in enumerate(lines):
        if not check(line.startswith("<|"), f"stdout line {i} starts an association", line):
            break
        if not check(assoc_balancedQ(line), f"stdout line {i} is balanced", line):
            break
    check(not any(ln.lstrip().startswith("#") for ln in lines),
          "no commentary on stdout", out)

    # Each record carries its own context, so nothing must be correlated across
    # lines to know which picture this is.
    for i, line in enumerate(lines):
        check('"Step"->' in line, f"record {i} carries Step", line)
        check('"Headers"->' in line, f"record {i} carries Headers", line)

    # trace_example.txt's first record prescribes a pass move; its last is a
    # bare snapshot -- geometry, with nothing claimed about a move.
    if lines:
        check('"Pass"' in lines[0], "the pass record carries a Pass member", lines[0])
        check('"Move"->' in lines[0], "the pass record carries its Move", lines[0])
        check('"Move"->' not in lines[-1], "the terminal record carries no Move", lines[-1])
        check('"Pass"' not in lines[-1], "the terminal record carries no Pass", lines[-1])
        check('"BoundingBox"' in lines[-1], "the terminal record still carries geometry", lines[-1])

    # The headers are echoed for the human, just not onto stdout.
    check("#step" in err, "headers still reach the terminal (stderr)", err or "(no stderr)")


def test_trace_verify_keeps_stdout_clean():
    """--verify is commentary too, however it rules.

    The exit code is deliberately not asserted: whether this fixture's move
    reproduces the next record's snapshot is the fixture's business, not this
    test's. What must hold either way is that the verdicts stay off stdout.
    """
    rc, out, err = run(["--trace", "--verify", "--format=wl", TRACE_FIXTURE])
    lines = wl_lines(out)
    check(not any(ln.lstrip().startswith("#") for ln in lines),
          "--verify writes no commentary to stdout", out)
    for i, line in enumerate(lines):
        if not check(assoc_balancedQ(line), f"--verify stdout line {i} is balanced", line):
            break
    check("#verify" in err, "--verify reports on stderr", err or "(no stderr)")


def test_trace_unicode_unchanged():
    """The default trace rendering must not have moved."""
    rc, out, err = run(["--trace", TRACE_FIXTURE])
    check(rc == 0, "--trace (unicode) exits 0", err)
    check("#step" in out, "--trace (unicode) still echoes headers to stdout", out[:400])
    check("<|" not in out, "--trace (unicode) emits no WL", out[:400])


def test_trace_unknot_record():
    """A record with no diagram is still a record, and still gets a line.

    A 0-crossing summand cannot be drawn, but a consumer stepping through a
    trace must not have to infer that a step went missing -- so wl emits an
    "Unknot" marker carrying the same context every other record carries.
    (trace_example.txt has no such record, so this builds one.)
    """
    trace = ("#trace v=0\n"
             "#step n=0 summand=0\n"
             "#comment a summand with no crossings\n"
             "\n")

    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
        f.write(trace)
        path = f.name

    try:
        rc, out, err = run(["--trace", "--format=wl", path])
        if not check(rc == 0, "0-crossing trace record exits 0", err):
            return
        lines = wl_lines(out)
        if not check(len(lines) == 1, f"0-crossing record emits one line, got {len(lines)}", out):
            return
        check(assoc_balancedQ(lines[0]), "unknot association is balanced", lines[0])
        check('"Unknot"->True' in lines[0], "0-crossing record is an Unknot marker", lines[0])
        check('"Step"->0' in lines[0], "unknot record still carries Step", lines[0])
        check('"Headers"->' in lines[0], "unknot record still carries Headers", lines[0])
    finally:
        os.unlink(path)


def main():
    if not os.path.exists(DRAW):
        print(f"draw_trace_check: {DRAW} not built; nothing to check")
        return 0

    test_plain_wl_still_works()
    test_move_wl_emits_pass()
    test_find_pass_wl_is_not_silent()
    test_trace_wl_is_pure_wl()
    test_trace_unknot_record()
    test_trace_verify_keeps_stdout_clean()
    test_trace_unicode_unchanged()

    status = "DRAW TRACE CHECK OK" if fails == 0 else "DRAW TRACE CHECK FAILED"
    print(f"\n{status} ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
