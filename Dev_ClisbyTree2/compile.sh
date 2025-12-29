 clang++                                                \
    -Wall                                               \
    -Wextra                                             \
    -std=c++20                                          \
    -O3                                                 \
    -march=native                                       \
    -mtune=native                                       \
    -fenable-matrix                                     \
    -pthread                                            \
    -I./../submodules/Tensors                           \
    -I/opt/homebrew/include                             \
    -L/opt/homebrew/lib                                 \
    -I/usr/local/include                                \
    -L/usr/local/lib                                    \
    -o Example_ClisbyTree2                              \
    main.cpp                                            \
    -flto                                               \
