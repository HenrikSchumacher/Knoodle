# KLUT identify throughput — profiling note for Henrik

*Context: the polygonal-knot enumeration workload — a firehose of ~10⁹–10¹⁰ small,
non-minimal diagrams, each `Simplify`-ed and looked up in the KLUT, usually to be
discarded. Goal: make per-diagram identification as fast as possible. All numbers
below are single-thread on small diagrams (avg ~11 crossings) unless noted; the
benchmark is `test/klut_bench.cpp` (`--profile=N` to sample, `--inflate=N` for
large inputs).*

## TL;DR

- **The KLUT lookup is a non-issue: ~0.5% of per-item time (~35 ns).** "Make the
  table lookup faster" has no headroom. The cost is entirely `Simplify`.
- **`Simplify` is ~82%, and on small diagrams ~half of *that* is memory
  allocation churn** — `malloc`/`free`/zeroing of per-call Tensor buffers and the
  `string→std::any` cache, *not* topological work (~20%). On large diagrams the
  picture inverts (allocation ~6%, Dijkstra reroute ~62%) — so this is purely a
  small-input concern.
- **Dependency-free win, landed: ~+15%** (single-thread) from boost flat maps over
  both the integer maps and the `string→any` cache — now a single flag
  (`-DKNOODLE_USE_BOOST_UNORDERED`, post-refactor) + two `Simplify` arg settings.
  No third-party allocator, no manual edits. (Mind the build gotcha below.)
- **The remaining ~1.2× → ~1.5× is in-source allocation *elimination*** (no
  third-party allocator), chiefly via **reusable Tensor workspace** for the
  `Compress` / `CreateCompressed` / `FromMacLeodCode` churn. (Tuning the compress
  *settings* buys only ~+2–5%, and removing the finalization compress is
  throughput-neutral — both tested; see below.)

## Where the time goes (CPU sampling, small diagrams)

Per-item leaf breakdown of the `construct → Simplify → MacLeodCode → lookup` chain:

| bucket | share |
|---|---|
| `malloc`/`free`/zeroing | ~49% |
| hashing + `string→std::any` cache | ~14% |
| pass-reduction (FindPass / Reidemeister / Disconnect) | ~11% |
| graph traversal / canonicalize | ~10% |
| other (timing, memcmp/move) | ~15% |

The allocation (~49%) decomposes into three sources:

1. **Integer hash-map nodes** — `Disconnect`'s `AssociativeContainer<Int,Int>`,
   `PassSimplifier::ArcData_HashMap2_T`. Node-based `std::unordered_map`.
   *Addressed by `-DKNOODLE_USE_BOOST_UNORDERED` (flat maps).*
2. **Tensor / PlanarDiagram copy-churn** — the dominant one. `Simplify` builds
   fresh compressed diagrams (`Compress` / `CreateCompressed`), and `Canonicalize`
   rebuilds each knot via `FromMacLeodCode`. Every such diagram allocates **and
   zero-fills** a full set of `Tensor1` members, then frees them. Sampling put the
   `__bzero`/`memset` overwhelmingly under `CreateCompressed`.
3. **The `string→std::any` cache** (`Tools::CachedObject`) — `std::any` heap
   boxing + `unordered_map<string,any>` nodes + key strings.

## Dependency-free wins (no allocator) — ~+15% on the refactored code

As of the hashing refactor on `origin/main`, `CachedObject` uses Tools'
`AssociativeContainer`, and `-DKNOODLE_USE_BOOST_UNORDERED` cascades to
`-DTOOLS_USE_BOOST_UNORDERED` — so a *single flag* now puts boost flat maps under
**both** the integer maps (Disconnect/PassSimplifier) **and** the `string→any`
cache. No manual `GetCache`/`SetCache` editing; the earlier 2-line cache hack is
obsolete.

Measured single-thread (reuse+topo, **system malloc**) on the refactored
`origin/main`:

| config | items/s | vs baseline |
|---|---|---|
| baseline (std maps + std cache) | ~135k | — |
| boost maps + auto boost cache (`-DKNOODLE_USE_BOOST_UNORDERED`) + reuse-reapr + topo | ~155k (best run 175k) | **~+15%** |

100% correct, zero runtime dependency. (Thermally noisy on a 4-P-core laptop,
range ~146–175k; the signal is a solid double-digit gain.) Parallel peak was
~unchanged on that machine — likely thermal/saturation, so re-confirm on a
homogeneous cluster node. (`reuse-reapr` = the `Simplify(reapr, args)` overload,
avoiding a fresh `Reapr` per call.)

