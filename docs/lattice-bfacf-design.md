# Lattice BFACF preprocessing — design

**Status:** experimental, branch `reapr-bfacf-gyration` only. Touches `src/` (Henrik's
read-only core); nothing here merges without his approval.

## Motivation

Simplify works poorly on very complicated knots sampled from **confined ensembles**. The
ReAPR embedding of such a knot is extremely congested and exposes little surface for pass
moves to act on. Hypothesis: relaxing the conformation with **BFACF lattice moves** before
handing it to Simplify decongests the diagram (fewer projected crossings, more pass-move
surface).

We do **not** know which objective works best — this mirrors the earlier ReAPR "which
energy?" question. So the design is a **swappable-objective testbed**, not a single policy.
Two lattice inputs matter: (1) knots already sampled on-lattice (the primary case), and
(2) a ReAPR `LinkEmbedding` re-expressed on the lattice.

## `LatticeLink` — data structure

Self-contained new class (`src/LatticeLink.hpp` + `src/LatticeLink/`). No edits to Henrik's
`LinkEmbedding`; the round-trip is done with free functions / a ctor on the *new* class.

### OccupiedSet
`boost::unordered_flat_set<Key>` on the **doubled lattice** (already a dependency — see
`PlanarDiagram.hpp`, `Klut.hpp`). Real vertex `v` → doubled `2v` (all-even). Edge midpoint →
exactly **one odd** coordinate. We store **both** vertices and midpoints, parity-tagged.

- Every birth/death/flip admissibility test is a single O(1) lookup.
- Storing vertices (not midpoints only) is required for correctness: a new vertex `v'` can
  collide with a *foreign* strand passing through `v'` along a different axis, whose
  midpoints are not the two edges being created. Midpoint-only checking silently admits a
  degree-4 self-intersection → wrong knot type. Storing both makes all tests uniform.
- Memory: `V + E ≈ 2E` entries. `Key` packs doubled `(x,y,z)`. **R_g-maximization spreads
  the polygon**, so size the key for large extent: wide bit-packing with a debug-asserted
  bound, or a `struct{int32 x,y,z}` key (boost can hash it) to remove the ceiling.

### EdgeList
Dense `std::vector` acting as an **intrusive doubly-linked list** (pointer-free; indices, not
pointers). Hot record `{ Key midpoint; int32 next; int32 prev; }` + a **parallel cold
vector** for `color` (touched only on birth-inherit and export). Multiple link components
coexist as disjoint cycles in the one array.

Why `std::vector` and not a list/deque/hive:
- **Uniform-random move-site selection must be O(1)** → requires dense, contiguous, indexable
  storage. `std::list` can't; `deque` adds indirection on the hot path; `hive`/free-lists
  leave gaps that break unbiased `rand()%size`.
- **Deletion = swap-to-end + `pop_back`** (not a free-list) precisely to keep the array
  gap-free so sampling stays valid. 32-bit `next`/`prev` keep the record cache-small;
  reallocation is safe because we store indices, not pointers. Reserve high; never
  `shrink_to_fit` in the loop.

### R_g accumulators (O(1) per move)
Maintain running `S1 = Σ xᵢ` (vec3), `S2 = Σ|xᵢ|²` (scalar), `N`. Then
`R_g² = S2/N − |S1/N|²`. Birth `+2` points, death `−2`, flip moves one point — each updates
`S1,S2,N` in O(1). No O(N) recompute, ever.

## Round-trip with `LinkEmbedding`

