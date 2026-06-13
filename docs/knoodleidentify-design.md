# knoodleidentify — design & implementation notes

*Drafted 2026-06-12, after merging origin/main (which brought `src/Klut.hpp`)
into `new-tools-structure`. **Status: implemented 2026-06-13**
(`tools/knoodleidentify.cpp`, built by the tools Makefile). The sections
below were written as the design; deviations and discoveries during
implementation are collected at the end.*

## Role

Third stage of the pipeline, completing identification:

```
knoodlesimplify --streaming-mode < diagrams.tsv | knoodleidentify
```

`knoodlesimplify` simplifies each input knot and factors it into connect-sum
**summands** (its output format: `k` separates knots, `s` separates summands,
then one 5- or 7-column signed PD code row per crossing). `knoodleidentify`
reads that stream and looks each summand up in the KLUT as it arrives,
emitting one identification line per input knot.

Two facts shape the design (see [klut-overview.md](klut-overview.md)):

1. **Simplification is part of the query.** The table stores MacLeod keys of
   *simplified, canonicalized* diagrams only (`Simplify` ends with
   `Canonicalize()` — `src/PlanarDiagramComplex/Simplify.hpp:356`). A raw
   diagram's key will not match. So this tool deliberately does *not* try to
   identify arbitrary diagrams; it identifies `knoodlesimplify` output.
2. **The table is knots-only, mostly prime.** A handful of composite knots
   appear (their pass-reduced diagrams aren't minimal, so they survive as
   single summands), but in general composites arrive as several summands and
   the composition happens here, in the output line.

## Lookup semantics per summand

| Summand | Result |
|---|---|
| 0 crossings (bare `s` line — verified: an R1 unknot input produces exactly this) | Unknot |
| 1–2 crossings | Cannot occur for a fully simplified knot; report as an anomaly (likely upstream bug or unsimplified input) |
| 3–13 crossings, key found | `Klut::FindName(pd)` → `K[c,idx,j,"coset"]` |
| 3–13 crossings, key missing | `NotFound` — flag loudly; means table-coverage gap or non-canonical input (both worth knowing) |
| > 13 crossings | Out of table range; report crossing count |
| Multi-component | `Klut` returns not_found for `LinkComponentCount() > 1`; we report links as unidentifiable (KLUT is knots-only) |

`Klut::FindName(cref<PlanarDiagram<Int>>)` (`src/Klut/FindName.hpp:75`)
already handles invalid/range/multi-component internally; the tool's job is
mostly the unknot case, friendly reporting, and composition.

## Output format

One line per input knot (i.e. per `k` block), streaming, flushed as each
block completes. Three modes:

### Default — Wolfram Language association (the multiset)

Motivated by SAP experiments where a single knot can have *hundreds* of
summands (e.g. 200 trefoils): the `#`-joined list is then unreadable and
O(n). Instead, emit a WL association from each distinct knot summand to its
multiplicity, which `ToExpression` parses directly:

```
<| KnotSymbol[3,1,True,"e/r"] -> 42 |>
<| KnotSymbol[3,1,True,"e/r"] -> 2, KnotSymbol[3,1,True,"m/mr"] -> 1, Unidentified[15] -> 1 |>
<||>
```

- **Knot symbol** is `KnotSymbol[c, i, a, "sym"]` — head `KnotSymbol`, third
  field `a` the amphichirality flag as a WL boolean `True`/`False`. This is
  exactly the object Henrik's `Klut.nb` ends up with after its
  `names[[All,0]] = KnotSymbol` head-swap and `[[All,3]] /. {0->False,
  1->True}` rewrite, so our output is drop-in (chosen 2026-06-13 with Jason;
  the raw table name `K[c,i,j,"sym"]` has head `K` and integer `j`).
- **Quotes are literal** (`"e/r"`), not backslash-escaped — required for a
  direct `ToExpression["<| ... |>"]` / `a = <|...|>` parse. (Escaping would
  only be needed if the whole association were itself wrapped as a WL string;
  it isn't. Verified parsing live with `wolframscript`: `AssociationQ` True,
  `Total[Values[...]]` matches summand count.)
- **Unknot summands are the connect-sum identity** and are omitted; an
  all-unknot knot yields the empty association `<||>`.
