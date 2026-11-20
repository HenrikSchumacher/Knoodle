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
#   all        - everything the manifest lists (the default): the group
#                binaries, the tests built alone, the tools and the benches
#   light      - what the light tier RUNS: `all` without the tools and benches
#   standalone - every target as its own binary, grouped or not
#   lint       - check that the manifest and the tree agree
#   <name>     - any single target from the manifest
#   libhomfly  - just the vendored libhomfly objects
#   clean

UNAME_S := $(shell uname -s)

# ---------------------------------------------------------------------------
# Parallel builds by default. Knoodle/Tensors/Tools are header-only, so a
# Tools or Tensors bump cannot be absorbed by relinking: every driver below
# recompiles the whole stack from scratch. Those compiles are independent,
# which made the serial build embarrassingly slow (minutes on every push, and
# this is the light tier). NPROC=n is the override knob (NPROC=1 for a serial
# build): under Apple's make 3.81 a command-line -jN never shows up in
# MAKEFLAGS during parsing, so the guard below cannot see it and this
# assignment would silently win. Submakes see the -j as "forced" and print a
# one-line jobserver warning; harmless.
NPROC ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
ifeq ($(filter -j%,$(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROC)
endif

CXX ?= clang++
CC  ?= cc

# ccache, when it is installed, for the GROUP compiles (which are nearly all of
# the test build). Nothing here needs it; with it, a compile whose preprocessed
# source and flags were seen before is a file copy. That pays whenever the same
# code is built twice: switching between branches, a second worktree, and in CI
# every push that did not touch what a group includes. `make CCACHE=` turns it
# off. ccache only caches a compile, never a compile-and-link, which is why the
# group rule below makes an object file first.
CCACHE ?= $(shell command -v ccache 2>/dev/null)
# time_macros: the vendored pcg-cpp (deps/pcg-cpp/include/pcg_extras.hpp) names
# __DATE__ and __TIME__ in a seed helper. ccache sees the words, assumes every
# compile differs, and never hits (verified 2026-11-12: two identical builds,
# 0 hits of 8). This tells it not to mind.
CCACHE_ENV := $(if $(CCACHE),CCACHE_SLOPPINESS=time_macros)

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
CXXFLAGS = -Wall -Wextra -Wno-ignored-qualifiers -std=c++20 -O3 $(ARCHFLAGS) -pthread -MMD -MP

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
# light/full/release argument columns DO contain spaces, and are deliberately not read
# here: only run_tests.py needs them.
# ---------------------------------------------------------------------------

COMMA         := ,
MANIFEST      := manifest.tsv
# kind=script rows (Python) have nothing to compile, so they are filtered out
# here. They are still manifest rows -- the runner and the lint see them -- which
# is the point: cli_stdin_check.py used to sit outside the manifest entirely,
# which is exactly the orphan condition the manifest exists to prevent.
MANIFEST_ROWS := $(shell awk -F'\t' '$$0 !~ /^\#/ && NF>=9 && $$2 != "script" { print $$1":"$$3":"(($$4=="-")?$$1".cpp":$$4)":"$$2":"$$9 }' $(MANIFEST))
TARGETS       := $(foreach r,$(MANIFEST_ROWS),$(word 1,$(subst :, ,$(r))))

# GROUPS. A test whose `group` column names a group is not built alone by the
# tiers: it is compiled into build/group_<name> with the rest of its group, as
# one translation unit (see gen_group.py for why -- it is a 3-4x saving in
# compile time). GROUP_ROWS is `group:needs`, one per group.
GROUP_ROWS    := $(shell awk -F'\t' '$$0 !~ /^\#/ && NF>=9 && $$2 == "test" && $$9 != "-" { print $$9":"$$3 }' $(MANIFEST) | sort -u)

# EXCLUDE_NEEDS lets a platform build only what it can link. CI images without
# SuiteSparse pass EXCLUDE_NEEDS=umfpack,homfly_umfpack,boost_umfpack. Build
# RULES are still generated for every target -- only what `all` builds is
# filtered -- so naming an excluded target explicitly still works and fails
# with a real linker error rather than "no rule to make target".
EXCLUDE_NEEDS ?=
EXCLUDED      := $(subst $(COMMA), ,$(EXCLUDE_NEEDS))
BUILD_TARGETS := $(foreach r,$(MANIFEST_ROWS),\
    $(if $(filter $(word 2,$(subst :, ,$(r))),$(EXCLUDED)),,\
        $(word 1,$(subst :, ,$(r)))))

# What each tier builds. A row is `target:needs:source:kind:group`.
#   ALONE_TESTS   kind=test with no group: its own binary, as before.
#   GROUP_BINS    one binary per group.
#   EXTRA_TARGETS kind=tool and kind=bench. Neither tier RUNS these, so the
#                 light tier does not build them either (55 CPU-seconds on
#                 every push, for nothing it then used). `all` does, and
#                 `all` is what check-full and CI on main build -- that is
#                 where they are kept from rotting.
ALONE_TESTS   := $(foreach r,$(MANIFEST_ROWS),\
    $(if $(filter $(word 2,$(subst :, ,$(r))),$(EXCLUDED)),,\
        $(if $(filter test:-,$(word 4,$(subst :, ,$(r))):$(word 5,$(subst :, ,$(r)))),\
            $(word 1,$(subst :, ,$(r))))))
GROUP_BINS    := $(foreach g,$(GROUP_ROWS),\
    $(if $(filter $(word 2,$(subst :, ,$(g))),$(EXCLUDED)),,\
        build/group_$(word 1,$(subst :, ,$(g)))))
EXTRA_TARGETS := $(foreach r,$(MANIFEST_ROWS),\
    $(if $(filter $(word 2,$(subst :, ,$(r))),$(EXCLUDED)),,\
        $(if $(filter test,$(word 4,$(subst :, ,$(r)))),,\
            $(word 1,$(subst :, ,$(r))))))

define BUILD_RULE
$(1): $(3) $$(DEPS_$(2)) $$(EXTRA_DEPS_$(1))
	@echo "=== Building $(1) on $$(UNAME_S) ==="
	$$(CXX) $$(CXXFLAGS) $$(FLAGS_$(2)) $(3) $$(LIBS_$(2)) -o $$@
	@echo "✓ $(1) compiled successfully"
endef

$(foreach r,$(MANIFEST_ROWS),$(eval $(call BUILD_RULE,$(word 1,$(subst :, ,$(r))),$(word 2,$(subst :, ,$(r))),$(word 3,$(subst :, ,$(r))))))

# Three rules per group: the generated source, its object file, the binary. gen_group.py
# rewrites the source only when its contents change, so editing an unrelated
# manifest row re-runs the generator (instant) but does not recompile. The
# members' own sources and headers reach make through the group's .d file.
# -I. because the generated file lives in build/ while the headers its members
# name ("../Knoodle.hpp", "diagram_sanity.hpp") are relative to this directory.
# $(3) is the members' EXTRA_DEPS (plantri_check's vendored plantri binary).
define GROUP_RULE
build/group_$(1).cpp: $$(MANIFEST) gen_group.py | build
	@python3 gen_group.py $(1) $$@

build/group_$(1).o: build/group_$(1).cpp
	@echo "=== Building group $(1) on $$(UNAME_S) ==="
	$$(CCACHE_ENV) $$(CCACHE) $$(CXX) $$(CXXFLAGS) -I. $$(FLAGS_$(2)) -c build/group_$(1).cpp -o $$@

build/group_$(1): build/group_$(1).o $$(DEPS_$(2)) $(3)
	$$(CXX) $$(CXXFLAGS) build/group_$(1).o $$(LIBS_$(2)) -o $$@
	@echo "✓ group $(1) compiled successfully"
endef

group_members = $(foreach r,$(MANIFEST_ROWS),$(if $(filter test:$(1),$(word 4,$(subst :, ,$(r))):$(word 5,$(subst :, ,$(r)))),$(word 1,$(subst :, ,$(r)))))

$(foreach g,$(GROUP_ROWS),$(eval $(call GROUP_RULE,$(word 1,$(subst :, ,$(g))),$(word 2,$(subst :, ,$(g))),$(foreach m,$(call group_members,$(word 1,$(subst :, ,$(g)))),$(EXTRA_DEPS_$(m))))))

all: light $(EXTRA_TARGETS)

light: $(GROUP_BINS) $(ALONE_TESTS)

# Every target as its own binary -- what `all` built before there were groups.
# No tier uses it. It answers "does this test still compile by itself?", which
# a group cannot: there, a test can lean on a header only its neighbour names.
standalone: $(BUILD_TARGETS)


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
#   make check-full  every test at full settings, under two minutes of runs. For when a change deserves more than the light tier:
#                    before merging to main, after touching the core.
#   make check-release  the full tier plus what is too slow to run that often
#                    (inflate_check to 100k crossings: seven minutes). Required at
#                    the commit of every v* tag; the pre-push hook checks.
#                    Better to delay a release than to ship a regression.
#
# All three build everything first, tools included: klut_e2e, cli_contract_check,
# cli_stdin_check and run_tests all drive the real binaries in ../tools.
#
# `tools` builds PolyFold too (devel/PolyFold), for polyfold_check. Until
# v1.1.0 nothing here built it and only the Homebrew formula did -- from a
# release tag -- so PolyFold broke twice without a test noticing. A PolyFold
# compile error now fails the tier before any test runs. It needs
# boost_program_options and UMFPACK; CI does not call this target.
#
# The light tier REDUCES every test rather than skipping any. Skipping is how
# `all` came to build 2 of 23 targets.
# ---------------------------------------------------------------------------

tools:
	@$(MAKE) -C ../tools all
	@$(MAKE) -C ../devel/PolyFold polyfold

check: light tools lint
	@python3 run_tier.py --tier=light

check-full: all tools lint
	@python3 run_tier.py --tier=full

check-release: all tools lint
	@python3 run_tier.py --tier=release

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
	rm -rf build logs $(TARGETS) $(PLANTRI)
	rm -f *.d

.PHONY: all light standalone libhomfly clean cli_stdin_check lint polygons tools check check-full check-release hooks

# Generated by -MMD; absent on a fresh checkout, hence the leading '-'.
-include $(wildcard *.d) $(wildcard build/*.d)
