# Move descriptors and trace streams — design

Status: draft convention (v0), 2026-08-10; two-deletions contract and the
drawing-parse check added 2026-08-12. Authoritative spec for the
combinatorial move-descriptor format shared by (a) the planned knoodledraw
overlay/debug mode for visualizing pass-move feasibility, and (b) the
longer-term "knoodleprove" pipeline, in which an instrumented Simplify /
middlepass records the moves it chose and a renderer replays them as a visual
proof. Companion to the OrthoDecorate work on branch `orthodecorate`.

## Design principles

1. **Descriptors are PD-level combinatorics.** A move is described against a
   specific PD snapshot using arc/darc references only — never OrthoDraw grid
   coordinates, bend vertices, or any other per-layout geometry. Geometry is
   re-derived by each renderer. (One principled exception: a `redraw` step
   carries *witness* geometry — the 3D embedding and rotation that certify
   the isotopy. That is part of the move's mathematical content, not layout.)
2. **Local conventions, no global enumeration.** Nothing in the format depends
   on the *order* in which any algorithm enumerates faces (or anything else).
   Faces are named by darcs (below). The only conventions a consumer must
   share are local ones already documented in Henrik's headers.
3. **Snapshot per step.** Simplify recompresses and renumbers arcs and
   crossings freely, so every descriptor travels with the full PD code it
   refers to. Records are self-contained; recording and replay are fully
   decoupled. Disk is cheap and nobody watches movies of 10^6-crossing
   diagrams.
4. **Annotations are redundant and fail-loud.** Human-readable extras (e.g.
   a face table) may be embedded, but they are never the source of truth: any
   consumer that reads one must recompute it and abort on mismatch (same
   philosophy as the CLI fail-loud contract). "Recompute" means *recompute the
   thing the annotation claims*, which for a relabelling annotation is a claim
   up to relabelling — see the `#pd` rule below, where getting this wrong made
   this document's own worked example abort.

## Inherited conventions (normative)

These are Knoodle's existing conventions; we cite them rather than invent:

- **PD code** (the v0 snapshot carrier, and the v1 `#pd` annotation) = the
  5-column signed PD code, one row per crossing:
  `a b c d s`, arc indices **0-based**, `s > 0` right-handed, `s <= 0`
  left-handed (`src/PlanarDiagram/PDCode.hpp`, `FromSignedPDCode` doc). The
  four slots of a row list the arc-ends in **counterclockwise cyclic order
  around the crossing, starting at the incoming understrand** (the standard
  PD-code convention; see the crossing diagrams in `PDCode.hpp`). This is the
  rotation system, and it is all a consumer needs to trace faces.
- **Darc** (directed arc): `da = 2*a + d` with `Tail = 0`, `Head = 1`
  (`src/PlanarDiagram.hpp:88`, `src/PlanarDiagram/Darcs.hpp`). `d = Head`
  means the darc points along the arc's orientation; `d = Tail` means
  against it.
- **Faces lie on the left.** Every face boundary cycle is oriented so that
  the face lies on the **left** of each of its darcs
  (`src/PlanarDiagram/Faces.hpp:25`). The next-darc map of the walk is
  `LeftDarc(da)` (`Darcs.hpp:48`); the traversal is `TraverseFaceAtDarc`
  (`Faces.hpp`). The O(1) lookup is `ArcFaces()(a,d)` = the face **left of
  darc `2a + d`** (the convention stated inside `ComputeFaces` and verified
  against `FaceDarcs()` on the trefoil; the doc comment at `ArcFaces()`
  itself used to state the opposite — fixed upstream via PR #29, merged
  2026-08-10; see [upstream-issues.md](upstream-issues.md) issue 3).

### Naming faces

- **A face is named by any darc on its boundary**: "the face left of darc
  `da`", written `L(da)`. This is complete and unambiguous, pinned entirely
  by the snapshot's own arc numbering plus the face-on-left rule.
- The **right face** of a darc is `R(da) = L(ReverseDarc(da))`, with
  `ReverseDarc(da) = da XOR 1`.
- When a face needs a standalone canonical name (annotations, human
  discussion), use its **minimal boundary darc**: the smallest `da` in its
  boundary cycle.
- Consequence: no face table is ever *required*. A dual-graph path is a
  sequence of darc crossings (below), and the face sequence is derived.

## Trace stream format

A trace is a text stream extending the existing knoodle TSV streaming format
(the `#color` header-line mechanism is the precedent). It is a sequence of
**records**, each:

```
#step n=<k> summand=<sid>
#candidate                    (optional; evaluated but not applied)
#comment <free text>          (optional, repeatable)
#view exterior=<da>           (optional, recommended; see Layout transitions)
#move <descriptor>            (absent on terminal records)
#faces <annotation>           (optional)
#state lines=<N>              (v1; the snapshot)
<N lines, verbatim PlanarDiagram::WriteToOutString output>
#result lines=<M>             (v1, optional; see below)
<M lines, an applier's result for this move>
#pd rows=<R>                  (v1: optional redundant annotation)
<R 5-column signed PD rows>
<blank line>
```

### v0 and v1: the snapshot carrier

**v1** carries the snapshot as `PlanarDiagram`'s internal-state serialization
in a `#state lines=N` block, with the 5-column PD code demoted to an optional
`#pd rows=R` annotation. **v0** carries the bare 5-column PD rows with no
`#state` block. The shipped `--trace` reader accepts both.

The carrier changed because PD codes come from `Traverse`, so every PD-code
path renumbers crossings and arcs and drops inactive slots — measured on the
73-crossing reproducer, writing and re-reading leaves *0 of 73 crossing rows
at the same index*. That is fatal for the failure class this format exists to
chase, which is a **label** bug: with a renumbering snapshot a verifier can
only compare a label-free invariant, which catches "wrong knot" but never
"right knot, wrong labels", and localizes nothing. Internal state makes the
before/after correspondence the identity on survivors, so comparison becomes
port-by-port and names the crossing that disagrees.

`lines=N` rather than a self-delimiting block is deliberate: a reader slices
N lines and hands them to `FromInString` without knowing the layout, so a
field added upstream changes N and the reader still works, or fails loud.

Three rules come with it, per principle 4:

- the **internal-state block is the source of truth**;
- the `#pd` annotation may ride along, and any consumer reading both **must**
  check that the two describe the same diagram **up to relabelling**, and abort
  if they do not. It **must not** compare them literally. `PDCode()` renumbers
  on the way out, so the annotation's bytes need not be `PDCode()` of the state
  even when both are right — the worked example below is exactly that case, and
  a literal recompute-and-abort aborts on it. Identifying the diagram up to
  relabelling is all this annotation is for, and it is enough to catch two
  carriers describing different diagrams;
- each stream declares the Knoodle version that wrote it
  (`#knoodle version=<string>`, once at the top), and a reader that cannot
  parse the state **fails loud** rather than falling back to the annotation.
  The format is documented upstream as debugging-only with no cross-version
  guarantee; this is for archive durability, since traces outlive the build
  that wrote them.

`#result lines=M` optionally carries what an *applier* produced for this
record's move — the point of comparison for a verifier that wants to check an
applier against `AfterDiagram` across a process boundary. Required on an
applied move in a batch meant for checking, optional on a `#candidate`.

**Status (2026-08-13):** v1 is implemented on both sides and exercised.
`src/MoveTrace.hpp` is the reference reader (and the `WriteStateBlock` writer
half); `knoodledraw --trace` consumes it, and `--verify` uses `#result` for the
port-by-port comparison described under "the verification contract". The first
real emitted stream — 41 records off a plateau walk on a 73-crossing diagram —
reads with 0 mismatches: 41/41 drawings verified, 29/29 applied moves agreeing
with `AfterDiagram` port-by-port.

The grammar lives in `src/`, not in a tool, on purpose: it is a contract
between two programs, and the bug class it exists to rule out is two
implementations of one grammar disagreeing.

- The **descriptor in a record applies to the PD snapshot in that same
  record** (the *before*-diagram). The resulting diagram is the snapshot of
  the next record for that summand. A renderer can therefore draw
  "diagram + move overlay" from a single record with no lookahead, which
  suits the streaming pipeline (`simplify --record | knoodledraw ...`).
- The first record of a summand carries its input diagram; a record with no
  `#move` is terminal for its summand.
- `summand=<sid>`: stable integer id per diagram within the trace. When a
  step splits a diagram (connect-sum separation, split links), child records
  use fresh ids and name their parent (see Open questions).
- Streams begin with `#trace v=0` or `#trace v=1` (above), so the format can
  evolve; v1 streams additionally carry `#knoodle version=<string>`.

## Move descriptor: `pass`

The fully specified kind, and the one both consumers need first. A pass move
detaches a strand (a run of consecutive arcs between two anchor crossings)
and reroutes it along a new corridor through the diagram's faces, passing
entirely over or entirely under everything it crosses.

```
#move kind=pass strand=<da_1,...,da_m> depart=<da_dep> cross=<da_x1>:<u|o>,...,<da_xk>:<u|o> land=<da_land>
```

- `strand`: the darcs of the rerouted strand in traversal order (so
  orientations are consistent). The anchors are the crossings at the tail of
  the first and the head of the last darc; they stay put.
- `depart`: a darc with `L(depart)` = the face through which the new corridor
  leaves the tail anchor. This fixes on which flank of the anchor the
  rerouted strand emerges. The corridor's first face is `F_0 = L(depart)`.
- `cross`: the arcs the new corridor crosses, in order, each as a **darc**
  with an over/under tag. Crossing `da_xi` means the corridor traverses that
  arc **from its left side to its right side**: it steps from face
  `F_{i-1} = L(da_xi)` to `F_i = R(da_xi)`. (Using a darc rather than a bare
  arc keeps this unambiguous even when the same face touches an arc on both
  sides.) The tag `u`/`o` says the rerouted strand goes under/over that arc;
  for a legal pass move all tags are equal.
- `land`: a darc with `L(land) = F_k`, the face from which the corridor
  reaches the head anchor, fixing the arrival flank.

### Local validation (every consumer must check)

1. `strand` darcs are consecutive: head of each = tail of the next; all arcs
   active and pairwise distinct; and the crossed arcs are pairwise distinct —
   the corridor is **arc-disjoint**.

   For `pass`, a crossed arc **may** belong to the strand: the corridor may
   cross W. That crossing goes with W — deleting W leaves nothing there to
   cross, so the after-diagram has no crossing for that entry and the corridor
   runs straight through. Proposition C′ (below) is what licenses this. For
   `middlepass` it is still forbidden (the *membership clause*: no crossed arc
   belongs to the strand), because nothing corresponding is proved for mixed
   tags.

   Arc-disjointness is not a stylistic preference. Crossing one arc twice is
   not something an applier can carry out: the first crossing splits the arc,
   after which the label denotes only the piece up to that new crossing, so a
   later step naming it again operates on a changed extent. That is the same
   aliasing hazard the transversals have. `FindShortestPath` cannot produce
   such a path in any case — it keeps a visited set on arcs and expands only
   unvisited ones — so requiring it here makes an existing precondition
   explicit rather than excluding anything an emitter could legitimately want.
2. `L(cross_1) = L(depart)`, and `L(cross_{i+1}) = R(cross_i)` for each
   consecutive pair — each crossing departs from the face the previous one
   arrived in.
3. `L(land) = R(cross_k)` (or `= L(depart)` if `k = 0`).
4. `L(depart)` has the strand's **first** arc on its boundary, and `L(land)`
   has its **last** arc on its boundary. Also, the two anchors must be
   distinct crossings.
5. All over/under tags equal, and equal to the strand's own role: W must run
   uniformly over, or uniformly under, at its interior crossings, and the tags
   must say which. A pass move slides a strand; it cannot turn an over-strand
   into an under-strand, and a descriptor that asks for that is a crossing
   change wearing a pass move's clothes.
6. The corridor is no longer than the strand: `k <= L - 1`, i.e.
   `path.CrossingCount() <= pass.CrossingCount()`.

Check 6 is a statement about the data structure, not about topology. A
lengthening pass is a perfectly good isotopy; it is simply not expressible
here. An applier rebuilds the strand in place out of the labels the move frees
-- W's own arcs and the transversal halves it heals away -- so a corridor with
more crossings than W had has nowhere to live, and the diagram would have to
grow. `Reroute` does not refuse such input either: its loop walks path
positions while the strand pointer runs off the end of W, and it returns a
diagram unrelated to the move (on a trefoil, 2 crossings out of 3). So this
must be caught before anything is applied or drawn.

One consequence worth knowing: a corridor with two or more crossings needs a
strand of three or more arcs, and neither the trefoil nor the figure-eight
admits one that satisfies the rest of the checks. Small diagrams support only
`L = 2, k = 1` pass moves.

Check 4 is deliberately stronger than "names a face incident to the anchor
crossing", which is what it used to say. A crossing has four quadrant faces,
and the anchors stay put across a pass move, so the rerouted strand leaves
(resp. reaches) the anchor through the very port the strand's first (resp.
last) arc occupies. Only the **two** quadrants flanking that port are
therefore reachable. Naming either of the other two is well formed under the
weaker reading and yet describes a strand attaching to a different port — it
would change the diagram at a crossing the move promised not to touch. That
is precisely the failure mode a picture of a pass move is meant to catch, so
it belongs in well-formedness rather than in soundness.

