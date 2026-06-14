# klut_check — exhaustive KLUT consistency test

*Tiers 1–2, implemented 2026-06-13. `test/klut_check.cpp`.*

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

**All 1,816,748 keys pass** (c=3..13), with zero failures across every check —
0 reconstruction failures, 0 Alexander mismatches, 0 HOMFLY bucket mismatches,
0 chirality mismatches, 0 reader (`FindName`) mismatches. Tier 1 takes ~44 s
(dominated by 1.5M HOMFLY evaluations at c=13); Tier 2 alone is ~0.4 s. The
distinct-knot counts match KnotInfo exactly — e.g. c=12 → 2176 (1288a + 888n),
c=13 → 9988 (4878a + 5110n).

## Usage

```sh
cd test
make klut_check           # needs UMFPACK + BLAS/LAPACK; links vendored libhomfly
./klut_check                          # full table, c=3..13 (Tiers 1+2)
./klut_check --reader-only            # Tier 2 only: whole table in ~0.4 s
./klut_check --no-homfly              # Alexander + reader (skips HOMFLY; fast)
./klut_check --up-to-crossing=10      # quick (sub-second through c=10)
```

Flags: `--data-dir`, `--from-crossing`, `--up-to-crossing`, `--no-alex`,
`--no-homfly`, `--no-reader`, `--reader-only`.

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
