# The Knoodle animator: design

Status: design agreed 2026-11-10 (JHC). Henrik: at about 6,000 lines this needs no
design review. The estimates below are from reading the prototype, not from built code.

Built (2026-11-10): the library in `src/Animate/`, `tools/knoodlemovie` and
`players/knoodle-player.html`, 6,601 lines, with four C++ tests. Every kind of move in
the committed traces is animated. See "What is built" and "What the first three pieces
found" at the end.

This document replaces the prototype on the local branch `orthomorph` (112 commits,
about 21,000 lines of animation code, never pushed). That branch was the experiment
that told us what the animator should do. It is the specification's source, not the
implementation's.

## What the animator does

`knoodleprove` checks a move trace: a sequence of diagrams joined by pass moves, middle
pass moves, curl removals (R1), and redraw moves. The animator turns a checked trace into
a movie of the simplification, as a continuous motion with no jumps.

`knoodleprove` is the only source of truth. The animator's contracts exist to give the
code a concrete target, and they are these:

- The drawing before and after a pass or middle pass is a valid diagram of the
  before-state and the after-state.
- During a reshape, the shadow of the diagram stays a valid embedding of the same plane
  graph.
- A redraw and a turn of the sphere (below) are views of an embedded space curve at
  every frame.
- A spin-off's slide to the tray is an illustration and claims nothing.

## Budget

| Piece | What it does | Lines |
|---|---|---|
| `PlaneDrawing` | The one drawing type; export from `OrthoDraw`; polyline kit (arc length, sub-curve, intersection, inside, box); matching two drawings point for point by arc length | 350 |
| Mesh kit | Ear clipping, Delaunay refinement, validity check, sparse solve | 400 |
| Morph | Compatible triangulation of two drawings (faces, frame, cuts, overlay); mean-value weights; frames; steady centre and shape | 800 |
| Sweep | Harmonic level curves timed by area; tracing and thinning | 420 |
| Turn | Change of exterior face: choice of pole; the Möbius image with edges cut into arcs; heights from crossings; where the build loop inserts it | 180 |
| Pass and middle pass | Route in the layout; still strands and the moving strand; splice the after-drawing; over and under roles from the witness | 500 |
| R1 | The loop and its contraction; last curl; runs of curls | 120 |
| Redraw | Choice of oblique view; the lift, turn and fade; morphs in and out | 450 |
| Spin-offs | The freed loop to a rounded square; the tray and its raster order; colours from the trace | 200 |
| Animation | Scene model; timeline; camera boxes; the build loop over records | 450 |
| `ToJSON` | Writer for one format | 100 |
| `FrameAt` | Display list at a time: gaps, disk, halos, corner rounding, space scenes | 280 |
| **Library** | header-only C++ in `src/Animate/`, not included by `Knoodle.hpp` | **4,250** |
| `knoodledraw --animate` | Read trace, build, write | 120 |
| Web player | One HTML file in `players/`, Canvas, no JavaScript dependencies, no build step | 670 |
| `knoodlemovie` | C++: rasterize the display list, write raw frames for ffmpeg | 300 |
| Tests | C++: a contract check (about 400) and a run over the committed traces (about 300) | 700 |
| **Total** | | **6,040** |

The range to expect is 5,500 to 7,000. If every risk at the end goes badly, about 8,000.
The Mathematica consumer (about 250 lines) lives in `Knoodle-paclet`.

## Design

```
move trace ──> thread, layout ──> PlaneDrawing ──> moves ───────> Animation ──> ToJSON  ──> web player, paclet
 (checked)     (OrthoDraw,            │  ^           │               │
               OrthoDecorate)         │  └───────────┘               └────────> FrameAt ──> knoodlemovie,
                                      └──────────> morph ──────────────^                    native embedders
                                                     ^
                                    mesh kit ────────┴──> (also the sweep inside moves)
```

### One drawing type

`PlaneDrawing` is a diagram drawn with floating-point polylines: one polyline per arc,
crossings as shared points, the over strand at each crossing, the components in order,
and the exterior face. An `OrthoDraw` layout exports to it. A move takes one and returns
one. The view of a lattice curve is one. The morph takes two of the same diagram.
Nothing after the layout step needs lattice coordinates.

The drawing after a pass is made directly: keep the still polylines, splice the route in,
read the new arcs off by walking each component, and label them by the colour-kept
isomorphism to the trace's next record (`DiagramsIsomorphicQ`). The prototype found this
drawing by rendering ASCII art and parsing it back; that round trip stays as a test.

### One morph

Every reshape is a Floater–Gotsman morph between two `PlaneDrawing`s of the same diagram.
There is no second morph to fall back to.

