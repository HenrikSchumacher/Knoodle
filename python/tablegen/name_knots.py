"""Name prime knots from signed PD codes.

Hyperbolic knots are identified with SnapPy's census lookup (Rolfsen names
like ``8_19`` up to 10 crossings, Hoste-Thistlethwaite names like ``11a367``
above). The finitely many non-hyperbolic prime knots up to 13 crossings are
torus knots; those (and hard unknots surviving in the "other" set) are
recognized by matching Alexander-polynomial invariants computed with
pyknoodle, since SnapPy identification needs a hyperbolic structure.
"""

import re

import snappy

import knoodle

ROLFSEN = re.compile(r"\d+_\d+")
HT_NAME = re.compile(r"K(\d+[an]\d+)")

# Torus knots with at most 13 crossings; T(p, q) has crossing number
# min(p(q-1), q(p-1)).
TORUS_KNOTS = {
    (2, 3): "3_1",
    (2, 5): "5_1",
    (2, 7): "7_1",
    (2, 9): "9_1",
    (2, 11): "11a367",
    (2, 13): "13a4878",
    (3, 4): "8_19",
    (3, 5): "10_124",
}

# Evaluation points for the unit-independent Alexander invariants
# |Delta(x) * Delta(1/x)| (the t^k unit cancels in the product).
EVAL_POINTS = (2.0, 3.0)


def _poly_mul(a, b):
    out = [0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b):
            out[i + j] += x * y
    return out


def _poly_div(a, b):
    """Exact polynomial division of integer coefficient lists."""
    a = list(a)
    out = [0] * (len(a) - len(b) + 1)
    for i in range(len(out) - 1, -1, -1):
        q, r = divmod(a[i + len(b) - 1], b[-1])
        assert r == 0
        out[i] = q
        for j, y in enumerate(b):
            a[i + j] -= q * y
    assert not any(a)
    return out


def _torus_alexander(p, q):
    """Coefficients of Delta_{T(p,q)}(t) = (t^{pq}-1)(t-1)/((t^p-1)(t^q-1))."""
    def cyc(n):
        return [-1] + [0] * (n - 1) + [1]  # t^n - 1

    num = _poly_mul(cyc(p * q), cyc(1))
    return _poly_div(_poly_div(num, cyc(p)), cyc(q))


def _poly_eval(coeffs, x):
    v = 0.0
    for c in reversed(coeffs):
        v = v * x + c
    return v


def _invariants_from_poly(coeffs):
    det = abs(_poly_eval(coeffs, -1.0))
    prods = tuple(
        abs(_poly_eval(coeffs, x) * _poly_eval(coeffs, 1.0 / x))
        for x in EVAL_POINTS
    )
    return (det,) + prods


def _invariants_from_diagram(pd_rows):
    analyzer = knoodle.KnotAnalyzer.from_pd_code(
        [list(row) for row in pd_rows], simplify=False)
    det = abs(analyzer.alexander(-1.0).to_double())
    prods = tuple(
        abs(analyzer.alexander(x).to_double()
            * analyzer.alexander(1.0 / x).to_double())
        for x in EVAL_POINTS
    )
    return (det,) + prods


_FALLBACK = [("0_1", _invariants_from_poly([1]))] + [
    (name, _invariants_from_poly(_torus_alexander(p, q)))
    for (p, q), name in TORUS_KNOTS.items()
]


def _fallback_name(pd_rows):
    inv = _invariants_from_diagram(pd_rows)
    for name, ref in _FALLBACK:
        if all(abs(a - b) <= 1e-6 * max(1.0, abs(b)) for a, b in zip(inv, ref)):
            return name
    return None


def snappy_link(pd_rows):
    """spherogram Link from a Knoodle signed PD code (m x 5 rows). The first
    four columns already follow the standard convention (counterclockwise
    from the incoming under-arc); spherogram wants 1-based arc labels."""
    return snappy.Link([tuple(int(x) + 1 for x in row[:4]) for row in pd_rows])


def identify_name(pd_rows):
    """Return the knot's name, or None if it cannot be identified."""
    try:
        ids = snappy_link(pd_rows).exterior().identify()
    except (RuntimeError, ValueError):
        ids = []

    names = [m.name() for m in ids]
    for name in names:
        if ROLFSEN.fullmatch(name):
            return name
    for name in names:
        m = HT_NAME.fullmatch(name)
        if m:
            return m.group(1)

    # Non-hyperbolic (torus knots) or unknots that survived the sieve.
    return _fallback_name(pd_rows)


if __name__ == "__main__":
    # Smoke test: left trefoil.
    trefoil = [[5, 2, 0, 3, -1], [3, 0, 4, 1, -1], [1, 4, 2, 5, -1]]
    print("trefoil ->", identify_name(trefoil))
