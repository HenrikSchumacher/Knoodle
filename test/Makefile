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
#
# The src headers are explicit prerequisites on purpose: this test exists to
# catch src-only changes, and without them a `make` after editing a Prosector
# would not rebuild -- the test would "pass" against the binary it built before
# the change. Same trap intersection_flag_reset_check documents below.
EMBEDDING_SRC_DEPS = ../src/LinkEmbedding/     ../src/LinkEmbedding_Int/ \
					 ../src/LinkEmbedding.hpp  ../src/LinkEmbedding2.hpp \
                     ../src/LinkEmbedding3.hpp ../src/LinkEmbedding4.hpp \
					 ../src/Prosector/ \
                     ../src/Prosector2.hpp ../src/Prosector3.hpp \
                     ../src/Prosector4.hpp  ../src/Prosector4a.hpp \
                     ../src/PlanarDiagram/FromEmbeddings.hpp

embedding_check: embedding_check.cpp embedding_fixtures.hpp link_alexander.hpp \
                 homfly_invariance.hpp $(LIBHOMFLY_OBJ) ../Knoodle.hpp \
                 $(EMBEDDING_SRC_DEPS)
	@echo "=== Building embedding_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    embedding_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ embedding_check compiled successfully"

# klut_bench — throughput benchmark for the KLUT Identify path (construct ->
# ki::Identify), single- and multi-threaded. See docs/klut-simplify-perf.md.
# Note: with -include of a Tensors header (UMFPACK build), the KNOODLE->TOOLS
# boost cascade is front-run, so pass -DTOOLS_USE_BOOST_UNORDERED explicitly to
# actually enable boost's flat maps.
klut_bench: klut_bench.cpp ../tools/klut_identify.hpp ../Knoodle.hpp
	@echo "=== Building klut_bench on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_bench.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_bench compiled successfully"

# klut_bench_boost — klut_bench with boost::unordered_flat_map under both the
# integer maps AND the CachedObject string->any cache (the ~+15% dependency-free
# build). boost.unordered is header-only. TOOLS_USE_BOOST_UNORDERED is passed
# EXPLICITLY: the -include of a Tensors header below front-runs Knoodle.hpp, so the
# KNOODLE_USE_BOOST_UNORDERED -> TOOLS_USE_BOOST_UNORDERED cascade would otherwise
# be defeated and boost would silently fall back to std. Verify after building:
#   nm klut_bench_boost | c++filt | grep -c boost::unordered   (must be nonzero)
klut_bench_boost: klut_bench.cpp ../tools/klut_identify.hpp ../Knoodle.hpp
	@echo "=== Building klut_bench_boost on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK -DKNOODLE_USE_BOOST_UNORDERED \
	    -DTOOLS_USE_BOOST_UNORDERED $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_bench.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_bench_boost compiled successfully"

# klut_check — exhaustive KLUT internal-consistency test. Same heavy config as
# inflate_check (Alexander via UMFPACK + BLAS/LAPACK; links vendored libhomfly).
klut_check: klut_check.cpp $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building klut_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_check compiled successfully"

# canon_check — locks the identify hot-path optimization: declining the final
# Canonicalize must NOT change the looked-up MacLeod code / KLUT id (i.e.
# WriteMacLeodCode is canonical-by-construction). Reads data/Klut; same heavy
# config as klut_bench. See docs/klut-identify-design.md.
canon_check: canon_check.cpp ../Knoodle.hpp
	@echo "=== Building canon_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    canon_check.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ canon_check compiled successfully"

# pd_internal_state_check — round trip for PlanarDiagram's internal-state
# serialization (WriteToOutString / FromInString, and the file pair
# WriteToFile / FromFile). Covers inactive slots and max_* vs active counts,
# which PD codes cannot express, and that malformed input yields an invalid
# diagram rather than a silently empty one. Light config.
pd_internal_state_check: pd_internal_state_check.cpp ../Knoodle.hpp
	@echo "=== Building pd_internal_state_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) pd_internal_state_check.cpp -o $@
	@echo "✓ pd_internal_state_check compiled successfully"

