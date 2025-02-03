g++-14                                              \
    -m64                                            \
    -Wall                                           \
    -Wextra                                         \
    -Wno-unknown-pragmas                            \
    -Wno-ignored-qualifiers                         \
    -std=c++20                                      \
    -Ofast                                          \
    -flto                                           \
    -pthread                                        \
    -mcpu=apple-m1                                  \
    -mtune=native                                   \
    -flax-vector-conversions                        \
    -Wno-psabi                                      \
    -opolyfold_gcc                                  \
    -I/opt/homebrew/Cellar/boost/1.87.0/include     \
    -I/opt/homebrew/include                         \
    -L/opt/homebrew/Cellar/boost/1.87.0/lib         \
    -L/opt/homebrew/lib                             \
    main.cpp                                        \
    -lboost_program_options                         \


#    -lboost_system                              \
#        -lboost_thread                              \
#    -I/opt/homebrew/include                     \
#    -L/opt/homebrew/lib                         \

#    -ncpu=apple-m1                              \