The theorem cannot fail: given two triangulations with the same combinatorics and the
same convex boundary, every intermediate frame is an embedding. What can fail is around
it.

- **Building the compatible triangulation.** One always exists. The prototype found it
  with heuristics that can give up (an offset walk with step halving to cut faces that
  are not simple polygons; an overlay with fixed tolerances). A failure here is a bug in
  our construction. The rebuild uses one construction that always terminates, with
  relative tolerances, and a failure is a test failure.
- **The same plane graph.** Both drawings must have the same exterior face and
  handedness. For a fresh layout we choose it: `OrthoDraw` takes the exterior face.
  Where the exterior has to change, a turn of the sphere comes first.
- **Floating point.** Frames are solved in doubles. Every triangle of every frame is
  checked for positive area. A direct solve removes the prototype's one known stall.

### One sweep

Every pass and middle pass sweeps its strand along level curves of a harmonic function
on the pass disk, timed by swept area. The disk is always bounded. The prototype's
orthogonal sweep, its unchecked linear slide, its sweep through infinity and its separate
smoothing step are not rebuilt; thinning happens once, where the level curves are traced.

A middle pass always sweeps its witness side. In the corpus that side has consistent
levels in 445 of 445 witnessed passes.

### Turning the sphere: a change of exterior face

Some moves need a different face at infinity: a middle pass whose witness side is the
unbounded side of its loop; a thread with no face correspondence from one record to the
next; a redraw recorded on another exterior. All three get one animated move:

1. The plane rolls up into a sphere by inverse stereographic projection, carrying the
   diagram.
2. The sphere turns until a point of the new exterior face is at the pole.
3. The sphere unrolls from that pole. The diagram is in the plane with the new face
   outside, and its edges are arcs of circles.
4. An ordinary reshape morphs that drawing into a fresh layout on the new exterior.

Up to a similarity, steps 1 to 3 are the map z ↦ 1/(z − p), with p the new pole, so the
drawing they end on is closed-form and does not depend on the sphere's size. On screen
they are a space curve seen from a fixed camera: each arc gets a small height running
from above to below according to its two crossings, which makes the diagram an embedded
curve near the surface. That is the same kind of scene as a redraw, so one scene type
carries both.

Measured with a throwaway prototype (2026-11-10; 91 faces on four layouts of 11, 15, 32
and 349 crossings):

- The drawing left after the unroll is always valid: no crossings but the diagram's own.
- Unrolled as is, local scale varies by about (diagram width / face width)². Rings round
  the new pole were the short ones and must become the long outer ones, so no radial
  correction beats about the first power. A radius that grows logarithmically about the
  image of infinity gets close to that.
- With the logarithmic radius the spread is 50 to 160 times for an arbitrary face at 11
  to 32 crossings, 17 to 65 times for the roomiest face (also at 349 crossings), and
  about 1,000 times for an arbitrary face at 349 crossings.

So the builder chooses the roomiest face that serves the move. Whether the reshape copes
with such a start is not yet tested.

### Redraw records

A redraw's lattice curve must be lifted from an `OrthoDraw` layout on the exterior face
the thread has at that record. Then the view at the start of the lift is that layout, the
reshape before the redraw goes straight to it, and the morph always exists. This needs
nothing new in `src/`: `OrthoDraw` takes the exterior face, `Reapr::Levels` is public, and
the record's rotation is a lattice one, so Reapr's random rotation is never called. The
recorder writes the exterior it lifted on into the record.

Recording redraws is not part of this work. middlestrands is to become the verified
simplifier that emits them. A redraw recorded on another exterior gets a turn first.

### Spin-offs

A spin-off is a component that a move leaves with no crossings; the trace names it by
colour. It keeps its colour, shrinks to a rounded square (its outline only, never filled:
JHC, 2026-11-11), and slides to a tray: a stack
of squares that starts at the bottom left of the view and fills to the right, then
upward. A square carries no mark; its colour identifies it and its place gives the order.
The tray is a layer over the picture and stays for the rest of the animation.

### Roles, not colours

An animation is a list of scenes. A scene has paths, each with a role (still strand,
moving strand, route, disk, loop, link component k, tray square), keyframes, and for each
crossing which path is over. Colour, opacity and width come from a style sheet keyed by
role. Built-in themes: dark, light, and the talk house style. Links get a colour per
component.

Redraws and turns are stored as parameters (a 3D curve and a staged motion), not as
sampled frames, to keep files small.

### The interface

```cpp
namespace Knoodle::Animate
{
    Animation   Build  ( const std::vector<Record> & trace, const Options &, std::string & why );
    std::string ToJSON ( const Animation & );
    Frame       FrameAt( const Animation &, double seconds, const Style & );   // strokes and fills
}
```

