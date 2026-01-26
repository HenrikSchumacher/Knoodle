#!/bin/bash
# Build script for knoodletool - Knoodle example program
#
# Dependencies: None!
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

clang++                                                 \
    -Wall                                               \
    -Wextra                                             \
    -std=c++20                                          \
    $OPT_FLAGS                                          \
    -fenable-matrix                                     \
    -pthread                                            \
    -I./../submodules/Min-Cost-Flow-Class/OPTUtils      \
    -I./../submodules/Min-Cost-Flow-Class/MCFClass      \
    -I./../submodules/Min-Cost-Flow-Class/MCFSimplex    \
    -I./../submodules/Tensors                           \
    -o knoodletool                                      \
    main.cpp                                            \

echo "Build complete: ./knoodletool"
