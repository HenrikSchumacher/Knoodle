# Makefile for knoodlesimplify and knoodledraw
CXX ?= g++

# Detect OS and set appropriate flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - use clang with matrix extensions
    CXX = clang++
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -fenable-matrix -pthread
else ifeq ($(UNAME_S),Linux)
    # Linux - g++ by default for ecosystem compatibility; clang if the user
    # overrides CXX (e.g. an HPC cluster where only clang dodges the gcc-13
    # function_traits<bool*> bug, issue #15). ?= lets an env/command-line CXX
    # through while keeping the gcc default.
    CXX ?= g++
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -pthread
    # Knoodle's clang matrix path (src/AffineTransforms/ClangMatrix.hpp) needs
    # the matrix-types extension; clang errors out without this flag.
    ifneq ($(findstring clang,$(CXX)),)
        CXXFLAGS += -fenable-matrix
    endif
else
    # Fallback for other Unix systems
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -pthread
endif

CPPFLAGS = -I$(HOMEBREW_PREFIX)/include \
           -I$(HOMEBREW_PREFIX)/include/suitesparse \
           -I./../submodules/Min-Cost-Flow-Class/OPTUtils \
           -I./../submodules/Min-Cost-Flow-Class/MCFClass \
           -I./../submodules/Min-Cost-Flow-Class/MCFSimplex \
           -I./../submodules/Tensors
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -lumfpack

# boost.unordered flat maps for Reapr/OrthoDraw containers + the Simplify object
# cache: a measurable win on the small-diagram firehose (~+20-25% in klut_bench).
# Header-only (no link step); declared as a dependency in the Homebrew formula.
# Applied to the performance-critical tools (knoodlesimplify, knoodleidentify)
# only -- knoodledraw is a debugging tool, so it stays dependency-free. The tools
# build has no -include of a Tensors header, so this single define cascades to
# TOOLS_USE_BOOST_UNORDERED on its own.
BOOST_FLAGS = -DKNOODLE_USE_BOOST_UNORDERED

# Version handling. Priority: an explicit KNOODLE_VERSION, then a VERSION file at
# the source-tree root (../VERSION) -- the vendored release tarball ships one so the
# binaries self-describe even with no .git -- then git describe in a real checkout.
ifndef KNOODLE_VERSION
    ifneq ($(wildcard ../VERSION),)
        KNOODLE_VERSION := $(shell cat ../VERSION)
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

# Shared header dependency
SHARED_DEPS = knoodle_io.hpp ../Knoodle.hpp

# Debug build flags (similar OS detection)
ifeq ($(UNAME_S),Darwin)
    DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                     -fenable-matrix -pthread
else ifeq ($(UNAME_S),Linux)
    ifeq ($(findstring llvm,$(CXX)),llvm)
        # Homebrew clang on Linux - use libc++ for complete ABI consistency
        DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                         -fenable-matrix -pthread \
                         -stdlib=libc++ \
                         -I$(HOMEBREW_PREFIX)/opt/llvm/include/c++/v1 \
                         -I$(HOMEBREW_PREFIX)/include
    else
        # Plain Linux clang/gcc (e.g. cluster clang module, not Homebrew llvm).
        DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                         -pthread
        # clang's matrix path (ClangMatrix.hpp) still needs the extension here.
        ifneq ($(findstring clang,$(CXX)),)
            DEBUG_CXXFLAGS += -fenable-matrix
        endif
    endif
else
    DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                     -pthread
endif

all: knoodlesimplify knoodledraw knoodleidentify

knoodlesimplify: knoodlesimplify.cpp $(SHARED_DEPS)
	@echo "=== Building knoodlesimplify on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodlesimplify.cpp..."
	$(CXX) $(CXXFLAGS) $(BOOST_FLAGS) $(CPPFLAGS) knoodlesimplify.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodlesimplify compiled successfully"

knoodledraw: knoodledraw.cpp $(SHARED_DEPS)
	@echo "=== Building knoodledraw on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodledraw.cpp..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) knoodledraw.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodledraw compiled successfully"

knoodleidentify: knoodleidentify.cpp $(SHARED_DEPS)
	@echo "=== Building knoodleidentify on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodleidentify.cpp..."
	$(CXX) $(CXXFLAGS) $(BOOST_FLAGS) $(CPPFLAGS) knoodleidentify.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodleidentify compiled successfully"
	@echo ""
	@echo "NOTE: this is a *local* knoodleidentify build. It needs the ~23 MB KLUT"
	@echo "      tables to identify knots. When run in-tree it finds them at"
	@echo "      ../data/Klut automatically; a copy installed elsewhere (e.g. via"
	@echo "      'make install', on a cluster) does NOT ship the tables, so point it"
	@echo "      at them explicitly with --data-dir=PATH or \$$KNOODLE_KLUT_DIR."
	@echo "      Most users should install via Homebrew, which wires this up for you."

# Tier 4 end-to-end pipeline test (test/klut_e2e.cpp). Same flags as the tools;
# it shells out to the built knoodlesimplify / knoodleidentify binaries.
klut_e2e: ../test/klut_e2e.cpp ../Knoodle.hpp
	@echo "=== Building klut_e2e on $(UNAME_S) ==="
	@echo "Compiling ../test/klut_e2e.cpp..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) ../test/klut_e2e.cpp $(LDFLAGS) -o ../test/klut_e2e
	@echo "✓ klut_e2e compiled successfully"

debug: CXXFLAGS = $(DEBUG_CXXFLAGS)
debug: all

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 knoodlesimplify $(DESTDIR)$(BINDIR)/
	install -m 755 knoodledraw $(DESTDIR)$(BINDIR)/
	install -m 755 knoodleidentify $(DESTDIR)$(BINDIR)/
	@echo ""
	@echo "NOTE: the KLUT tables were NOT installed. knoodleidentify installed to"
	@echo "      $(DESTDIR)$(BINDIR) will look for them at ../share/knoodle/Klut"
	@echo "      relative to the binary; if you did not put them there, run it with"
	@echo "      --data-dir=PATH or \$$KNOODLE_KLUT_DIR pointing at a data/Klut dir."

clean:
	rm -f knoodlesimplify knoodledraw knoodleidentify

.PHONY: all install clean debug