- **`LinkEmbedding → LatticeLink` (ctor):** validate each edge is integer-coordinate and
  axis-aligned; **reject zero-length edges** (coincident consecutive vertices) loudly — if
  that ever fires on ReAPR output it is a **ReAPR bug** (upstream handoff), not something to
  absorb. Then **subdivide every edge into unit steps**. Size of the result = **total L1
  length** of the polyline, not the vertex count (couples to ReAPR's `scaling`).
- **`LatticeLink → LinkEmbedding` (export):** visited-bitmap walk over the dense array (no
  maintained component heads — they'd need fixups on every swap-pop). **Re-weld** every
  maximal collinear run of unit edges back into one long edge, then build via the
  `comp_ptr`/`comp_color` ctor + `ReadVertexCoordinates` (as `Reapr/Embedding.hpp` does).

## BFACF moves

Proposal: uniform random directed edge `(p→q)`, `q = p + û`; uniform ⊥ unit direction `d`
(4 in 3D). Plaquette-opposite sites `p' = p+d`, `q' = q+d`. Classify by `OccupiedSet`
occupancy of `p'`,`q'` combined with local `EdgeList` `prev`/`next`:

- **Birth (+2):** `p'`,`q'` both free → replace `(p→q)` with `(p→p')(p'→q')(q'→q)`. Append
  edges; two swaps of `next`/`prev`.
- **Death (−2):** walk already contains the three-sided plaquette `(p→p')(p'→q')(q'→q)` →
  collapse to `(p→q)`. Delete two edges via swap-pop **highest-index-first**, re-reading
  `next`/`prev` after the first pop (the two deletees are walk-adjacent but arbitrarily
  placed in the array — easy corruption site).
- **Flip (0):** length-preserving corner slide; move one vertex, update `OccupiedSet` (remove
  old, add new) and two position records.

**Self-avoidance is enforced by the engine on every move** (admissibility = newly-occupied
site(s) absent from `OccupiedSet`). BFACF is ergodic on the fixed-knot-type class *given*
self-avoidance, so **knot type — and, via the shared OccupiedSet across components, link
type — is invariant by construction**. No objective can break it.

## Swappable objective seam

Enum + `switch` in the move loop — the `Reapr::Energy_T` idiom (runtime-selectable, one
predicted branch per attempt). The engine hands the objective a `MoveContext`; the objective
returns accept/reject and owns its schedule. The objective **never touches the lattice** —
only policy lives there, so a broken experimental objective yields a bad conformation but
never a wrong knot.

```cpp
enum class Objective_T : Int8 { MinLength = 0, MaxRg2Banded = 1, /* ... */ };

struct MoveContext {
    MoveType type;            // Birth | Death | Flip
    Int      dN;              // +2 / -2 / 0
    Real     dRg2;            // exact, O(1)
    Int      N;               // pre-move length
    Real     proposal_ratio;  // engine-computed BFACF kinematic ratio; use or ignore
    Size_T   step;            // for annealing schedules
};
```

### First objective: `MinLength` simulated annealing (reference baseline)
Literature policy, known to reduce crossings. `E = N`; accept `min(1, proposal_ratio ·
exp(−β·dN))` with a mild death bias (sub-critical fugacity) and an annealing schedule
`β ↑` in `step()`. Gives an immediate, comparable number vs ReAPR-alone. Later objectives are added as new enum
arms + `switch` cases:

- **`MaxRg2Banded`** — climb `dRg2` at non-zero temperature with a **soft length band**, NOT
  fixed length. All three moves stay active (flips-only is non-ergodic — corner flips alone
  can't unfold a polygon; you need births/deaths for the conformation to change). Length is
  free to wander in ≈`[N₀/2, 2·N₀]`; a restoring bias (soft-wall / quadratic penalty on `N`,
  ≈0 inside the band, growing outside) pulls it back toward the target `N₀` as it nears the
  edges. Computed entirely from `N`,`dN`,`dRg2` in `MoveContext` — no engine change.
- **tilted-equilibrium**, **confinement-aware** — further arms.

## Implementation status (branch `reapr-bfacf-gyration`)

- **Step 1 (done):** `src/LatticeLink.hpp` + `LatticeLink/{Gyradius,RoundTrip}.hpp`.
  Round-trip + O(1) accumulators. Test `test/lattice_roundtrip_check.cpp`.
- **Step 2 (done):** `LatticeLink/Moves.hpp`. Unified BFACF move (Propose/ApplyProposal
  split, birth/death/flip, swap-pop `DeleteSlot`, `SelfConsistentQ` audit). Each edge owns
  exactly 2 OccupiedSet keys (tail vertex + midpoint), so updates are local. Knot-type
  invariance verified by HOMFLY on a ReAPR trefoil (integer + axis-aligned, ingested
  directly) + unknot controls: `test/lattice_bfacf_invariance.cpp`.
- **Step 3 (done):** `LatticeLink/Objectives.hpp`. `Objective_T` enum + `switch` in the
  `AcceptanceRatio` path (Metropolis; `Step()`/`Run()` drive it). `MinLength` collapses a
  bloated ReAPR trefoil (≈900 edges) to the **minimal 24-edge lattice trefoil**, still a
  trefoil — the decongestion goal, demonstrated. `MaxRg2Banded` expands the compact trefoil
  (R_g² ×17) within a soft length band. Both preserve HOMFLY:
  `test/lattice_bfacf_objective.cpp`. Empirics: constant β≈4–5 (strong death bias) reaches
  minimal length fast; β≲1.6 (near/above critical fugacity) does not shrink.

## Correctness invariants & test plan

1. **Round-trip:** `LinkEmbedding → LatticeLink → LinkEmbedding` is identity up to collinear
   re-welding, on known knots.
2. **Knot/link-type invariance under moves:** run N sweeps, export, compare HOMFLY before/
   after via the existing oracle `test/homfly_invariance.hpp` (same harness as
   `plantri_check`/`inflate_check`). Any mismatch = a self-avoidance bug in the engine.
3. **R_g accumulator check:** periodically recompute `R_g²` from scratch and assert it equals
   the incremental value (debug builds).

## File layout (branch-only)

```
src/LatticeLink.hpp            # main header (mirrors Reapr.hpp)
src/LatticeLink/
    RoundTrip.hpp              # LinkEmbedding <-> LatticeLink (validate+subdivide / re-weld)
    Moves.hpp                  # BFACF propose/classify/apply/revert + O(1) accumulator update
    Objectives.hpp            # Objective_T enum + switch (MinLength first)
    Gyradius.hpp              # S1/S2/N accumulators, R_g^2
docs/lattice-bfacf-design.md  # this file
```

Integration into the Simplify pipeline (a `knoodlesimplify` preprocessing flag, or a new
`tools/` driver) comes after the engine + baseline objective validate end-to-end.

## Deferred / open

- Confinement-aware objective (reject moves leaving a region) — future enum arm.
- Exact BFACF acceptance constants / fugacity calibration — pin during implementation
  against the reference (Janse van Rensburg, *The BFACF algorithm*).
- Coordinate `Key` sizing decision (packed vs struct) — driven by observed extent under
  R_g growth.
