# `embedding_check` — degeneracy and performance test for the LinkEmbedding family

Status: written 2026-08-13 against `dev_prosector` at `0a821267`; **re-verified
2026-08-14 against `fb4c8f0e`**, after Henrik fixed most of what it found. Everything
below was **verified by running it**, not inferred; each finding names a minimal
reproducer that is committed alongside, and each now carries its outcome.

**Five of the seven findings are fixed** (A, C, E, F, G). One is half fixed (D) and one
is open (B). The scoreboard is [§5](#5-findings); the one-line version is that the test
did its job — every fix announced itself, either as an XPASS on a stale marker or as a
tier that started running.

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
backend.

`i64` (integral `Real_`) **is in the default set** as of 2026-08-14: finding
[C](#c-integral-real_-does-not-compile) was fixed in `4d2d0624`, so the gate came off.
It adds 81 passing checks, including the whole of the `exact` tier, which is the
sharpest oracle here.

**It cannot be randomly rotated, and does not need to be.** A rotation of a lattice
point is not a lattice point, so `static_cast<Real_T>` truncates every coordinate back
onto the grid — and the *rotation matrix* truncates the same way, its entries living in
[-1,1], so at `int64_t` it collapses to 0 and ±1 and `Transform` receives a singular
matrix. Naively rotating integral coordinates produces dozens of spurious "edges
intersect in 3D" and then an assertion failure inside `LinesColinearTest`.

The replacement is exact: **an integer matrix of positive determinant**
(`kt::UnimodularIntegerMatrix`, a product of elementary integer shears). It maps integer
coordinates to integer coordinates with no rounding at all, and because `GL+(3,ℝ)` is
connected it is isotopic to the identity, so the image is the same knot. What it is not
is a *lattice* configuration — the edges stop being unit vectors — and that is exactly
what makes the projection generic. Measured on `lattice_04`: 96 pairs of vertices share
a projected column before, 0 after, with `max|coord|` growing only from 4 to 36 against a
60-bit budget.

Two consequences, both in the code:

- **Do not compose them.** Coordinates grow geometrically under repeated application and
  the exact path holds only while `scaling_exponent >= 0`, so every image is taken from
  the *original* curve. That rules out exactly one thing — the rotation tier's `composed`
  path, whose whole purpose is to accumulate — and it is skipped for integral coordinates
  with that reason. `fresh` runs normally.
- **The radius-of-gyration assertion does not apply.** `R_g` is conserved by *rigid*
  motions; a unimodular integer matrix has determinant 1 without being an isometry, so it
  moves distances freely. That assertion is guarded to rotations. The other three —
  re-aiming re-aims, knot type is constant, no upward trend — all still hold, and they
  are the ones that carry the claim.

`i32` (the 32-bit backend) still does not build — finding
[D](#d-the-32-bit-backend-does-not-compile) is half fixed — and stays behind
`-DKNOODLE_TEST_INT32_BACKEND`. The binary prints which combinations it could not build;
it does not skip them silently.

The integral-coordinate *path* is exercised regardless: `ReadVertexCoordinates` detects
all-integral input and sets `scaling_exponent = 0`, so integer-valued doubles take the
exact, unscaled, zero-rounding-error route. Every fixture except the deliberately
rotated ones is integral, and the benchmark confirms `rounding_err = 0` throughout.

---

## 2. What it checks

Seven tiers, increasing in strength.

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

Cheap, and it pins the file format the rest of the fixtures rely on. It was disabled by
finding [F](#f-frominstring-has-two-inverted-assertions), and it **re-enabled itself**
when `8db4cdc4` fixed the assertions — the probe runs the reader in a child process
rather than declaring a marker, so there was nothing to remember to remove. Worth
copying the next time a whole tier is blocked on someone else's bug.

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
   computation — which is exactly what finding [G](#g-linkembeddingtransform-does-not-invalidate-its-caches) looked like from outside, and how this tier caught it.
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

#### A refusal is not a wrong answer

`LinkEmbedding` returns **error code 8** when two intersection times along an edge fall
within `intersection_time_tolerance` of each other: it cannot order them, so it declines
instead of guessing. For some rotations at some precisions that ordering genuinely is
not well defined — on `f32`, `lattice_10` and `lattice_16` both hit it at step 12 of the
composed sequence — and refusing is correct behaviour, not a defect. (Confirmed with
Henrik, 2026-08-14.)

So a decline is accepted: the sequence stops there, the steps already collected are
still checked for growth, and a `NOTE` line reports it. A decline is not a failure, but
one that starts happening at step 3 instead of step 12 is something a human should see.

The knot-type comparison (assertion 2) is **skipped** on a shortened run. The PD is
built only on the last step, so a declined sequence has no final classification;
comparing against a default-constructed one reports `link component count 1 vs 0`, which
is pure artefact and reads exactly like a real regression.

Both fixtures fail at *step 12 precisely*, and that is the tell that this is a
degenerate rotation rather than accumulated drift: the sequence is seeded
deterministically, so every run composes the same rotations, and drift would give out at
different steps for different curves. Rotation #12 simply lands these highly symmetric
axis-aligned curves in a near-degenerate projection that `f32` cannot resolve and `f64`
can.

The check keys on the code alone and needs no class test: 8 belongs to the
floating-point path, and the `LinkEmbedding_Int` family (2/3/4) returns 0 on success or
1/2/3 for "no coordinates", "unknown", "self-intersects in 3-space". A refusal from
those therefore stays a failure, which is right — they claim to resolve degeneracies.

None of this was reachable until 2026-08-14: `xfail_rotation = 1` masked the whole class
while finding G's stale caches made every rotation return the same answer. Removing that
marker is what exposed it.

### `symmetry` — the 24 rotations of the cube give the same knot

Signed permutation matrices of determinant +1 map the cubic lattice onto itself, so a
lattice configuration stays one **and its projection stays maximally degenerate** — 96
shared projected columns on `lattice_04` before and after. That is the point: where a
random rotation hands the degenerate machinery an easy case every time, this hands it 24
independent hard cases per fixture, all of which must agree on the link type. The
crossing *count* legitimately differs between images, since they are different
projections, so only the type is compared.

Determinant +1 is what makes the claim true — orientation-preserving, hence isotopic to
the identity, hence the same knot. The entries are 0 and ±1, so it is exact for every
coordinate type, integral and floating point alike.

A class with no symbolic perturbation may legitimately refuse a degenerate image; those
are skipped rather than scored, and a class that refuses all 24 is reported as skipped.

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
in floating point with no symbolic perturbation, so refusing a degenerate projection is
correct behaviour, not a defect — it is the reason `2/3/4` exist. It is anchored on a
generic rotation instead and still gets a real invariance test.

The excusal is **wholesale**, via `ResolvesDegeneraciesQ( cls ) { return cls != 1; }`,
and not an allow-list of codes: no comparison against a specific error value exists
anywhere in the harness except `err == 0` (success) and an `err == 6` message
decoration. Worth stating because the codes move — the lattices refuse with **9**
("degenerate edges"), `deg_triple_point` now refuses with **8** where it used to
segfault, and any new code is accepted automatically.

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
| `arc_trefoil` | the trefoil in **arc presentation**: every crossing on the binding axis |

`deg_hopf_flat` is the one whose type is fixed by construction rather than by
computation: component 1's vertical edge at (2,2) pierces the disk bounded by the square
component 0 exactly once, so it is a Hopf link. The others' types are established by the
test's own generic-projection comparison, which is self-validating and does not need
them known in advance.

`arc_trefoil` is the one worth singling out, because it is not hand-crafted spite —
it is the shape of the **petaluma random-knot model**, so it is what a generator in the
wild hands to Knoodle. The z-axis is the binding and every arc lies in one half-plane
through it, so *all* the binding points project to the origin, every vertical segment
projects to a point, and each arc's outbound and return segments project onto exactly
the same segment. The naive projection has **no transversal crossings at all** — it is
five doubled rays through one point, and every crossing in the answer is manufactured by
the symbolic perturbation.

It works, and that is the demo: `LinkEmbedding2/3/4` recover 8 crossings, agree
crossing-for-crossing, and the exact-shear oracle confirms them independently (stable at
`N = 8`, every class, every coordinate type). They simplify to the left-handed trefoil.
`LinkEmbedding` refuses with error code 9, which is correct and needs no marker.
`make_arc_presentation.py` emits more from any grid diagram.

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

**It worked.** On 2026-08-14 the `Transform` fix produced 24 XPASS from the single
`xfail_rotation = 1` line in `DEFAULT.expect`, and the triple-point fix produced 4 more
from the two `xcrash_invariant` markers. Henrik removed all three in `b2a799a2`. There
is one caution the episode is worth remembering for: **a marker can hide more than one
thing.** That one rotation marker was also masking the legitimate code-8 declines
described in [§2](#a-refusal-is-not-a-wrong-answer), which only surfaced once it came
out — so budget for a second look after removing a broad marker, not just a green run.

**Markers live in the fixture data, not in the code** (`test/embeddings/*.expect` and
`DEFAULT.expect`), they are read at runtime, and there is no `xpass` marker to change
them to: you delete the line.

As of `fb4c8f0e` the only markers left are the four zero-length ones in
`deg_zero_length.expect`; `DEFAULT.expect` carries none.

Markers scope by **class list only** — there is no coordinate-type dimension — so a
defect that affects `f32` but not `f64` cannot be expressed as one without also masking
the working case. That is why the code-8 declines are handled in the tier rather than
marked.

---

## 5. Findings

All reproduced on `dev_prosector` at `0a821267`, and all **re-verified against
`fb4c8f0e`** on 2026-08-14 with the standalone reproducers rather than through the
harness. Severity is my read; the evidence is the part that matters.

| | finding | status at `fb4c8f0e` |
| --- | --- | --- |
| A | `FromLinkEmbedding(LinkEmbedding2&)` treats success as error | **fixed** — `1d8761c7` + `34cd0fc3` |
| B | zero-length edges rejected as 3D intersections | **open** |
| C | integral `Real_` does not compile | **fixed** — `4d2d0624` |
| D | the 32-bit backend does not compile | **half** — `Sign` resolved, `WideInt` remains |
| E | `RequireIntersections` segfaults on a triple point | **fixed** — `d26c301f` |
| F | `FromInString` has two inverted assertions | **fixed** — `8db4cdc4` |
| G | `Transform` does not invalidate its caches | **fixed** — `8db4cdc4` |

### A. `PlanarDiagram::FromLinkEmbedding(LinkEmbedding2&)` returns an invalid diagram for every successful projection

**FIXED** — `1d8761c7` unified the conventions on `int`, which closes this at the root
rather than at the caller; `34cd0fc3` finished the conversion (see the tail of this
entry). Verified: `FromLinkEmbedding` now returns a valid 7-crossing diagram on the
reproducer below.

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

**How it was actually fixed, and the tail that came with it.** `1d8761c7` changed
`LinkEmbedding_Int::RequireIntersections` from `bool` to `int` (0 = success), which is
the better fix — it removes the split instead of patching one caller. Two things did not
make it across, and `34cd0fc3` finished them:

- the function still ended in `return true`, which in an `int` function is `1` — so
  every *successful* projection still reported an error, the identical symptom relocated
  from the caller into the callee. Isolated by flipping that one line on an otherwise
  identical build.
- `FromKnotEmbedding` was rewritten to test the call directly but its `eprint` still
  named `err`. That is a non-dependent name, so it failed at template *definition* time
  and **every translation unit including `Knoodle.hpp` stopped compiling** — a bare
  `#include` reproduced it.

Two lessons worth keeping. A convention change is a whole-family edit, and the compiler
only catches the half of it that is type-checked — `return true` from an `int` function
is silently well-formed. And `embedding_check` could not have caught either one: it
routes around `FromLinkEmbedding` entirely, which is why this finding needed a
standalone reproducer to confirm and would not have announced its own fix.

**`LinkEmbedding3`/`LinkEmbedding4` were never affected — because they are not wired up
yet.** There is no `FromLinkEmbedding` overload taking them;
`PD_T::FromLinkEmbedding(le3)` fails to compile with *no matching function*. They share
the convention (all three include the same `src/LinkEmbedding_Int/`); they simply have no
public path into `PlanarDiagram` yet. That is **expected** — both classes are still
EXPERIMENTAL and the overloads are planned once they are fully tested, so
`FromLinkEmbedding_Raw` is the intended route meanwhile. It is why this test calls
`_Raw`, and why it could notice neither this nor finding A.

The only thing wrong there is that the class docs are ahead of the code: they open with
"then it can be handed over to class `PlanarDiagram` or `PlanarDiagramComplex`", which is
not true yet. Recorded as upstream issue 10, as a TODO rather than a defect.

### B. Zero-length edges are rejected as 3D intersections

**OPEN at `fb4c8f0e`** — still reproduces on all three of `LinkEmbedding2/3/4`, both
coordinate types: the square below gives *"Edges 0 and 2 intersect in 3D"* and
`RequireIntersections` fails, while `LinkEmbedding` accepts it. Acknowledged upstream in
`4d2d0624`: *"Jason's tests revealed that ProsectorX does handle degenerate edges well,
**but not their neighbors**. Will have to work on this."* — which is the same diagnosis
as the paragraph below, reached independently. The four `deg_zero_length` markers are
the only ones left in the tree.

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

**FIXED** — `4d2d0624`. `LinkEmbedding2/3/4<int64_t,int64_t,int64_t>` now instantiates;
the reproducer compiles clean. `-DKNOODLE_TEST_INTEGER_COORDS` could be turned on by
default now, which is the small job this leaves behind.

Note when reproducing anything in this class: the offending members are instantiated
only when *used*, so declaring the type is not enough — the reproducer has to reach
`RequireIntersections()`. A bare declaration compiled happily and made this look fixed
before it was.

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

**HALF FIXED at `fb4c8f0e`.** The `Prosector2` ambiguity is gone; what remains is
`src/WideInt.hpp:114`, *"excess elements in array initializer"*. Henrik is mid-flight
here — the `__int128` commits of 2026-08-14 end in a `Rollback` — and notes in
`4d2d0624` that `int32_t` as `IReal` is problematic anyway because two `WInt32` do not
`long_mul` into a `WInt64`, *"and we want Int64 here anyways for performance reasons"*.
So this may well close by narrowing the documentation rather than by making the pairing
work, which would be a fine outcome; the defect is that the docs promise something that
does not build.

**Severity: low, but the docs recommend this pairing.**

The docs suggest `Real_ = float` with `IReal_ = int32_t`. Neither works:

- `Prosector2`: `call to 'Sign' is ambiguous`, `src/Prosector2/Helpers.hpp:16` (also 19, 21).
- `Prosector3`, `Prosector4`: `excess elements in array initializer`, `src/WideInt.hpp:112`.

Behind `-DKNOODLE_TEST_INT32_BACKEND`.

### E. `LinkEmbedding::RequireIntersections` segfaults on a triple point

**FIXED** — `d26c301f`, "Fixed segfault in LinkEmbedding::FindIntersections in case of
close encounter." The reproducer now exits 0. It does not *succeed*: it returns error
code 8, having detected that two intersection times are too close to order, and declines
— which is the correct outcome for a float class on a degenerate projection. That new
code is what the rotation tier had to learn to accept; see
[§2](#a-refusal-is-not-a-wrong-answer).

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

**FIXED** — in `8db4cdc4`. The measurement below now reads `3 -> 7, 7, 7` where it read
`3 / 3 / 0 / 10`: `RequireIntersections`, `FindIntersections`, and the refreshed pair
all agree. This is the one that produced 24 XPASS from a single marker line.

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

**FIXED** — `8db4cdc4`. Both readers parse again with assertions enabled.

**Correction to the original report, found while re-verifying:** the defect was
duplicated in **two** files, not one. This entry named
`src/LinkEmbedding2/FromFile.hpp:139`, but the abort actually observed came from
`src/LinkEmbedding/FromFile.hpp:141` — the same pair of assertions, copied. A patch
against only the cited file would have left the reader still aborting. Worth a habit:
when a defect is in duplicated code, grep for the duplicate before filing the location.

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

Re-measured at `fb4c8f0e`, 2026-08-14.

| fixture | projection | LE1 | LE2 | LE3 | LE4 |
| --- | --- | ---: | ---: | ---: | ---: |
| `lattice_04` (64 edges) | degen | *refuses* | 0.025 | 0.014 | 0.014 |
| | generic | 0.010 | 0.016 | 0.009 | 0.009 |
| `lattice_08` (512) | degen | *refuses* | 0.389 | 0.182 | 0.200 |
| | generic | 0.111 | 0.238 | 0.094 | 0.096 |
| `lattice_12` (1728) | degen | *refuses* | 1.728 | 1.047 | 1.068 |
| | generic | 0.473 | 0.888 | 0.589 | 0.575 |
| `lattice_16` (4096) | degen | *refuses* | 5.853 | 3.514 | 3.568 |
| | generic | 1.700 | 3.516 | 1.950 | 1.909 |

`LinkEmbedding4` still matches the floating-point class on generic input (1.91 ms vs
1.70 ms at 16³) while also handling the degenerate projection `LinkEmbedding` cannot run
at all — that is the headline and it is unchanged.

**What did change: 3 and 4 are now indistinguishable.** The earlier table showed a
monotone 2 → 3 → 4, and it no longer holds — at every size the two are within noise of
each other, and at `lattice_08` and `lattice_16` degen, LE3 is nominally the faster.
That is the expected consequence of `4d2d0624` merging the three classes onto one shared
`src/LinkEmbedding_Int/` implementation: the differences that made 4 faster were the
duplicated code that got collapsed. If a future Prosector is meant to beat 3, this table
is the baseline it has to beat.

`rounding_err = 0` everywhere, as expected for integral input on the
`scaling_exponent = 0` path.

---

## 7. Current status

At `fb4c8f0e` plus the i64 switch-on, 2026-08-14:

```
./embedding_check             1106 passed, 0 failed, 89 skipped, 42 known-failing
./embedding_check --isolate   2156 passed, 0 failed, 141 skipped, 60 known-failing
```

Before integral coordinates and the `symmetry` tier were added, the same tree gave
792/0/32/24 and 1748/0/80/42 — so the two together are worth about 40% more checks. All
42 known-failures are still finding B: `deg_zero_length` now across five tiers rather
than four, since an axis permutation of a zero-length edge still has a zero-length
edge.

Exit 0 in both modes, and **no XPASS** — every marker still in the tree describes a
defect that is still real.

The 53 known-failures of 2026-08-13 are down to 24, and all 24 are finding B: the
`deg_zero_length` fixture across `exact`, `cross`, `invariant` and `rotation`, for
classes 2/3/4 at both coordinate types. The skips are that same fixture where the
backends fail identically and there is nothing left to compare.

For comparison, the 2026-08-13 baseline was:

```
./embedding_check              650 passed, 0 failed, 34 skipped, 53 known-failing
./embedding_check --isolate   1606 passed, 0 failed, 78 skipped, 186 known-failing
```

The pass count went **up** as well as the known-failure count going down, because fixing
findings E and F let whole units run that previously crashed or were skipped.