The distinct-anchors clause excludes the case where the strand leaves and
returns to the same crossing (an R_I curl at the end of a strand, and its
relatives). Both junctions would then be quadrants of one crossing and "which
port" stops being well posed; rather than pick one, consumers refuse.

Every check is O(local) against the snapshot; a verifier needs `LeftDarc`
orbits and nothing else. A descriptor that passes all checks is a
*combinatorially well-formed* pass move; feasibility semantics beyond that
(what middlepass proves) are the emitter's business — which is exactly why
the debug mode is useful for auditing them.

### Worked example (trefoil)

Snapshot (right-hand trefoil, arcs 0–5):

```
0	4	1	3	1
2	0	3	5	1
4	2	5	1	1
```

Its face structure (derived, shown here as annotation): five faces with
boundary darc cycles `{0,4,8}`, `{1,6}`, `{2,9}`, `{3,11,7}`, `{5,10}`.

An illustrative (not necessarily simplifying) descriptor:

```
#move kind=pass strand=1,3 depart=1 cross=6:u land=3
```

reads: reroute the strand consisting of arcs 0 then 1 (traversed along their
orientation: darcs 1 = 2·0 + Head and 3 = 2·1 + Head), leaving its tail anchor
through face `L(1)` = `{1,6}`, passing **under** arc 3 by crossing darc 6
(stepping from `L(6)` = `{1,6}` to `R(6)` = `{3,11,7}`), and reaching the head
anchor through face `L(3)` = `{3,11,7}`. Checks: `L(cross₁) = L(depart)` ✓,
`L(land) = R(cross₁)` ✓, `depart` names arc 0 and `land` names arc 1, the
strand's first and last ✓ (check 4), the strand runs under at its one interior
crossing and the tag says `u` ✓ (check 5), and one corridor crossing replaces
one strand crossing ✓ (check 6).

