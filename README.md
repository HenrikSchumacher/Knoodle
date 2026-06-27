# Knoodle
by Henrik Schumacher and Jason Cantarella

A collection of tools for computational knot theory. _Knoodle_ includes, `PlanarDiagram` (a class to manipulate planar diagrams, including rerouting of overstrands and understrands (see `PlanarDiagram::Simplify5`),  `REAPR` (a fast algorithm for simplifying knot diagrams), `ClisbyTree` (a fast algorithm for generating self-avoiding polygons in the "pearl-necklace" model of self-avoidance).

# Installation

To make sure that all submodules are cloned, too, please clone by running the following in the command line:

    git clone --depth 1 --recurse-submodules --shallow-submodules git@github.com:HenrikSchumacher/Knoodle.git

The library can be installed via _homebrew_ on a wide variety of systems. The native platform is MacOS/Apple Clang. Installation is continuously tested via GitHub Actions on the `ubuntu-latest` and `macos-26` runners, and WSL2 uses the same Linuxbrew path as native Linux. It should install on a wide variety of linux systems with the following commands: 

```
brew install git-lfs
git-lfs install
brew tap designbynumbers/cantarellalab
brew install knoodle
```

This will also install the command line tools _polyfold_, _knoodlesimplify_, _knoodledraw_, and _knoodleidentify_. On Linux and WSL2 the formula builds with Homebrew's gcc; on macOS it uses Apple Clang. The build compiles from source with CPU-specific optimizations and typically takes about 5–10 minutes.

### Windows (WSL2)

Knoodle installs under WSL2 using Linuxbrew, exactly as on native Linux. Use WSL 2 (not WSL 1), and a recent Ubuntu (24.04 recommended). No GitHub SSH key is required — the formula clones Knoodle's submodules over HTTPS automatically. The compile is template-heavy and memory-hungry, and WSL2 defaults to ~50% of host RAM, so on a modest machine the compiler may be killed mid-build (`internal compiler error: Killed (program cc1plus)`). If that happens, raise the limit by creating `C:\Users\<you>\.wslconfig` with:

```
[wsl2]
memory=8GB
swap=8GB
```

then run `wsl --shutdown` in Windows PowerShell, reopen your distro, and retry `brew install knoodle`.

Please contact us if you are interested and need support. 

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

_PolyFold_ produces a directory containing files named "Tools_Log.txt" (for debugging; it should be empty), "Info.m" (a Mathematica file which holds an Association giving information about the run and the samples), "PDCodes.tsv" (if the `-c` option is given) and "Polygon_<n>.tsv" (unless the `-P-1` option is used). 

# Command-line tools

_Knoodle_ ships three command-line knot tools — _knoodlesimplify_, _knoodledraw_, and _knoodleidentify_ — which are installed by the homebrew formula alongside _PolyFold_. They can also be compiled by running make in the tools subdirectory. The three tools share a common set of input formats and are designed to compose as Unix filters, so a typical pipeline looks like:

```
knoodlesimplify --streaming-mode < input.tsv | knoodledraw
generator | knoodleidentify
```

## Input formats
All three tools auto-detect their input. You can give a tool a single file, several files, an entire directory of files, or pipe data to it on stdin. The recognized formats are tsv files with varying number of columns:

|Columns|Meaning|
|-------|-------|
|4	| unsigned PD code (4 arc labels per crossing) |
|5	| signed PD code (4 arcs + handedness) |
|6	| unsigned PD code + link-component colors |
|7	| signed PD code + link-component colors |
|3	| 3D geometry (x, y, z coordinates) |


and also `.kndlxyz` files, which describe a multi-component 3D link embedding as a collection of polylines. `.kndlxyz` is the multi-component extension of the 3-column 3D geometry format. It is plain text in which each non-blank line is one vertex, given as three whitespace-separated real numbers:
```
  <x>  <y>  <z>
```  
A block of consecutive vertex lines describes a single closed polygonal link component (the last vertex is joined back to the
first). A blank line separates one component from the next, so a file with k blank-line-delimited blocks is a k-component link.
Components are numbered/colored in the order they appear (0, 1, 2, …). The geometry is projected to a diagram the same way as
3-column input — by default down the z-axis — so use `--randomize-projection` if any segments are vertical. A file containing only non-blank lines with coordinates is parsed as a knot (one-component link) as one would expect.

Within a file or stream, knots and connected summands are separated by lines containing `k` and `s`, exactly as they are in _polyfold_ output (see the _PolyFold_ section above). When the input is 3D geometry, the default projection is down the z-axis, so pass `--randomize-projection` if your geometry contains vertical segments.

## knoodlesimplify

_knoodlesimplify_ is a convenient interface to the knot simplifications in _Knoodle_. It reads diagrams (as PD codes or 3D polygons) and writes simplified PD codes. With `--streaming-mode` it reads from stdin and writes to stdout, so it can be dropped into a pipeline; it is also a quick and robust way to convert polygons to PD codes (use `-s=0` to project a polygon to a PD code with no simplification).

```
Usage: knoodlesimplify [options] [input_files...]

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
                                TV, Height, TV_MCF

Input options:
  --input=FILE                Specify input file (can use multiple times)
  --streaming-mode            Read from stdin, write to stdout
  --randomize-projection      Apply random shear to 3D geometry projection

Output options:
  --output=FILE               Write all output to FILE
  -q, --quiet                 Suppress per-knot reports, show counter only

Other:
  -h, --help                  Show this help message
```

## knoodledraw

_knoodledraw_ reads knot/link diagrams and produces ASCII-art (or Unicode box-drawing) pictures of them. It is a Unix filter: with no input files it reads from stdin, so it pairs naturally with _knoodlesimplify_. Labels for crossings, arcs, faces, and link components can be combined (e.g. `-calf`), and the layout, quality, and highlighting are all tunable.

```
Usage: knoodledraw [options] [input_files...]

Reads knot/link diagrams and produces ASCII art drawings.
If no input files are given, reads from stdin (Unix filter mode).

Quality presets:
  --quality=PRESET            fast, default, best, debug

Grid and spacing:
  --x-grid-size=N             Horizontal grid size (default: 4/6/8 for 0/1/2+ labels)
  --y-grid-size=N             Vertical grid size (default: 2/3/4 for 0/1/2+ labels)
  --x-gap-size=N              Horizontal gap size (default: 1)
  --y-gap-size=N              Vertical gap size (default: 1)
  --x-rounding-radius=N       Horizontal rounding radius (default: 4)
  --y-rounding-radius=N       Vertical rounding radius (default: 4)

Label options (combinable, e.g. -calf):
  -c, --label-crossings       Show crossing numbers (0-based)
  -a, --label-arcs            Show arc numbers (0-based)
  -f, --label-faces           Show face numbers (0-based)
  -l, --label-components      Color/label link components

Output options:
  --ascii                     Use plain ASCII output (default: Unicode box-drawing)
  --highlight=ELEMENTS        Highlight elements (a=arc, c=crossing, f=face)
                              e.g. --highlight="a0,c3,f2"; multiple flags allowed
  --checkerboard-coloring     Highlight alternating faces (checkerboard pattern)

Algorithm options:
  --bend-method=METHOD        mcf (default), clp
  --compaction=METHOD         topo-number, topo-order, length-mcf (default),
                              length-clp, area-clp
  --randomize-bends=N         [experimental] Randomization rounds (default: 0)

Boolean tuning (--flag / --no-flag):
  redistribute-bends (on)     Redistribute bends after optimization
  turn-regularize (on)        Regularize turns
  saturate-regions (on)       Saturate regions
  saturate-exterior (on)      Saturate exterior region
  filter-saturating-edges (on) Filter saturating edges
  randomize-virtual-edges (off) Randomize virtual edges
  dual-simplex (off)          Use dual simplex method

Input options:
  --randomize-projection      Apply random shear to 3D geometry projection

Other:
  -h, --help                  Show this help message
```

Examples:
```
  knoodlesimplify --streaming-mode < input.tsv | knoodledraw
  knoodledraw diagram.tsv
  knoodledraw --quality=fast --ascii diagram.tsv
  knoodledraw --quality=debug --ascii diagram.tsv
```

## knoodleidentify

_knoodleidentify_ identifies the knot type of each input diagram by looking it up in the Knoodle Knot Lookup Table (KLUT). It reads raw diagrams (the same formats as _knoodlesimplify_) and simplifies them internally — so feed it the same stream you would feed _knoodlesimplify_, not the output of _knoodlesimplify_. For each input it decomposes the knot into prime summands, simplifies (escalating with _ReAPR_ when needed), and looks each summand up, writing one line per knot.

By default each line is a Wolfram Language association mapping each distinct prime summand to its multiplicity, which parses directly via ToExpression, e.g.

<| KnotSymbol[3,1,True,"e/r"] -> 2, KnotSymbol[5,1,True,"m/mr"] -> 1 |>
An unknot yields <||>. A summand that cannot be placed in the table carries its signed PD code for offline analysis (e.g. Unidentified[15, {{1,2,3,4,-1}, ...}]).

```
Usage: knoodleidentify [options] [input_files...]

Examples:
  generator | knoodleidentify        # identify a piped stream of diagrams
  knoodleidentify diagrams.tsv       # identify diagrams from a file
  knoodleidentify                    # read diagrams from the terminal (Ctrl-D)

Options:
  --data-dir=PATH     KLUT data directory containing Klut_Keys_NN.bin
                      and Klut_Values_NN.tsv. Default: $KNOODLE_KLUT_DIR,
                      else data/Klut next to this executable's parent,
                      else ./data/Klut.
  --max-crossings=N   Use subtables up to N crossings (3-13, default 13).
  --expanded          One line per knot, summands joined by ' # ' (uses
                      the raw K[...] table names).
  --tsv               Per-summand output: knot_index, summand_index,
                      crossing_count, name (tab-separated).
  --quiet             Suppress the stderr summary and per-summand warnings.
  -h, --help          Show this help.
```
  
The knot-symbol forms (in the default and --tsv output) are: KnotSymbol[c,i,a,"sym"] for an identified knot (c crossings, index i, alternating flag a, symmetry coset "sym"); Unidentified[N,PD] for a diagram with more than 13 crossings (over the table range); NotFound[N,PD] for a diagram of 13 or fewer crossings left unresolved after ReAPR (this should not happen); Link[N] for multi-component input (the table is knots-only); and Invalid[] for an invalid diagram or internal error. Unknot summands are the connect-sum identity and are omitted.
