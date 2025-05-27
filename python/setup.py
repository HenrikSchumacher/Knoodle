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
        include_dirs=[
            pybind11.get_include(),
            knoodle_base_dir,
            os.path.join(knoodle_base_dir, 'submodules', 'Tensors'),
            os.path.join(knoodle_base_dir, 'submodules', 'Tensors', 'submodules', 'Tools'),
            os.path.join(knoodle_base_dir, 'src'),
        ],
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
    version='1.2.0',
    author='Dimos Gkountaroulis',
    description='Python bindings for Knoodle library',
    ext_modules=ext_modules,
    packages=['knoodle'],
    zip_safe=False,
    python_requires='>=3.7',
)