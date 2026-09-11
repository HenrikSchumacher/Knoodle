#!/usr/bin/env python3
"""Emit a knot in arc presentation (open book form) as a .crd fixture.

An arc presentation puts the z-axis through the knot as a *binding*: every arc
lies in one half-plane through that axis, running

    (0,0,z) -> (a,b,z) -> (a,b,z1) -> (0,0,z1)

This is equivalent to a grid diagram, and it is the shape of the petaluma
random-knot model -- so it is what a generator in the wild hands to Knoodle.

It is also the most degenerate projection we can write down. Projected down z:

  * all n binding points land on the origin, so 2n radial segments corner there;
  * every vertical segment projects to a single point;
  * each arc's outbound and return segments project onto EXACTLY the same
    segment -- a complete collinear overlap of two non-adjacent edges.

There are no transversal crossings at all in the naive projection. Every
crossing in the answer comes from the symbolic perturbation, which is why this
is the sharpest available test of it.

Input is a grid diagram: two permutations X and O of range(n), where row i has
its X in column X[i] and its O in column O[i]. Column j becomes the page (the
half-plane) joining the two binding heights whose X or O sits in that column.

Usage:
    ./make_arc_presentation.py --name arc_trefoil \\
        --xs 0,1,2,3,4 --os 2,3,4,0,1 > arc_trefoil.crd
"""

import argparse
import math
import sys

# Integer page directions, in cyclic order around the origin. Integer entries
# keep the fixture exact (the test's oracle shears in exact integer arithmetic),
# and no two are parallel, so distinct pages stay distinct half-planes.
DIRECTIONS = [
    (1, 0), (1, 2), (-1, 1), (-2, -1), (1, -2),
    (2, 1), (0, 1), (-2, 1), (-1, -1), (-1, -2),
    (2, -1), (3, 1), (1, 3), (-1, 3), (-3, 1),
    (-3, -2), (-1, -3), (1, -3), (3, -2), (3, 2),
]


def pages_of(xs, os):
    """column -> the two binding heights it joins."""
    n = len(xs)
    out = {}
    for j in range(n):
        rows = [i for i in range(n) if xs[i] == j or os[i] == j]
        if len(rows) != 2:
            raise SystemExit(f"column {j} touches {len(rows)} rows, want 2 "
                             "(are X and O both permutations?)")
        out[j] = rows
    return out


def traverse(pages, n):
    """Walk the arcs, returning (page, from_height, to_height) in order."""
    at = {h: [j for j in pages if h in pages[j]] for h in range(n)}
    for h, ps in at.items():
        if len(ps) != 2:
            raise SystemExit(f"height {h} is on {len(ps)} pages, want 2")

    order, h, came = [], 0, None
    for _ in range(n):
        j = next(p for p in at[h] if p != came)
        nxt = next(k for k in pages[j] if k != h)
        order.append((j, h, nxt))
        came, h = j, nxt

    if h != 0:
        raise SystemExit("the arcs do not close into one component; this grid "
                         "describes a link, which needs one component per cycle")
    if len(order) != n:
        raise SystemExit("traversal did not use every arc")
    return order


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--xs", required=True, help="comma-separated X columns")
    ap.add_argument("--os", required=True, help="comma-separated O columns")
    ap.add_argument("--name", default="arc", help="for the provenance comment")
    args = ap.parse_args()

    xs = [int(v) for v in args.xs.split(",")]
    os_ = [int(v) for v in args.os.split(",")]
    n = len(xs)

    if sorted(xs) != list(range(n)) or sorted(os_) != list(range(n)):
        raise SystemExit("X and O must each be a permutation of 0..n-1")
    if n > len(DIRECTIONS):
        raise SystemExit(f"only {len(DIRECTIONS)} page directions are tabulated")

    pages = pages_of(xs, os_)
    order = traverse(pages, n)
    dirs = DIRECTIONS[:n]

    angles = [round(math.degrees(math.atan2(b, a)) % 360, 1) for a, b in dirs]
    print(f"# {args.name}: arc presentation, {n} arcs", file=sys.stderr)
    print(f"# pages {pages}", file=sys.stderr)
    print(f"# page angles {angles}", file=sys.stderr)

    for (j, h1, h2) in order:
        dx, dy = dirs[j]
        for v in ((0, 0, h1), (dx, dy, h1), (dx, dy, h2)):
            print(f"{v[0]} {v[1]} {v[2]}")


if __name__ == "__main__":
    main()
