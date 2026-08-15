#!/usr/bin/env python3
"""Check that manifest.tsv and the tree agree.

Two directions, and both matter:

  * a test source with no manifest row would not be built by `all`, which is
    exactly how klut_identify_check stopped compiling and link_alex_probe
    started exiting 1 without anyone noticing;
  * a manifest row with no source would break the build for everyone.

Run via `make lint`. Exit 0 = they agree.
"""

import os
import sys

MANIFEST = "manifest.tsv"

# Sources that are deliberately not build targets. Keep this list short and
# say why for each -- it is the escape hatch the lint exists to constrain.
NOT_TARGETS = {
    # helper headers have no main(); only .cpp files are considered anyway
}

VALID_KINDS = {"test", "tool", "bench"}
VALID_NEEDS = {"plain", "homfly", "umfpack", "homfly_umfpack", "boost_umfpack"}


def read_manifest(path=MANIFEST):
    rows = {}
    for lineno, line in enumerate(open(path), 1):
        if line.startswith("#") or not line.strip():
            continue
        f = line.rstrip("\n").split("\t")
        if len(f) != 6:
            raise SystemExit(
                f"{path}:{lineno}: expected 6 tab-separated columns, got {len(f)}\n"
                f"  {f!r}\n"
                f"  (columns are: target kind needs source light-args heavy-args)"
            )
        target, kind, needs, source, light, heavy = f
        rows[target] = {
            "lineno": lineno,
            "kind": kind,
            "needs": needs,
            "source": source if source != "-" else target + ".cpp",
            "light": [] if light == "-" else light.split(),
            "heavy": [] if heavy == "-" else heavy.split(),
        }
    return rows


def main():
    rows = read_manifest()
    problems = []

    for target, r in sorted(rows.items()):
        if r["kind"] not in VALID_KINDS:
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: kind '{r['kind']}' is not one of "
                + "/".join(sorted(VALID_KINDS))
            )
        if r["needs"] not in VALID_NEEDS:
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: needs '{r['needs']}' is not one of "
                + "/".join(sorted(VALID_NEEDS))
            )
        if not os.path.exists(r["source"]):
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: no such source file '{r['source']}'"
            )

    claimed = {r["source"] for r in rows.values()}
    for cpp in sorted(f for f in os.listdir(".") if f.endswith(".cpp")):
        if cpp in claimed or cpp in NOT_TARGETS:
            continue
        # Only a translation unit with main() is a missing target; anything
        # else is a helper that happens to be a .cpp.
        try:
            has_main = "int main" in open(cpp, errors="replace").read()
        except OSError:
            has_main = True
        if has_main:
            problems.append(
                f"{cpp}: has main() but no row in {MANIFEST}, so `all` will not "
                f"build it and it will rot unnoticed. Add a row, or list it in "
                f"NOT_TARGETS with a reason."
            )

    if problems:
        print(f"manifest lint: {len(problems)} problem(s)\n")
        for p in problems:
            print("  " + p)
        return 1

    kinds = {}
    for r in rows.values():
        kinds[r["kind"]] = kinds.get(r["kind"], 0) + 1
    summary = ", ".join(f"{n} {k}" for k, n in sorted(kinds.items()))
    print(f"manifest lint OK: {len(rows)} targets ({summary})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
