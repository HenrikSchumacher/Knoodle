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
