clang++                                     \
    -Wall                                   \
    -Wextra                                 \
    -std=c++20                              \
    -O3                                     \
    -ffast-math                             \
    -flto                                   \
    -fenable-matrix                         \
    -pthread                                \
    -march=native                           \
    -mtune=native                           \
    -I/opt/homebrew/include                 \
    -L/opt/homebrew/lib                     \
    -o polyfold                             \
    main.cpp                                \
    -lboost_program_options                 \
#    -DTOOLS_DEACTIVATE_VECTOR_EXTENSIONS   \
#    -DTOOLS_DEACTIVATE_MATRIX_EXTENSIONS   \
