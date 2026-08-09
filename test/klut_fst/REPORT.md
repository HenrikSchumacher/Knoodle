# KLUT FST compression experiment — results

Branch `klut-fst-experiment`, 2026-08-09. Runs the ROUND-1 experiment from
`handoff/klut-fst-compression/`: build a minimal acyclic subsequential FST
over the (MacLeod code → knot-type ID) map of the 13-crossing KLUT table and
measure bytes/key and query time against the shipped representation.

## Setup

- FST library: Rust `fst` crate v0.4.7 (BurntSushi) — a production
  implementation of the Daciuk–Mihov–Watson–Watson sorted-key streaming
  construction with onward (Mohri canonical) outputs, the same algorithm as
  Lucene's FST package. Chosen over Lucene because this machine has no Java
  runtime; per JHC's call in this session.
- Data: `data/Klut/Klut_Keys_13.bin` + `Klut_Values_13.tsv` —
  1,536,686 keys (13-byte short MacLeod codes), 33,814 knot-type names.
  Keys use 44 distinct byte values (12..95), so 6 bits/symbol suffice after
  a dense remap.
- Pipeline: `export_pairs.py` flattens the grouped key file into sorted
  (key, type-ID) records; `fst_bench` builds one FST per serialization
  variant, verifies every key maps to its ID, checks membership (perturbed
  keys must miss), and times random-order lookups of every key.
- Baseline probe: `klut_hash_probe.cpp` loads the same subtable through the
  real `Knoodle::Klut` reader built with `-DKNOODLE_USE_BOOST_UNORDERED`
  (as knoodleidentify ships) and measures RSS delta and `FindID` time.

## Results (13 crossings, 1,536,686 keys, 33,814 types)

| representation                                | B/key | total    | query      |
|-----------------------------------------------|------:|---------:|-----------:|
| shipped in-memory table (boost flat map, RSS)  | 54.8  | 84.2 MB  | 94 ns      |
| flat sorted array, keys+IDs (hypothetical)     | 17.0  | 26.1 MB  | (bin search) |
| raw key file on disk (IDs implicit)            | 13.0  | 20.0 MB  | —          |
| FST, forward bytes                             | 9.35  | 14.4 MB  | 486 ns     |
| FST, reversed bytes                            | 9.16  | 14.1 MB  | 543 ns     |
| FST, 6-bit packed                              | 9.04  | 13.9 MB  | 429 ns     |
| FST, 6-bit packed + reversed                   | 8.91  | 13.7 MB  | 417 ns     |
| structureless floor, 1.23·log₂(types)          | 2.31  |  3.6 MB  | —          |
| zstd -19 on sorted keys (NOT queryable)        | 2.11  |  3.2 MB  | —          |

Build time: 0.3 s for the full 13-crossing FST. Membership test: 10,000/10,000
perturbed keys correctly rejected (the off-domain escalation hook works).

Smaller tables for the trend (forward-bytes FST): 11 crossings 8.85 B/key,
12 crossings 9.36 B/key — bytes/key is essentially flat in crossing number
while raw keys grow 1 byte per crossing.

Structural facts: mean longest-common-prefix between lexicographic neighbors
is 7.03 of 13 bytes, so plain front-coding of the sorted file would cost
≈ 7 B/key before IDs — the FST's ~9 B/key (IDs included) is consistent with
~6 unshared tail symbols per key at ~1.2 B/transition of format overhead plus
output deltas. The serialization sweep moves the number by only 5%
(8.91–9.36), so the encoding convention is not the lever ROUND-1 hoped.

## Reading the numbers

- **vs shipped RAM**: the FST is a 6.1× reduction (84 MB → 14 MB) and the
  query stays sub-microsecond (417–486 ns vs 94 ns hash) — noise next to
  computing the MacLeod code, and ~100× faster than an NVMe probe.
- **vs the raw file**: only 1.45×. The exact-minimal FST mostly re-derives
  prefix sharing and pays it back in per-transition format overhead.
- **vs what's achievable**: zstd's 2.1 B/key shows ~4× more latent structure
  than the FST captures. The redundancy is NOT (only) prefix/suffix shape —
  the 6-bit alphabet alone is a 0.75 factor, and the sorted-neighbor deltas
  are themselves highly predictable.

## Extrapolation to 14–16 crossings

Key-count growth is ×5.7 (11→12) and ×6.6 (12→13); assuming ×7–8 onward:
~1.1·10⁷ keys at 14, ~8·10⁷ at 15, ~6·10⁸ at 16. At a flat ~9.5 B/key:

| crossings | keys (est.) | FST (est.) | shipped repr. (est., 55 B/key) |
|-----------|------------:|-----------:|-------------------------------:|
| 14        | ~11 M       | ~0.1 GB    | ~0.6 GB                        |
| 15        | ~80 M       | ~0.8 GB    | ~4.4 GB                        |
| 16        | ~600 M      | ~6 GB      | ~33 GB                         |

So the FST comfortably puts 14 and 15 in RAM next to the knot-type tables;
16 is borderline (~6 GB for this table alone).

## Suggested round-2 direction

The measured gap (FST 9 vs zstd 2.1) points away from FST serialization
tweaks and toward a queryable delta structure: blocks of front-coded,
6-bit-packed sorted keys (with per-block entropy coding if wanted), a
sampled first-key index, binary search to a block, linear decode within.
Estimated 2.5–4 B/key at a few hundred ns/query, same membership behavior
as the FST, trivially simple C++. The FST's one unique asset — emitting the
type ID at the earliest deciding prefix — bought little here because the
deciding prefix is late (mean LCP 7/13 across *all* neighbors, including
same-type ones).

