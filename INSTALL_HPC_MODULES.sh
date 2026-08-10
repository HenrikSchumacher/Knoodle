#!/usr/bin/env bash
# Build Knoodle's tools on an air-gapped HPC cluster that provides its libraries
# through environment modules (Lmod / EasyBuild) rather than Homebrew or system
# packages. Run this from the root of an unpacked vendored source tarball.
#
# It expects EasyBuild-style modules: `module load Name/version-toolchain` and the
# `$EBROOT<NAME>` root variables they export. The module NAMES AND VERSIONS BELOW ARE
# AN EXAMPLE from one validated site -- discover your site's equivalents with
# `module avail` / `module spider clang boost suitesparse`. What must hold, whatever
# the exact names:
#   * a C++20 compiler -- clang is the safe default (see the restrict note below),
#   * Boost (>= 1.81 to keep the unordered-map speedup; older is fine, see below),
#   * SuiteSparse (provides UMFPACK).
# Keep all three on the SAME toolchain generation (e.g. all foss-2024a / GCCcore-13.3.0)
# so their ABIs match.
set -euo pipefail
cd "$(dirname "$0")"

# --- 0) Compile on a compute node, not the login node (optional) ----------------
#   Use your scheduler's interactive-session command, e.g.
#     salloc --ntasks=4 --mem=8G --time=1:00:00
#     srun --pty --ntasks=4 --mem=8G --time=1:00:00 bash
#   (some sites wrap this as `interact`, `qlogin`, etc.)

# --- 1) Load the module stack (EDIT to match your site; keep one toolchain) ------
module purge
module load Clang/18.1.8-GCCcore-13.3.0     # any C++20 clang; see restrict note below
module load Boost/1.85.0-GCC-13.3.0
module load SuiteSparse/7.10.1-foss-2024a   # provides UMFPACK

echo "==> compiler: $(clang++ --version | head -1)"
echo "==> SuiteSparse root (EBROOTSUITESPARSE): ${EBROOTSUITESPARSE:?unset -- adjust the module name above}"

# --- 2) Build -------------------------------------------------------------------
# Flag/override rationale:
#  * CXX="clang++ -DTOOLS_NO_RESTRICT": append the define to CXX, NOT via CXXFLAGS=
#    (which would clobber the makefile's own flags). TOOLS_NO_RESTRICT drops the
#    __restrict qualifier that older clang/gcc (e.g. clang 18, gcc 13) choke on in
#    Tensors' function_traits. Appending to CXX still trips the makefile's
#    `findstring clang` branch, so clang's -fenable-matrix is auto-added.
#  * HOMEBREW_PREFIX=$EBROOTSUITESPARSE: makes the makefile's
#    -I$(HOMEBREW_PREFIX)/include/suitesparse resolve umfpack.h (it lives under
#    include/suitesparse and is typically NOT on $CPATH). Boost headers usually ARE
#    on $CPATH, so no extra -I is needed for Boost.
#  * LDFLAGS="-lboost_program_options -lumfpack": module-provided libs are on
#    $LIBRARY_PATH, so no -L is needed -- just name the two libraries.
#  * If your Boost module is < 1.81, add BOOST_FLAGS= to the make line to fall back
#    to std::unordered (skips the unordered_flat_map optimization).
cd tools
make clean
make CXX="clang++ -DTOOLS_NO_RESTRICT" \
     HOMEBREW_PREFIX="$EBROOTSUITESPARSE" \
     LDFLAGS="-lboost_program_options -lumfpack"

# --- 3) Verify ------------------------------------------------------------------
echo; echo "==> built binaries:"
ls -la knoodlesimplify knoodleidentify knoodledraw
command -v file >/dev/null && file knoodlesimplify
echo; echo "==> smoke run (no KLUT needed):"
./knoodlesimplify --help | head -5
./knoodlesimplify ExampleKnot.tsv | head   # real simplify pipeline on the shipped sample
# knoodleidentify additionally needs the KLUT, baked into this tarball at ../data/Klut.

echo; echo "Done. A clean build + a native ELF that runs == success on this cluster."

# --- Notes ----------------------------------------------------------------------
# * gcc path: TOOLS_NO_RESTRICT also strips __restrict for gcc, so a sufficiently
#   recent gcc works too -- `module load` your GCC + Boost + SuiteSparse (same
#   toolchain) and use CXX="g++ -DTOOLS_NO_RESTRICT". clang is the validated default.
# * To reconfirm the restrict issue on an older compiler, build once WITHOUT the flag
#   (CXX="clang++") and watch for the `function_traits<... __restrict &>` error, then
#   rebuild WITH it.
