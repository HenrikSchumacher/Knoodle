#!/usr/bin/env python3
"""run_tests_data_check -- run_tests.py fails on data files it cannot read.

run_tests.py reads data/diagrams, which is stored with Git LFS. On 2026-11-21 a
fresh worktree had 88 of the 1268 link-table files left as LFS pointer stubs.
read_tsv_file parsed each one to nothing, Tier 1c kept a file only if it had
crossings, and `make check-full` passed after testing 1180 of them. homfly_check
was no backstop: it reports a pointer as a skip, and run_tests counts a skip as
a pass. run_tests now reads every data file strictly and records one it cannot
read as a failure that names the file. A pointer's message says to run
`git lfs pull`, and so does a closing line that counts them.

This drives run_tests' own tier functions over temporary directories holding
good files, an LFS pointer and malformed files. The oracle calls are stubbed
out, so homfly_check and knoodlesimplify never run: it needs no binaries and
runs in CI. What is under test is how each tier handles its files, not the
oracle. The pointer is made the way a no-LFS checkout makes one: `git show
HEAD:<path>` prints the blob git stores, which is the pointer whether or not
this checkout has the real file. The last section reads the in-tree data with
the strict readers: each file must parse, or be refused as an LFS pointer.

Run: python3 run_tests_data_check.py
"""

from __future__ import annotations

import contextlib
import io
import shutil
import subprocess
import sys
import tempfile
import types
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(HERE))
import run_tests as rt  # noqa: E402

# The real data directories, kept before any test points them elsewhere.
REAL_LINKTABLE = rt.LINKTABLE_DIR
REAL_UNKNOT_DIRS = list(rt.UNKNOT_DIRS)
REAL_CHILDRENSGAME = rt.CHILDRENSGAME_DIR

checks = 0
fails = 0


def check(ok, what, detail=""):
    global checks, fails
    checks += 1
    if ok:
        return
    print(f"  FAILED  {what}")
    if detail:
        for line in str(detail).rstrip().splitlines()[:8]:
            print(f"          | {line}")
    fails += 1


def section(s):
    print(f"\n=== {s} ===")


def pd_text(rows):
    return "".join("\t".join(str(x) for x in r) + "\n" for r in rows)


TREFOIL_TSV = pd_text(rt.TREFOIL)
FIGURE_EIGHT_TSV = pd_text(rt.FIGURE_EIGHT)
EMBEDDING_TSV = "0\t0\t0\n1\t0\t0.5\n0\t1\t-0.25\n"
SHORT_ROW_TSV = "0\t4\t1\t3\t1\n2\t0\t3\n"
FOUR_COLUMN_TSV = "0\t4\t1\t3\n2\t0\t3\t5\n"


def lfs_pointer_blob():
    """The blob git stores for a link-table file: its LFS pointer."""
    return subprocess.run(
        ["git", "-C", str(REPO), "show", "HEAD:data/diagrams/linktable/L2a1_0.tsv"],
        capture_output=True, check=True).stdout


def write(path: Path, content):
    if isinstance(content, bytes):
        path.write_bytes(content)
    else:
        path.write_text(content)
    return path


def quietly(fn, *args):
    """Call fn(*args) with stdout captured. Returns (value, output)."""
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        value = fn(*args)
    return value, buf.getvalue()


def raises_data_error(fn, path):
    try:
        fn(path)
    except rt.DataFileError as e:
        return e
    return None


def stub_verify_invariance(suite, args, items):
    """Stands in for homfly_check: every item a tier hands over passes."""
    for (name, _path, _regina_input) in items:
        suite.record(rt.TestResult(name=name, passed=True))


def fake_args():
    # Tier 1a's homfly_check self-test runs args.homfly_check; `true` passes it.
    return types.SimpleNamespace(cross_check=False, homfly_check=shutil.which("true"),
                                 simplify_level=6)


def failures(suite):
    return [r for r in suite.results if not r.passed]


# ---------------------------------------------------------------------------


