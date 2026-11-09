#!/usr/bin/env python3
"""
cli_stdin_check.py - regression test for the interactive-stdin notice.

The Knoodle CLI tools are Unix filters: given no input file, they read stdin.
A bare interactive invocation would otherwise sit silently looking hung, so each
filter prints a one-line notice to *stderr* when stdin is a terminal (isatty),
and stays silent when stdin is a pipe/redirect (so pipelines are unaffected).

For every tool below this verifies BOTH halves:
  (1) piped (empty) stdin -> the notice does NOT appear on stderr
  (2) pty stdin (EOF)     -> the notice DOES appear on stderr
and then that knoodlesimplify, given no files, reads a pipe but shows a
terminal its usage text, and that -q silences the notice.

stdlib only (subprocess, pty, os) -- no Regina / venv needed. Run directly:
    python3 cli_stdin_check.py
Exit status is 0 iff every case passes.
"""

import os
import pty
import subprocess
import sys
import tempfile
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent / "tools"

# The substring every tool prints in its interactive-stdin notice.
NOTICE = "reading diagrams from stdin"

# (label, argv-after-binary) for each tool expected to emit the notice on a tty.
# All three filters (knoodledraw, knoodlesimplify --streaming-mode, knoodleidentify)
# carry the same interactive-stdin notice.
TOOLS = [
    ("knoodledraw", []),
    ("knoodlesimplify", ["--streaming-mode"]),
    ("knoodleidentify", []),
]

TIMEOUT = 30  # seconds; a hang (no EOF handling) trips this and fails the case.


def run_piped(argv, cwd):
    """Run with empty *piped* stdin (a non-tty). Return captured stderr text."""
    p = subprocess.run(argv, input="", capture_output=True, text=True,
                       timeout=TIMEOUT, cwd=cwd)
    return p.stderr


def run_pty(argv, cwd):
    """Run with a *pty* as stdin, immediately EOF'd. Return captured stderr text."""
    master, slave = pty.openpty()
    proc = subprocess.Popen(argv, stdin=slave, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True, cwd=cwd)
    os.close(slave)    # the child holds its own dup of the slave tty
    os.close(master)   # closing the master makes the child's tty read hit EOF
    try:
        _, err = proc.communicate(timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        proc.kill()
        _, err = proc.communicate()
        raise
    return err


def main():
    failures = 0
    checked = 0
    with tempfile.TemporaryDirectory() as cwd:  # isolate any tool log files
        for label, extra in TOOLS:
            binary = TOOLS_DIR / label
            if not binary.exists():
                print(f"FAIL  {label}: binary not found ({binary}); build the tools first")
                failures += 1
                continue
            argv = [str(binary), *extra]

            # (1) piped stdin: notice must be ABSENT.
            try:
                err = run_piped(argv, cwd)
            except subprocess.TimeoutExpired:
                print(f"FAIL  {label} [piped]: timed out (did not exit on EOF)")
                failures += 1
            else:
                checked += 1
                if NOTICE in err:
                    print(f"FAIL  {label} [piped]: notice printed for non-tty stdin")
                    failures += 1
                else:
                    print(f"ok    {label} [piped]: silent for non-tty stdin")

            # (2) pty stdin: notice must be PRESENT.
            try:
                err = run_pty(argv, cwd)
            except subprocess.TimeoutExpired:
                print(f"FAIL  {label} [tty]: timed out (did not exit on EOF)")
                failures += 1
            else:
                checked += 1
                if NOTICE in err:
                    print(f"ok    {label} [tty]: notice printed for interactive stdin")
                else:
                    print(f"FAIL  {label} [tty]: no notice for interactive stdin")
                    failures += 1

        # (3) knoodlesimplify with no files reads a PIPE without being told
        # to: the same diagram on stdout as --streaming-mode writes, but with
        # its per-knot report on stderr rather than in a log file.
        simplify = TOOLS_DIR / "knoodlesimplify"
        if simplify.exists():
            trefoil = "2 0 3 5 1\n0 4 1 3 1\n4 2 5 1 1\n"
            bare = subprocess.run([str(simplify), "-s=0"], input=trefoil,
                                  capture_output=True, text=True, timeout=TIMEOUT, cwd=cwd)
            streamed = subprocess.run([str(simplify), "--streaming-mode", "-s=0"],
                                      input=trefoil, capture_output=True, text=True,
                                      timeout=TIMEOUT, cwd=cwd)
            checked += 1
            if bare.returncode == 0 and bare.stdout.strip() and bare.stdout == streamed.stdout:
                print("ok    knoodlesimplify [piped, no flags]: reads stdin, writes what "
                      "--streaming-mode writes")
            else:
                print(f"FAIL  knoodlesimplify [piped, no flags]: rc={bare.returncode}, "
                      f"stdout {bare.stdout!r} vs --streaming-mode {streamed.stdout!r}")
                failures += 1

            # (4) ... but a TERMINAL on stdin, with no files, gets the usage
            # text, as before: nobody meant to type diagrams at it.
            try:
                err = run_pty([str(simplify)], cwd)
            except subprocess.TimeoutExpired:
                print("FAIL  knoodlesimplify [tty, no flags]: timed out")
                failures += 1
            else:
                checked += 1
                if "Usage:" in err and NOTICE not in err:
                    print("ok    knoodlesimplify [tty, no flags]: prints the usage text")
                else:
                    print(f"FAIL  knoodlesimplify [tty, no flags]: no usage text: {err[:200]!r}")
                    failures += 1

        # (5) -q silences the notice in the two filters that take it.
        for label in ("knoodledraw", "knoodleidentify"):
            binary = TOOLS_DIR / label
            if not binary.exists():
                continue
            try:
                err = run_pty([str(binary), "-q"], cwd)
            except subprocess.TimeoutExpired:
                print(f"FAIL  {label} -q [tty]: timed out")
                failures += 1
                continue
            checked += 1
            if NOTICE in err:
                print(f"FAIL  {label} -q [tty]: notice printed despite -q")
                failures += 1
            else:
                print(f"ok    {label} -q [tty]: no notice")

    print()
    if failures:
        print(f"*** cli_stdin_check: {failures} case(s) FAILED ***")
        return 1
    print(f"PASS: interactive-stdin notice correct ({checked} cases).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
