from setuptools import setup, Extension
import pybind11
import os

# Path to Knoodle headers
current_script_dir = os.path.dirname(os.path.abspath(__file__))
project_root_dir_guess = os.path.abspath(os.path.join(current_script_dir, '..'))
knoodle_base_dir = os.path.join(project_root_dir_guess)#, 'Knoodle')

print(f"DEBUG: knoodle_base_dir is set to: {knoodle_base_dir}")
if not os.path.isdir(knoodle_base_dir):
    raise FileNotFoundError(f"Knoodle directory not found at {knoodle_base_dir}")
if not os.path.isfile(os.path.join(knoodle_base_dir, 'Knoodle.hpp')):
    raise FileNotFoundError(f"Knoodle.hpp not found in {knoodle_base_dir}")

cpp_args = [
    '-std=c++20',
    '-fvisibility=hidden',
    '-g0',
    '-mmacosx-version-min=13.4',
    '-DPOLYFOLD_NO_QUATERNIONS=1',
    '-fenable-matrix',
    '-Wno-deprecated-declarations',
]

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
        extra_compile_args=cpp_args
    )
]

setup(
    name='pyknoodle', 
    version='1.0.0',
    author='Dimos Gkountaroulis',
    description='Python bindings for Knoodle knot analysis library',
    ext_modules=ext_modules,
    zip_safe=False,
    python_requires='>=3.7',
)