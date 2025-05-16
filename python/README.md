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
points = np.array([
    # Your 3D points here
    [1.0, 0.0, 0.0],
    [0.0, 1.0, 0.0],
    # ... more points
])

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

## Troubleshooting

If you encounter import errors, ensure:

1. The installed package matches your Python version
2. Your C++ compiler supports C++20 features
3. All submodules are properly initialized if installing from source

For Linux users, if you see `GLIBCXX` version errors, ensure your GCC version is compatible.
