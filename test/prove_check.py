#!/usr/bin/env python3
"""knoodleprove: the picture-independent claims of a move trace.

Build: a kind=script row in test/manifest.tsv (runs the real binaries).

The two tools split the claims of a record between them, and the split is the
thing to protect:

  * THE SPLIT. knoodledraw --trace --verify reports `drawing:` and nothing
    else -- the two deletions are the one claim about a picture. knoodleprove
    reports everything else and never a `drawing:` line. Neither tool may
    quietly grow the other's half: a claim that moved would otherwise be a
    claim nobody makes.
  * IT CAN FAIL. A witness that lies (witness_rec_anchor_lie), a snapshot that
    is not what the move produces, a descriptor that does not parse: each must
    be a MISMATCH and exit 1. A checker that cannot fail proves nothing.
  * HONEST COVERAGE. A move kind with no checker (redraw) is reported
    UNCHECKED, never skipped in silence.

r1 has a checker of its own, and for r1 the local checks ARE soundness (there
is no witness to demand), so this also exercises the curl fixtures: the
ordinary case, the spinoff, and a `loop` that does not name a loop arc.

Fixtures are the committed ones: test/trace_example.txt, test/r1_example.txt,
and the real witnessed middlepass records embedded in test/witness_fixtures.hpp
(read out of the header, so there is one copy of each).
"""

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PROVE = os.path.join(HERE, "..", "tools", "knoodleprove")
DRAW = os.path.join(HERE, "..", "tools", "knoodledraw")
TRACE_EXAMPLE = os.path.join(HERE, "trace_example.txt")
R1_EXAMPLE = os.path.join(HERE, "r1_example.txt")
R1_TRACE = os.path.join(HERE, "r1_trace_example.txt")
R1_SPINOFF = os.path.join(HERE, "r1_spinoff_example.txt")
WITNESS_HPP = os.path.join(HERE, "witness_fixtures.hpp")

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


def run(binary, args, stdin_text=None):
    p = subprocess.run([binary] + args, input=stdin_text,
                       capture_output=True, text=True, timeout=120)
    return p.returncode, p.stdout, p.stderr


def verify_lines(text):
    return [ln for ln in text.split("\n") if ln.startswith("#verify")]


def witness_records():
    src = open(WITNESS_HPP).read()
    return re.findall(r'static const char \* (\w+) = R"TRACE\((.*?)\)TRACE";',
                      src, re.S)


def read(path):
    with open(path) as f:
        return f.read()


# -- the split ---------------------------------------------------------------

def check_split(name, stream):
    """Each tool reports its own half, and only its own half."""
    _, dout, derr = run(DRAW, ["--trace", "--verify"], stream)
    draw_claims = verify_lines(dout + derr)
    stray = [ln for ln in draw_claims if " drawing: " not in ln]
    check(not stray, f"{name}: knoodledraw --verify reports only `drawing:`",
          "\n".join(stray))

    prc, pout, perr = run(PROVE, [], stream)
    stray = [ln for ln in verify_lines(pout) if " drawing: " in ln]
    check(not stray, f"{name}: knoodleprove reports no `drawing:` claim",
          "\n".join(stray))
    check("#verify" not in perr,
          f"{name}: knoodleprove's report goes to stdout", perr)
    return prc, pout


trace_example = read(TRACE_EXAMPLE)
r1_example = read(R1_EXAMPLE)
records = witness_records()
check(len(records) >= 8, "witness_fixtures.hpp yields its records",
      f"found {len(records)}")

check_split("trace_example", trace_example)
check_split("r1_example", r1_example)
for name, body in records:
    check_split(name, body)

# -- verdicts ---------------------------------------------------------------

# The claims knoodledraw no longer makes, which must therefore be knoodleprove's.
rc, out, _ = run(PROVE, [], records[0][1])
for claim in ("disk (V0)", "classes (V4)", "labels (V2/V3/V5)"):
    check(f"{claim}: VERIFIED" in out,
          f"the witness claim `{claim}` is knoodleprove's", out)
