"""Tests for figure-eight (4_1) and 5_1 knots using the Knoodle Python bindings."""

import math
import knoodle


def make_torus_knot_coords(p, q, n=300, R=3.0, r=1.0):
    """Generate coordinates for a (p,q)-torus knot."""
    coords = []
    for i in range(n):
        t = 2 * math.pi * i / n
        x = (R + r * math.cos(q * t)) * math.cos(p * t)
        y = (R + r * math.cos(q * t)) * math.sin(p * t)
        z = r * math.sin(q * t)
        coords.extend([x, y, z])
    return coords


def make_figure_eight_coords(n=200):
    """Generate coordinates for the figure-eight knot (4_1)."""
    coords = []
    for i in range(n):
        t = 2 * math.pi * i / n
        x = (2 + math.cos(2 * t)) * math.cos(3 * t)
        y = (2 + math.cos(2 * t)) * math.sin(3 * t)
        z = math.sin(4 * t)
        coords.extend([x, y, z])
    return coords


def test_figure_eight():
    """Test figure-eight knot (4_1) from coordinates."""
    print("=== Figure-eight knot (4_1) from coordinates ===")
    coords = make_figure_eight_coords()
    ka = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)

    assert ka.crossing_count == 4, f"Expected 4 crossings, got {ka.crossing_count}"
    assert ka.writhe == 0, f"Expected writhe 0, got {ka.writhe}"
    assert not ka.is_unknot(), "Should not be unknot"
    assert ka.is_prime, "Should be prime"
    assert not ka.is_composite, "Should not be composite"
    assert ka.prime_component_count == 1, f"Expected 1 prime component, got {ka.prime_component_count}"
    assert ka.link_component_count == 1, f"Expected 1 link component, got {ka.link_component_count}"
    assert ka.unlink_count == 0, f"Expected 0 unlinks, got {ka.unlink_count}"

    # Alexander polynomial: determinant of 4_1 is 5
    alex = ka.alexander(-1.0)
    assert abs(abs(alex.to_double()) - 5.0) < 1e-6, f"Expected |Alexander(-1)| = 5, got {alex.to_double()}"

    # Alexander(1) should be 1 for any knot
    alex1 = ka.alexander(1.0)
    assert abs(alex1.to_double() - 1.0) < 1e-6, f"Expected Alexander(1) = 1, got {alex1.to_double()}"

    # MacLeod code should be non-empty
    macleod = ka.macleod_string()
    assert len(macleod) > 0, "MacLeod string should be non-empty"
    print(f"  MacLeod: {macleod}")

    # Should be proven minimal (alternating knot)
    assert ka.proven_minimal(), "Figure-eight should be proven minimal"

    # PD code should exist
    assert len(ka.pd_code) > 0, "PD code should be non-empty"
    print(f"  PD code: {ka.pd_code}")

    # PD code round-trip
    matrix = ka.get_pd_code_matrix()
    assert len(matrix) == 4, f"Expected 4 crossings in matrix, got {len(matrix)}"
    ka_rt = knoodle.KnotAnalyzer.from_pd_code(matrix, simplify=False)
    assert ka_rt.crossing_count == 4, f"Round-trip: expected 4 crossings, got {ka_rt.crossing_count}"
    assert ka_rt.writhe == 0, f"Round-trip: expected writhe 0, got {ka_rt.writhe}"
    alex_rt = ka_rt.alexander(-1.0)
    assert abs(abs(alex_rt.to_double()) - 5.0) < 1e-6, f"Round-trip: expected |Alexander(-1)| = 5, got {alex_rt.to_double()}"

    # Handedness: figure-eight is alternating, so handedness alternates
    hand = ka.get_crossing_handedness()
    assert len(hand) == 4, f"Expected 4 handedness values, got {len(hand)}"
    pos = sum(1 for h in hand if h == 1)
    neg = sum(1 for h in hand if h == -1)
    assert pos == 2 and neg == 2, f"Expected 2 positive and 2 negative crossings, got {pos}+ {neg}-"

    print("  PASSED")


