#!/bin/bash
# Build script for knoodlesimplify and knoodledraw
#
# Dependencies:
#   - Boost (for random number generation)
#   - UMFPACK/SuiteSparse (for sparse linear algebra)
#
# Usage:
#   ./compile.sh           # Build with optimizations
#   ./compile.sh debug     # Build with debug symbols

if [ "$1" = "debug" ]; then
    OPT_FLAGS="-g -O0 -DDEBUG"
    echo "Building in debug mode..."
else
    OPT_FLAGS="-O3 -march=native -mtune=native -flto"
    echo "Building in release mode..."
fi

COMMON_FLAGS="                                              \
    -Wall                                                   \
    -Wextra                                                 \
    -std=c++20                                              \
    $OPT_FLAGS                                              \
    -fenable-matrix                                         \
    -pthread                                                \
    -lumfpack                                               \
    -I./../submodules/Min-Cost-Flow-Class/OPTUtils          \
    -I./../submodules/Min-Cost-Flow-Class/MCFClass          \
    -I./../submodules/Min-Cost-Flow-Class/MCFSimplex        \
    -I./../submodules/Tensors                               \
    -I/opt/homebrew/include                                 \
    -I/opt/homebrew/include/suitesparse                     \
    -L/opt/homebrew/lib                                     \
    -I/usr/local/include                                    \
    -I/usr/local/include/suitesparse                        \
    -L/usr/local/lib                                        \
    -DKNOODLE_USE_UMFPACK                                   \
    -DKNOODLE_USE_BOOST_UNORDERED"

echo "Building knoodlesimplify..."
clang++ $COMMON_FLAGS -o knoodlesimplify knoodlesimplify.cpp
echo "Build complete: ./knoodlesimplify"

echo "Building knoodledraw..."
clang++ $COMMON_FLAGS -o knoodledraw knoodledraw.cpp
echo "Build complete: ./knoodledraw"
