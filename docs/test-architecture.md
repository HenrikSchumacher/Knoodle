# Test architecture: careful make + manifest

Status: PLAN, 2026-08-15. Nothing here is built yet.

## Why

Three concrete failures motivate this, all found in the 2026-08-14/15 sweep:

1. **Stale binaries.** `test/Makefile` and `tools/Makefile` have no `-MMD -MP`.
   Prerequisites name `Knoodle.hpp` but not `src/`, so editing a library header
   leaves every test binary stale. This bit twice in one session:
   `local_moves_check` reported a pre-fix result after Henrik's fix was merged.
2. **Orphan targets.** `all` builds 2 of 23 test binaries. The other 21 are built
   only when someone names them. Consequence: `klut_identify_check` no longer
   compiles (`undeclared identifier 'ScopedUnlock'`) and nobody noticed, and
   `link_alex_probe` exits 1 with `3/5 links passed invariance`.
3. **A regression the suite could not see.** GitHub #33 (R_IIa half-applies on a
   locked diagram; `-s=3` silently returns a different knot) survived because the
   corpus never exercises the local tiers: `local_moves_check` reports that they
   changed the answer on **1 of 581** diagrams.

The design below is aimed at those three, in that order.

## 1. The manifest

One file, `test/manifest.tsv`, as the single source of truth. Columns:

    target  kind  needs  light-args  heavy-args

* `kind` ∈ `test` | `tool` | `bench` | `script`. This distinction is the one the
  sweep proved we need: `reapr_corner_probe` requires a `FILE.tsv` argument (it
  is a tool), and `klut_bench` / `klut_bench_boost` are benchmarks. Mixing them
  with tests is why "just run everything" has never worked. `script` (added
  2026-08-15) covers Python tests, which compile to nothing but must still be
  listed -- `cli_stdin_check.py` sat outside the manifest entirely, which is
  precisely the orphan condition the manifest exists to prevent.
* `needs` ∈ `-` | `homfly` | `umfpack` | `homfly,umfpack` | `klut`. Drives both
  the link line and CI skips on platforms lacking a dependency.
* `light-args` / `heavy-args`: the argv for each tier. Empty means "no arguments".

Consumed by the Makefile (via `$(eval)`) and by `run_tests.py`. Two lints, run as
part of `make check`:

* a `test/*_check.cpp` with no manifest row fails;
* a manifest row with no source file fails.

That is what makes orphans structurally impossible rather than a matter of
remembering.

## 2. Makefile

1. **`DEPFLAGS = -MMD -MP` plus `-include $(wildcard build/*.d)`.** Already
   written and reviewed on `orthodecorate`; merge to `dev_prosector` and `main`.
   Fixes failure (1) outright and is independent of everything else here.
2. **Collapse 23 recipes into ~4 pattern rules**, keyed on `needs`. The sweep
   found the 23 recipes already reduce to 4 distinct shapes.
3. **`all:` builds every manifest binary** — tests, tools and benches alike.
   Fixes failure (2).
4. **`make check`** = `all` + light tier. **`make check-full`** = `all` + heavy tier.
5. Keep the hand-rolled brew discovery. Library dependencies are brew's job; a
   test or tool that needs more elaborate provisioning than that is not a test or
   tool, it is a separate project that deserves its own repo.
6. `-j` is already safe (`| build` is order-only) and stays that way.

## 3. Tiers

Measured at defaults on 2026-08-15: **186 s total run time**, of which six tests
are 178 s and the other fifteen sum to 4.4 s.

The light tier **reduces** every test; it never skips one. Skipping is how the 21
orphans happened.

### Light (`make check`) — budget: build time + 30 s of run time

**BUILT 2026-08-15.** `make check` = `all` + `tools` + `lint` + `run_tier.py
--tier=light`. Measured over 27 runnable targets:

| | measured |
|---|---|
| **wall, 4 workers** | **15.0 s** |
| cpu across workers | 23.4 s |
| slowest: `embedding_check` | 15.0 s |
| next: `plantri_check --up-to-crossing=6` | 1.6 s |
| everything else (25 targets) | ≈ 7 s cpu |

`embedding_check` alone now sets the wall time, so more workers buy nothing until
that one test is reduced. The ~10 s `nearmiss_sphere` fixture is the lever if the
budget ever gets tight.

