g++                                              \
    -m64                                            \
    -Wall                                           \
    -Wextra                                         \
    -Wno-unknown-pragmas                            \
    -Wno-ignored-qualifiers                         \
    -std=c++20                                      \
    -Ofast                                          \
    -flto                                           \
    -pthread                                        \
    -march=native                                   \
    -mtune=native                                   \
    -flax-vector-conversions                        \
    -Wno-psabi                                      \
    -opolyfold_gcc                                  \
    main.cpp                                        \
    -lboost_program_options                         \


#    -lboost_system                              \
#        -lboost_thread                              \
#    -I/opt/homebrew/include                     \
#    -L/opt/homebrew/lib                         \

#    -ncpu=apple-m1                              \