Check 4 has teeth, in two ways. The descriptor
`strand=11 depart=7 cross=7:u land=1` — reroute arc 5 under arc 3 —
satisfies the chain rule, and `L(1)` = `{1,6}` *is* a quadrant at arc 5's head
anchor, so the old, weaker check 4 accepted it. But `land=1` names arc 0, not
the strand's last arc 5: `{1,6}` is the quadrant between arcs 0 and 3, on the
far side of the crossing from arc 5's port, while arc 5's own two faces are
`{3,11,7}` and `{5,10}`. A strand landing there is not the strand we started
with. Rejected.

Separately, `depart=7` names arc 3 rather than the strand's first arc 5, even
though `L(7)` = `{3,11,7}` *is* the right face — `depart=11` is the same face
by its canonical name. That one is not a wrong move, just a non-normal spelling
of a right one, and it is rejected too.

(This example was itself wrong in earlier revisions of this document, in
exactly that way — which is the argument for the stronger check in miniature.)

## The two deletions (what a pass-move picture claims)

A drawn pass move is a superposition of two states, each one deletion away.
This is the contract a renderer must satisfy, and it is checkable:

- **Delete the corridor** (the `p`-arcs) and what remains is a valid embedding
  of the diagram the descriptor was written against.
- **Delete the strand `W`** (the `w`-arcs) and what remains is a valid
  embedding of the diagram the move produces — which, when the corridor is a
  shortest path, is the diagram `Reroute` returns.

The two are joined at the **dots**. A corridor endpoint is not a free-floating
point beside an anchor: it is a dot placed **at a portal point of the strand's
own first (resp. last) arc** — the same designated crossing sites the corridor
uses to cross any other arc. Check 4 is what makes this well posed: because
`depart`/`land` must be darcs *of* the strand's end arcs, `Portal(depart)` and
`Portal(land)` are exactly the candidate dot sites, and the corridor leaves the
dot sideways into `L(depart)` (resp. arrives from `L(land)`).

The piece of arc between an anchor and its dot is therefore **shared**: under
the first deletion it is the start of `W`, under the second it is the start of
the rerouted strand. That is precisely why the rerouted strand attaches to the
anchor through the very port `W` vacated, which is the property check 4 exists
to guarantee and the one a picture is meant to expose when it fails.

Two consequences for renderers:

- **The corridor must be simple.** Once a stroke is drawn it is really there,
  so later legs of a corridor must route around earlier ones. A corridor that
  crossed itself would make the second deletion a non-embedding.
  `OrthoDecorate::RouteAcrossDarcs` enforces this by routing sequentially with
  the already-drawn cells as walls, and failing loudly rather than overlapping.
- **Deleting `W` means healing what it crossed.** At each of `W`'s interior
  crossings the transversal was interrupted; with `W` gone the transversal must
  be restored, or the drawing has a hole where a strand should run. In an
  orthogonal layout the strand runs straight through a crossing, so the healed
  stroke is simply the perpendicular one through that cell.

### Checking it: parse the drawing back

`test/pass_view_check.cpp` renders each deletion view and reads it back into a
`PlanarDiagram` with `test/drawing_extractor.hpp`, then compares port-by-port.
The extractor takes its structure from the characters alone and never consults
the diagram under test — necessarily, since a corridor attached to the wrong
port still draws a perfectly legal diagram of some knot.

The correspondence handed to the comparison is **geometric**: each parsed
crossing is matched to the crossing whose grid cell it was read from, and the
corridor's new crossings by their order along the corridor (which is how
`AfterDiagram` numbers them). This is deliberately stronger than testing for
isomorphism — a drawing isomorphic to the right diagram by *some* map, but not
by the map the geometry dictates, is a failure, not a pass.

**This has replaced MacLeod comparison.** The MacLeod code is an invariant of
the oriented diagram, so it answered "same knot?" and nothing finer: it could
not see a right knot drawn with wrong labels, it localized no failure, and —
being a knot invariant — it does not exist for links, which ruled it out as the
foundation for a spec that is link-capable throughout. Nothing uses it now.

## Conformance tiers: well-formed vs sound

Two distinct predicates apply to a record, and tools split along them:

The tools split along the same line, and each reports only its own half:
`knoodledraw --trace --verify` reports `drawing:` and nothing else (the two
deletions, checked by rendering each view and parsing it back), while
`knoodleprove` reports every claim that needs no drawing — `pd`, `trace`,
`split`, `spinoffs`, `result`, and the witness — and never builds a layout.
The checks they share live in `tools/trace_verify.hpp`.

- **Well-formed** (= renderable): the descriptor passes its kind's local
  checks against the snapshot. Renderers (knoodledraw) draw ANY well-formed
  record — explicitly including moves that are topologically infeasible.
  An infeasible move is not meaningless or un-renderable; it is *wrong*,
  and a drawing of it is often exactly the picture one needs (e.g. showing
  WHY a candidate was rejected). Rendering never claims soundness.
