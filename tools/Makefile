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
#
# The version is NOT on the tools' own compile lines. It used to be, as
# -DGIT_VERSION=..., which made every commit a different compile: nothing could
# be cached, and -- since make compares dates, not flags -- a tool built three
# commits ago went on reporting that commit until something else rebuilt it.
# It now goes into one small file per tool, knoodle_version.cpp, which is
# recompiled whenever the version text changes and linked in. See the rules.
ifdef KNOODLE_VERSION
    VERSION_TEXT   := $(KNOODLE_VERSION)
    VERSION_DEFINE  = -DGIT_VERSION=\"$(KNOODLE_VERSION)\"
else
    GIT_VERSION := $(shell git describe --abbrev=100 --dirty --always --tags 2>/dev/null || echo "unknown")
    VERSION_TEXT   := $(GIT_VERSION)
    VERSION_DEFINE  = -DGIT_VERSION=\"\\\"$(GIT_VERSION)\\\"\"
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

# The tools. Add one here and it is built, cleaned and dependency-tracked;
# `install` lists what it installs by name.
#
# knoodlemovie is the animator's movie writer (docs/animate-design.md). It is
# built by `all`, so that every build compiles the animator; not installed yet.
TOOLS = knoodlesimplify knoodledraw knoodleidentify knoodleprove knoodlemovie

all: $(TOOLS)

# ---------------------------------------------------------------------------
# How a tool is built: three steps.
#
#   build/<tool>.o          the tool itself. Its flags are the same for every
#                           commit, so ccache (used when installed; `make
#                           CCACHE=` turns it off) can hand back the object for
#                           source it has compiled before -- in CI, for every
#                           push that did not touch what the tool includes.
#   build/<tool>_version.o  knoodle_version.cpp: the version text and the build
#                           time. Compiled again when the version text changes
#                           (a commit, a tag, the tree going dirty) or the tool
#                           was recompiled. It includes nothing; it is instant.
#   <tool>                  the two, linked.
#
# So after a commit a tool is relinked, not recompiled, and says the right
# version. knoodle_io.hpp reads the linked values when KNOODLE_VERSION_LINKED is
# defined, and the GIT_VERSION macro as before when it is not (the tests, and
# anyone else who includes it, are unchanged).
# ---------------------------------------------------------------------------

# Per-tool extra flags. boost::unordered for the two performance-critical tools.
TOOL_FLAGS_knoodlesimplify = $(BOOST_FLAGS)
TOOL_FLAGS_knoodleidentify = $(BOOST_FLAGS)

CCACHE ?= $(shell command -v ccache 2>/dev/null)
# time_macros: deps/pcg-cpp names __DATE__ and __TIME__, which makes ccache
# treat every compile as new. The tools' own build time is in the version
# object, which is never cached.
CCACHE_ENV := $(if $(CCACHE),CCACHE_SLOPPINESS=time_macros)

build:
	@mkdir -p build

# Rewritten only when the text changes, so it is the version CHANGING that
# rebuilds the version objects, not every run of make.
build/version.txt: FORCE | build
	@echo '$(VERSION_TEXT)' | cmp -s - $@ || echo '$(VERSION_TEXT)' > $@

FORCE:

$(TOOLS:%=build/%.o): build/%.o: %.cpp $(SHARED_DEPS) | build
	@echo "=== Building $* on $(UNAME_S) with $(CXX) ==="
	$(CCACHE_ENV) $(CCACHE) $(CXX) $(CXXFLAGS) $(DEPFLAGS) $(TOOL_FLAGS_$*) $(CPPFLAGS) -DKNOODLE_VERSION_LINKED -c $< -o $@

$(TOOLS:%=build/%_version.o): build/%_version.o: knoodle_version.cpp build/version.txt build/%.o
	@$(CXX) $(CXXFLAGS) $(VERSION_DEFINE) -c knoodle_version.cpp -o $@

$(TOOLS): %: build/%.o build/%_version.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	@echo "✓ $@ compiled successfully"
	@$(NOTE_$@)

define NOTE_knoodleidentify
echo ""; \
echo "NOTE: this is a *local* knoodleidentify build. It needs the ~23 MB KLUT"; \
echo "      tables to identify knots. When run in-tree it finds them at"; \
echo "      ../data/Klut automatically; a copy installed elsewhere (e.g. via"; \
echo "      'make install', on a cluster) does NOT ship the tables, so point it"; \
echo "      at them explicitly with --data-dir=PATH or \$$KNOODLE_KLUT_DIR."; \
echo "      Most users should install via Homebrew, which wires this up for you."
endef

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
	install -m 755 knoodleprove $(DESTDIR)$(BINDIR)/
	@echo ""
	@echo "NOTE: the KLUT tables were NOT installed. knoodleidentify installed to"
	@echo "      $(DESTDIR)$(BINDIR) will look for them at ../share/knoodle/Klut"
	@echo "      relative to the binary; if you did not put them there, run it with"
	@echo "      --data-dir=PATH or \$$KNOODLE_KLUT_DIR pointing at a data/Klut dir."

clean:
	rm -f $(TOOLS) $(TOOLS:%=%.d)
	rm -rf build

# Generated by DEPFLAGS, next to each object; absent on a fresh checkout.
-include $(wildcard build/*.d)

.PHONY: all install clean debug FORCE
