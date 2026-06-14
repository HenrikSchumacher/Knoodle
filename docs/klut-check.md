# klut_check — exhaustive KLUT consistency test

*Tiers 1–3, implemented 2026-06-13. `test/klut_check.cpp`.*

## What it checks

For **every** key in the KLUT (`data/Klut/Klut_Keys_NN.bin`, mapped to its name
via `Klut_Values_NN.tsv`):

**Tier 1 — invariant consistency.** Reconstruct the diagram from its MacLeod
code (`PlanarDiagram::FromMacLeodCode`) and verify three relationships, using
only the table itself, no external reference:

- **(A) Alexander agrees within a knot.** All keys whose name shares `(c, i, j)`
  have the same Alexander value. Alexander is mirror/reverse-invariant, so every
  symmetry coset of one knot agrees. Catches a key in the wrong knot's bucket.
- **(B) HOMFLY agrees within a name bucket.** All keys with the exact same name
  `K[c,i,j,"coset"]` have the same HOMFLY. Catches a key in the wrong coset
  (e.g. a mirror filed with its non-mirror sibling — same Alexander, different
  HOMFLY).
- **(C) HOMFLY honors chirality across a knot's buckets.** For one `(c,i,j)`,
  every bucket's HOMFLY equals or is the mirror of a reference (mirror sends
  `L → L⁻¹`, i.e. `(l,m) → (-l,m)`): `{e,r}` share a HOMFLY, `{m,mr}` share its
  mirror, amphichiral knots are palindromic.

Invariants: Alexander via `Alexander_UMFPACK` (fast, knots-only) and HOMFLY via
the vendored libhomfly (`KnoodleJenkins`). KLUT keys are knots, so both apply.

**Tier 2 — reader correctness.** `Klut::FindName(key)` must return the same
name the value file assigns to that key. This exercises Henrik's hash-table
loader and lookup in `src/Klut.hpp` (the `Subtable` load, `MacLeodCodeToKey`
packing, and the associative-container lookup) — directly, with no
reconstruction, so it runs in a fraction of a second over the whole table.

**Tier 3 — KnotInfo label cross-check** (opt-in, `--knotinfo=FILE`). Build
reference invariants for every ≤13-crossing knot from KnotInfo's own
`pd_notation` (using the *same* Alexander + libhomfly tooling, so the
comparison is exact, no convention mismatch), then verify each KLUT name:
- the knot's Alexander matches KnotInfo's `(c,i,j)` knot (proves it's the
  *right* knot, not just internally consistent);
- each bucket's HOMFLY matches KnotInfo's with the chirality its coset implies
  (`e/r` → as-is, `m/mr` → mirror; amphichiral → either) — proves the coset
  labels are right;
- **coverage**: every KLUT name is a real KnotInfo knot, and every KnotInfo
  knot ≤13 crossings appears in the table.

The `(c,i,j)` mapping (KnotInfo `"N_i"` for c≤10 / `"Na_i"`,`"Nn_i"` for c≥11)
also independently re-confirms that `j` is the alternating flag. Needs the
KnotInfo TSV from the package snapshot, so Tier 3 is opt-in; Tiers 1–2 run from
repo data alone.

## The name format: `K[c, i, j, "coset"]`

**Corrected here** (our earlier notes had `j` wrong): a knot is identified by
**three** fields — `c` = crossing number, `i` = index, **`j` = the alternating
flag** (1 = alternating, 0 = non-alternating). `j` is *not* amphichirality:
`12a_i` and `12n_i` are different knots that share `i` and differ only in `j`.
The earlier "j = amphichirality" claim was wrong — `klut_check` caught it,
because grouping by `(c,i)` alone conflated alternating with non-alternating
knots and produced ~200k spurious Alexander mismatches; grouping by `(c,i,j)`
gives 0. (Confirmed independently: KnotInfo's column 11 is "alternating", and
the per-crossing distinct-knot counts then match KnotInfo exactly.) The
`"coset"` field is the symmetry class over {e, r, m, mr}, grouping the variants
that are the same knot (trefoil = `e/r` + `m/mr`; figure-eight = `e/m/r/mr`).

## Result

**All 1,816,748 keys pass** (c=3..13), with zero failures across **every** check
in all three tiers — 0 reconstruction failures; 0 Alexander, HOMFLY-bucket and
chirality mismatches (Tier 1); 0 reader `FindName` mismatches (Tier 2); 0
KnotInfo Alexander/HOMFLY mismatches, **0 KLUT-knots-not-in-KnotInfo, and 0
KnotInfo-knots-not-in-KLUT** (Tier 3). So the labels are provably the correct
KnotInfo knots with the right chirality, and the table has **complete coverage**
of all prime knots ≤13 crossings. The full run is ~44 s (dominated by 1.5M
HOMFLY evaluations at c=13); Tier 2 alone is ~0.4 s; the KnotInfo reference (~13k
knots) builds in a few seconds. Distinct-knot counts match KnotInfo exactly —
c=12 → 2176 (1288a + 888n), c=13 → 9988 (4878a + 5110n).

## Usage

```sh
cd test
make klut_check           # needs UMFPACK + BLAS/LAPACK; links vendored libhomfly
./klut_check                          # full table, c=3..13 (Tiers 1+2)
./klut_check --knotinfo=/path/to/knotinfo_data_complete.tsv   # add Tier 3
./klut_check --reader-only            # Tier 2 only: whole table in ~0.4 s
./klut_check --no-homfly              # Alexander + reader (skips HOMFLY; fast)
./klut_check --up-to-crossing=10      # quick (sub-second through c=10)
```

Flags: `--data-dir`, `--knotinfo=FILE`, `--from-crossing`, `--up-to-crossing`,
`--no-alex`, `--no-homfly`, `--no-reader`, `--reader-only`.

## Scope and next tiers

Tiers 1–2 prove the table is **internally consistent** and that its **reader
returns the right name** — but neither can catch a whole knot being mislabeled
the *same* wrong way in every one of its keys (a systematic label error against
the outside world). The remaining follow-ups close that gap:

- **Tier 3 — KnotInfo label cross-check:** reference invariants from KnotInfo
  `pd_notation` confirm each name's `(c,i,j)` is the *correct* knot with the
  right chirality for its coset (validates labels absolutely; a handful of
  HOMFLY-degenerate ≤13-crossing pairs would need noting).
- **Tier 4 — end-to-end pipeline:** reconstruct → `knoodlesimplify |
  knoodleidentify` → same name (sampled).