These read no files, write no files, print nothing and hold no global state. A server,
the paclet, a tablet app or a Sage binding calls them directly.

`FrameAt` is the reference for what a frame looks like: a display list of stroked
polylines and filled polygons with gaps already cut. The web player is a port of it plus
controls. `knoodlemovie` rasterizes it and writes raw frames to standard output for
ffmpeg. Movies carry no caption text, so there is no font in the tree.

## Dependencies

No new dependency to build or install Knoodle.

| Need | Candidate | Saves | Verdict |
|---|---|---|---|
| Sparse matrix | Tensors' `Sparse::MatrixCSR`, already included | none | Use (2026-11-12, at Henrik's request): the matrix, its products and its sorting are Tensors' in every build |
| Sparse solve | Tensors' UMFPACK wrapper, already in the tree | about 60 | Use when `KNOODLE_USE_UMFPACK` is set; a small BiCGSTAB otherwise, so builds without SuiteSparse still work |
| Sparse solve without SuiteSparse | Tensors' `ConjugateGradient`, sparse Cholesky, `GMRES` | about 35 | Cannot yet. The matrices are not symmetric (mean value weights, which is what keeps a level set one curve), so the first two do not apply; `GMRES` calls `BLAS::trsv`, and the plain build has no BLAS |
| Orientation predicate | Tools' `Oriented2D.hpp`, already included | about 20 | Use |
| Triangulation | CDT (MPL-2.0), earcut (ISC) | 40 to 100 | No. Neither refines a mesh nor triangulates two drawings compatibly |
| Quality meshing | Triangle, CGAL | about 240 | No. Licences |
| Polygon operations | Boost.Geometry | about 100 | No. Compile time |
| JSON | Boost.JSON, nlohmann/json | about 50 | No. We only write JSON |
| Rasterizing for movies | a headless browser driven by a script | about 180 | No. A 200-line rasterizer in C++ is more durable than a browser and a scripting runtime |
| Video encoding | ffmpeg | n/a | Yes, found on the path when a movie is made; never touched by `make` or the light test tier |

Nothing in the animator is written in Python.

## What the prototype had that this does not

- The orthogonal grid morph (`OrthoMorph.hpp`), the settle step and the gold sliding
  pieces.
- The tidy pass. Layouts are shown as `OrthoDraw` makes them. The tidy made drawings 16
  to 33 percent smaller in area; if that is wanted, it belongs in `OrthoDraw`.
- Orthogonal sweeps, sweeps through infinity, `--animate-behind`.
- Movie output from `knoodledraw`, its six movie and theme flags, captions in movies,
  the embedded font.
- The Wolfram Language serialization and the Mathematica player (the paclet imports the
  JSON).
- knoodle-animation v1. The new format is not compatible with it.

## Sequence

