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

MANIFEST      := manifest.tsv
MANIFEST_ROWS := $(shell awk -F'\t' '$$0 !~ /^\#/ && NF>=4 { print $$1":"$$3":"(($$4=="-")?$$1".cpp":$$4) }' $(MANIFEST))
TARGETS       := $(foreach r,$(MANIFEST_ROWS),$(word 1,$(subst :, ,$(r))))

define BUILD_RULE
$(1): $(3) $$(DEPS_$(2)) $$(EXTRA_DEPS_$(1))
	@echo "=== Building $(1) on $$(UNAME_S) ==="
	$$(CXX) $$(CXXFLAGS) $$(FLAGS_$(2)) $(3) $$(LIBS_$(2)) -o $$@
	@echo "✓ $(1) compiled successfully"
endef

$(foreach r,$(MANIFEST_ROWS),$(eval $(call BUILD_RULE,$(word 1,$(subst :, ,$(r))),$(word 2,$(subst :, ,$(r))),$(word 3,$(subst :, ,$(r))))))

all: $(TARGETS)


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

# The two lints that make orphans impossible: every *_check.cpp must have a
# manifest row, and every manifest row must have a source file.
lint:
	@python3 manifest_lint.py

clean:
	rm -rf build $(TARGETS) $(PLANTRI)
	rm -f *.d

.PHONY: all libhomfly clean cli_stdin_check lint polygons

# Generated by -MMD; absent on a fresh checkout, hence the leading '-'.
-include $(wildcard *.d)
