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
#   CI_NO_BOOST      set to 1 to build the tools WITHOUT boost::unordered_flat_map
#                    (make BOOST_FLAGS=). Needed where the system Boost is < 1.81
#                    (no flat_map) -- e.g. Rocky 9's Boost 1.75, mirroring how the
#                    Sapelo2 cluster builds. Recent Boost (macOS/Fedora/Ubuntu) keeps
#                    it on, matching the Homebrew formula.
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
# Old Boost (< 1.81) has no unordered_flat_map; drop the boost define there.
[ "${CI_NO_BOOST:-}" = 1 ] && MK+=("BOOST_FLAGS=")

# 1. Build the three CLI tools via the Makefile -- the brew formula's build path.
make -C tools all ${MK[@]+"${MK[@]}"}

# 2. Smoke test: every tool prints --help and exits 0 (does not touch the KLUT,
#    so this is valid without the Git-LFS data).
for t in knoodlesimplify knoodledraw knoodleidentify; do
    echo "== ${t} --help =="
    "tools/${t}" --help >/dev/null
done

# 3. The light tier, driven from test/manifest.tsv -- the SAME definition
#    `make check` uses locally and the pre-push hook will use. Before this,
#    CI kept its own hand-written list of three drivers, which is two
#    definitions of "light tier" and they drift.
#
#    CI cannot run all of it, and says so rather than pretending:
#
#      --exclude-needs   the UMFPACK/BLAS link shapes. Several CI images have
#                        no SuiteSparse, and the light drivers are the ones
#                        that build without it.
#      --exclude         tests that need the Git-LFS data (data/Klut,
#                        data/diagrams), which CI does not fetch; plus
#                        cli_stdin_check, which simulates a terminal with a pty
#                        and is timing/TTY-sensitive in headless CI.
#
#    run_tier.py lists every skip in its summary, so a CI pass never reads as
#    a full-tier pass.

CI_EXCLUDE_NEEDS="umfpack,homfly_umfpack,boost_umfpack"
CI_EXCLUDE="cli_stdin_check,klut_e2e,key_roundtrip_probe,run_tests"

# EXCLUDE_NEEDS filters what `all` BUILDS, using the same manifest column
# run_tier.py filters what it RUNS -- so the two cannot disagree.
make -C test all EXCLUDE_NEEDS="${CI_EXCLUDE_NEEDS}" ${MK[@]+"${MK[@]}"}
make -C test lint

cd test
python3 run_tier.py --tier=light \
    --exclude-needs="${CI_EXCLUDE_NEEDS}" \
    --exclude="${CI_EXCLUDE}"

echo "=== CI build + light-tier tests: PASS ==="
