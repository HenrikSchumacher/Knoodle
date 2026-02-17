
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
