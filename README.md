# Knoodle
by Henrik Schumacher and Jason Cantarella

A collection of tools for computational knot theory. _Knoodle_ includes, `PlanarDiagram` (a class to manipulate planar diagrams, including rerouting of overstrands and understrands (see `PlanarDiagram::Simplify5`),  `REAPR` (a fast algorithm for simplifying knot diagrams), `ClisbyTree` (a fast algorithm for generating self-avoiding polygons in the "pearl-necklace" model of self-avoidance).

# Installation

To make sure that all submodules are cloned, too, please clone by running the following in the command line:

    git clone --depth 1 --recurse-submodules --shallow-submodules git@github.com:HenrikSchumacher/Knoodle.git

Currently the package is configured and tested only for macos (Apple Silicon) and with Apple Clang as compiler. The library should compile also with other platforms, provided the correct compiler flags are given. These can be edited in the file `Test_ClisbyTree\compile.sh`. It's some time since we ran this build system under Linux or Windows. So please contact us if you are interested and need support. Also, please contact us if you make this work on other systems; we would gladly add the configurations into this package.

The command-line tool _PolyFold_ can be compiled by running "compile.sh" in the subdirectory Test_ClisbyTree. After compiling:

```
./PolyFold --help

Allowed options:
  -h [ --help ]                   produce help message
  -d [ --diam ] arg (=1)          set hard sphere diameter
  -n [ --edge-count ] arg         set number of edges
  -b [ --burn-in ] arg            set number of burn-in steps
  -s [ --skip ] arg               set number of steps skipped between samples
  -N [ --sample-count ] arg       set number of samples
  -o [ --output ] arg             set output directory
  -i [ --input ] arg              set input file
  -T [ --tag ] arg                set a tag to append to output directory
  -e [ --extend ]                 extend name of output directory by 
                                  information about the experiment, then append
                                  value of --tag option
  -v [ --verbosity ] arg (=1)     how much information should be printed to 
                                  Log.txt file.
  --pcg-multiplier arg            specify 128 bit unsigned integer used by 
                                  pcg64 for the "multiplier" (implementation 
                                  dependent -- better not touch it unless you 
                                  really know what you do)
  --pcg-increment arg             specify 128 bit unsigned integer used by 
                                  pcg64 for the "increment" (every processor 
                                  should have its own)
  --pcg-state arg                 specify 128 bit unsigned integer used by 
                                  pcg64 for the state (use this for seeding)
  -m [ --low-mem ]                force deallocation of large data structures; 
                                  this will be a bit slower but peak memory 
                                  will be less
  -a [ --angles ]                 compute statistics on curvature and torsion 
                                  angles and report them in file "Info.m"
  -g [ --squared-gyradius ]       compute squared radius of gyration and report
                                  in file "Info.m"
  -c [ --pd-code ]                compute pd codes and print to file 
                                  "PDCodes.tsv"
  -B [ --bounding-boxes ]         compute statistics of bounding boxes and 
                                  report them in file "Info.m"
  -P [ --polygons ] arg (=-1)     print every [arg] sample to file; if [arg] is
                                  negative, no samples are written to file ; if
                                  [arg] is 0, then only the polygon directly 
                                  after burn-in and the final sample are 
                                  written to file
  --histograms arg (=0)           create histograms for curvature and torsion 
                                  angles with [arg] bins
  -C [ --checks ] arg (=1)        whether to perform hard sphere collision 
                                  checks
  -R [ --reflections ] arg (=0.5) probability that a pivot move is orientation 
                                  reversing
  -H [ --hierarchical ] arg (=0)  whether to use hierarchical moves for burn-in
                                  and sampling
  -S [ --shift ] arg (=1)         shift vertex indices randomly in each sample
  -Z [ --recenter ] arg (=1)      translate each sample so that its barycenter 
                                  is the origin
```

PolyFold produces a directory containing files named "Tools_Log.txt" (for debugging; it should be empty), "Info.m" (a Mathematica file which holds an Association giving information about the run and the samples), "PDCodes.tsv" (if the -c option is given) and "Polygon_<n>.tsv" (unless the `-P-1` option is used). 

The `Polygon_<n>.tsv` files contain lines in the form ```<x> \t <y> \t <z> \n``` giving the coordinates of each vertex. 

The `PDCodes.tsv` file gives a collection of **extensively simplified** planar diagram codes describing the knot types of the samples. While these PD codes are guaranteed to describe the knot type correctly, the number of crossings in these PD codes is orders of magnitude smaller than the number of crossings in planar projections of the polygons. The numbering of arcs and crossings in these codes is not canonical. A `PDCodes.tsv` file contains lines in one of three forms:

```k  ```

A `k` line is start of a knot. There should be `--sample-count` `k` lines in the file. This is equal to the number of `Polygon_<n>.tsv` files if and only if `-P1` is given; otherwise, there are more samples than polygons. A `k` line followed immediately by another `k` line (or the end of the the file) denotes an unknot.

```s <0/1>```

An `s` line is the start of a connected summand of a knot. The digit following the `s` is `1` if the diagram of the summand is proved to be minimal (e.g. it is reduced and alternating) and `0` if the diagram has not been proved to be minimal. (Of course, `s 0` summands for nonalternating knots are very often minimal in practice, but the current version of PolyFold doesn't recognize this automatically.)

```<a> <b> <c> <d> <+1/-1>```

A line in this form describes a crossing in an oriented diagram of a connected summand of a knot. These lines follow an `s` line and precede another `s` or `k` line or the end of the file. `<a> <b> <c> <d>` are non-negative integers referring to 0-indexed strands of the diagram meeting at a crossing. The `+1/-1` gives the sign of the crossing. The ordering follows the convention in Regina: "Each 4-tuple represents a single crossing, and lists the four arcs that meet at that crossing in counter-clockwise order, beginning with the incoming lower arc." A block of `c` of these lines in a row gives the code for a `c`-crossing summand of the knot. The arc indices `0, 1, ..., 2c-1` should each occur twice in the block, and the arcs are numbered with the orientation of the knot.
