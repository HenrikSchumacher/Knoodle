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
- `test/homfly_check.cpp` — C++ driver. Modes:
  - default: self-test — Tier-1 encoding (Knoodle Jenkins → libhomfly vs
    reference values), invariance on the panel knots, and a discrimination
    guard (distinct knots get distinct HOMFLY, so the equality check isn't
    vacuous).
  - `--invariance [files…]`: the invariance test (below). Each input is a
    5-col PD code or a 3-col 3D embedding (auto-detected), or one diagram on
    stdin. Exit nonzero on any HOMFLY change.
  - `--emit-poly`: read a 5-col signed PD code on stdin, print libhomfly terms
    `L_exp M_exp coef` (bridge for the Regina cross-check).
  - `--stress N`: memory-bound regression guard for the gc_shim fix.
  - Uses `homfly()` → `Poly{Term{coef,m,l}}`.
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

## The invariance test (`--invariance`)

HOMFLY is a link invariant, so simplification must not change it. For each
diagram the test compares `homfly(before)` with `homfly(whole simplified
diagram)`. The simplified `PlanarDiagramComplex` is reassembled into one
diagram by `ToSingleDiagram()`, which connect-sums the pieces back together
(and converts anelli → farfalle). That is HOMFLY-faithful for knots and
**non-split** links — no product/δ bookkeeping needed, uniform for knots and
links. (A genuinely split-link input would need the δ^(pieces−1) · ∏ rule from
`run_tests.py:compute_composite_homfly`; all our corpora are non-split, so it's
out of scope. An input that simplifies to the trivial knot → polynomial 1.)

**Validated 2026-06-13:** `homfly_check --invariance` over the full corpus —
**1304 / 1304 diagrams preserved HOMFLY**, 0 changed (1268 link-table links +
36 children's-game 3D embeddings), in ~0.12 s. The children's-game embeddings
are the strongest case: they start from messy projections that require real
simplification down to a minimal knot. The link-table diagrams are minimal but
still pass through full Reapr re-simplification (random re-embedding →
re-projection → canonicalize), so the path is genuinely exercised, not a no-op.

Sensitivity: the before-HOMFLY is exact (252/252 vs Regina) and the polynomial
equality distinguishes distinct knots (the default self-test's discrimination
guard), so a simplification that changed knot type would surface as
before ≠ after.

## Split links and the δ rule

libhomfly's `o_make` **segfaults on split (disconnected) diagrams** — where the
link components don't all connect through shared crossings. It crashes even on
the bare unknot `"1 0"`, and on e.g. Hopf ⊔ unknot. (Connected multi-component
links are fine — the 1268 link-table links and libhomfly's own Borromean
reference all compute.) Multi-component plantri diagrams routinely simplify to
split links, so this matters for tier 2.

`homfly_check` handles them itself with the **split-union δ rule**
(`HomflyOfPossiblySplit`):

1. Parse the Jenkins code; union-find the components into connected pieces
   (joined when they share a crossing).
2. Compute each connected piece separately via libhomfly (renumbering its
   crossings to 0..m−1); a free unknot piece contributes the polynomial 1.
3. Combine: `H = δ^(k−1) · ∏ H(piece)`, where `δ = −(L + L⁻¹)/M` is the HOMFLY
   of the 2-component unlink in libhomfly's (L,M) = Regina homflyLM convention.

Both the input and the simplified diagram may be split; the only remaining
`Unsupported` (→ "skip") case is when `ToSingleDiagram()` cannot produce a
single diagram at all.

**δ-rule validation:**
- δ verified by hand: `δ² = H(3-unlink)`, `δ · H(trefoil) = H(trefoil ⊔ unknot)`.
- Value-level vs Regina: `HomflyOfPossiblySplit` of a split PD (trefoil ⊔
  figure-eight, etc.) matches `regina.homflyLM` term-for-term
  (`homfly_xcheck.py` panel; the default `homfly_check` self-test also checks
  `H(A ⊔ B) == δ·H(A)·H(B)`).
- Verdict-level at scale: tier 2 up to 8 crossings, `--cross-check` → 320/320,
  **0 oracle disagreements** (many split links). Because each input's *before*
  is connected and computed directly, a δ-rule bug in the split *after* would
  surface as homfly_check-changed vs Regina-preserved — none did.

## run_tests.py integration

The HOMFLY-invariance tiers (1a, 1c, 1d, 2) call `homfly_check --invariance`
(machine-readable `RESULT \t pass|fail|skip \t crossings \t label` lines) as
their oracle — no Regina/Python in the default run. Tier 1b (known unknots) is
unchanged: it checks "simplifies to 0 crossings" via the knoodlesimplify CLI
(no HOMFLY — those before-diagrams are far too big to compute). Regina is now
optional (`import` guarded by `REGINA_AVAILABLE`); `--cross-check` runs it
alongside and requires both oracles to agree (catching any disagreement), and
verifies the split links homfly_check skips.

Validated 2026-06-13:
- Full default Tier 1: 2495/2495 (1a 5/5, 1b 1186/1186, 1c 1268/1268, 1d 36/36).
- Tier 2 up to 8 crossings: 320/320 (default, no Regina); `--cross-check`
  320/320 with 0 disagreements (split links handled natively by the δ rule).

## Status

Complete: vendoring, gc shim with bounded memory, Jenkins→libhomfly bridge,
encoding self-test, Regina cross-validation (252/252), the invariance test
(1304/1304 corpus), the **split-union δ rule** for disconnected diagrams
(validated by value vs Regina and by `--cross-check` at scale), and the
`run_tests.py` re-point with Regina behind `--cross-check`. The default test run
needs no Python/Regina and handles knots, non-split links, and split links.
