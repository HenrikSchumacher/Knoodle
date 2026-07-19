"""Enumerate prime reduced knot shadows with plantri.

Runs ``plantri -q -c2m2 -d -E (c+2)`` -- arbitrary simple quadrangulations of
the sphere, dualized -- to obtain the 4-edge-connected quartic planar
multigraphs on c vertices. These are exactly the shadows that can carry
reduced prime diagrams: 4-edge-connectivity rules out nugatory crossings and
connected sums. Each single-component (knot) shadow is converted to an
unsigned PD code in Knoodle's convention: arcs numbered 0..2c-1 along the
traversal, 4 entries per crossing in rotation order starting at an incoming
arc (so the entry opposite the first is its successor arc).
"""

import subprocess

EDGE_CODE_HEADER = b">>edge_code<<"


def run_plantri(crossing_count, plantri="plantri"):
    """Run plantri and return the raw dual edge-code bytes."""
    n = crossing_count + 2  # quadrangulation vertices; dual has n-2 vertices
    result = subprocess.run(
        [plantri, "-q", "-c2m2", "-d", "-E", str(n)],
        capture_output=True,
        check=True,
    )
    return result.stdout


def parse_edge_code(data):
    """Yield one graph per generated map: a list of per-vertex rotations,
    each rotation being the list of incident edge numbers in rotation order."""
    if not data.startswith(EDGE_CODE_HEADER):
        raise ValueError("not an edge_code stream")
    pos = len(EDGE_CODE_HEADER)
    while pos < len(data):
        length = data[pos]
        pos += 1
        if length == 0:
            # Large-graph encoding (never needed for c <= 16).
            raise NotImplementedError("edge_code big-graph form not supported")
        body = data[pos:pos + length]
        pos += length
        rotations = [[]]
        for b in body:
            if b == 255:
                rotations.append([])
            else:
                rotations[-1].append(b)
        yield rotations


def shadow_pd_code(rotations):
    """Convert a quartic planar map to a flat unsigned PD code.

    Returns None when the shadow has more than one strand component
    (a link shadow rather than a knot shadow).
    """
    c = len(rotations)
    if any(len(rot) != 4 for rot in rotations):
        raise ValueError("map is not 4-regular")

    # Each edge number occurs at exactly two darts (vertex, slot).
    edge_ends = {}
    for v, rot in enumerate(rotations):
        for s, e in enumerate(rot):
            edge_ends.setdefault(e, []).append((v, s))
    if any(len(ends) != 2 for ends in edge_ends.values()):
        raise ValueError("edge does not have exactly two ends")

    def step(v, s):
        """From the dart entering v at slot s, go straight through and
        return the dart entering the next vertex."""
        s_out = (s + 2) % 4
        e = rotations[v][s_out]
        (v1, s1), (v2, s2) = edge_ends[e]
        return (v2, s2) if (v1, s1) == (v, s_out) else (v1, s1)

    # Trace the strand through the shadow; a knot shadow has one strand,
    # i.e. the orbit of entering darts has length 2c.
    start = (0, 0)
    orbit = []
    cur = start
    while True:
        orbit.append(cur)
        cur = step(*cur)
        if cur == start:
            break
    if len(orbit) != 2 * c:
        return None

    # Entering dart k carries incoming arc k; the strand leaves through the
    # opposite slot as arc k+1 (mod 2c).
    entering = {}
    slot_arc = {}
    for k, (v, s) in enumerate(orbit):
        entering.setdefault(v, []).append(s)
        slot_arc[(v, s)] = k
        slot_arc[(v, (s + 2) % 4)] = (k + 1) % (2 * c)

    code = []
    for v in range(c):
        s0 = entering[v][0]
        code.extend(slot_arc[(v, (s0 + i) % 4)] for i in range(4))
    return code


def knot_shadows(crossing_count, plantri="plantri"):
    """All prime reduced knot shadows with the given crossing count, as a
    list of flat unsigned PD codes (4 ints per crossing)."""
    shadows = []
    for rotations in parse_edge_code(run_plantri(crossing_count, plantri)):
        code = shadow_pd_code(rotations)
        if code is not None:
            shadows.append(code)
    return shadows


if __name__ == "__main__":
    import sys

    plantri = sys.argv[1] if len(sys.argv) > 1 else "plantri"
    for c in range(3, 11):
        shadows = knot_shadows(c, plantri)
        print(f"c={c}: {len(shadows)} knot shadows")