Two reductions were verified to still do real work rather than becoming no-ops:
the reduced `inflate_check` prints `1/1 trials passed` with HOMFLY preserved, and
the reduced `klut_check` prints `PASS: KLUT is consistent`.

`embedding_check` resists reduction: `--rotation-steps=6 --rotations=1` **exits
1**, because the rotation tier's trend statistic compares quarter-means and six
steps make the quarters too small to be meaningful. It therefore runs full in the
light tier at 15 s, half the budget. About 10 s of that is `nearmiss_sphere`;
regenerating it at K = 24 buys back ~7 s and keeps the f32 story intact, if the
polygon corpus needs the headroom.

`klut_bench`, `klut_bench_boost` and `reapr_corner_probe` are not tests and run in
neither tier. They are still built by `all`, so they cannot rot.

### Heavy (`make check-full`) — budget: hours are fine

**BUILT 2026-08-15**, measured at **242 s wall / 428 s cpu**, 27 passed. Heavy is
not merely "the defaults" -- the manifest's `heavy-args` column expands coverage:
`inflate_check --trials=5`, `link_inflate_check --trials=3`,
`plantri_check --up-to-crossing=7` (26 s on its own),
`embedding_check --coords=f64,f32,i32,i64 --rotations=8`.

Four minutes is comfortably short of the budget, so this has room to grow as the
corpus does.

## 4. The random-polygon corpus (new)

This is the fix for failure (3). Hard unknots are *built* to resist local moves,
which is exactly why they do not exercise them. Random equilateral polygons of a
few hundred edges do.

* **Location** `test/polygons/`, alongside the existing `test/embeddings/`
  convention. **Extension `.crd`, not `.tsv`** — `.gitattributes` routes all
  `*.tsv` through Git LFS, and putting kilobyte text files in LFS is both wasteful
  and a CI checkout dependency we do not want.
* **Size** 10–20 polygons, mixed edge counts (roughly 8 × 60, 8 × 120, 4 × 240),
  coordinates at ~9 significant digits. About 100 KB total.
* **Generator** `test/polygons/make_polygons.cpp`, a one-off built by
  `make polygons` and not run by either tier. It uses
  `Sampler_T::RandomEquilateralLink`, the same sampler `klut_bench
  --polygon-edges` already drives, with a fixed seed recorded in each file's
  header. Checked in for provenance; the corpus itself is the artifact.
* **Expected answers.** Random 150-gons are mostly but not entirely unknots, so
  the corpus cannot assert "unknot". Each `.crd` gets an `.expect` recording the
  raw diagram's crossing count, its Alexander |det| fingerprint, and its HOMFLY
  when the simplified diagram is small enough to trust.
* **The test** `polygon_levels_check`: for every polygon, sweep
  `PDC_T::Simplify_Args_T` over `local_opt_level` ∈ {0, 1, 2, 4} crossed with the
  `rerouteQ` / `disconnectQ` / `compressQ` settings the `-s` presets select, and
  require every result to agree with the reference invariant. Alexander is the
  tripwire on large outputs (the pattern `inflate_check` already uses); HOMFLY is
  the oracle once the diagram is under the verifiable cap. This is precisely the
  shape that catches GH #33: level 0 and level 4 must return the same knot.

**Entirely in-process — no `knoodlesimplify` invocations.** This test is about
`PlanarDiagramComplex::Simplify`, and the CLI is a different contract with its own
suite (§5). Shelling out would confuse the two: a failure would not say whether
the library computed the wrong answer or the tool passed the wrong arguments.

## 5. Tool contract tests

A test suite for `knoodlesimplify` and a test suite for
`PlanarDiagramComplex::Simplify` are different things. The tool's contract is
narrow and has exactly three clauses:

1. parse input correctly,
2. be a faithful CLI for the library call,
3. write output correctly.

Nothing in that list is "compute the right knot". So the tool suite must never
deduce what call was made by inspecting the mathematics of the output — that
conflates the two contracts and makes every failure ambiguous.

### 5a. Flag wiring — `simplify_config_check`

The good news from the survey: clause 2 is already **two pure functions**.

    ParseArguments(argc, argv)  ->  std::optional<Config>
    BuildSimplifyArgs(config)   ->  PDC_T::Simplify_Args_T

`knoodlesimplify.cpp:656` then does `const PDC_T::Simplify_Args_T args =
BuildSimplifyArgs(config);` and passes it straight through. So the whole
argv → arguments path can be tested by **calling those two functions directly and
comparing structs field by field** — no intercept, no debug output to parse, and
crucially no second code path that could drift from the real one. A print-and-
compare debug mode would be strictly weaker: it tests the printer as much as the
wiring, and it can only check the fields somebody remembered to print.

