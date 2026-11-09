# KLUT table loading: failure audit

*2026-11-08. Branch `claude/klut-table-audit-ee96fc`, cut from 617f0155 (the
LFS-pointer check, committed locally and not pushed). This is an audit only:
nothing here changes behaviour yet. The fixes in the last section wait for JHC.*

*Status, later on 2026-11-08: JHC approved Batch A, and the commit after this
report implements it (T1–T6, T8 in its pre-manifest form, T10, and
`test/klut_table_check.py`). The results below describe the tool before that
commit. Of the six silent wrong-name cases in the summary, three are now
refused at startup: the count-digit flip, the 4 KiB zero fill, and the tested
name-digit flip (it happened to duplicate another name; a flip to an unused
index would still pass). The alternating-flag flip, the reordered names and the
duplicated key still pass silently. They need T7 (a checksum) or T1b (an eager
load). A table swapped mid-run is now an error. Still open: T1b, T7, T9, the
S1–S5 PR to Henrik, F1–F2 and P1–P3.*

*Status, update: JHC then approved T1b, T7, T9 and the formula and paclet
fixes, and they are implemented in the commit after the Batch A one (the
formula and paclet changes are in their own repositories). T7 checks the
values files' SHA-256 on every run and the keys files' only with
`--verify-tables`, and refuses a table set other than the build's unless
`--allow-other-tables`. With T1b and T7 together, every damage case in this
report is caught. The only gap left is a flipped bit in a keys file that makes
no duplicate key: an ordinary run misses it, and `--verify-tables` catches it.
The S1–S5 PR waits for a discussion with JHC.*

## Summary

`knoodleidentify` trusts whatever files it finds. It never checks their size,
structure, checksum or version. 617f0155 added the first check, which looks for
Git LFS pointer files. Every case below was run on scratch copies of
`data/Klut`. The results:

1. **Bad tables give wrong knot names, and the tool exits 0.** Six kinds of
   damage produced wrong names, with no error and exit 0:
   - a one-bit flip in a count digit (33,813 wrong keys);
   - a one-bit flip in a name digit;
   - a one-bit flip in the alternating flag;
   - a 4 KiB zero-filled block (1,129,264 wrong keys, which is 73% of the
     13-crossing table);
   - a values file whose names are reordered (91% wrong);
   - one duplicated key, after which the left trefoil comes back as the right
     trefoil.

   The wrong names look like real answers, for example `K[13,4621,1,"mr"]`
   for 13n4622.
2. **Missing, empty, truncated or unreadable tables come back as `NotFound`.**
   The message is "table gap or a hard simplification case?". The tool exits 1,
   unless `--quiet` is given: then it exits 0 and prints `NotFound` on stdout.
   Some of these cases print a library `ERROR:` line, and some print nothing at
   all.
3. **An unreadable directory crashes the tool.** An uncaught
   `filesystem_error` leads to `abort()` and exit 134. On macOS that files a
   crash report.
4. **The install-time checks don't load the tables.** The Homebrew formula's
   test and `release_tarball_check` both pass with all 11 key files deleted.
5. **The paclet never sees the exit code.** It reads stdout only, so even the
   failures that are loud today return an answer in Mathematica.
6. **Version skew does not exist yet, and nothing would catch it.** The table
   bytes are identical in every release from v1.0.0 to HEAD. There is no format
   version, header or manifest.

