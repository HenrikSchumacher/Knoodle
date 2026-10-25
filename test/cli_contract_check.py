#!/usr/bin/env python3
"""cli_contract_check -- clause 3 of the tool contract: write output correctly.

The tool contract has three clauses: parse input correctly (knoodle_io_check),
be a faithful CLI for the library call (simplify_config_check), and write output
correctly. This is the third, and it is deliberately observed FROM OUTSIDE THE
PROCESS -- stdout, stderr and exit codes as a shell sees them.

That is the gap it fills. error_tap_check already covers the fail-loud
machinery, but in-process: it exercises CerrErrorTap and AtomicOutFile as
objects. Nobody checked that the assembled tool actually behaves that way when
you run it. klut_e2e already drives both binaries end to end, but it asks a
semantic question -- does the pipeline name the right knot -- not a contract
question.

Nothing here computes a knot type. The questions are only: is the output
re-readable, does it have the shape it claims, and does failure look like
failure.

Run: python3 cli_contract_check.py   (needs the tools built in ../tools)
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(HERE, "..", "tools")
SIMPLIFY = os.path.join(TOOLS, "knoodlesimplify")
DRAW = os.path.join(TOOLS, "knoodledraw")
IDENTIFY = os.path.join(TOOLS, "knoodleidentify")

# (binary, parser source, flags deliberately kept out of --help).
#
# The contract: --help documents ONE preferred spelling of every flag the
# parser accepts, and the parser accepts every case/separator variant of that
# spelling. The flag list is read from the parser itself, so adding a flag
# without documenting it fails here rather than going unnoticed -- which is how
# --reapr-rotation-trials came to be mis-remembered as --rotation-trials while
# writing simplify_config_check, and how --format went undocumented in
# knoodledraw's --help.
FLAG_SOURCES = [
    (SIMPLIFY, "knoodlesimplify_config.hpp", {"--debug-print-simplify-args"}),
    (DRAW,     "knoodledraw.cpp",            set()),
    (IDENTIFY, "knoodleidentify.cpp",        set()),
]

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


# A trefoil and a 2-component link, as 5-column signed PD codes.
TREFOIL = "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n"
HOPF = "0 2 1 3 1\n2 0 3 1 1\n"


def data_lines(out):
    """The numeric rows of knoodlesimplify output, ignoring k/s/u markers."""
    rows = []
    for line in out.splitlines():
        line = line.strip()
        if not line or line[0] in "ksuKSU":
            continue
        rows.append(line.split())
    return rows


# ---------------------------------------------------------------------------


def test_output_is_readable_again():
    section("output re-reads as input")

    # The round trip that matters: whatever the tool writes, it must be willing
    # to read. A writer whose own reader rejects its output is the bug this
    # guards -- and it is not hypothetical, it is exactly what happened to
    # LinkEmbedding's colored .kndlxyz writer (see link_color_roundtrip).
    for name, text in (("trefoil", TREFOIL), ("hopf link", HOPF)):
        rc, out, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], text)
        check(rc == 0, f"{name}: -s=0 exits 0")
        check(out.strip() != "", f"{name}: -s=0 produces output")

        rc2, out2, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], out)
        check(rc2 == 0, f"{name}: its own output is accepted as input")
        check(data_lines(out2) == data_lines(out),
              f"{name}: a second pass at -s=0 is a fixed point")


def test_column_counts():
    section("column counts")

    # THE RULE IS NOT "knots are 5 and links are 7". It is knoodlesimplify.cpp:820,
    #
    #     colored_output = (input_column_count >= 6) || split_into_summands
    #
    # i.e. colors are written when they carry information: when the input
    # already had them, or when splitting means the reader needs to know which
    # summands were once one component. A 5-column LINK comes back as 5 columns,
    # because nothing was learned about its colors.
    #
    # Getting this wrong is not hypothetical -- reading 7-column output as
    # 5-column produced three wrong commits on 2026-08-14 -- and the convenient
    # shorthand "5 for knots, 7 for links" is what made it easy to get wrong.
    # So each branch of the real condition is asserted separately.

    colored_trefoil = "".join(r.rstrip() + " 0 0\n" for r in TREFOIL.splitlines())
    connect_sum = "s\n" + TREFOIL + "s\n" + TREFOIL

    cases = [
        ("5-column knot, no split", TREFOIL, 5,
         "colors carry no information, so none are written"),
        ("5-column LINK, no split", HOPF, 5,
         "a link is NOT automatically 7 columns"),
        ("7-column input", colored_trefoil, 7,
         "input colors are preserved"),
        ("input that splits into summands", connect_sum, 7,
         "colors record which summands shared a component"),
    ]

    for what, text, want, why in cases:
        rc, out, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], text)
        rows = data_lines(out)
        check(rc == 0 and rows != [], f"{what}: produces rows")
        if not rows:
            continue
        widths = sorted({len(r) for r in rows})
        check(widths == [want], f"{what} -> {want} columns ({why}); got {widths}")
        check(len(widths) == 1,
              f"{what}: all rows of one diagram have the same width")


def test_failure_looks_like_failure():
    section("failure is loud")

    # The fail-loud contract, observed from outside: a library error means a
    # nonzero exit AND no output on stdout. error_tap_check pins the mechanism;
    # this pins the behaviour.
    bad_inputs = [
        ("a 2-column line", "1 2\n"),
        ("a 9-column line", "1 2 3 4 5 6 7 8 9\n"),
        ("a non-numeric token", "2 0 3 5 1\nnot a number\n"),
        ("mixed column counts", "2 0 3 5 1\n1 2 3 4\n"),
        ("3D mixed with a PD code", "0 0 0\n1 0 0\n2 0 3 5 1\n"),
    ]
    for what, text in bad_inputs:
        rc, out, err = run([SIMPLIFY, "--streaming-mode"], text)
        check(rc != 0, f"{what}: exits nonzero")
        check(out.strip() == "", f"{what}: writes nothing to stdout")
        check(err.strip() != "", f"{what}: says why on stderr")

    # A rejected flag must not be treated as a filename or ignored.
    for flag in ("--local-opt-level=7", "--dijkstra-strategy=sideways",
                 "--reapr-energy=nonsense", "--no-such-flag"):
        rc, out, err = run([SIMPLIFY, "--streaming-mode", flag], TREFOIL)
        check(rc != 0, f"{flag}: exits nonzero")
        check(out.strip() == "", f"{flag}: writes nothing to stdout")


def parser_flags(path):
    """Long flags the parser's own comparisons accept."""
    flags = set()
    with open(path) as f:
        for line in f:
            # knoodledraw spells its boolean pairs as MatchBoolFlag(arg, "name"),
            # which stands for both --name and --no-name.
            if "MatchBoolFlag(arg," in line:
                pieces = line.split('"')
                if len(pieces) >= 2:
                    flags.add("--" + pieces[1])
                    flags.add("--no-" + pieces[1])
                continue
            if "arg ==" not in line and "starts_with(" not in line:
                continue
            for piece in line.split('"'):
                # "--" and "--no-" are prefix tests inside MatchBoolFlag itself,
                # not flags.
                if piece.startswith("--") and len(piece) > 2 and piece != "--no-":
                    flags.add(piece.rstrip("="))
    return flags