1. Split the non-animation work off `orthomorph` into pull requests against main: exact
   lattice input; the exterior-thread seam fix and `ThreadInPlace`; the `OrthoDecorate`
   routing changes; the witness and `OrthoDraw` regression tests.
   (Done 2026-11-11, and it came out smaller. Exact lattice input was on main already.
   The seam fix and `ThreadInPlace` were dropped: they served the prototype, and nothing
   here calls them. The generated redraw witnesses and their generator are the local branch
   `redraw-witness-fixtures`, the `OrthoDraw` regression test is
   `orthodraw-regression-test`. The `OrthoDecorate` routing changes are kept on the local
   branch `orthodecorate-routing` and are not to be sent as a pull request: see "A text
   backend" below.)
2. Move the picture-independent headers from `tools/` to `src/` with no change in
   behaviour: exterior threading, witness checking, the R1 and redraw descriptors,
   diagram agreement.
3. The library, each step with its contract test: `PlaneDrawing` and the mesh kit; the
   morph; the sweep; R1; pass and middle pass; the turn; redraw; spin-offs; the build
   loop, `ToJSON` and `FrameAt`.
4. The web player and `knoodlemovie`.
5. Elsewhere: the paclet's consumer; the server for the KnotInfo demo.
6. The text backend (below), and with it the retirement of `OrthoDecorate`.

Separately, about 3,500 lines of `knoodledraw.cpp` are drawing logic that an embedder
would want as library calls. Moving them is its own piece of work.

## Risks

1. **Splicing the after-drawing directly** has not been written. Corner cases at the
   strand's two ends may cost more than the 150 lines allowed.
2. **One morph front end for every drawing.** The polyline front end has only been used
   around redraws, and its cut-finding step is the least robust code in the prototype.
3. **No fallback morph.** The prototype always had the grid morph behind it, so the
   failure rate of the triangulation on the corpus is unknown. Measure it first; the
   target is zero.
4. **The drawing a turn leaves** is very uneven in scale (above). The reshape from it is
   untested.
5. **Routing after a turn.** The corridor router is assumed to find the route on the
   now-bounded side in the fresh layout.
6. **The redraw view choice** may shrink less than assumed unless it can share
   `knoodleprove`'s lattice projection.

## A text backend, and the end of `OrthoDecorate` (planned 2026-11-11)

Nothing in this section is built.

`knoodledraw --trace` draws a pass move as text: the diagram on `OrthoDraw`'s grid in
box-drawing characters, with the strand, its route and the labels of arcs and crossings.
`src/OrthoDecorate.hpp` routes the strand for that picture, on the lattice. Since
`Route.hpp`, the animator does not use `OrthoDecorate`, so Knoodle has two routers and two
pictures of the same move, and the text picture is a still from the movie. JHC,
2026-11-11: `OrthoDecorate` is by now a tool for debugging and for talk pictures, and one
renderer should make both. What must not be lost: **a reliable ASCII renderer with
everything labelled clearly.**

So text is a third consumer of the animation, beside the rasterizer and the web player.

**Which frames.** The frames where a move is shown still: the hold of a pass or a middle
pass (strand, route and disk), and the drawing before and after any move. These are the
pictures `knoodledraw` draws now. A move starts from a fresh `OrthoDraw` layout, so in its
hold every still strand runs along grid lines already. The only thing off the lattice is
the route.

**The route, on the grid.** The backend staircases the route when it draws: horizontal
and vertical steps on a grid finer than the layout's. `Tighten` holds a route 0.45 of a
step off its polygon's sides where there is room, so a grid finer than that keeps the
staircase in the faces the route runs through; where the way is narrower it may not. So
the staircased route is checked the way every route is: the surgery must leave the next record's diagram. A staircase that
fails the check is not drawn. `Route.hpp` and the movies do not change. (Considered and
left alone: making `Tighten` itself keep to horizontal, vertical and 45 degrees. It would
change the movies for the sake of the text, and box-drawing characters have no diagonal
that joins `─` and `│`.)

**Labels.** Arcs and crossings are labelled, as in `knoodledraw`'s text now. The display
list `FrameAt` returns has no labels and movies carry no text, so labels belong to this
backend alone: it reads them from the drawing (a `PlaneDrawing` keeps the diagram's own
arc and crossing labels). Placing
them so that they are always legible is the part whose size is not known.

**Frames in motion** (a reshape, a sweep, a redraw, a turn) are not on a grid. The first
version does not draw them as text. If they are wanted, the display list can be rasterized
into character cells (braille dots, two by four to a cell), which needs no grid.

**What this retires, once it is as good:**

| Code | Lines |
|---|---|
| `src/OrthoDecorate.hpp` | 1,568 |
| `tools/pass_view.hpp` | 1,127 |
| the move overlay in `tools/knoodledraw.cpp` | not counted |
| `test/orthodecorate_check.cpp`, `pass_view_check.cpp`, `draw_trace_check.py`, `r1_draw_check.py` | 2,352 |

**What has to happen first:**

1. The text backend draws every pass, middle pass and curl removal in the committed
   traces, labelled, and JHC finds the labels as clear as `knoodledraw`'s. (Whether a run
   of curl removals starts each one from a layout has to be checked.)
2. middlestrands' gates (`oracle/verifier_gate.py` and three more) run
   `knoodledraw --trace --verify` and count its `drawing: VERIFIED` lines. That line says
   the picture realizes the move. Its successor is the builder's own check, that the
   surgery leaves the next record's diagram; the gates change to read that. This is a
   handoff to middlestrands.
3. `test/witness_check.cpp` builds an `OrthoDecorate` in three places, to check the
   witness's side convention against a drawing. Those checks move to a `PlaneDrawing`.

Until then `OrthoDecorate` stays on main as it is, and is not improved. The prototype's
two changes to its router (turns cost; a corridor that crosses nothing can go round the
drawing) are on the local branch `orthodecorate-routing` (968b2bb7), measured, with the
text of a pull request written. JHC, 2026-11-11: keep the branch, send no pull request. It
is insurance: if label placement turns out to be most of what `OrthoDecorate` and its
overlay are, and this backend is not worth building, those changes are wanted after all.

## What the first three pieces found (2026-11-10)

Risk 3 was "the failure rate of the triangulation on the corpus is unknown; the target
is zero". Measured with `test/animate_morph_check`. Per exterior face it makes seven
morphs between layouts of one diagram (other bend settings; another compaction; a
quarter turn; a half turn; a sheared copy turned 37 degrees at 1000 times the size; a
copy a million times smaller), and one more per diagram without the steady centre and
shape.