What this buys that output-inspection cannot:

* every field of `Simplify_Args_T` asserted, including the ones the flag under
  test is *not* supposed to touch — the "`--local-opt-level=2` must not perturb
  `rerouteQ`" class of bug;
* the preset ladder (`-s=0..5`) asserted as a table, which is where
  `simplify-level-wiring` found levels 1/2/3 were silently no-ops;
* rejection paths asserted — `--local-opt-level=7` must yield `nullopt`, not a
  clamped value;
* flag-over-preset precedence asserted, which is the whole point of the
  `has_value()` cascade at lines 585–600.

**DONE 2026-08-15.** `Config`, `ParseArguments` and `BuildSimplifyArgs` extracted
to `tools/knoodlesimplify_config.hpp`; `test/simplify_config_check` has 129
checks. Verified behaviour-preserving against a pre-refactor binary: identical
on 147 diagram runs, 19 flag runs, and `--help`.

### 5a-bis. The other two tools have DIFFERENT contracts

Not the same treatment, despite the same file shape. Revised 2026-08-15 (JHC);
deferred to a later pass.

**`knoodledraw` → `OrthoDraw`.** Same contract as `knoodlesimplify` and the same
argument for testing it: its flags must reach `OrthoDraw_T::Settings_T`
faithfully. Cheaper than expected -- **`BuildSettings(const Config&) ->
OrthoDraw_T::Settings_T` already exists** (`knoodledraw.cpp:1941`), so no code
needs restructuring to produce the struct; it only needs extracting to a header,
exactly as `knoodlesimplify` was. Two extras worth covering that
`knoodlesimplify` has no analogue for: `ValidateCLPSettings` and
`ValidateSettingsCombinations` reject certain settings *combinations*, so
refusal is part of this contract in a way it was not there.

**`knoodleidentify` → `ki::Identify`.** A much narrower surface, and narrow
**by design** rather than by omission:

* There *is* an argument struct -- `ki::IdentifyParams` (`cap`, `deep_cx`, `rot`,
  plus `n0` and `base` which are not exposed) -- but it is built inline in `main`
  from three Config fields, not by a function. Testing it means extracting that
  three-line construction first.
* It exposes **no** simplification options at all, and must not start.
  `knoodleidentify`'s core *is* a structured simplification-and-Reapr-escalation
  policy that has to be controlled directly; handing users the generic simplify
  knobs would let them defeat it. So "the CLI does not forward these" is the
  contract, and a test here should assert their **absence** rather than their
  wiring.

**Plus a small debug flag, for the one thing purity does not cover:** that `main`
really does pass that struct to `Simplify` unmodified. A hidden
`--debug-print-simplify-args` that dumps the struct *immediately before the call*,
from the same object that is passed, keeps it to one code path and doubles as a
user-facing debugging aid. This is belt-and-braces — the variable is already
`const` — but it is cheap and it pins the call site.

### 5b. Input parsing — `knoodle_io_check`

Clause 1, and this one needs **no refactor at all**. `tools/knoodle_io.hpp` is a
header; its anonymous namespace is per-TU, so a test that includes it gets its own
copy and can call everything directly: `ParseNumericLine`, `CleanLine`,
`DetectFormat`, `CreateDiagramFrom3D`, `CreateDiagramFromPDCode`, `ReadKnot`.

Unit tests, not corpus tests: malformed lines, comment and blank handling, the
4/5/6/7-column discrimination in `DetectFormat`, float-vs-integer detection,
blank-line component separation, and the failure paths. Fast — this belongs in
the light tier's 4.4 s bucket.

### 5c. Output and exit codes — `cli_contract_check`

Clause 3. Assertions on shape rather than mathematics:

* output re-reads as input, and a second `-s=0` pass is a fixed point;
* **column counts** — and the rule is NOT what the shorthand says. It is
  `knoodlesimplify.cpp:820`:

      colored_output = (input_column_count >= 6) || split_into_summands

  Colors are written when they carry information: when the input had them, or
  when splitting means the reader needs to know which summands were once one
  component. **A 5-column LINK comes back as 5 columns.** "5 for knots, 7 for
  links" is wrong, and believing it is what made the 2026-08-14 misreading easy;
  all four branches of the real condition are asserted separately.
