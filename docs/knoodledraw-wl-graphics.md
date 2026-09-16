# knoodledraw — Wolfram graphics output mode

*Drafted 2026-07-01. **Status: IMPLEMENTED** as `knoodledraw --format=wl`
(verified 2026-08-10 by running it). The design below is what shipped; the
emitted association carries `BoundingBox`, `Arcs` (each with `Id`,
`Component`, `Color`, and a `Points` polyline), `Crossings` (`Id`, `Pos`),
and `Faces` (`Id`, `Exterior`, `Color`, `Boundary`), plus `<|"Unknot"->True|>`
markers for 0-crossing summands. Read the "Open questions" section at the
bottom with that in mind — some entries were settled by the implementation.*

*One gotcha worth carrying into any new renderer: `--format=wl` polylines come
from `ArcLines()`, whose endpoints are deliberately shortened by
`x/y_gap_size` at under-crossings — that break IS the over/under art, so it is
correct here. Do not reuse those polylines for topology (face flood-fill,
adjacency): the gaps leak between faces. Use `ArcVertices()` +
`VertexCoordinates()` for anything combinatorial. This bit us in
`OrthoDecorate` Phase 1 (see `docs/move-descriptor.md` and commit 70f55a0).*

*Original motivation:*
*Drafted 2026-07-01. Motivated by the
Knoodle-paclet feasibility discussion: a paclet exposing `KnoodleDraw` as a
drop-in for KnotTheory\`'s `DrawPD` wants a native, styleable Mathematica
`Graphics` object in a notebook cell, not ASCII/Unicode art. Code references
below were **verified by reading `src/OrthoDraw/Coordinates.hpp` and
`Plotting.hpp`** on this date unless marked otherwise.*

## Goal

Add an output mode to `knoodledraw` that emits the **orthogonal layout as
structured geometry** (not a rasterized character grid, and not a pre-styled
picture). A thin Wolfram-side layer in the paclet turns that geometry into
`Graphics[...]`, owning all styling.

## Why this is cheap: the geometry is already computed

The ASCII renderer is only one rasterization of geometry `OrthoDraw` already
computes and caches. The same accessors feed a structured exporter with no new
layout work and **no changes to Henrik's `src/`** — they are public, cached
methods `knoodledraw` already links against:

- `Width()` / `Height()` — bounding box of the integer grid
  (`src/OrthoDraw/Coordinates.hpp:15` / `:3`).
- `VertexCoordinates()` — integer grid `{x,y}` of every vertex
  (`src/OrthoDraw/Coordinates.hpp:45`).
- `ArcLines()` — **per-arc polyline** of integer grid points, corner by corner
  (`src/OrthoDraw/Coordinates.hpp:298`). A ragged array: one sublist of
  `{x,y}` points per arc.
- `ArcSplines()` — rounded-corner variant of the same
  (`src/OrthoDraw/Coordinates.hpp:399`), governed by the rounding-radius
  settings in `Plotting.hpp`.

**The over/under breaks are already baked into `ArcLines()`.** When an arc
terminates as the *under*-strand at a crossing (`A_overQ(a,Head/Tail)` false),
its endpoint is inset by `x_gap_size`/`y_gap_size`
(`src/OrthoDraw/Coordinates.hpp:341–345` for the tail, `:374–378` for the
head). So the classic broken-under-strand look comes for free from the point
data — the exporter does not have to compute gaps itself.

Net: a WL-graphics mode is a ~50-line **serializer** over `ArcLines()` /
`ArcSplines()`, living entirely in `tools/knoodledraw.cpp` (our domain).

## Design principle: emit geometry, not `Graphics`

Do **not** have C++ print ready-made `Graphics[{Line[...], ...}]`. Mirror
`knoodleidentify`, which already emits *semantic* data (a Wolfram association of
`KnotSymbol[...]`, `tools/knoodleidentify.cpp` ~569–591) rather than a formatted
string. Emit unstyled, `ToExpression`-ready geometry and let the paclet assemble
and style the picture. Rationale:

- **Styling stays in Mathematica** — colors, thickness, dashing, labels, and
  restyling *without recompiling C++*. Putting styling in the exporter would
  mean reimplementing WL graphics options in C++.
- **Unlocks what a real `Graphics` is for**: `Tooltip` on arcs/crossings/
  components (the indices already computed for `--label-*`), color-by-component
  (the existing `comp_map`), zoom, and vector export (PDF/SVG).
- **Keeps both crossing render styles open** (see below).

### Proposed output schema

A single `ToExpression`-ready association per diagram:

```
<| "BoundingBox" -> {w, h},
   "Arcs"      -> { <| "Id"->a, "Component"->c, "Points"->{{x,y},...} |>, ... },
   "Crossings" -> { <| "Pos"->{x,y}, "Over"->arcId, "Under"->arcId, "Sign"->±1 |>, ... } |>