| Diagrams | Morphs | Could not be made |
|---|---|---|
| The committed traces' states and results: 91 diagrams of up to 266 crossings, 8 exterior faces each | 4,487 | 0 |
| 780 diagrams from `data/diagrams` (hard unknots, the link table sample), 3 exterior faces each | 17,160 | 0 |
| 20 random unknots of about 3,350 crossings, 1 exterior face each | 160 | 0 |

The first two rows are the same with UMFPACK and with BiCGSTAB; the third was run with
UMFPACK only. Every frame of every morph passed the contract (every triangle positive,
the two ends exact, no two strand pieces meeting).

It was not zero at first. Before two changes, 4 of 140 morphs on the 3,000-crossing
diagrams and 5 or 6 of 16,380 on the others could not be made, every one of them the
copy a million times smaller, every one for the same reason: "the triangulation is not
valid in the second drawing: n triangles not positive". The rings were sized to hold
both drawings in place, so the small drawing sat in a huge annulus of needle triangles
below the flatness tolerance. Now each drawing is brought to its own centre and unit
size before anything is built (and the frames are solved there), so the triangulation
does not depend on where the drawings are or how large; and a polygon's corner counts
as an ear's tip only above a second, larger tolerance, so that a point interpolated
onto a segment (off it by rounding) cannot pass for a corner.

How the construction differs from the design text:

- **No cut-finding search.** The prototype routed two cuts with step halving. Here each
  drawing's outside region is ear-clipped on its own and the cut is the shortest path
  through that triangulation (Dijkstra). One cut is enough: the cut annulus is a
  polygon that touches itself, which the ear clipping accepts.
- **The frame is a stack of rings, not a square.** Two unrelated layouts put the cut at
  unrelated angles, so a fixed frame would force the cut to wind. Instead the innermost
  ring turns with the cut and the rings between share the turn out; a strip between two
  rings has one triangulation valid for any turn up to a whole side. No search, no
  tolerance.
- **The overlay has no geometry in it.** Which chords of the two triangulations cross,
  and in what order, is read off the vertex numbers; each crossing is placed evenly
  along its chord. The prototype clipped triangles numerically with fixed tolerances.
  Ears common to both drawings are clipped first, which keeps the overlay small.
- **Only connected diagrams.** A face of a connected diagram is one closed walk, so the
  only region that needs a cut is the outside. A split diagram is refused with a
  reason; nothing in the corpus is one.

What was wrong or under-budgeted in this document:

