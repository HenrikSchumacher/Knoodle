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
    # Linux - use gcc for ecosystem compatibility
    CXX = g++
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -pthread
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

# Version handling
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
        DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                         -pthread
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

clean:
	rm -f knoodlesimplify knoodledraw knoodleidentify

.PHONY: all install clean debug
