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
    # Add Accelerate framework for CBLAS headers (required for BLAS_Wrappers.hpp)
    import subprocess
    try:
        sdk_path = subprocess.check_output(['xcrun', '--show-sdk-path']).decode().strip()
        accelerate_headers = os.path.join(
            sdk_path,
            'System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Headers'
        )
        if os.path.exists(accelerate_headers):
            suitesparse_paths.append(accelerate_headers)
            print(f"DEBUG: Found Accelerate/CBLAS headers at {accelerate_headers}")
    except Exception as e:
        print(f"WARNING: Could not find Accelerate framework: {e}")

    # First try conda environment
    # CONDA_PREFIX may not be set during pip install, so also check sys.prefix
    conda_prefix = os.environ.get('CONDA_PREFIX', '')
    if not conda_prefix:
        # Try to detect from Python executable path
        python_prefix = sys.prefix
        if 'anaconda' in python_prefix or 'conda' in python_prefix or 'envs' in python_prefix:
            conda_prefix = python_prefix
    suitesparse_found = False

    if conda_prefix:
        # Conda SuiteSparse may have headers directly in include/ (not include/suitesparse/)
        conda_include = os.path.join(conda_prefix, 'include')
        conda_suitesparse_subdir = os.path.join(conda_prefix, 'include', 'suitesparse')
        conda_lib = os.path.join(conda_prefix, 'lib')

        # Check for umfpack.h in either location
        if os.path.exists(os.path.join(conda_include, 'umfpack.h')):
            suitesparse_paths.append(conda_include)
            suitesparse_lib_paths.append(conda_lib)
            suitesparse_found = True
            print(f"DEBUG: Found SuiteSparse in conda at {conda_include}")
        elif os.path.exists(conda_suitesparse_subdir) and os.path.exists(os.path.join(conda_suitesparse_subdir, 'umfpack.h')):
            suitesparse_paths.append(conda_suitesparse_subdir)
            suitesparse_paths.append(conda_include)
            suitesparse_lib_paths.append(conda_lib)
            suitesparse_found = True
            print(f"DEBUG: Found SuiteSparse in conda at {conda_suitesparse_subdir}")

    # Fallback to Homebrew if not found in conda
    if not suitesparse_found:
        homebrew_prefix = '/opt/homebrew'  # Apple Silicon
        if not os.path.exists(homebrew_prefix):
            homebrew_prefix = '/usr/local'  # Intel Mac

        suitesparse_base = os.path.join(homebrew_prefix, 'opt', 'suite-sparse')
        if os.path.exists(suitesparse_base):
            suitesparse_paths.append(os.path.join(suitesparse_base, 'include', 'suitesparse'))
            suitesparse_lib_paths.append(os.path.join(suitesparse_base, 'lib'))
            print(f"DEBUG: Found SuiteSparse at {suitesparse_base}")
        else:
            print(f"WARNING: SuiteSparse not found at {suitesparse_base}")

    # OpenBLAS - check conda first, then homebrew
    openblas_found = False
    if conda_prefix:
        conda_lib_path = os.path.join(conda_prefix, 'lib')
        conda_openblas_lib = os.path.join(conda_lib_path, 'libopenblas.dylib')
        if os.path.exists(conda_openblas_lib):
            # Ensure conda lib path is added
            if conda_lib_path not in suitesparse_lib_paths:
                suitesparse_lib_paths.append(conda_lib_path)
            suitesparse_libs.extend(['openblas'])
            openblas_found = True
            print(f"DEBUG: Found OpenBLAS in conda at {conda_prefix}")

    if not openblas_found:
        homebrew_prefix = '/opt/homebrew' if os.path.exists('/opt/homebrew') else '/usr/local'
        openblas_base = os.path.join(homebrew_prefix, 'opt', 'openblas')
        if os.path.exists(openblas_base):
            suitesparse_paths.append(os.path.join(openblas_base, 'include'))
            suitesparse_lib_paths.append(os.path.join(openblas_base, 'lib'))
            suitesparse_libs.extend(['openblas'])
            print(f"DEBUG: Found OpenBLAS at {openblas_base}")
        else:
            print(f"WARNING: OpenBLAS not found, using system BLAS/LAPACK")
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

# UMFPACK compatibility header for single-precision stubs
umfpack_compat_header = os.path.join(current_script_dir, 'src', 'umfpack_compat.hpp')

cpp_args = [
    '-std=c++20',
    '-fvisibility=hidden',
    '-g0',
    '-DPOLYFOLD_NO_QUATERNIONS=1',
    '-DUSE_UMFPACK=1',  # Enable Alexander polynomial computation
    '-Wno-deprecated-declarations',
    '-include', umfpack_compat_header,  # Include stubs for single-precision UMFPACK functions
]

if sys.platform == 'darwin':
    cpp_args.append('-mmacosx-version-min=13.4')
    cpp_args.append('-fenable-matrix')
    cpp_args.append('-fno-lto')  # Disable LTO to avoid conda clang/system linker mismatch
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
    os.path.join(knoodle_base_dir, 'submodules', 'Min-Cost-Flow-Class', 'OPTUtils'),
    os.path.join(knoodle_base_dir, 'submodules', 'Min-Cost-Flow-Class', 'MCFClass'),
    os.path.join(knoodle_base_dir, 'submodules', 'Min-Cost-Flow-Class', 'MCFSimplex'),
] + suitesparse_paths

link_args = []

if sys.platform == 'darwin':
    # Disable LTO to avoid conda clang/system linker mismatch
    link_args.append('-fno-lto')
    # Link against Accelerate framework for BLAS/LAPACK
    link_args.append('-framework')
    link_args.append('Accelerate')
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