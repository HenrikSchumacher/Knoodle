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
   philosophy as the CLI fail-loud contract).

## Inherited conventions (normative)

These are Knoodle's existing conventions; we cite them rather than invent:

- **PD snapshot** = the 5-column signed PD code, one row per crossing:
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
#view exterior=<da>           (optional, recommended; see Layout transitions)
#move <descriptor>            (absent on terminal records)
#faces <annotation>           (optional)
<5-column signed PD rows, one per line>
<blank line>
```

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
- Streams should begin with `#trace v=0` so the format can evolve.

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
   active and pairwise distinct; no crossed arc belongs to the strand; and the
   crossed arcs are pairwise distinct — the corridor is **arc-disjoint**.

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
  `middlepass` it is quotient-simplicity of the corridor plus the §G
  feasibility witness (below).

## Step kind: `middlepass`

Middlestrands' `MiddleStrandSimplifier` moves (see
`handoff/middlepass-descriptor-emission/`): identical grammar and checks
1–4 as `pass`, but the over/under tags are **per-crossing** — check 5 is
dropped. This is the majority move class in practice (67% of applied moves
in the first shakedown), not an edge case.

```
#move kind=middlepass strand=... depart=DA cross=DA:u|o,... land=DA
```

Well-formedness = checks 1–4. Soundness additionally requires (verifier
tier, not rendering):

- **quotient-simplicity** of the corridor: deleting the strand merges the
  two flank faces of each strand arc; the corridor must not revisit a
  *class* of that quotient (union-find over flank-face pairs; middlestrands
  will contribute the spec text and reference check);
- the **§G feasibility witness**: header lines `#feas ...` / `#fvar ...`
  carrying the disk and the piece-class labelling, verified by the
  check-don't-solve contract V0–V5 proposed in
  `ROUND-1-RESPONSE-ADDENDUM-feasibility-witness.md`. These header names
  are RESERVED here; the normative text lands after the witness emitter
  exists and real payloads have been validated against it (same shakedown
  discipline as the grammar itself).

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

## Other step kinds (reserved, args to be specified when instrumented)

- `kind=r1`, `kind=r2`, `kind=r3` — Reidemeister moves; small darc-based arg
  lists, to be pinned down when Simplify instrumentation lands. Locally
  checkable, proof-grade.
- `kind=split parent=<sid>` — a summand-splitting event (connect-sum or
  split-link separation). Structural, checkable.
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

## Open questions

- Exact arg schemas for `r1`/`r2`/`r3` (decide when instrumenting Simplify).
- Split bookkeeping details: whether the parent's terminal record or the
  children's initial records carry the `split` descriptor, and how unknot
  eliminations are recorded.
- Whether `#faces` should also carry per-face canonical names for the WL
  side's convenience, or WL derives them (leaning: derive).
- Landing/departure flank when an anchor crossing is itself removed by the
  move (can a pass move's anchors be R1-collapsed in the same step in
  middlepass? If so the descriptor needs a compound kind or a step split).
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