- `PlaneDrawing` is 466 lines against 350. The export, the kit and the matching are
  about as estimated; reading the faces off the geometry (so that a drawing does not
  depend on a `PlanarDiagram`'s face numbering) and the embedding check were not
  counted. `Mesh.hpp` (405 against 400) and `Morph.hpp` (770 against 800) are on budget.
- "Use Tools' `Oriented2D.hpp`" saves nothing: its corrected predicate does not compile
  from `Knoodle.hpp` (it calls `TwoMulFMA`, declared in `Interval.hpp`, which is not
  included), and only the naive one is usable. `PlaneDrawing.hpp` has its own 6 lines.
- The UMFPACK wrapper needs each row's columns sorted (unsorted, every solve failed).
  The BiCGSTAB estimate was right, and BiCGSTAB converged on everything it was given;
  it is about twice as slow, and it was not run on the 3,000-crossing diagrams.
- `OrthoDraw` seeds its random engine itself, so a `randomize_bends` layout cannot be
  reproduced: the counts above move a little from run to run, and a failure report
  names the diagram and the exterior but cannot replay the layout.
- "Relative tolerances" has a floor the design did not mention: a drawing's interpolated
  points are only as exact as its coordinates, about 1e-16 times (distance from the
  origin / segment length). A drawing very far from the origin for its size will fail.
- Not a failure, but a cost: the overlay adds points. The 266-crossing diagram's 1,700
  drawing points become a problem of about 6,000; a 3,350-crossing one's 18,000 become
  up to 77,000 (about a third of a second a morph at four frames, with UMFPACK). Unrelated layouts also turn the innermost
  ring by more than a full turn in places (up to 26 rings of 12 points).
- Not tested: drawings that are not affine images of an orthogonal layout (the turn's
  circular arcs, a redraw's oblique view). Risk 4 stands. Mesh refinement is built and
  unit-tested but the morph does not use it; the sweep will.

## What is built (2026-11-10)

`tools/knoodlemovie TRACE` writes the animation as JSON for the web player
(`--json=FILE`), one frame as a PPM (`--frame=SECONDS`), or every frame as raw rgb24 for
ffmpeg. It has a Makefile target but is not in `all` and is not installed.

| File | What it holds | Lines | Planned |
|---|---|---|---|
| `PlaneDrawing.hpp` | the drawing type, the polyline kit, matching | 466 | 350 |
| `Mesh.hpp` | ear clipping, refinement, the sparse solve | 405 | 400 |
| `Morph.hpp` | the Floater–Gotsman morph | 770 | 800 |
| `Sweep.hpp` | the harmonic sweep | 616 | 420 |
| `Pass.hpp` | the weave; pass surgery; welding a sweep frame to the still strands | 405 | 500, with the next two |
| `Diagram.hpp` | `ToPlanarDiagram`: the diagram a drawing draws | 60 | |
| `Build.hpp` | the build loop; r1, pass, middle pass, redraw; turns; spin-offs; the camera | 1,113 | 450 + 120 + 450 |
| `Route.hpp` | routes through the faces | 582 | (was OrthoDecorate, existing) |
| `Redraw.hpp` | the view of a space curve as a drawing | 98 | |
| `Turn.hpp` | the turn of the sphere | 211 | 180 |
| `Tray.hpp` | spin-offs and the tray | 124 | 200 |
| `Scene.hpp` | scenes, roles, the style sheet | 282 | part of 450 |
| `Frame.hpp` | `FrameAt` | 558 | 280 |
| `Json.hpp` | `ToJSON` | 197 | 100 |
| `Raster.hpp` | the rasterizer | 179 | part of 300 |
| `tools/knoodlemovie.cpp` | the tool | 108 | part of 300 |
| `players/knoodle-player.html` | the web player | 427 | 670 |
| **Total** | | **6,601** | **5,340** |

The plan's 6,040 included 700 lines of tests; without them it was 5,340, so this is about
1,260 over (JHC, 2026-11-10: the extra length is acceptable). `Sweep.hpp` and `Frame.hpp` ran over, and `Route.hpp` (582) was not in the plan:
it replaces the use of `OrthoDecorate`, which is existing code. The tests are 1,433 lines
(JHC, 2026-11-10: fast tests are wanted and do not count).

Tests, all C++, all in the light tier:

| Test | What it checks |
|---|---|
| `animate_morph_check` | the kits; the morph's contract on every state of the committed traces (and, by hand, about 21,800 morphs: none failed) |
| `animate_sweep_check` | the sweep's contract and its timing by area, on pockets, lobes and scaled copies (490 checks) |
| `animate_scene_check` | `FrameAt`, gaps, the camera, space scenes, the turn, the tray, `ToJSON`, the rasterizer's pixels (102 checks) |
| `animate_build_check` | every committed trace builds; a layout read back as a diagram is the diagram, label for label; nothing jumps from one scene to the next except at a cut; every cut has a reason on a short list (5,222 checks) |

On the 13 committed traces: 15 curl removals, 20 passes, 7 middle passes with a witness and
2 redraws animate; 2 turns of the sphere are inserted (5 before routes went through the
faces); 6 components go to the tray. Two
cuts remain, both with a reason: a middle pass that carries no witness, and the redraw
whose turned curve is a split link (`redraw_split_refused.trace`, which `knoodleprove`
refuses too).

What happened to the risks:

1. **Splicing the after-drawing directly: done, and it holds.** The drawing is held as a
   weave (each component a closed polyline with its crossings named at the vertices), a
   pass is three edits to it, and the drawing left is read back, turned into a diagram by
   `ToPlanarDiagram`, and carried to the next record's labels by the colour-kept
   isomorphism. Every pass and middle pass in the traces passes that check. No picture is
   printed and parsed anywhere.
2. **One morph front end: holds.** Layouts, drawings left by moves, views of space curves
   and drawings left by turns all go through the same `Morph`.
3. **No fallback morph: holds.** No reshape failed on the traces once the bugs below were
   fixed.
4. **The drawing a turn leaves: the reshape copes,** on every turn the traces have needed (5 with
   lattice routes, 2 now). One
   real failure on the way: the turn's edge subdivision could leave a piece 1e-14 long,
   which ear clipping rightly refused.
5. **Routing after a turn: holds** on the traces. The face put outside is the roomiest
   one inside the strand's loop that the corridor does not pass through.
6. **The redraw view choice** came out small (98 lines plus about 150 in `Build.hpp`): a
   ladder of tilts, each checked by the isomorphism, with no exact integer projection.
   That is enough for a picture; `knoodleprove` remains the check.

What the design had wrong, or did not say:

- **A redraw has no separate lift.** The reshape into the tilted view is the lift. The
  space scene is depth colouring in, the turn, depth colouring out.
- **The exterior named by a curl's own arc flips when the curl goes.** The face outside a
  loop is on the other side of the strand the loop joins. The exterior is now read off
  the drawing after every surgery (the one face that runs clockwise).
- **A route may cross the strand's own old position** (FHW's first pass). It makes no
  crossing there, and the sweep's region is then a chain of lobes.
- **The first layout takes the trace's `#view exterior=`.** After that the drawing's own
  exterior is carried, and a turn is inserted when a move needs another.
- **A fresh layout is turned by the nearest quarter turn** before a reshape, so that the
  picture does not spin.

**Every pass is swept** (JHC, 2026-11-10: "the first harmonic sweep didn't seem to run"). A
sweep cannot be made when the region the strand sweeps holds the strand's own other end (the
route crosses W, and one region between W and the route lies inside another). The builder
used to take the route of least area and, when that could not be swept, jump. Now it takes a
route that can be swept before one that cannot; and when none can (the route's last leg has
one way to go, through a face that wraps round the strand), it turns the sphere to put a
face of that side outside, lays the drawing out again, and sweeps the other side: the same
turn a middle pass gets when its witness side holds infinity. `animate_build_check` now
fails on any pass that jumps. On the committed traces: 27 passes, all swept, 3 turns.