def test_readers(d: Path, pointer: bytes):
    section("the strict readers")

    ptr = write(d / "L2a1_0.tsv", pointer)
    check(rt.is_lfs_pointer(ptr),
          "`git show HEAD:data/diagrams/linktable/L2a1_0.tsv` is an LFS pointer "
          "(else this test proves nothing)", pointer[:200])

    for reader in (rt.read_pd_file, rt.read_diagram_file, rt.check_embedding_file):
        e = raises_data_error(reader, ptr)
        what = f"{reader.__name__}(LFS pointer)"
        check(e is not None, f"{what}: raises DataFileError")
        if e is not None:
            check(e.lfs_pointer, f"{what}: flags it as an LFS pointer", e)
            check("L2a1_0.tsv" in str(e), f"{what}: names the file", e)
            check("git lfs pull" in str(e), f"{what}: says to run `git lfs pull`", e)

    good = write(d / "trefoil.tsv", "# a comment\n\n" + TREFOIL_TSV)
    check(rt.read_pd_file(good) == rt.TREFOIL,
          "read_pd_file: reads a PD code, skipping comments and blank lines")
    check(rt.read_diagram_file(good) == ("pd", rt.TREFOIL),
          "read_diagram_file: a PD code comes back as ('pd', crossings)")

    emb = write(d / "triangle.tsv", EMBEDDING_TSV)
    check(rt.check_embedding_file(emb) == 3, "check_embedding_file: counts 3 vertices")
    check(rt.read_diagram_file(emb) == ("embedding", EMBEDDING_TSV),
          "read_diagram_file: an embedding comes back as ('embedding', text)")

    bad_cases = [
        ("short_row.tsv", SHORT_ROW_TSV, rt.read_pd_file, "line 2"),
        ("not_int.tsv", "0\t4\t1\t3\tx\n", rt.read_pd_file, "line 1"),
        ("empty.tsv", "", rt.read_pd_file, "no PD code rows"),
        ("comments_only.tsv", "# nothing\n\n", rt.read_pd_file, "no PD code rows"),
        ("pd_as_embedding.tsv", TREFOIL_TSV, rt.check_embedding_file, "line 1"),
        ("empty_embedding.tsv", "", rt.check_embedding_file, "no vertices"),
        ("four_columns.tsv", FOUR_COLUMN_TSV, rt.read_diagram_file, "neither"),
        ("binary.tsv", b"\xff\xfe\x00\x81\n", rt.read_diagram_file, "not a text file"),
    ]
    for name, content, reader, phrase in bad_cases:
        path = write(d / name, content)
        e = raises_data_error(reader, path)
        what = f"{reader.__name__}({name})"
        check(e is not None, f"{what}: raises DataFileError")
        if e is not None:
            check(not e.lfs_pointer, f"{what}: not flagged as an LFS pointer", e)
            check(name in str(e) and phrase in str(e),
                  f"{what}: names the file and says '{phrase}'", e)


def test_tier1c(d: Path, pointer: bytes):
    section("Tier 1c: an LFS pointer and a bad file among good ones")

    write(d / "K3a1_0.tsv", TREFOIL_TSV)
    write(d / "K4a1_0.tsv", FIGURE_EIGHT_TSV)
    write(d / "L2a1_0.tsv", pointer)
    write(d / "L9z9_0.tsv", SHORT_ROW_TSV)
    rt.LINKTABLE_DIR = d

    suite = rt.TestSuite()
    _, out = quietly(rt.run_tier1c, suite, fake_args())
    check(len(suite.results) == 4, "one result per file, 4 in all",
          [r.name for r in suite.results])
    check(sorted(r.name for r in failures(suite)) == ["L2a1_0", "L9z9_0"],
          "the pointer and the bad file fail; the good files pass",
          [(r.name, r.passed) for r in suite.results])
    check("Tier 1c: Link Table: 2/4 passed" in out,
          "the section line counts all 4 files", out)
    check("L2a1_0.tsv: a Git LFS pointer" in out,
          "the FAIL line names the pointer file", out)
    check("L9z9_0.tsv: line 2 is not a PD code row" in out,
          "the FAIL line names the bad file and its line", out)

    ok, out = quietly(suite.summary)
    check(ok is False, "the run fails")
    check("1 data file(s) are Git LFS pointer files" in out
          and "git lfs pull" in out and "git lfs checkout" in out,
          "the summary counts the pointer and says `git lfs pull` / `git lfs checkout`",
          out)

    # A run with no pointers does not print the LFS line.
    clean = rt.TestSuite()
    quietly(clean.record, rt.TestResult(name="x", passed=False, error="HOMFLY changed"))
    _, out = quietly(clean.summary)
    check("LFS" not in out, "no LFS line when no file was a pointer", out)


def test_tier1d(d: Path, pointer: bytes):
    section("Tier 1d: an LFS pointer among embeddings")

    write(d / "gukov-01.tsv", EMBEDDING_TSV)
    write(d / "gukov-02.tsv", pointer)
    write(d / "gukov-03.tsv", TREFOIL_TSV)  # a PD code where an embedding belongs
    rt.CHILDRENSGAME_DIR = d

    suite = rt.TestSuite()
    _, out = quietly(rt.run_tier1d, suite, fake_args())
    check(len(suite.results) == 3, "one result per file, 3 in all",
          [r.name for r in suite.results])
    check(sorted(r.name for r in failures(suite)) == ["gukov-02", "gukov-03"],
          "the pointer and the PD file fail; the embedding passes",
          [(r.name, r.passed, r.error) for r in suite.results])
    check(sum(r.lfs_pointer for r in suite.results) == 1,
          "exactly one result is flagged as an LFS pointer")