- **Sound** (= proof-grade): the step additionally carries whatever
  witness its kind requires, and the witness checks out. Only verifiers
  (knoodleprove) demand this, and only for records that advance the
  diagram. For `redraw` the witness is the embedding+rotation (above); for
  `middlepass` it is the feasibility witness, whose soundness theorem
  (middlestrands' Theorem B) was ratified on 2026-09-14 — see below. For
  `pass` and `r1` the required witness is **none**: see immediately below.

### Kinds for which well-formed implies sound

For two kinds the two tiers coincide, so a verifier has nothing to demand
beyond the local checks.

- **`r1`** — a curl removal carries no witness; its local checks are the whole
  story. Stated with the kind, below.
- **`pass`** — **a well-formed `kind=pass` record is sound.** Check 5 is what
  does the work: requiring W's tags to be uniform and equal to its own role at
  its interior crossings is exactly the hypothesis under which the rerouting is
  an isotopy, for a corridor of any length and on either side of W.

  *Proposition C, middlestrands `cpp-design/slide-realization-theorem.md` §5.1
  ("Uniform passes: feasible always, and without variables"), ratified
  2026-09-16. The proof is not reproduced here.*

  Two boundaries worth stating, because "well-formed implies sound" invites
  being read more broadly than it is meant.

  1. It is about `pass`, not `middlepass`. Mixed tags are exactly what the
     proposition does not cover, and are why the feasibility witness exists.
  2. It does not settle whether an anchor may coincide with an interior
     crossing; uniformity does not imply that, and it remains a separate
     hypothesis on the emitter's side.

  **Proposition C′** (*ibid.* §5.2, "Uniform passes with an unrestricted
  corridor", ratified 2026-09-16) is stronger, and drops two hypotheses that are
  worth keeping apart: that the crossed arcs avoid W, and that they are pairwise
  distinct. (The crossing *points* remain distinct; it is the *arcs* that may
  repeat.) Check 1 forbids both today, in two separate clauses — and the two are
  at quite different stages.

  **A corridor that crosses W.** Check 1's "no crossed arc belongs to the
  strand" forbids naming such an arc. This is not an exotic case:
  `FindShortestPath` **hides** W while searching, which is exactly what lets a
  corridor step between the two faces flanking a strand arc at no cost, so
  W-crossing corridors are the search's ordinary output — middlestrands measure
  59.3% of the corridors with `k ≥ 1` on raw projections. The consequence today
  is that neither side can write down what the search returned: no W arc is
  named, so check 1 passes vacuously and **check 2** fails instead, its face
  chain broken by a merge that exists only once W is gone. `knoodledraw
  --find-pass` loses these corridors for exactly this reason.

  **Decided 2026-09-19 (JHC): the clause is relaxed for `pass` and kept for
  `middlepass`.** C′ covers uniform passes and nothing else; the mixed-tag
  analogue is unproved, so a `middlepass` still may not name a W arc. The
  relaxation was proposed in `handoff/middlepass-descriptor-emission/` ROUND-11.
  It needed renderer and applier work first, since those paths had never run:
  ROUND-14 and ROUND-16 record what broke, and `test/pass_view_check.cpp`
  (`RunStrandCrossingTests`) asserts both halves of the rule.

  **The same arc crossed twice.** Here C′'s permission does *not* simply
  transfer, and the clause divides:

  - a repeated **surviving** arc stays forbidden. The first crossing splits it,
    so a later step naming it again addresses a changed extent — the aliasing
    hazard check 1's own rationale describes. C′ says the topology is fine; it
    does not make an applier able to carry the move out.
  - a repeated **W** arc carries no such hazard, that arc being deleted rather
    than split. Whether distinctness should therefore be stated as constraining
    surviving crossings only is **open**, and it arises only once W arcs may be
    named at all. ROUND-11 asks for a measurement before it is settled.

  So for the repeated-arc clause, C′'s role is to establish that it is an
  *applier* constraint rather than a topological one — not to license removing
  it. The membership clause was different: once our own applier
  (`AfterDiagram`) and renderer handled a W arc correctly, the applier
  constraint was gone and C′ settled the topology.

## Step kind: `middlepass`

Middlestrands' `MiddleStrandSimplifier` moves (see
`handoff/middlepass-descriptor-emission/`): identical grammar and checks
1–4 as `pass`, but the over/under tags are **per-crossing** — check 5 is
dropped. One difference inside check 1: a `middlepass` keeps the **membership
clause** — no crossed arc may belong to the strand — which `pass` has dropped
under Proposition C′. There is no mixed-tag analogue of C′. This is the majority move class in practice (67% of applied moves
in the first shakedown), not an edge case.

```
#move kind=middlepass strand=... depart=DA cross=DA:u|o,... land=DA
```

Well-formedness = checks 1–4 **and check 6**. Check 6 is *not* dropped: a
middlepass has no more room in the data structure for extra crossings than a
classical pass does, and only check 5's uniformity requirement goes away.
(The handoff rounds say "checks 1–4" throughout because they predate check 6.)

**Quotient-simplicity is not a soundness condition.** An earlier
revision of this document required *quotient-simplicity* of the corridor —
deleting the strand merges the two flank faces of each strand arc, and the
corridor was said not to be allowed to revisit a class of that quotient. That
requirement is **withdrawn**, and it should never have been written down here:
middlestrands retracted the mechanism behind it in
`ROUND-2-RESPONSE.md` after their own poster fixture for it turned out to
apply soundly while revisiting the quotient four times. Theorem B (below)
settles the question: its hypothesis has no quotient-simplicity in it, and
middlestrands removed their realization guard on 2026-09-14 (`ROUND-8.md`).

### The feasibility witness: `#feas` / `#fvar` (normative)

A `middlepass` record may carry a witness to the feasibility solve that chose
its over/under tags: the sweep disk on one side of the move, and a labelling of
every piece on that side. Finding the labelling takes a solver; checking it
takes none. The header lines go after `#move` and before `#state`:

```
#feas side=<0|1> disk=<c1,c2,...>        (disk may be empty: "disk=")
#fvar <piece>[,<piece>...]=<a|b|f>       (one line per class)
```

(Proposed in `ROUND-1-RESPONSE-ADDENDUM-feasibility-witness.md`, emitted by
middlestrands from 2026-08-13, and pinned here in ROUND-6 after V0 and V4 had
passed all 41 witnesses of their `fixture-zf061098-walk-v2-witnessed.trace`.)

**Pieces.** The loop is W — every arc of the strand — followed by the corridor.
W's arcs carry no pieces. An arc the corridor crosses is two pieces, split
where the corridor crosses it: `<arc>t`, the half incident to the arc's tail
crossing, and `<arc>h`, the half incident to its head crossing. Every other arc
is one piece, `<arc>`. A piece never meets the loop, so it lies on one side of
it.

**Sides.** Side 0 is the side holding the first non-W piece reached by walking
W's component forward, along its orientation, from the arc after W's
orientation-last arc; a crossed arc is entered through its `t` half. Side 1 is
the other.

**Interior crossings.** A crossing is interior on side `s` iff every incident
piece lies on side `s`, W's arcs counting as neutral. So an anchor is interior
on side `s` when its three non-W pieces all lie there. `disk=` lists the
interior crossings of side `s`.

**Classes.** The `#fvar` lines name every piece on side `s`, each exactly once.
Labels: `a` above the disk, `b` below, `f` free (never forced); checkers fill
`f` ⟹ below. Readers must not depend on the order of the classes, or of the
pieces within one.

**Readers MUST refuse** `#fvar` before `#feas`, a second `#feas`, a `side`
other than 0 or 1, a missing `disk=` field, an empty entry in a list, a label
other than `a`/`b`/`f`, a malformed piece, and a piece in two classes or twice
in one.

**The checks.** `disk=` and the classes are asserted by the emitter; the checks
are what make them true.

| | check | where it runs |
|---|---|---|
| V0 | rebuild both sides from the snapshot and the descriptor; `disk=` equals side `s`'s interior crossings, and the classes name exactly side `s`'s pieces | `knoodledraw --verify`: `disk (V0)` |
| V1 | at each interior crossing, each strand's two pieces share a class (a strand pair containing a W arc is skipped) | emitter; implied by V4 |
| V2 | at each interior crossing of W, the transversal's side-`s` piece — the one incident to that crossing — is `a` if the transversal passes over W, and below (`b`, or `f` after the fill) if under. A germ constrains that piece and nothing else: a crossed arc running between two interior crossings of W (a *chord*) has one half on each side, each bound only by its own germ. (Until 2026-09-14 a germ at either end of a chord forced both halves; Theorem B makes that coupling a surplus constraint, and it was withdrawn in `ROUND-8.md`.) | emitter; `knoodledraw --verify`: `labels (V2/V3/V5)` |
| V3 | at each interior crossing, never under = `a` with over below, after the fill. At an anchor one strand runs along W's end arc, which carries no piece; the ordering still binds whatever non-W pieces the two strands have there. This is Theorem B's (G3) at the anchor, which is an ordinary interior crossing of its side. (A check that skips anchors accepts witnesses whose moves change the knot: synthetic examples take unknot diagrams to determinants 29 and 5.) | emitter's solver; `knoodledraw --verify`: `labels (V2/V3/V5)` |
| V4 | the classes are exactly the unions of V1's equalities: no merge that no chain forces, and no split | `knoodledraw --verify`: `classes (V4)` |
| V5 | for each `cross=DA:tag`, tag `o` ⟺ the side-`s` half of that arc is below, after the fill | emitter; `knoodledraw --verify`: `labels (V2/V3/V5)` |

V2, V3 and V5 also run on the verifier's side even though the emitter runs
them, so that a record's verdict never rests on the emitter's own gate.

V0 rebuilds the sides as a flood over face fragments. The corridor cuts each
face it visits in two, at the middle of the darcs it passes through; fragments
join across every arc half the loop does not occupy, which is every half except
W's interior arcs and, on W's first and last arc, the half toward W's interior
(the stub toward the anchor stays passable, which is what puts an anchor where
the rule above puts it); exactly two regions remain. V0 reports `UNCHECKED`
rather than guessing when W is a single arc, the diagram is split, or the
corridor visits a face twice.

**What a passing witness proves.** An isotopy. Middlestrands' Theorem B
(`cpp-design/slide-realization-theorem.md` in their repository, ratified by JHC
on 2026-09-14): let D be a connected diagram, W a strand whose crossings
c_0, …, c_m are pairwise distinct, and the corridor an embedded path from the
interior of W's first arc to the interior of its last, crossing other arcs
transversally. If the side-`s` constraint system is feasible, and the tags are
read off that same assignment, then the rerouted diagram is ambient isotopic to
D — knots and links alike, with no quotient-simplicity hypothesis and no anchor
condition.

In witness terms: V0 fixes the side, its pieces and its disk; V1 with V4, V2
and V3 are the theorem's (G2), (G1) and (G3), so a labelling that passes them is
a feasible assignment; and V5 says the record's tags are read off that
labelling by (G4). The theorem is applied to the witness itself, so it does not
matter which assignment the emitter used to choose its tags — V5 is what ties
the tags to the witness.

The theorem's other hypotheses, and where each is established:

| hypothesis | where |
|---|---|
| D connected | V0 reports `UNCHECKED` on a split diagram |
| W's crossings pairwise distinct | V0 reports `UNCHECKED` when W passes through a crossing twice |
| the corridor embedded | checks 1–3 (arc-disjoint, a face chain); V0 reports `UNCHECKED` when the corridor visits a face twice |
| the corridor attached inside W's end arcs, transversally | check 4 (`depart` and `land` are darcs of W's first and last arcs) |

