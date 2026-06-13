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

ifeq ($(UNAME_S),Darwin)
    ARCHFLAGS = -march=native -mtune=native -fenable-matrix
else
    ARCHFLAGS = -march=native -mtune=native
endif

CXXFLAGS = -Wall -Wextra -std=c++20 -O3 $(ARCHFLAGS) -pthread

# Knoodle core include paths (mirror tools/Makefile).
KNOODLE_INC = -I$(HOMEBREW_PREFIX)/include \
              -I./../submodules/Min-Cost-Flow-Class/OPTUtils \
              -I./../submodules/Min-Cost-Flow-Class/MCFClass \
              -I./../submodules/Min-Cost-Flow-Class/MCFSimplex \
              -I./../submodules/Tensors

# Vendored libhomfly (public domain; gc shimmed to libc — see vendor/libhomfly).
LIBHOMFLY_DIR = vendor/libhomfly
LIBHOMFLY_SRC = $(wildcard $(LIBHOMFLY_DIR)/*.c)
LIBHOMFLY_OBJ = $(patsubst $(LIBHOMFLY_DIR)/%.c,build/libhomfly_%.o,$(LIBHOMFLY_SRC))

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

# klut_check — exhaustive KLUT internal-consistency test. Same heavy config as
# inflate_check (Alexander via UMFPACK + BLAS/LAPACK; links vendored libhomfly).
klut_check: klut_check.cpp $(LIBHOMFLY_OBJ) ../Knoodle.hpp
	@echo "=== Building klut_check on $(UNAME_S) ==="
	$(CXX) $(CXXFLAGS) -DKNOODLE_USE_UMFPACK $(KNOODLE_INC) $(UMFPACK_INC) \
	    klut_check.cpp $(LIBHOMFLY_OBJ) $(UMFPACK_LDFLAGS) -o $@
	@echo "✓ klut_check compiled successfully"

clean:
	rm -rf build homfly_check key_roundtrip_probe inflate_check

.PHONY: all libhomfly clean
