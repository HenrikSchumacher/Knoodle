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

all: knoodlesimplify knoodledraw

knoodlesimplify: knoodlesimplify.cpp $(SHARED_DEPS)
	@echo "=== Building knoodlesimplify on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodlesimplify.cpp..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) knoodlesimplify.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodlesimplify compiled successfully"

knoodledraw: knoodledraw.cpp $(SHARED_DEPS)
	@echo "=== Building knoodledraw on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo ""
	@echo "Compiling knoodledraw.cpp..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) knoodledraw.cpp $(LDFLAGS) -o $@
	@echo "✓ knoodledraw compiled successfully"

debug: CXXFLAGS = $(DEBUG_CXXFLAGS)
debug: all

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 knoodlesimplify $(DESTDIR)$(BINDIR)/
	install -m 755 knoodledraw $(DESTDIR)$(BINDIR)/

clean:
	rm -f knoodlesimplify knoodledraw

.PHONY: all install clean debug
