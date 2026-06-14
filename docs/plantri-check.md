# plantri_check — exhaustive generator-based HOMFLY-invariance test

*Implemented 2026-06-14. `test/plantri_check.cpp`. On local `main`.*

## What it checks

Simplification must not change a diagram's knot/link type, so it must not change
its HOMFLY polynomial. `plantri_check` enumerates **every** small planar diagram
with the vendored `plantri` and verifies `HOMFLY(D) == HOMFLY(Simplify(D))` on
each — knots *and* links.

The pipeline is a self-contained C++ port of `test/run_tests.py`'s Tier 2:

```
plantri -E V  ->  edge_code graphs (planar quadrangulations)
   -> quad_to_shadow:   dual of the quadrangulation = a 4-valent graph (shadow)
   -> assign + trace:   each of the 2^n over/under assignments -> a signed PD code
   -> CheckInvariance:  HOMFLY(diagram) vs HOMFLY(Simplify(diagram))
```

For crossing number `n`, plantri is asked for `V = n + 2` vertices. The dual of
a planar quadrangulation is a 4-valent graph — exactly a knot/link **shadow**;
its 2ⁿ crossing sign assignments give all the diagrams on that shadow.

## What it checks per diagram

`Simplify` carries a **color** on every arc — a *persistent label* for each link
component, preserved across simplification (connect-sum summands of one
component share its color). After one `Simplify`, the test checks two invariants
(plus an optional third, `--klut`) — exit is nonzero if any fails:

1. **Link-component count** (cheap, HOMFLY-free). `Simplify` must not change the
   number of link components = `PlanarDiagramComplex::ColorCount()`. This catches
   *component-loss* bugs directly and is the only check needed for a fast
   high-crossing torture run (`--no-homfly`).
2. **Per-sublink HOMFLY** (the strong property). Because colors are persistent
   labels, not only must the whole-link HOMFLY be preserved, but the link type
   of **every sublink** — the components in *any subset* of colors — must be too.
   The test reassembles the result with `ToSingleDiagram()` and, for each
   non-empty color subset `S`, compares `HOMFLY(input restricted to S)` against
   `HOMFLY(simplified restricted to S)` via `SubdiagramByColors`. The full
   subset is the ordinary whole-link check; the proper subsets additionally
   catch a single component's knot type changing, or two components' linking
   changing, even when the whole-link polynomial happens to match. (For a knot,
   k = 1 component, so this is exactly one comparison — no extra cost; cost grows
   as 2^k in the component count k, capped at k ≤ 16.)

3. **KLUT coverage** (optional, `--klut[=DIR]`). The KLUT promises that *every
   simplified, diagrammatically prime, ≤13-crossing knot is a table key*. The
   simplified result is already a complex of diagrammatically-prime pieces, so
   for each piece that is a single-component knot with 3 ≤ crossings ≤ 13 we look
   up its MacLeod key via `Klut::FindName`; a `NotFound` is a real coverage gap.
   This is the *only* test in the suite that exercises the table's coverage
   contract — `klut_check` tests internal consistency, and `klut_e2e` fed back
   already-minimal table keys (Simplify was a no-op there). It rides along on the
   `Simplify` that the invariant checks already run, so it costs only a hash
   lookup per piece; `--klut` is off by default, keeping the test self-contained.

HOMFLY is computed by the vendored, public-domain **libhomfly**, fed by the
library's own `PlanarDiagram::ToJenkinsCodeString()`, with the split-link
**delta rule** (`H(A ⊔ B) = δ·H(A)·H(B)`). The oracle lives in
`test/homfly_invariance.hpp`, **shared** with `homfly_check.cpp`, so
`plantri_check` uses the exact HOMFLY oracle that `homfly_check` cross-validates
against libhomfly's published reference polynomials. No Python, no Regina, no
`tools/`.

### A note on split links and `ToSingleDiagram()`

`Simplify` returns a `PlanarDiagramComplex` whose arcs carry **colors**: same
color = connect-summed (HOMFLY multiplies, no δ); distinct colors co-occurring
in one connected piece = linked (no δ); distinct colors in disjoint pieces =
split (one δ each). `ToSingleDiagram()` reassembles via
`Splitting → AnelliToFarfalle → Connect`; the *anelli→farfalle* step encodes the
split δ factors, so it is **HOMFLY-faithful for split links too**. This was
verified (2026-06-14): an explicit color-aware `δ^(S−1)·∏H(piece)` computation
agrees with `ToSingleDiagram` on 2- and 3-way split links and on all 8.5M
8-crossing diagrams. So no "split-link oracle fix" was needed — a change on a
split link is a genuine `Simplify` bug, and we kept the simpler library path.

## plantri modes

`V = n + 2`; `-E` is plantri's binary edge_code output.

| mode (`--plantri-mode`) | command | graphs at V=8 |
|---|---|---|
| `everything` (default) | `plantri -Q -E V` (all quadrangulations) | 733 |
| `no-r1` | `plantri -Q -c2 -E V` (no length-2 cycles) | 17 |
| `simple` | `plantri -q -E V` (simple quads; V even, ≥ 8) | 1 |

`everything` is the broad-coverage mode (and what reproduces the historical
counts); `simple` matches the documented KLUT-generation invocation but only
reaches crossing number ≥ 6 (one graph). This also pins down the repo's
long-standing open question — *which plantri command line builds the diagrams* —
to these three.

## Vendoring

