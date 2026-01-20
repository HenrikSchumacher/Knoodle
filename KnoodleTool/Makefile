# Makefile for knoodletool
CXX ?= clang++
CXXFLAGS = -Wall -Wextra -std=c++20 -O3 -march=native -mtune=native \
           -fenable-matrix -pthread
CPPFLAGS = -I$(HOMEBREW_PREFIX)/include \
           -I$(HOMEBREW_PREFIX)/include/suitesparse \
           -I./../submodules/Min-Cost-Flow-Class/OPTUtils \
           -I./../submodules/Min-Cost-Flow-Class/MCFClass \
           -I./../submodules/Min-Cost-Flow-Class/MCFSimplex \
           -I./../submodules/Tensors
LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -lumfpack

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

knoodletool: main.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) main.cpp -o $@

debug: CXXFLAGS = -Wall -Wextra -std=c++20 -g -O0 -DDEBUG \
                  -fenable-matrix -pthread
debug: knoodletool

install: knoodletool
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 knoodletool $(DESTDIR)$(BINDIR)/

clean:
	rm -f knoodletool

.PHONY: install clean debug
