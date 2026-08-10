# Move descriptors and trace streams — design

Status: draft convention (v0), 2026-08-10. Authoritative spec for the
combinatorial move-descriptor format shared by (a) the planned knoodledraw
overlay/debug mode for visualizing pass-move feasibility, and (b) the
longer-term "knoodleprove" pipeline, in which an instrumented Simplify /
middlepass records the moves it chose and a renderer replays them as a visual
proof. Companion to the OrthoDecorate work on branch `orthodecorate`.

## Design principles

1. **Descriptors are PD-level combinatorics.** A move is described against a
   specific PD snapshot using arc/darc references only — never OrthoDraw grid
   coordinates, bend vertices, or any other per-layout geometry. Geometry is
   re-derived by each renderer.
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
  (`Faces.hpp`). `ArcFaces()(a,1)` is the face right of the forward darc
  (`Faces.hpp:34`).

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
   active and pairwise distinct; no crossed arc belongs to the strand.
2. `L(cross_1) = L(depart)`, and `L(cross_{i+1}) = R(cross_i)` for each
   consecutive pair — each crossing departs from the face the previous one
   arrived in.
3. `L(land) = R(cross_k)` (or `= L(depart)` if `k = 0`).
4. `depart` and `land` name faces incident to the respective anchor
   crossings.
5. All over/under tags equal.

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
#move kind=pass strand=11 depart=7 cross=3:u land=9
```

reads: reroute the strand consisting of arc 5 (traversed along its
orientation: darc 11 = 2·5 + Head), leaving its tail anchor through face `L(7)` =
`{3,11,7}`, passing **under** arc 1 by crossing darc 3 (stepping from
`L(3) = {3,11,7}` to `R(3) = L(2) = {2,9}`), and reaching the head anchor
through face `L(9) = {2,9}`. Checks: `L(3) = L(7)` ✓, `L(9) = R(3)` ✓.

## Other step kinds (reserved, args to be specified when instrumented)

- `kind=r1`, `kind=r2`, `kind=r3` — Reidemeister moves; small darc-based arg
  lists, to be pinned down when Simplify instrumentation lands. Locally
  checkable, proof-grade.
- `kind=split parent=<sid>` — a summand-splitting event (connect-sum or
  split-link separation). Structural, checkable.
- `kind=redraw` — a re-embedding step (Reapr). **Not locally checkable**: it
  is a link-type-preserving jump, not a certificate. Traces containing
  `redraw` steps are visual *narratives*; a proof-grade trace contains only
  the checkable kinds. Renderers must visibly distinguish the two (this is a
  contract, so that "knoodleprove" never silently overclaims).

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
