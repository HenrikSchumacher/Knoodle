g++-15                                              \
    -m64                                            \
    -Wall                                           \
    -Wextra                                         \
    -Wno-unknown-pragmas                            \
    -Wno-ignored-qualifiers                         \
    -Wno-psabi                                      \
    -std=c++20                                      \
    -O3                                             \
    -ffast-math                                     \
    -flto=auto                                      \
    -pthread                                        \
    -mcpu=apple-m1                                  \
    -mtune=native                                   \
    -oPolyFold                                      \
    -I/opt/homebrew/Cellar/boost/1.87.0/include     \
    -I/opt/homebrew/include                         \
    -L/opt/homebrew/Cellar/boost/1.87.0/lib         \
    -L/opt/homebrew/lib                             \
    -DPOLYFOLD_NO_QUATERNIONS                       \
    -DGIT_VERSION=\"\\\"$(git describe --abbrev=100 --dirty --always --tags)\\\"\" \
    main.cpp                                        \
    -lboost_program_options                         \


#    -flax-vector-conversions                        \
