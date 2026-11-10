#!/usr/bin/env python3
"""orthodraw_regression: has OrthoDraw's default pipeline changed its drawings?

A debugging tool, run by hand. It is in no test tier and no hook: OrthoDraw is
tuned by hand, and a drawing that changes is not a failure. What this answers
is WHETHER the drawings changed, between which builds, and what they look like
now, e.g. when hunting for the commit that changed a layout.

`knoodledraw --ascii` is run over a fixed corpus (71 files of data/diagrams)
and each drawing is compared with the one kept in test/orthodraw_golden.txt,
which also says which build drew it.

    orthodraw_regression.py            compare; at a terminal, ask what to do
    orthodraw_regression.py --check    compare and list, never ask (git bisect run)
    orthodraw_regression.py --show     compare and print old and new, never ask
    orthodraw_regression.py --accept   keep this build's drawings from now on

When drawings differ and this is a terminal:

    v        see the old and new drawings, one corpus file after another
             (space: the next one, q: back)
    accept   typed in full: this build's drawings are the correct ones from
             now on (rewrites test/orthodraw_golden.txt; commit it)
    space    ignore this

Exit status: 0 nothing changed, or the change was accepted; 1 drawings differ;
2 it could not be run (no knoodledraw, no golden file, corpus not checked out).

The corpus is Git LFS data. A file that is still an LFS pointer is named as
such: run `git lfs pull`, or `git lfs checkout` in a fresh worktree.

Environment, for test/orthodraw_regression_check.py: ORTHODRAW_REGRESSION_ROOT
(where data/diagrams is), _DRAW (the knoodledraw to run), _GOLDEN, _BUILD (the
name of this build), _KEYS (the answers, instead of a keyboard: v, space, q,
accept, or any other word as a mistyped accept).
"""

import datetime
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ENV = os.environ.get
ROOT = ENV("ORTHODRAW_REGRESSION_ROOT") or os.path.join(HERE, "..")
DRAW = ENV("ORTHODRAW_REGRESSION_DRAW") or os.path.join(HERE, "..", "tools", "knoodledraw")
GOLDEN = ENV("ORTHODRAW_REGRESSION_GOLDEN") or os.path.join(HERE, "orthodraw_golden.txt")
KEYS = ENV("ORTHODRAW_REGRESSION_KEYS")

HEADER = """\
# OrthoDraw's default drawings: `knoodledraw --ascii FILE` for each corpus file.
# Written by test/orthodraw_regression.py --accept. Do not edit: the drawings
# are compared character for character, trailing spaces included.
"""


def corpus():
    d = os.path.join(ROOT, "data", "diagrams")
    files = sorted(glob.glob(os.path.join(d, "hardunknots", "*.tsv")))
    files += sorted(glob.glob(os.path.join(d, "tsunknots", "*.tsv")))[:20]
    files += sorted(glob.glob(os.path.join(d, "linktable", "*.tsv")))[:20]
    files += sorted(glob.glob(os.path.join(d, "childrensgame", "*.tsv")))[:10]
    return [os.path.relpath(f, ROOT) for f in files]


def pointerQ(rel):
    with open(os.path.join(ROOT, rel), "rb") as f:
        return f.read(24).startswith(b"version https://git-lfs")


def draw(rel):
    """(exit status, the drawing's lines)."""
    p = subprocess.run([DRAW, "--ascii", os.path.join(ROOT, rel)],
                       capture_output=True, timeout=300)
    text = p.stdout.decode("utf-8", errors="replace")
    if text.endswith("\n"):
        text = text[:-1]
    return p.returncode, (text.split("\n") if text else [])


def this_build():
    if ENV("ORTHODRAW_REGRESSION_BUILD"):
        return ENV("ORTHODRAW_REGRESSION_BUILD")
    try:
        p = subprocess.run(["git", "-C", HERE, "describe", "--tags", "--always", "--dirty"],
                           capture_output=True, text=True, timeout=30)
        if p.returncode == 0 and p.stdout.strip():
            return p.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        pass
    return "an unknown build"


def read_golden():
    """(build, date, {file: (exit status, lines)}), or None if there is no file."""
    if not os.path.exists(GOLDEN):
        return None
    build, date, drawings = "an unknown build", "", {}
    with open(GOLDEN, encoding="utf-8") as f:
        lines = f.read().split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        i += 1
        if line.startswith("# recorded\t"):
            parts = line.split("\t")
            build, date = parts[1], (parts[2] if len(parts) > 2 else "")
        elif line.startswith("=== "):
            rel, rc, n = line[4:].split("\t")
            n = int(n.split("=")[1])
            drawings[rel] = (int(rc.split("=")[1]), lines[i:i + n])
            i += n
    return build, date, drawings


def write_golden(build, drawings):
    with open(GOLDEN, "w", encoding="utf-8") as f:
        f.write(HEADER)
        f.write(f"# recorded\t{build}\t{datetime.date.today().isoformat()}\n")
        for rel, (rc, lines) in drawings.items():
            f.write(f"=== {rel}\texit={rc}\tlines={len(lines)}\n")
            for line in lines:
                f.write(line + "\n")


