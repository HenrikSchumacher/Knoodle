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
import re
import sys

MANIFEST = "manifest.tsv"

# Sources that are deliberately not build targets. Keep this list short and
# say why for each -- it is the escape hatch the lint exists to constrain.
NOT_TARGETS = {
    # helper headers have no main(); only .cpp files are considered anyway
}

VALID_KINDS = {"test", "tool", "bench", "script"}
VALID_NEEDS = {"plain", "homfly", "umfpack", "homfly_umfpack", "boost_umfpack", "-"}


def read_manifest(path=MANIFEST):
    rows = {}
    for lineno, line in enumerate(open(path), 1):
        if line.startswith("#") or not line.strip():
            continue
        f = line.rstrip("\n").split("\t")
        if len(f) != 9:
            raise SystemExit(
                f"{path}:{lineno}: expected 9 tab-separated columns, got {len(f)}\n"
                f"  {f!r}\n"
                f"  (columns are: target kind needs source light-args full-args"
                f" release-args work group)\n"
                f"  (a row from before the release tier wants '=' inserted after"
                f" its full-args, and a group -- or '-' -- at the end)"
            )
        target, kind, needs, source, light, full, release, work, group = f
        rows[target] = {
            "group": None if group == "-" else group,
            "lineno": lineno,
            "kind": kind,
            "needs": needs,
            "source": source if source != "-" else target + ".cpp",
            "light": [] if light == "-" else light.split(),
            "full": [] if full == "-" else full.split(),
            "release": None if release == "=" else
                       [] if release == "-" else release.split(),
            "work": None if work == "-" else work,
        }
    return rows


def main():
    rows = read_manifest()
    problems = []

    for target, r in sorted(rows.items()):
        # A script row names its own interpreter file and compiles to nothing.
        if r["kind"] == "script" and not r["source"].endswith(".py"):
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: kind=script must name a .py "
                f"source, got '{r['source']}'"
            )
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
        # A reduced test with no work pattern can hollow out unnoticed.
        if r["kind"] == "test" and (r["light"] or r["full"] or r["release"]) and not r["work"]:
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: has reduced arguments but no "
                f"work pattern, so it could be reduced to a no-op and still pass"
            )
        if r["work"]:
            try:
                re.compile(r["work"])
            except re.error as e:
                problems.append(
                    f"{MANIFEST}:{r['lineno']}: {target}: work pattern is not a "
                    f"valid regex ({e})")
        if not os.path.exists(r["source"]):
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: no such source file '{r['source']}'"
            )

    # Groups: one translation unit, one link line. So every member must be a
    # compiled test and they must all link the same way.
    group_needs = {}
    for target, r in sorted(rows.items(), key=lambda kv: kv[1]["lineno"]):
        g = r["group"]
        if not g:
            continue
        if not re.fullmatch(r"[A-Za-z0-9_]+", g):
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: group '{g}' must be letters, "
                f"digits and '_' only (it becomes a file name and a make target)")
        if r["kind"] != "test":
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: only kind=test can be in a "
                f"group, this is kind={r['kind']}. Set its group column to '-'.")
        first = group_needs.setdefault(g, (r["needs"], target))
        if first[0] != r["needs"]:
            problems.append(
                f"{MANIFEST}:{r['lineno']}: {target}: needs '{r['needs']}' but group "
                f"'{g}' is linked as '{first[0]}' (from {first[1]}). A group is one "
                f"binary, so its tests must share one link shape.")

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
