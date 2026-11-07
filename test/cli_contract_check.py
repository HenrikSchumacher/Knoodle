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
        (IDENTIFY, ["--projection=Random", "--projection=X", "--reapr-energy=tv_mcf"]),
        (SIMPLIFY, ["--projection=RANDOM", "--format=PDC", "--reapr-energy=TV-CLP",
                    "--dijkstra-strategy=BiDirectional",
                    "--compaction-method=AREA_LENGTH_CLP"]),
        (DRAW, ["--projection=Y", "--format=WL", "--quality=DEBUG", "--bend-method=MCF",
                "--compaction=LENGTH_MCF", "--reapr-energy=TV_CLP",
                "--pass-view=Before"]),
    ]

    value_rejected = [
        (IDENTIFY, ["--projection=w", "--projection=rand", "--reapr-energy=bogus"]),
        (SIMPLIFY, ["--projection=sideways", "--format=tsv", "--dijkstra-strategy=sideways"]),
        (DRAW, ["--projection=q", "--format=bogus", "--quality=turbo", "--pass-view=sideways"]),
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


def test_3d_input_is_quiet():
    section("3D input")

    # A clean 3D polygon must give a diagram and say nothing on stderr. v1.1.0
    # printed "RequireIntersections: Failed to compute intersections because no
    # vertex coordinates have been loaded, yet" three times on EVERY 3D input:
    # PlanarDiagram::FromCoordinates called the embedding's accessors after
    # DeleteTree(), which marks the coordinates unloaded. The answer was right;
    # the warnings read as a failure. (Also the input that aborted gcc builds of
    # v1.1.0 -- see wideint_check.)
    import math
    lines = []
    for i in range(24):
        a = 2.0 * math.pi * i / 24.0
        lines.append(f"{math.sin(a) + 2.0 * math.sin(2.0 * a):.10g} "
                     f"{math.cos(a) - 2.0 * math.cos(2.0 * a):.10g} "
                     f"{-math.sin(3.0 * a):.10g}")
    trefoil_3d = "\n".join(lines) + "\n"

    rc, out, err = run([SIMPLIFY, "--streaming-mode"], trefoil_3d)
    check(rc == 0, "3D trefoil: exits 0")
    check(len(data_lines(out)) == 3,
          f"3D trefoil: simplifies to 3 crossings; got {len(data_lines(out))}")
    noise = [l for l in err.splitlines() if "WARNING" in l or "ERROR" in l]
    check(noise == [], f"3D trefoil: nothing on stderr; got {noise[:2]}")


# A Hopf link as a .kndlxyz: two squares, a blank line between them -- once on
# the integer lattice, once off it (the floating-point path).
HOPF_XYZ = {
    "lattice": "0 0 0\n2 0 0\n2 2 0\n0 2 0\n\n1 1 -1\n3 1 -1\n3 1 1\n1 1 1\n",
    "float":   ("0.05 0.01 0.02\n2.03 0.02 -0.01\n2.01 2.04 0.03\n0.02 1.97 0.01\n\n"
                "1.01 1.02 -1.03\n3.02 0.98 -1.01\n2.99 1.03 1.02\n1.03 0.99 0.97\n"),
}


def colors_of(rows):
    return {r[5] for r in rows if len(r) == 7} | {r[6] for r in rows if len(r) == 7}


def test_3d_routes_agree():
    section("3D input: every route reads the same link")

    # Until 2026-11 ReadKnot skipped blank lines, so a .kndlxyz on stdin -- or
    # given to knoodleidentify at all, which has no extension routing -- became
    # ONE closed curve through both squares: the Hopf link came out as the
    # unknot (`s`, `<||>`), while `knoodlesimplify FILE.kndlxyz` was right.
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        for kind, text in HOPF_XYZ.items():
            outs = {}
            # stdin, streaming
            rc, out, err = run([SIMPLIFY, "--streaming-mode", "-s=0"], text)
            check(rc == 0, f"Hopf ({kind}) on stdin: exits 0 (rc={rc}): {err.strip()[:160]}")
            outs["stdin"] = data_lines(out)
            # a .kndlxyz file, and the same text as a .tsv file
            for ext in ("kndlxyz", "tsv"):
                src = os.path.join(tmp, f"hopf_{kind}.{ext}")
                dst = os.path.join(tmp, f"hopf_{kind}_{ext}.out")
                with open(src, "w") as f:
                    f.write(text)
                rc, _, err = run([SIMPLIFY, "-q", "-s=0", f"--output={dst}", src])
                check(rc == 0, f"Hopf ({kind}) as .{ext}: exits 0 (rc={rc}): {err.strip()[:160]}")
                outs[ext] = data_lines(open(dst).read()) if os.path.exists(dst) else []

            for route, rows in outs.items():
                check(len(rows) in (2, 3) and all(len(r) == 7 for r in rows)
                      and colors_of(rows) == {"0", "1"},
                      f"Hopf ({kind}) via {route}: a 2-colour link diagram, got {rows}")
            check(outs["stdin"] == outs["kndlxyz"] == outs["tsv"],
                  f"Hopf ({kind}): stdin, .kndlxyz and .tsv give the same diagram")

            rc, out, err = run([SIMPLIFY, "--streaming-mode"], text)
            rows = data_lines(out)
            check(rc == 0 and len(rows) == 2 and colors_of(rows) == {"0", "1"},
                  f"Hopf ({kind}) simplifies to the 2-crossing Hopf link, got {rows}")

            src = os.path.join(tmp, f"hopf_{kind}.kndlxyz")
            for how, cmd, stdin in (("file", [IDENTIFY, src], ""), ("stdin", [IDENTIFY], text)):
                rc, out, err = run(cmd, stdin)
                check(rc == 0 and out.strip().startswith("<| Link["),
                      f"knoodleidentify Hopf ({kind}) from {how}: a Link, got {out.strip()[:60]!r}")

            for how, cmd, stdin in (("file", [DRAW, src], ""), ("stdin", [DRAW], text)):
                rc, out, err = run(cmd, stdin)
                check(rc == 0 and out.strip() != "",
                      f"knoodledraw Hopf ({kind}) from {how}: draws (rc={rc}): {err.strip()[:160]}")

    # A trefoil beside a distant square is a split LINK, not a trefoil: the
    # square projects crossingless, so only the component count shows it.
    import math
    tref = []
    for i in range(24):
        a = 2.0 * math.pi * i / 24.0
        tref.append(f"{math.sin(a) + 2.0 * math.sin(2.0 * a):.10g} "
                    f"{math.cos(a) - 2.0 * math.cos(2.0 * a):.10g} "
                    f"{-math.sin(3.0 * a):.10g}")
    split = "\n".join(tref) + "\n\n20 0 0\n21 0 0\n21 1 0\n20 1 0\n"
    rc, out, err = run([IDENTIFY], split)
    check(rc == 0 and out.strip().startswith("<| Link["),
          f"knoodleidentify: a trefoil beside a square is a Link, got {out.strip()[:60]!r}")
    rc, out, err = run([DRAW], split)
    check(rc == 0 and out.strip() != "",
          f"knoodledraw draws a trefoil beside a square (rc={rc}): {err.strip()[:160]}")

    # A repeated vertex off the lattice: a zero-length edge, which aborted in
    # LinesColinearTest (exit 134) before the float path dropped repeats too.
    stacked = ("0.5 0.5 0.5\n4.5 0.5 0.5\n4.5 0.5 0.5\n4.5 4.5 0.5\n"
               "4.5 4.5 5.5\n4.5 0.5 5.5\n4.5 0.5 5.5\n0.5 0.5 5.5\n")
    for name, cmd in (("knoodlesimplify", [SIMPLIFY, "--streaming-mode"]),
                      ("knoodleidentify", [IDENTIFY]),
                      ("knoodledraw", [DRAW])):
        rc, out, err = run(cmd, stacked)
        check(rc == 0, f"{name}: a float polygon with repeated vertices exits 0 (rc={rc}):"
                       f" {err.strip()[:160]}")


def test_input_flags():
    section("--projection, --seed, --exact-input, -q, and identify's Reapr flags")

    import math
    import tempfile

    lattice = HOPF_XYZ["lattice"]
    decimal = HOPF_XYZ["float"]                # 0.01 and friends: they round
    dyadic = lattice.replace("0 0 0\n", "0.5 0.25 0\n", 1)
    tref = "".join(f"{math.sin(a) + 2 * math.sin(2 * a):.10g} "
                   f"{math.cos(a) - 2 * math.cos(2 * a):.10g} {-math.sin(3 * a):.10g}\n"
                   for a in (2 * math.pi * i / 24 for i in range(24)))

    simplify = [SIMPLIFY, "--streaming-mode", "-s=0"]

    # --randomize-projection is --projection=random by another name: under one
    # seed they draw the same rotation and give the same diagram.
    for kind, text in (("lattice", lattice), ("float", tref)):
        _, a, _ = run(simplify + ["--projection=random", "--seed=17"], text)
        _, b, _ = run(simplify + ["--randomize-projection", "--seed=17"], text)
        check(data_lines(a) != [] and a == b,
              f"{kind}: --randomize-projection is --projection=random (same seed, same diagram)")

    # --seed repeats every tool's random projection.
    for name, cmd in (("knoodlesimplify", simplify),
                      ("knoodledraw", [DRAW, "--mono"]),
                      ("knoodleidentify", [IDENTIFY])):
        for kind, text in (("lattice", lattice), ("float", tref)):
            outs = [run(cmd + ["--projection=random", "--seed=99"], text) for _ in range(2)]
            check(all(o[0] == 0 for o in outs) and outs[0][1] == outs[1][1] and outs[0][1].strip(),
                  f"{name} ({kind}): --projection=random --seed=99 repeats exactly")

    # Different seeds draw different rotations (a 1635-crossing curve would
    # make this certain; 30 seeds of the trefoil make it near enough).
    seen = {run(simplify + ["--projection=random", f"--seed={k}"], tref)[1] for k in range(30)}
    check(len(seen) > 1, f"knoodlesimplify: 30 seeds give {len(seen)} different diagrams, not 1")

    # Reapr (-s=6) repeats too, once OrthoDraw's own randomization -- which
    # seeds itself, out of --seed's reach -- is turned off.
    fixed = [SIMPLIFY, "--streaming-mode", "--projection=random", "--seed=5",
             "--randomize-bends=0", "--no-randomize-virtual-edges"]
    outs = [run(fixed, tref) for _ in range(2)]
    check(all(o[0] == 0 for o in outs) and outs[0][1] == outs[1][1],
          "knoodlesimplify -s=6: --seed with OrthoDraw's randomization off repeats exactly")

    for tool in (SIMPLIFY, DRAW, IDENTIFY):
        name = os.path.basename(tool)
        for bad in ("--seed=-1", "--seed=abc", "--seed=18446744073709551616", "--seed="):
            rc, _, _ = run([tool, bad, "--help"])
            check(rc != 0, f"{name} rejects {bad}")
        rc, _, _ = run([tool, "--seed=18446744073709551615", "--help"])
        check(rc == 0, f"{name} accepts --seed=2^64-1")

    # A failing run's report names its seed, so it can be repeated.
    with tempfile.TemporaryDirectory() as tmp:
        p = subprocess.run([SIMPLIFY, "--streaming-mode", "--seed=4242"],
                           input="1 2 3 9\n", capture_output=True, text=True,
                           env=dict(os.environ, KNOODLE_DUMP_DIR=tmp))
        report = os.path.join(tmp, "knoodlesimplify-error-report.txt")
        text = open(report).read() if os.path.exists(report) else ""
        check(p.returncode != 0 and "--seed=4242" in text,
              f"the error report names the run's seed (rc={p.returncode})")

    # --exact-input: integer coordinates always pass, rotated or not; dyadic
    # ones pass down an axis; decimals that round, and any non-integer
    # coordinates under --projection=random, are refused -- with no output.
    # (knoodlesimplify reads the pipe without --streaming-mode, which would
    # send the refusal to its log file rather than to stderr.)
    for name, cmd in (("knoodlesimplify", [SIMPLIFY, "-s=0"]), ("knoodledraw", [DRAW]),
                      ("knoodleidentify", [IDENTIFY])):
        for label, text, flags, ok in (
                ("lattice", lattice, [], True),
                ("lattice, random", lattice, ["--projection=random"], True),
                ("dyadic", dyadic, [], True),
                ("dyadic, down x", dyadic, ["--projection=x"], True),
                ("dyadic, random", dyadic, ["--projection=random"], False),
                ("decimal", decimal, [], False)):
            rc, out, err = run(cmd + ["--exact-input"] + flags, text)
            if ok:
                check(rc == 0 and out.strip(), f"{name} --exact-input takes {label} input (rc={rc}):"
                                               f" {err.strip()[:160]}")
            else:
                check(rc != 0 and "--exact-input" in err and "KnotSymbol" not in out
                      and data_lines(out) == [] and "Link[" not in out,
                      f"{name} --exact-input refuses {label} input, writing nothing (rc={rc})")
        rc, _, _ = run(cmd, decimal)
        check(rc == 0, f"{name} still takes the decimal input without --exact-input")

    # Far from the origin for its spread: the floating-point grid overflows
    # (GH #50). --exact-input names the problem instead of projecting garbage.
    far = "".join(f"{1000.5 + float(x)} {y} {z}\n" if x else "\n"
                  for x, y, z in (l.split() if l else (None, None, None)
                                  for l in tref.splitlines()))
    rc, out, err = run([IDENTIFY, "--exact-input"], far)
    check(rc != 0 and "too far from the origin" in err,
          f"knoodleidentify --exact-input refuses an off-origin float knot by name (rc={rc})")

    # -q: knoodledraw has warnings to silence; knoodleidentify's stays as it was.
    rc, _, err = run([DRAW, "--highlight=z1"], "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n")
    check(rc == 0 and "Warning" in err, "knoodledraw warns about an unknown highlight type")
    for q in ("-q", "--quiet"):
        rc, out, err = run([DRAW, q, "--highlight=z1"], "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n")
        check(rc == 0 and out.strip() and err.strip() == "", f"knoodledraw {q} silences it")
    rc, out, _ = run([DRAW, "-cq"], "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n")
    check(rc == 0 and out.strip(), "knoodledraw combines -q with the label letters (-cq)")
    rc, out, err = run([IDENTIFY, "-q"], tref)
    check(rc == 0 and "KnotSymbol[3,1" in out and err.strip() == "",
          f"knoodleidentify -q identifies quietly (rc={rc}): {err.strip()[:120]}")

    # knoodleidentify takes knoodlesimplify's spelling of the Reapr flags.
    for flags in (["--reapr-rotation-trials=3"], ["--rotation-trials=3"],
                  ["--reapr-energy=TV"], ["--reapr-scaling=1.5"]):
        rc, out, err = run([IDENTIFY] + flags, tref)
        check(rc == 0 and "KnotSymbol[3,1" in out,
              f"knoodleidentify {' '.join(flags)} identifies the trefoil (rc={rc}): {err.strip()[:120]}")
    for bad in ("--reapr-rotation-trials=0", "--reapr-scaling=-1", "--reapr-scaling=x",
                "--escalation-rounds=0", "--escalation-band=x"):
        rc, out, _ = run([IDENTIFY, bad], tref)
        check(rc != 0 and "KnotSymbol" not in out,
              f"knoodleidentify {bad} fails (it used to print the help and exit 0)")


def main():
    print("cli_contract_check -- tool output, exit codes and round-tripping")

    if not os.path.exists(SIMPLIFY):
        print(f"error: {SIMPLIFY} is not built. Run `make all` in ../tools.")
        return 2

    test_output_is_readable_again()
    test_column_counts()
    test_failure_looks_like_failure()
    test_3d_input_is_quiet()
    test_3d_routes_agree()
    test_input_flags()
    test_help()
    test_flag_spellings()
    test_draw_is_a_filter()
    test_every_test_runs()

    print(f"\n{'CLI CONTRACT CHECK OK' if fails == 0 else 'CLI CONTRACT CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
