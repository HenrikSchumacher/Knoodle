clang++                                     \
    -Wall                                   \
    -Wextra                                 \
    -std=c++20                              \
    -O3                                     \
    -ffast-math                             \
    -flto                                   \
    -pthread                                \
    -fenable-matrix                         \
    -march=native                           \
    -mtune=native                           \
    -I/opt/homebrew/include                 \
    -L/opt/homebrew/lib                     \
    -o PolyFold                             \
    main.cpp                                \
    -lboost_program_options                 \



