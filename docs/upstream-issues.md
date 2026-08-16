# Upstream issues (src/ — Henrik's territory)

Bugs we've found reading/using the core library. Each needs a minimal repro
before filing. Status: `found` → `confirmed` → `filed` → `fixed upstream`.

**Numbers are permanent ids: never reuse one, even after the issue closes.**
(They were reused once already — see the note at the bottom.)

> **Pending action, 2026-08-19.** Issues **11**, **5** and **4** are still open
> and have not moved. If they are still unmoved on that date, we will file them
> as GitHub issues, for the reason recorded in this file's own workflow note:
> *this document is an index, not the record.* An entry here is easy to lose to
> history; a GitHub issue is not, and can be closed with a SHA the way #33 and
> #34 were. Filing them is not an escalation — it is the same treatment every
> confirmed issue here has had, just applied on a timer so nothing quietly
> ages out.

**Scoreboard, re-verified 2026-08-14 against `dev_prosector` `fb4c8f0e`** with
standalone reproducers, not through a test harness:

| # | issue | status |
| --- | --- | --- |
| 16 | `tools/knoodle_io.hpp` still parses with `getline`/`istringstream`/`stod`; `Tools::InString` + `from_chars` is 5.5x faster on coordinate input | **ENHANCEMENT — [GH #36](https://github.com/HenrikSchumacher/Knoodle/issues/36)** (ours to do, not Henrik's) |
| 15 | `ScopedUnlock` is documented in both `PlanarDiagram` and `PlanarDiagramComplex` but defined nowhere, so the documented spelling does not compile | **PR'd — [GH #35](https://github.com/HenrikSchumacher/Knoodle/pull/35)** |
| 14 | `PlanarDiagramComplex(const PD_T &)` delegates to a `PD_T &&` constructor, so the overload cannot be instantiated | **PR'd — [GH #35](https://github.com/HenrikSchumacher/Knoodle/pull/35)** |
| 13 | `IntersectionType()` misses a transversal 3-space intersection; LE2 succeeds silently, LE3/4 throw from an accessor | **FIXED — `b53a9ecb`; [GH #34](https://github.com/HenrikSchumacher/Knoodle/issues/34) closed 2026-08-15** |
| 12 | `ArcSimplifier`'s R_IIa is not atomic: on a locked diagram it half-applies and CHANGES THE KNOT | **FIXED — `e4ebccc3`; [GH #33](https://github.com/HenrikSchumacher/Knoodle/issues/33) closed 2026-08-15** |
| 11 | `LinesColinearTest` asserts that two distinct degenerate segments coincide | **open — aborts** |
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

## 13. A transversal 3-space intersection is not detected by `IntersectionType()`

**Status:** FIXED by Henrik in `b53a9ecb` ("Fixed a bug in Prosector classes
that allowed 3D intersections to go unnoticed"), 2026-08-15.
[GitHub issue #34](https://github.com/HenrikSchumacher/Knoodle/issues/34) closed
the same day. Both halves went: LinkEmbedding2 no longer succeeds silently, and
LinkEmbedding3/4 no longer throw out of the accessor -- all four refuse and
report the count. `test/intersection3d_check` announced it as 24 XPASS and its
`kDetection3DIsBroken` flag is now `false`.

Filed 2026-08-14. No patch was proposed; where the check belonged was Henrik's call.
These are the experimental classes and this is what debugging them looks like,
so it is recorded rather than escalated.

Four vertices reach it — a quadrilateral whose two non-adjacent edges cross at
(2,2,0):

```
0 0 0 / 4 4 0 / 4 0 0 / 0 4 0
```

`LinkEmbedding2` reports success with `IntersectionCount3D() = 0`, on both
`main` and `dev_prosector` (the return conventions differ, the answer does not).
`LinkEmbedding3/4` on `dev_prosector` notice and throw a `std::runtime_error`
out of `IntersectionCount3D()`, from an internal check that says
`IntersectionType()` should have returned `Flag_T::Error` and did not.

Detection works for the shared-endpoint shape (`deg_zero_length` is correctly
reported, code 3); what is missed is a transversal crossing at interior points.
Not a planarity artifact — a non-flat curve with the same single crossing
behaves the same.

**The operational point (JHC):** random knot generation will eventually produce
a curve that genuinely self-intersects, or is numerically indistinguishable from
one. That path should end in a declined diagram, not an exception out of an
accessor. `LinkEmbedding2`'s silent success is the other side of it.

Covered by `test/intersection3d_check`, all XFAIL behind one flag so it reports
XPASS when this changes. `embedding_check` asserts the converse everywhere.

## 12. `ArcSimplifier` R_IIa half-applies on a locked diagram and changes the knot type

**Status:** FIXED by Henrik in `e4ebccc3`, 2026-08-15 -- ArcSimplifier's
SwitchCrossing redirected to `PlanarDiagram::SwitchCrossing_Private`, which is
not lock-guarded, so R_IIa no longer half-applies.
[GitHub issue #33](https://github.com/HenrikSchumacher/Knoodle/issues/33) closed
the same day. `test/local_moves_check` reported Monster@level-4 as XPASS and its
known-failure list is now empty.

Filed 2026-08-14, deliberately with no PR: the fix turned on which operations the
locking feature was meant to guard, which was Henrik's design call, not ours.
The analysis below is kept because the mechanism is worth remembering -- it is
the non-atomic-composite-mutation shape, and the corpus gap that let it survive
is why `test/polygon_levels_check` exists.
Regression test `test/local_moves_check` is on **both** `main` (`5c7d64a4`) and
`dev_prosector`, recording Monster@level-4 as a known failure so it reports
XPASS the moment this is fixed.
**Severity:** high — a shipped tool silently returns a different knot and
exits 0.

`knoodlesimplify -s=3` turns a 19-crossing diagram of the UNKNOT into a
3-crossing diagram of the TREFOIL.

```
$ knoodlesimplify --streaming-mode -s=3 < lattice_04.pd
WARNING: PlanarDiagram<I64>::SwitchCrossing: This method is considered **UNSAFE**,
and the diagram is currently locked ...
s
2  0  3  5  1
0  4  1  3  1
4  2  5  1  1        <- the trefoil
$ echo $?
0
```

Level sweep on the same input (HOMFLY of each output, measured on the output
diagram):

| level | crossings out | `SwitchCrossing` warnings | HOMFLY |
| --- | --- | --- | --- |
| `-s=0` | 19 | 0 | `1` (unknot) |
| `-s=1` | 16 | 0 | `1` (unknot) |
| `-s=2` | 0 | 0 | unknot |
| `-s=3` | 3 | **2** | **trefoil** |

`-s=2` already finds the unknot correctly. `-s=3`, nominally *more*
simplification, returns a different knot.

### Mechanism

`src/PlanarDiagramComplex/ArcSimplifier/R_IIa_diff_o_same_u.hpp:58-67` applies a
composite move:

```cpp
Reconnect<true,true,false>(w_3,!u_0,n_0);
Reconnect<true,true,false>(w_2, u_0,s_0);
Reconnect<true,true,false>(n_3,!u_1,n_1);
Reconnect<true,true,false>(s_2, u_1,s_1);

SwitchCrossing(c_0);
SwitchCrossing(c_1);
DeactivateCrossing(c_2);
DeactivateCrossing(c_3);
```

`SwitchCrossing` on a locked complex refuses and returns `false`
(`ModifyDiagram.hpp:7`), and the ArcSimplifier wrapper **discards that**:

```cpp
// ArcSimplifier/Helpers.hpp:23
void SwitchCrossing( const Int c_ ) { (void) pd.SwitchCrossing(c_); }
```

So the four reconnections and the two deactivations apply while the two
crossing changes do not. **The move is half-applied**, and the half that ran is
the half that rewires the diagram. R_IIa is only knot-preserving as a whole; a
partial application is a crossing change.

This is the non-atomic-composite-mutation shape: several steps, any of which can
individually fail, with the failure dropped on the floor.

### Fix

Two independent ones, and the first is the important one:

1. **The library must not half-apply.** `ArcSimplifier::SwitchCrossing` must not
   discard the result; R_IIa must refuse or roll back if a switch cannot be
   performed. A locked diagram should yield *no change*, never a wrong one.
2. **The caller should unlock what it owns.** This is the same family as the
   PDC locking regression already noted in our tree — a lock guard turning
   mutations into silent no-ops.

### Scope: which moves, which levels, since when

**Only two of twenty move files are affected.** `SwitchCrossing` is called from
`R_IIa_diff_o_same_u.hpp` and `R_IIa_same_o_diff_u.hpp` only. The other
eighteen use `Reconnect` and `DeactivateCrossing`, and **those are not
lock-guarded** — which is precisely the asymmetry that makes R_IIa half-apply
instead of failing cleanly. Guarded in `PlanarDiagram/Modify.hpp`:
`SwitchCrossing`, `CreateCrossing`, `Connect`, `ReverseColoredArcs`, plus
`ComputeArcColors` and `ResolveCrossing`. Not guarded: `Reconnect`,
`DeactivateCrossing`.

**It is the default state.** `PlanarDiagram.hpp:22`: *"Per default every new
created diagram is locked."* So a caller reaches the broken path without doing
anything unusual.

**Only at `local_opt_level = 4`.** The assisted R_Ia/R_IIa patterns are gated
behind that tier, so `local_opt_level` 1 (R_I) and 2 (R_I+R_II) are unaffected.
In `knoodlesimplify` that is exactly `-s=3`.

**Broken since 2026-07-25.** R_IIa has called `SwitchCrossing` since
`44377849` (2026-03-15); the locking mechanism landed in `7d83c7b2`
(2026-07-25), whose own message reads *"Maybe this temporarily breaks some
features."* It did — this one, for three weeks.

**Why nobody noticed.** The production default is `-s=6`, and levels 4-6
deliberately set `local_opt_level = 0` — a comment in
`tools/knoodlesimplify.cpp` records that Henrik's performance testing found the
local pass does not help once rerouting is engaged. So the only configuration
that reaches the bug is a diagnostic tier nothing routinely runs.

### How often it fires

Rarely, but when it fires it is always wrong. Over the 21 known unknots in
`data/diagrams/hardunknots/` at `-s=3`:

| | |
| --- | --- |
| emitted `SwitchCrossing` warnings | 1 |
| outputs small enough to verify by HOMFLY (<= 40 crossings) | 13 |
| verified **not** the unknot | 1 — the same one |

A one-for-one correlation: no warnings, no corruption; warnings, corruption.
So R_IIa does not fire on most diagrams, and "locked diagrams silently fail"
overstates it — but the move is unsound whenever it does fire.

### Repro fixtures

**Best repro — `data/diagrams/hardunknots/Monster.tsv`**, which already ships
in the repo. Ten crossings, a known unknot, 5-column PD code:

```
$ knoodlesimplify --streaming-mode -s=3 < data/diagrams/hardunknots/Monster.tsv
```

gives two `SwitchCrossing` warnings and a **6-crossing** output whose HOMFLY is
`2 - M^2 + 2L^2 - 3L^2 M^2 + L^2 M^4 + L^4 - L^4 M^2` — emphatically not the
unknot. `-s=2` on the same input returns 10 crossings and stays unknotted.

Also reaches it: `test/embeddings/lattice_04.crd` (2 warnings; a 19-crossing
unknot becomes a trefoil), `lattice_06` (6), `lattice_08` (14). A bare trefoil
does not trigger it at any level — R_IIa has to actually fire.

### How it was found, and what it cost

It is the cause of a chain of contradictions in one session: the CLI reported
`lattice_04` as a trefoil while every in-process route (HOMFLY on the raw
object, on a round-trip through its own PD code, and on the file; plus
`PDC::Simplify`) reported the unknot. I corrected the fixture docs the wrong way
twice before finding the warning on stderr that explains it. **A silent
half-applied mutation costs more than a crash.**

## 11. `Prosector::LinesColinearTest` aborts on two distinct degenerate segments

**Status:** confirmed 2026-08-14, `dev_prosector` @ `56807ba3`.
**Severity:** high — an `assert` takes the process down on legal `double`
input, where the documented contract is to return an error code.

`src/Prosector/DegeneracyChecks.hpp:97`. The function's stated precondition is
only *"the two lines are colinear"*. Its loop `continue`s when both segments
are degenerate in coordinate `k`, so if both are degenerate in all three it
falls through to:

```cpp
assert(x_0[0] == y_0[0]);
assert(x_0[1] == y_0[1]);
assert(x_0[2] == y_0[2]);      // <-- fires
...
// If we arrive here, then both intervals are degenerate and colinear. So their
// intersection is one point, namely `x_0 == x_1 == y_0 == y_1`.
return true;
```

Two *distinct* points are trivially collinear, so the precondition permits them
and the conclusion does not follow. Both the assertion and the `return true`
below it are wrong: two distinct points do not intersect, so the answer is
`false`.

The pair reaches the test whenever the two degenerate segments share a
**projected** point and differ in height — the broad phase keeps them, and then
all three coordinates compare equal-within-each-segment.

### Repro

Eight vertices, plain `double`, all small integers:

```
0 0 0   4 0 0   4 0 0   4 4 0   4 4 5   4 0 5   4 0 5   0 0 5
        ^^^^^^^^^^^^^                   ^^^^^^^^^^^^^
        zero-length at (4,0,0)          zero-length at (4,0,5)
```

Feed that to `LinkEmbedding2<double,int64_t,int64_t>::RequireIntersections`:

```
Assertion failed: (x_0[2] == y_0[2]), function LinesColinearTest,
file DegeneracyChecks.hpp, line 97.
```

Committed as `test/embeddings/deg_stacked_points.crd`, with `xcrash` markers so
the suite tracks it and announces its own fix.

### Why it matters more than it looks

**One root cause behind three symptoms.** It is also what aborts (a) two lattice
knots separated by 1e16, where `f64` rounding (ULP = 2 at that scale) collapses
vertices into zero-length edges, and (b) integral coordinates transformed by a
truncated rotation matrix. Neither looked like the same bug.

**Not reachable from the CLI today**, because `tools/knoodle_io.hpp:58` still
binds `LinkEmb_T = LinkEmbedding<Real,Int,float>` — the float class, which
handles this gracefully (the tools report library errors and exit 1, as
designed). It is reachable now by any direct user of `LinkEmbedding2/3/4`,
which their class docs invite, and it **blocks the switchover** those classes
are headed for: the day that binding changes, this becomes a user-facing abort
on legal `double` input.

**Suggested fix:** return `false` instead of asserting. Two degenerate segments
intersect iff they are the same point, which the existing code can test
directly.

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
