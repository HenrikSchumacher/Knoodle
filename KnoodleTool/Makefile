# Makefile for knoodletool
CXX ?= clang++

# Detect OS and handle clang consistently
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - use clang with matrix extensions
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -fenable-matrix -pthread
else ifeq ($(UNAME_S),Linux)
    # Linux - check if we're using Homebrew clang (ABI consistency)
    # If CXX contains "llvm", we can use -fenable-matrix
    ifeq ($(findstring llvm,$(CXX)),llvm)
        CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
                   -fenable-matrix -pthread
    else
        # Default Linux GCC build (no -fenable-matrix)
        CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
                   -pthread
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

# Version handling
ifdef KNOODLE_VERSION
    CXXFLAGS += -DGIT_VERSION=\"$(KNOODLE_VERSION)\"
else
    GIT_VERSION := $(shell git describe --abbrev=100 --dirty --always --tags 2>/dev/null || echo "unknown")
    CXXFLAGS += -DGIT_VERSION=\"\\\"$(GIT_VERSION)\\\"\"
endif

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

# Debug build flags (similar OS detection)
ifeq ($(UNAME_S),Darwin)
    DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                     -fenable-matrix -pthread
else ifeq ($(UNAME_S),Linux)
    ifeq ($(findstring llvm,$(CXX)),llvm)
        DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                         -fenable-matrix -pthread
    else
        DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                         -pthread
    endif
else
    DEBUG_CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                     -pthread
endif

knoodletool: main.cpp
	@echo "=== Building for $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo "=== Flags: $(CXXFLAGS) ==="
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) main.cpp $(LDFLAGS) -o $@

debug: CXXFLAGS = $(DEBUG_CXXFLAGS)
debug: knoodletool

install: knoodletool
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 knoodletool $(DESTDIR)$(BINDIR)/

clean:
	rm -f knoodletool

.PHONY: install clean debug
