# Makefile for Knoodle test drivers.
#
# Targets:
#   homfly_check        - HOMFLY correctness oracle (vendored libhomfly, no Python)
#   key_roundtrip_probe - MacLeod-key round-trip stability probe over the KLUT
#   inflate_check       - randomized inflate-to-100k stress test (needs UMFPACK +
#                         BLAS/LAPACK; NOT built by `make all`, build explicitly)
#   libhomfly           - just the vendored libhomfly objects
#
# Run `make` to build the quick drivers; `make inflate_check` for the stress test.

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

CXXFLAGS = -Wall -Wextra -std=c++20 -O3 $(ARCHFLAGS) -pthread

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

all: homfly_check key_roundtrip_probe

build:
	@mkdir -p build

build/libhomfly_%.o: $(LIBHOMFLY_DIR)/%.c | build
	$(CC) -O2 -w -I$(LIBHOMFLY_DIR) -c $< -o $@

libhomfly: $(LIBHOMFLY_OBJ)

homfly_check: homfly_check.cpp $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building homfly_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) homfly_check.cpp $(LIBHOMFLY_OBJ) -o $@
	@echo "✓ homfly_check compiled successfully"

key_roundtrip_probe: key_roundtrip_probe.cpp ../Knoodle.hpp
	@echo "=== Building key_roundtrip_probe on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) key_roundtrip_probe.cpp -o $@
	@echo "✓ key_roundtrip_probe compiled successfully"

# inflate_check needs the Alexander invariant (UMFPACK sparse solver) and
# BLAS/LAPACK. On macOS that is Apple Accelerate; elsewhere use OpenBLAS and
# adjust UMFPACK_LDFLAGS / the force-included header accordingly.
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
link_split_check: link_split_check.cpp link_alexander.hpp homfly_invariance.hpp \
                  $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building link_split_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    link_split_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ link_split_check compiled successfully"

# klut_bench — throughput benchmark for the KLUT identify path (construct ->
# Simplify -> canonicalize -> lookup), single- and multi-threaded. See
# docs/klut-simplify-perf.md. Note: with -include of a Tensors header (UMFPACK
# build), the KNOODLE->TOOLS boost cascade is front-run, so pass
# -DTOOLS_USE_BOOST_UNORDERED explicitly to actually enable boost's flat maps.
klut_bench: klut_bench.cpp ../Knoodle.hpp
	@echo "=== Building klut_bench on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_bench.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_bench compiled successfully"

# klut_check — exhaustive KLUT internal-consistency test. Same heavy config as
# inflate_check (Alexander via UMFPACK + BLAS/LAPACK; links vendored libhomfly).
klut_check: klut_check.cpp $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building klut_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_check compiled successfully"

# Vendored plantri binary (the test shells out to it). Single .c file, C stdlib
# only; -w because plantri's own source is not warning-clean and is unmodified.
$(PLANTRI): vendor/plantri/plantri.c
	@echo "=== Building vendored plantri on $(UNAME_S) ==="
	$(CC) -O2 -w -o $@ $<
	@echo "✓ plantri compiled successfully"

# plantri_check — exhaustive plantri-generated diagrams, HOMFLY-invariance under
# Simplify as the oracle (same light config as homfly_check; no UMFPACK). Needs
# the vendored plantri binary at runtime.
plantri_check: plantri_check.cpp homfly_invariance.hpp $(LIBHOMFLY_OBJ) $(PLANTRI) ../Knoodle.hpp
	@echo "=== Building plantri_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) plantri_check.cpp $(LIBHOMFLY_OBJ) -o $@
	@echo "✓ plantri_check compiled successfully"

clean:
	rm -rf build homfly_check key_roundtrip_probe inflate_check \
	       klut_check klut_bench plantri_check link_alex_probe link_inflate_check \
	       link_split_check $(PLANTRI)

.PHONY: all libhomfly clean
