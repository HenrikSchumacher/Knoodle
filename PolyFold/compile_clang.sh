clang-19                                        \
    -Wall                                       \
    -Wextra                                     \
    -std=c++20                                  \
    -lc++                                       \
    -Ofast                                      \
    -ffast-math                                 \
    -flto                                       \
    -fenable-matrix                             \
    -pthread                                    \
    -march=native                               \
    -mtune=native                               \
    -I/opt/homebrew/include                     \
    -L/opt/homebrew/lib                         \
    -fverbose-asm                               \
    -o PolyFold                                 \
    -DGIT_VERSION=\"\\\"$(git describe --abbrev=100 --dirty --always --tags)\\\"\" \
    main.cpp                                    \
    -lboost_program_options                     \
 
#objdump --reloc --line-numbers --demangle --non-verbose --syms PolyFold > PolyFold.asm
    
#        -g                                          \
    
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