So a well-formed `middlepass` record whose witness passes V0–V5, with V0
`VERIFIED` rather than `UNCHECKED`, prescribes an isotopy. The theorem is about
the move, not about an applier: the `result`, `drawing` and `trace` checks are
still what ties a record's snapshots to its move. The theorem covers a single
arc W, but V0 reports `UNCHECKED` there; that is a limit of the reconstruction,
not of the theorem.

Emitter guidance: recommended canonical `depart`/`land` darcs are the
strand-flank darcs at the anchors (they pin the emerging flank even when a
face touches an anchor twice); any darc naming the correct face is legal.

## Candidate records and comments

- A record may carry the header `#candidate` (no arguments): its `#move`
  was **evaluated but not applied**. The diagram does not advance — the
  next record of the summand has the same snapshot. Renderers draw
  candidate records exactly like applied ones (that is their purpose:
  pictures of rejected moves); verifiers exclude them from the proof chain
  and impose no soundness requirement on them.
- `#comment <free text>` headers are echoed verbatim by renderers and
  ignored by verifiers: telemetry, rejection reasons, implication chains.

## Step kind: `r1` (curl removal)

An R1 step removes a curl: a loop arc, together with the crossing it closes on.
It is **not** a pass move, and the pass grammar refuses it by name — check 4's
distinct-anchors clause exists precisely to exclude "an R_I curl at the end of a
strand, and its relatives". So it gets its own kind, and a simpler contract.

```
#move kind=r1 loop=<da>
```

One field. `loop` is a darc `2a + d`; `a` is the loop arc, and by the
face-on-the-left convention of "Inherited conventions" **`L(loop)` is the
monogon face that collapses**. Naming the loop as a *darc* rather than an arc is
what makes the collapsing side unambiguous without a second field, and it keeps
the grammar consistent with `pass`, whose every field is a darc.

Everything else derives, and a consumer must derive it the way the applier does
(`src/PlanarDiagramComplex/LoopRemover.hpp`):

- `c = Arcs()(a,Head)` — the crossing that dies;
- `a_prev = NextArc(a,!d,c)` and `a_next = NextArc(a,d,c)` — the surviving ends.

### Local validation (every consumer must check)

1. `a = loop/2` is active, and `Arcs()(a,Head) == Arcs()(a,Tail) == c` with `c`
   active. This is the loop test, and it is the one the applier makes.
2. `L(loop)` is a **monogon**: `FaceDarcs()[L(loop)]` is exactly the single darc
   `loop`. This is what pins which side collapses.
3. If `a_prev == a_next`, the record MUST carry `#spinoffs 1` (below).

Checks 1 and 2 are not independent in practice: a loop arc bounds exactly one
monogon, and every monogon is bounded by a loop arc. Measured over 43 raw
projections (2026-09-16): **537 monogons, 537 loop arcs, a perfect bijection,
none bounding two monogons.** Check 2 is kept anyway, because it is what makes
`L(loop)` meaningful as *the* collapsing face, and it is O(1).

