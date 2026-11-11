# The Knoodle animator: design

Status: design agreed 2026-11-10 (JHC). Henrik: at about 6,000 lines this needs no
design review. The estimates below are from reading the prototype, not from built code.

Built (2026-11-10): the library in `src/Animate/`, `tools/knoodlemovie` and
`players/knoodle-player.html`, 6,128 lines, with four C++ tests. Every kind of move in
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
colour. It keeps its colour, shrinks to a rounded square, and slides to a tray: a stack
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
| Sparse solve | Tensors' UMFPACK wrapper, already in the tree | about 60 | Use when `KNOODLE_USE_UMFPACK` is set; a 90-line BiCGSTAB otherwise, so builds without SuiteSparse still work |
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
2. Move the picture-independent headers from `tools/` to `src/` with no change in
   behaviour: exterior threading, witness checking, the R1 and redraw descriptors,
   diagram agreement.
3. The library, each step with its contract test: `PlaneDrawing` and the mesh kit; the
   morph; the sweep; R1; pass and middle pass; the turn; redraw; spin-offs; the build
   loop, `ToJSON` and `FrameAt`.
4. The web player and `knoodlemovie`.
5. Elsewhere: the paclet's consumer; the server for the KnotInfo demo.

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
| `Build.hpp` | the build loop; r1, pass, middle pass, redraw; turns; spin-offs; the camera | 1,009 | 450 + 120 + 450 |
| `Route.hpp` | routes through the faces | 437 | (was OrthoDecorate, existing) |
| `Redraw.hpp` | the view of a space curve as a drawing | 98 | |
| `Turn.hpp` | the turn of the sphere | 131 | 180 |
| `Tray.hpp` | spin-offs and the tray | 124 | 200 |
| `Scene.hpp` | scenes, roles, the style sheet | 278 | part of 450 |
| `Frame.hpp` | `FrameAt` | 476 | 280 |
| `Json.hpp` | `ToJSON` | 185 | 100 |
| `Raster.hpp` | the rasterizer | 179 | part of 300 |
| `tools/knoodlemovie.cpp` | the tool | 108 | part of 300 |
| `players/knoodle-player.html` | the web player | 381 | 670 |
| **Total** | | **6,128** | **5,340** |

The plan's 6,040 included 700 lines of tests; without them it was 5,340, so this is about
790 over. `Sweep.hpp` and `Frame.hpp` ran over, and `Route.hpp` (437) was not in the plan:
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

**Routes go through the faces, not along the lattice** (JHC, 2026-11-10, after seeing the
first movies: the morph works on polylines, so a route need not be grid lines). `Route.hpp`
takes each face of the drawing in hand as a polygon. A leg of the route enters at a point
of the polygon's boundary and leaves at another, and in between follows the polygon's
skeleton. The skeleton is the medial axis (JHC, 2026-11-10; his Mathematica prototype used
the straight skeleton, and the two differ only round reflex corners, where the medial axis
has a short parabolic arc): the inside part of the Voronoi diagram of the polygon's edges,
from Boost.Polygon, which is exact on integer input, so the corners are snapped to a grid
of 2^29 first. Each end of a leg goes straight in along the bisector of its corner to the
axis, and the leg takes the shortest way along the axis between them, never through a
corner of the polygon. This is the first use of Boost inside `src/Animate/`; Knoodle
already requires Boost, so it is no new dependency. (The first version followed the
midpoints of the diagonals of a Delaunay triangulation, which could run close to a strand.) A leg, once laid, cuts its polygon in two, so a
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
- The web player's frames agree with `FrameAt` numerically, and its interface code has
  been run against a stub page, but it has not been opened in a browser.
- The tray is not drawn during a turn or a redraw (space scenes have no paths).
- Timings, the camera and the look of a turn have not been tuned by anyone watching.
- The corpus beyond the committed traces has not been run through `Build`.