**The junction points slide** (JHC, 2026-11-10). The two points where a pass route leaves the
strand used to sit half way along the strand's first and last arcs, whatever the route did,
which could leave a stub of old strand with the route doubling back beside it. Now each
slides along its arc to where the route is shortest: the tail to suit the route's first leg,
then the head to suit its last, tried a step apart and kept half a step from the arc's ends
(a quarter of the arc if that is less). This made loops as small as half a step, which showed
that `Match` kept two corners 1e-9 to 1e-6 apart in parameter as two points on top of one
another; its merge tolerance is now 1e-6.

**The drawing is balanced on the sphere** (JHC, 2026-11-11). The Moebius maps of the sphere to
itself that are not rotations are the shifts, one for each point w inside the ball
(`Shift`, in Frame.hpp: x goes to ((1 - |w|^2) x + 2 (1 + w.x) w) / (1 + 2 w.x + |w|^2); an
inversion in a sphere square to the unit sphere, then a reflection; the map of the conformal
barycentre papers). JHC's proposal: after the drawing rolls up, apply the shift that evens
its faces' areas. A turn is now: point the face out, roll up, balance (the shift comes in
along the ray from the centre to w), turn the new face's pole to the top, settle into the
fresh layout on the sphere, unroll. `Stage` carries the shift (`mob`, and how much of it).

What is made least is the spread (standard deviation) of the logarithms of the face areas,
by a search from the identity along the six axis directions, halving the stride when none
gains. JHC named largest less smallest area; tried first, it only shrinks the largest face
(among many faces the smallest is next to nothing whatever is done) and crowds the fine part
of the drawing to one side. Largest over smallest is not smooth, and the search stops where
it starts. On the committed traces the spread goes 1.32 to 1.04 (FHW, 32 faces), 1.39 to 1.10
(16 faces) and 1.10 to 0.16 (4 faces): as JHC expected, it helps small diagrams most.

The balanced sphere, seen from its top, is so much less uneven than the unbalanced one that
the reshape is now worked out from it directly. The radial logarithmic "finish" map, its
undoing, and the stage that evened the drawing out are gone; the paragraph below describes
them as they were for a day. Trap met: face areas are read off fans of triangles and must add
up to the sphere; they do so only to about one part in a million (neighbouring faces cut
their common edge at different points), so a check to 1e-6 refused every step of the search.

**The reshape after a turn happens on the sphere** (JHC, 2026-11-10: after a turn "most of
the content is squashed into a ring round the equator of the sphere"; could the Möbius
transformation be continued until the central face is smaller?). It cannot: with the new
face kept outside, the only Möbius maps left are similarities of the plane, which change no
face's size against another's. The turned drawing is uneven by nature (a face's size goes as
the inverse square of its distance from the pole), and a radial map shrinks one empty face
only for the next to take its place (tried: centred on the roomiest point of the old
outside). What evens it out is the reshape into a fresh layout, which used to follow, flat.
Now `TurnTo` does it on the sphere: the drawing rolls up, the sphere turns, the drawing
settles into the layout while still on the sphere, and the layout unrolls. The reshape is
worked out in the plane from the turned drawing evened out, as before (the morph does not
like the raw one), and shown with the evening undone at its start and gone by its end
(`Stage::unfinish`), one stage a morph frame, the renderer running each into the next
(`Stage::blend`). `TurnTo` leaves a fresh layout, so its callers no longer reshape. If the
layout or the morph fails it falls back to unrolling the turned drawing and reshaping flat.

**A sweep's keyframes are never blended** (`Easing::Step`). JHC saw one frame of a sweep
with the still strands scrambled: each keyframe cuts the still strands where the moving
strand meets them, so two keyframes with the same point counts need not correspond, and the
renderer blended them. A scan of the FHW movie for a frame unlike both its neighbours, or
for judder, now finds nothing.

