# Upstream issues (src/ — Henrik's territory)

Bugs we've found reading/using the core library. Each needs a minimal repro
before filing. Status: `found` → `confirmed` → `filed` → `fixed upstream`.

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
