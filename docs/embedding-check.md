# `embedding_check` — degeneracy and performance test for the LinkEmbedding family

Status: written 2026-08-13 against `dev_prosector` at `0a821267` ("Documentation.").
Everything below was **verified by running it**, not inferred; each finding names a
minimal reproducer that is committed alongside.

`LinkEmbedding2/3/4` exist to survive projections `LinkEmbedding` cannot: vertices
projecting onto vertices, a vertex projecting into a segment, a segment projecting to a
point, several segments through one projected point, collinear overlaps, zero-length
edges. Nothing tested that. `inflate_check` and friends feed the embedding path random
coordinates, which are generic with probability 1, so those branches were never taken.

`test/embedding_check.cpp` does. It is the only test that deliberately drives the
degenerate branches, and the only one with an oracle sharp enough to say the answer is
*right* rather than merely *plausible*.

---

## 1. How to run it

```
cd test
make embedding_check
./embedding_check                # every tier, every buildable class, both coord types
```

Needs the same heavy configuration as `link_inflate_check`: the vendored libhomfly
objects (for HOMFLY) and UMFPACK + BLAS/LAPACK (for the Alexander fallback). The
Makefile target handles both.

Exit status is 0 when nothing failed. Known defects are reported but do not fail the
run — see [§4](#4-known-defect-markers).

### Options

```
--class=LIST       1,2,3,4                                (default all)
--coords=LIST      f64,f32,i32,i64                        (default f64,f32)
--tier=LIST        census,reader,exact,cross,invariant,rotation (default all)
--fixtures=DIR     fixture directory                      (default ./embeddings)
--only=SUBSTR      only fixtures whose name contains SUBSTR (exact match wins)
--rotations=N      random generic projections per fixture (default 3)
--rotation-steps=N successive re-aimings in the rotation tier (default 24)
--homfly-cap=N     max crossings for the HOMFLY oracle    (default 40)
--isolate          run each unit in a child process, so a crash is *scored*
                   instead of ending the run
--bench            timing mode
--reps=N           repetitions per benchmark point        (default 3)
--list             list fixtures with their degeneracy census, and exit
--verbose          print every check, not just failures
```

### The runs you will actually want

```
./embedding_check --tier=exact --class=4              # am I still exact?
./embedding_check --only=lattice --verbose            # the space-filling cases
./embedding_check --isolate                           # scores crashes instead of dying
./embedding_check --bench --reps=7 --only=lattice     # the timing table
./embedding_check --tier=rotation --verbose           # re-aiming doesn't inflate
./embedding_check --list                              # what each fixture contains
```

`--isolate` re-executes the binary once per (tier, fixture, class, coords) unit. It is
slower, but it is the mode to use while a crash is outstanding: a segfault becomes a
reported result rather than the end of the run.

### Coordinate types

`f64` and `f32` are `double` and `float` coordinates, both against the 64-bit integer
backend. `i32` (the 32-bit backend) and `i64` (integral `Real_`) do not compile today —
see findings [C](#c-integral-real_-does-not-compile) and
[D](#d-the-32-bit-backend-does-not-compile) — and are behind
`-DKNOODLE_TEST_INTEGER_COORDS` / `-DKNOODLE_TEST_INT32_BACKEND` so the test picks them
up the moment they land. The binary prints which combinations it could not build; it
does not skip them silently.

The integral-coordinate *path* is exercised regardless: `ReadVertexCoordinates` detects
all-integral input and sets `scaling_exponent = 0`, so integer-valued doubles take the
exact, unscaled, zero-rounding-error route. Every fixture except the deliberately
rotated ones is integral, and the benchmark confirms `rounding_err = 0` throughout.

---

## 2. What it checks

Six tiers, increasing in strength.

### `census` — the fixtures really are degenerate

Classifies every projected edge pair in exact integer arithmetic (`__int128`), entirely
independently of the code under test, and checks the result against what each fixture
declares in its `.expect` sidecar. This is what makes "we exercised the corner cases"
*checkable* instead of assumed: if a fixture silently stops being degenerate, the
declared counts stop matching and the test says so.

It also pins `spatial == 0` on every fixture. Two edges genuinely meeting in 3-space is
the one degeneracy no perturbation can repair, so a fixture containing one would make a
*correct* error return look like a failure.

One subtlety worth carrying into the library: vertices joined by a zero-length edge are
the same point of 3-space, so the edges flanking one are **adjacent**. The census
collapses them by union-find before the 3D-intersection test. Not doing so is what
produces finding [B](#b-zero-length-edges-are-rejected-as-3d-intersections).

### `reader` — the library's own reader agrees with an independent parse

Cheap, and it pins the file format the rest of the fixtures rely on. Currently disabled
by finding [F](#f-frominstring-has-two-inverted-assertions); it re-enables itself
automatically once that is fixed.

### `exact` — the sharp one

`Prosector2`, `Prosector3` and `Prosector4` all document the *same* symbolic
perturbation: project along `{ε, ε³, 1}` and take `ε → 0⁺`. Because that direction is
written down, the correct answer on a degenerate projection is **exactly computable**.
Put `ε = 1/N` and clear denominators:

```
X = N³·x − N²·z ,   Y = N³·y − z ,   Z = N³·z
```

This is a linear isomorphism of 3-space (determinant `N⁹ > 0`), so it changes neither
the link type nor the edge numbering — and for `N` large enough its *vertical*
projection is generic and realizes the `ε → 0⁺` limit.

So the test requires the degenerate projection to reproduce the sheared, generic one
**crossing for crossing**. Not "the same knot" — the same crossings, in the same order
along each edge, with the same over/under and handedness. That is what catches a
wrong-but-plausible crossing set that still happens to be the right knot type.

`N` is escalated along a ladder (4, 8, 16, 64, 256, 4096) until two consecutive values
agree, so the test *certifies* that `N` was large enough rather than assuming it. The
value needed grows with the lattice, which is what one would expect — a denser projection
needs a finer perturbation to separate everything:

| fixtures | stabilizes at |
| --- | --- |
| the seven `deg_*` that run (all but `deg_zero_length`), and `lattice_04` | `N = 8` |
| `lattice_06`, `lattice_08` | `N = 16` |
| `lattice_10`, `lattice_12`, `lattice_16` | `N = 64` |

The ladder also stops before leaving the exactly-representable range of `double`, so a
silently rounded shear can never be compared against.

The comparison object is built from the embedding's own `EdgePointers()` /
`EdgeIntersections()` / `EdgeStates()` arrays, before any `PlanarDiagram` exists, so a
mismatch localizes to the intersection computation rather than to PD assembly.

**This tier passes on every fixture, for all of `LinkEmbedding2/3/4`, including a 16³
lattice knot with 6736 crossings.** That is a strong statement about the perturbation
machinery and it is worth keeping true.

### `rotation` — repeated re-aiming does not make the knot grow

Rotating a *fixed* curve changes which projection you look at, so the crossing count
legitimately varies between rotations. What must not happen is a **trend**: rotations are
drawn uniformly from SO(3), so the projections are exchangeable and the count has no
reason to drift. If it climbs, the embedding is degrading rather than being re-aimed.

Two paths run per fixture and class, and the difference between them is the point:

- **fresh** — each step rotates the *original* coordinates by an independent random
  matrix. Nothing accumulates; this is the control, measuring the honest spread of
  crossing counts over projections.
- **composed** — one embedding object is held and `Transform()` is called on it
  repeatedly, each rotation landing on the output of the last. This is where error, and
  any translation folded into something advertised as a rotation, accumulates. It is the
  path Rattle used to take and that `00a8407` had to route around.

Four assertions per path:

1. **Re-aiming actually re-aims.** Two dozen uniformly random rotations cannot all yield
   a bit-identical crossing set. If they do, the rotation is not reaching the
   computation — which is exactly what finding [G](#g-linkembeddingtransform-does-not-invalidate-its-caches) looks like from outside.
2. **The knot type never changes**, by the same oracles the `invariant` tier uses.
3. **The radius of gyration is conserved.** `R_g` is exactly invariant under any rigid
   motion, *including translation*, so the internal Sterbenz shift does not register as
   spurious motion the way a bounding box or a centroid norm would. The tolerance scales
   with the working precision (`256·√k·ε`), because composing `k` rotations random-walks
   roundoff at `√k·ε` — ~1e-15 at `f64` but ~6e-7 at `f32`, so one fixed threshold would
   be either blind or noisy. It still sits orders of magnitude below real drift: the
   inflation `embedding_inflation_check` documents moves coordinates by a factor of ~4000.
4. **The crossing count does not trend upward**, comparing the first quarter's mean
   against the last quarter's.

This complements `test/embedding_inflation_check.cpp` rather than duplicating it. That
test pins the *geometry* of re-aiming — that `‖c‖` and the extent are conserved — for
`LinkEmbedding` via Reapr. This tier pins the *diagram*, and does it for all four classes.

Measured on `lattice_08`, 24 composed re-aimings of `LinkEmbedding4`: crossings range
402–535, quarter means 474 → 496, `R_g` constant. No trend, which is the claim.

### `cross` — the three backends agree with each other

Sharing one perturbation contract, `LinkEmbedding2/3/4` must produce *identical*
crossing sets on every input. This is the cheapest strong regression guard between
versions, and it is what will fire first if a new Prosector diverges. `LinkEmbedding`
joins the comparison on generic (rotated) input only, where there are no degeneracies to
resolve and every backend should agree.

### `invariant` — the knot type survives

For each fixture, the degenerate projection is compared against random rotations of the
same curve (degeneracy-free with probability 1). HOMFLY where the simplified diagram is
under `--homfly-cap`, the single-variable Alexander `|det|` fingerprint on the unit
circle above it, and the link-component count always.

`LinkEmbedding` is deliberately *not* held to the degenerate half of this. It computes
in floating point with no symbolic perturbation, so refusing a degenerate projection
(error code 5, 9) is correct behaviour, not a defect — it is the reason `2/3/4` exist.
It is anchored on a generic rotation instead and still gets a real invariance test.

---

## 3. The fixtures

`test/embeddings/*.crd` — three columns per vertex, blank line between components, each
component implicitly closed. The extension is `.crd` and not `.tsv` because
`.gitattributes` routes all `*.tsv` through git-lfs.

Eight hand-built curves, one degeneracy each, small enough to read:

| fixture | what it contains |
| --- | --- |
| `deg_corner_corner` | two vertices project onto each other; 4 edges through one point |
| `deg_vertex_on_edge` | a vertex projects into a segment's interior |
| `deg_vertical_edge` | an edge projects to a point, lying on another edge |
| `deg_stacked_verticals` | 6 edges through one projected point, 2 of them vertical |
| `deg_collinear_overlap` | two non-adjacent edges sharing a projected segment |
| `deg_zero_length` | a zero-length edge, on top of a corner-corner |
| `deg_triple_point` | three non-adjacent segments concurrent at one point |
| `deg_hopf_flat` | 2-component link; one component projects to a segment |

`deg_hopf_flat` is the one whose type is fixed by construction rather than by
computation: component 1's vertical edge at (2,2) pierces the disk bounded by the square
component 0 exactly once, so it is a Hopf link. The others' types are established by the
test's own generic-projection comparison, which is self-validating and does not need
them known in advance.

Plus six closed Hamiltonian cycles on the `k³` cubic lattice, `k = 4 … 16`, from
`ndmansfield -record-loops` (provenance and exact seeds in `test/embeddings/README`).
These are space-filling knots with unit edges that present every degeneracy class at
once and in bulk — `lattice_16` has a projected point with **30** edges through it, and
its projection yields 6736 crossings. Only even box sizes appear: a `k³` grid graph is
bipartite with unequal colour classes when `k³` is odd, so no Hamiltonian cycle exists.

`./embedding_check --list` prints each fixture's census.

---

## 4. Known-defect markers

`.expect` files carry `xfail_<tier>` (known wrong answer) and `xcrash_<tier>` (known
crash), each optionally scoped to a class list. A `DEFAULT.expect` beside the fixtures
carries markers that apply to *every* fixture, for defects that belong to a class rather
than to a curve; a tier may hold several markers with disjoint class lists, and the first
one matching the class under test wins, fixture markers ahead of inherited ones.

```
xfail_exact = 2,3,4 | zero-length edge rejected as a 3D intersection
```

Both are checked in **both** directions. A marked check that starts passing is reported
as an XPASS *failure*, so a stale marker gets removed rather than quietly masking a
later regression. `xcrash` combinations are skipped in the normal run (to keep it alive)
and scored properly under `--isolate`.

When you fix one of the findings below, the corresponding marker will announce itself.

---

## 5. Findings

All reproduced on `dev_prosector` at `0a821267`. Severity is my read; the evidence is
the part that matters.

### A. `PlanarDiagram::FromLinkEmbedding(LinkEmbedding2&)` returns an invalid diagram for every successful projection

**Severity: high — this is the main construction path.**

`RequireIntersections` no longer reports success the same way across the family:

| class | signature | success is |
| --- | --- | --- |
| `LinkEmbedding` | `int RequireIntersections()` (`src/LinkEmbedding/FindIntersections.hpp:4`) | `0` |
| `LinkEmbedding2/3/4` | `bool RequireIntersections(bool force_recomputeQ = false)` (`src/LinkEmbedding2/Intersections.hpp:10`) | `true` |

The two conventions disagree about the meaning of `1`. `src/PlanarDiagram/FromEmbeddings.hpp:122`
still assumes the `int` one:

```cpp
int err = L.template RequireIntersections<true>();
if( err != 0 )
{
    eprint(... "RequireIntersections reported error code " ...);
    return { PD_T::InvalidDiagram(), Tensor1<Int,Int>() };
}
```

so `true` → `1` → the failure branch, on **every** call. Reproduced on `lattice_04`
(a perfectly ordinary curve):

```
RequireIntersections returned 1 (true = success), intersections = 19
ERROR: PlanarDiagram<I64>::FromLinkEmbedding(LinkEmbedding2<R64,I64>):
       RequireIntersections reported error code 1. Returning invalid diagram.
FromLinkEmbedding -> pd.ValidQ() = 0, crossings = 0
```

19 crossings computed, then discarded.

Beyond the immediate fix, the split is worth resolving: two sibling classes with the
same method name and opposite meanings for the same return value is a trap for every
caller. This is exactly how it caught me — my first run against your tree reported 268
failures, all "error code 1", because I had assumed the `int` convention.

`embedding_check` now inspects the return *type* rather than assuming, and reaches
`FromLinkEmbedding_Raw` directly, so it is unaffected.

### B. Zero-length edges are rejected as 3D intersections

**Severity: medium — contradicts the class documentation, and is a regression relative
to `LinkEmbedding`.**

`LinkEmbedding2/3/4`'s class docs list *"line segments that have length 0"* among the
degeneracies handled. They are not: a zero-length edge is reported as the unfixable
"edges intersect in 3D" case.

This is inherent, not fixture-specific. A zero-length edge between vertices `V_k = V_k+1`
means edge `k−1` ends at `V_k` and edge `k+1` starts at `V_k+1` — the same point of
3-space — while their indices are not consecutive, so the adjacency test misses it. A
bare planar square with one duplicated vertex fails the same way:

```
0 0 0 / 4 0 0 / 4 0 0 / 4 4 0 / 0 4 0
  -> ComputeEdgeEdgeIntersection: Edges 0 and 2 intersect in 3D.
```

`LinkEmbedding` handles it without complaint, so this is a regression relative to it.

The fix is the union-find the census already does: collapse vertices joined by a
zero-length edge into one before the 3D-intersection test.

Reproducer: `test/embeddings/deg_zero_length.crd`. Markers: `xfail_{exact,cross,invariant}`
scoped to classes 2,3,4.

### C. Integral `Real_` does not compile

**Severity: low — documented but unimplemented.**

The class documentation says `Real_` may be a signed integral type, and that
`IReal_` must then equal `Real_`. `src/LinkEmbedding2/EdgeCoordinates.hpp:71` says
otherwise:

```cpp
// Case 2. `Real` is am integral type
//   Then we can simply copy.

// Let's handle only case 1.2.a) for now.
static_assert( FloatQ<Real>, "" );
```

so `LinkEmbedding2/3/4<Int64,Int64,Int64>` fails to instantiate. Not urgent — integral
input already gets the exact path through the `input_integralQ` branch — but the docs
currently promise something that does not build. Behind `-DKNOODLE_TEST_INTEGER_COORDS`.

### D. The 32-bit backend does not compile

**Severity: low, but the docs recommend this pairing.**

The docs suggest `Real_ = float` with `IReal_ = int32_t`. Neither works:

- `Prosector2`: `call to 'Sign' is ambiguous`, `src/Prosector2/Helpers.hpp:16` (also 19, 21).
- `Prosector3`, `Prosector4`: `excess elements in array initializer`, `src/WideInt.hpp:112`.

Behind `-DKNOODLE_TEST_INT32_BACKEND`.

### E. `LinkEmbedding::RequireIntersections` segfaults on a triple point

**Severity: high for `LinkEmbedding`, though `2/3/4` supersede it. Possibly the segfault
noted in `6c63b82a`.**

Three segments whose projections pass through one common point crash the intersection
computation outright. Six-vertex reproducer, committed as
`test/embeddings/deg_triple_point.crd`:

```
-4  0  0        edges 0, 2 and 4 lie on y=0, x=0 and y=x
 4  0  0        respectively, so all three project through
 0 -4  1        the origin, at heights 0, 1 and 2
 0  4  1
-3 -3  2
 3  3  2
```

Bisected: moving any *one* of the three edges off the origin makes it complete normally,
and widening the z-separation does not help, so the trigger is the concurrency itself and
not a near-tie in depth. `LinkEmbedding2/3/4` resolve it correctly — 6 crossings,
matching the explicit shear.

`deg_hopf_flat` crashes the same way for the same reason.

Markers: `xcrash_invariant` scoped to class 1. Run with `--isolate` to see it scored.

### G. `LinkEmbedding::Transform` does not invalidate its caches

**Severity: high — silent wrong answers. Found by the `rotation` tier.**

`src/LinkEmbedding/VertexCoordinates.hpp:174` rotates `edge_coords` but resets neither
`intersections_computedQ` nor `bounding_boxes_computedQ`.
`LinkEmbedding2::Transform` resets all three explicitly
(`src/LinkEmbedding2/VertexCoordinates.hpp:117-119`).

So after a `Transform`:

| call | result |
| --- | --- |
| `RequireIntersections()` | the **stale pre-rotation** answer |
| `FindIntersections()` | **0** — the AABB boxes are stale too, so the tree finds no candidate pairs |
| `ComputeBoundingBoxes(); FindIntersections()` | correct |

Measured on a 40-gon rotated 90° about the x-axis: 3 crossings before, 3 after
`Transform` + `RequireIntersections`, 0 after `FindIntersections`, and 10 once the boxes
are refreshed. `LinkEmbedding2` goes 3 → 10 unaided.

This is silent — you get a diagram back, it is just the previous one. Nothing in the
library hits it today because `00a8407` routed Rattle around `Transform`, but the method
is public and documented as re-aiming the projection.

Fix: mirror `LinkEmbedding2` and clear both flags.

Marker: `xfail_rotation` scoped to class 1, in `test/embeddings/DEFAULT.expect` — the
defect belongs to the class, not to any one curve.

### F. `FromInString` has two inverted assertions

**Severity: high in any build without `-DNDEBUG` — it aborts on every input.**

`src/LinkEmbedding2/FromFile.hpp`:

```cpp
assert( component_ptr_agg.size() != color_agg.size() );   // line 139
...
assert( !coords_may_followQ );                            // line 157
```

Both fire on the first vertex of any well-formed file, with or without `#color` lines.
By the time the first assertion is reached, one colour has always just been recorded for
the component being opened — pushed either by the `#color` branch or by the
`comp_needs_colorQ` branch immediately above — so the two sizes are *equal* and `!=`
fails. The second one was confirmed by correcting the first locally and re-running:

```
Assertion failed: (component_ptr_agg.size() != color_agg.size()), ... line 139
  -> corrected to == ->
Assertion failed: (!coords_may_followQ), ... line 157
```

Compiled with `-DNDEBUG` the same reader parses correctly (`ok: 6 edges, 1 components`),
so this is the assertions and not the parse. `test/Makefile` builds at `-O3` without
`-DNDEBUG`, which is how it surfaced.

The `reader` tier probes for this in a child process and resumes automatically once it
is fixed — there is no marker to remember to remove.

---

## 6. Baseline timings

`./embedding_check --bench --reps=7 --only=lattice --coords=f64`, best of 7, milliseconds,
covering read → intersect → build PD. `degen` is the raw lattice projection, `generic` a
random rotation of the same curve — the only comparison `LinkEmbedding` can join.

| fixture | projection | LE1 | LE2 | LE3 | LE4 |
| --- | --- | ---: | ---: | ---: | ---: |
| `lattice_04` (64 edges) | degen | *refuses* | 0.017 | 0.010 | 0.009 |
| | generic | 0.006 | 0.011 | 0.007 | 0.006 |
| `lattice_08` (512) | degen | *refuses* | 0.347 | 0.187 | 0.142 |
| | generic | 0.092 | 0.185 | 0.091 | 0.081 |
| `lattice_12` (1728) | degen | *refuses* | 1.842 | 1.100 | 1.024 |
| | generic | 0.474 | 0.884 | 0.676 | 0.566 |
| `lattice_16` (4096) | degen | *refuses* | 6.349 | 3.654 | 3.589 |
| | generic | 1.699 | 3.386 | 2.252 | 1.898 |

Two things stand out. `LinkEmbedding4` has essentially caught up with the floating-point
class on generic input (1.90 ms vs 1.70 ms at 16³) while also handling the degenerate
projection that `LinkEmbedding` cannot run at all. And the progression 2 → 3 → 4 is
monotone at every size, with the largest jump between 2 and 3.

`rounding_err = 0` everywhere, as expected for integral input on the
`scaling_exponent = 0` path.

---

## 7. Current status

```
./embedding_check              650 passed, 0 failed, 34 skipped, 53 known-failing
./embedding_check --isolate   1606 passed, 0 failed, 78 skipped, 186 known-failing
```

Whole run: about 2.2 s.

The skips are the zero-length fixture (finding B makes the backends fail identically, so
there is nothing to compare) and the known crashes (finding E). The known-failures are
findings B, E, F and G.
