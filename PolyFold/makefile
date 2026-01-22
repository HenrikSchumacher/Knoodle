# Makefile for PolyFold
CXX ?= clang++

# Detect OS and handle clang consistently
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - use clang with matrix extensions
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -fenable-matrix -pthread -DPOLYFOLD_NO_QUATERNIONS
else ifeq ($(UNAME_S),Linux)
    # Linux - check if we're using Homebrew clang (ABI consistency)
    # If CXX contains "llvm", we can use -fenable-matrix
    ifeq ($(findstring llvm,$(CXX)),llvm)
        CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
                   -fenable-matrix -pthread -DPOLYFOLD_NO_QUATERNIONS
    else
        # Default Linux GCC build (no -fenable-matrix)
        CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
                   -pthread -DPOLYFOLD_NO_QUATERNIONS
    endif
else
    # Fallback for other Unix systems
    CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
               -pthread -DPOLYFOLD_NO_QUATERNIONS
endif

CPPFLAGS = -I$(HOMEBREW_PREFIX)/include
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -lboost_program_options

# Version handling
ifdef KNOODLE_VERSION
    CXXFLAGS += -DGIT_VERSION=\"$(KNOODLE_VERSION)\"
else
    GIT_VERSION := $(shell git describe --abbrev=100 --dirty --always --tags 2>/dev/null || echo "unknown")
    CXXFLAGS += -DGIT_VERSION=\"\\\"$(GIT_VERSION)\\\"\"
endif

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

polyfold: main.cpp
	@echo "=== Building PolyFold on $(UNAME_S) ==="
	@echo "=== Compiler: $(CXX) ==="
	@echo "=== Compiler flags: $(CXXFLAGS) ==="
	@echo "=== Include paths: $(CPPFLAGS) ==="
	@echo "=== Link flags: $(LDFLAGS) ==="
	@echo ""
	@echo "Compiling main.cpp..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) main.cpp $(LDFLAGS) -o $@
	@echo "✓ PolyFold compiled successfully"

install: polyfold
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 polyfold $(DESTDIR)$(BINDIR)/

clean:
	rm -f polyfold

.PHONY: install clean
