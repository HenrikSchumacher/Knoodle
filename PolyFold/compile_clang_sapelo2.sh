clang++                                         \
    -Wall                                       \
    -Wextra                                     \
    -std=c++20                                  \
    -lc++                                       \
    -O3                                         \
    -ffast-math                                 \
    -fenable-matrix                             \
    -pthread                                    \
    -march=native                               \
    -mtune=native                               \
    -I/usr/include/                             \
    -L/usr/lib64                                \
    -fverbose-asm                               \
    -o ${SLURM_SUBMIT_DIR}/polyfold-${SLURM_JOB_ID}/PolyFold \
    -DPOLYFOLD_NO_QUATERNIONS                       \
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
