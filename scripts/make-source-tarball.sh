#!/usr/bin/env bash
# Build a self-contained, vendored source tarball of Knoodle for offline/air-gapped
# builds (e.g. the UGA Sapelo2 cluster) and as a GitHub release asset. It bakes in the
# recursive submodules + the vendored deps/ + the real (de-LFS'd) KLUT data, and excludes
# .git, the large test-diagram corpus, and non-build trees -- so it builds with nothing
# but a C++20 compiler, Boost, and SuiteSparse, and `knoodleidentify` finds its KLUT.
#
# Run from the root of a Knoodle checkout that has submodules initialized and Git-LFS
# available. Usage: make-source-tarball.sh [version] [output-dir]
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

# The bulk of the KLUT is the .bin keys (Klut_Keys_13.bin alone is ~20 MB), so a
# .tsv-only check is not enough: if LFS didn't smudge the .bin we'd ship pointer
# stubs and a silently broken knoodleidentify. Verify a representative .tsv AND the
# largest .bin -- both must exist, not be Git-LFS pointers, and the big .bin must be
# real-sized (a pointer is ~130 bytes; ~20 MB expected).
file_size() { stat -f '%z' "$1" 2>/dev/null || stat -c '%s' "$1" 2>/dev/null || echo 0; }
for f in data/Klut/Klut_Values_03.tsv data/Klut/Klut_Keys_13.bin; do
  [ -f "$f" ] || { echo "ERROR: missing $f -- run: git submodule update --init --recursive && git lfs pull"; exit 1; }
  if head -c 64 "$f" | grep -q 'git-lfs'; then
    echo "ERROR: $f is a Git-LFS pointer, not content. Run: git lfs pull"; exit 1
  fi
done
keys_sz="$(file_size data/Klut/Klut_Keys_13.bin)"
if [ "${keys_sz:-0}" -lt 1000000 ]; then
  echo "ERROR: data/Klut/Klut_Keys_13.bin is only ${keys_sz} bytes (expected ~20 MB); the KLUT keys are not fully present (LFS not pulled?)."; exit 1
fi

# 3) Stage the build-relevant trees (validated minimal set) + the runtime KLUT.
mkdir -p "$STAGE/data"
cp -a PolyFold tools src legacy deps submodules "$STAGE"/
cp -a data/Klut "$STAGE/data/"
cp -a Knoodle.hpp Reapr_Legacy.hpp "$STAGE"/

# 4) Keep ONLY git-tracked files (allowlist, not denylist). cp -a stages the whole
# working tree, so a dirty checkout could otherwise carry in untracked/ignored cruft
# -- build outputs, logs, editor/agent dirs (.claude), .DS_Store, etc. Rather than
# chase each kind, we delete every staged path that neither the superproject nor any
# submodule tracks. This is structural: nothing that isn't `git add`-ed can ship.
# (data/diagrams is never copied; the generated VERSION is added in step 4b, after.)
TRACKED="$TMP/tracked.txt"
{
  git ls-files -- PolyFold tools src legacy deps Knoodle.hpp Reapr_Legacy.hpp data/Klut
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
find "$STAGE" -exec touch -h -t 200001010000.00 {} +
OUTDIR_ABS="$(cd "$OUTDIR" && pwd)"
( cd "$TMP" \
  && find "$NAME" -print0 | LC_ALL=C sort -z \
     | tar --no-recursion --numeric-owner --owner=0 --group=0 --null -T - -cf - ) \
  | gzip -n -9 > "$OUTDIR_ABS/$NAME.tar.gz"

echo "==> Wrote $OUTDIR/$NAME.tar.gz  ($(du -h "$OUTDIR/$NAME.tar.gz" | cut -f1))"
echo "    top-level dir: $NAME/   (KLUT files: $(ls "$STAGE/data/Klut" | wc -l | tr -d ' '))"
