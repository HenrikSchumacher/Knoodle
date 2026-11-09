#!/usr/bin/env bash
# Build a self-contained, vendored source tarball of Knoodle for offline/air-gapped
# builds (e.g. the UGA Sapelo2 cluster) and as a GitHub release asset. It bakes in the
# recursive submodules + the vendored deps/ + the real (de-LFS'd) KLUT data, and excludes
# .git, the large test-diagram corpus, and non-build trees -- so it builds with nothing
# but a C++20 compiler, Boost, and SuiteSparse, and `knoodleidentify` finds its KLUT.
#
# Run from the root of a Knoodle checkout that has submodules initialized and Git-LFS
# available. Usage: make-source-tarball.sh [version] [output-dir]
#
# Output is reproducible within a given tar implementation, but NOT byte-identical
# across implementations: GNU tar and bsdtar emit different headers/padding for the
# same content (verified -- no --format flag bridges them). The canonical release
# asset is the one built by .github/workflows/release-tarball.yml, which always runs
# GNU tar; the Homebrew formula pins that sha256. A local build (e.g. bsdtar on macOS)
# is content-identical and builds fine -- ideal for offline/cluster use -- but will
# have a different sha. For a byte-identical artifact, build with GNU tar (the runner,
# a Linux container, or `brew install gnu-tar` -> gtar).
set -euo pipefail

VERSION="${1:-$(git describe --tags --always --dirty 2>/dev/null || echo unknown)}"
OUTDIR="${2:-.}"
NAME="knoodle-${VERSION}-vendored"
TMP="$(mktemp -d)"
STAGE="$TMP/$NAME"
trap 'rm -rf "$TMP"' EXIT

echo "==> Building $NAME"

# 1) Make sure the KLUT LFS objects are real content, not pointer files.
if command -v git-lfs >/dev/null 2>&1; then
  git lfs pull --include="data/Klut/**" >/dev/null 2>&1 || true
fi

# 2) Sanity checks: submodules initialized, KLUT present and de-smudged.
[ -f submodules/Tensors/Tensors.hpp ] || { echo "ERROR: submodules not initialized (git submodule update --init --recursive)"; exit 1; }
[ -f submodules/Tensors/submodules/Tools/Tools.hpp ] || { echo "ERROR: nested submodule Tools missing (need --recursive)"; exit 1; }

