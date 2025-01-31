clang++                     \
    -Wall                   \
    -Wextra                 \
    -std=c++20              \
    -Ofast                  \
    -flto                   \
    -fenable-matrix         \
    -pthread                \
    -march=native           \
    -mtune=native           \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib     \
    -o polyfold             \
    main.cpp                \
    -lboost_program_options \