```

- `"Points"` comes straight from `ArcLines()` (polyline) or `ArcSplines()`
  (rounded). Under-strand gaps are already present in the points.
- Emit per-crossing `"Over"`/`"Under"` even though the gaps already encode the
  break, so the WL renderer can *choose* its style:
  1. **Broken under-strand** (default) — draw every arc's polyline as-is; the
     baked-in gaps produce the breaks. Simplest.
  2. **Z-ordered strands** — draw strands solid and paint the over-strand on
     top at each crossing. Needs the over/under, hence emit it.

## Reframe: a `--format=` switch, not a one-off mode

The same serialized geometry is exactly what an **SVG or TikZ/PGF** exporter
would consume. So the clean move is to factor `knoodledraw`'s output behind a
format switch rather than bolt on a single WL mode:

```
--format=unicode   (current default)
--format=ascii     (= current --ascii)
--format=wl        (new: geometry association for the paclet)
--format=svg       (future, same geometry serializer)
--format=tikz      (future)
```

The existing ASCII/Unicode path becomes one renderer of the geometry it already
uses; `wl`/`svg`/`tikz` are additional renderers over `ArcLines()`/
`ArcSplines()`. This is an architecture improvement independent of the paclet.
(`--ascii` should stay as a back-compat alias for `--format=ascii`.)

## Coordinate mapping notes

- Integer grid → `Graphics` coordinates is a direct 1:1 map, almost certainly
  with a **y-flip** (grid origin vs. WL's bottom-left origin) — confirm the axis
  convention against `VertexCoordinates()` when implementing.
- Rounded corners: emit `ArcSplines()` and render with `BezierCurve`, or emit
  `ArcLines()` and let WL round. Rounding radius lives in the `Plotting.hpp`
  settings (`x_rounding_radius`/`y_rounding_radius`).

## Pass moves and trace streams (added 2026-09-16)

Until 2026-09-16 `--format=wl` was silently incompatible with the pass-move
flags: the wl branch in `DrawKnot` returned before the overlay code ever ran,
so `--move` and `--trace` died with "`--move` descriptor rejected: no drawable
summands" (the fail-loud guard firing on a move that was never *tried*), and
`--find-pass` was a silent no-op. All three now work.

### `"Pass"` — a routed pass move, as geometry

When a move routes, the diagram's association carries one extra member:

```
"Pass"-><| "Kind"->"pass"|"middlepass", "View"->"both"|"before"|"after",
           "Strand"->{da,..}, "Depart"->da, "Land"->da,
           "Corridor"-><|"Points"->{{x,y},..},
                         "Crossings"->{<|"Id"->i,"Pos"->{x,y},
                                         "Darc"->da,"Over"->True|False|>,..}|>,
           "Dots"->{{x,y},{x,y}}, "Anchors"->{{x,y},{x,y}},
           "Disk"->{{x,y},..} |>