> **Build gotcha — this silently robs the win if missed.** The
> `KNOODLE_USE_BOOST_UNORDERED → TOOLS_USE_BOOST_UNORDERED` cascade lives in
> `Knoodle.hpp`, so it is *defeated* when a build `-include`s a Tensors header
> (the UMFPACK/Accelerate path) **before** `Knoodle.hpp` — the Tools container is
> then fixed to `std::unordered_map` before the cascade runs, and boost silently
> never activates. In such builds (the UMFPACK test/tool targets), **also pass
> `-DTOOLS_USE_BOOST_UNORDERED` directly on the command line.** Verify with
> `nm <binary> | c++filt | grep -c boost::unordered` (must be nonzero — it was
> *zero* on the first attempt here, giving a false ~0% result until corrected).

## Compression settings — detailed breakdown (and the redundancy)

There are three distinct compress sites; only two have knobs, and tuning them buys
little. **The big compression cost is structural, not a setting.**

1. **Initial compress** — `compress_initialQ` (default `true`), one `Compress()`
   per input "to make the input canonical ordering." Setting it `false` measured
   **~+5%** on small diagrams. But your own comment warns it "can make a huge
   difference in runtime" (canonical ordering speeds the passes), and on large
   diagrams skipping it is likely net-negative. Knob exists.
2. **Per-pass `ConditionalCompress`** — `compression_threshold`
   (`SimplifyPasses.hpp:67`, `SimplifyStrands.hpp:26`), semantics "compress only
   if `crossing_count > threshold`." Setting it to 100 (never compress a
   ≤100-crossing diagram) measured **~0%** — these calls were already near-no-ops
   because after the initial compress the diagram stays tight
   (`crossing_count == max_crossing_count`, so `ConditionalCompress` short-circuits).
   Knob exists but is effectively inert for this workload.
3. **Finalization `CreateCompressed`** — in the proven-minimal push
   (`PushDiagramDone(pd.CreateCompressed())`), gated only by
   `crossing_count < max_crossing_count`, **no knob.** Sampling put a lot of the
   `__bzero` under this call, so it *looked* like a redundancy worth removing:
   `Simplify` ends with `Canonicalize()` (`Canonicalize.hpp`), which rebuilds each
   knot via `macleod = pd.MacLeodCode(); pd = FromMacLeodCode(macleod, …)`, and
   `MacLeodCode()` (`MacLeodCode.hpp:109`) traverses the active structure (it does
   **not** need a compacted diagram — verified), so the compacted copy is consumed
   only to feed `MacLeodCode` and then discarded.

   **Tested it — and removing it is throughput-neutral (a wash within run-to-run
   noise), not a win.** Correctness is preserved (so the compress *is* redundant
   for *correctness*), but skipping it doesn't speed anything up: the alloc/zero it
   saves is offset because (a) `Canonicalize`'s `FromMacLeodCode` still allocates
   the canonical diagram regardless, and (b) the un-compacted diagram carries its
   larger inflated buffers through the `MacLeodCode` traversal and the eventual
   free. Net ≈ 0. *(I prototyped this behind a flag, measured 3× each way — stock
   128–137k, no-compress 129–132k — and reverted.)*

   Lesson: the Tensor-allocation cost is **not** concentrated in one removable
   compress call; it's spread across the initial/per-pass compresses **and**
   `Canonicalize`'s per-knot `FromMacLeodCode` rebuild. You can't cherry-pick one
   away — it needs buffer *reuse* across all of them (next section).

**Best achievable by *tuning settings* alone: ~+2–5%** (essentially the
`compress_initialQ=false` effect, small-diagrams only). There is **no free
structural win hiding in the compress calls** — confirmed by the experiment above.

## Reusable Tensor workspace — the main remaining lever

