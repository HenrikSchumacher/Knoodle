# HOMFLY oracle for the correctness tests — libhomfly (vendored)

*Implemented 2026-06-13. Replaces the hard Regina/Python dependency in the
HOMFLY-invariance test with a vendored, dependency-free C library; Regina is
kept only as an optional cross-check.*

## Why

The HOMFLY-invariance tests check that `knoodlesimplify` preserves knot type
(HOMFLY before == HOMFLY of the connect-sum of summands after). That tests
Henrik's core `PlanarDiagramComplex::Simplify`, with HOMFLY as an *independent*
oracle. The oracle was Regina (Python) — a heavy install (large C++ lib + venv
+ pip). Goal: a self-contained, no-Python oracle.

## What we vendored

[libhomfly](https://github.com/miguelmarco/libhomfly) — Miguel Marco's
library-ification of Robert Jenkins' HOMFLY program. Vendored under
`test/vendor/libhomfly/` (~2500 lines of C).

- **License: public domain** (the Unlicense; Jenkins' original was public
  domain and Marco kept it). No compatibility concern, vendor freely.
- **Dependency removed:** upstream needs Boehm GC (`<gc.h>`). GC use is shallow
  (`GC_INIT` ×1, `GC_MALLOC` ×11, `GC_REALLOC` ×1), so the vendored
  `gc_shim.{h,c}` replaces it and the copy builds against libc only.
- **Memory bounded across calls (fixed 2026-06-13).** libhomfly relied on the
  collector to free its working set, so a naive `malloc` shim would leak per
  `homfly()` call. Instead `gc_shim.c` is a *tracked* allocator: every block is
  registered (with an O(1) header-index so `realloc` updates its slot), and
  `knoodle_gc_free_all()` reclaims everything. The driver calls it after
  copying out each result. Safe because libhomfly keeps **no** allocated state
  across calls — its file-scope global polynomials (`llplus` … in `control.c`)
  are re-created every call by `c_init()` → `p_term` → `p_add`, which
  overwrites `.term` with a fresh allocation without reading the freed pointer.
  - Verified by `homfly_check --stress N`: 200k figure-eight HOMFLY calls grow
    peak RSS by < 0.1 MiB (total ~1.8 MiB). The guard is sensitive — neutering
    `free_all` makes the same run climb to ~140 MiB at 50k and the test fails.
    Single-threaded only (the registry is a global).

## The bridge: Knoodle Jenkins code == libhomfly input

The reason this is clean: the core library already emits libhomfly's exact
input format. `PlanarDiagram::ToJenkinsCodeString()` (`src/PlanarDiagram/
JenkinsCode.hpp`, added upstream in "PD code and Jenkins code for
PlanarDiagramComplex") produces:

```
[#components]
[#crossings] [crossing-id, +1 over / -1 under] ...   (per component)
[crossing-id, +1 right / -1 left] ...                (handedness, all crossings)
```

which is precisely what `homfly()`/`homfly_str()` parse. Knoodle's trefoil
Jenkins string is **byte-identical** to libhomfly's own reference trefoil input
— Henrik clearly built Jenkins code for this. So there is **no hand-written
conversion layer** (the part that would normally be the bug-prone risk); the
oracle feed is core library code.

## Convention: libhomfly (L, M) ≡ Regina homflyLM (l, m), exactly

Verified empirically (2026-06-13): no substitution, no sign flips, term for
term. Trefoil: libhomfly `-L^-4 -2L^-2 +M^2L^-2` = Regina `x^-2 y^2 -2x^-2
-x^-4`. So cross-checking is just: rebuild libhomfly's term list as a
`regina.Laurent2` (via `set(l, m, coef)`) and compare with `==`.

## Result — cross-validation against Regina

`test/homfly_xcheck.py` ran the panel **trefoil + figure-eight + 250 real
link-table diagrams**: **252 / 252 agreed exactly** with Regina's `homflyLM`.
The trefoil (chiral) confirms chirality is handled consistently; the 250 links
exercise multi-component diagrams. The Jenkins→libhomfly oracle is trustworthy.

(One panel entry — a "mirror trefoil" made by flipping the sign column — was
dropped: that does not yield a valid PD, and Knoodle correctly rejects it.
Mirrors need a real chirality transform, not a sign flip.)

## Files

- `test/vendor/libhomfly/` — vendored sources + `gc_shim.h`, `LICENSE`,
  `README.upstream.md`, `reference_data.txt` (upstream's own reference polys).
- `test/homfly_check.cpp` — C++ driver. Default: Tier-1 encoding self-test
  (Knoodle Jenkins → libhomfly vs reference values). `--emit-poly`: read a
  5-col signed PD code on stdin, print libhomfly terms `L_exp M_exp coef`
  (bridge for the Regina cross-check). `--stress N`: memory-bound regression
  guard for the gc_shim fix. Uses `homfly()` → `Poly{Term{coef,m,l}}`.
- `test/homfly_xcheck.py` — optional Regina cross-check (needs venv + Regina).
- `test/Makefile` — builds `homfly_check` (+ `key_roundtrip_probe`); compiles
  the vendored C to `build/` objects, links into the C++ driver.

## How to run

```sh
cd test
make homfly_check
./homfly_check                          # Tier-1 encoding self-test
./venv/bin/python homfly_xcheck.py --samples 250   # optional Regina agreement
```

## Status / next steps

- **Done:** vendoring, gc shim with bounded memory (tracked allocator +
  `free_all`, verified by `--stress`), Jenkins→libhomfly bridge, Tier-1
  encoding self-test, exact Regina cross-validation (252/252).
- **Next:** build the pure-C++ invariance test on top of `homfly_check` —
  read diagrams, `Simplify`, compare `homfly(before)` to the product of
  `homfly(summandᵢ)` (HOMFLY is multiplicative under connect sum; unknot
  summands = 1). Then re-point `run_tests.py`'s small-diagram tiers (1a, 1c, 2)
  at it, keeping Regina behind a `--cross-check` flag. (Large unknot tiers test
  "simplifies to the unknot", not HOMFLY — both before-diagrams are too big to
  HOMFLY, same as today.)