def table_flags(path):
    """Flags listed in the parser's kKnownFlags table."""
    flags = set()
    inside = False
    with open(path) as f:
        for line in f:
            if "kKnownFlags[] = {" in line:
                inside = True
                continue
            if not inside:
                continue
            if "}" in line and '"' not in line:
                break
            for piece in line.split('"'):
                if piece.startswith("--") and len(piece) > 2:
                    flags.add(piece.rstrip("="))
    return flags


def documented(flag, text):
    """Does --help describe `flag`?

    The preferred spelling has to appear verbatim -- that spelling IS the
    contract, and the parser's tolerance for case and separator variants is
    what spares users from having to match it exactly.

    The one concession is the --no- forms of boolean flags. A tool may either
    spell both out (knoodlesimplify's "--canonicalize / --no-canonicalize") or
    document the positive form and state the convention once in a section
    header (knoodledraw's seven pairs). Demanding all fourteen long negated
    spellings verbatim would bloat the help for no gain.
    """
    if flag in text:
        return True
    if flag.startswith("--no-"):
        return ("--" + flag[len("--no-"):]) in text and "--no-" in text
    return False


def test_help():
    section("--help")

    for tool, source, hidden in FLAG_SOURCES:
        name = os.path.basename(tool)
        if not os.path.exists(tool):
            print(f"  SKIP  {name} is not built")
            continue

        rc, out, err = run([tool, "--help"])
        text = out + err
        check(rc == 0, f"{name} --help exits 0")
        check("Usage:" in text, f"{name} --help prints a usage line")

        src = os.path.join(TOOLS, source)
        flags = parser_flags(src)
        table = table_flags(src)

        check(len(flags) > 8, f"{name}: found {len(flags)} flags in the parser")

        undocumented = sorted(f for f in flags - hidden if not documented(f, text))
        check(not undocumented,
              f"{name} --help documents every flag the parser accepts "
              f"(missing: {', '.join(undocumented) if undocumented else 'none'})")

        # The canonicalizing table is what makes variant spellings work, so a
        # flag missing from it silently loses that tolerance.
        missing = sorted(flags - table)
        extra = sorted(table - flags)
        check(not missing and not extra,
              f"{name}: kKnownFlags matches the parser "
              f"(missing: {', '.join(missing) or 'none'}; "
              f"stale: {', '.join(extra) or 'none'})")

        for h in hidden:
            check(h not in text, f"{name}: {h} stays out of --help")


