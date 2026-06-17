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
# knoodledraw / knoodlesimplify (--streaming-mode) carry the same notice on main.
TOOLS = [
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

    print()
    if failures:
        print(f"*** cli_stdin_check: {failures} case(s) FAILED ***")
        return 1
    print(f"PASS: interactive-stdin notice correct ({checked} cases).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
