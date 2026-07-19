# PyKnoodle

Python bindings for the Knoodle library, a powerful tool for analyzing mathematical knots.

## Description

PyKnoodle provides Python access to the core functionality of the C++ Knoodle library for knot analysis. It enables:

- Analyzing 3D curves to determine if they form knots
- Generating PD codes and Gauss codes from knot representations
- Determining if a given representation is an unknot
- And more knot theory operations

## Requirements

### System Requirements

- Python 3.7 or higher
- C++ compiler with C++20 support:
  - GCC 10+ (Linux)
  - Clang 13+ (macOS)
- CMake 3.14+

### Python Dependencies

- pybind11 (installed automatically during setup)

### Platform-Specific Requirements

#### macOS
- macOS 13.4 or higher
- XCode command line tools

#### Linux
- Build essentials package

## Installation

### From Source

1. Clone the Knoodle repository with submodules:
   ```bash
   git clone --recursive https://github.com/yourusername/Knoodle.git
   cd Knoodle
   ```

2. Install the Python package:
   ```bash
   cd python
   pip install -e .
   ```

## Quick Start

```python
import knoodle
import numpy as np

# Create a 3D curve representing a knot
coordinates = np.array([
    # Your 3D points here
    [1.0, 0.0, 0.0],
    [0.0, 1.0, 0.0],
    # ... more points
])

print("Shape", coordinates.shape)

# Convert the NumPy array to a list as Knoodle expects
points = coordinates.flatten().tolist()

# Analyze the curve
result = knoodle.analyze_curve(points)

# Get information about the knot
pd_code = knoodle.get_pd_code(points)
gauss_code = knoodle.get_gauss_code(points)
is_unknot = knoodle.is_unknot(points)

print(f"PD Code: {pd_code}")
print(f"Gauss Code: {gauss_code}")
print(f"Is Unknot: {is_unknot}")

# Use the KnotAnalyzer class for more advanced operations
analyzer = knoodle.KnotAnalyzer(points)
# ...more operations with analyzer
```

## Knot identification

`KnotAnalyzer` computes a canonical **MacLeod code** for a simplified prime
diagram. To turn that code into a knot name (for example `3_1`, `4_1`,
`8_19`), look it up in a `KnotLookupTable`:

```python
import knoodle

analyzer = knoodle.KnotAnalyzer(points)

# Point this at a directory of Klut_Keys_NN.bin / Klut_Values_NN.tsv tables.
table = knoodle.KnotLookupTable("/path/to/tables", max_crossings=13)

print(table.lookup(analyzer))           # e.g. "3_1", "0_1" for the unknot, or None
print(table.lookup(analyzer.macleod_code()))  # look up a raw code (bytes or list of ints)

# Composite knots: identify each prime factor
for comp in analyzer.prime_components:
    print(table.lookup(comp))
```

`lookup` returns the knot name, `"0_1"` for the unknot, or `None` when the
code is not in the table (unknown knot, or crossing count above the loaded
tables). The table loads every `Klut_*` file it finds up to `max_crossings`.

### Generating the tables

The lookup tables are **not shipped** with pyknoodle (they are large, and the
13-crossing layer is expensive to build). This mirrors the C++ library, where
`PrimeKnotLookupTable` is constructed from a caller-supplied directory. To
build them yourself:

1. **Enumerate diagrams.** Generate 4-valent planar graphs with
   [plantri](https://users.cecs.anu.edu.au/~bdm/plantri/) for each crossing
   count.
2. **Sieve minimal diagrams.** Feed the plantri output to Knoodle's
   `PlantriSiever` (`src/PlantriSiever.hpp`), which enumerates the crossing-sign
   assignments, simplifies each with REAPR, and collects the MacLeod codes of
   the proven-minimal diagrams (including all four chirality/reversal variants).
3. **Name the classes.** Match each code class to a knot name using a knot
   census such as [SnapPy](https://snappy.computop.org/) or
   [Regina](https://regina-normal.github.io/) (torus and other non-hyperbolic
   knots need a small special case).
4. **Emit the tables.** Write `Klut_Keys_NN.bin` (the concatenated `NN`-byte
   codes, grouped by knot) and `Klut_Values_NN.tsv` (`name count` per line, in
   the same order) into a directory, then pass that directory to
   `KnotLookupTable`.

`KnotLookupTable` raises a `RuntimeError` with these instructions if it is
pointed at a directory containing no tables.

## Troubleshooting

If you encounter import errors, ensure:

1. The installed package matches your Python version
2. Your C++ compiler supports C++20 features
3. All submodules are properly initialized if installing from source
4. Try to use the latest versions of conda to avoid error with dependencies, like boost. 

For Linux users, if you see `GLIBCXX` version errors, ensure your GCC version is compatible.
