# Makefile for knoodlesimplify and knoodledraw
CXX ?= g++

# Warning set shared by the optimized and debug builds below.
#
# -Wno-ignored-qualifiers: every function returning one of Tools' cptr<T>/mptr<T>
# aliases (Tools.hpp:259, `const T * TOOLS_RESTRICT const`) trips this. The
# culprit is the TRAILING const -- verified in isolation: top-level const alone
# warns, TOOLS_RESTRICT alone does not, and pointee const does not.
#
# It is safe to silence because the return type is a pointer. Per [expr.type]/2 a
# prvalue of type cv T, T non-class and non-array, is adjusted to T, so the call
# expression `p.data()` has type `const Scal *` with the top-level const already
# gone -- no caller can observe it. The pointee constness, which is the part that
# matters, survives untouched. Note the const IS still part of the function type
# (decltype(&T::data) keeps it); what is discarded is the expression type, which
# is why this can never change codegen. For a CLASS return type the const would
# NOT be discarded and would not be a no-op -- and neither compiler warns there,
# so the diagnostic only ever fires where the qualifier is genuinely inert.
#
# Both compilers implement the warning; clang exempts qualifiers that arrive
# through a type alias, which is the only reason the macOS build looks clean.
# Under gcc it is ~120 per translation unit, essentially all from the
# Tensors/Tools submodules, burying the warnings that do matter in the CI logs.
# test/Makefile already carries this flag; keep the two in step.
WARNFLAGS = -Wall -Wextra -Wno-ignored-qualifiers

# Detect OS and set appropriate flags
UNAME_S := $(shell uname -s)

# ---------------------------------------------------------------------------
# Parallel builds by default. Knoodle/Tensors/Tools are header-only, so a
# Tools or Tensors bump cannot be absorbed by relinking: every tool below
# recompiles the whole stack from scratch. Those compiles are independent,
# which made the serial build embarrassingly slow (the three tools compile in
# parallel now). NPROC=n is the override knob (NPROC=1 for a serial
# build): under Apple's make 3.81 a command-line -jN never shows up in
# MAKEFLAGS during parsing, so the guard below cannot see it and this
# assignment would silently win. Submakes see the -j as "forced" and print a
# one-line jobserver warning; harmless.
NPROC ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
ifeq ($(filter -j%,$(MAKEFLAGS)),)
    MAKEFLAGS += -j$(NPROC)
endif
ifeq ($(UNAME_S),Darwin)
    # macOS - use clang with matrix extensions
    CXX = clang++
    CXXFLAGS = $(WARNFLAGS) -std=c++20 -O3 -march=native -mtune=native \
               -fenable-matrix -pthread
else ifeq ($(UNAME_S),Linux)
    # Linux - g++ by default for ecosystem compatibility; clang if the user
    # overrides CXX (e.g. an HPC cluster where only clang dodges the gcc-13
    # function_traits<bool*> bug, issue #15). ?= lets an env/command-line CXX
    # through while keeping the gcc default.
    CXX ?= g++
    CXXFLAGS = $(WARNFLAGS) -std=c++20 -O3 -march=native -mtune=native \
               -pthread
    # Knoodle's clang matrix path (src/AffineTransforms/ClangMatrix.hpp) needs
    # the matrix-types extension; clang errors out without this flag.
    ifneq ($(findstring clang,$(CXX)),)
        CXXFLAGS += -fenable-matrix
    endif
else
    # Fallback for other Unix systems
    CXXFLAGS = $(WARNFLAGS) -std=c++20 -O3 -march=native -mtune=native \
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

# Header dependency tracking. `../Knoodle.hpp` above only catches edits to that
# one file, not to the ~200 headers it pulls in, so editing e.g.
# src/OrthoDecorate.hpp used to leave the binaries stale and you would silently
# test the previous build. -MMD writes a .d file next to each binary listing
# every header it actually included; -MP adds phony targets so a deleted or
# renamed header does not wedge the build. The .d files are included below.
DEPFLAGS = -MMD -MP

# Debug build flags (similar OS detection)
ifeq ($(UNAME_S),Darwin)
    DEBUG_CXXFLAGS = $(WARNFLAGS) -std=c++20 -g -O0 -DDEBUG \
                     -fenable-matrix -pthread
else ifeq ($(UNAME_S),Linux)
    ifeq ($(findstring llvm,$(CXX)),llvm)
        # Homebrew clang on Linux - use libc++ for complete ABI consistency
        DEBUG_CXXFLAGS = $(WARNFLAGS) -std=c++20 -g -O0 -DDEBUG \
                         -fenable-matrix -pthread \
                         -stdlib=libc++ \
                         -I$(HOMEBREW_PREFIX)/opt/llvm/include/c++/v1 \
                         -I$(HOMEBREW_PREFIX)/include
    else
        # Plain Linux clang/gcc (e.g. cluster clang module, not Homebrew llvm).
        DEBUG_CXXFLAGS = $(WARNFLAGS) -std=c++20 -g -O0 -DDEBUG \
                         -pthread
        # clang's matrix path (ClangMatrix.hpp) still needs the extension here.
        ifneq ($(findstring clang,$(CXX)),)
            DEBUG_CXXFLAGS += -fenable-matrix
        endif
    endif
else
    DEBUG_CXXFLAGS = $(WARNFLAGS) -std=c++20 -g -O0 -DDEBUG \
                     -pthread
endif

all: knoodlesimplify knoodledraw knoodleidentify

knoodlesimplify: knoodlesimplify.cpp $(SHARED_DEPS)
	@echo "=== Building knoodlesimplify on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodlesimplify.cpp..."
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(BOOST_FLAGS) $(CPPFLAGS) knoodlesimplify.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodlesimplify compiled successfully"

knoodledraw: knoodledraw.cpp $(SHARED_DEPS)
	@echo "=== Building knoodledraw on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodledraw.cpp..."
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(CPPFLAGS) knoodledraw.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodledraw compiled successfully"

knoodleidentify: knoodleidentify.cpp $(SHARED_DEPS)
	@echo "=== Building knoodleidentify on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodleidentify.cpp..."
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(BOOST_FLAGS) $(CPPFLAGS) knoodleidentify.cpp $(LDFLAGS) -o $@
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
	rm -f knoodlesimplify.d knoodledraw.d knoodleidentify.d

# Generated by DEPFLAGS; absent on a fresh checkout, hence the leading '-'.
-include knoodlesimplify.d knoodledraw.d knoodleidentify.d

.PHONY: all install clean debug
