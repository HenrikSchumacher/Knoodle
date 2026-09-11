 clang++                                                \
    -Wall                                               \
    -Wextra                                             \
    -Wconversion                                        \
    -Wimplicit                                          \
    -std=c++20                                          \
    -O3                                                 \
    -march=native                                       \
    -mtune=native                                       \
    -fenable-matrix                                     \
    -pthread                                            \
    -lCoinUtils                                         \
    -lClp                                               \
    -I/opt/homebrew/include                             \
    -I/opt/homebrew/include/suitesparse                 \
    -I./../submodules/Min-Cost-Flow-Class/OPTUtils      \
    -I./../submodules/Min-Cost-Flow-Class/MCFClass      \
    -I./../submodules/Min-Cost-Flow-Class/MCFSimplex    \
    -I./../submodules/Tensors                           \
    -I/usr/local/include/coin-or                        \
    -L/usr/local/lib                                    \
    -L/opt/homebrew/lib                                 \
    -o TestOrthoDraw                                    \
    main.cpp                                            \
    -flto                                               \

#   -Wimplicit-int-conversion-on-negation               \
#   -Wshorten-64-to-32                                  \
#   -Wpointer-integer-compare                           \