def test_5_1():
    """Test 5_1 knot (torus knot T(5,2)) from coordinates."""
    print("=== 5_1 knot (torus T(5,2)) from coordinates ===")
    coords = make_torus_knot_coords(5, 2, n=300)
    ka = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)

    assert ka.crossing_count == 5, f"Expected 5 crossings, got {ka.crossing_count}"
    assert abs(ka.writhe) == 5, f"Expected |writhe| = 5, got {ka.writhe}"
    assert not ka.is_unknot(), "Should not be unknot"
    assert ka.is_prime, "Should be prime"
    assert not ka.is_composite, "Should not be composite"
    assert ka.prime_component_count == 1, f"Expected 1 prime component, got {ka.prime_component_count}"
    assert ka.link_component_count == 1, f"Expected 1 link component, got {ka.link_component_count}"
    assert ka.unlink_count == 0, f"Expected 0 unlinks, got {ka.unlink_count}"

    # Alexander polynomial: determinant of 5_1 is 5
    alex = ka.alexander(-1.0)
    assert abs(abs(alex.to_double()) - 5.0) < 1e-6, f"Expected |Alexander(-1)| = 5, got {alex.to_double()}"

    # Alexander(1) should be 1
    alex1 = ka.alexander(1.0)
    assert abs(alex1.to_double() - 1.0) < 1e-6, f"Expected Alexander(1) = 1, got {alex1.to_double()}"

    # MacLeod code should be non-empty
    macleod = ka.macleod_string()
    assert len(macleod) > 0, "MacLeod string should be non-empty"
    print(f"  MacLeod: {macleod}")

    # Should be proven minimal (alternating torus knot)
    assert ka.proven_minimal(), "5_1 should be proven minimal"

    # PD code should exist
    assert len(ka.pd_code) > 0, "PD code should be non-empty"
    print(f"  PD code: {ka.pd_code}")

    # PD code round-trip
    matrix = ka.get_pd_code_matrix()
    assert len(matrix) == 5, f"Expected 5 crossings in matrix, got {len(matrix)}"
    ka_rt = knoodle.KnotAnalyzer.from_pd_code(matrix, simplify=False)
    assert ka_rt.crossing_count == 5, f"Round-trip: expected 5 crossings, got {ka_rt.crossing_count}"
    assert abs(ka_rt.writhe) == 5, f"Round-trip: expected |writhe| = 5, got {ka_rt.writhe}"
    alex_rt = ka_rt.alexander(-1.0)
    assert abs(abs(alex_rt.to_double()) - 5.0) < 1e-6, f"Round-trip: expected |Alexander(-1)| = 5, got {alex_rt.to_double()}"

    # 5_1 is a torus knot, all crossings have same sign
    hand = ka.get_crossing_handedness()
    assert len(hand) == 5, f"Expected 5 handedness values, got {len(hand)}"
    assert all(h == hand[0] for h in hand), f"All crossings should have same sign for torus knot, got {hand}"

    print("  PASSED")


def test_figure_eight_from_pd_code():
    """Test figure-eight from its known PD code."""
    print("=== Figure-eight knot from PD code ===")

    # First create from coordinates to get a valid PD code
    coords = make_figure_eight_coords()
    ka_ref = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)
    matrix = ka_ref.get_pd_code_matrix()

    # Reconstruct from that PD code with simplification
    ka = knoodle.KnotAnalyzer.from_pd_code(matrix, simplify=True, simplify_level=5)
    assert ka.crossing_count == 4, f"Expected 4 crossings, got {ka.crossing_count}"
    assert ka.writhe == 0, f"Expected writhe 0, got {ka.writhe}"
    assert abs(abs(ka.alexander(-1.0).to_double()) - 5.0) < 1e-6, "Alexander(-1) should be 5"

    # Also test unsigned PD code
    unsigned_matrix = [row[:4] for row in matrix]
    ka_u = knoodle.KnotAnalyzer.from_pd_code(unsigned_matrix, simplify=True, simplify_level=5)
    assert ka_u.crossing_count == 4, f"Unsigned: expected 4 crossings, got {ka_u.crossing_count}"
    assert abs(abs(ka_u.alexander(-1.0).to_double()) - 5.0) < 1e-6, "Unsigned: Alexander(-1) should be 5"

    print("  PASSED")


def test_5_1_from_pd_code():
    """Test 5_1 from its known PD code."""
    print("=== 5_1 knot from PD code ===")

    # Create from coordinates to get valid PD code
    coords = make_torus_knot_coords(5, 2, n=300)
    ka_ref = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)
    matrix = ka_ref.get_pd_code_matrix()

    # Reconstruct from PD code
    ka = knoodle.KnotAnalyzer.from_pd_code(matrix, simplify=True, simplify_level=5)
    assert ka.crossing_count == 5, f"Expected 5 crossings, got {ka.crossing_count}"
    assert abs(ka.writhe) == 5, f"Expected |writhe| = 5, got {ka.writhe}"
    assert abs(abs(ka.alexander(-1.0).to_double()) - 5.0) < 1e-6, "Alexander(-1) should be 5"

    # Unsigned PD code
    unsigned_matrix = [row[:4] for row in matrix]
    ka_u = knoodle.KnotAnalyzer.from_pd_code(unsigned_matrix, simplify=True, simplify_level=5)
    assert ka_u.crossing_count == 5, f"Unsigned: expected 5 crossings, got {ka_u.crossing_count}"
    assert abs(abs(ka_u.alexander(-1.0).to_double()) - 5.0) < 1e-6, "Unsigned: Alexander(-1) should be 5"

    print("  PASSED")


