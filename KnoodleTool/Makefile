# Makefile for knoodletool - Knoodle example program
#
# Dependencies:
#   - Boost (for random number generation)
#   - UMFPACK/SuiteSparse (for sparse linear algebra)
#
# Usage:
#   make              # Build with optimizations (release)
#   make release      # Build with optimizations
#   make debug        # Build with debug symbols
#   make clean        # Remove build artifacts
#   make install      # Install to PREFIX (default: /usr/local)
#
# Override compiler:
#   make CXX=g++
#   make CXX=clang++

# Compiler (can be overridden)
CXX ?= c++

# Output binary
TARGET = knoodletool

# Source files
SOURCES = main.cpp

# Installation prefix
PREFIX ?= /usr/local

# Base compiler flags
CXXFLAGS_BASE = -Wall -Wextra -std=c++20 -pthread

# Include paths
INCLUDES = \
    -I./../submodules/Min-Cost-Flow-Class/OPTUtils \
    -I./../submodules/Min-Cost-Flow-Class/MCFClass \
    -I./../submodules/Min-Cost-Flow-Class/MCFSimplex \
    -I./../submodules/Tensors

# Platform-specific paths
# macOS with Homebrew
INCLUDES += -I/opt/homebrew/include -I/opt/homebrew/include/suitesparse
LDFLAGS_PLATFORM = -L/opt/homebrew/lib

# Linux / macOS with /usr/local
INCLUDES += -I/usr/local/include -I/usr/local/include/suitesparse
LDFLAGS_PLATFORM += -L/usr/local/lib

# Linux system paths
INCLUDES += -I/usr/include/suitesparse

# Libraries
LIBS = -lumfpack

# Release flags
CXXFLAGS_RELEASE = -O3 -march=native -mtune=native -flto

# Debug flags
CXXFLAGS_DEBUG = -g -O0 -DDEBUG

# Clang-specific flags (detected automatically)
CLANG_FLAGS =
ifneq (,$(findstring clang,$(shell $(CXX) --version 2>/dev/null)))
    CLANG_FLAGS = -fenable-matrix
endif

# Default target is release
.PHONY: all release debug clean install

all: release

release: CXXFLAGS = $(CXXFLAGS_BASE) $(CXXFLAGS_RELEASE) $(CLANG_FLAGS)
release: $(TARGET)
	@echo "Release build complete: ./$(TARGET)"

debug: CXXFLAGS = $(CXXFLAGS_BASE) $(CXXFLAGS_DEBUG) $(CLANG_FLAGS)
debug: $(TARGET)
	@echo "Debug build complete: ./$(TARGET)"

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS_PLATFORM) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET)

install: release
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
