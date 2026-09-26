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
  * HONEST COVERAGE. What is not checked says so; a move kind with no
    checker at all is UNCHECKED, never skipped.

redraw is checked in full (docs/move-descriptor.md, checks 1-5): the rotation
by equality, and the lattice witness E projected EXACTLY (LinkEmbedding_Int +
Prosector) to this snapshot and R*E to the next, colours kept. The link
fixture's two components are interchangeable, so a colour swap that an
uncoloured isomorphism would wave through is the tamper that matters most.

r1 has a checker of its own, and for r1 the local checks ARE soundness (there
is no witness to demand), so this also exercises the curl fixtures: the
ordinary case, the spinoff, a `loop` that does not name a loop arc, and the
LAST curl, which leaves nothing: an empty #result, a drawing with no
crossings, and a stream that ends there (test/r1_empty_result.trace,
test/r1_last_curl.trace -- middlestrands' ROUND-24 reproducers).

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
REDRAW = os.path.join(HERE, "redraw_example.trace")
REDRAW_LINK = os.path.join(HERE, "redraw_link_example.trace")
REDRAW_SPLIT = os.path.join(HERE, "redraw_split_refused.trace")
R1_SPINOFF = os.path.join(HERE, "r1_spinoff_example.txt")
R1_EMPTY_RESULT = os.path.join(HERE, "r1_empty_result.trace")
R1_LAST_CURL = os.path.join(HERE, "r1_last_curl.trace")
LINK_RESULT = os.path.join(HERE, "link_result_example.trace")
PASS_WCROSS = os.path.join(HERE, "pass_wcross_example.trace")
HEALED_CURL = os.path.join(HERE, "middlepass_healed_curl.trace")
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
check("#verify step 1 redraw: VERIFIED" in out
      and "#verify step 1 projection: VERIFIED" in out
      and "#verify step 1 trace: VERIFIED (9 crossings expected, 9 found,"
          " colours kept)" in out,
      "trace_example: the redraw's lattice trefoil projects to this snapshot,"
      " and R*E to the next (3 -> 9 crossings)", out)

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

# The LAST curl (ROUND-24 §7): nothing is left, and each tool had a false
# negative on it. knoodleprove found no untouched crossing to seed the result
# match with; knoodledraw refused a drawing with no crossings in it. Two empty
# diagrams agree, and a stream that stops after an empty claim has continued
# exactly as claimed.
for path, label in [(R1_EMPTY_RESULT, "r1_empty_result"),
                    (R1_LAST_CURL, "r1_last_curl")]:
    rc, out, _ = run(PROVE, [path])
    check(rc == 0, f"{label}: exit 0", out)
    check("#verify step 0 trace: VERIFIED (0 crossings expected, and the"
          " stream ends)" in out,
          f"{label}: an empty claim is answered by the stream ending", out)
    drc, dout, derr = run(DRAW, ["--trace", "--verify"], read(path))
    check(drc == 0 and "#verify step 0 drawing: VERIFIED (the deletion)"
          in dout + derr,
          f"{label}: knoodledraw parses the crossing-free drawing",
          f"exit {drc}\n{derr}")

rc, out, _ = run(PROVE, [R1_EMPTY_RESULT])
check("#verify step 0 result: VERIFIED (both results are empty)" in out,
      "r1_empty_result: an empty #result agrees with an empty surgery", out)
check("colors:" not in out,
      "r1_empty_result: no colour verdict where there is no arc to carry one"
      " (spinoffs: says where the colour went)", out)

# ... and none of that is unconditional.
# (a) An empty claim followed by a record that still has a crossing.
preamble, last = records_of(read(R1_LAST_CURL))
rc, out, _ = run(PROVE, [], "\n\n".join(
    [preamble, last, last.replace("#step n=0", "#step n=1", 1)]) + "\n")
check(rc == 1 and "#verify step 0 trace: MISMATCH (0 crossings expected,"
      " 1 found)" in out,
      "an empty claim followed by a crossing: MISMATCH", out)
# (b) A claim with crossings in it is still UNCHECKED at the end of a stream.
rc, out, _ = run(PROVE, [], records_of(read(R1_TRACE))[0] + "\n")
check(rc == 0 and "#verify step 0 trace: UNCHECKED (no following record" in out,
      "a non-empty claim at the end of the stream: still UNCHECKED", out)
# (c) A #result that keeps the curl the move removes.
kept = read(R1_EMPTY_RESULT)
head, block = kept.rsplit("#result", 1)
block = (block.replace("crossing_count = 0", "crossing_count = 1", 1)
              .replace("arc_count = 0", "arc_count = 2", 1)
              .replace("C_state = {0}", "C_state = {1}", 1)
              .replace("A_state = {0,0}", "A_state = {1,1}", 1))
check(head + "#result" + block != kept, "kept-curl tamper: applied")
rc, out, _ = run(PROVE, [], head + "#result" + block)
check(rc == 1 and "result: MISMATCH -- crossing counts differ: 0 (ours) vs 1"
      " (theirs)" in out,
      "a #result that keeps the curl: MISMATCH", out)

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

# -- redraw -------------------------------------------------------------------

# `redraw` is restricted to the two cyclic axis permutations, which are integer
# matrices -- so the rotation is checked by EQUALITY, with no tolerance -- and
# its witness is a lattice curve, projected exactly.
rc, out, _ = run(PROVE, [REDRAW])
check(rc == 0, "redraw_example: exit 0", out)
check("redraw: VERIFIED (rotation x->y->z->x" in out,
      "redraw: the permitted rotation is recognised and named", out)
check("projection: VERIFIED (E has 1 component(s), 4 lattice vertices" in out,
      "redraw: project(E) is the curl snapshot (check 1)", out)
check("spinoffs: VERIFIED (colours 0 declared, 0 crossingless" in out,
      "redraw: the rotation frees the curl, and #spinoffs names it", out)
check("trace: VERIFIED (0 crossings expected, and the next record carries no"
      " diagram)" in out,
      "redraw: the empty next record answers project(R*E) (check 2)", out)

rc, out, _ = run(PROVE, [REDRAW_LINK])
check(rc == 0, "redraw_link_example: exit 0", out)
check("projection: VERIFIED (E has 2 component(s), 8 lattice vertices" in out,
      "redraw link: project(E) is the Hopf snapshot, colours kept", out)
check("trace: VERIFIED (4 crossings expected, 4 found, colours kept)" in out,
      "redraw link: project(R*E) is the next snapshot, colours kept", out)

rc, out, _ = run(PROVE, [REDRAW_SPLIT])
check(rc == 1 and "rotated: MISMATCH" in out
      and "falls apart into 2 diagram components" in out,
      "redraw split: a projection that falls apart is refused (check 5)", out)
check("projection: VERIFIED" in out,
      "redraw split: ... and it is refused for THAT reason, not an earlier one",
      out)

def tampered(path, old, new, count=1):
    text = read(path)
    check(text.count(old) == count, f"tamper target present: {old!r}")
    return text.replace(old, new)

redraw_link = read(REDRAW_LINK)
for stream, verdict, why, label in [
    # The Hopf link is symmetric: swapping the colours in the NEXT snapshot
    # leaves a diagram that is isomorphic if colours are ignored. Check 4 is
    # exactly the refusal to ignore them.
    (tampered(REDRAW_LINK, "A_color = {3,3,3,3,0,0,0,0}",
              "A_color = {0,0,0,0,3,3,3,3}"),
     "trace: MISMATCH", "keeping every arc's colour",
     "colours swapped in the next snapshot"),
    (redraw_link.replace("#component color=3", "#component color=X")
                .replace("#component color=0", "#component color=3")
                .replace("#component color=X", "#component color=0"),
     # Caught at the NEXT record, not this one, and that is correct: the
     # 2-crossing Hopf diagram is itself symmetric, so a colour-kept
     # isomorphism to the step-0 snapshot exists either way. The 4-crossing
     # view has no such symmetry.
     "trace: MISMATCH", "keeping every arc's colour",
     "colours swapped between the embedding's components"),
    (tampered(REDRAW_LINK, "#component color=3", "#component color=5"),
     "projection: MISMATCH", "the embedding's components are colours 0,5 but"
     " the snapshot's are 0,3",
     "an embedding colour the snapshot does not have"),
    (tampered(REDRAW_LINK, "1\t3\t1\n", "1\t0\t1\n"),
     "projection: MISMATCH", "",
     "one vertex moved (E no longer projects to the snapshot)"),
    (tampered(REDRAW, "#spinoffs colors=0\n", ""),
     "spinoffs: MISMATCH", "0 crossingless in project(R*E)",
     "a freed component not declared"),
    (tampered(REDRAW, "#spinoffs colors=0\n", "#spinoffs n=1\n"),
     "spinoffs: MISMATCH", "the bare count cannot say",
     "spinoffs given as a bare count"),
]:
    rc, out, _ = run(PROVE, [], stream)
    check(rc == 1, f"redraw with {label}: exit 1", out)
    check(verdict in out and why in out, f"redraw with {label}: {verdict}", out)

# The grammar: integer coordinates, per-component blocks. A malformed witness
# is a parse error naming the line, not a verdict.
for stream, why, label in [
    (tampered(REDRAW, "2\t2\t0\n", "2.0\t2\t0\n"), "is not an integer",
     "a non-integer coordinate"),
    (tampered(REDRAW, "#embedding components=1\n#component color=0 rows=4",
              "#embedding rows=4"), "bad '#embedding' header",
     "the retired flat '#embedding rows=' form"),
    (tampered(REDRAW_LINK, "#component color=3", "#component color=0"),
     "names colour 0 twice", "two components with one colour"),
    (tampered(REDRAW, "2\t2\t0\n", "2\t2\t2147483648\n"),
     "out of range", "a coordinate outside int32"),
]:
    rc, _, err = run(PROVE, [], stream)
    check(rc == 1 and why in err, f"redraw witness with {label}: parse error",
          err)

redraw = read(REDRAW)
for bad, label in [
    ("1,0,0,0,1,0,0,0,1", "the identity (which changes nothing)"),
    ("0,1,0,1,0,0,0,0,1", "a swap (det = -1, a reflection)"),
    ("0,0,1,1,0,0,0,1",   "eight entries"),
    ("0,0,1,1,0,0,0,1,0.5", "a non-integer entry"),
]:
    stream = redraw.replace("rot=0,0,1,1,0,0,0,1,0", "rot=" + bad)
    check(stream != redraw, f"redraw tamper applied: {label}")
    rc, out, _ = run(PROVE, [], stream)
    check(rc == 1, f"redraw with {label}: exit 1", out)
    check("redraw: MISMATCH" in out, f"redraw with {label}: MISMATCH", out)

# -- colours ------------------------------------------------------------------

# Structure is not the whole claim: on a link, a component carries its colour
# across a move, and an applier that renumbers components has changed the
# labelling that tells the components apart. The fixture is a 2-component link
# with a #result and a #spinoffs colour list.
link = read(LINK_RESULT)
rc, out, _ = run(PROVE, [LINK_RESULT])
check(rc == 0, "link_result_example: exit 0", out)
check("colors: VERIFIED (2 component colours carried through the move)" in out,
      "link: the colours are checked, and the count of them is reported", out)
check("spinoffs: VERIFIED (colours 0 reported, 0 from the surgery)" in out,
      "link: #spinoffs colors= is compared as a LIST, not just a count", out)

# Recolour one arc of the applier's #result: the structure is untouched, so
# `result:` must still pass and `colors:` must catch it. That split is the
# point -- a diagram can be right while its components are mislabelled.
# rsplit: the fixture's own provenance comment mentions "#result" too, and the
# block we want is the header line, which comes last.
head, result_block = link.rsplit("#result", 1)
m = re.search(r"A_color = \{([^}]*)\}", result_block)
vals = m.group(1).split(",")
i = next(k for k, v in enumerate(vals) if v.strip() == "1")
vals[i] = "0"
recoloured = (head + "#result" + result_block[:m.start(1)] + ",".join(vals)
              + result_block[m.end(1):])
check(recoloured != link, "colour tamper: the substitution applied")
rc, out, _ = run(PROVE, [], recoloured)
check(rc == 1, "a recoloured #result: exit 1", out)
check("result: VERIFIED" in out,
      "a recoloured #result: the STRUCTURE still agrees", out)
check("colors: MISMATCH" in out and "which component it belongs to" in out,
      "a recoloured #result: the colours do not, and the arc is named", out)

# A #spinoffs colour list naming the wrong component.
wrong_spin = link.replace("#spinoffs colors=0", "#spinoffs colors=1")
check(wrong_spin != link, "spinoff colour tamper: the substitution applied")
rc, out, _ = run(PROVE, [], wrong_spin)
check(rc == 1, "#spinoffs naming the wrong colour: exit 1", out)
check("spinoffs: MISMATCH (colours 1 reported, 0 from the surgery)" in out,
      "#spinoffs naming the wrong colour: caught, with both lists", out)

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
wrong_next = "\n\n".join([blocks[0], tampered_block] + blocks[2:])
rc, out, _ = run(PROVE, [], wrong_next)
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

# A descriptor that PARSES but is not well-formed is a fault in the record,
# not a limit of the surgery: a MISMATCH and exit 1, never UNCHECKED and exit 0
# (ROUND-22 §2). The clean stream's step 2 is a W-crossing pass tagged `o,o`;
# `u,o` is a middlepass wearing a pass move's clothes, and `u,u` is uniform but
# opposite to W's role -- a crossing change.
rc, out, _ = run(PROVE, [PASS_WCROSS])
check(rc == 0, "pass_wcross_example: exit 0", out)
check(len(re.findall(r"result: VERIFIED", out)) >= 4,
      "pass_wcross_example: every pass record's #result is VERIFIED", out)
CLEAN_W = "#move kind=pass strand=31,33,35,39 depart=31 cross=23:o,33:o land=38"
for tags, why, label in [
    ("23:u,33:o", "check 5", "mixed tags on a pass"),
    ("23:u,33:u", "that is a crossing change", "tags opposite to W's role"),
]:
    bad = tampered(PASS_WCROSS, CLEAN_W, CLEAN_W.replace("23:o,33:o", tags))
    rc, out, _ = run(PROVE, [], bad)
    check(rc == 1, f"{label}: exit 1", out)
    check("#verify step 2 descriptor: MISMATCH -- not well-formed" in out
          and why in out, f"{label}: a descriptor MISMATCH naming why", out)
    check("step 2 result/drawing/trace: UNCHECKED" not in out,
          f"{label}: not waved through as a limit of the surgery", out)

# A strand must CONTINUE along its component at each interior crossing, not
# merely meet the next arc there (ROUND-24 §6(d)). Darcs 1 and 9 of the
# trefoil meet at crossing 1 on different branches; as a uniform pass with a
# k=0 corridor it was well formed, AfterDiagram smoothed the crossing, and the
# trace check VERIFIED the trefoil going to a Hopf link. No witness backs a
# uniform pass, so well-formedness is the whole soundness claim.
TREFOIL_TO_HOPF = ("#trace v=0\n#step n=0 summand=0\n"
                   "#move kind=pass strand=1,9 depart=0 land=8\n"
                   "0\t4\t1\t3\t1\n2\t0\t3\t5\t1\n4\t2\t5\t1\t1\n\n"
                   "#step n=1 summand=0\n2\t0\t3\t1\t1\n0\t2\t1\t3\t1\n")
rc, out, _ = run(PROVE, [], TREFOIL_TO_HOPF)
check(rc == 1 and "descriptor: MISMATCH" in out
      and "turns onto the other branch" in out,
      "a strand that turns onto the other branch: descriptor MISMATCH", out)
check("trace: VERIFIED" not in out,
      "a strand that turns onto the other branch: the Hopf link is not"
      " waved through", out)

# A transversal that heals into a curl at crossing 91 (arcs 125 -> 126 -> 127
# through two interior crossings of W), crossed twice by the corridor.
# AfterDiagram used to repoint the loop arc's tail where it meant its head and
# build a diagram that fails CheckAll (ROUND-22 §3).
rc, out, _ = run(PROVE, [HEALED_CURL])
check(rc == 0, "middlepass_healed_curl: exit 0", out)
check("#verify step 0 result: VERIFIED" in out
      and "#verify step 0 trace: VERIFIED (266 crossings expected, 266 found)"
      in out,
      "middlepass_healed_curl: the healed curl is split at its head", out)

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
