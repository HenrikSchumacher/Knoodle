# Upstream issues (src/ — Henrik's territory)

Bugs we've found reading/using the core library. Each needs a minimal repro
before filing. Status: `found` → `confirmed` → `filed` → `fixed upstream`.

**Numbers are permanent ids: never reuse one, even after the issue closes.**
(They were reused once already — see the note at the bottom.)

**Scoreboard, re-verified 2026-08-14 against `dev_prosector` `fb4c8f0e`** with
standalone reproducers, not through a test harness:

| # | issue | status |
| --- | --- | --- |
| 10 | `LinkEmbedding3/4` class docs promise a path into `PlanarDiagram` that is not built yet | **docs ahead of code**, not a defect |
| 9 | `Transform` does not invalidate its caches | fixed — `8db4cdc4` |
| 8 | `FromInString` aborts on every input | fixed — `8db4cdc4` |
| 7 | `FromLinkEmbedding(LinkEmbedding2&)` treats success as error | fixed — `1d8761c7` + `34cd0fc3` |
| 6 | `RequireIntersections` segfaults on a triple point | fixed — `d26c301f` |
| 5 | zero-length edges rejected as 3D intersections | **open** |
| 4 | `LinkEmbedding2/3/4` do not compile with the 32-bit backend | **half fixed** |
| 3 | integral `Real_` documented but unimplemented | fixed — `4d2d0624` |
| 2 | `Alexander` single-value overload takes outputs by value | fixed upstream |
| 1 | `FromUnsignedPDCode` not migrated to `FromPDCode<targs>` | fixed upstream |

## 10. `LinkEmbedding3`/`LinkEmbedding4` docs promise a `PlanarDiagram` path that is not built yet

**Status:** NOT A BUG — planned functionality that is not built yet. Confirmed
with Henrik 2026-08-14: both classes are still marked EXPERIMENTAL, he intends
to add the `FromLinkEmbedding` overloads once they are fully tested, and
`FromLinkEmbedding_Raw` is the intended route until then.

