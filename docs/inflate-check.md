# inflate_check — randomized inflate-to-100k stress test

*Implemented 2026-06-13. `test/inflate_check.cpp`.*

## Idea

Take a known knot, **inflate** it to ~100,000 crossings by repeatedly
Reapr-embedding it in 3D and reprojecting from a random direction (each round
multiplies the crossing count), then run **KnoodleSimplify** and confirm it
collapses back to a small diagram of the *same* knot. Three independent failure
signals catch bugs anywhere along the pipeline:

1. **Alexander tripwire (per round).** The Alexander value (a knot invariant,
   computed fast via sparse UMFPACK even at 100k crossings) must not change. If
   a reprojection round corrupts the diagram into a different knot, it shows up
   immediately — and we know *which* round.
2. **Simplify must reduce.** If KnoodleSimplify can't pull the giant diagram
   back down to a small one, that's a strong bug signal (it should, since it's
   still a small knot wearing a huge disguise).
3. **HOMFLY at the small end.** Once recovered to a computable size, HOMFLY of
   the recovered diagram (via the vendored libhomfly) must equal the start's.

## Why it works (validated)

- **Inflation is real and multiplicative.** A trefoil grows
  3 → 5 → 14 → 47 → 109 → … and reaches 100k+ in ~20 rounds, staying a single
  connected knot. `emb = reapr.Embedding(pd); emb.Rotate(reapr.RandomRotation());
  PD_T::FromLinkEmbedding(emb)` is one round.
- **Alexander is a stable tripwire.** Across a full inflation of the trefoil to
  ~250k crossings the value held to ~5 significant digits (the determinant −3
  exact; ~1e-5 float drift at the top end — the test uses a 1e-3 relative
  tolerance, comfortably between the drift and the O(1) gap to other knots).
- **Simplify is fast at scale.** Collapsing a 247k-crossing trefoil took
  ~0.17 s and recovered exactly 3 crossings; a 120k-crossing inflation of a real
  12-crossing KLUT knot recovered exactly 12 crossings.

## Alexander is knots-only

`Alexander_UMFPACK` aborts on multi-component links (no multivariable Alexander
in Knoodle), so this test runs on **knots**. That is the one place it diverges
from "start from the link table": the fast huge-diagram tripwire needs a knot.
(Links would only get the Simplify-reduces and HOMFLY-at-ends checks, with no
per-round tripwire on the giant stages.)

## Reproducibility / audit trail

The run is deterministic from a master `--seed`. Each round re-seeds the Reapr
RNG with `splitmix64(seed + round)`, so a failing round is reproducible from
(its input diagram, that derived seed) alone. On any failure the input diagram
is dumped to `inflate_fail_seed<S>_round<R>.tsv` and the recipe is printed:
load that PD, set the Reapr engine to the named seed, do one embed+reproject.

## Usage

```sh
cd test
make inflate_check          # needs UMFPACK + BLAS/LAPACK (Accelerate on macOS)
./inflate_check --start=klut:12 --seed=3            # realistic 12-crossing knot
./inflate_check --start=trefoil --target=100000     # default target
./inflate_check --trials=8 --target=20000 --seed=100 # fuzz many seeds, faster
```

`--start`: `trefoil` (default) | `figure_eight` | `klut:C[:i]` (reconstruct the
i-th C-crossing knot from the KLUT MacLeod table) | `pd:FILE`. Other flags:
`--target`, `--max-rounds`, `--reduce-to`, `--trials`, `--verbose`.

## Performance note

The bottleneck is the Reapr embedding of the largest diagrams (energy
minimisation on a ~100k-vertex curve): tens of seconds per trial at target 100k,
~2 s at target 15k. Simplify and Alexander are cheap. For broad seed fuzzing,
use a smaller `--target`; for a single deep run, keep 100k.

## Build

Heavier than the other test drivers: `-DKNOODLE_USE_UMFPACK`, force-include
`Tensors/Accelerate.hpp` (defines the BLAS types), and link
`-lumfpack -framework Accelerate`. Not part of `make all`; build with
`make inflate_check`. See `test/Makefile`.

## Found along the way

An upstream bug in the single-value `Alexander_UMFPACK::Alexander(pd, arg,
mantissa, exponent, …)` overload — `mantissa`/`exponent` are passed **by value**,
so it can't return results. The batch (pointer) overload works and is what this
test uses. See [upstream-issues.md](upstream-issues.md).
