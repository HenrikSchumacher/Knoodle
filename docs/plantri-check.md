# plantri_check — exhaustive generator-based HOMFLY-invariance test

*Implemented 2026-06-14. `test/plantri_check.cpp`. On local `main`.*

## What it checks

Simplification must not change a diagram's knot/link type, so it must not change
its HOMFLY polynomial. `plantri_check` enumerates **every** small planar diagram
with the vendored `plantri` and verifies `HOMFLY(D) == HOMFLY(Simplify(D))` on
each — knots *and* links.

The pipeline is a self-contained C++ port of `test/run_tests.py`'s Tier 2:

```
plantri -E V  ->  edge_code graphs (planar quadrangulations)
   -> quad_to_shadow:   dual of the quadrangulation = a 4-valent graph (shadow)
   -> assign + trace:   each of the 2^n over/under assignments -> a signed PD code
   -> CheckInvariance:  HOMFLY(diagram) vs HOMFLY(Simplify(diagram))
```

For crossing number `n`, plantri is asked for `V = n + 2` vertices. The dual of
a planar quadrangulation is a 4-valent graph — exactly a knot/link **shadow**;
its 2ⁿ crossing sign assignments give all the diagrams on that shadow.

## The oracle

HOMFLY is computed by the vendored, public-domain **libhomfly**, fed by the
core library's own `PlanarDiagram::ToJenkinsCodeString()`. The oracle —
diagram → Jenkins → libhomfly, the split-link **delta rule**
(`H(A ⊔ B) = δ·H(A)·H(B)`, since libhomfly only handles connected diagrams), and
the `Simplify`-preserves-HOMFLY check — lives in `test/homfly_invariance.hpp`,
**shared** with `homfly_check.cpp`. So `plantri_check` uses the exact oracle
that `homfly_check` cross-validates against libhomfly's published reference
polynomials. No Python, no Regina, no `tools/` — just the library, vendored
libhomfly, and vendored plantri.

Per diagram the result is **preserved** (pass), **changed** (a genuine
simplification bug → hard failure, exit nonzero), or **skipped** (the simplified
complex can't be reassembled into one diagram for the oracle — rare).

## plantri modes

`V = n + 2`; `-E` is plantri's binary edge_code output.

| mode (`--plantri-mode`) | command | graphs at V=8 |
|---|---|---|
| `everything` (default) | `plantri -Q -E V` (all quadrangulations) | 733 |
| `no-r1` | `plantri -Q -c2 -E V` (no length-2 cycles) | 17 |
| `simple` | `plantri -q -E V` (simple quads; V even, ≥ 8) | 1 |

`everything` is the broad-coverage mode (and what reproduces the historical
counts); `simple` matches the documented KLUT-generation invocation but only
reaches crossing number ≥ 6 (one graph). This also pins down the repo's
long-standing open question — *which plantri command line builds the diagrams* —
to these three.

## Vendoring

`test/vendor/plantri/plantri.c` is plantri **5.8 (March 4 2026)**, unmodified,
under the **Apache License 2.0** (`LICENSE-2.0.txt`; see also
`README.upstream.md`). plantri is one translation unit (C stdlib only), built by
the `plantri` target in `test/Makefile`. Earlier plantri releases were not
redistributable; the 5.8 relicensing to Apache 2.0 is what makes this vendoring
legitimate.

## Result

**All diagrams pass** — `Simplify` preserved HOMFLY on every one, with no
changes, in `everything` mode:

| up to | diagrams | (knots / links) | time |
|---|---|---|---|
| 6 crossings | 51,428 | 26,856 / 24,572 | ~0.9 s |
| 7 crossings | +587,008 | 280,832 / 306,176 | ~11 s |

Diagram counts match `run_tests.py`'s Tier 2 exactly (V=4: 3 graphs, V=5: 7,
V=6: 30, V=7: 124, V=8: 733), confirming the C++ converter is a faithful port.

**Historical note.** As of 2026-03-24, Tier 2 had **8 known failures** up to 6
crossings — all 4-component links from graph g510 at V=8 — then under
investigation in `knoodlesimplify`. They are **gone**: both this C++ test and
the Python Tier 2 now report 0 failures over all 51,428 diagrams, so the
underlying simplification bug was fixed in the interim.

## Usage

```sh
cd test
make plantri_check          # builds vendored plantri + the test (no UMFPACK)
./plantri_check                              # everything mode, c=3..6 (~0.9 s)
./plantri_check --up-to-crossing=7           # +7-crossing (~11 s)
./plantri_check --knots-only                 # skip link diagrams
./plantri_check --plantri-mode=no-r1         # fewer graphs, no R1 pairs
./plantri_check --crossing-assignments=8     # sample 8 random sign masks/shadow
```

Flags: `--plantri`, `--plantri-mode`, `--crossing-assignments`,
`--from-crossing`, `--up-to-crossing`, `--knots-only`, `--seed`. Exit code is
nonzero iff some diagram's HOMFLY changed under simplification.
