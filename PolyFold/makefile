# Makefile for PolyFold
CXX ?= clang++
CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
           -fenable-matrix -pthread -DPOLYFOLD_NO_QUATERNIONS
CPPFLAGS = -I$(HOMEBREW_PREFIX)/include
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -lboost_program_options

# Version handling - use KNOODLE_VERSION from environment, or fallback
ifdef KNOODLE_VERSION
    CXXFLAGS += -DGIT_VERSION=\"$(KNOODLE_VERSION)\"
else
    # Fallback for non-brew builds
    GIT_VERSION := $(shell git describe --abbrev=100 --dirty --always --tags 2>/dev/null || echo "unknown")
    CXXFLAGS += -DGIT_VERSION=\"\\\"$(GIT_VERSION)\\\"\"
endif

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

polyfold: main.cpp
	@echo "=== Actual compilation command ==="
	@echo "$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) main.cpp -o $@ $<"
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) main.cpp -o $@ $

install: polyfold
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 polyfold $(DESTDIR)$(BINDIR)/

clean:
	rm -f polyfold

.PHONY: install clean