* the fail-loud contract from `cli-fail-loud-contract`, observed **from outside
  the process**: nonzero exit *and* empty stdout *and* something on stderr.
  `error_tap_check` already pins the mechanism (CerrErrorTap, AtomicOutFile) in
  process; nobody had checked the assembled tool behaves that way.
* `--help` exits 0 and documents every flag **read out of the parser source**, so
  a flag added without documentation fails here. 37 flags found; verified
  non-vacuous by confirming the deliberately hidden
  `--debug-print-simplify-args` is what the lint reports when unexempted.
* `knoodledraw` works as a Unix filter on `knoodlesimplify`'s output, and refuses
  garbage rather than drawing it.

## 6. The stderr scan

"Silently refused" is the wrong description of a locked complex, and it has been
repeated in several commit messages including our own. `Push` on a locked
`PlanarDiagramComplex` calls `wprint`, which writes to **`std::cerr`** (and to the
log file) with a `WARNING: ` prefix. The library says exactly what is wrong. We
have not been reading it.

The mechanism is uniform and machine-readable (`Tools/src/Profiler_Singleton.hpp`,
`Tools/src/Logger.hpp`):

| call | goes to | prefix |
|---|---|---|
| `wprint` | `std::cerr` + log file | `WARNING: ` |
| `eprint` | `std::cout` **and** `std::cerr` + log file | `ERROR: ` |
| `nprint` | **log file only** | `NOTE: ` |

**The tools do not use those.** `knoodle_io.hpp`'s `LogError` writes
`"Error: "` -- mixed case, not `ERROR:` -- to `*g_log_stream`, which is stderr
by default but is redirected to a file in streaming mode (its own comment says
"the tap on cerr won't see it"). So a scan must match `^(ERROR|Error|WARNING):`
to catch both layers, and for the tools it must also know where the log stream
was pointed. Found while writing `knoodle_io_check` (§5b), whose refusal cases
emit six `Error:` lines that a naive `^ERROR:` scan reports as zero.

So a scan of stderr for `^WARNING:` and `^ERROR:` costs nothing and needs no
change to any test. `nprint` is the gap: NOTE-level output reaches no stream at
all, so a stream scan cannot see it and anything important must not be reported
that way.

### What the suite does today

Measured 2026-08-15, stderr only:

| | exit | WARNING | ERROR |
|---|---|---|---|
| 10 of 12 fast tests | 0 | 0 | 0 |
| `intersection3d_check` | 0 | 48 | 48 |
| `link_color_roundtrip` | 0 | 0 | 2 |
| `inflate_check`, `link_inflate_check`, `klut_check`, `plantri_check` (reduced) | 0 | 0 | 0 |
| `embedding_check` | 0 | **16993** | **473** |

The two small hits are correct: both tests deliberately provoke a refusal and are
asserting that it happens. `embedding_check`'s volume is also accounted for — all
four ERROR shapes and every frequent WARNING shape come from its degenerate
fixtures (`ComputeEdgeEdgeIntersection`, `DegenerateEdges`, `FindIntersections`),
which is what those fixtures exist to trigger. Nothing unexplained turned up.

That is the encouraging part: the baseline is nearly clean, so switching the scan
on is cheap rather than a flood to triage.

### Design

* The manifest gains a **`stderr-policy`** column: `clean` (the default -- any
  `WARNING:`/`ERROR:` line fails the test) or a path to an allow-file.
* An allow-file lists permitted message **shapes** as regexes, not counts. Counts
  would be brittle: `embedding_check`'s 16993 will move with any fixture edit,
  and a count that must be updated constantly gets updated without being read.
* The runner captures stderr per test, normalizes numbers, and fails on any line
  no shape matches.
* **The allow-file is itself an assertion.** A permitted shape that stops
  appearing means a fixture stopped provoking what it was built to provoke --
  the same signal an XPASS gives, and it should be reported the same way.

### Why this is worth doing now

It would have caught `klut_identify_check`'s locking bug the moment it appeared:
seven `WARNING: ...Push: ...currently locked...` lines on stderr, and exit 0. The
test reported a green connect-sum group while every `Push` did nothing. Under this
scan that is a failure at the first run, not a discovery weeks later.

## 7. The no-op guard

Reduced arguments can silently turn a test into a test of nothing, and an exit-0
that checked nothing is worse than a red one. Every test should print a work
statistic — the coverage counter added to `local_moves_check`
("local tiers changed the answer on N of M diagrams") is the pattern — and the
light tier should assert it is nonzero. Without this the light tier quietly
becomes theatre.

