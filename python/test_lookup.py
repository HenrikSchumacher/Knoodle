"""Tests for KnotLookupTable over the shipped data/Klut tables, including the
cross-table naming regression against the retired tablegen pipeline's tables
(python/fixtures/snappy_tables, SnapPy-named, c = 3..10)."""

import collections
import math
import os

import pytest

import knoodle

FIXTURE_DIR = os.path.join(os.path.dirname(__file__), "fixtures", "snappy_tables")

# The complete, empirically verified divergence between SnapPy names (used by
# the old tables) and KnotInfo names (used by data/Klut). Everything else
# agrees on all 8218 keys of the c = 3..10 tables:
#  - the Perko-pair renumbering: SnapPy keeps the original Rolfsen labels
#    above the removed duplicate (no 10_162, up to 10_166), KnotInfo shifts
#    them down by one, and
#  - the classic 10_83 / 10_86 swap.
SNAPPY_TO_KNOTINFO = {
    "10_83": "10_86",
    "10_86": "10_83",
    "10_163": "10_162",
    "10_164": "10_163",
    "10_165": "10_164",
    "10_166": "10_165",
}


def make_torus_knot_coords(p, q, n=300, R=3.0, r=1.0, mirror=False):
    coords = []
    for i in range(n):
        t = 2 * math.pi * i / n
        z = r * math.sin(q * t)
        coords.extend([
            (R + r * math.cos(q * t)) * math.cos(p * t),
            (R + r * math.cos(q * t)) * math.sin(p * t),
            -z if mirror else z,
        ])
    return coords


def make_figure_eight_coords(n=200):
    coords = []
    for i in range(n):
        t = 2 * math.pi * i / n
        coords.extend([
            (2 + math.cos(2 * t)) * math.cos(3 * t),
            (2 + math.cos(2 * t)) * math.sin(3 * t),
            math.sin(4 * t),
        ])
    return coords


def make_granny_coords(n=96):
    def trefoil(center, zshift):
        pts = []
        for k in range(n):
            t = 2 * math.pi * k / n
            pts.append((
                math.sin(t) + 2 * math.sin(2 * t) + center,
                math.cos(t) - 2 * math.cos(2 * t),
                -math.sin(3 * t) + zshift,
            ))
        return pts

    a = trefoil(-5.0, 0.0)
    b = trefoil(5.0, 0.3)
    ia = max(range(n), key=lambda i: a[i][0])
    ib = min(range(n), key=lambda i: b[i][0])
    loop = a[ia:] + a[:ia] + b[ib:] + b[:ib]
    return [x for p in loop for x in p]


def default_table():
    try:
        return knoodle.KnotLookupTable()
    except RuntimeError as e:
        pytest.skip(f"shipped tables unavailable: {e}")


def read_fixture_table(c):
    values = os.path.join(FIXTURE_DIR, f"Klut_Values_{c:02d}.tsv")
    keys_file = os.path.join(FIXTURE_DIR, f"Klut_Keys_{c:02d}.bin")
    if not (os.path.exists(values) and os.path.exists(keys_file)):
        pytest.skip(f"fixture tables for c={c} not present")
    keys = open(keys_file, "rb").read()
    if keys.startswith(b"version https://git-lfs"):
        pytest.skip("fixture tables are LFS pointers (run 'git lfs pull')")
    names = []
    with open(values) as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2:
                names.append((parts[0], int(parts[-1])))
    out = []
    pos = 0
    for name, count in names:
        for _ in range(count):
            out.append((name, keys[pos:pos + c]))
            pos += c
    assert pos == len(keys), f"key file length mismatch for c={c}"
    return out


def test_default_table_loads():
    table = default_table()
    assert table.max_crossings == 13


def test_basic_names():
    table = default_table()

    trefoil = knoodle.KnotAnalyzer(
        make_torus_knot_coords(2, 3), simplify=True, simplify_level=5)
    assert table.lookup(trefoil) == "3_1"
    assert table.lookup_raw(trefoil).startswith("K[3,1,1,")

    fig8 = knoodle.KnotAnalyzer(
        make_figure_eight_coords(), simplify=True, simplify_level=5)
    assert table.lookup(fig8) == "4_1"
    # The figure-eight is fully amphichiral: one coset covering all variants.
    assert table.lookup_raw(fig8) == 'K[4,1,1,"e/m/r/mr"]'

    five_one = knoodle.KnotAnalyzer(
        make_torus_knot_coords(5, 2), simplify=True, simplify_level=5)
    assert table.lookup(five_one) == "5_1"


def test_unknot():
    table = default_table()
    coords = []
    for k in range(32):
        t = 2 * math.pi * k / 32
        coords.extend([math.cos(t), math.sin(t), 0.0])
    ka = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)
    assert ka.crossing_count == 0
    assert table.lookup(ka) == "0_1"


def test_chirality_cosets():
    table = default_table()
    raw = set()
    for mirror in (False, True):
        ka = knoodle.KnotAnalyzer(
            make_torus_knot_coords(2, 3, mirror=mirror),
            simplify=True, simplify_level=5)
        assert table.lookup(ka) == "3_1"
        raw.add(table.lookup_raw(ka))
    # The two mirror images fall in the trefoil's two chirality cosets.
    assert raw == {'K[3,1,1,"e/r"]', 'K[3,1,1,"m/mr"]'}


def test_granny_components():
    table = default_table()
    ka = knoodle.KnotAnalyzer(make_granny_coords(), simplify=True, simplify_level=5)
    assert ka.is_composite
    assert ka.prime_component_count == 2
    assert [table.lookup(c) for c in ka.prime_components] == ["3_1", "3_1"]


def test_partial_table():
    table = default_table()  # skip early if tables are absent
    small = knoodle.KnotLookupTable(max_crossings=5)
    assert small.max_crossings == 5

    fig8 = knoodle.KnotAnalyzer(
        make_figure_eight_coords(), simplify=True, simplify_level=5)
    assert small.lookup(fig8) == "4_1"

    # A code longer than the loaded range cannot be found.
    code = [0] * 7
    assert small.lookup(code) is None
    del table


def test_missing_directory():
    with pytest.raises(RuntimeError, match="git lfs"):
        knoodle.KnotLookupTable("/nonexistent/klut/dir")


def test_cross_table_regression():
    """Every key of the old SnapPy-named tables must be found in data/Klut,
    and the names must agree through exactly the known divergence map."""
    table = default_table()

    total = 0
    diverge = collections.Counter()
    for c in range(3, 11):
        for snappy_name, key in read_fixture_table(c):
            total += 1
            got = table.lookup(list(key))
            assert got is not None, (
                f"key of {snappy_name} (c={c}) missing from data/Klut")
            if got != snappy_name:
                diverge[(snappy_name, got)] += 1

    assert total == 8218, f"fixture corpus changed size: {total}"
    assert dict(diverge) == {
        (snappy, knotinfo): 16
        for snappy, knotinfo in SNAPPY_TO_KNOTINFO.items()
    }
