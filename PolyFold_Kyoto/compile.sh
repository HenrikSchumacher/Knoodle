clang++                                         \
    -Wall                                       \
    -Wextra                                     \
    -std=c++20                                  \
    -O3                                         \
    -ffast-math                                 \
    -flto                                       \
    -fenable-matrix                             \
    -pthread                                    \
    -march=native                               \
    -mtune=native                               \
    -I/opt/homebrew/include                     \
    -L/opt/homebrew/lib                         \
    -o PolyFold                                 \
    -DPOLYFOLD_NO_QUATERNIONS                   \
    -DGIT_VERSION=\"\\\"$(git describe --abbrev=100 --dirty --always --tags)\\\"\" \
    main.cpp                                    \
    -lboost_program_options                     \