_, dout, derr = run(DRAW, ["--trace", "--verify"], records[0][1])
check("V0" not in dout + derr, "knoodledraw makes no witness claim", dout + derr)

rc, out, _ = run(PROVE, [TRACE_EXAMPLE])
check(rc == 0, "trace_example (as a FILE argument): exit 0", out)
check("#verify step 0 trace: VERIFIED" in out,
      "trace_example: the pass move produces the next snapshot", out)
check("#verify step 1 move: UNCHECKED (no checker for kind=redraw yet)" in out,
      "trace_example: redraw is reported UNCHECKED, not skipped", out)

rc, out, _ = run(PROVE, ["-"], r1_example)
check(rc == 0, "r1_example (stdin via '-'): exit 0", out)
check("#verify step 0 r1: VERIFIED (loop arc 4 at crossing 3" in out,
      "r1_example: the curl's local checks pass, and name the loop", out)

# -- r1 -----------------------------------------------------------------------

# The two-record fixtures: record 1 was produced by the LIBRARY's LoopRemover,
# so `trace: VERIFIED` is two independent implementations agreeing.
_, dout, derr = run(DRAW, ["--trace", "--verify"], read(R1_TRACE))
check("#verify step 0 drawing: VERIFIED (the deletion)" in dout + derr,
      "r1: knoodledraw checks the ONE deletion of the r1 picture", dout + derr)

rc, out, _ = run(PROVE, [R1_TRACE])
check(rc == 0, "r1_trace_example: exit 0", out)
check("#verify step 0 trace: VERIFIED" in out,
      "r1_trace_example: the surgery produces the next snapshot", out)

drc, dout, derr = run(DRAW, ["--trace", "--verify"], read(R1_SPINOFF))
check(drc == 1 and "diagram components" in derr,
      "r1 spinoff: knoodledraw refuses a split snapshot (it cannot draw one)",
      f"exit {drc}\n{derr}")

rc, out, _ = run(PROVE, [R1_SPINOFF])
check(rc == 0, "r1_spinoff_example: exit 0", out)
check("the component comes free" in out,
      "r1_spinoff_example: the spinoff is named in the r1 verdict", out)
check("split: 1 crossingless component(s) came free" in out,
      "r1_spinoff_example: the freed component is reported", out)
check("spinoffs: VERIFIED (1 reported, 1 from the surgery)" in out,
      "r1_spinoff_example: #spinoffs agrees with the surgery", out)
check("#verify step 0 trace: VERIFIED" in out,
      "r1_spinoff_example: the surgery produces the next snapshot", out)

# A `loop` darc that is not a loop arc must be caught, not waved through.
r1_bad = r1_example.replace("#move kind=r1 loop=9", "#move kind=r1 loop=1")
check(r1_bad != r1_example, "r1: the substitution applied")
rc, out, _ = run(PROVE, [], r1_bad)
check(rc == 1, "r1 with a non-loop arc: exit 1", out)
check("#verify step 0 r1: MISMATCH" in out and "not a loop arc" in out,
      "r1 with a non-loop arc: MISMATCH, and says why", out)

# The other side of `loop` is not a monogon, so naming it must fail too.
r1_other = r1_example.replace("#move kind=r1 loop=9", "#move kind=r1 loop=8")
rc, out, _ = run(PROVE, [], r1_other)
check(rc == 1, "r1 naming the non-monogon side: exit 1", out)
check("#verify step 0 r1: MISMATCH" in out and "monogon" in out,
      "r1 naming the non-monogon side: MISMATCH about the monogon", out)

# An r1 whose next snapshot is not what the move produces: keep the curl
# record, but follow it with the OTHER fixture's result (a trefoil, 3
# crossings, where this move leaves 4).
def records_of(text):
    return [b for b in text.split("\n\n") if b.strip()]

spliced = (records_of(read(R1_TRACE))[0] + "\n\n"
           + records_of(read(R1_SPINOFF))[1] + "\n")
rc, out, _ = run(PROVE, [], spliced)
check(rc == 1, "r1 followed by the wrong snapshot: exit 1", out)
check("#verify step 0 trace: MISMATCH" in out,
      "r1 followed by the wrong snapshot: the trace claim fails", out)
