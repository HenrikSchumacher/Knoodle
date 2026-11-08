#!/usr/bin/env python3
"""lattice_input_check -- integer-coordinate input is projected EXACTLY.

When every coordinate of a 3D input is an integer, the tools project it with
LinkEmbedding_Int on Prosector_Int<Int64> (tools/knoodle_io.hpp, LatticeEmb_T):
no scaling, no rounding, symbolic perturbation for every degenerate projection.
Every input route takes it: 3-column data from stdin or a file, of which a
.kndlxyz file is one instance (ReadKnot, all three tools).

Before that, lattice input went through the floating-point embeddings, which
refuse a lattice curve seen down an axis: every fixture below exited 1 unless
--randomize-projection happened to shear it into general position.

The fixtures are test/embeddings/*.crd (see its README); their .expect files
document the knot types relied on here. Where no type is documented, the oracle
is agreement: the exact projection, the exact projections down the other two
axes, and random exact rotations must all name the same knot.

Run: python3 lattice_input_check.py   (needs the tools built in ../tools)
"""

import glob
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(HERE, "..", "tools")
SIMPLIFY = os.path.join(TOOLS, "knoodlesimplify")
DRAW = os.path.join(TOOLS, "knoodledraw")
IDENTIFY = os.path.join(TOOLS, "knoodleidentify")
FIXTURES = os.path.join(HERE, "embeddings")


def klut_tables_present():
    """True if data/Klut holds the KLUT tables themselves.

    A checkout made without Git LFS (or where `git lfs pull` never ran) has
    LFS pointer files there instead. CI fetches the tables
    (scripts/ci-fetch-klut.sh); a local checkout may not have. knoodleidentify then names nothing: it
    still simplifies and canonicalizes, and prints the result as
    NotFound[n, {...}], but exits 1.
    """
    tables = glob.glob(os.path.join(HERE, "..", "data", "Klut", "*.tsv"))
    if not tables:
        return False
    for path in tables:
        with open(path, "rb") as f:
            if f.read(40).startswith(b"version https://git-lfs"):
                return False
    return True


KLUT = klut_tables_present()

# Without the tables, the NotFound output stands in for the table name. For the
# trefoil and figure-eight used here it is a stable fingerprint: their minimal
# diagrams are unique, and 90 of 90 readbacks of each agreed (2026-11-08).
TREFOIL_ID = "KnotSymbol[3,1," if KLUT else "NotFound[3,"


def identify_ok(rc):
    """knoodleidentify's exit code for a run that named its knot -- or, without
    the tables, fingerprinted it (exit 1, NotFound)."""
    return rc == 0 or (not KLUT and rc == 1)

# Knot types the fixtures' .expect files document.
UNKNOTS = ["lattice_04", "deg_collinear_overlap", "deg_corner_corner",
           "deg_stacked_points", "deg_stacked_verticals", "deg_triple_point",
           "deg_vertex_on_edge", "deg_vertical_edge", "deg_zero_length"]
TREFOILS = ["arc_trefoil"]
# Single-component lattice knots whose type is not documented.
UNDOCUMENTED = ["lattice_06", "lattice_08", "lattice_10", "lattice_12", "lattice_16"]

RANDOM_TRIALS = 3

checks = 0
fails = 0


def check(ok, what):
    global checks, fails
    checks += 1
    if ok:
        return
    print(f"  FAILED  {what}")
    fails += 1


def section(s):
    print(f"\n=== {s} ===")


