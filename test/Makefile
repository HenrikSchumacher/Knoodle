# Makefile for Knoodle test drivers.
#
# WHAT EXISTS IS DEFINED IN manifest.tsv, NOT HERE. Every build rule below is
# generated from that file, so a target cannot be added to the tree without
# also being built by `all`. Before this, `all` built 2 of 23 binaries and the
# rest were built only when someone named them -- which is how klut_identify_check
# stopped compiling and link_alex_probe started exiting 1 without anyone noticing.
#
# Each test documents itself in the header comment of its own .cpp; that is the
# place to look for what a test does and why it exists.
#
# Targets:
#   all        - every binary in the manifest (the default)
#   lint       - check that the manifest and the tree agree
#   <name>     - any single target from the manifest
#   libhomfly  - just the vendored libhomfly objects
#   clean

UNAME_S := $(shell uname -s)

CXX ?= clang++
CC  ?= cc

# Knoodle uses clang's matrix extension (-fenable-matrix), so clang/LLVM is the
# supported toolchain (macOS clang, or a module-loaded clang/LLVM on a cluster).
# Detect clang by running the compiler (robust to the binary's name -- it may be
# `c++`, `clang++`, or a module-provided name), so a clang build gets the flag on
# any OS; a gcc build (unsupported by the core) simply omits it and fails loudly.
# MARCH defaults to `native` (tune to the build host). On a cluster where the
# login and compute nodes differ, override it with a safe baseline (e.g.
# `make MARCH=x86-64-v3`) so the binary doesn't hit illegal instructions.
MARCH ?= native
CXX_IS_CLANG := $(shell $(CXX) --version 2>/dev/null | grep -ci clang)
ARCHFLAGS = -march=$(MARCH) -mtune=$(MARCH)
ifneq ($(CXX_IS_CLANG),0)
    ARCHFLAGS += -fenable-matrix
endif

# -MMD -MP: emit a .d file per driver listing every header it actually
# included, so editing a Knoodle header rebuilds the tests that use it. Hand-
# written prerequisites cannot keep up: the recipes below name ../Knoodle.hpp
# but not src/, so on 2026-08-15 local_moves_check twice reported a pre-fix
# result after Henrik's fix had already been merged. -MP adds phony targets so
# a deleted or renamed header does not wedge the build.
CXXFLAGS = -Wall -Wextra -std=c++20 -O3 $(ARCHFLAGS) -pthread -MMD -MP

# Knoodle core include paths (mirror tools/Makefile). The Homebrew include dir
# is only added when HOMEBREW_PREFIX is set (macOS); on Linux/cluster the
# submodule headers + the toolchain's own headers suffice for this light build.
KNOODLE_INC = -I./../submodules/Min-Cost-Flow-Class/OPTUtils \
              -I./../submodules/Min-Cost-Flow-Class/MCFClass \
              -I./../submodules/Min-Cost-Flow-Class/MCFSimplex \
              -I./../submodules/Tensors
ifdef HOMEBREW_PREFIX
    KNOODLE_INC += -I$(HOMEBREW_PREFIX)/include
endif

