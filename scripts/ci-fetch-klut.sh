#!/usr/bin/env bash
#
# Fetch the KLUT tables (data/Klut, ~24 MB, stored with Git LFS) for CI.
#
# Only data/Klut: the light tier needs the tables (knoodleidentify, klut_e2e,
# key_roundtrip_probe) but not data/diagrams, whose randomunknots/ alone is
# ~104 MB. Anything else stored in LFS stays a pointer in CI, so a test file
# that was LFS-tracked by accident still fails there instead of being masked.
#
# The workflow restores .git/lfs/objects from actions/cache first where it can,
# so this usually only checks the objects out instead of downloading them.
#
# Fails here, naming the file, if any table is still a pointer afterwards --
# otherwise every identify check would fail later with a misleading NotFound.

set -euo pipefail

cd "$(dirname "$0")/.."

git lfs install --local --skip-repo   # filters only: no hooks (ours live in core.hooksPath)
git lfs pull --include="data/Klut/**"

bad=0
count=0
for f in data/Klut/*; do
    count=$((count + 1))
    if head -c 24 "$f" | grep -q '^version https://git-lfs'; then
        echo "ci-fetch-klut: $f is still a Git LFS pointer" >&2
        bad=1
    fi
done

if [ "$count" -eq 0 ]; then
    echo "ci-fetch-klut: data/Klut is empty" >&2
    exit 1
fi
[ "$bad" -eq 0 ] || exit 1

echo "ci-fetch-klut: ${count} KLUT files present"
