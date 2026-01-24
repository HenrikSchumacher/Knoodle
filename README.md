# Knoodle
by Henrik Schumacher and Jason Cantarella

A collection of tools for computational knot theory. _Knoodle_ includes, `PlanarDiagram` (a class to manipulate planar diagrams, including rerouting of overstrands and understrands (see `PlanarDiagram::Simplify5`),  `REAPR` (a fast algorithm for simplifying knot diagrams), `ClisbyTree` (a fast algorithm for generating self-avoiding polygons in the "pearl-necklace" model of self-avoidance).

# Installation

To make sure that all submodules are cloned, too, please clone by running the following in the command line:

    git clone --depth 1 --recurse-submodules --shallow-submodules git@github.com:HenrikSchumacher/Knoodle.git

The library can be installed via _homebrew_ on a wide variety of systems. The native platform is MacOS/Apple Clang, but installation has been recently tested on Ubuntu 24.04.3 LTS and WSL 2.6.3.0 (running in Windows 10). It should install on a wide variety of linux systems with the following commands: 

```
brew install git-lfs
git-lfs install
brew tap designbynumbers/cantarellalab
brew install knoodle
```

This will also install the command line tools _polyfold_ and _knoodletool_. Please contact us if you are interested and need support. 

# PolyFold

The command-line tool _PolyFold_ can be compiled by running `make` in the subdirectory PolyFold. After compiling:

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

# knoodletool

The command-line tool _knoodletool_ can be compiled by running `make` in the subdirectory KnoodleTool. After compiling:

```
Usage: knoodletool [options] [input_files...]

Simplification options:
  -s=N, --simplify-level=N    Set simplification level:
                                0          No simplification (PD code only)
                                3          Simplify3 (Reidemeister I + II)
                                4          Simplify4 (+ path rerouting)
                                5          Simplify5 (+ summand detection)
                                6+/max/full/reapr
                                           Full Reapr pipeline (default)
  --max-reapr-attempts=K      Maximum iterations for Reapr (default: 25)
  --no-compaction             Skip compaction in OrthoDraw (Reapr only)
  --reapr-energy=E            Set Reapr energy function (Reapr only):
                                TV, Dirichlet, Bending, Height, TV_CLP, TV_MCF

Input options:
  --input=FILE                Specify input file (can use multiple times)
  --streaming-mode            Read from stdin, write to stdout
  --randomize-projection      Apply random shear to 3D geometry projection

Output options:
  --output=FILE               Write all output to FILE
  --output-ascii-drawing      Generate ASCII art diagrams
  -q, --quiet                 Suppress per-knot reports, show counter only

Other:
  -h, --help                  Show this help message
```

`knoodletool` is designed to provide a convenient interface to the knot simplifications in `Knoodle`. You can give it a single TSV file containing the coordinates of a 3d polygon or a PD code, an entire directory of such files, or you can pipe PD codes or polygons to `knoodletool` on `stdin` and receive simplified PD codes on `stdout` with `--streaming-mode`. In streaming mode (or in files), knots and summands are separated by lines containing `k` and `s` as they are in `polyfold` output. You can also use `knoodletool` as a quick and robust way to convert polygons to PD codes. The default projection is down the `z`-axis, so use `--randomize-projection` if your input geometry contains vertical segments. 