**Removal only.** `kind=r1` denotes removal of an existing curl. The inverse is
a perfectly good isotopy and is simply not expressible here — the descriptor
names a loop that must already be in the snapshot. This mirrors check 6 for pass
moves, and is stated now so it does not become an open question later.

**Which label survives.** `LoopRemover` heals with `Reconnect(a_next,!d,a_prev)`,
which keeps **`a_next`** alive; `a`, `a_prev` and `c` are deactivated. A verifier
seeding the identity on surviving labels (as the `result` check does) needs this.

**Spinoffs.** When `a_prev == a_next` the whole component was a single curl — an
8-shaped unlink — and the applier frees a crossingless component
(`CreateUnlinkFromArc`). That is exactly what `#spinoffs` already describes for
pass moves, so the same header is reused verbatim with no new machinery. Rare
(0 of the 537 loops above) but real.

**No witness.** Unlike `middlepass`, an `r1` needs nothing beyond its local
checks: a well-formed `r1` is sound. For this kind the two conformance tiers
coincide.

### What an R1 picture claims (one deletion, not two)

A pass move superposes two states because it **adds** a corridor. An R1 adds
nothing, so "The two deletions" does not apply; it is replaced by something
weaker and simpler:

- the **before** view is the input drawing itself, with the loop arc marked and
  the monogon shaded — there is nothing to delete to recover the snapshot;
- the **after** view deletes the loop arc and heals the crossing, and parsed
  back it must be the diagram the move produces.

Exactly one checkable claim, rather than two.

**The healed crossing is a corner, and which corner is forced.** The loop's two
ends at `c` are rotationally adjacent — that is what it means for `L(loop)` to be
a monogon. So the two surviving ends are adjacent too, and the healed strand
cannot run straight through `c`: it must turn. It turns **away from the
monogon**, occupying the quadrant diagonally opposite the collapsing face. A
renderer reads the monogon's quadrant at `c` and stamps the opposite corner; it
never has to search.

### Renderer guidance

The monogon is the R1 analogue of a pass move's swept disk, and it is cheaper:
that disk is not a face of the diagram (hence `PassDiskCells`), whereas **the R1
disk *is* a face**, named directly by `L(loop)`. Shading it needs no new
geometry — the same face machinery `--checkerboard-coloring` already uses.

Of the three things an R1 overlay draws, two are existing primitives:

| element | how |
|---|---|
| monogon shading | face highlight at `L(loop)` |
| the loop arc | strand marking, as `W` is marked for a pass move |
| the dying crossing | a NEW overlay kind (`Collapse`) — a pass move's anchors *survive*; this crossing does not |

In a structured-geometry backend (`knoodledraw --format=wl`) an R1 record carries
an `"R1"` member, a **sibling** of `"Pass"` rather than a variant under a shared
key: the payloads have no schema in common, so the presence of the key is the
discriminator and a `Kind` tag over them would be a union in name only.

```
"R1"-><|"Kind"->"r1","View"->"both"|"before"|"after",
        "Loop"->da,"Arc"->a,"Crossing"->c,
        "Monogon"->f,
        "Survivor"->a_next,"Absorbed"->a_prev,
        "Corner"-><|"Pos"->{x,y},"Kind"->"CornerSW"|>,
        "Spinoff"->True|False|>
```

`"Monogon"` is an **id into the association's own `"Faces"` list**, not
duplicated geometry — the consumer already holds that boundary polyline. As with
`"Pass"`, `"View"` records what was asked for and is deliberately not applied to
the geometry: shrinking the loop away is the animator's job, and this gives it
the boundary it needs to do it.

At most one move key appears per record, matching the one `#move` line a record
may carry. Sibling keys make that a convention rather than a shape; emitters
must not write two.

## Other step kinds (reserved, args to be specified when instrumented)

- `kind=r2`, `kind=r3` — **not needed as kinds of their own.** A reducing R2 *is*
  a pass move with an empty cross list: `k = 0` with `L = 3`, so the net crossing
  change is `k - (L-1) = -2`. Check 3 already carries that case explicitly ("or
  `= L(depart)` if `k = 0`"), checks 1 and 5 are vacuous when nothing is crossed,
  and check 6 holds. An R3 is a pass move that is not reducing (`k = L-1`),
  expressible for the same reason, with no reason to search for one. Verified
  2026-09-16: six R2s found on six raw projections, each written as a `kind=pass`
  k=0 record and each drawn with `drawing: VERIFIED (both deletions)`.
- `kind=split parent=<sid>` — a summand-splitting event (connect-sum or
  split-link separation). Structural, checkable. Under **two** doubts, both
  recorded in "Open questions": it is probably a *stream-level* concern rather
  than a drawable payload (a split forks the record stream into two summand
  timelines, which a renderer drawing records in sequence has no notion of),
  and it may not belong here at all, being the one candidate kind that is not
  an isotopy — the diagram does not change, only its description.
- `kind=redraw` — a re-embedding step (Reapr). Fully specified below; with
  its payload it is **computationally checkable** (a heavier verification
  kernel than the combinatorial kinds, but not a trust-me jump).

## Step kind: `redraw` (Reapr re-embedding)

A Reapr step replaces the diagram by re-embedding it in 3-space and choosing
a better projection. Recorded naively ("the diagram changed, trust me") it
would be unverifiable. Instead the record carries the witness of the isotopy:

```
#move kind=redraw rot=<r00,r01,r02,r10,r11,r12,r20,r21,r22>
#embedding rows=<n>
<n rows of 3-column float coordinates, existing embedding TSV conventions>
<5-column signed PD rows of the before-diagram>
<blank line>
```

- **`#embedding`**: the 3D polygonal embedding `E` the step used, in the
  same 3-column TSV format the tools already read and write (component
  conventions included — `knoodledraw --embedding` output is the reference).
  Floats are printed with round-trip precision (`%.17g`).
- **`rot`**: a rotation matrix `R` (row-major, orthonormal with `det = +1`
  within a stated tolerance; verifiers check this). The new view is `R·E`.
- The projection convention (which axis is the viewing direction, larger
  coordinate on top) is **whatever `PD_T::FromCoordinates` /
  `LinkEmbedding` implement** (`src/PlanarDiagram/FromEmbeddings.hpp`) —
  cited as normative rather than restated here, so this spec cannot drift
  from the code.

### Verification contract

1. `project(E)` reproduces **this record's PD snapshot exactly** (the
   recorder must emit as its before-snapshot the PD that `FromCoordinates`
   returns for `E`, so equality is literal, not up-to-relabeling).
2. `project(R·E)` reproduces the **next record's PD snapshot exactly**
   (same recorder-side rule for the after-snapshot).
3. `R` is orthonormal, `det(R) = +1` (tolerance stated in the trace header
   once fixed).

