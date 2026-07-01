# Invariant-throughput benchmark — KLUT vs libhomfly vs Regina

*Context: KLUT resolves a diagram to a **knot identity**; libhomfly and Regina
compute the **HOMFLY polynomial**. KLUT's output is strictly stronger (HOMFLY
does not separate all knots ≤13 — mutants collide). This benchmark measures what
that extra information costs: per-diagram invariant throughput for all three, over
one frozen dataset, reported stratified by knot complexity.*

The three engines span a **reduction spectrum**, which is the interpretive frame
for every number below:

| engine | what it does | cost driver |
|---|---|---|
| **libhomfly** | HOMFLY on the **raw** diagram, no simplification | the *diagram* crossing count (≈exponential) |
| **Regina** | simplify internally, then compute the polynomial | the *reduced* diagram / treewidth |
| **KLUT** | pass-simplify, then table lookup (Reapr escalation only on a miss) | mostly the simplify; lookup is negligible |

## Pieces

Generator and drivers all read/emit the 5-column signed PD lingua franca.

| file | role |
|---|---|
| `test/plantri_gen.hpp` | shared generator core (`PlantriStream`, `QuadToShadow`, `AssignAndTrace`), extracted from `plantri_check.cpp` (its exhaustive test is the regression guard) |
| `test/make_bench_set.cpp` | builds the frozen dataset (below) |
| `test/bench_io.hpp` / `test/bench_io.py` | shared dataset reader + per-diagram result schema |
| `test/klut_ivt_bench.cpp` | KLUT driver — **also the labeling oracle** |
| `test/libhomfly_ivt_bench.cpp` | libhomfly driver (raw diagram) |
| `test/regina_ivt_bench.py` | Regina driver (needs the `venv`) |
| `test/bench_report.py` | joins the three result TSVs, prints stratified throughput |
| `test/bench_xcheck.py` | correctness cross-check (needs Regina) |
| `test/run_ivt_bench.sh` | end-to-end orchestration |

Build the C++ binaries with `make ivt_bench` (light config — no UMFPACK; the KLUT
driver's Reapr uses TopologicalNumbering + MCF, like `plantri_check`).

## The dataset (firehose, done reproducibly)

`make_bench_set` produces a frozen, seed-reproducible set of **knot** diagrams:

1. For each crossing number *n* in `[c-min, c-max]` (plantri vertex count `V = n+2`),
   stream every quadrangulation out of the vendored `plantri` and dualize it to a
   4-valent shadow.
2. **Knots-only filter**: keep a shadow iff it is single-component. Component count
   is a property of the shadow alone (the strand tracing goes straight through each
   crossing, independent of over/under), so it needs no sign choice.
3. **Seeded reservoir sample** (Algorithm R) of `--per-crossing` knot-shadows — a
   *uniform* sample over plantri's exhaustive output, with **no dependence on
   plantri's internal enumeration order**. This is deliberately *not* "the first N"
   (a structurally-biased prefix) nor a `res/mod` slice (`res/mod` splits the
   generation **tree**, so a single residue class over-represents whole structural
   families — it is a parallelization tool, not a sampler).
4. Assign `--masks-per-shadow` iid Bernoulli(½) over/under masks per retained
   shadow → signed PD code.

plantri's output for a fixed *V* is deterministic, so the whole run is reproducible
from `(plantri binary, seed)` — same file, byte for byte. Nothing here touches
Reapr, so unlike the `klut_bench` pool this set is fully seed-reproducible.

### Why plantri-random, not random polygons

For a fixed *diagram* size (which is what bounds HOMFLY cost) the random-diagram
model gives far more knotting per crossing than the random-polygon model, whose
knotting probability is tiny at small edge counts. So plantri + iid signs is the
better firehose. It is still dominated by easy knots — a random 13-crossing diagram
usually *reduces* well below 13 crossings — so the firehose barely exercises the
high-crossing table entries. That is a property of the natural measure, not a flaw;
it is why the report **stratifies by reduced crossing number** and why a separate
type-stratified generator (sampling the KLUT tables directly) is the tool for
guaranteeing coverage of rare high-crossing types.

## Method

- **Same frozen file, same order, single-thread** for all three (apples-to-apples;
  KLUT's parallel path is a separate measurement).
- Each driver splits **construct** (build its native object from the signed PD)
  from **compute** (the invariant itself), and emits one TSV row per diagram:
  `index  nc  construct_ns  compute_ns  result_key`.
- The KLUT driver's `result_key` is `"<reduced_c>:<label>"`; its leading integer is
  the identified knot's **reduced crossing number**. The report joins the three
  TSVs by `index` and bins *every* engine's timing by that number — so the random
  firehose is reported stratified by true knot complexity with no separate pass.
- **Report the tail.** A mean over a million unknots is meaningless; `bench_report.py`
  gives p50/p99/max per bin — the firehose's wall-clock rides on the tail.

## Free correctness oracle

Running all three on one dataset cross-validates them (`bench_xcheck.py`):

1. **libhomfly HOMFLY == Regina HOMFLY**, term for term (their `(L,M)` are
   identical — see `homfly_xcheck.py`). Rebuilds libhomfly's canonical term string
   as a `regina.Laurent2` and compares against the Regina driver's output.
2. **KLUT "unknot" ⟺ trivial HOMFLY** (= 1).

A small self-check run (`c=3..7`, 977 diagrams) passes both, with 0 KLUT labels
mapping to >1 distinct HOMFLY — i.e. KLUT's labels are chirality-aware (the two
trefoil handednesses get distinct labels, each with its own mirror HOMFLY).

## Running it

```sh
cd test
./run_ivt_bench.sh                      # full 3..13 set, all engines
C_MAX=8 PER_CROSSING=2000 ./run_ivt_bench.sh    # quick
REGINA_LIMIT=100000 ./run_ivt_bench.sh          # Regina on a subsample of a big set
```

Configure via env vars (`OUTDIR`, `DATASET`, `C_MIN`, `C_MAX`, `PER_CROSSING`,
`SEED`, `PLANTRI_MODE`, `REGINA_LIMIT`, `PYTHON`) — see the script header. A large
`per-crossing` at `c=13` enumerates many shadows (a one-time cost); freeze the
dataset once and reuse it (`DATASET=...`) for repeat runs.

## Caveats

- KLUT is a specialized ≤13 lookup table; that it beats general polynomial
  computation is expected. The informative outputs are the **shape vs reduced
  crossing number** (KLUT ≈ flat, libhomfly climbing) and the **tail**, not the
  headline ratio.
- Regina's per-call cost includes constructing the `Link`; the driver times that
  separately (`construct_ns`).
- To isolate "cost of not simplifying" from "cost of the polynomial", libhomfly can
  additionally be run on a pre-simplified copy (future add-on).