def test_flag_spellings():
    section("flag spelling is forgiving")

    # A user who types --CheckerboardColoring or --checkerboard_coloring has
    # said unambiguously what they want, so the tools serve it rather than
    # complain. --help documents one preferred spelling; the parser accepts
    # every case/separator variant of it.
    #
    # Probing with a trailing --help keeps this free of data files and of KLUT
    # tables: the variant has to parse before --help ends the loop.
    accepted = [
        (SIMPLIFY, ["--Streaming-Mode", "--streamingmode", "--STREAMING_MODE"]),
        (DRAW, ["--CheckerboardColoring", "--checkerboard_coloring",
                "--CHECKERBOARD-COLORING"]),
        (IDENTIFY, ["--Randomize-Projection", "--randomizeprojection",
                    "--RANDOMIZE_PROJECTION"]),
    ]

    # Abbreviations and single-dash long flags stay rejected: every accepted
    # spelling has to map to exactly one documented flag, so that adding a flag
    # can never change how an existing spelling parses.
    rejected = [
        (SIMPLIFY, ["--streaming", "-streaming-mode"]),
        (DRAW, ["--checker", "-checkerboard-coloring"]),
        (IDENTIFY, ["--randomize", "-randomize-projection"]),
    ]

    for tool, spellings in accepted:
        name = os.path.basename(tool)
        if not os.path.exists(tool):
            print(f"  SKIP  {name} is not built")
            continue
        for spelling in spellings:
            rc, out, err = run([tool, spelling, "--help"])
            check(rc == 0 and "Unknown option" not in (out + err),
                  f"{name} accepts {spelling}")

    for tool, spellings in rejected:
        name = os.path.basename(tool)
        if not os.path.exists(tool):
            continue
        for spelling in spellings:
            rc, out, err = run([tool, spelling, "--help"])
            check(rc != 0, f"{name} still rejects {spelling}")

    # A flag whose value comes from a FIXED VOCABULARY takes the same tolerance
    # as the flag name: --format=WL is as clear as --format=wl. Open-ended
    # values -- paths, PD descriptors, numbers -- are matched verbatim instead,
    # since dropping '-' from a path or a negative number changes its meaning.
    value_accepted = [
        (SIMPLIFY, ["--format=PDC", "--reapr-energy=TV-CLP",
                    "--dijkstra-strategy=BiDirectional",
                    "--compaction-method=AREA_LENGTH_CLP"]),
        (DRAW, ["--format=WL", "--quality=DEBUG", "--bend-method=MCF",
                "--compaction=LENGTH_MCF", "--reapr-energy=TV_CLP",
                "--pass-view=Before"]),
    ]

    value_rejected = [
        (SIMPLIFY, ["--format=tsv", "--dijkstra-strategy=sideways"]),
        (DRAW, ["--format=bogus", "--quality=turbo", "--pass-view=sideways"]),
    ]

    for tool, spellings in value_accepted:
        name = os.path.basename(tool)
        if not os.path.exists(tool):
            print(f"  SKIP  {name} is not built")
            continue
        for spelling in spellings:
            rc, out, err = run([tool, spelling, "--help"])
            check(rc == 0, f"{name} accepts {spelling}")

    for tool, spellings in value_rejected:
        name = os.path.basename(tool)
        if not os.path.exists(tool):
            continue
        for spelling in spellings:
            rc, out, err = run([tool, spelling, "--help"])
            check(rc != 0, f"{name} still rejects {spelling}")


def test_every_test_runs():
    """A test function nobody calls is not a test.

    test_flag_spellings sat here fully written and never registered, so its
    checks read as green while never executing once -- the check count was the
    only tell. Cheap to prevent, expensive to notice.
    """
    section("every test is registered")

    import inspect

    module = sys.modules[__name__]
    defined = {name for name, _ in inspect.getmembers(module, inspect.isfunction)
               if name.startswith("test_")}
    body = inspect.getsource(main)
    uncalled = sorted(n for n in defined if f"{n}()" not in body)

    check(not uncalled,
          f"every test_* function is called by main() "
          f"(uncalled: {', '.join(uncalled) if uncalled else 'none'})")


def test_draw_is_a_filter():
    section("knoodledraw as a Unix filter")

    if not os.path.exists(DRAW):
        print("  SKIP  knoodledraw is not built")
        return

    # The documented pipeline: knoodlesimplify | knoodledraw.
    rc, simplified, _ = run([SIMPLIFY, "--streaming-mode", "-s=0"], TREFOIL)
    check(rc == 0, "knoodlesimplify produces input for knoodledraw")

    rc, out, err = run([DRAW], simplified)
    check(rc == 0, "knoodledraw accepts knoodlesimplify's output on stdin")
    check(out.strip() != "", "knoodledraw writes a drawing to stdout")

    # Garbage in must not be drawn as if it were a diagram.
    rc, out, err = run([DRAW], "1 2\n")
    check(rc != 0, "knoodledraw exits nonzero on unparseable input")
    check(out.strip() == "", "knoodledraw writes nothing to stdout on bad input")


def main():
    print("cli_contract_check -- tool output, exit codes and round-tripping")

    if not os.path.exists(SIMPLIFY):
        print(f"error: {SIMPLIFY} is not built. Run `make all` in ../tools.")
        return 2

    test_output_is_readable_again()
    test_column_counts()
    test_failure_looks_like_failure()
    test_help()
    test_flag_spellings()
    test_draw_is_a_filter()
    test_every_test_runs()

    print(f"\n{'CLI CONTRACT CHECK OK' if fails == 0 else 'CLI CONTRACT CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