def show(rel, old, new, old_name, new_name, where):
    """Old and new side by side if the terminal is wide enough, else one above the other."""
    print(f"\n{where}  {rel}")
    for name, d in (("old", old), ("new", new)):
        if d is not None and d[0] != 0:
            print(f"  ({name}: knoodledraw exited {d[0]})")
    a = old[1] if old is not None else ["(not in the corpus then)"]
    b = new[1] if new is not None else ["(no longer in the corpus)"]
    # Shown without trailing spaces (knoodledraw pads its rows); compared with them.
    wa = max([len(s.rstrip()) for s in a] + [len(old_name) + 5])
    wb = max([len(s.rstrip()) for s in b] + [len(new_name) + 5])
    if wa + wb + 3 <= shutil.get_terminal_size((80, 24)).columns:
        print(f"{'old: ' + old_name:<{wa}}   new: {new_name}")
        for k in range(max(len(a), len(b))):
            x = a[k] if k < len(a) else ""
            y = b[k] if k < len(b) else ""
            print(f"{x.rstrip():<{wa}} {'|' if x == y else '!'} {y.rstrip()}".rstrip())
    else:
        print(f"--- old: {old_name}")
        print("\n".join(s.rstrip() for s in a))
        print(f"--- new: {new_name}")
        print("\n".join(s.rstrip() for s in b))


class Answers:
    """The user's answers: from the keyboard, or from ORTHODRAW_REGRESSION_KEYS."""

    def __init__(self):
        self.script = KEYS.split() if KEYS is not None else None

    def key(self):
        """One key, unechoed: 'v', ' ', 'q', 'a', ..."""
        if self.script is not None:
            if not self.script:
                return " "
            word = self.script[0]
            if word in ("v", "q"):
                self.script.pop(0)
                return word
            if word == "space":
                self.script.pop(0)
                return " "
            return "a"          # a typed word: the rest is read by rest_of_line
        import termios
        import tty
        fd = sys.stdin.fileno()
        saved = termios.tcgetattr(fd)
        try:
            tty.setcbreak(fd)
            c = os.read(fd, 1)      # not sys.stdin: its buffer would swallow keys typed ahead
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, saved)
        return c.decode("utf-8", errors="replace") if c else " "

    def rest_of_line(self):
        """After an 'a': the whole word as typed."""
        if self.script is not None:
            return self.script.pop(0)
        sys.stdout.write("a")
        sys.stdout.flush()
        word = b"a"
        while True:                 # the terminal is cooked again: it echoes, and gives whole lines
            c = os.read(sys.stdin.fileno(), 1)
            if not c or c in (b"\n", b"\r"):
                return word.decode("utf-8", errors="replace")
            word += c


def main():
    args = sys.argv[1:]
    known = {"--check", "--show", "--accept"}
    if any(a not in known for a in args) or len(args) > 1:
        print(__doc__)
        return 0 if args in (["-h"], ["--help"]) else 2

    if not os.path.exists(DRAW):
        print(f"orthodraw_regression: no knoodledraw at {DRAW}; run `make` in tools/")
        return 2
    files = corpus()
    pointers = [rel for rel in files if pointerQ(rel)]
    if not files or pointers:
        for rel in pointers:
            print(f"{rel} is a Git LFS pointer, not the diagram")
        print(f"orthodraw_regression: the corpus is not checked out ({len(files)} files,"
              f" {len(pointers)} of them Git LFS pointers); run `git lfs pull`")
        return 2

    new = {rel: draw(rel) for rel in files}
    build = this_build()

    def accept():
        write_golden(build, new)
        print(f"orthodraw_regression: the drawings of {build} are kept in"
              f" {os.path.relpath(GOLDEN)} ({len(new)} files). Commit it.")
        return 0

    golden = read_golden()
    if golden is None:
        if "--accept" in args:
            return accept()
        print(f"orthodraw_regression: no golden file at {GOLDEN}; --accept writes one")
        return 2
    old_build, old_date, old = golden

    changed = [rel for rel in files if old.get(rel) != new[rel]]
    changed += [rel for rel in old if rel not in new]
    if not changed:
        print(f"orthodraw_regression: {len(files)} drawings unchanged from {old_build}"
              f" (recorded {old_date}) to this build ({build})")
        return 0

    print(f"The output of OrthoDraw on {len(changed)} of {len(files)} test diagrams has"
          f" changed from {old_build} (recorded {old_date}) to this build ({build}):")
    for rel in changed:
        print(f"  {rel}")

    def view(answers=None):
        for k, rel in enumerate(changed):
            show(rel, old.get(rel), new.get(rel), old_build, build, f"[{k + 1}/{len(changed)}]")
            if answers is not None and k + 1 < len(changed):
                print("space: the next one    q: back", flush=True)
                if answers.key() in ("q", "Q", "\x1b"):
                    break

    if "--accept" in args:
        return accept()
    if "--show" in args:
        view()
        return 1
    interactiveQ = (KEYS is not None) or (sys.stdin.isatty() and sys.stdout.isatty())
    if "--check" in args or not interactiveQ:
        return 1

    answers = Answers()
    while True:
        print("\nPress v to see old and new diagrams, type accept to accept the new layouts"
              " as correct from now on, space to ignore this.", flush=True)
        try:
            c = answers.key()
        except KeyboardInterrupt:
            c = " "
        if c in ("v", "V"):
            view(answers)
        elif c in ("a", "A"):
            word = answers.rest_of_line().strip()
            if word == "accept":
                return accept()
            print(f"'{word}' is not accept: nothing is changed.")
        elif c in (" ", "\n", "\r", "q", "Q", "\x1b", "\x03", "\x04"):
            print("Ignored: the golden file is as it was.")
            return 1


if __name__ == "__main__":
    sys.exit(main())