## Addendum: v2-invariant prefix (JHC follow-up, same day)

Question: does prefixing each key with its v2 invariant (Casson / Conway a₂,
constant on every key of a type by invariance) help the FST? The
`knoodleinvariants` v2 tool computed v2 for all 1,536,686 keys (via
`keys_to_pd.cpp` reconstruction, ~1 min); `augment_v2.py` then built a 2×2
experiment — {plain, v2-prefixed} × {original IDs, IDs renumbered in key
order} — to separate key-side from output-side effects.

Validation bonus: v2 is constant on all 33,814 types (30 distinct values,
range −8..21), cross-checking both the table and the v2 implementation.

| variant                        | forward B/key | best B/key |
|--------------------------------|--------------:|-----------:|
| plain (round-1 baseline)       | 9.35          | 8.91       |
| plain + ID remap               | 9.37          | 8.98       |
| v2 prefix                      | 11.60         | 10.37      |
| v2 prefix + ID remap           | 11.33         | 10.57      |

**The v2 prefix makes the FST ~24% larger, and the ID remap is pure noise.**
Both facts have the same explanation: the FST's size lives almost entirely in
unshared key *tails* (output bytes are negligible — that's what the remap
control shows). Prefixing by v2 re-partitions the sorted stream into 30
buckets, and since same-type keys are *scattered* across MacLeod lex space
(~45 keys/type, not neighbors), bucketing separates former lexicographic
neighbors: mean neighbor LCP of the MacLeod part drops from 7.03 to 5.11
bytes. Two extra unshared symbols × ~1.2 B/transition ≈ the +2.2 B/key
observed.

Where v2 *does* pay: on the **value side**. Knowing v2 at query time is
worth 3.64 bits/key of the type ID (key-weighted log₂(types per v2 bucket)
= 11.41 vs 15.05 unconditional), dropping the structureless retrieval floor
from 2.31 to 1.75 B/key. That is the first empirical data point for the
invariant-cascade architecture (DESIGN-CONTEXT idea 3): invariants belong in
the output/exception ledger, not in the key string.

## Round 2: front-coded blocks (fcb_bench.cpp, same day)

The structure pitched in round 1, implemented in dependency-free C++ (plus
optional per-block zstd): keys sorted, fixed-size blocks, first key whole,
later keys as (LCP, suffix); a sampled index (first key + offset per block);
query = binary search the index, decode one block linearly with early exit.
IDs are a flat uint16 array in key order (33,814 < 2¹⁶), 2.00 B/key, listed
separately below. All variants verified over all 1,536,686 keys;
10,000/10,000 perturbed keys miss (membership intact).

13 crossings, selected rows (full sweep in the tool's output):

| variant   | block | keys B/key | total B/key | query    |
|-----------|------:|-----------:|------------:|---------:|
| fcb-byte  | 64    | 7.33       | 9.33        | 418 ns   |
| fcb-6bit  | 16    | 6.37       | 8.37        | 286 ns   |
| fcb-6bit  | 64    | 5.33       | 7.33        | 447 ns   |
| fcb-6bit  | 256   | 5.07       | 7.07        | 1.3 µs   |
| zstd-raw  | 1024  | 2.89       | 4.89        | 21 µs    |
| zstd-fc   | 256   | 2.84       | 4.84        | 4.1 µs   |
| zstd-fc   | 1024  | 2.44       | 4.44        | 14 µs    |

(zstd variants: per-block zstd-19 with a 110 KB ZDICT-trained dictionary,
decompressed per query; `zstd-fc` compresses the front-coded payload,
`zstd-raw` the raw block keys. Dictionary + index counted in "keys".)

Takeaways:

- **Plain front coding beats the FST with ~150 lines of dependency-free
  C++**: fcb-6bit B=64 = 7.33 B/key at 447 ns (FST: 8.91 at 417 ns), and
  B=16 = 8.37 B/key at 286 ns — *faster* than the FST.
- **Per-block zstd closes most of the gap to the global-zstd ceiling**:
  keys-side 2.44 B/key at B=1024 vs the 2.11 non-queryable bound. The
  queryable structure basically achieves the compressor's view of the data.
- **The ID array now dominates** (2.00 of 4.44 B/key at the small end).
  Unconditional ID entropy is log₂ 33814 = 15.05 bits = 1.88 B — raw
  uint16 is within 6% of optimal, so only *context* helps: v2-conditioned
  ranks (−3.64 bits, → ~1.43 B/key + a rank↔ID table) or a fuller
  invariant cascade. This is where the addendum's value-side v2 plugs in.
- Query cost scales with block size (decompression dominates); even the
  21 µs worst case matches the NVMe-probe floor, and the 4 µs sweet spot
  (zstd-fc B=256, 4.84 B/key total) is well under it.

Updated RAM ladder for 13 crossings: shipped 84.2 MB → FST 13.7 MB →
fcb-6bit/64 11.3 MB → zstd-fc/256 7.4 MB (→ ~6.6 MB with v2-ranked IDs).
Extrapolated to 16 crossings (~6·10⁸ keys, B/key creeping up with key
length): roughly 3–4 GB for zstd-fc/256 vs ~6 GB FST vs ~33 GB shipped.

## Reproducing

```sh
# export sorted pairs (writes to $SCRATCH)
python3 export_pairs.py ../../data/Klut 13 $SCRATCH/pairs_13.bin
# build + measure FSTs (needs rust: brew install rust)
cd fst_bench && cargo build --release
./target/release/fst_bench $SCRATCH/pairs_13.bin 13
# shipped-table baseline (build line in klut_hash_probe.cpp header)
./klut_hash_probe ../../data/Klut 13 $SCRATCH/pairs_13.bin
```
