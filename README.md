# Knoodle
by Henrik Schumacher and Jason Cantarella

A collection of tools for computational knot theory. _Knoodle_ includes, `PlanarDiagram` (a class to manipulate planar diagrams, including rerouting of overstrands and understrands (see `PlanarDiagram::Simplify5`),  `REAPR` (a fast algorithm for simplifying knot diagrams), `ClisbyTree` (a fast algorithm for generating self-avoiding polygons in the "pearl-necklace" model of self-avoidance).

# Installation

To make sure that all submodules are cloned, too, please clone by running the following in the command line:

    git clone --depth 1 --recurse-submodules --shallow-submodules git@github.com:HenrikSchumacher/Knoodle.git

Currently the package is configured and tested only for macos (Apple Silicon) and with Apple Clang as compiler. The library should compile also with other platforms, provided the correct compiler flags are given. These can be edited in the file `Test_ClisbyTree\compile.sh`. It's some time since we ran this build system under Linux or Windows. So please contact us if you are interested and need support. Also, please contact us if you make this work on other systems; we would gladly add the configurations into this package.

The command-line tool _PolyFold_ can be compiled by running "compile.sh" in the subdirectory Test_ClisbyTree. After compiling:

    ./PolyFold --help

    Allowed options:
      -h [ --help ]               produce help message
      -d [ --diam ] arg           set hard sphere diameter
      -n [ --edge-count ] arg     set number of edges
      -b [ --burn-in ] arg        set number of burn-in steps
      -s [ --skip ] arg           set number of steps skipped between samples
      -N [ --sample-count ] arg   set number of samples
      -o [ --output ] arg         set output directory
      -T [ --tag ] arg            set a tag to append to output directory
      -e [ --extend ]             extend name of output directory by information 
                                  about the experiment, then append value of --tag 
                                  option
      -v [ --verbosity ] arg      how much information should be printed to Log.txt
                                  file.
      -M [ --pcg-multiplier ] arg specify 128 bit unsigned integer used by pcg64 
                                  for the "multiplier" (implementation dependent --
                                  better not touch it unless you really know what 
                                  you do)
      -I [ --pcg-increment ] arg  specify 128 bit unsigned integer used by pcg64 
                                  for the "increment" (every processor should have 
                                  its own)
      -S [ --pcg-state ] arg      specify 128 bit unsigned integer used by pcg64 
                                  for the state (use this for seeding)
      -m [ --low-mem ]            force deallocation of large data structures; this
                                  will be a bit slower but peak memory will be less
      -a [ --angles ]             compute statistics on curvature and torsion 
                                  angles and report them in file "Info.m"
      -g [ --squared-gyradius ]   compute squared radius of gyration and report in 
                                  file "Info.m"
      -c [ --pd-code ]            compute pd codes and print to file "PDCodes.tsv"
      -P [ --polygons ] arg       print every [arg] sample to file
      -H [ --histograms ] arg     create histograms for curvature and torsion 
                                  angles with [arg] bins
      --no-checks                 perform folding without checks for overlap of 
                                  hard spheres
      -R [ --reflections ]        allow pivot moves to be ortientation reversing
