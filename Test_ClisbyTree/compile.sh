clang++                                         \
    -Wall                                       \
    -Wextra                                     \
                                                \
    -Weverything                                \
    -Wno-c++98-compat                           \
    -Wno-c++98-compat-local-type-template-args  \
    -Wno-c++98-compat-pedantic                  \
    -Wno-covered-switch-default                 \
    -Wno-global-constructors                    \
    -Wno-exit-time-destructors                  \
    -Wno-padded                                 \
                                                \
    -Wno-suggest-override                       \
    -Wno-zero-as-null-pointer-constant          \
                                                \
    -std=c++20                                  \
    -O3                                         \
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