Check 1 and 2 are runnable today: `knoodlesimplify -s=0` is precisely the
embedding→PD converter with no simplification. Rigid rotation preserves
link type by theorem, so a `redraw` passing these checks is verified —
the trust base is `FromCoordinates` itself (plus float projection
robustness) rather than the `LeftDarc` walk, which is why proof-grade and
redraw-grade remain *labelled distinctly* even though both are checkable.

### Animation recipe (what the payload buys)

1. **Lift**: interpolate the z-coordinate (viewing-axis coordinate) from 0
   to its value in `E`. The projected diagram is constant throughout (x,y
   fixed, over/under order fixed for every t > 0), so the viewer watches the
   before-diagram inflate into 3D without any combinatorial event.
2. **Rotate**: follow the geodesic from identity to `R` in SO(3). The
   projected diagram morphs continuously — the tangencies and triple points
   the camera sweeps through are exactly the Reidemeister moves of the
   isotopy, happening on screen.
3. **Flatten**: interpolate the new viewing-axis coordinate of `R·E` to 0,
   landing on the after-diagram.

Endpoints are pinned by the verification contract; the interior of the
movie is honest by construction (a rigid rotation of a fixed curve).

## Layout transitions: tension morphs (renderer guidance)

Two drawings of the *same* PD snapshot arise all over this pipeline: after a
pass move is shown inside the frozen before-layout (strand swapped for its
routed corridor), that drawing and the fresh OrthoDraw layout of the
after-diagram are two drawings of one diagram; likewise the raw projection
at the end of a `redraw`'s flatten phase vs. the tidy layout of the same
diagram; likewise any deliberate re-layout. Naively these are hard cuts.
They can instead be animated continuously and injectively:

1. **Common refinement.** Both drawings realize the same combinatorial map
   (same snapshot), differing only in bend/subdivision vertices per arc —
   take the common refinement of the arc subdivisions.
2. **Compatible triangulations.** Triangulate the two realizations
   compatibly (same triangulation graph validly embedded in both; Steiner
   points as needed — Aronov–Seidel–Souvaine, O(n²) Steiner worst case,
   irrelevant at watchable diagram sizes). Watch for angle-π polygon
   vertices from collinear bend chains in orthogonal layouts.
3. **Tension coordinates.** Fix the exterior face as a large square in both.
   For each drawing, express every interior vertex as a positive
   convex combination of its neighbors (possible for any embedded
   triangulation with convex boundary, e.g. mean-value coordinates —
   Floater).
4. **Interpolate tensions, not positions.** Linearly interpolate the two
   weight sets and solve the sparse linear system per frame. Every
   intermediate is a valid planar embedding — no triangle ever flips
   (Floater–Gotsman / Surazhsky–Gotsman injective-morphing theorems).

**What the trace must guarantee** for this to work: both endpoint drawings
use the *same* exterior face, embedded as the convex boundary. Hence
`#view exterior=<da>` — the face `L(da)` that renderers must pass to
OrthoDraw as its exterior-face argument (the constructor takes it).

**Exterior stability across steps.** For combinatorial kinds the descriptor
itself is the correspondence map across the step (it names exactly the
arcs/faces it touches), so "the exterior face is unchanged by the move" is
well-defined and checkable: recommended practice is to name the exterior in
consecutive records by a surviving darc (one not in `strand`/`cross`/the
move's args). A step that *must* change the exterior face (the move consumes
it) is a **seam**: renderers fall back to a cut there, and recorders should
choose exteriors to make seams rare — a single compatible choice threaded
through the whole sequence is the ideal. Across `redraw` there is no
combinatorial correspondence (the witness is geometric), so
exterior-continuity is not defined there; the lift/rotate/flatten animation
covers that transition instead, plus one tension morph from the raw
projection layout to the tidy layout on the far side.

## Annotations

- `#faces f<min_darc>=<da,da,...>; ...` — the face table as boundary darc
  cycles, keyed by canonical (minimal) darc. Optional, redundant, fail-loud:
  consumers that use it must verify it against the snapshot and abort on
  mismatch. Emitters should include it; it makes traces greppable and
  self-documenting.

## Determinism

Proof replays must render identically across runs: layout randomization
(`randomize_bends`, `randomize_virtual_edgesQ`) stays off in this pipeline,
and any seeded choice an emitter makes must be recorded in the stream.

## Consequences for OrthoDecorate (branch `orthodecorate`)

- Phase 2 (portals, waypoint selection) is unaffected internally — pure
  geometry on one layout.
- Phase 3's public API takes a parsed `pass` descriptor (strand darcs,
  depart, cross list, land) and resolves faces itself via `ArcFaces()` in
  O(1) per darc — it does **not** take a face-index path. The API surface is
  the descriptor.
- The corridor's endpoints are dots at portal points of the strand's end arcs,
  not quadrant cells beside the anchors; the arc between anchor and dot is
  shared by the two deletion views. See "The two deletions" above.
- Phase 4 renders the routed corridor; the ASCII overlay is one backend. The
  route stays a polyline in OrthoDraw grid coordinates (the same coordinate
  system as `ArcSplines()`/plotting), so graphics backends (SVG/TikZ/WL) are
  siblings, and the "move happens within one fixed layout" animation trick
  (before-layout, swap strand for routed corridor, then cut to fresh layout)
  falls out for free.
- The knoodledraw debug mode reads records in this format (handwritten or
  emitted by middlestrands) — it is the shakedown cruise for this spec.
  Nothing gets proposed to Henrik (the eventual one-callback instrumentation
  hook in Simplify) until the format has survived that use.
- **Implemented today**: `knoodledraw --move="strand=... depart=...
  cross=...:u land=..."` accepts exactly the `pass` payload grammar (the
  `#move` and `kind=pass` tokens optional) and overlays the corridor on the
  drawing — heavy gold strokes, corridor visibly broken at under-crossings,
  anchors emphasized in red, and a dot at each end where the corridor branches
  off the strand's own arc. A rejected descriptor prints which spec check
  failed and exits nonzero.
- **Also implemented**: `knoodledraw --find-pass=A,B` names only the strand's
  **first and last arc** and lets Knoodle's own
  `PlanarDiagramComplex::FindShortestRerouting` supply the corridor.

  This is the form to prefer, because of what `Reroute`'s contract actually
  says. Its one stated precondition is that the corridor came from the
  shortest-path search and *is* a shortest path — not well-formedness, not
  strict shortening, optimality. A hand-written `--move=` descriptor cannot
  promise that, and a well-formed descriptor outside the contract can have the
  applier do something unrelated to it (the PR #30 reproducer is exactly that:
  `k = 8` against a minimum of `4`). Naming the endpoints instead of the
  corridor puts the move inside the contract **by construction**, so the
  drawing depicts a move production would actually accept and carry out.

  `FindShortestRerouting` caps the corridor at `L-2`, so what it returns is
  both optimal and strictly simplifying, and it does not reroute — it returns a
  path and leaves the diagram alone. **Finding nothing is an answer, not a
  failure:** a strand with no strictly shorter route through the faces simply
  has none, and the tool says so, draws the diagram with that strand
  highlighted and no corridor, and exits 0.

  The same search backs `test/oracle_vs_reroute --find A B`, which runs the
  whole chain in one command: find the corridor, confirm it is a shortest path,
  verify both deletions of the drawing against `AfterDiagram`, then run
  `Reroute` through the friend harness and compare port-by-port and by
  determinant. Shared via `tools/find_pass.hpp`, so the picture and the check
  cannot disagree about what was found.
