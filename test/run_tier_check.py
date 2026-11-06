#!/usr/bin/env python3
"""run_tier_check -- the tier runner's own report, against a fake suite.

WHY THIS EXISTS. On 2026-11-06 a red push reported embedding_check's failure
as three lines of KNOWN failures (XFAILs): the runner showed the last three
lines of stdout, the test ended with its xfail list, and the four real failures
were above the cut. Nothing was saved, so there was nothing to go back to.

This builds a small suite of fake tests in a temporary directory -- one for
each way a test can fail -- runs the real run_tier.py on it, and checks what a
reader is shown:

  fail_check     prints a FAIL (and an XPASS) early, then pages of PASS lines,
                 then an XFAIL summary at the very end -- the 2026-11-06 shape.
                 The report must show the FAIL and XPASS, and no XFAIL.
  crash_check    an assertion on stderr, then death by a signal. The report
                 must say SIGTERM, not -15, and show the stderr line.

                 SIGTERM and NOT abort(): on macOS every abort() files a
                 "Python quit unexpectedly" crash report, and this runs in every
                 `make check`. The first version used os.abort() and left one
                 crash report per push on 2026-11-06.
  quiet_check    exits 3 without ever printing FAIL: the tail is the fallback.
  nowork_check   exits 0 but its stdout misses the work pattern.
  missing_check  a kind=test whose binary does not exist.
  pass_check, env_check   pass; env_check reports whether it saw GIT_DIR.

It also checks the logs (full stdout AND stderr, per test), failed.txt, that a
fake manifest never stamps the real tree, that git's hook variables are
stripped before tests run, and that a stamp names the commit the run STARTED
at, never one made while it ran.

Run: python3 run_tier_check.py
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
RUN_TIER = os.path.join(HERE, "run_tier.py")
STAMPS = os.path.join(HERE, ".stamps")

checks = 0
fails = 0


def check(ok, what):
    global checks, fails
    checks += 1
    if not ok:
        print(f"  FAILED  {what}")
        fails += 1


def section(s):
    print(f"\n=== {s} ===")


FAKES = {
    "pass_check.py": 'print("checked 5 things")\n',

    "fail_check.py": (
        'import sys\n'
        'print("fail_check")\n'
        'for i in range(30): print(f"  PASS  case {i}")\n'
        'print("    FAIL  case 31")\n'
        'print("          first difference at position 113")\n'
        'print("    XPASS case 40: marked xfail but it passed")\n'
        'for i in range(30): print(f"  PASS  case {41+i}")\n'
        'print("passed 60, failed 2, known-failing 3")\n'
        'print("known failures:")\n'
        'for i in range(3): print(f"  - XFAIL case x{i}: a known defect")\n'
        'sys.exit(1)\n'),

    "crash_check.py": (
        'import os, signal, sys\n'
        'print("crash_check starting")\n'
        'sys.stdout.flush()\n'
        'sys.stderr.write("Assertion failed: (n > 0), function Widget\\n")\n'
        'sys.stderr.flush()\n'
        '# SIGTERM, not abort(): see the docstring (macOS crash reports).\n'
        'os.kill(os.getpid(), signal.SIGTERM)\n'),

    "quiet_check.py": (
        'import sys\n'
        'for i in range(10): print(f"step {i}")\n'
        'sys.exit(3)\n'),

    "nowork_check.py": 'print("checked 0 things")\n',

    "env_check.py": (
        'import os\n'
        'print("GIT_DIR=" + str(os.environ.get("GIT_DIR")))\n'
        'print("checked 1 things")\n'),
}

WORK = r"checked [1-9][0-9]* things"

FULL_MANIFEST = [
    ("pass_check",    "script", "pass_check.py",    WORK),
    ("fail_check",    "script", "fail_check.py",    "-"),
    ("crash_check",   "script", "crash_check.py",   "-"),
    ("quiet_check",   "script", "quiet_check.py",   "-"),
    ("nowork_check",  "script", "nowork_check.py",  WORK),
    ("missing_check", "test",   "-",                "-"),
    ("env_check",     "script", "env_check.py",     WORK),
]


def write_suite(d, rows):
    for name, body in FAKES.items():
        with open(os.path.join(d, name), "w") as f:
            f.write(body)
    path = os.path.join(d, "manifest.tsv")
    with open(path, "w") as f:
        f.write("# fake manifest for run_tier_check\n")
        for target, kind, source, work in rows:
            f.write("\t".join([target, kind, "-", source, "-", "-", work]) + "\n")
    return path


def run(manifest, log_dir, extra_env=None):
    env = dict(os.environ, **(extra_env or {}))
    p = subprocess.run([sys.executable, RUN_TIER, "--tier=light", "--jobs=3",
                        f"--manifest={manifest}", f"--log-dir={log_dir}"],
                       capture_output=True, text=True, env=env)
    return p.returncode, p.stdout + p.stderr


def failed_blocks(out):
    """The 'failed:' section, split into one block of lines per test."""
    blocks, cur, inside = {}, None, False
    for ln in out.splitlines():
        if ln == "failed:":
            inside = True
            continue
        if not inside:
            continue
        if not ln.strip():
            break
        if ln.startswith("  - "):
            cur = ln[4:].split(":", 1)[0]
            blocks[cur] = [ln]
        elif cur:
            blocks[cur].append(ln)
    return {k: "\n".join(v) for k, v in blocks.items()}


def stamps():
    return set(os.listdir(STAMPS)) if os.path.isdir(STAMPS) else set()


def test_red_suite():
    with tempfile.TemporaryDirectory() as d:
        manifest = write_suite(d, FULL_MANIFEST)
        logs = os.path.join(d, "logs")
        before = stamps()
        rc, out = run(manifest, logs, {"GIT_DIR": "/nonexistent/hook/gitdir"})
        blocks = failed_blocks(out)

        section("the report shows what failed, not the last lines")
        check(rc == 1, f"a red suite exits 1 (got {rc})")
        fb = blocks.get("fail_check", "")
        check("FAIL  case 31" in fb, "fail_check: the early FAIL line is shown")
        check("first difference at position 113" in fb,
              "fail_check: the FAIL line's indented detail is shown with it")
        check("XPASS case 40" in fb, "fail_check: an XPASS (stale marker) counts as a failure line")
        check("XFAIL" not in fb, "fail_check: no XFAIL (known-failure) line is shown")
        check("PASS  case" not in fb, "fail_check: passing lines are not shown")
        check("full output:" in fb and "fail_check.log" in fb,
              "fail_check: the report names the log")

        cb = blocks.get("crash_check", "")
        check("killed by SIGTERM" in cb, "crash_check: a crash is named by its signal")
        check("Assertion failed: (n > 0)" in cb, "crash_check: stderr is shown")

        qb = blocks.get("quiet_check", "")
        check("exit 3" in qb, "quiet_check: the exit code is shown")
        check("step 9" in qb and "no FAIL line" in qb,
              "quiet_check: with no FAIL line, the tail is shown and labelled")

        check("nowork_check: stdout never matched" in out, "nowork_check: reported as no work")
        check("not built: missing_check" in out, "missing_check: reported as not built")
        check("pass_check" not in blocks and "env_check" not in blocks,
              "passing tests are not in the failed section")

        section("logs and failed.txt")
        light = logs
        names = open(os.path.join(light, "failed.txt")).read().split()
        check(names == ["fail_check", "crash_check", "quiet_check", "nowork_check", "missing_check"],
              f"failed.txt lists every non-passing test, in manifest order (got {names})")
        flog = open(os.path.join(light, "fail_check.log")).read()
        check("case 70" in flog and "XFAIL case x2" in flog,
              "fail_check.log holds the WHOLE stdout")
        clog = open(os.path.join(light, "crash_check.log")).read()
        check("==== stderr ====" in clog and "Assertion failed" in clog.split("==== stderr ====")[1],
              "crash_check.log holds stderr, under its own heading")
        check("killed by SIGTERM" in clog, "the log header records how the test ended")
        check(os.path.exists(os.path.join(light, "pass_check.log")),
              "passing tests get a log too")

        section("hook environment and stamps")
        elog = open(os.path.join(light, "env_check.log")).read()
        check("GIT_DIR=None" in elog,
              "GIT_DIR from the hook is not passed on to tests")
        check(stamps() == before, "a fake manifest never stamps the real tree")


def test_green_suite_does_not_stamp():
    with tempfile.TemporaryDirectory() as d:
        manifest = write_suite(d, [FULL_MANIFEST[0], FULL_MANIFEST[-1]])
        logs = os.path.join(d, "logs")
        # A stale failed.txt and log from an earlier run must be cleared.
        os.makedirs(logs)
        with open(os.path.join(logs, "failed.txt"), "w") as f:
            f.write("stale_check\n")
        with open(os.path.join(logs, "stale_check.log"), "w") as f:
            f.write("old\n")
        before = stamps()
        rc, out = run(manifest, logs)

        section("a green run")
        check(rc == 0, f"a green suite exits 0 (got {rc})")
        check(open(os.path.join(logs, "failed.txt")).read() == "",
              "failed.txt is empty when green (and the stale one is gone)")
        check(not os.path.exists(os.path.join(logs, "stale_check.log")),
              "the previous run's logs are cleared")
        check("stamped" not in out and stamps() == before,
              "a green fake manifest does not stamp")


def test_stamp_names_the_commit_that_ran():
    """write_stamp against stubbed tree states, stamping into a temp dir.

    On 2026-11-06 a 12-minute heavy run stamped a commit made 8 minutes into
    it: HEAD was read only at the end. The first case is the positive control,
    so the refusals below cannot pass merely because nothing ever stamps.
    """
    import contextlib
    import io
    sys.path.insert(0, HERE)
    import run_tier

    A, B = "a" * 40, "b" * 40
    cases = [
        ("same commit, clean throughout -> stamped", (A, ""), (A, ""), True),
        ("HEAD moved during the run -> no stamp", (A, ""), (B, ""), False),
        ("tree dirty at the end -> no stamp", (A, ""), (A, " M x.cpp"), False),
        ("tree dirty at the start -> no stamp", (A, " M x.cpp"), (A, ""), False),
    ]
    section("a stamp names the commit the run started at")
    saved = run_tier.STAMPS, run_tier.tree_state
    try:
        for what, start, end, want in cases:
            with tempfile.TemporaryDirectory() as d:
                run_tier.STAMPS = d
                run_tier.tree_state = lambda end=end: end
                msg = io.StringIO()
                with contextlib.redirect_stdout(msg):
                    run_tier.write_stamp("light", start)
                got = os.listdir(d)
                ok = got == [f"light-{end[0]}"] if want else got == []
                check(ok, f"{what} (stamps: {got})")
                if what.startswith("HEAD moved"):
                    check("HEAD moved" in msg.getvalue(),
                          "a moved HEAD is reported as the reason for no stamp")
    finally:
        run_tier.STAMPS, run_tier.tree_state = saved


def main():
    print("run_tier_check -- the tier runner's report, against a fake suite")
    test_red_suite()
    test_green_suite_does_not_stamp()
    test_stamp_names_the_commit_that_ran()
    print(f"\n{'RUN_TIER CHECK OK' if fails == 0 else 'RUN_TIER CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