`test/vendor/plantri/plantri.c` is plantri **5.8 (March 4 2026)**, unmodified,
under the **Apache License 2.0** (`LICENSE-2.0.txt`; see also
`README.upstream.md`). plantri is one translation unit (C stdlib only), built by
the `plantri` target in `test/Makefile`. Earlier plantri releases were not
redistributable; the 5.8 relicensing to Apache 2.0 is what makes this vendoring
legitimate.

## Result

Through **7 crossings, every diagram passes** — `Simplify` preserved HOMFLY on
all of them — in `everything` mode:

| up to | diagrams | (knots / links) | bugs | time |
|---|---|---|---|---|
| 6 crossings | 51,428 | 26,856 / 24,572 | 0 | ~0.9 s |
| 7 crossings | +587,008 | 280,832 / 306,176 | 0 | ~11 s |
| 8 crossings | +8,543,488 | 3,741,184 / 4,802,304 | **208 links** | ~3 min |

Diagram counts match `run_tests.py`'s Tier 2 exactly (V=4: 3 graphs, V=5: 7,
V=6: 30, V=7: 124, V=8: 733), confirming the C++ converter is a faithful port.

**8 crossings surfaces 208 genuine simplification bugs.** Every one is a
multi-component **unlink** that `Simplify` reduces to a *lower* unlink — e.g. a
3-component unlink (HOMFLY `before = δ²`) comes back with two components
(`after = δ`). It is a real bug, not an oracle artifact: `Simplify`
*deterministically* returns a 2-component result for these 3-unlink inputs
(`ColorCount`/`DiagramCount` = 2), i.e. it **drops a split unknot component**.
Both the cheap component-count check and the HOMFLY check flag the same 208.
This 8-crossing Tier-2 sweep had never been run before (the Python suite OOM'd
there; this test streams plantri, so memory is bounded). Dump the offending
diagrams with `--dump-changed=DIR` to investigate.

**Historical note.** As of 2026-03-24, Tier 2 had **8 known failures** up to 6
crossings — 4-component links at V=8 g510 — then under investigation. They are
**gone**: both this test and the Python Tier 2 now report 0 failures over all
51,428 ≤6-crossing diagrams, so that bug was fixed in the interim. The 208 new
failures are a *distinct*, deeper class found only at 8 crossings.

## Usage

```sh
cd test
make plantri_check          # builds vendored plantri + the test (no UMFPACK)
./plantri_check                              # everything mode, c=3..6 (~0.9 s)
./plantri_check --up-to-crossing=7           # +7-crossing (~11 s, clean)
./plantri_check --up-to-crossing=8           # +8-crossing (~3 min, finds 208)
./plantri_check --up-to-crossing=8 --dump-changed=bugs/   # save reproducers
./plantri_check --knots-only                 # skip link diagrams
./plantri_check --crossing-assignments=8     # sample 8 random sign masks/shadow
```

`--dump-changed=DIR` writes one `.tsv` per offending diagram, directly
re-checkable via `./homfly_check --invariance bugs/*.tsv`.

plantri output is **streamed**, so memory stays bounded at any crossing number;
**Ctrl-C** stops early and still prints a summary. For an all-night torture test
at, say, 13 crossings, drop HOMFLY (the component-count check is far cheaper and
catches the component-loss class), sample sign masks, and cap the work:

```sh
./plantri_check --from-crossing=13 --up-to-crossing=13 \
                --crossing-assignments=64 --no-homfly --max-diagrams=50000000
```

Flags: `--plantri`, `--plantri-mode`, `--crossing-assignments`,
`--from-crossing`, `--up-to-crossing`, `--no-homfly`, `--knots-only`,
`--max-diagrams`, `--max-graphs`, `--progress-every`, `--dump-changed`,
`--shard`, `--klut`, `--seed`. Exit code is nonzero iff `Simplify` changed the
component count or HOMFLY of some diagram, or (`--klut`) a simplified prime knot
piece was missing from the table.

## Cluster fan-out (`--shard=RES/MOD`)

The test parallelizes trivially across cluster jobs by reusing **plantri's own
`res/mod`** graph partitioning — designed for exactly this. `--shard=RES/MOD`
appends `RES/MOD` to every plantri invocation, so the job processes plantri's
disjoint `1/MOD` slice of the graphs at each crossing number. The union of all
`MOD` shards is the complete, exhaustive sweep (verified: V=8's 733 graphs split
`191 + 321 + 221` across `0/3,1/3,2/3`), with no inter-job communication.

Each shard ends with a single sum-reducible line:

```
SUMMARY	shard=5/166	from=3	to=9	tested=...	knots=...	links=...
        comp_changed=...	homfly_knot=...	homfly_link=...	skipped=...	bugs=...
```

Reproducers are dumped with a shard tag (`s5-166_V11_g…_m…comp.tsv`) so many
jobs can share one `--dump-changed` directory without name collisions, and each
is still independently re-checkable via `homfly_check --invariance`.

`test/cluster/` has a ready SLURM array template and a reducer:

```sh
# 166 array tasks, exhaustive 9-crossing component sweep, on shared scratch:
sbatch --array=0-165 test/cluster/plantri_sweep.sbatch
# then reassemble:
python3 test/cluster/plantri_reduce.py sweep_9cx/summary/job_*.out
```

The reducer sums the counts, reports any missing shards (incomplete sweep), and
rolls the per-shard exit codes into a single PASS/FAIL. At 9 crossings (~121M
diagrams) over 166 shards, each task is ~730k diagrams — a few seconds
(component-only) to ~30 s (full per-sublink HOMFLY). Balance is approximate
(plantri's res/mod classes aren't exactly equal), but with hundreds of thousands
of graphs per crossing number the spread is small, and SLURM array tasks are
independent so stragglers don't block the gather.
