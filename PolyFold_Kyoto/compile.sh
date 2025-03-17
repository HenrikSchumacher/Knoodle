clang++                                     \
    -Wall                                   \
    -Wextra                                 \
    -std=c++20                              \
    -ffast-math                             \
    -ffast-math                             \
    -flto                                   \
    -pthread                                \
    -fenable-matrix                         \
    -march=native                           \
    -mtune=native                           \
    -I/opt/homebrew/include                 \
    -L/opt/homebrew/lib                     \
    -DGIT_VERSION=\"\\\"$(git describe --abbrev=100 --dirty --always --tags)\\\"\" \
    -o PolyFold                             \
    main.cpp                                \
    -lboost_program_options                 \