The Tensor/PlanarDiagram copy-churn (source #2 above) is the dominant remaining
allocation. Today every `Compress` / `CreateCompressed` / `FromMacLeodCode`
mints a fresh `PlanarDiagram` with freshly allocated, zero-filled `Tensor1`
members. A reusable per-thread scratch (grow-only buffers `clear()`-ed between
uses instead of reallocated) would eliminate most of it.

**Expected payoff — bounded by a measurement we already have.** As an experiment
we swapped in a fast allocator (mimalloc) purely to *measure* — it makes
allocation cheap without changing any logic — and it bought **~1.45–1.5×**
single-thread (and lifted parallel throughput similarly). Since reusable workspace
*eliminates* the allocation rather than merely speeding it, **~1.5× is the proven
ceiling** (likely a touch more — elimination also removes the zeroing that even a
fast allocator pays). So:

- in-source allocation elimination (workspace reuse + the cache fix) should move
  us from the ~1.2× drop-in level toward **~1.4–1.5×**,
- exact gain depends on how completely the workspace covers
  `Compress`/`CreateCompressed`/`FromMacLeodCode`/per-PD-member allocations,
- and it's allocator-agnostic, so it holds on any cluster default.

*Note: the mimalloc figure was a measurement only; per your call we are not
proposing any third-party allocator (mimalloc/tcmalloc/jemalloc) as a dependency.
Its sole value here is bracketing the prize.*

## Secondary: the `string→std::any` cache (~14%)

`Tools::CachedObject` stores everything in a `<std::string, std::any>` map. Per
access: a (often heap-constructed) string key + hash, `std::any` type-erasure
(heap for large values), node allocation. The per-access mutex is already off for
`PlanarDiagram` (`CachedObject<true,false,…>`). For a *fresh, one-shot* diagram
per item the cache never amortizes — it's built and torn down per call.

**Status: largely addressed by the refactor.** `CachedObject` now uses Tools'
`AssociativeContainer`, so with the boost flag its map is `boost::unordered_flat_map`
— folded into the ~+15% above; this is what made the old manual 2-line hack
unnecessary. What remains uncaptured is the `std::any` boxing and the string keys
themselves; a typed/enum-keyed cache would remove those, but it's a larger refactor
touching every `SetCache`/`GetCache` site and is not on the critical path.

## Recommended sequence

1. **Landed (dependency-free, ~+15%):** `-DKNOODLE_USE_BOOST_UNORDERED` (boost
   integer maps + auto boost cache; add `-DTOOLS_USE_BOOST_UNORDERED` for
   `-include` builds — see the build gotcha) + reuse-reapr + `TopologicalNumbering`
   compaction.
2. **Main remaining lever:** reusable Tensor workspace (SBO `Tensor1`) for the
   `Compress` / `CreateCompressed` / `FromMacLeodCode` churn → toward ~1.4–1.5×,
   allocator-free. See the sizing appendix.
3. **Optional:** typed cache (the `std::any` boxing + string keys).

*(Not recommended: skipping the finalization `CreateCompressed` — tested,
throughput-neutral. See compression section.)*

## Sizing appendix — SBO `Tensor1` (the ~1.5× lever)

Idea: treat the existing `Simplify` as a spec and port the *substrate*, not the
algorithm — a small-buffer-optimized `Tensor1` (inline storage up to a fixed
capacity, heap-spill beyond) for the tiny-diagram path. Scan findings that size it:

- **All PlanarDiagram storage bottoms out in one primitive.** The AoS containers
  (`MatrixList_AoS`/`VectorList_AoS`) are thin wrappers over a `Tensor1`, so only
  **one** SBO container is needed; the ~7 PD member aliases re-point through it.
- **Interface to implement is small and known: ~12–15 methods** — `data`, `Size`,
  `Dim`, `Fill`/`SetZero`, `Read`/`Write`(+`Parallel`), `AllocatedByteCount`,
  indexing, ctors, move/copy/swap.
- **No in-place growth (0 `RequireSize`/`Resize` on members).** Storage is sized
  once at construction, so the SBO can be the *simple* fixed-capacity-at-construction
  kind (decide inline-vs-heap once), not the hard dynamically-relocating kind. The
  spill covers the rare oversize automatically → no algorithm fallback path needed.
- **Pointer stability is the one semantic gap.** 112 `.data()` sites, but the
  dangerous pattern (a raw pointer into a PD held across a *move* of that PD) is
  essentially absent (audit): the code uses short-scope pointers on a stable local
  `pd` and moves diagrams only at work-list boundaries. ASan + the KLUT differential
  oracle (`klut_e2e`/`klut_bench`) is the safety net.
- **Object-size tradeoff:** SBO gives every `Tensor1` a *minimum* size = inline
  capacity, which is dead weight when spilled — so keep `InlineCap=0` (today's
  behavior, byte-identical via `[[no_unique_address]]`) as the default and scope the
  nonzero cap to the tiny pipeline (per-build macro on the aliases, or a storage
  template param). Do **not** make it global (bloats the large-diagram path).
- **Effort:** ~1 week for a validated prototype, dominated by the SBO `Tensor1`
  (in the Tensors submodule); plumbing is small; the algorithm is untouched. Gating
  item is the ASan differential run vs the heap build across the KLUT.

## Caveat on the numbers

The benchmark pool is a **proxy**: KLUT minimal diagrams reconstructed from the
shipped key tables, then perturbed by one Reapr reproject into small non-minimal
diagrams of the same knot. The *structural* findings (lookup 0.5%, Simplify 82%,
allocation-dominated) held identically from 11 to
1225 crossings and don't depend on the input distribution. But the *absolute*
throughput and the precise small-crossing regime should be confirmed on real
line-arrangement diagrams; `klut_bench` can be pointed at a file of real PD codes
to make these numbers exact for the production workload.
