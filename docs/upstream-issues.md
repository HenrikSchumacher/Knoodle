# Upstream issues (src/ — Henrik's territory)

Bugs we've found reading/using the core library. Each needs a minimal repro
before filing. Status: `found` → `confirmed` → `filed` → `fixed upstream`.

## 3. `ArcFaces()` doc comment states the opposite of the implemented convention

**Status:** filed 2026-08-10 — PR #29
(https://github.com/HenrikSchumacher/Knoodle/pull/29, branch
`fix-arcfaces-doc-comment` off origin/main, comment-only change).
**Severity:** documentation only — but this is exactly the comment an API
consumer reads to learn which slot is which face, and it is wrong. Filed
promptly because the planned Doxygen/GitHub-Pages docs site would harvest
it into a silently-wrong API-reference page.

`src/PlanarDiagram/Faces.hpp:34` (doc comment on `ArcFaces()`):

> The convention is that `ArcFaces()(a,1)` is the face to the _right_ of the
> forward arc `ToDarc(a,Head)`.

But the implementation convention, per the comment inside `ComputeFaces`
(same file, "Convention: _Right_ face first ... This way the directed arc
`da = 2 * a + d` has its left face in `dA_f[da]`") and per the data, is:

> `ArcFaces()(a,d)` is the face to the **left** of the darc `2a + d` —
> i.e. `(a,1)` is the **left** face of the forward darc, `(a,0)` its right
> face (= left of the backward darc).

**Evidence (right-hand trefoil `[[0,4,1,3,+],[2,0,3,5,+],[4,2,5,1,+]]`):**
`FaceDarcs()` reports the face with boundary cycle `{3,11,7}` — which by the
face-on-left rule (`Faces.hpp:25`) lies LEFT of darc `11 = ToDarc(5,Head)` —
while `ArcFaces()(5,1)` returns exactly that face and `ArcFaces()(5,0)`
returns the face `{5,10}`. So slot 1 is the left face of the forward darc,
contradicting the `ArcFaces()` doc comment (and agreeing with the
`ComputeFaces` comment).

**Fix:** flip the sentence at `Faces.hpp:34` (or reword to match
`ComputeFaces`). No code change.

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