```

The key is **absent** when no move routed, so test for it rather than for a
flag. `"Disk"` appears only under `--pass-disk`.

The ASCII backend rasterizes the corridor into character cells
(`RenderPassRoute` → `OverlayCell_T` → `StampPassOverlay`); wl emits the route's
own polyline instead — the same data, one abstraction level up.

**Two things to know before rendering:**

1. **Corridor points may fall outside `"BoundingBox"`.** `OrthoDecorate` routes
   inside a free margin ring around the drawing, and a corridor through the
   exterior face uses it, so coordinates can be negative or exceed `w`/`h`. Fit
   to the union of the diagram and the corridor, not to `"BoundingBox"`.
   (Everything is in one space: `OrthoDecorate` works in drawing coordinates
   shifted by its margin — `src/OrthoDecorate.hpp`, "drawing coords = grid
   coords − Margin()" — and the margin comes off in the emitter.)
2. **`"View"` is a record of what was asked, not something applied to the
   geometry.** The ASCII backend renders `after` by deleting the strand and
   healing the arcs behind it (`ApplyAfterView`) — real surgery on the drawing.
   The wl path emits the full before-geometry plus the whole corridor in every
   view and leaves the composition to the consumer, who has the strand darcs
   and the corridor and can do it faithfully.

   **This is a design choice, not a missing feature** (JHC, 2026-09-16). ASCII
   has to do the surgery inside `knoodledraw` because ASCII *is* the endpoint —
   there is no consumer downstream to do it. `wl` has one, and this is the same
   principle as "emit geometry, not `Graphics`" above: the exporter ships
   unstyled, uncomposed data and the consumer owns what is made of it. A
   renderer that wants the after-view has strictly more to work with than a
   pre-composed one would give it — it can cross-fade the strand into the
   corridor rather than cut, which is exactly what the animation work needs
   (see the tension-morph design in `docs/move-descriptor.md`). Revisit only if
   a consumer turns out to want the composed view and nothing else.

### `--trace --format=wl`

One association per record, one per line, each self-contained — so the trace is
consumed exactly like ordinary wl output. The record's context is folded into
the same association it describes:

```
<|"Step"->n,"Headers"->{".."},"Move"->"kind=pass ..","Exterior"->da,
  "Candidate"->True, "BoundingBox"->.., "Arcs"->.., "Pass"->.. |>
```

A 0-crossing record emits `<|"Step"->n,..,"Unknot"->True|>`.

**Everything else goes to stderr in wl mode** — echoed `#`-headers and all
`--verify` output. Commentary interleaved with geometry is what made the old
output unparseable, and the paclet's reader (`runGeometry`, which keeps only
lines matching `StringStartsQ["<|"]`) already assumed it would not be there.
So stdout is pure WL, and `--verify` still reports to the terminal and still
exits nonzero on a mismatch.

## Open questions / TODO checklist

- [ ] Confirm the per-crossing over/under + sign are reachable from
      `knoodledraw`'s context (the ASCII path already draws breaks, so the data
      is available — **locate the exact accessor / PD call** and cite it here).
- [ ] Decide `ArcLines` vs `ArcSplines` as the `wl` default (polyline is
      simpler and exact; splines look nicer). Possibly expose via an option.
- [ ] Confirm the grid y-axis orientation and whether a flip is needed.
- [ ] Wire `--format=` into arg parsing (`tools/knoodledraw.cpp`), keeping
      `--ascii` as an alias; route `wl` to the new serializer.
- [ ] Paclet-side: `Graphics` assembler + styling, `Tooltip` wiring, and a
      demo target — `KnoodleDraw[KnotData[{6,2}]]` rendering as a native,
      tooltip-annotated `Graphics` (exercises the geometry exporter and the
      representation-conversion spine at once).

## Relationship to the paclet

This mode is the rendering half of the proposed Knoodle paclet (three functions:
`KnoodleDraw`, `KnoodleSimplify`, `KnoodleIdentify`). `KnoodleIdentify` already
speaks Wolfram (its default output is an association); `KnoodleSimplify` round-
trips PD codes; this mode gives `KnoodleDraw` a real notebook graphic. The
paclet is expected to wrap the CLI binaries via `RunProcess`, ship per-platform
clang-built binaries plus the ~23 MB KLUT data, and normalize input
representations (`PD[X[...]]`, DT/Dowker, Gauss, sampled `SpaceCurve`) to
Knoodle's column-TSV on the Wolfram side.