# component_check — regression guard for the CollapseArcRange unlink-loss bug
# (Henrik's 5151f39). Embedded 8-crossing 2-component-unlink reproducer; default
# Simplify must preserve the link-component count. Light config (no UMFPACK).
component_check: component_check.cpp ../Knoodle.hpp
	@echo "=== Building component_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) component_check.cpp -o $@
	@echo "✓ component_check compiled successfully"

# link_color_roundtrip — regression guard for LinkEmbedding's colored .kndlxyz
# round trip: WriteToFile(colorQ=true) emits "#color <int>" headers that
# FromInString used to reject, so the writer produced files its own reader could
# not read. Light config (no UMFPACK).
link_color_roundtrip: link_color_roundtrip.cpp ../Knoodle.hpp
	@echo "=== Building link_color_roundtrip on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) link_color_roundtrip.cpp -o $@
	@echo "✓ link_color_roundtrip compiled successfully"

# klut_identify_check — PR test for the Identify routine (test/klut_identify.hpp):
# single knots (perturbed), connect sums (multi-diagram PDC + single spliced
# diagram), the unknot, and multi-component links. Same heavy config as klut_bench.
klut_identify_check: klut_identify_check.cpp ../tools/klut_identify.hpp ../Knoodle.hpp
	@echo "=== Building klut_identify_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_identify_check.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_identify_check compiled successfully"

# klut_identify_random_check — characterization/stability test: Identify a fixed
# random 2048-gon's unsimplified diagram (3348 crossings, embedded). Exercises
# decomposition, Reapr escalation, and the Unidentified path on a realistic input.
# Same heavy config as klut_identify_check.
klut_identify_random_check: klut_identify_random_check.cpp ../tools/klut_identify.hpp ../Knoodle.hpp
	@echo "=== Building klut_identify_random_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_identify_random_check.cpp $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_identify_random_check compiled successfully"

# Vendored plantri binary (the test shells out to it). Single .c file, C stdlib
# only; -w because plantri's own source is not warning-clean and is unmodified.
$(PLANTRI): vendor/plantri/plantri.c
	@echo "=== Building vendored plantri on $(UNAME_S) ==="
	$(CC) -O2 -w -o $@ $<
	@echo "✓ plantri compiled successfully"

# plantri_check — exhaustive plantri-generated diagrams: HOMFLY-invariance under
# Simplify as the oracle, plus (with --klut) the KLUT identify-contract check
# (ki::Identify A+B). Light config (no UMFPACK; ki::Identify's Reapr uses TV+MCF).
# Needs the vendored plantri binary at runtime.
plantri_check: plantri_check.cpp homfly_invariance.hpp ../tools/klut_identify.hpp \
               $(LIBHOMFLY_OBJ) $(PLANTRI) ../Knoodle.hpp
	@echo "=== Building plantri_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) $(KNOODLE_INC) plantri_check.cpp $(LIBHOMFLY_OBJ) -o $@
	@echo "✓ plantri_check compiled successfully"

# cli_stdin_check — regression test for the interactive-stdin notice (isatty).
# Pure-Python (stdlib only, no Regina/venv); needs the tools built in ../tools.
cli_stdin_check:
	@echo "=== Running cli_stdin_check (interactive-stdin notice) ==="
	python3 cli_stdin_check.py

clean:
	rm -rf build homfly_check key_roundtrip_probe inflate_check \
	       klut_check klut_bench klut_bench_boost canon_check component_check \
	       klut_identify_check klut_identify_random_check \
	       plantri_check link_alex_probe link_inflate_check \
	       link_split_check embedding_check $(PLANTRI)

.PHONY: all libhomfly clean cli_stdin_check
