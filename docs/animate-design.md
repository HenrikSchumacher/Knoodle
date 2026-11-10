# The Knoodle animator: design

Status: design agreed 2026-11-10 (JHC); nothing is implemented yet. Henrik: at about
6,000 lines this needs no design review. The estimates below are from reading the
prototype, not from built code.

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