# Check all 22 KLUT files, not a sample: if LFS didn't smudge one of them we'd ship
# a pointer stub, and a damaged table can make knoodleidentify name knots wrongly
# (docs/klut-table-failure-audit.md). Each subtable is a pair: Klut_Values_NN.tsv
# lists `name<TAB>count` lines, and Klut_Keys_NN.bin must hold exactly
# NN * (sum of the counts) bytes. This is the structural part of the check
# knoodleidentify itself makes at startup; release_tarball_check then runs the
# built tool against the unpacked tables.
file_size() { stat -f '%z' "$1" 2>/dev/null || stat -c '%s' "$1" 2>/dev/null || echo 0; }
for c in 03 04 05 06 07 08 09 10 11 12 13; do
  keys="data/Klut/Klut_Keys_$c.bin"; values="data/Klut/Klut_Values_$c.tsv"
  for f in "$keys" "$values"; do
    [ -f "$f" ] || { echo "ERROR: missing $f -- run: git submodule update --init --recursive && git lfs pull"; exit 1; }
    if head -c 64 "$f" | grep -q 'git-lfs'; then
      echo "ERROR: $f is a Git-LFS pointer, not content. Run: git lfs pull"; exit 1
    fi
  done
  total="$(awk -F'\t' 'NF != 2 || $2 !~ /^[1-9][0-9]*$/ { bad = 1 } { s += $2 } END { print (bad ? -1 : s) }' "$values")"
  [ "$total" -gt 0 ] 2>/dev/null || { echo "ERROR: $values is not a list of name<TAB>count lines."; exit 1; }
  want=$(( total * 10#$c )); have="$(file_size "$keys")"
  if [ "$have" != "$want" ]; then
    echo "ERROR: $keys holds $have bytes, but $values lists $total keys of $((10#$c)) bytes ($want bytes)."; exit 1
  fi
done

# 3) Stage the build-relevant trees (validated minimal set) + the runtime KLUT.
# This is an ALLOWLIST: only the trees named here ship. `test/` and `docs/` are
# intentionally excluded -- test/ holds the whole testing process (correctness,
# robustness, and performance/benchmark drivers) plus its OWN vendored, test-only
# deps (test/vendor/libhomfly, test/vendor/plantri) and Python venv, none of which
# belong in a lean point-release tarball. Keep test-only vendored deps under
# test/vendor/, NEVER in the shipped deps/ tree, and do not add test/ or docs/ here.
#
# ONE list, used by both the copy here and the tracked-file filter in step 4, so the
# two cannot drift apart. (They did: v1.1.0 found both still naming PolyFold/ and
# Reapr_Legacy.hpp, months after the Grand cleanup moved one to devel/PolyFold and
# deleted the other -- and `cp -a` of a missing path kills the release workflow.)
# Paths keep their place in the tree, so devel/PolyFold ships as devel/PolyFold
# and its makefile's ../../ include paths still resolve.
SHIP=(tools devel/PolyFold src legacy deps submodules data/Klut
      Knoodle.hpp INSTALL_HPC_MODULES.sh README.md LICENSE CITATION.cff)
for p in "${SHIP[@]}"; do
  [ -e "$p" ] || { echo "ERROR: '$p' is in the tarball allowlist but not in this checkout -- was it moved or deleted? Update SHIP in $0."; exit 1; }
done
for p in "${SHIP[@]}"; do
  mkdir -p "$STAGE/$(dirname "$p")"
  cp -a "$p" "$STAGE/$(dirname "$p")/"
done

# 4) Keep ONLY git-tracked files (allowlist, not denylist). cp -a stages the whole
# working tree, so a dirty checkout could otherwise carry in untracked/ignored cruft
# -- build outputs, logs, editor/agent dirs (.claude), .DS_Store, etc. Rather than
# chase each kind, we delete every staged path that neither the superproject nor any
# submodule tracks. This is structural: nothing that isn't `git add`-ed can ship.
# (data/diagrams is never copied; the generated VERSION is added in step 4b, after.)
TRACKED="$TMP/tracked.txt"
{
  git ls-files -- "${SHIP[@]}"
  # Each submodule lists its own tracked files; prefix with its path in the superproject.
  git submodule foreach --recursive --quiet 'git ls-files | sed "s#^#$displaypath/#"'
} | LC_ALL=C sort -u > "$TRACKED"

# Drop submodule .git links/dirs first (untracked + huge to walk file-by-file).
find "$STAGE" -depth -name .git -exec rm -rf {} +

# Delete any staged file/symlink that is not in the tracked set, then prune empty dirs.
( cd "$STAGE" && find . \( -type f -o -type l \) -print ) | sed 's#^\./##' \
  | LC_ALL=C sort -u > "$TMP/staged.txt"
comm -23 "$TMP/staged.txt" "$TRACKED" | while IFS= read -r rel; do
  [ -n "$rel" ] && rm -f "$STAGE/$rel"
done
find "$STAGE" -depth -type d -empty -delete

# 4b) Add the generated VERSION marker (consumed by the makefiles' ../VERSION fallback).
printf '%s\n' "$VERSION" > "$STAGE/VERSION"

# 5) Roll the tarball. Reproducible across re-runs (stable sha256) so the workflow's
# `gh release upload --clobber` can't silently change an asset that a Homebrew formula
# has already pinned: normalize every mtime, emit entries in a stable sorted order,
# and gzip with -n so no timestamp is baked into the gzip header. Done with portable
# primitives (touch / sorted find -T / gzip) so it works the same under GNU tar on the
# CI runner and bsdtar on macOS -- avoiding GNU-only flags like --sort/--mtime.
#
# --no-recursion is essential: the find list already contains every directory AND
# file, so tar must archive each listed path exactly once. Without it, tar recurses
# into each listed directory and re-archives its contents -- duplicating every file
# once per ancestor dir (bsdtar balloons a 34 MB tree to ~140 MB / 5x the entries;
# GNU tar dedups, so the bug was invisible on the Linux runner). Both tars honor it.
#
# COPYFILE_DISABLE=1: macOS bsdtar otherwise writes an AppleDouble "._<name>" entry
# for every file carrying extended attributes (provenance, quarantine, ...), which
# unpack on Linux as stray files. test/release_tarball_check caught this at v1.1.0.
# GNU tar ignores the variable, so the canonical (runner-built) asset is unchanged.
find "$STAGE" -exec touch -h -t 200001010000.00 {} +
OUTDIR_ABS="$(cd "$OUTDIR" && pwd)"
( cd "$TMP" \
  && find "$NAME" -print0 | LC_ALL=C sort -z \
     | COPYFILE_DISABLE=1 tar --no-recursion --numeric-owner --owner=0 --group=0 --null -T - -cf - ) \
  | gzip -n -9 > "$OUTDIR_ABS/$NAME.tar.gz"

echo "==> Wrote $OUTDIR/$NAME.tar.gz  ($(du -h "$OUTDIR/$NAME.tar.gz" | cut -f1))"
echo "    top-level dir: $NAME/   (KLUT files: $(ls "$STAGE/data/Klut" | wc -l | tr -d ' '))"