def test_batch_alexander():
    """Test batch Alexander polynomial evaluation for both knots."""
    print("=== Batch Alexander for both knots ===")

    coords_41 = make_figure_eight_coords()
    ka_41 = knoodle.KnotAnalyzer(coords_41, simplify=True)

    coords_51 = make_torus_knot_coords(5, 2, n=300)
    ka_51 = knoodle.KnotAnalyzer(coords_51, simplify=True)

    test_points = [-1.0, 1.0, 2.0, -2.0]

    results_41 = ka_41.alexander(test_points)
    assert len(results_41) == 4, f"Expected 4 results for 4_1, got {len(results_41)}"
    assert abs(abs(results_41[0].to_double()) - 5.0) < 1e-6, "4_1: Alexander(-1) should be 5"
    assert abs(results_41[1].to_double() - 1.0) < 1e-6, "4_1: Alexander(1) should be 1"

    results_51 = ka_51.alexander(test_points)
    assert len(results_51) == 4, f"Expected 4 results for 5_1, got {len(results_51)}"
    assert abs(abs(results_51[0].to_double()) - 5.0) < 1e-6, "5_1: Alexander(-1) should be 5"
    assert abs(results_51[1].to_double() - 1.0) < 1e-6, "5_1: Alexander(1) should be 1"

    print("  PASSED")


def test_simplify_levels():
    """Test that all simplify levels produce correct results for both knots."""
    print("=== Simplify levels for both knots ===")

    coords_41 = make_figure_eight_coords()
    coords_51 = make_torus_knot_coords(5, 2, n=300)

    for level in [1, 2, 3, 4, 5]:
        ka_41 = knoodle.KnotAnalyzer(coords_41, simplify=True, simplify_level=level)
        assert ka_41.crossing_count == 4, f"4_1 level {level}: expected 4 crossings, got {ka_41.crossing_count}"

        ka_51 = knoodle.KnotAnalyzer(coords_51, simplify=True, simplify_level=level)
        assert ka_51.crossing_count == 5, f"5_1 level {level}: expected 5 crossings, got {ka_51.crossing_count}"

    print("  PASSED")


def test_invariants():
    """Test Alexander invariants at standard evaluation points."""
    print("=== Alexander invariants ===")

    coords_41 = make_figure_eight_coords()
    ka_41 = knoodle.KnotAnalyzer(coords_41, simplify=True)
    inv_41 = ka_41.alexander_invariants()

    assert "at_minus_1" in inv_41, "Missing at_minus_1"
    assert "at_1" in inv_41, "Missing at_1"
    assert "at_i" in inv_41, "Missing at_i"
    assert abs(abs(inv_41["at_minus_1"].to_double()) - 5.0) < 1e-6, "4_1: |Alexander(-1)| should be 5"
    assert abs(inv_41["at_1"].to_double() - 1.0) < 1e-6, "4_1: Alexander(1) should be 1"

    coords_51 = make_torus_knot_coords(5, 2, n=300)
    ka_51 = knoodle.KnotAnalyzer(coords_51, simplify=True)
    inv_51 = ka_51.alexander_invariants()

    assert abs(abs(inv_51["at_minus_1"].to_double()) - 5.0) < 1e-6, "5_1: |Alexander(-1)| should be 5"
    assert abs(inv_51["at_1"].to_double() - 1.0) < 1e-6, "5_1: Alexander(1) should be 1"

    print("  PASSED")


def test_macleod_agreement_with_shipped_tables():
    """KnotAnalyzer computes MacLeod codes with the current PlanarDiagram
    class, while the shipped data/Klut tables were generated by upstream's
    independent Klutter pipeline. Lookups only work if the two encodings
    agree, which nothing in the library enforces -- pin it here."""
    print("=== MacLeod code agreement: KnotAnalyzer vs shipped data/Klut ===")
    import pytest
    try:
        table = knoodle.KnotLookupTable()
    except RuntimeError as e:
        pytest.skip(f"shipped tables unavailable: {e}")

    for coords, c, name in [
        (make_torus_knot_coords(2, 3), 3, "3_1"),
        (make_figure_eight_coords(), 4, "4_1"),
        (make_torus_knot_coords(5, 2), 5, "5_1"),
    ]:
        ka = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)
        assert ka.crossing_count == c

        code = bytes(ka.macleod_code())
        assert table.lookup(code) == name, (
            f"KnotAnalyzer MacLeod code for {name} not found in data/Klut")
        raw = table.lookup_raw(code)
        assert raw is not None and raw.startswith(f"K[{c},"), raw
    print("  PASSED")


def test_fp_environment_restored():
    """Knoodle's AABBTree sets the FE_UPWARD rounding mode internally; the
    bindings must restore the floating-point environment before returning
    (a leak breaks numpy and every other float consumer in the process)."""
    import ctypes
    libm = ctypes.CDLL(None)
    knoodle.KnotAnalyzer(make_torus_knot_coords(2, 3),
                         simplify=True, simplify_level=5)
    assert libm.fegetround() == 0, "rounding mode leaked (not FE_TONEAREST)"


if __name__ == "__main__":
    test_figure_eight()
    test_5_1()
    test_figure_eight_from_pd_code()
    test_5_1_from_pd_code()
    test_batch_alexander()
    test_simplify_levels()
    test_invariants()
    test_macleod_agreement_with_shipped_tables()
    test_fp_environment_restored()
    print("\nALL TESTS PASSED!")