check("4 crossings expected, 3 found" in out,
      "r1 followed by the wrong snapshot: the counts are named", out)

for name, body in records:
    rc, out, _ = run(PROVE, [], body)
    if name == "witness_rec_anchor_lie":
        check(rc == 1, f"{name}: the lie exits 1", out)
        check(re.search(r"labels \(V2/V3/V5\): MISMATCH", out) is not None,
              f"{name}: the lie is caught at V2/V3/V5", out)
    else:
        check(rc == 0, f"{name}: exit 0", out)
        for claim in ("disk (V0)", "classes (V4)", "labels (V2/V3/V5)"):
            check(f"{claim}: VERIFIED" in out, f"{name}: {claim} VERIFIED", out)

# -- it can fail ------------------------------------------------------------

# A snapshot the move does NOT produce: replace the record after the pass move
# with r1_example's 5-crossing diagram -- valid, just not the trefoil the move
# makes. (Flipping one sign is no good: a signed PD code with one sign flipped
# is not a different diagram, it is an invalid one, and is refused as such.)
TREFOIL_ROWS = "0\t4\t1\t3\t1\n2\t0\t3\t5\t1\n4\t2\t5\t1\t1"
other_rows = "\n".join(ln for ln in r1_example.split("\n")
                       if ln and not ln.startswith("#"))
blocks = trace_example.split("\n\n")
tampered_block = blocks[1].replace(TREFOIL_ROWS, other_rows, 1)
check(tampered_block != blocks[1], "tamper: the substitution applied")
tampered = "\n\n".join([blocks[0], tampered_block] + blocks[2:])
rc, out, _ = run(PROVE, [], tampered)
check(rc == 1, "tampered snapshot: exit 1", out)
check("#verify step 0 trace: MISMATCH" in out,
      "tampered snapshot: the trace claim is a MISMATCH", out)

# A pass descriptor that does not parse is a MISMATCH, not a silent skip.
broken = trace_example.replace("strand=1,3 depart=1", "strand=1,3 depart=", 1)
check(broken != trace_example, "broken descriptor: the substitution applied")
rc, out, _ = run(PROVE, [], broken)
check(rc == 1, "unparseable descriptor: exit 1", out)
check("#verify step 0 descriptor: MISMATCH" in out,
      "unparseable descriptor: reported as a MISMATCH", out)

# A record carrying no diagram is the next STATE (a crossingless summand), so a
# pending claim must be answered against it, not carried over it. Splice one in
# after trace_example's pass move: the move claims 3 crossings, the stream says
# the summand is crossingless, and that is a MISMATCH -- it used to be compared
# against the record two steps later and reported VERIFIED.
recs = [b for b in trace_example.split("\n\n") if b.strip()]
crossingless = recs[0] + "\n\n#step n=9 summand=0\n\n" + "\n\n".join(recs[1:]) + "\n"
rc, out, _ = run(PROVE, [], crossingless)
check(rc == 1, "a crossingless record answers the pending claim: exit 1", out)
check("trace: MISMATCH (3 crossings expected, and the next record carries no"
      " diagram)" in out,
      "a crossingless record answers the pending claim, not a later one", out)

# A malformed stream is an error with a line number.
rc, out, err = run(PROVE, [], "#trace v=1\n#state lines=2\nnot a diagram\n")
check(rc == 1, "malformed stream: exit 1", out + err)
check("knoodleprove:" in err and "trace line" in err,
      "malformed stream: the error names a line", err)

# -- the CLI ----------------------------------------------------------------

rc, _, err = run(PROVE, ["--help"])
check(rc == 0 and "Usage: knoodleprove" in err, "--help: exit 0 with usage", err)
rc, _, err = run(PROVE, ["--bogus"])
check(rc == 1 and "unknown option" in err, "unknown option: exit 1", err)
rc, _, err = run(PROVE, [os.path.join(HERE, "no-such-trace.txt")])
check(rc == 1 and "cannot open" in err, "missing file: exit 1", err)

print(f"prove_check: ({checks} checks, {fails} failed)")
sys.exit(1 if fails else 0)
