#!/usr/bin/env python3
"""knoodledraw: drawing an R1 (curl removal).

Build: a kind=script row in test/manifest.tsv (runs the real binary).

WHY THIS EXISTS. An R1 is not a pass move -- the pass grammar refuses it by
name, because check 4's distinct-anchors clause exists precisely to exclude
"an R_I curl at the end of a strand, and its relatives" (docs/move-descriptor
.md). So it has its own descriptor, `#move kind=r1 loop=<da>`, its own
overlay, and a weaker contract: ONE deletion rather than two, since an R1 adds
nothing to the picture to delete back out.

The properties worth pinning here are the ones that are easy to get wrong and
silent when they are:

  * `loop` is a DARC, and L(loop) is the monogon that collapses. Naming the
    arc's OTHER darc must be refused, not silently collapse the wrong side --
    that check is the whole reason the field is a darc rather than an arc.
  * the after view really deletes the curl (strictly less ink, no marker left).
  * the monogon shading is opt-in under --pass-disk, and absent without it.
  * `--format=wl` with an r1 FAILS LOUD. The "R1" member is specified but not
    implemented, and emitting geometry with the move silently missing is
    exactly the bug --find-pass had on that path until 2026-09-16. This test
    locks the loud failure in until the emitter lands, at which point the
    expectation flips rather than quietly starting to pass.

The fixture's bare PD is derived from test/r1_example.txt by stripping its
headers, so the trace fixture and the plain-diagram fixture cannot drift apart
(and so no second fixture has to be named .tsv, which .gitattributes routes
through git-lfs).
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DRAW = os.path.join(HERE, "..", "tools", "knoodledraw")
TRACE_FIXTURE = os.path.join(HERE, "r1_example.txt")

# The fixture's curl: loop arc a=4 closes on crossing c=3; L(9) is the monogon.
R1_GOOD = "kind=r1 loop=9"
R1_OTHER_SIDE = "kind=r1 loop=8"   # the loop arc's other darc: not a monogon
R1_NOT_A_LOOP = "kind=r1 loop=0"   # arc 0 is not a loop arc

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
    p = subprocess.run([DRAW] + args, input=stdin_text,
                       capture_output=True, text=True, timeout=120)
    return p.returncode, p.stdout, p.stderr


def bare_pd():
    """The fixture's PD rows, with the trace headers stripped."""
    with open(TRACE_FIXTURE, encoding="utf-8") as f:
        rows = [ln for ln in f if ln.strip() and not ln.startswith("#")]
    return "".join(rows)


def ink(s):
    """Non-blank drawing cells, ignoring ANSI-free ASCII output."""
    return sum(1 for ch in s if ch not in " \n")


def test_r1_draws():
    pd = bare_pd()
    rc, out, err = run(["--ascii", "--move=" + R1_GOOD], pd)
    if not check(rc == 0, "r1 --move exits 0", err):
        return
    check(out.strip() != "", "r1 --move draws something", out)
    # --ascii has no colour, so the curl is marked with a letter instead.
    check("r" in out, "the curl is marked in --ascii", out)


def test_after_view_deletes_the_curl():
    """The one deletion: after = the diagram the move produces."""
    pd = bare_pd()
    rc_b, before, err_b = run(["--ascii", "--move=" + R1_GOOD], pd)
    rc_a, after, err_a = run(["--ascii", "--pass-view=after", "--move=" + R1_GOOD], pd)

    if not check(rc_b == 0 and rc_a == 0, "both views exit 0", err_b + err_a):
        return

    check("r" not in after, "the after view leaves no curl marker", after)
    check(after != before, "the after view differs from the before view", after)
    # Deleting a curl can only remove ink: the healed corner replaces a
    # crossing in place, and the rest of the loop is erased.
    check(ink(after) < ink(before),
          f"the after view has less ink ({ink(after)} < {ink(before)})", after)

    # `both` adds nothing to superpose for an R1, so it must equal `before`.
    rc_2, both, _ = run(["--ascii", "--pass-view=both", "--move=" + R1_GOOD], pd)
    check(rc_2 == 0 and both == before,
          "for an r1, --pass-view=both coincides with =before", both)


def test_monogon_shading_is_opt_in():
    pd = bare_pd()
    rc_d, disk, err_d = run(["--ascii", "--pass-disk", "--move=" + R1_GOOD], pd)
    rc_n, none, err_n = run(["--ascii", "--move=" + R1_GOOD], pd)

    if not check(rc_d == 0 and rc_n == 0, "shaded and unshaded both exit 0",
                 err_d + err_n):
        return
    check("." in disk, "--pass-disk shades the monogon", disk)
    check("." not in none, "no shading without --pass-disk", none)


def test_the_other_darc_is_refused():
    """Naming the loop arc's other darc must be refused, not silently wrong.

    This is what the field being a DARC buys: L(loop) picks the collapsing
    side, and the wrong side is a detectable error rather than a bad picture.
    """
    pd = bare_pd()
    rc, out, err = run(["--ascii", "--move=" + R1_OTHER_SIDE], pd)
    check(rc != 0, f"the other darc is refused (rc={rc})", out)
    check("monogon" in err, "the refusal says the face is not a monogon", err)
    check(out.strip() == "", "a refused r1 leaves no drawing behind", out)


def test_a_non_loop_arc_is_refused():
    pd = bare_pd()
    rc, out, err = run(["--ascii", "--move=" + R1_NOT_A_LOOP], pd)
    check(rc != 0, f"a non-loop arc is refused (rc={rc})", out)
    check("not a loop arc" in err, "the refusal says it is not a loop arc", err)


def test_wl_fails_loud_for_now():
    """Not implemented -- but refused, never silently dropped.

    When the "R1" emitter lands this flips to asserting the member is present.
    """
    pd = bare_pd()
    rc, out, err = run(["--format=wl", "--move=" + R1_GOOD], pd)
    check(rc != 0, f"--format=wl with an r1 fails loud (rc={rc})", out)
    check("<|" not in out, "no geometry is emitted with the move missing", out)
    check("R1" in err, "the refusal names what is missing", err)


def test_trace_draws_an_r1_record():
    if not check(os.path.exists(TRACE_FIXTURE), "trace fixture exists",
                 TRACE_FIXTURE):
        return

    rc, out, err = run(["--trace", "--ascii", TRACE_FIXTURE])
    if not check(rc == 0, "--trace on an r1 record exits 0", err):
        return
    check("kind=r1" in out, "the r1 move header is echoed", out)
    check("r" in out.replace("kind=r1", ""), "the curl is marked", out)

    rc_a, after, err_a = run(["--trace", "--ascii", "--pass-view=after",
                              TRACE_FIXTURE])
    check(rc_a == 0, "--trace --pass-view=after exits 0", err_a)
    check(after != out, "the trace after view differs from the before view",
          after)


def main():
    if not os.path.exists(DRAW):
        print(f"r1_draw_check: {DRAW} not built; nothing to check")
        return 0

    test_r1_draws()
    test_after_view_deletes_the_curl()
    test_monogon_shading_is_opt_in()
    test_the_other_darc_is_refused()
    test_a_non_loop_arc_is_refused()
    test_wl_fails_loud_for_now()
    test_trace_draws_an_r1_record()

    status = "R1 DRAW CHECK OK" if fails == 0 else "R1 DRAW CHECK FAILED"
    print(f"\n{status} ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
