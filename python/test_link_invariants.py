"""Tests for link_invariants: the m x (m+2) Alexander face matrix (link
determinant) and the m x 5 PD code (pairwise linking numbers)."""

import math

import numpy as np
import pytest

import knoodle


def circle(n, radius=1.0, center=(0.0, 0.0, 0.0), plane="xy", tilt=0.0):
    """A closed polygon on a circle. plane='xy' or 'xz'; tilt rotates an xy
    circle about the x-axis (0 keeps it planar in xy)."""
    coords = []
    c, s = math.cos(tilt), math.sin(tilt)
    for k in range(n):
        t = 2 * math.pi * k / n
        u, v = radius * math.cos(t), radius * math.sin(t)
        if plane == "xy":
            x, y, z = u, c * v, s * v
        else:
            x, y, z = u, -s * v, c * v
        coords.extend([x + center[0], y + center[1], z + center[2]])
    return coords


def cycle_edges(offset, n):
    edges = []
    for k in range(n):
        edges.extend([offset + k, offset + (k + 1) % n])
    return edges


def hopf(plane_b="xz", tilt_b=0.0, m=32):
    coords = circle(m) + circle(m, center=(1.0, 0.0, 0.0), plane=plane_b, tilt=tilt_b)
    edges = cycle_edges(0, m) + cycle_edges(m, m)
    return coords, edges


def abs_det(A, drop):
    A = np.asarray(A)
    M = np.delete(A, list(drop), axis=1)
    assert M.shape[0] == M.shape[1]
    return abs(np.linalg.det(M))


def test_hopf_determinant():
    coords, edges = hopf(plane_b="xy", tilt_b=0.4)
    A, drop, pd = knoodle.link_invariants(coords, edges, complex(-1.0, 0.0))

    assert len(pd) == 2 and all(len(row) == 5 for row in pd)
    assert abs(sum(row[4] for row in pd)) == 2  # 2 |lk| = 2

    assert len(A) == 2 and len(A[0]) == 4  # m x (m+2)
    assert abs_det(A, drop) == pytest.approx(2.0, abs=1e-9)  # det(Hopf) = 2

    A_i, drop_i, _ = knoodle.link_invariants(coords, edges, complex(0.0, 1.0))
    assert abs_det(A_i, drop_i) == pytest.approx(math.sqrt(2.0), abs=1e-9)


def test_hopf_degenerate_projection():
    """Circle B in the xz-plane makes the plain z-projection degenerate; the
    random-rotation retry must recover the same invariants."""
    coords, edges = hopf(plane_b="xz")
    A, drop, pd = knoodle.link_invariants(coords, edges, complex(-1.0, 0.0))

    assert len(pd) == 2
    assert abs(sum(row[4] for row in pd)) == 2
    assert abs_det(A, drop) == pytest.approx(2.0, abs=1e-9)


def test_trefoil_via_face_matrix():
    """Single-component input: the face-matrix determinant at -1 must agree
    with KnotAnalyzer's Alexander determinant."""
    n = 96
    coords = []
    for k in range(n):
        t = 2 * math.pi * k / n
        coords.extend([
            math.sin(t) + 2 * math.sin(2 * t),
            math.cos(t) - 2 * math.cos(2 * t),
            -math.sin(3 * t),
        ])
    edges = cycle_edges(0, n)

    A, drop, pd = knoodle.link_invariants(coords, edges, complex(-1.0, 0.0))
    assert len(pd) == 3
    assert abs_det(A, drop) == pytest.approx(3.0, abs=1e-9)

    ka = knoodle.KnotAnalyzer(coords, simplify=True, simplify_level=5)
    assert ka.alexander(-1.0).to_double() == pytest.approx(-3.0, abs=1e-9)


def test_split_unlink_guard():
    """Two far-apart circles: a split unlink has no crossings left after
    simplification, so no matrix and no PD code are produced."""
    m = 24
    coords = circle(m) + circle(m, center=(10.0, 0.0, 0.0), tilt=0.3)
    edges = cycle_edges(0, m) + cycle_edges(m, m)

    A, drop, pd = knoodle.link_invariants(coords, edges, complex(-1.0, 0.0))
    assert A == []
    assert pd == []
    assert tuple(drop) == (-1, -1)