- **Keys are sorted** deterministically (crossing number, index,
  amphichirality, coset; non-knot categories grouped after) — stable for
  diffing/tests, independent of summand arrival order.
- **Non-knot summand keys** (all WL-parseable inert heads, argument =
  crossing count): `Unidentified[N]` (over the 13-crossing table range),
  `NotFound[N]` (in range but absent — a table gap or unsimplified input,
  also warned on stderr), `Link[N]` (multi-component; KLUT is knots-only),
  `Invalid[]`.

### `--expanded` — human-readable, one summand per `#`

The original form: summands joined with ` # ` in arrival order, using the raw
`K[...]` table names and `<...>` human markers. Unknots shown only if nothing
else (`Unknot`).

```
<unidentified:15> # K[3,1,1,"e/r"] # K[3,1,1,"e/r"] # K[3,1,1,"m/mr"]
```

### `--tsv` — one row per summand

`knot_index \t summand_index \t crossing_count \t symbol`, where `symbol`
uses the same WL vocabulary as the default mode (`KnotSymbol[...]`, `Unknot`,
`Unidentified[N]`, …). Unknot rows are included (per-summand, not aggregated).

## CLI

Unix filter like `knoodledraw`: stdin → stdout, errors/log to stderr.

```
knoodleidentify [options] [input_files...]
  --data-dir=PATH    KLUT data directory (default: see below)
  --max-crossings=N  load subtables up to N (default 13 = Klut::max_crossing_count)
  --expanded         '#'-joined per-summand output (raw K[...] names)
  --tsv              machine-readable per-summand output
  --quiet            suppress the stderr summary and anomaly warnings
```

Data directory default: try `$KNOODLE_KLUT_DIR`, then `../data/Klut` relative
to the executable, then `./data/Klut`. (The repo ships the tables in
`data/Klut/` — 11 files, ~40 MB of keys; `Klut::Subtable` lazy-loads each
crossing number on first use, so startup is cheap.)

## Implementation notes

- Reuse `tools/knoodle_io.hpp`: `ReadKnot()` already parses the `k`/`s`
  stream and 4/5/6/7-column formats, and `CreateDiagramFromPDCode()` rebuilds
  a `PD_T`. New tool is a ~200-line sibling of `knoodledraw`.
- One `Knoodle::Klut` instance for the process. Note `FindName(pd)` is
  non-const (shared `s_mac_leod_buffer`) — single-threaded use only, which a
  filter is anyway.
- `Int` is `int64_t` in our tools; `Klut::FindID(PlanarDiagram<Int>)` is
  templated on `Int`, fine.
- Makefile: third target alongside the existing two.

## Verified so far

- **Key stability — RESOLVED (2026-06-12), gating question answered yes.**
  `test/key_roundtrip_probe.cpp` checked every key in every
  `Klut_Keys_NN.bin` (1,816,748 keys, c = 3..13):
  - Stage 1, `key → FromMacLeodCode → WriteMacLeodCode → key`: 0 mismatches.
  - Stage 2, with the knoodleidentify cycle inserted (`PDCode<{signQ=true,
    colorQ=false}>` matrix → `FromPDCode<{signQ=true,colorQ=false}>` with
    `compressQ=true`): 0 mismatches, 0 invalid reconstructions.
  - Stage 0: trefoil → `PDC_T::Simplify` → `Klut::FindName` returns
    `K[3,1,1,"e/r"]`, identical before/after round-trip.
  So the tool can `FromPDCode` read-back text and look up directly — no
  re-canonicalization needed. (Key count per c: 2, 2, 4, 12, 34, 260, 1196,
  6708, 40402, 231442, 1536686.)
- **Text round-trip idempotence (1 case):** simplified trefoil fed back
  through `knoodlesimplify` reproduces the byte-identical PD code.
- **Unknot format:** R1-curl input → output is a bare `s` line, no crossings.

## Open questions / risks

1. ~~Key stability across the text round-trip~~ — resolved, see above.
2. **Table coverage contract** — a `NotFound` on a clean ≤13-crossing summand
   is ambiguous (gap vs bug). The self-consistency test below quantifies it.
