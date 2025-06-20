from setuptools import setup, Extension
import pybind11
import os
import sys

# Path to Knoodle headers
current_script_dir = os.path.dirname(os.path.abspath(__file__))
knoodle_base_dir = os.path.abspath(os.path.join(current_script_dir, '..'))
 

print(f"DEBUG: knoodle_base_dir is set to: {knoodle_base_dir}")
if not os.path.isdir(knoodle_base_dir):
    raise FileNotFoundError(f"Knoodle directory not found at {knoodle_base_dir}")
if not os.path.isfile(os.path.join(knoodle_base_dir, 'Knoodle.hpp')):
    raise FileNotFoundError(f"Knoodle.hpp not found in {knoodle_base_dir}")

# SuiteSparse/UMFPACK configuration
suitesparse_paths = []
suitesparse_lib_paths = []
suitesparse_libs = ['umfpack', 'amd', 'cholmod', 'colamd', 'suitesparseconfig']

if sys.platform == 'darwin':
    # macOS with Homebrew
    homebrew_prefix = '/opt/homebrew'  # Apple Silicon
    if not os.path.exists(homebrew_prefix):
        homebrew_prefix = '/usr/local'  # Intel Mac
    
    # SuiteSparse
    suitesparse_base = os.path.join(homebrew_prefix, 'opt', 'suite-sparse')
    if os.path.exists(suitesparse_base):
        suitesparse_paths.append(os.path.join(suitesparse_base, 'include', 'suitesparse'))
        suitesparse_lib_paths.append(os.path.join(suitesparse_base, 'lib'))
        print(f"DEBUG: Found SuiteSparse at {suitesparse_base}")
    else:
        print(f"WARNING: SuiteSparse not found at {suitesparse_base}")
    
    # OpenBLAS (required for BLAS/LAPACK on macOS)
    openblas_base = os.path.join(homebrew_prefix, 'opt', 'openblas')
    if os.path.exists(openblas_base):
        suitesparse_paths.append(os.path.join(openblas_base, 'include'))
        suitesparse_lib_paths.append(os.path.join(openblas_base, 'lib'))
        suitesparse_libs.extend(['openblas'])  # OpenBLAS provides BLAS/LAPACK
        print(f"DEBUG: Found OpenBLAS at {openblas_base}")
    else:
        print(f"WARNING: OpenBLAS not found at {openblas_base}")
        # Fallback to system BLAS/LAPACK (Accelerate framework)
        suitesparse_libs.extend(['blas', 'lapack'])
        
elif sys.platform.startswith('linux'):
    # Linux - check multiple common locations
    linux_include_candidates = [
        '/usr/include/suitesparse',      # Ubuntu/Debian standard
        '/usr/local/include/suitesparse', # Local install
        '/usr/include',                   # Some distros put headers directly here
        '/opt/suitesparse/include',      # Custom install location
    ]
    
    linux_lib_candidates = [
        '/usr/lib',                      # Standard location
        '/usr/lib/x86_64-linux-gnu',     # Ubuntu/Debian x86_64
        '/usr/lib/aarch64-linux-gnu',    # Ubuntu/Debian ARM64
        '/usr/lib64',                    # RedHat/CentOS/Fedora
        '/usr/local/lib',                # Local install
        '/usr/local/lib64',              # Local install 64-bit
        '/opt/suitesparse/lib',          # Custom install
    ]
    
    # Find include directory
    suitesparse_include_found = False
    for path in linux_include_candidates:
        umfpack_header = os.path.join(path, 'suitesparse', 'umfpack.h') if path.endswith('include') else os.path.join(path, 'umfpack.h')
        if os.path.exists(umfpack_header):
            if path.endswith('suitesparse'):
                suitesparse_paths.append(path)
            else:
                suitesparse_paths.append(os.path.join(path, 'suitesparse'))
            suitesparse_include_found = True
            print(f"DEBUG: Found SuiteSparse headers at {path}")
            break
    
    if not suitesparse_include_found:
        print("WARNING: SuiteSparse headers not found in standard locations")
    
    # Find library directory
    suitesparse_lib_found = False
    for path in linux_lib_candidates:
        umfpack_lib = os.path.join(path, 'libumfpack.so')
        if os.path.exists(umfpack_lib):
            suitesparse_lib_paths.append(path)
            suitesparse_lib_found = True
            print(f"DEBUG: Found SuiteSparse libraries at {path}")
            break
    
    if not suitesparse_lib_found:
        print("WARNING: SuiteSparse libraries not found in standard locations")
    
    # Add BLAS/LAPACK dependencies for Linux
    suitesparse_libs.extend(['blas', 'lapack'])
    