## 8. Push policy and hooks

### Rules

| action | what runs | if red |
|---|---|---|
| local commit, any branch including `main` | nothing | — commits are free; `main` may carry WIP history |
| push to a feature branch | light tier | **warn**, push proceeds; Claude-authored commits must say so in the message (below) |
| push to `main` | light tier | **refuse**; explicit override required, for everybody including Henrik |
| push a `v*` tag | heavy-tier stamp check | **refuse** unless a clean heavy run is recorded for that exact commit |

The tag case is where the release gate belongs: a versioned GitHub release starts
with a tag push, so gating the tag is simpler and harder to route around than
wrapping `gh release create`.

### The refusal message for `main`

Verbatim shape, so it tells the reader what to do rather than just saying no:

```
Tests failed: local_moves_check, embedding_check, polygon_levels_check

Pushing red tests to main is blocked. Recommend investigating before pushing to
main, or pushing this commit to a branch instead.

  To push to main anyway:
      KNOODLE_PUSH_RED=1 git push origin main

  To put it on a branch instead:
      git push origin HEAD:refs/heads/<branch-name>
```

The override is an environment variable, **not** `--no-verify`, because
`--no-verify` also skips the Git LFS pre-push hook and would corrupt an LFS push.

### The WIP-marker rule

When the light tier is red and the push is to a branch, the hook warns. If the
commits being pushed carry the `Prepared with Claude Code` credit line — i.e. they
are ours — the hook additionally requires the message to say plainly that this is
a WIP push that does not pass testing, and refuses with an `git commit --amend`
instruction otherwise. Overridable by the same variable.

This reuses a marker that is already mandatory (see `CLAUDE.md`), so it costs
nothing to detect.

**Decided: enforced, not merely warned.** The intended dynamic is that the hook is
a timely reminder rather than a request for a decision — Claude sees the refusal,
amends the message to say WIP, and re-pushes without asking. That behaviour needs
to be written down in two places to be reliable:

* `CLAUDE.md`, so the WIP line is written the first time when the tier is already
  known to be red;
* the refusal message itself, which should give the literal
  `git commit --amend` invocation rather than describing it.

The human-facing half is unchanged: for Henrik, or for a hand-written commit, this
is a warning-shaped refusal with an obvious one-line fix.

### Mechanics

* **Hooks are versioned.** `scripts/githooks/`, activated by
  `git config core.hooksPath scripts/githooks` from a `make hooks` target. This is
  what lets Henrik get the same policy.
* **Chain to LFS.** This repo's current `pre-push`, `post-commit`, `post-merge`
  and `post-checkout` hooks are git-lfs's. Moving `core.hooksPath` disables
  them, so the versioned `pre-push` must call `git lfs pre-push "$@"` first and
  the other three must be carried over verbatim. Getting this wrong breaks LFS
  silently, so it needs a test of its own.
* **Stamps.** A clean tier run on a clean worktree writes
  `test/.stamps/<tier>-<sha>`. The hook skips a tier whose stamp matches `HEAD`.
  One mechanism serves both the push gate (avoid re-running 30 s on every push)
  and the release gate (prove the heavy tier ran on this exact commit).
* **CI.** Done. `scripts/ci-build-and-test.sh` used to hard-code three drivers;
  it now runs `run_tier.py --tier=light`, so there is one definition of the tier
  instead of two that drift. Coverage went 3 -> 12 tests.

  **One definition, two platforms.** CI genuinely cannot run everything: several
  images have no SuiteSparse, and it does not fetch the Git-LFS data. Rather
  than keep a second list, both the build and the run filter on the manifest's
  own `needs` column -- `make all EXCLUDE_NEEDS=...` for what gets BUILT,
  `run_tier.py --exclude-needs=...` for what gets RUN, so the two cannot
  disagree. `--exclude` covers runtime requirements the column cannot express
  (the KLUT data; `cli_stdin_check`, which needs a pty).

  Every skip is printed in the summary under "SKIPPED on this platform (this run
  covered less than a full tier)". A subset run must never read as a full one --
  that is the same failure mode as a reduced test silently becoming a no-op.

## 9. Prerequisite cleanups

These are not extras. Each is a test that currently cannot participate in any
tier, and the first two must be resolved before a hook can gate on the suite.