def run(cmd, stdin_text=""):
    """Run a command, returning (exit_code, stdout, stderr) separately."""
    p = subprocess.run(cmd, input=stdin_text, capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def fixture(name):
    with open(os.path.join(FIXTURES, name + ".crd")) as f:
        return f.read()


def vertices(text):
    return [tuple(int(t) for t in line.split()) for line in text.splitlines() if line.strip()]


def data_lines(out):
    """The numeric rows of knoodlesimplify output, ignoring k/s/u markers."""
    return [line.split() for line in out.splitlines()
            if line.strip() and not re.fullmatch(r"[kKsSuU]( .*)?", line.strip())]


def summand_markers(out):
    return sum(1 for line in out.splitlines() if line.strip() in ("s", "S"))


def identify(text, *flags):
    return run([IDENTIFY, *flags], text)


def simplify_xyz(tmp, name, text, *flags):
    """knoodlesimplify on `text` written as a .kndlxyz file; (rc, output file text)."""
    src = os.path.join(tmp, name + ".kndlxyz")
    dst = os.path.join(tmp, name + ".out")
    with open(src, "w") as f:
        f.write(text)
    if os.path.exists(dst):
        os.remove(dst)
    rc, _, _ = run([SIMPLIFY, *flags, f"--output={dst}", src])
    out = open(dst).read() if os.path.exists(dst) else ""
    return rc, out


def test_three_column_input():
    section("3-column lattice input: exact, deterministic, the documented type")

    for name in UNKNOTS + TREFOILS + UNDOCUMENTED:
        text = fixture(name)

        rc, out1, err = run([SIMPLIFY, "--streaming-mode", "-s=0"], text)
        check(rc == 0, f"{name}: knoodlesimplify -s=0 exits 0 (rc={rc}): {err.strip()[:200]}")
        _, out2, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], text)
        check(out1 == out2, f"{name}: the exact projection is the same diagram every time")

        rc, out, err = identify(text)
        check(identify_ok(rc), f"{name}: knoodleidentify exits 0 (rc={rc}): {err.strip()[:200]}")
        if name in UNKNOTS:
            check(out.strip() == "<||>", f"{name}: identified as the unknot, got {out.strip()[:80]}")
        if name in TREFOILS:
            check(out.strip().startswith("<| " + TREFOIL_ID),
                  f"{name}: identified as a trefoil, got {out.strip()[:80]}")


def test_random_projection_agrees():
    section("--projection: the other axes and exact random rotations name the same knot")

    for name in UNKNOTS + TREFOILS + UNDOCUMENTED:
        text = fixture(name)
        _, exact, _ = identify(text)
        # Over the table, the output carries the reduced PD code itself, which
        # depends on the projection; only a table type is comparable.
        if "Unidentified" in exact:
            continue
        # Down the x- and y-axes: as degenerate as down z for these fixtures,
        # and as exact.
        for axis in ("x", "y"):
            rc, out, err = identify(text, f"--projection={axis}")
            check(identify_ok(rc) and out == exact,
                  f"{name}: --projection={axis} gives {out.strip()[:80]} (rc={rc}),"
                  f" down z gives {exact.strip()[:80]} {err.strip()[:120]}")
        # --randomize-projection is the old spelling of --projection=random.
        for t in range(RANDOM_TRIALS):
            flag = "--randomize-projection" if t % 2 else "--projection=random"
            rc, out, err = identify(text, flag)
            check(identify_ok(rc), f"{name}: randomized trial {t} exits 0 (rc={rc}): {err.strip()[:200]}")
            check(out == exact, f"{name}: randomized trial {t} gives {out.strip()[:80]},"
                                f" exact gives {exact.strip()[:80]}")


def test_spelling_and_repeats():
    section("Integers spelled as floats, and repeated vertices, change nothing")

    text = fixture("lattice_08")
    _, exact, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], text)
    check(len(data_lines(exact)) > 0, "lattice_08 projects to a diagram with crossings")

    # Detection is by value: "4.0" is a lattice coordinate.
    as_floats = "".join(" ".join(f"{c}.0" for c in v) + "\n" for v in vertices(text))
    rc, out, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], as_floats)
    check(rc == 0 and out == exact, "lattice_08 written as 'x.0' gives the identical diagram")

    # Every vertex twice: the same polygon with zero-length edges, which
    # Prosector_Int cannot take (deg_zero_length, upstream issue 11).
    doubled = "".join((" ".join(map(str, v)) + "\n") * 2 for v in vertices(text))
    rc, out, err = run([SIMPLIFY, "--streaming-mode", "-s=0"], doubled)
    check(rc == 0 and out == exact,
          f"lattice_08 with every vertex repeated gives the identical diagram (rc={rc}):"
          f" {err.strip()[:200]}")


