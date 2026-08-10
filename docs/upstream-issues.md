# Upstream issues (src/ — Henrik's territory)

Bugs we've found reading/using the core library. Each needs a minimal repro
before filing. Status: `found` → `confirmed` → `filed` → `fixed upstream`.

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
