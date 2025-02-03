# KnotTools
by Henrik Schumacher and Jason Cantarella

A collection of tools for computational knot theory, including _Mathematica_ interfaces. KnotTools includes REAPR (a fast algorithm for simplifying knot diagrams), PolygonFolder (a fast algorithm for generating self-avoiding polygons in the "pearl-necklace" model of self-avoidance), and _Mathematica_ interfaces to _Regina_ and _SnapPy_ for the computation of invariants such as the HOMFLY, Jones and Alexander polynomials.

# Installation

To make sure that all submodules are cloned, too, please clone by running the following in the command line:

    git clone --depth 1 --recurse-submodules --shallow-submodules git@github.com:HenrikSchumacher/KnotTheory.git

Currently the package is configured and tested only for macos (Apple Silicon) and with Apple Clang as compiler. The library should compile also with other platforms, provided the correct compiler flags are given. These can be edited in the file `LibraryResources/Source/BuildSettings.m`. It's some time since we ran this build system under Linux or Windows. So please contact us if you are interested and need support. Also, please contact us if you make this work on other systems; we would gladly add the configurations into this package.

The command-line tool _polyfold_ can be compiled by running "compile.sh" in the subdirectory Test_ClisbyTree. After compiling:

    ./polyfold --help

      Allowed options:
          -h [ --help ]             produce help message
          -t [ --threads ] arg      set number of threads
          -j [ --jobs ] arg         set number of jobs (time series)
          -n [ --edge-count ] arg   set number of edges
          -b [ --burn-in ] arg      set number of burn-in steps
          -s [ --skip ] arg         set number of steps skipped between samples
          -N [ --sample-count ] arg set number of samples
          -o [ --output ] arg       set output directory