- **Also implemented**: `--pass-disk` shades the **swept disk**. Between the
  two dots, `W` and the corridor are two paths with the same endpoints, so
  together they close into a loop; when they do not cross each other that loop
  is a Jordan curve, and the region it bounds is exactly the disk `W` sweeps
  out as it slides over to the corridor — the move's before and after
  positions are its two sides. When they do cross, the loop is not simple and
  the complement has several bounded pieces; the largest is shaded.

  The disk is a region of the *plane*, not a face of the diagram: other
  strands run across it, and it is the union of every face they cut it into.
  It is shaded exactly the way `--checkerboard-coloring` shades faces (a dim
  background, `.` in `--ascii`), and only blank cells are shaded, so arcs
  crossing the disk stay legible. Its defining property — every cell on its
  boundary belongs to `W` or to the corridor, and it touches both — is
  asserted in `test/pass_view_check.cpp`.
- **Also implemented**: `--pass-view=both|before|after` selects which of the
  two deletions to draw (see "The two deletions" above). `both` is the default
  superposition; `before` deletes the corridor, leaving the input diagram with
  its dots marked; `after` deletes `W`, healing the transversals it crossed, so
  the drawing is the diagram the move produces laid out in the frozen
  before-layout. The single-deletion views omit the `--mono` `w`/`p` marker
  letters — with only one of the two present there is nothing to disambiguate,
  and the letters would only corrupt the strokes for a reader.
- **Also implemented**: `knoodledraw --trace --verify` makes two separate
  claims good, and reports them separately because only one needs lookahead.

  `#verify step <n> drawing:` is the two-deletions contract, checked entirely
  inside one record — each view is rendered, parsed back, and compared
  port-by-port under the geometric correspondence (above). This is the check
  that would have caught the arc-label aliasing of PR #30 without anyone
  noticing a corridor attached oddly.

  `#verify step <n> trace:` is the claim that what the move produces really is
  the *next* record's snapshot, so it needs one record of lookahead.
  `OrthoDecorate::AfterDiagram` builds the result from the descriptor alone —
  never calling the applier that produced the trace — and since a PD-code
  snapshot renumbers everything, there is no shared labelling to appeal to;
  this one therefore asks the weaker question of whether the two diagrams are
  isomorphic at all. That needs no graph-isomorphism machinery: a rooted flag
  determines the map, so fixing one crossing and trying each partner in turn is
  O(n²), the same bound and the same reason as Weinberg's planar-graph test.
  Unlike the MacLeod code it replaced, it applies to links.

  Either reporting `MISMATCH` exits nonzero. A move whose corridor does not
  route in the record's own layout reports `drawing: UNCHECKED` with the
  routing failure — the descriptor may still be well formed, and that is a
  fact about the layout, not the move.
- **Also implemented**: `knoodledraw --trace` reads a trace stream of this
  spec's records and renders each snapshot under its echoed headers:
  `#move kind=pass` becomes the corridor overlay on that record's diagram,
  `#view exterior=<da>` pins OrthoDraw's exterior face to `L(da)` (via
  `ArcFaces`), `#embedding` blocks are skipped with a note (animating the
  redraw witness is a later backend's job), and other kinds/`#faces` lines
  are echoed unrendered. Malformed records and rejected descriptors abort
  with a line-numbered message and nonzero exit. Example stream:
  `test/trace_example.txt`.
- **Also implemented**: `--verify` checks a record's feasibility witness when
  it carries one, reporting `#verify step <n> disk (V0):` and
  `classes (V4):` (see "The feasibility witness" above). The reader
  (`src/MoveTrace.hpp`) parses `#feas`/`#fvar` into the record and refuses the
  malformed forms listed there; the checks are `tools/witness_check.hpp`, and
  `test/witness_check` pins them on four real witnesses, on corruptions each
  check must catch, and on middlestrands' Whitehead-link witness that omits a
  trapped circle.

## Open questions

- ~~Exact arg schemas for `r1`/`r2`/`r3`.~~ **Settled 2026-09-16.** `r1` is
  specified above; `r2` and `r3` need no kind of their own, both being pass
  moves (see "Other step kinds"). The instrumentation target has also moved:
  it is NOT Simplify. Simplify stays uninstrumented by design — it performs
  millions of moves reducing hundred-million-crossing inputs, where a trace is
  neither storable nor watchable, and where recording would tax a hot loop for
  a purpose unrelated to it. The traced simplifier is the verified one being
  prototyped in middlestrands.
- **Is `split` a move at all, and should a verified simplifier ever split?**
  Splitting is a PERFORMANCE device — work the pieces separately — and neither
  proving nor drawing obviously benefits from it. It is also the one candidate
  kind that is **not an isotopy**: unlike every other kind the diagram does not
  change, only its description does, which makes `#move kind=split` a category
  error rather than merely an awkward fit. Two ways out, both open: drop
  splitting from the verified simplifier altogether (the trace is then one
  linear chain per component, and no renderer ever needs concurrent summand
  timelines), or keep it as a stream-level header that is not a move. Note that
  a crossingless component coming free is a DIFFERENT event, already handled by
  `#spinoffs`, and is unaffected by this question.
- Whether `#faces` should also carry per-face canonical names for the WL
  side's convenience, or WL derives them (leaning: derive).
- `redraw` instrumentation: whether Reapr exposes (or can be made to expose,
  via the same one-callback hook) the embedding `E` and rotation `R` at the
  moment it commits to a projection — and whether its projection step is
  exactly one rotation or a compound (if compound, record the composition or
  one step per rotation).
- `redraw` for links: component correspondence between embedding strands and
  PD components across the step (the `#color` machinery from the color
  roundtrip work is the likely vehicle).
- Numeric tolerances: for the `R ∈ SO(3)` check, and how close to a
  degenerate projection the recorded `E`/`R` may legally sit (verifier
  should probably re-run `FindIntersections` and demand a clean pass, which
  `-s=0` already does).
- Exterior-face seams: whether a recorder can always thread one compatible
  exterior choice through a whole Simplify run (does any move sequence
  *force* consuming every candidate exterior?), and — if seams prove
  unavoidable — whether to animate them as sphere re-rooting (project to
  S², rotate the chosen face through infinity) rather than cutting. Parked:
  seams are believed rare under a careful choice, so cuts suffice for now.
- Compatible-triangulation details for step 2 of the tension morph
  (Steiner-point placement realizing one triangulation graph in both
  orthogonal layouts; angle-π bend-chain vertices).