Most of the fixes are in `tools/`, which is ours. Five small reader bugs are in
`src/Klut/Subtable.hpp` (Henrik's). The rest are downstream, in the formula and
the paclet.

## How tables are found, opened and trusted today

**Search order.** This is `ResolveDataDir`, `tools/knoodleidentify.cpp:341`.
It is not the order given in the brief or in `CLAUDE.md`. The first candidate
that contains `Klut_Values_03.tsv` wins:

| # | candidate | note |
|---|---|---|
| 1 | `--data-dir=PATH` | **exclusive**: when given, it is the only candidate (no fallback) |
| 2 | `$KNOODLE_KLUT_DIR` | **not exclusive**: if set but wrong, falls through *silently* |
| 3 | `<real exe dir>/../share/knoodle/Klut` | Homebrew / FHS layout (missing from `--help` and the README) |
| 4 | `<real exe dir>/../data/Klut` | in-tree build |
| 5 | `-DKNOODLE_KLUT_DIR` compile-time path | never set by any Makefile or the formula |
| 6 | `./data/Klut` (current directory) | dev convenience; used silently |

The "real exe dir" comes from `_NSGetExecutablePath` on macOS or
`/proc/self/exe` on Linux, canonicalized through `weakly_canonical`. Native
Windows has no exe path, so only candidates 1, 2 and 6 exist; the paclet always
passes `--data-dir`. The test for a candidate looks at one file, and uses the
*throwing* `fs::exists`.

**Trust.** Five things decide what the tool believes:

- **The LFS check.** `KlutTablesAreReal` (617f0155) reads the first 41 bytes of
  each subtable file up to `--max-crossings`. It refuses the directory if any
  file is an LFS pointer. It skips files that are missing or can't be read.
- **The `Klut` constructor** (`src/Klut.hpp`) only records whether each file
  exists. It reports nothing.
- **Lazy loading.** `Subtable::Read()` (`src/Klut/Subtable.hpp`) loads a
  subtable the first time a diagram of that crossing number is looked up. So a
  damaged 13-crossing table is not noticed until the first 13-crossing knot,
  and earlier output has already been written. `Read()` has four problems:
  - It checks the key stream *before* each read, never after. A shortfall of
    exactly one key is never detected: the short read is inserted as a key.
  - A count that fails to parse ends the table silently, and the table is
    still marked loaded.
  - Keys left over after the last count are ignored.
  - Duplicate keys are resolved first-wins (`lut.insert`).
- **Names are read twice.** `LoadNames()` in the tool re-reads every values
  file at startup with its own parser. `Subtable::Read()` assigns ids later.
  If the two disagree, `NameOf()` falls back to `K[c,?,?,"?"]`, which the tool
  prints as `KnotSymbol[c,0,False,"?"]` and counts as identified.
- **A table error looks like a miss.** `klut_identify.hpp` `Found()` treats
  `Klut::error` (subtable failed to load) the same as `not_found`. Identify
  escalates through Reapr, then reports `NotFound[N, PD]`.

**Exit status.** The tool exits 1 when the library printed an `ERROR:` line or
the tool called `LogError`. Each `NotFound` calls `LogError`, unless `--quiet`
is given. `--quiet` also suppresses the count. So with `--quiet`, a damaged
table that prints no library error exits 0. knoodleidentify writes to stdout
one knot at a time. Its fail-loud contract is "nonzero exit plus a notice". It
withholds no output.

## Results by failure mode

### How the experiments were run

- **Inputs.** Each input is a PD code built from a table key, so its true name
  is known.
  - The **30-sample** set is the first key of the first, middle and last knot of
    every subtable, 3 to 13 crossings.
  - The **targeted** set is 7 keys chosen to hit each corruption: the last key
    of `K[13,601,1,"e/r"]`, the first key of `K[13,1,1,"m/mr"]`, both keys of
    13n5110, the first keys of 13a4622 and 13n187, and the right trefoil.
- **Table copies.** Every damaged table is a copy of `data/Klut` in a scratch
  directory, passed with `--data-dir`.
- **Every key.** A small probe built against the library looks all 1,816,748
  keys up in a damaged `Klut`. It names each result the way knoodleidentify
  does (`LoadNames` + `NameOf`), and counts right names, wrong names,
  `not_found` and `error`.
- **Crash reports.** Every run had a `DYLD_INSERT_LIBRARIES` shim that turns
  SIGABRT and SIGSEGV into `_exit`, so no run filed a crash report.
- **Where the scripts are.** The recipes are in the appendix. The probe sources
  and raw outputs are in the side-session notes folder, under
  `klut-audit-probe/`.

Baseline: clean copy, 30/30 and 7/7 right, exit 0; probe 1,816,748 / 1,816,748
right.

In the tables below, "keys" are the probe's counts over all 1,816,748 keys, of
which the 13-crossing table has 1,536,686. "Tool" is knoodleidentify on the
targeted set. **Bold** marks a wrong name.

### 1. Git LFS pointers (617f0155)

| case | today (617f0155) | v1.0.6 (installed brew binary) |
|---|---|---|
| all 22 files are pointers (`git show HEAD:data/Klut/<f>`) | exit 1, stdout empty, error names the dir and says `git lfs pull` | 7/7 `NotFound`, **exit 0** |

Review of 617f0155:

- **What it gets right.** It runs before any input is read. It covers all three
  resolution routes, and the message is clear. The test builds pointer files
  the way a no-LFS checkout does, so it runs the same in CI.
- **What it can't see.** It only recognizes the LFS spec header. It does not
  notice a pointer-sized file that is not a pointer: an empty file, an HTML
  error page, a truncated copy. It also skips unreadable files on purpose.
- **What to do with it.** It is a special case of the structural check proposed
  in T1, which subsumes it. Keep its message: LFS is the commonest real cause.

### 2. The files can't be found, or only some can be read

| case | how made | tool today | risk |
|---|---|---|---|
| empty dir / nonexistent dir / a regular file as `--data-dir` | `mkdir`; `/etc/hosts` | exit 1, stdout empty, "Could not find KLUT data directory. Tried: ..." | none: correct |
| binary copied away from its tables | copy to `elsewhere/bin` | exit 1, lists all 3 candidates tried | none: correct |
| Homebrew keg via `bin/` symlink; symlinked prefix; keg binary directly | fake `Cellar/knoodle/1.1.0/{bin,share/knoodle/Klut}` | 7/7 right, exit 0 | none: exe-relative resolution works |
| `$KNOODLE_KLUT_DIR` set to a wrong path | `KNOODLE_KLUT_DIR=/nonexistent` | silently uses the next candidate (in-tree tables), exit 0 | low today; a typo silently selects another table set |
| copied binary, current directory has a `data/Klut` | damaged copy at `./data/Klut` | silently uses it: **`K[13,186,0,"m/mr"]`**, exit 0 | medium: a stray checkout's tables are used, unannounced |
| only `Klut_Values_03.tsv` present | | chosen as the table dir; trefoil → `NotFound[3]` ×7, "table gap?", exit 1, no library error | the dir test looks at one file |
| `Klut_Keys_13.bin` missing (or `Klut_Values_13.tsv` missing) | `rm` | 13-crossing knots → `NotFound` "table gap?", 0 library errors, exit 1; **exit 0 with `--quiet`** | a missing file reads as "hard case"; nothing says a file is missing |
| `Klut_Keys_13.bin` empty | `: >` | `ERROR: ... Problem while reading 1-th key`; `NotFound`; exit 1 (also with `--quiet`) | the error only appears once a 13-crossing knot arrives |
| `Klut_Values_13.tsv` empty | `: >` | `NotFound`, 0 library errors, exit 1; **exit 0 with `--quiet`** | silent empty table |
| `Klut_Keys_13.bin` mode 000 | `chmod 000` | `ERROR: ... Could not open`; `NotFound`; exit 1 | loud, but late (lazy) |
| `Klut_Values_03.tsv` mode 000 | `chmod 000` | still chosen as the dir (exists); `ERROR: ... Could not open`; trefoil `NotFound`; exit 1 | |
| **table dir mode 000** | `chmod 000 dir` | **`libc++abi: terminating due to uncaught exception ... filesystem_error`, abort, exit 134** | crash (and a macOS crash report); same if *any* candidate dir is unreadable, e.g. `./data` |

Homebrew paths were checked by simulation. The installed Homebrew knoodle 1.0.6
resolves its keg's `share/knoodle/Klut`, and that copy is byte-identical to
`data/Klut` at HEAD. Linuxbrew under `/home/linuxbrew` takes the same code path
through `/proc/self/exe`. That was reasoned from the code; no Linux machine was
available.

### 3. Corrupted on disk

| case | how made | keys (probe) | tool today | risk |
|---|---|---|---|---|
| `Klut_Keys_13.bin` truncated to half | `truncate -s 9988459` | 1,536,686 error | `ERROR: ... Problem while reading 768344-th key`; `NotFound`; exit 1 | loud but late |
| `Klut_Keys_13.bin` minus 1 byte | `truncate -s 19976917` | 1 not_found, **no error** | last key of 13n5110 → `NotFound`; exit 1, **exit 0 with `--quiet`** | silent: reader's check-before-read bug |
| `Klut_Values_13.tsv` truncated to half | `truncate -s 339161` | 1,105,668 not_found, **no error** | `NotFound` for the second half of the table; 0 library errors; exit 1, **0 with `--quiet`** | silent: parse stop ends the table |
| count digit `9`→`8` (one bit) at line 1973 | edit `\t9`→`\t8` | **31,841 WRONG**, 1 not_found | targeted set: **`K[13,601,1,"m/mr"]` for `"e/r"`** (the mirror image) plus one `NotFound`, exit 1, **0 with `--quiet`**. 30-sample set: 30/30 right, exit 0 (each knot's *first* key is unaffected; its *last* key is filed under the next knot) | **wrong names, silent** |
| count digit `4`→`5` (one bit) at line 1 | edit `\t64`→`\t65` | **33,813 WRONG** | **4 of 7 wrong, exit 0** (e.g. 13a4622 → `K[13,4621,1,"mr"]`) | **wrong names, silent**; one key short is never detected |
| name digit `187`→`186` (one bit) | edit line 16908 | **256 WRONG** | **`K[13,186,0,"m/mr"]` for 13n187, exit 0** | **wrong name, silent** |
| alternating flag `0`→`1` (one bit) | edit line 33814 | **2 WRONG** | **`K[13,5110,1,...]` (alternating) for 13n5110, exit 0** | **wrong name, silent** |
| 4 KiB of zeros in `Klut_Values_13.tsv` @300 KiB | `dd`-style zero fill | **1,129,264 WRONG**, 462 not_found | **`K[13,46\0\0…\0,4682,1,"mr"]` (NUL bytes on stdout) and `K[13,188,0,"e/r"]` for 13n187**; exit 1 from a `NotFound`, **0 with `--quiet`** | **wrong names, silent**: NULs glue ~200 lines into one name and shift every later id |
| 4 KiB of zeros in `Klut_Keys_13.bin` @8 MiB | zero fill | 316 not_found | 7/7 right (Reapr escalation finds another key) | low |
| 1000 random one-bit flips in `Klut_Keys_13.bin` (bit 2; and bit 0) | seeded Python | 999 / 1000 not_found, 0 wrong | 7/7 and 30/30 right | low: lost keys; escalation recovers |
| one duplicated key (`Klut_Keys_03.bin` key 0 := key 1) | Python | **1 WRONG**, 1 not_found | **left trefoil → `K[3,1,1,"e/r"]`**; right trefoil `NotFound[3]` | **wrong name**: duplicates are first-wins |

There is no size, magic-number, checksum or sha256 check anywhere in the read
path. The tarball script checks 2 of the 22 files, for being a pointer and for
size (`make-source-tarball.sh`). Key bit 0 is a crossing's handedness, so a
flipped key is often the code of a real diagram. In 1,250 flips none collided
with an existing key. But a collision is first-wins, as the duplicate case
shows. A flipped key that matches a non-minimal diagram's code would also name
that diagram wrongly.

### 4. Version skew

**What exists today.** There is no format version, header or manifest, and
nothing ties a table set to a Knoodle version. The files are a bare TSV of
`name<TAB>count` and raw key bytes.

**What has happened so far: nothing.** `data/Klut` was committed once
(02b9ba5b, 2026-05-03). Its tree hash is `2aa65b1b` at v1.0.0, v1.0.3, v1.0.6,
v1.1.0 and HEAD. So a "v1.0.x table set" and a "v1.1 table set" are the same
bytes. The reader's parsing is unchanged since then, apart from bookkeeping.
Verified both ways:

- the installed v1.0.6 binary reads HEAD's tables: 7/7 right;
- the HEAD binary reads the brew 1.0.6 keg's tables, which `cmp` shows to be
  identical.

**What would happen.** Three future changes are plausible, and none would be
detected:

| future change | simulated by | result |
|---|---|---|
| tables regenerated with a different knot order, or keys from one generation paired with values from another | names permuted among lines with equal counts (file sizes and key counts unchanged) | **1,402,193 of 1,536,686 13-crossing keys WRONG (91%); 6 of 7 wrong, exit 0, no message; v1.0.6 binary identical** |
| table files replaced while a long run is going (`git lfs pull`, `git checkout`, rsync to a cluster) | start on table set A, identify a trefoil, swap `Klut_Values_13.tsv`, identify 13n5110 | names come from A (read at startup), ids from B (read on first use): **`K[13,3804,1,"mr"]`**; with a shorter A, **`KnotSymbol[13,0,False,"?"]`** counted as identified; exit 0 |
| `Klut::max_crossing_count` raised to 16 with 13-crossing tables | (code reading) | the missing 14–16 subtables fail to load, so `Klut::error` → `NotFound[14..16]` instead of `Unidentified` (the paclet's `TODO.md` already records this quirk) |
| a change to `WriteMacLeodCode`'s canonical form in `src/` | (code reading) | old keys stop matching: mass `NotFound`, or wrong names wherever an old key equals a new code |
| a new on-disk format (e.g. the parked front-coded `klut-fst-experiment`) read by an old binary | (code reading) | the old reader would misparse it silently |

### 5. Other ways it can go wrong

- **`--quiet` changes the verdict.** `NotFound` is an error (exit 1) without
  `--quiet` and a clean answer (exit 0) with it. Every silent-table case above
  becomes exit 0.
- **The paclet ignores exit codes.**
  - `runIdentify` (`Knoodle-paclet/Kernel/Knoodle.wl:1097`) calls
    `RunProcess[..., "StandardOutput"]` and parses whatever is there.
  - A nonzero exit and its stderr are discarded, so `NotFound` and wrong names
    reach the user as an `Association`.
  - When stdout is empty, the user sees `KnoodleIdentify::failed`, which blames
    "a degenerate projection or an invalid diagram".
- **The paclet's fallback directory.** `$KnoodleDataDirectory` falls back to
  `/Users/jasoncantarella/Knoodle/data/Klut` when the `KlutData` asset is
  missing. On JHC's machine that silently uses a dev checkout's tables, which
  could be pointers or another version. Elsewhere it gives the misleading
  message above.
- **The paclet builds copy tables unchecked.**
  - `ci/build-paclet.sh` runs `git lfs pull`, then copies
    `Klut_Keys_*.bin` and `Klut_Values_*.tsv`, with no pointer, size or hash
    check.
  - `ci/release.sh` takes the tables from the release tarball, which is checked
    for 2 of 22 files.
  - A manifest file would not be copied by either script as written.
- **The install tests prove less than they claim.**
  - The formula's test says that running on an empty input "fully loads the
    KLUT at startup … so it exits 0 only if every Klut_*_NN file installed and
    loads". That is false: an empty input loads nothing.
  - Verified: exit 0 with all 11 key files deleted, with `Keys_13` empty, and
    with the permuted and zero-filled values files.
  - `release_tarball_check`'s "knoodleidentify finds the KLUT next to itself"
    is the same check, with the same blind spot.
- **`--max-crossings=N` with N < 13.** Knots above N come back as `NotFound`
  with "table gap?" and exit 1. Example: 12-crossing 12a39 with
  `--max-crossings=10`. The cause is that `IdentifyParams::max_cx` is not set
  from the flag. The paclet documents this behaviour, so changing it is a
  choice.
- **The tool doesn't say which tables it used.** The stderr summary never names
  the directory. Combined with the silent `$KNOODLE_KLUT_DIR` fall-through and
  the current-directory fallback, a user cannot tell which table set answered.
- **The docs are wrong about the search order.** `--help` and the README omit
  `share/knoodle/Klut`. The brief and `CLAUDE.md` give the order as "in-tree,
  then `--data-dir`, then `$KNOODLE_KLUT_DIR`". `CLAUDE.md` is untracked, so
  fixing it is a local edit.

## Proposed fixes

The sizes are rough. The checks are designed so that a table problem stops the
tool **before it reads any input**: exit 1, empty stdout, and a message naming
the directory, the file and what is wrong. That is the only placement the paclet
cannot misread, even before P1 lands.

### `tools/` (ours)

| id | fix | catches | size |
|---|---|---|---|
| **T1** | **Startup structural check**, replacing `KlutTablesAreReal`. For each c ≤ `--max-crossings`: both files present and readable (non-throwing calls). Strict values parse: each line is `name<TAB>count<LF>`, printable ASCII, name matches `K[c,i,j,"coset"]` for this c, count ≥ 1. Then `size(keys) == c × Σcount` exactly. Keep the LFS-pointer message as a special case. Cost ~5 ms: it reads the 0.85 MB of values and stats the keys. | missing, empty, unreadable, LFS pointer, truncation of either file, count flips, NULs and zero-fill in values, partial sets | ~150 lines + test |
| T1b | *(optional, JHC's call: costs ~55 ms per run)* **Eager load.** Call `RequireSubtables()` at startup, so that any load failure happens before output. Then compare each subtable's unique-key count (rows of the public `Klut::SubtableCodes(c)`) with Σcount. A shortfall means duplicate keys, which T1 cannot see from file sizes. This is the tools-side alternative to S4. | duplicate keys, any lazy-load failure | ~25 lines |
| **T2** | **One source of names.** Use the names parsed by T1, the same parse that fixes the ids, and drop `LoadNames`' second read. Make `NameOf`'s fallback an error (`Invalid[]` plus `LogError`), never `KnotSymbol[c,0,False,"?"]`. | mid-run swaps, a names/ids mismatch | ~30 lines |
| **T3** | **`Klut::error` is a table error, not a miss.** `klut_identify.hpp` stops escalating, the summand becomes `Invalid[]` with a "table failed to load" error, and the tool exits nonzero. | anything that fails to load lazily | ~20 lines |
| **T4** | **`--quiet` must not change the exit status.** `NotFound` still counts toward the exit code; only the message is suppressed. | silent tables under `--quiet` | ~3 lines |
| **T5** | **No crash on unreadable directories.** Use `error_code` overloads in `ResolveDataDir`, and report "permission denied: DIR". | the exit-134 crash | ~10 lines |
| **T6** | **Say which tables answered.** Add the table dir to the stderr summary. A set but wrong `$KNOODLE_KLUT_DIR` becomes an error, like `--data-dir`. Warn when the current-directory fallback is used, or drop that fallback. | unannounced stray table sets | ~20 lines |
| **T7** | **Manifest and `--verify-tables`.** Add `data/Klut/Klut_Manifest.txt`. It must not be named `*.tsv` or `*.bin`, which `.gitattributes` sends to LFS. It holds a format version, a table-set id, and each file's size, key count and sha256. The tool always checks sizes and counts against it. It checks sha256 with `--verify-tables` (≈50 ms for 23 MB), or on every run (JHC's call). The binary records at build time the table-set id it was built with, and refuses another set unless given `--allow-other-tables`. Roll out lenient for one release (warn if absent), because the paclet scripts don't copy the manifest yet. | everything above, including name-digit flips, permuted or regenerated tables, key bit flips, version skew | ~250 lines incl. a dependency-free SHA-256, a generator script and tests |
| **T8** | **Install checks that load the tables.** `release_tarball_check` runs `knoodleidentify --verify-tables` (until T7: identify the trefoil and a 13-crossing knot and check the names). `make-source-tarball.sh` verifies all 22 files against the manifest instead of checking 2. | broken release artifacts | ~40 lines |
| T9 | *(optional)* Set `params.max_cx` from `--max-crossings`, so knots above N are `Unidentified`, not "table gap" `NotFound`. Needs a paclet doc change. | misleading `NotFound` | ~5 lines |
| T10 | **Docs.** Fix the `--help` and README search order (add `share/knoodle/Klut`, state that `--data-dir` is exclusive), document the new checks, and fix the local `CLAUDE.md` line. | | small |

Tests for T1–T8 go in a new `test/klut_table_check.py`, with a `manifest.tsv`
row, alongside `klut_lfs_pointer_check.py` or merged into it. It would rebuild
each damaged copy above from `data/Klut` in a temp dir, and assert exit 1, empty
stdout and a message naming the file. It needs the real tables, so it skips with
a note in no-LFS CI checkouts, as the in-tree part of
`klut_lfs_pointer_check` already does. The pointer-only cases still run in CI.

### `src/` (Henrik's; one small PR)

These are defence in depth: with T1, T3 and T7 in place, the tools never hand
`Klut` a table these bugs could hurt. They are still outright bugs in
`Subtable::Read`, and other `Klut` users (tests, benches, the paclet if it ever
links the library) hit them. Following the collaboration norms, they would go as
one curated minimal diff on an experimental branch, with a demo, at merge time.

| id | bug | fix | size |
|---|---|---|---|
| S1 | stream checked before each key read, never after: one missing key is inserted as a short key, undetected (count +1 → 33,813 wrong; keys −1 byte silent) | `if (!k_stream.read(...)) { fail }` | 2 lines |
| S2 | a count that fails to parse ends the table silently and marks it loaded (half the table dropped; zero-fill → 1.13M wrong) | fail unless the values stream ends at clean EOF | ~5 lines |
| S3 | keys left after the last count are ignored | fail unless `k_stream.peek() == EOF` | 2 lines |
| S4 | duplicate keys are first-wins (left trefoil → right trefoil) | fail when `lut.insert(...).second` is false | 3 lines |
| S5 | constructor marks missing files failed without saying so; uses throwing `fs::exists` | `eprint` the missing file; non-throwing `exists` | 3 lines |

### Homebrew formula (`cantarellalab`, downstream)

| id | fix | size |
|---|---|---|
| F1 | Fix the test and its comment. After the Knoodle release that ships T7, run `knoodleidentify --verify-tables`. Before that, identify a trefoil and a 13-crossing knot and assert their names, which loads subtables 3 and 13. | ~10 lines |
| F2 | Nothing to change in the paths: the keg layout, symlinks and symlinked prefix all resolve. Homebrew's tarball sha256 already covers corruption in transit. | — |

### Mathematica paclet (`Knoodle-paclet`, downstream)

| id | fix | size |
|---|---|---|
| P1 | `runIdentify` (and the other `runTool` callers) take `RunProcess`'s full result. On a nonzero exit, issue a message carrying the tool's `Error:` lines and return `$Failed`. This is the paclet's half of the fail-loud contract, for all three tools. | ~20 lines |
| P2 | Drop the hard-coded `/Users/jasoncantarella/Knoodle/data/Klut` fallback in release builds (keep it behind a dev switch), and say "KLUT asset missing". | ~5 lines |
| P3 | Copy the manifest with the tables, and run `knoodleidentify --verify-tables` against the staged `Resources/Klut` in `build-paclet.sh` and `release.sh`. | ~5 lines |

### Suggested batches and open questions

- **Batch A** is `tools/` only, with no format change and no downstream
  dependency: T1–T6, T8 (in its pre-manifest form), T10, and the test. It
  turns every missing, empty, unreadable or truncated table into an error that
  comes before any output. It also catches every count corruption and
  zero-fill found here. Four silent wrong-name cases remain:
  - duplicate keys, which need T1b or S4;
  - name-digit flips, permuted or regenerated tables, and key bit flips, which
    only a hash (T7) can see.

  The S1–S5 PR goes to Henrik after Batch A.
- **Batch B** is T7, F1 and P3. Together they ship the manifest and
  `--verify-tables`, so they need a release and the formula and paclet updates.
- **P1 and P2** are independent and can land any time. P1 matters most
  downstream.

Questions for JHC:

1. Should `NotFound` count toward the exit code under `--quiet` (T4)? Without
   `--quiet` it already does.
2. What should every invocation pay for? Today a small knot takes ~10 ms.
   The eager load (T1b) adds ~55 ms, and sha256 (T7) adds ~50 ms. The paclet
   starts one process per call. The alternative is to run these checks only
   with `--verify-tables` and in the install and CI checks.
3. Should the binary refuse a table set it was not built with (T7), or warn?
4. Should `--max-crossings=N` report knots above N as `Unidentified` (T9)?

## Appendix: how to reproduce

Copy the tables to a scratch directory and damage the copy, never `data/Klut`.
Every run below unsets `KNOODLE_KLUT_DIR` and uses `--data-dir`.

```bash
T=$(mktemp -d); cp -R data/Klut $T/clean
mk() { rm -rf $T/$1; cp -R $T/clean $T/$1; }
mk keys13_missing;   rm $T/keys13_missing/Klut_Keys_13.bin
mk keys13_empty;     : > $T/keys13_empty/Klut_Keys_13.bin
mk vals13_empty;     : > $T/vals13_empty/Klut_Values_13.tsv
mk keys13_minus1;    truncate -s 19976917 $T/keys13_minus1/Klut_Keys_13.bin
mk vals13_half;      truncate -s 339161   $T/vals13_half/Klut_Values_13.tsv
mk vals13_count_inc; sed -i '' '1s/\t64$/\t65/' $T/vals13_count_inc/Klut_Values_13.tsv
mk vals13_nameflip;  sed -i '' '16908s/K\[13,187,/K[13,186,/' $T/vals13_nameflip/Klut_Values_13.tsv
mk vals13_zero4k;    python3 -c "import sys;p=sys.argv[1];b=bytearray(open(p,'rb').read());b[307200:311296]=bytes(4096);open(p,'wb').write(b)" $T/vals13_zero4k/Klut_Values_13.tsv
mk perm_dir;         chmod 000 $T/perm_dir        # restore with chmod 755 afterwards
```

The probe (`probe.cpp`) emits inputs whose names are known. `emit` writes the
30-sample set; `emitsel` takes `c:knot:keyidx` triples, with -1 meaning the
last key. Then run the tool and compare its output with the names file:

```bash
knoodleidentify --expanded --data-dir=$T/vals13_nameflip < inputs.tsv; echo "exit $?"
```

On macOS, run anything that may abort (here, the mode-000 directory) with a
`DYLD_INSERT_LIBRARIES` shim whose constructor installs SIGABRT and SIGSEGV
handlers calling `_exit`. Otherwise every run files a crash report.
