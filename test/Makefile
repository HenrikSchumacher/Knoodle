# Makefile for Knoodle test drivers.
#
# Targets:
#   homfly_check        - HOMFLY correctness oracle (vendored libhomfly, no Python)
#   key_roundtrip_probe - MacLeod-key round-trip stability probe over the KLUT
#   libhomfly           - just the vendored libhomfly objects
#
# Run `make` to build all drivers.

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

clean:
	rm -rf build homfly_check key_roundtrip_probe

.PHONY: all libhomfly clean