So this is a **TODO rather than a defect**: it does not sit on the
`found -> confirmed -> filed -> fixed upstream` ladder and should not be filed
anywhere. What is left of it is narrow and familiar — **the documentation is
ahead of the code**, which is exactly the shape issue 3 had ("integral `Real_`
is documented but unimplemented"). Either the overloads land, or the class
docs gain a "not yet" until they do; the code is doing nothing wrong.

Recorded only because the question falls out of issue 7 naturally and deserves
answering once rather than each time.

Their class docs open with: *"…computing the crossings. Then it can be handed
over to class `PlanarDiagram` or `PlanarDiagramComplex`."* There is no
`FromLinkEmbedding` overload taking either one:

```
PD_T::FromLinkEmbedding(le2)  -> compiles
PD_T::FromLinkEmbedding(le3)  -> error: no matching function for call to 'FromLinkEmbedding'
PD_T::FromLinkEmbedding(le4)  -> error: no matching function for call to 'FromLinkEmbedding'
```

`src/PlanarDiagram/FromEmbeddings.hpp` has exactly two overloads taking an
embedding object — `LinkEmbedding` and `LinkEmbedding2`. The only route for
3 and 4 is `FromLinkEmbedding_Raw`, whose own comment reads *"For internal use
only. Users should not call this. Testing makes it necessary to make this
public."* — and which is precisely what `embedding_check` calls, which is why
neither this nor issue 7 was noticed by the test suite.

**Worth passing on when the overloads do land:** since `4d2d0624` all three
share one `src/LinkEmbedding_Int/` implementation, so one overload templated
over that family beats two more copies — copies are how issue 7 happened, a
per-overload convention mistake in exactly this spot.

**How we hit it:** asking why issue 7 did not affect 3 and 4. The answer was
not "they are safe" but "they are not wired up yet" — which also explains why
`embedding_check` calls `FromLinkEmbedding_Raw`: at the time it was written,
that was the only thing to call.

## 9. `LinkEmbedding::Transform` does not invalidate its caches

**Status:** FIXED UPSTREAM in `8db4cdc4` (2026-08-14). Verified: the 40-gon
measurement below now reads 3 -> 7, 7, 7 where it read 3 / 3 / 0 / 10.
Confirmed 2026-08-13, branch `dev_prosector` @ `0a821267`.
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

**Status:** FIXED UPSTREAM in `8db4cdc4` (2026-08-14). Verified: both readers
parse with assertions enabled.
Confirmed 2026-08-13, branch `dev_prosector` @ `0a821267`.

**Correction to this entry as originally written:** the assertions were
duplicated in TWO readers, and the one that actually aborted was
`src/LinkEmbedding/FromFile.hpp:141`, not the `LinkEmbedding2` copy cited
below. A patch against only the cited file would have left the other
aborting. Grep for duplicates before filing a location.
**Severity:** any build without `-DNDEBUG` cannot read an embedding file at all.

`src/LinkEmbedding2/FromFile.hpp:139` asserts
`component_ptr_agg.size() != color_agg.size()` and line 157 asserts
`!coords_may_followQ`. Both are true-when-they-should-be-false and fire on the
first vertex of any well-formed file, `#color`-tagged or not. `-DNDEBUG` builds
parse correctly, so the parse itself is fine. Full derivation and the
confirm-the-second-one experiment: [embedding-check.md §5F](embedding-check.md#f-frominstring-has-two-inverted-assertions).

## 7. `FromLinkEmbedding(LinkEmbedding2&)` treats every success as an error

**Status:** FIXED, in two steps. `1d8761c7` unified the conventions on `int`
(the better fix: it removes the split rather than patching one caller), and
`34cd0fc3` finished the conversion — `LinkEmbedding_Int::RequireIntersections`
still ended in `return true`, which in an `int` function is 1, so success kept
reporting an error; and `FromKnotEmbedding`'s `eprint` still named a deleted
`err`, a non-dependent name that stopped every translation unit including
`Knoodle.hpp` from compiling. See also issue 10: 3 and 4 were never affected
only because they are unreachable.
Confirmed 2026-08-13, branch `dev_prosector` @ `0a821267`.
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

**Status:** FIXED UPSTREAM in `d26c301f` (2026-08-14). It no longer crashes; it
returns error code 8, having detected two intersection times too close to order,
and declines — correct behaviour for a float class on a degenerate projection.
Confirmed 2026-08-13. Possibly the crash noted in `6c63b82a`.
**Severity:** hard crash, no graceful error. `LinkEmbedding2/3/4` are fine.

Three segments projecting through one common point crash the intersection
computation. Six-vertex reproducer committed as
`test/embeddings/deg_triple_point.crd`; bisected to the concurrency itself
(moving any one edge off the point, or widening the z-separation, both fix it).
Detail: [embedding-check.md §5E](embedding-check.md#e-linkembeddingrequireintersections-segfaults-on-a-triple-point).

## 5. Zero-length edges rejected as 3D intersections by `LinkEmbedding2/3/4`

**Status:** OPEN, re-verified 2026-08-14 at `fb4c8f0e` — still reproduces on all
three classes at both coordinate types. Acknowledged upstream in `4d2d0624`:
*"ProsectorX does handle degenerate edges well, but not their neighbors. Will
have to work on this."*
Confirmed 2026-08-13.
**Severity:** medium — contradicts the class docs, and regresses vs `LinkEmbedding`.

The class documentation lists "line segments that have length 0" as handled;
they are reported as the unfixable 3D-intersection case instead. Inherent: a
zero-length edge always makes its two neighbours share a point of 3-space while
their indices are non-consecutive. A planar square with one duplicated vertex
reproduces it. Fix = collapse zero-length-joined vertices before the 3D test.
Detail: [embedding-check.md §5B](embedding-check.md#b-zero-length-edges-are-rejected-as-3d-intersections).

## 4. `LinkEmbedding2/3/4` do not compile with the 32-bit backend

**Status:** HALF FIXED at `fb4c8f0e`. The `Prosector2` `Sign` ambiguity is gone;
`src/WideInt.hpp:114` "excess elements in array initializer" remains, and the
`__int128` work of 2026-08-14 ends in a `Rollback`. May close by narrowing the
docs instead: `4d2d0624` notes `int32_t` as `IReal` is problematic anyway, and
"we want Int64 here anyways for performance reasons".
Confirmed 2026-08-13. **Severity:** low, but the class docs
recommend the `Real_ = float`, `IReal_ = int32_t` pairing.

`Prosector2` — `call to 'Sign' is ambiguous`, `src/Prosector2/Helpers.hpp:16`
(also 19, 21). `Prosector3`/`Prosector4` — `excess elements in array
initializer`, `src/WideInt.hpp:112`.

## 3. Integral `Real_` is documented but unimplemented

**Status:** FIXED UPSTREAM in `4d2d0624` (2026-08-14). Verified: the i64
instantiation compiles. Leaves a small job — `-DKNOODLE_TEST_INTEGER_COORDS`
could now be on by default in `embedding_check`.
Confirmed 2026-08-13. **Severity:** low.

`src/LinkEmbedding2/EdgeCoordinates.hpp:71`: *"Let's handle only case 1.2.a) for
now"*, followed by `static_assert(FloatQ<Real>)`. The class docs say `Real_` may
be a signed integral type; that instantiation does not build. Integral input
still gets an exact path via the `input_integralQ` branch, so this is a docs/code
mismatch rather than a functional gap.

## 2. `Alexander_UMFPACK::Alexander` (single-value overload) takes outputs by value

**Status:** FIXED UPSTREAM. Verified 2026-08-14 at `fb4c8f0e`: the signature is
now `int Alexander(cref<PD_T>, ExtScal, mref<ExtScal>, mref<ExtInt>, bool)` and
it returns the right number — a trefoil at t = e^(i*pi/4) gives 0.414214, i.e.
sqrt(2) - 1, which is Delta(t) = t - 1 + 1/t there.

**Correction to the workaround note below: there was never a workaround to
retire.** Both of our call sites -- `test/inflate_check.cpp` and
`test/klut_check.cpp` -- evaluate the polynomial at FIVE arguments in one call,
so the batch overload is simply the right one and would stay the right one
even if the single-value overload had always worked. What was stale was this
file's description of that call as a workaround, not the call.
Found 2026-06-13 (writing `test/inflate_check.cpp`).
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

**Status:** FIXED UPSTREAM. Verified 2026-08-14 at `fb4c8f0e`: it forwards
`FromPDCode<{.signQ = false, .colorQ = false, .checksQ = checksQ}>`, compiles,
and builds a valid trefoil. **Workaround retired 2026-08-14**: `tools/knoodle_io.hpp`'s `case 4` calls
`FromUnsignedPDCode` again, alongside `FromSignedPDCode` in `case 5` as it was
always meant to. Verified: 4-column input draws identically to the 5-column
form.
Found 2026-06-12 (during origin/main merge). **Severity:** any
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

---

## Note: the numbering was reused once

Issue **3** names two different things depending on which branch you read.
`orthodecorate` has "`ArcFaces()` doc comment states the opposite of the
implemented convention" (filed as PR #29, merged 2026-08-10, and correct on
main since); `dev_prosector` has "integral `Real_` is documented but
unimplemented". The two entries were written on branches that could not see
each other, and both took the next free number.

Nothing is lost — the ArcFaces entry is resolved and lives on `orthodecorate`
— but a merge of the two branches will produce two `## 3.` headings, and any
reference to "upstream issue 3" is ambiguous. When those branches meet,
renumber the ArcFaces entry to **11** rather than reusing 3.

The general fix, if this file keeps growing: file real GitHub issues and let
the tracker own the numbering, keeping this file as an index of what was filed.
GitHub numbers are repo-unique and permanent, and cannot diverge per branch.
Cite code with commit-SHA permalinks rather than branch-name URLs, which rot —
several line numbers in this file had already drifted by the time the fixes
landed.
