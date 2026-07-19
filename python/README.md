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

The lookup tables are **not shipped** with pyknoodle (the high-crossing layers
are expensive to build). This mirrors the C++ library, where
`PrimeKnotLookupTable` is constructed from a caller-supplied directory. The
`tablegen/` directory contains a complete generation pipeline:

1. **Enumerate shadows** (`tablegen/plantri_shadows.py`): runs
   [plantri](https://users.cecs.anu.edu.au/~bdm/plantri/)
   (`-q -c2m2 -d -E`, i.e. duals of arbitrary simple quadrangulations = prime
   reduced shadows) and converts each single-component shadow to an unsigned
   PD code.
2. **Sieve minimal diagrams** (`knoodle.sieve_shadows`, modeled on
   `src/PlantriSiever.hpp`): enumerates every over/under assignment,
   simplifies with REAPR, and collects the MacLeod codes of diagrams that
   stay at c crossings (all four chirality/reversal variants), with one
   representative PD code each.
3. **Name the knots** (`tablegen/name_knots.py`): identifies each
   representative with [SnapPy](https://snappy.computop.org/); the
   non-hyperbolic torus knots (and surviving hard unknots) are matched by
   Alexander-polynomial invariants instead.
4. **Emit and validate** (`tablegen/make_tables.py`): writes
   `Klut_Keys_NN.bin` / `Klut_Values_NN.tsv` and checks the number of
   distinct knots per crossing number against the knot census.

Setup and run (SnapPy lives in a venv because conda's `python-snappy`
compression package occupies the `snappy` module name):

```bash
python3 -m venv --system-site-packages python/tablegen/.venv
python/tablegen/.venv/bin/pip install snappy
python/tablegen/.venv/bin/python3 python/tablegen/make_tables.py \
    --plantri /path/to/plantri --out /path/to/tables --max-crossings 10
```

`KnotLookupTable` raises a `RuntimeError` with these instructions if it is
pointed at a directory containing no tables.

## Troubleshooting

If you encounter import errors, ensure:

1. The installed package matches your Python version
2. Your C++ compiler supports C++20 features
3. All submodules are properly initialized if installing from source
4. Try to use the latest versions of conda to avoid error with dependencies, like boost. 

For Linux users, if you see `GLIBCXX` version errors, ensure your GCC version is compatible.
