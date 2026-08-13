# Upstream issues (src/ — Henrik's territory)

Bugs we've found reading/using the core library. Each needs a minimal repro
before filing. Status: `found` → `confirmed` → `filed` → `fixed upstream`.

## 9. `LinkEmbedding::Transform` does not invalidate its caches

**Status:** confirmed 2026-08-13, branch `dev_prosector` @ `0a821267`.
**Severity:** high — silent wrong answers. `LinkEmbedding2/3/4` are correct.

`src/LinkEmbedding/VertexCoordinates.hpp:174` rotates `edge_coords` but resets
neither `intersections_computedQ` nor `bounding_boxes_computedQ`, where
`LinkEmbedding2::Transform` resets all three
(`src/LinkEmbedding2/VertexCoordinates.hpp:117-119`). After a `Transform`,
`RequireIntersections()` hands back the stale pre-rotation diagram and
`FindIntersections()` returns 0 because the AABB boxes are stale too; only
`ComputeBoundingBoxes()` first gives the right answer. Measured on a 40-gon
rotated 90° about x: 3 / 3 / 0 / 10 against `LinkEmbedding2`'s 3 → 10.

Nothing in the library hits it since `00a8407` routed Rattle around `Transform`,
but the method is public. Fix = mirror `LinkEmbedding2`. Detail:
[embedding-check.md §5G](embedding-check.md#g-linkembeddingtransform-does-not-invalidate-its-caches).

## 8. `FromInString` aborts on every input (two inverted assertions)

**Status:** confirmed 2026-08-13, branch `dev_prosector` @ `0a821267`.
**Severity:** any build without `-DNDEBUG` cannot read an embedding file at all.

`src/LinkEmbedding2/FromFile.hpp:139` asserts
`component_ptr_agg.size() != color_agg.size()` and line 157 asserts
`!coords_may_followQ`. Both are true-when-they-should-be-false and fire on the
first vertex of any well-formed file, `#color`-tagged or not. `-DNDEBUG` builds
parse correctly, so the parse itself is fine. Full derivation and the
confirm-the-second-one experiment: [embedding-check.md §5F](embedding-check.md#f-frominstring-has-two-inverted-assertions).

## 7. `FromLinkEmbedding(LinkEmbedding2&)` treats every success as an error

**Status:** confirmed 2026-08-13, branch `dev_prosector` @ `0a821267`.
**Severity:** high — the main construction path returns `InvalidDiagram()` for
every successful projection.

`RequireIntersections` returns `bool` (true = success) on `LinkEmbedding2/3/4`
but `int` (0 = success) on `LinkEmbedding`; the two disagree about the meaning
of `1`. `src/PlanarDiagram/FromEmbeddings.hpp:122` still does
`int err = ...; if(err != 0)`, so `true` takes the failure branch. Reproduced on
an ordinary lattice curve: 19 intersections computed, then discarded. Detail:
[embedding-check.md §5A](embedding-check.md#a-planardiagramfromlinkembeddinglinkembedding2-returns-an-invalid-diagram-for-every-successful-projection).

Worth fixing the convention split, not just the caller.

## 6. `LinkEmbedding::RequireIntersections` segfaults on a triple point

**Status:** confirmed 2026-08-13. Possibly the crash noted in `6c63b82a`.
**Severity:** hard crash, no graceful error. `LinkEmbedding2/3/4` are fine.

Three segments projecting through one common point crash the intersection
computation. Six-vertex reproducer committed as
`test/embeddings/deg_triple_point.crd`; bisected to the concurrency itself
(moving any one edge off the point, or widening the z-separation, both fix it).
Detail: [embedding-check.md §5E](embedding-check.md#e-linkembeddingrequireintersections-segfaults-on-a-triple-point).

## 5. Zero-length edges rejected as 3D intersections by `LinkEmbedding2/3/4`

**Status:** confirmed 2026-08-13.
**Severity:** medium — contradicts the class docs, and regresses vs `LinkEmbedding`.

The class documentation lists "line segments that have length 0" as handled;
they are reported as the unfixable 3D-intersection case instead. Inherent: a
zero-length edge always makes its two neighbours share a point of 3-space while
their indices are non-consecutive. A planar square with one duplicated vertex
reproduces it. Fix = collapse zero-length-joined vertices before the 3D test.
Detail: [embedding-check.md §5B](embedding-check.md#b-zero-length-edges-are-rejected-as-3d-intersections).

## 4. `LinkEmbedding2/3/4` do not compile with the 32-bit backend

**Status:** confirmed 2026-08-13. **Severity:** low, but the class docs
recommend the `Real_ = float`, `IReal_ = int32_t` pairing.

`Prosector2` — `call to 'Sign' is ambiguous`, `src/Prosector2/Helpers.hpp:16`
(also 19, 21). `Prosector3`/`Prosector4` — `excess elements in array
initializer`, `src/WideInt.hpp:112`.

## 3. Integral `Real_` is documented but unimplemented

**Status:** confirmed 2026-08-13. **Severity:** low.

`src/LinkEmbedding2/EdgeCoordinates.hpp:71`: *"Let's handle only case 1.2.a) for
now"*, followed by `static_assert(FloatQ<Real>)`. The class docs say `Real_` may
be a signed integral type; that instantiation does not build. Integral input
still gets an exact path via the `input_integralQ` branch, so this is a docs/code
mismatch rather than a functional gap.

## 2. `Alexander_UMFPACK::Alexander` (single-value overload) takes outputs by value

**Status:** found 2026-06-13 (writing `test/inflate_check.cpp`).
**Severity:** the single-value overload silently can't return its result;
callers get uninitialized `mantissa`/`exponent`. The batch overload is fine.

`src/KnotInvariants/Alexander_UMFPACK.hpp:93-100`:

```cpp
template<typename ExtScal, IntQ ExtInt>
void Alexander(
    cref<PD_T> pd,
    ExtScal arg,
    ExtScal mantissa,    // <-- by value: result is discarded
    ExtInt  exponent,    // <-- by value: result is discarded
    bool multiply_toQ
) const
```

`mantissa` and `exponent` are outputs but are passed **by value**, so the
computed values never reach the caller. **Fix:** make them references —
`ExtScal& mantissa, ExtInt& exponent` (matching the by-pointer batch overload
just below, which works). Note the scalar template argument must be **complex**
(`Alexander_UMFPACK<std::complex<double>,Int>`) — the normalization evaluates
the determinant at complex arguments; instantiating with a real `Scal` fails to
compile (`Alexander_Strands_Det` is called with `Complex(...)`).

**Workaround in our code:** `test/inflate_check.cpp` uses the batch overload
`Alexander(pd, args, n, mantissas, exponents, multiply_toQ)` (pointers), which
returns correctly.

## 1. `FromUnsignedPDCode` not migrated to the new `FromPDCode<targs>` API

**Status:** found 2026-06-12 (during origin/main merge). **Severity:** any
caller fails to compile; latent because nothing in-tree instantiates it.

`src/PlanarDiagram/PDCode.hpp:408`:

```cpp
template<bool checksQ = true, IntQ T, IntQ ExtInt>
static PD_T FromUnsignedPDCode( ... )
{
    return FromPDCode<false,false,checksQ>(   // old-style bool template args
        pd_code, crossing_count, proven_minimalQ_, compressQ
    );
}
```

`FromPDCode` now takes `template<FromPDCode_TArgs_T targs, IntQ T, IntQ
ExtInt>` (same file, line 546), so `false` doesn't match the aggregate
non-type parameter and instantiation fails. The sibling `FromSignedPDCode`
(line 371) *was* migrated:

```cpp
return FromPDCode<{.signQ = true, .colorQ = false, .checksQ = checksQ}>( ... );
```

**Fix (one line):**

```cpp
return FromPDCode<{.signQ = false, .colorQ = false, .checksQ = checksQ}>(
    pd_code, crossing_count, proven_minimalQ_, compressQ
);
```

**How we hit it:** `tools/knoodle_io.hpp` called `FromUnsignedPDCode` for
4-column input; after merging origin/main (commit 8264efa "Made it easier to
export and import PD codes") the build broke. We worked around it in our
territory by calling `FromPDCode<{.signQ=false,.colorQ=false}>` directly
(`tools/knoodle_io.hpp`, `CreateDiagramFromPDCode` case 4).

**Repro for the PR:** one-line TU:
`auto pd = Knoodle::PlanarDiagram<int64_t>::FromUnsignedPDCode(ptr, n);`
fails to compile before the fix, compiles after.
