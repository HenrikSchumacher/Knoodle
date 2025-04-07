 clang++                                        \
    -Wall                                       \
    -Wextra                                     \
    -std=c++20                                  \
    -O3                                         \
    -ffast-math                                 \
    -march=native                               \
    -mtune=native                               \
    -fenable-matrix                             \
    -pthread                                    \
    -I/opt/homebrew/include                     \
    -L/opt/homebrew/lib                         \
    -o PolyFold                                 \
    -DPOLYFOLD_NO_QUATERNIONS                   \
    -DGIT_VERSION=\"\\\"$(git describe --abbrev=100 --dirty --always --tags)\\\"\" \
    main.cpp                                    \
    -lboost_program_options                     \
    -flto                                       \
#    -fverbose-asm                               \
 
#objdump --reloc --line-numbers --demangle --non-verbose --syms PolyFold > PolyFold.asm
    

    
#
#                                                \
#    -Weverything                                \
#    -Wno-c++98-compat                           \
#    -Wno-c++98-compat-local-type-template-args  \
#    -Wno-c++98-compat-pedantic                  \
#    -Wno-covered-switch-default                 \
#    -Wno-global-constructors                    \
#    -Wno-exit-time-destructors                  \
#    -Wno-padded                                 \
#                                                \
#    -Wno-suggest-override                       \
#    -Wno-zero-as-null-pointer-constant          \
#                                                \