def test_kndlxyz_links(tmp):
    section(".kndlxyz lattice links")

    # deg_hopf_flat: a Hopf link (documented); its diagram has 2 crossings.
    for flags in ([], ["--randomize-projection"]):
        rc, out = simplify_xyz(tmp, "deg_hopf_flat", fixture("deg_hopf_flat"), *flags)
        rows = data_lines(out)
        check(rc == 0, f"deg_hopf_flat {flags}: knoodlesimplify exits 0 (rc={rc})")
        check(len(rows) == 2 and all(len(r) == 7 for r in rows)
              and {r[5] for r in rows} | {r[6] for r in rows} == {"0", "1"},
              f"deg_hopf_flat {flags}: simplifies to a 2-crossing, 2-colour link: {rows}")

    # The raw crossing counts the .expect notes document: 19 + 19 and 88.
    for name, raw in (("split_lattice", 38), ("stacked_lattice", 88)):
        rc, out = simplify_xyz(tmp, name, fixture(name), "-s=0")
        check(rc == 0 and len(data_lines(out)) == raw,
              f"{name}: projects to {raw} crossings, got {len(data_lines(out))} (rc={rc})")
        for flags in ([], ["--randomize-projection"]):
            rc, out = simplify_xyz(tmp, name, fixture(name), *flags)
            check(rc == 0 and data_lines(out) == [] and summand_markers(out) == 2,
                  f"{name} {flags}: simplifies to a 2-component unlink (rc={rc}): {out[:80]!r}")

    rc, _ = simplify_xyz(tmp, "nearmiss_sphere", fixture("nearmiss_sphere"), "-s=0")
    check(rc == 0, f"nearmiss_sphere (48 components): knoodlesimplify -s=0 exits 0 (rc={rc})")

    # A split link is one diagram with two diagram components, which OrthoDraw
    # refuses; knoodledraw used to go on and segfault.
    path = os.path.join(tmp, "split_lattice.kndlxyz")
    with open(path, "w") as f:
        f.write(fixture("split_lattice"))
    rc, out, err = run([DRAW, path])
    check(rc == 0 and out.strip() != "",
          f"knoodledraw draws the split link split_lattice (rc={rc}): {err.strip()[:200]}")


def test_failure_looks_like_failure():
    section("A lattice curve that is not an embedding fails loudly")

    # Two edges of a planar bowtie cross at (1,1,0): no perturbation can help.
    bowtie = "0 0 0\n2 2 0\n2 0 0\n0 2 0\n"
    for tool, cmd in (("knoodlesimplify", [SIMPLIFY, "--streaming-mode"]),
                      ("knoodleidentify", [IDENTIFY])):
        rc, out, _ = run(cmd, bowtie)
        check(rc != 0, f"{tool} exits nonzero on a self-intersecting lattice curve")
        check(data_lines(out) == [] and "KnotSymbol" not in out,
              f"{tool} writes no diagram for a self-intersecting lattice curve: {out[:80]!r}")

    # 2^45-sized coordinates fit the exact projection, but not an exact random
    # rotation (which scales by about 2^16 against Prosector_Int's 2^58 headroom).
    big = "".join(" ".join(str(c << 45) for c in v) + "\n" for v in vertices(fixture("lattice_04")))
    rc, out, _ = identify(big)
    check(rc == 0 and out.strip() == "<||>", f"lattice_04 scaled by 2^45 is still the unknot (rc={rc})")
    rc, out, err = identify(big, "--randomize-projection")
    check(rc != 0 and "too large" in err,
          f"...but its random rotation refuses rather than overflows (rc={rc}): {err.strip()[:200]}")


def main():
    print("lattice_input_check -- integer-coordinate input through the exact projector")
    if not KLUT:
        print("note: data/Klut holds Git LFS pointers, not the KLUT tables (no `git lfs pull`);"
              " knoodleidentify's NotFound fingerprints stand in for knot names")

    for tool in (SIMPLIFY, DRAW, IDENTIFY):
        if not os.path.exists(tool):
            print(f"error: {tool} is not built. Run `make all` in ../tools.")
            return 2

    test_three_column_input()
    test_random_projection_agrees()
    test_spelling_and_repeats()
    with tempfile.TemporaryDirectory() as tmp:
        test_kndlxyz_links(tmp)
    test_failure_looks_like_failure()

    print(f"\n{'LATTICE INPUT CHECK OK' if fails == 0 else 'LATTICE INPUT CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