# Vendored libhomfly (public domain; gc shimmed to libc — see vendor/libhomfly).
LIBHOMFLY_DIR = vendor/libhomfly
LIBHOMFLY_SRC = $(wildcard $(LIBHOMFLY_DIR)/*.c)
LIBHOMFLY_OBJ = $(patsubst $(LIBHOMFLY_DIR)/%.c,build/libhomfly_%.o,$(LIBHOMFLY_SRC))

# Vendored plantri (Apache 2.0; single translation unit — see vendor/plantri).
PLANTRI = vendor/plantri/plantri

# The UMFPACK link shapes need the Alexander invariant (UMFPACK sparse solver)
# and BLAS/LAPACK. On macOS that is Apple Accelerate; elsewhere use OpenBLAS
# and adjust UMFPACK_LDFLAGS / the force-included header accordingly.
ifeq ($(UNAME_S),Darwin)
    UMFPACK_INC      = -I$(HOMEBREW_PREFIX)/include/suitesparse \
                       -include ../submodules/Tensors/Accelerate.hpp
    UMFPACK_LDFLAGS  = -L$(HOMEBREW_PREFIX)/lib -lumfpack -framework Accelerate
else
    UMFPACK_INC      = -I$(HOMEBREW_PREFIX)/include/suitesparse \
                       -include ../submodules/Tensors/OpenBLAS.hpp
    UMFPACK_LDFLAGS  = -L$(HOMEBREW_PREFIX)/lib -lumfpack -lopenblas
endif

inflate_check: inflate_check.cpp $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building inflate_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    inflate_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ inflate_check compiled successfully"

# error_tap_check — regression test for the CLI tools' fail-loudly machinery
# (cerr error tap + staged/atomic output). Light build, no UMFPACK.
error_tap_check: error_tap_check.cpp ../tools/knoodle_io.hpp ../Knoodle.hpp
	@echo "=== Building error_tap_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) error_tap_check.cpp -o $@
	@echo "✓ error_tap_check compiled successfully"

# embedding_inflation_check — pins that re-aiming a projection is a rigid motion:
# it must not walk the embedding's coordinates away from the origin as the trial
# counts grow. The src headers are explicit deps because the behaviour under test
# lives entirely in src/; without them make would not rebuild for a src-only
# change and the test would appear to pass against unfixed code.
embedding_inflation_check: embedding_inflation_check.cpp ../Knoodle.hpp \
                           ../src/PlanarDiagramComplex/Simplify.hpp \
                           ../src/Reapr/Embedding.hpp \
                           ../src/LinkEmbedding.hpp \
                           ../src/LinkEmbedding/Helpers.hpp
	@echo "=== Building embedding_inflation_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) embedding_inflation_check.cpp -o $@
	@echo "✓ embedding_inflation_check compiled successfully"
# intersection_flag_reset_check — pins that LinkEmbedding's intersection flag
# counters describe one projection, not the embedding's lifetime. Needs the src
# header as an explicit dep: this is a src-only change, and without it make would
# not rebuild and the test would appear to pass against unfixed code.
intersection_flag_reset_check: intersection_flag_reset_check.cpp ../Knoodle.hpp \
                               ../src/LinkEmbedding/FindIntersections.hpp
	@echo "=== Building intersection_flag_reset_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) intersection_flag_reset_check.cpp -o $@
	@echo "✓ intersection_flag_reset_check compiled successfully"

# lock_api_check -- exercise the locking API that had no user: the const-lvalue
# PDC constructor (which was uninstantiable) and the lock state around Push.
lock_api_check: lock_api_check.cpp ../Knoodle.hpp \
                ../src/PlanarDiagramComplex.hpp ../src/PlanarDiagram.hpp
	@echo "=== Building lock_api_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) lock_api_check.cpp -o $@
	@echo "✓ lock_api_check compiled successfully"

# reapr_corner_probe — reproduce/diagnose the FindIntersections CornerCorner
# degeneracy that makes Rattle bail after 10 failed random rotations.
reapr_corner_probe: reapr_corner_probe.cpp ../Knoodle.hpp
	@echo "=== Building reapr_corner_probe on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) reapr_corner_probe.cpp -o $@
	@echo "✓ reapr_corner_probe compiled successfully"

# link_alex_probe — prototype: single-variable Alexander |det| fingerprint for
# LINKS (no knots-only normalization). Same heavy config as inflate_check.
link_alex_probe: link_alex_probe.cpp link_alexander.hpp ../Knoodle.hpp
	@echo "=== Building link_alex_probe on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    link_alex_probe.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ link_alex_probe compiled successfully"

# link_inflate_check — inflate-and-recover stress test for LINKS. Tripwires:
# single-variable Alexander |det| fingerprint + component count, plus a HOMFLY
# cross-check (split-safe oracle homfly_invariance.hpp -> vendored libhomfly).
link_inflate_check: link_inflate_check.cpp link_alexander.hpp homfly_invariance.hpp \
                    $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building link_inflate_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    link_inflate_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ link_inflate_check compiled successfully"

# link_split_check — direct split-link corner-case test (oracle correctness vs
# delta rule, Simplify preservation incl. free loops, |det| graceful degradation).
# intersection3d_check -- the introspection contract from the failing side:
# curves that really do self-intersect in 3-space, with IntersectionCount3D()
# checked against an independent exact-integer census. Exit 0 = pass.
intersection3d_check: intersection3d_check.cpp embedding_fixtures.hpp ../Knoodle.hpp
	@echo "=== Building intersection3d_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) intersection3d_check.cpp -o $@
	@echo "✓ intersection3d_check compiled successfully"

# local_moves_check -- the ArcSimplifier's local patterns must preserve the
# knot. Runs data/diagrams/hardunknots (21 diagrams that are all the UNKNOT)
# through local_opt_level 0, 2 and 4 and requires every recognizable output to
# still be unknotted. Pins upstream issue 12. Exit 0 = pass.
local_moves_check: local_moves_check.cpp homfly_invariance.hpp \
                   $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building local_moves_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    local_moves_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ local_moves_check compiled successfully"

link_split_check: link_split_check.cpp link_alexander.hpp homfly_invariance.hpp \
                  $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building link_split_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    link_split_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ link_split_check compiled successfully"

# embedding_check — degeneracy + performance test for the LinkEmbedding family
# (LinkEmbedding, LinkEmbedding2/3/4 over Prosector2/3/4). Checks that a
# degenerate projection gives the same crossings as the explicit integer shear
# realizing Prosector's {eps,eps^3,1} symbolic perturbation, that 2/3/4 agree
# with each other, and that the knot type survives. Heavy config: HOMFLY needs
# the vendored libhomfly objects, the large-diagram fallback needs Alexander
# (UMFPACK + BLAS/LAPACK).
# ---------------------------------------------------------------------------
# Link shapes. The manifest's `needs` column names one of these; the generated
# rule below expands FLAGS_/LIBS_/DEPS_ for it. There are exactly five, which
# is what the twenty-three hand-written recipes reduced to.
# ---------------------------------------------------------------------------

FLAGS_plain           = $(KNOODLE_INC)
LIBS_plain            =
DEPS_plain            =

FLAGS_homfly          = $(KNOODLE_INC)
LIBS_homfly           = $(LIBHOMFLY_OBJ)
DEPS_homfly           = $(LIBHOMFLY_OBJ)

FLAGS_umfpack         = -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC)
LIBS_umfpack          = $(UMFPACK_LDFLAGS)
DEPS_umfpack          =

FLAGS_homfly_umfpack  = -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC)
LIBS_homfly_umfpack   = $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS)
DEPS_homfly_umfpack   = $(LIBHOMFLY_OBJ)

FLAGS_boost_umfpack   = -DKNOODLE_USE_UMFPACK -DKNOODLE_USE_BOOST_UNORDERED \
                        -DTOOLS_USE_BOOST_UNORDERED $(KNOODLE_INC) $(UMFPACK_INC)
LIBS_boost_umfpack    = $(UMFPACK_LDFLAGS)
DEPS_boost_umfpack    =

# Per-target extra prerequisites, for the handful that need something beyond
# their link shape. Header dependencies are NOT listed anywhere: -MMD -MP
# discovers them, which is why these rules can be generated at all.
EXTRA_DEPS_plantri_check = $(PLANTRI)

# ---------------------------------------------------------------------------
# Rule generation from the manifest.
#
# One shell call reads it into `target:needs:source` triples -- those three
# columns never contain spaces, so they survive make's word splitting. The
# light/heavy argument columns DO contain spaces, and are deliberately not read
# here: only run_tests.py needs them.
# ---------------------------------------------------------------------------

COMMA         := ,
MANIFEST      := manifest.tsv
# kind=script rows (Python) have nothing to compile, so they are filtered out
# here. They are still manifest rows -- the runner and the lint see them -- which
# is the point: cli_stdin_check.py used to sit outside the manifest entirely,
# which is exactly the orphan condition the manifest exists to prevent.
MANIFEST_ROWS := $(shell awk -F'\t' '$$0 !~ /^\#/ && NF>=4 && $$2 != "script" { print $$1":"$$3":"(($$4=="-")?$$1".cpp":$$4) }' $(MANIFEST))
TARGETS       := $(foreach r,$(MANIFEST_ROWS),$(word 1,$(subst :, ,$(r))))

# EXCLUDE_NEEDS lets a platform build only what it can link. CI images without
# SuiteSparse pass EXCLUDE_NEEDS=umfpack,homfly_umfpack,boost_umfpack. Build
# RULES are still generated for every target -- only what `all` builds is
# filtered -- so naming an excluded target explicitly still works and fails
# with a real linker error rather than "no rule to make target".
EXCLUDE_NEEDS ?=
BUILD_TARGETS := $(foreach r,$(MANIFEST_ROWS),\
    $(if $(filter $(word 2,$(subst :, ,$(r))),$(subst $(COMMA), ,$(EXCLUDE_NEEDS))),,\
        $(word 1,$(subst :, ,$(r)))))

define BUILD_RULE
$(1): $(3) $$(DEPS_$(2)) $$(EXTRA_DEPS_$(1))
	@echo "=== Building $(1) on $$(UNAME_S) ==="
	$$(CXX) $$(CXXFLAGS) $$(FLAGS_$(2)) $(3) $$(LIBS_$(2)) -o $$@
	@echo "✓ $(1) compiled successfully"
endef

$(foreach r,$(MANIFEST_ROWS),$(eval $(call BUILD_RULE,$(word 1,$(subst :, ,$(r))),$(word 2,$(subst :, ,$(r))),$(word 3,$(subst :, ,$(r))))))

all: $(BUILD_TARGETS)


# ---------------------------------------------------------------------------
# Infrastructure
# ---------------------------------------------------------------------------

build:
	@mkdir -p build

build/libhomfly_%.o: $(LIBHOMFLY_DIR)/%.c | build
	$(CC) -O2 -w -I$(LIBHOMFLY_DIR) -c $< -o $@

libhomfly: $(LIBHOMFLY_OBJ)

# Vendored plantri binary (plantri_check shells out to it). Single .c file, C
# stdlib only; -w because plantri's own source is not warning-clean and is
# unmodified.
$(PLANTRI): vendor/plantri/plantri.c
	@echo "=== Building vendored plantri on $(UNAME_S) ==="
	$(CC) -O2 -w -o $@ $<
	@echo "✓ plantri compiled successfully"

# cli_stdin_check -- regression test for the interactive-stdin notice (isatty).
# Pure-Python (stdlib only, no Regina/venv); needs the tools built in ../tools.
# Not in the manifest: nothing to compile.
cli_stdin_check:
	@echo "=== Running cli_stdin_check (interactive-stdin notice) ==="
	python3 cli_stdin_check.py

# The random-polygon corpus generator. NOT a test and not in the manifest: it
# is run by hand when the corpus needs regenerating. The corpus is checked in.
polygons/make_polygons: polygons/make_polygons.cpp ../Knoodle.hpp
	@echo "=== Building make_polygons on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) polygons/make_polygons.cpp -o $@
	@echo "✓ make_polygons compiled successfully"

polygons: polygons/make_polygons

# ---------------------------------------------------------------------------
# The tiers.
#
#   make check       every test at light settings. Budget: build time plus 30 s
#                    of RUN time. This is what the pre-push hook and CI run, so
#                    the budget is what keeps it from being dodged.
#   make check-full  every test at full settings, before a release. Hours are
#                    fine -- better to delay a release than to ship a
#                    regression.
#
# Both build everything first, tools included: klut_e2e, cli_contract_check,
# cli_stdin_check and run_tests all drive the real binaries in ../tools.
#
# The light tier REDUCES every test rather than skipping any. Skipping is how
# `all` came to build 2 of 23 targets.
# ---------------------------------------------------------------------------

tools:
	@$(MAKE) -C ../tools all

check: all tools lint
	@python3 run_tier.py --tier=light

check-full: all tools lint
	@python3 run_tier.py --tier=heavy

# Install the versioned hooks by pointing core.hooksPath at scripts/githooks.
#
# That DISABLES the hooks git-lfs installed in .git/hooks, which is why
# scripts/githooks carries verbatim copies of all four of them. Losing the LFS
# hooks is silent -- objects just stop uploading -- so this is checked by
# test/hooks_check.
hooks:
	@git -C .. config core.hooksPath scripts/githooks
	@echo "core.hooksPath -> scripts/githooks"
	@echo "  pre-push:  Git LFS, then the push policy (see scripts/githooks/knoodle-push-policy)"
	@echo "  to undo:   git config --unset core.hooksPath"

# The two lints that make orphans impossible: every *_check.cpp must have a
# manifest row, and every manifest row must have a source file.
lint:
	@python3 manifest_lint.py

clean:
	rm -rf build $(TARGETS) $(PLANTRI)
	rm -f *.d

.PHONY: all libhomfly clean cli_stdin_check lint polygons tools check check-full hooks

# Generated by -MMD; absent on a fresh checkout, hence the leading '-'.
-include $(wildcard *.d)