def test_tier1b(d: Path, pointer: bytes):
    section("Tier 1b: bad files never reach knoodlesimplify")

    write(d / "pd.tsv", TREFOIL_TSV)
    write(d / "embedding.tsv", EMBEDDING_TSV)
    write(d / "pointer.tsv", pointer)
    write(d / "four_columns.tsv", FOUR_COLUMN_TSV)
    rt.UNKNOT_DIRS = [("fake", d)]

    calls = []
    real_pd, real_raw = rt.run_knoodlesimplify, rt.run_knoodlesimplify_raw
    # Both stubs report 0 crossings, i.e. "simplified to the unknot".
    rt.run_knoodlesimplify = lambda diagrams, *a, **k: calls.append("pd") or [[]]
    rt.run_knoodlesimplify_raw = lambda raw, *a, **k: calls.append("raw") or []
    try:
        suite = rt.TestSuite()
        _, out = quietly(rt.run_tier1b, suite, Path("/nonexistent/knoodlesimplify"), 6)
    finally:
        rt.run_knoodlesimplify, rt.run_knoodlesimplify_raw = real_pd, real_raw

    check(len(suite.results) == 4, "one result per file, 4 in all",
          [r.name for r in suite.results])
    check(sorted(r.name for r in failures(suite)) == ["fake/four_columns", "fake/pointer"],
          "the pointer and the 4-column file fail; PD and embedding pass",
          [(r.name, r.passed, r.error) for r in suite.results])
    check(sorted(calls) == ["pd", "raw"],
          "knoodlesimplify ran once for the PD file and once for the embedding, "
          "never for a bad file (an unparseable file must not 'simplify to 0')",
          calls)
    bad = {r.name: r for r in failures(suite)}
    if "fake/pointer" in bad:
        check(bad["fake/pointer"].lfs_pointer and "git lfs pull" in bad["fake/pointer"].error,
              "the pointer's error is the LFS message", bad["fake/pointer"].error)


def test_tier1a(d: Path, pointer: bytes):
    section("Tier 1a: the Hopf link file is a pointer")

    write(d / "L2a1_0.tsv", pointer)
    rt.LINKTABLE_DIR = d

    suite = rt.TestSuite()
    _, out = quietly(rt.run_tier1a, suite, fake_args())
    hopf = [r for r in suite.results if r.name.startswith("Hopf link")]
    check(len(hopf) == 1 and not hopf[0].passed and hopf[0].lfs_pointer,
          "the Hopf link fails as an LFS pointer instead of reaching homfly_check",
          [(r.name, r.passed, r.error) for r in suite.results])
    check(len(failures(suite)) == 1, "nothing else in Tier 1a fails",
          [(r.name, r.error) for r in failures(suite)])


def test_count_check():
    section("end_section: one result per data file")

    suite = rt.TestSuite()
    _, out = quietly(suite.section, "Tier X")
    for name in ("a", "b"):
        suite.record(rt.TestResult(name=name, passed=True))
    _, out = quietly(suite.end_section, 3)
    bad = failures(suite)
    check(len(bad) == 1 and "one result per data file" in bad[0].name
          and "2 results for 3 data files" in bad[0].error,
          "2 results for 3 files is recorded as a failure",
          [(r.name, r.error) for r in bad] or out)
    check("Tier X: 2/3 passed" in out, "and the section line counts it", out)

    suite = rt.TestSuite()
    quietly(suite.section, "Tier Y")
    suite.record(rt.TestResult(name="a", passed=True))
    quietly(suite.end_section, 1)
    check(not failures(suite), "a matching count records nothing extra")


def test_in_tree():
    section("the in-tree data: each file parses or is refused as an LFS pointer")

    # Each tier's directories, with the reader that tier uses on them.
    plan = [(REAL_LINKTABLE, rt.read_pd_file)]
    plan += [(p, rt.read_diagram_file) for (_label, p) in REAL_UNKNOT_DIRS]
    plan += [(REAL_CHILDRENSGAME, rt.check_embedding_file)]

    n_files = n_pointers = 0
    for d, reader in plan:
        for f in sorted(d.glob("*.tsv")):
            n_files += 1
            try:
                reader(f)
            except rt.DataFileError as e:
                if e.lfs_pointer:
                    n_pointers += 1
                else:
                    check(False, f"{reader.__name__} reads {rt.display_path(f)}", e)
    check(n_files >= 2000,
          f"run_tests' data directories hold at least 2000 .tsv files; found {n_files}")
    kind = ("all real data" if n_pointers == 0 else
            "all LFS pointers (a checkout without LFS data)" if n_pointers == n_files else
            f"{n_pointers} LFS pointers -- run_tests' tiers 1a-1d will fail here "
            "until `git lfs pull`")
    print(f"  {n_files} files: {kind}")


def main():
    print("run_tests_data_check -- run_tests.py vs data files it cannot read")

    pointer = lfs_pointer_blob()
    rt.verify_invariance = stub_verify_invariance
    with tempfile.TemporaryDirectory(prefix="run-tests-data-") as scratch:
        root = Path(scratch)
        for name, test in (("readers", test_readers), ("tier1c", test_tier1c),
                           ("tier1d", test_tier1d), ("tier1b", test_tier1b),
                           ("tier1a", test_tier1a)):
            d = root / name
            d.mkdir()
            test(d, pointer)
    test_count_check()
    test_in_tree()

    print(f"\n{'RUN_TESTS DATA CHECK OK' if fails == 0 else 'RUN_TESTS DATA CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