* **`klut_identify_check` does not compile** — `undeclared identifier
  'ScopedUnlock'`, plus a PDC constructor mismatch. Rotted precisely because
  nothing builds it. Fix or retire.
* **`link_alex_probe` exits 1**: `3/5 links passed invariance`. **Diagnosed
  2026-08-15, and it is not a library bug.** Two of its five hardcoded fixture
  paths do not exist: `data/diagrams/linktable/` names 3-component links with an
  extra index (`L6a4_0_0.tsv`, not `L6a4_0.tsv`). The probe prints
  `INVALID diagram, skipping` for a missing file, counts it as a non-pass and
  exits 1. The three links that do load pass invariance on all six rounds and
  produce 0 discrimination collisions. Fix the paths — better, glob the corpus so
  the list cannot drift again.

  **But note what the two broken names are:** precisely the links with mu >= 3. So
  the |det| fingerprint has never been exercised beyond two components, which is
  where its justification is least obviously safe (it rests on the reduced strand
  matrix degenerating at t = 1 to rank n - mu). Fixing the paths may convert a
  naming bug into a real one. Correct the paths early, because a red target left
  standing is what makes the push-hook override habitual; defer whatever they
  expose until the rest of the suite is in place to catch collateral damage.

  Note also that this file is a **prototype**, not a regression test — the idea it
  validated was adopted into `link_inflate_check`, which passes. The manifest
  should probably classify it accordingly.
* `reapr_corner_probe` → classify as `tool` in the manifest.
* `klut_bench --help` takes 10.3 s because it loads KLUT tables before parsing
  argv. Parse first.

## 10. Sequencing

1. ~~`DEPFLAGS` merged to both branches.~~ **DONE 2026-08-15** —
   `dev_prosector` 1578b908, `main` fabf4efa (rebased onto `origin/main`).
   Neither is pushed. A fresh `component_check` now tracks 364–376 of Knoodle's
   own `src/` headers where it previously tracked 1.
2. ~~Fix `link_alex_probe`'s fixture paths; fix or retire
   `klut_identify_check`.~~ **DONE 2026-08-15** — `dev_prosector` a70f1f58 +
   d5377afd, `main` ddcde7ab + abbb1579. Both green, neither pushed.

   The deferral clause turned out not to be needed: the corrected mu >= 3
   fixtures (Borromean rings and L6n1) **pass** invariance across all six
   reprojection rounds, so nothing was exposed that needs the net first.

   `klut_identify_check` did surface two src/ defects, now
   **[PR #35](https://github.com/HenrikSchumacher/Knoodle/pull/35)** against
   `dev_prosector` (issues 14 and 15 in `docs/upstream-issues.md`):
   `ScopedUnlock` is documented in `PlanarDiagram.hpp` and
   `PlanarDiagramComplex.hpp` but defined nowhere, and
   `PlanarDiagramComplex(const PD_T &)` (`:145`) delegates to a constructor that
   only takes `PD_T &&` (`:112`), so that overload cannot be instantiated. The
   PR adds `test/lock_api_check` alongside the fixes. Our tests work around both
   independently (`std::move`, `Unlock()`), so nothing here waits on the merge.
3. Manifest, pattern rules, `all` covering everything, the two lints.
4. `knoodle_io_check` — no refactor needed, so it is the cheapest real coverage
   available and should not wait behind anything.
5. The polygon corpus + `polygon_levels_check`; measure its runtime.
6. ~~Extract `knoodlesimplify_config.hpp`; `simplify_config_check`; the
   `--debug-print-simplify-args` flag.~~ **DONE 2026-08-15.** The other two
   tools are deferred to a later pass and want different contracts, not the
   same one -- see §5a-bis.
7. ~~`cli_contract_check`.~~ **DONE 2026-08-15**, as a `kind=script` Python
   test: 53 checks. Found that the column-count rule is not what we thought --
   see §5c.
8. ~~`make check` / `check-full`; point CI at `make check`.~~ **DONE
   2026-08-15.** CI coverage went from 3 hand-listed drivers to 12 tests, driven
   by the manifest. See "One definition, two platforms" below.
9. Versioned hooks (with the LFS chain), push policy, stamps; the `CLAUDE.md` WIP
   rule lands with them.
10. Release gate on `v*` tags.
11. Work guards in the reduced-argument tests.

Steps 1 and 2 are worth doing on their own merits regardless of whether the rest
lands.
