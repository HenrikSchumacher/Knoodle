clang++                     \
    -Wall                   \
    -Wextra                 \
    -std=c++20              \
    -Ofast                  \
    -flto                   \
    -fenable-matrix         \
    -pthreads               \
    -march=native           \
    -mtune=native           \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib     \
    -o PolyFold             \
    -DGIT_VERSION=\"\\\"$(git describe --abbrev=100 --dirty --always --tags)\\\"\" \
    main.cpp                \
    -lboost_program_options \
    -lstdc++                \