3. **Composites in the table**: if a summand lookup returns one of the
   composite entries, just report it — but it'd be good to enumerate which
   composites are in the table (scan `Klut_Values_*.tsv` for names that are
   connect sums — what does KnotInfo call them?).

## Implementation notes (2026-06-13)

**Output-format revision (later 2026-06-13):** the default is now the WL
association described above; the `#`-joined form moved to `--expanded`.
`KnotSymbol[c,i,True/False,"sym"]` rendering, key sorting, unknot-as-identity
(`<||>`), and the `Unidentified/NotFound/Link/Invalid` keys are all in
`ProcessStream` / `WLSymbol` / `SummandLess` in `tools/knoodleidentify.cpp`.
Verified: 42-trefoil synthetic knot → `<| KnotSymbol[3,1,True,"e/r"] -> 42 |>`;
parses as a real `Association` under `wolframscript`. This also **resolves the
`j`-field open question** (overview doc): `j` is the KnotInfo amphichirality
flag (Y/N → 1/0 in the raw `K[...]` table form, `True`/`False` in WL).

What was built matches the original sketch, with these deviations/discoveries:

- **Output markers** standardized as `K[...]`, `Unknot`, `<unidentified:N>`
  (over table range), `<notfound:N>` (in range, no key — also warned on
  stderr), `<link:N>` (multi-component), `<invalid>`. In default mode,
  `Unknot` summands are suppressed when nontrivial summands exist (connect-sum
  identity); a knot with only trivial summands prints `Unknot`.
- **`InputKnot.empty_summand_count`** added to `knoodle_io.hpp`: a bare `s`
  line (knoodlesimplify's unknot output) now parses as an unknot summand
  instead of vanishing. knoodlesimplify also now carries incoming empty
  summands through (`SimplifyKnot` adds them to `unknot_count`) and
  `WriteKnot` emits a bare `s` per counted unknot (previously Reapr-detected
  unknots were silently dropped from the output stream).
- **Bug found & fixed in `ReadKnot`** (`tools/knoodle_io.hpp`): an `s` marker
  did not mark the knot as having content, so for a bare-`s` knot the
  *terminating* `k` was treated as an optional *leading* `k` and swallowed —
  merging the following knot into the unknot's record. Fixed with a separate
  `saw_content` flag (distinct from `first_data_seen`, which gates format
  detection). This affected knoodledraw too.
- **Smoke-tested end-to-end:** trefoil → `K[3,1,1,"e/r"]`; figure-eight →
  `K[4,1,1,"e/m/r/mr"]`; R1 unknot → `Unknot`; mixed 3-knot stream correct in
  both modes; `ExampleKnot.tsv` (1316 crossings) → `<unidentified:15> #
  K[3,1,1,"e/r"] # K[3,1,1,"e/r"] # K[3,1,1,"m/mr"]` (mirror trefoil
  distinguished!); L10a1 link → `<link:10>`; 5 hard unknots → `Unknot`;
  8 children's-game 3D embeddings → 4_1, 3_1, 5_1-mirror, unknots.
- Note: summand decomposition is stochastic (Reapr random embeddings) — the
  same input can split differently across runs (ExampleKnot gave 2 summands
  in one run, 4 in another). Tests comparing decompositions must be
  order-insensitive and tolerant of over-range residuals.

## Test plan (test/ additions)

1. **Exhaustive self-consistency:** for each key in each
   `Klut_Keys_NN.bin`, reconstruct the diagram
   (`PlanarDiagram::FromMacLeodCode`), print its PD code, run it through
   `knoodlesimplify | knoodleidentify`, and check the name matches the key's
   own table entry. Exercises round-trip + lookup on every key (~hundreds of
   thousands of cases, all fast).
2. **Exemplar spot checks:** known knots from test data (trefoil, 4_1, and
   the linktable knots' single-component analogues) → expected names.
3. **Connect sums:** generated composites (e.g. via
   `PlanarDiagramComplex::Connect`/`Unite`, or by concatenating known
   diagrams in input) → expected `#`-joined names, order-insensitively.
4. **Negative cases:** >13-crossing knot (a known 14+ crossing exemplar) →
   clean out-of-range report; a link → knots-only report.
