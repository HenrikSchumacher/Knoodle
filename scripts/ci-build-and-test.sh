#!/usr/bin/env bash
#
# CI build + light-tier test for the Knoodle command-line tools.
#
# Mirrors the *build path* the designbynumbers/cantarellalab Homebrew formula
# uses (the tools/Makefile), but with native (apt/dnf/brew-deps-only) toolchains
# instead of `brew install knoodle` -- so portability breaks surface here, in the
# source repo, before they reach a downstream PR. This is the "can we build the
# CLI tools and run our internal tests" half; the brew formula's own CI keeps the
# packaging-specific checks (e.g. KLUT installed to the right pkgshare).
#
# Light tier only: builds the three tools via the Makefile, smoke-runs --help,
# then builds + runs the self-contained test drivers that need NO Git-LFS data
# and NO UMFPACK/BLAS (homfly_check, component_check, plantri_check, and the
# pure-Python cli_stdin_check). The heavy UMFPACK/KLUT tier is intentionally out
# of scope here.
#
# Per-leg configuration comes from the environment (set by the workflow matrix):
#   CI_CXX           compiler override passed to make as CXX= (empty => let the
#                    Makefile pick: clang++ on macOS, g++ on Linux). On Linux+clang
#                    this must carry -DTOOLS_NO_RESTRICT, e.g.
#                    CI_CXX="clang++ -DTOOLS_NO_RESTRICT".
#   HOMEBREW_PREFIX  prefix the tools/Makefile searches for Boost + SuiteSparse
#                    headers/libs (/opt/homebrew on mac, /usr on native Linux).
#
set -euxo pipefail

cd "$(dirname "$0")/.."

# Boost + SuiteSparse live under HOMEBREW_PREFIX for the Makefile's -I/-L flags.
# Default to brew's prefix on macOS, else /usr for native apt/dnf installs.
export HOMEBREW_PREFIX="${HOMEBREW_PREFIX:-$(brew --prefix 2>/dev/null || echo /usr)}"

# make CXX= override, if any. Guarded expansion (${arr[@]+...}) keeps an empty
# array from tripping `set -u` under macOS's bash 3.2.
MK=()
[ -n "${CI_CXX:-}" ] && MK+=("CXX=${CI_CXX}")

# 1. Build the three CLI tools via the Makefile -- the brew formula's build path.
make -C tools all ${MK[@]+"${MK[@]}"}

# 2. Smoke test: every tool prints --help and exits 0 (does not touch the KLUT,
#    so this is valid without the Git-LFS data).
for t in knoodlesimplify knoodledraw knoodleidentify; do
    echo "== ${t} --help =="
    "tools/${t}" --help >/dev/null
done

# 3. Light self-contained test drivers (no Git-LFS, no UMFPACK/BLAS). These build
#    from the test/Makefile; the light drivers ignore HOMEBREW_PREFIX.
make -C test component_check homfly_check plantri_check ${MK[@]+"${MK[@]}"}

# Run the drivers from test/: plantri_check resolves its vendored plantri binary
# relative to the CWD (vendor/plantri/plantri), and cli_stdin_check reaches the
# tools via ../tools.
cd test

echo "== component_check (link-component preservation reproducer) =="
./component_check

echo "== homfly_check (vendored-libhomfly correctness panel) =="
./homfly_check

echo "== plantri_check (exhaustive diagrams, HOMFLY invariant under Simplify, c<=6) =="
./plantri_check --up-to-crossing=6

if command -v python3 >/dev/null 2>&1; then
    echo "== cli_stdin_check (interactive-stdin notice) =="
    python3 cli_stdin_check.py
else
    echo "== cli_stdin_check SKIPPED (no python3) =="
fi

echo "=== CI build + light-tier tests: PASS ==="
