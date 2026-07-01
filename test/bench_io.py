"""Shared reader for the frozen benchmark dataset (make_bench_set output) and the
common per-diagram result-row schema, for the Regina driver and the cross-check.

Mirrors bench_io.hpp. Dataset format (text):

    # ... provenance comment lines ...
    count N
    nc  x0 x1 x2 x3 s0  x0 x1 x2 x3 s1  ...      (N lines; nc crossings, 5 ints each)

Result-row schema (TSV, joined across engines by `index`):

    index  nc  construct_ns  compute_ns  result_key
"""

RESULT_HEADER = "# index\tnc\tconstruct_ns\tcompute_ns\tresult_key"


def load_dataset(path):
    """Return (provenance, diagrams).

    provenance : list of '#' header lines (str).
    diagrams   : list of (nc, crossings), where crossings is a list of nc lists
                 [x0, x1, x2, x3, sign] (0-based signed PD, one per crossing).
    """
    provenance = []
    diagrams = []
    have_count = False
    expected = None
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                provenance.append(line)
                continue
            parts = line.split()
            if not have_count:
                if parts[0] != "count" or len(parts) < 2:
                    raise ValueError(f"expected 'count N' before diagrams in {path}")
                expected = int(parts[1])
                have_count = True
                continue
            nc = int(parts[0])
            ints = list(map(int, parts[1:]))
            if len(ints) != 5 * nc:
                raise ValueError(f"malformed diagram line in {path}")
            crossings = [ints[5 * i : 5 * i + 5] for i in range(nc)]
            diagrams.append((nc, crossings))
    if expected is not None and len(diagrams) != expected:
        raise ValueError(f"diagram count mismatch in {path}: {len(diagrams)} != {expected}")
    return provenance, diagrams