**The turn of the sphere, made easier to follow** (JHC, 2026-11-10). The face that is to be
the outside is pointed out (`Stage::marks`): filled on the flat drawing before anything
moves and while the drawing rolls up, fading as the sphere turns it to the top; as the
sphere unrolls the outside is filled instead, then let go; its outline is drawn throughout.
The ball is drawn, faintly. And the turned drawing, which is very uneven, is evened out while
still on the sphere and only then unrolled; before, the two ran together and the
half-unrolled drawing left the picture. The pole was already the roomiest point found in an
eligible face (`FaceInside`), so that was left alone. The web player draws the same; it was
smoke-tested with node on turn frames (and compared with the C++ pixels in a browser the next day; see below).

**Routes go through the faces, not along the lattice** (JHC, 2026-11-10, after seeing the
first movies: the morph works on polylines, so a route need not be grid lines). `Route.hpp`
takes each face of the drawing in hand as a polygon. A leg of the route enters at a point
of the polygon's boundary and leaves at another. Which way it goes between them is read off
a triangulation of the polygon: the chain of triangles from the way in to the way out, through
the midpoints of the diagonals it crosses. How it goes is then a band (`Tighten`): each end
gets a stem straight in from its corner; between the stems the leg is pushed off the
polygon's sides, to 0.45 of a step where there is room, and pulled taut, never nearer a side
than that, or than it already is where the way is narrower. The band rounds the corners it
wraps and bridges the pockets of the outside. Where a leg crosses the next arc slides too:
places a step apart are tried, and the one that makes this leg plus the straight line on to
the arc after shortest is taken.

For a few hours on 2026-11-10 the way was the medial axis, from Boost.Polygon's Voronoi
diagram of segments (JHC's choice over a hand-rolled straight skeleton, which his Mathematica
prototype used). JHC then asked whether the tightening made it unnecessary. It did: any path
inside the polygon gives the same band, the pictures are the same, the build test runs in 9 s
instead of 16, and `src/Animate/` is again free of Boost. A leg, once laid, cuts its polygon in two, so a
later leg cannot cross it. The outside is a frame round the drawing joined to the
drawing's outline by a cut. The frame is the outline's convex hull pushed out by one step
(the middle length of the drawing's pieces: the grid step, for a layout), so a route round
the outside runs like a band about half a step off the drawing (JHC, 2026-11-10: with a far
rectangular frame the outside routes looked weird); the four ways of placing the cut are the ways round. Where
the move leaves a choice (which way round; which part of an arc already met) the choices
are tried in turn, and each route offered is checked the way every route is: the surgery
must leave the next record's diagram. Of the routes that pass, the builder takes the one
whose strand sweeps the least area, and for a middle pass one that keeps the witness side
bounded.

- All 27 pass and middle-pass moves in the committed traces route this way. `Build.hpp`
  no longer uses `OrthoDecorate`, keeps no layout, and can route in any drawing.
- Choosing the way round saves turns of the sphere: 2 are needed on the traces, not 5.
- A route's corner may now lie on the strand it crosses; the surgery and the weld accept
  that.
- Not done: the route is the skeleton as it comes, with no smoothing, and it can pass
  close to a strand where the triangulation is thin.

Not done:

- `Build.hpp` includes `tools/r1_move.hpp` and `tools/diagram_agreement.hpp`. Moving the
  picture-independent headers to `src/` is sequence step 2, still to do.
- `knoodledraw --animate` does not exist; `knoodlemovie --json` does that job.
- The two point types (`Point` in `PlaneDrawing.hpp`, `P2` in `Scene.hpp`) are not unified.
- The picture is plainer than the prototype's: no halo where a strand passes over the
  disk, no rounded corners in a reshape or a redraw, the disk always drawn below the
  strands of its layer.
- The web player was opened in a browser on 2026-11-11 (JHC: "let's test the web player"),
  served beside the FHW and lasso animations. It loads from the address, plays, pauses,
  steps scene by scene, takes the space bar, and changes theme, with no console error. Its
  pixels were compared with the C++ rasterizer's at 13 times across every kind of scene
  (1600 x 900, averaged over 20-pixel cells): the worst cell differs by 5 of 255. Two faults
  were found and mended, both in loading: a missing file gave a JSON parse error, and a
  change of the `#file` in the address did nothing until a reload. Not tried: opening a file
  with the Open button or by dropping it, other browsers than Chromium, a phone.
- The tray is not drawn during a turn or a redraw (space scenes have no paths).
- Timings, the camera and the look of a turn have not been tuned by anyone watching.
- The corpus beyond the committed traces has not been run through `Build`.
