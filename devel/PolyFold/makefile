# Makefile for PolyFold
CXX ?= g++

# Detect OS and set appropriate flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - use clang with matrix extensions
    CXX = clang++
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -fenable-matrix -pthread -DPOLYFOLD_NO_QUATERNIONS
else ifeq ($(UNAME_S),Linux)
    # Linux - g++ by default for ecosystem compatibility; clang if the user
    # overrides CXX (e.g. an HPC cluster where only clang dodges the gcc-13
    # function_traits<bool*> bug, issue #15). ?= lets an env/command-line CXX
    # through while keeping the gcc default.
    CXX ?= g++
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -pthread -DPOLYFOLD_NO_QUATERNIONS
    # Knoodle's clang matrix path (src/AffineTransforms/ClangMatrix.hpp) needs
    # the matrix-types extension; clang errors out without this flag.
    ifneq ($(findstring clang,$(CXX)),)
        CXXFLAGS += -fenable-matrix
    endif
else
    # Fallback for other Unix systems
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -pthread -DPOLYFOLD_NO_QUATERNIONS
endif

CPPFLAGS = -I$(HOMEBREW_PREFIX)/include \
           -I$(HOMEBREW_PREFIX)/include/suitesparse \
           -I../../submodules/Min-Cost-Flow-Class/OPTUtils \
           -I../../submodules/Min-Cost-Flow-Class/MCFClass \
           -I../../submodules/Min-Cost-Flow-Class/MCFSimplex \
           -I../../submodules/Tensors
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -lboost_program_options -lumfpack

# Version handling. Priority: an explicit KNOODLE_VERSION, then a VERSION file at
# the source-tree root (../../VERSION, two levels up since PolyFold moved to
# devel/) -- the vendored release tarball ships one so the binaries self-describe
# even with no .git -- then git describe in a real checkout.
ifndef KNOODLE_VERSION
    ifneq ($(wildcard ../../VERSION),)
        KNOODLE_VERSION := $(shell cat ../../VERSION)
    endif
endif
ifdef KNOODLE_VERSION
    CXXFLAGS += -DGIT_VERSION=\"$(KNOODLE_VERSION)\"
else
    GIT_VERSION := $(shell git describe --abbrev=100 --dirty --always --tags 2>/dev/null || echo "unknown")
    CXXFLAGS += -DGIT_VERSION=\"\\\"$(GIT_VERSION)\\\"\"
endif

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

# Header dependency tracking, as in tools/Makefile. The rule used to depend on
# main.cpp alone, so an edit anywhere in src/ left a stale polyfold that still
# passed -- and test/polyfold_check, which builds this through `make -C test
# tools`, would have kept testing the old binary. -MMD writes polyfold.d listing
# every header actually included; -MP keeps a deleted header from wedging make.
DEPFLAGS = -MMD -MP

polyfold: main.cpp ../../Knoodle.hpp
	@echo "=== Building PolyFold on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo "=== Compiler flags: $(CXXFLAGS) ==="
	@echo "=== Include paths: $(CPPFLAGS) ==="
	@echo "=== Link flags: $(LDFLAGS) ==="
	@echo ""
	@echo "Compiling main.cpp..."
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(CPPFLAGS) main.cpp $(LDFLAGS) -o $@
	@echo "✓ PolyFold compiled successfully"

install: polyfold
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 polyfold $(DESTDIR)$(BINDIR)/

clean:
	rm -f polyfold polyfold.d

.PHONY: install clean

-include polyfold.d