else:
    print(f"WARNING: Unsupported platform {sys.platform} for SuiteSparse auto-detection")

cpp_args = [
    '-std=c++20',
    '-fvisibility=hidden',
    '-g0',
    '-DPOLYFOLD_NO_QUATERNIONS=1',
    '-Wno-deprecated-declarations',
]

if sys.platform == 'darwin':
    cpp_args.append('-mmacosx-version-min=13.4')
    cpp_args.append('-fenable-matrix')
elif sys.platform.startswith('linux'): 
    compat_header = os.path.join(current_script_dir, 'src', 'linux_compat.hpp')
    cpp_args = ['-include', compat_header] + cpp_args
    cpp_args.append('-fpermissive')
    cpp_args.append('-DUSE_FASTINTS=1')  

# Include directories
include_dirs = [
    pybind11.get_include(),
    knoodle_base_dir,
    os.path.join(knoodle_base_dir, 'submodules', 'Tensors'),
    os.path.join(knoodle_base_dir, 'submodules', 'Tensors', 'submodules', 'Tools'),
    os.path.join(knoodle_base_dir, 'src'),
] + suitesparse_paths

link_args = []

if sys.platform == 'darwin':
    pass
elif sys.platform.startswith('linux'):
    link_args.append('-Wl,--strip-all')
    link_args.append('-Wl,-rpath,$ORIGIN')

ext_modules = [
    Extension(
        'knoodle._knoodle',
        sources=[
            'src/bindings.cpp',
            'src/wrapper.cpp'
        ],
        include_dirs=include_dirs,
        library_dirs=suitesparse_lib_paths,
        libraries=suitesparse_libs,
        language='c++',
        extra_compile_args=cpp_args,
        extra_link_args=link_args
    )
]

knoodle_dir = os.path.join(current_script_dir, 'knoodle')
if not os.path.exists(knoodle_dir):
    os.makedirs(knoodle_dir)
    
knoodle_dir = os.path.join(current_script_dir, 'knoodle')
if not os.path.exists(knoodle_dir):
    os.makedirs(knoodle_dir)
    
init_file = os.path.join(knoodle_dir, '__init__.py')
if not os.path.exists(init_file):
    with open(init_file, 'w') as f:
        f.write('''
try:
    from ._knoodle import *
    # Import specific names to verify everything works
    from ._knoodle import KnotAnalyzer, analyze_curve, get_pd_code, get_gauss_code, is_unknot
except ImportError as e:
    import sys
    print(f"Error importing from ._knoodle: {e}", file=sys.stderr)
    try:
        # Alternative import path in case the module is not correctly located
        import importlib.util
        import os
        
        # Try to find the module file
        for path in os.listdir(os.path.dirname(__file__)):
            if path.startswith('_knoodle') and path.endswith(('.so', '.pyd')):
                print(f"Found module file: {path}", file=sys.stderr)
                
        # Try direct import
        import _knoodle
        from _knoodle import *
        print("Successfully imported via direct import", file=sys.stderr)
    except ImportError as e2:
        print(f"Alternative import also failed: {e2}", file=sys.stderr)
''')


setup(
    name='pyknoodle', 
    version='1.2.2',
    author='Dimos Gkountaroulis',
    description='Python bindings for Knoodle library',
    ext_modules=ext_modules,
    packages=['knoodle'],
    zip_safe=False,
    python_requires='>=3.7',
)