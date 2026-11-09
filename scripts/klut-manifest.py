#!/usr/bin/env python3
"""klut-manifest.py -- write or check data/Klut/Klut_Manifest.txt.

The manifest records, for every KLUT table file, its size in bytes, the number
of keys it describes, and its SHA-256. knoodleidentify checks the tables against
it at startup (sizes, key counts, and the values files' SHA-256 on every run;
the keys files' SHA-256 with --verify-tables), so a damaged, partial or
mismatched table set is an error instead of wrong answers. See
docs/klut-table-failure-audit.md (T7).

The manifest also names the table set: `set` is the first 16 hex digits of the
SHA-256 of its file lines, so any change to any table changes it.
tools/klut_table_set.hpp records the set knoodleidentify is built for, and the
tool refuses another set unless given --allow-other-tables. This script keeps
the two in step.

    scripts/klut-manifest.py            # rewrite the manifest and the header
                                        # from data/Klut (after changing tables)
    scripts/klut-manifest.py --check    # verify data/Klut, the manifest and the
                                        # header agree; exit 1 if not

The manifest is named *.txt on purpose: .gitattributes sends *.bin and *.tsv to
Git LFS, and the manifest must be readable in a checkout made without LFS.

Format (version 1), one record per line, fields separated by single spaces,
'#' lines are comments:

    klut-manifest 1
    set <16 hex digits>
    <file name> <bytes> <keys> <sha256>      (one line per table file)

<keys> is (bytes / c) for Klut_Keys_cc.bin and the sum of the counts for
Klut_Values_cc.tsv; the two agree for an intact pair.
"""

import argparse
import hashlib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
KLUT = os.path.join(REPO, "data", "Klut")
HEADER = os.path.join(REPO, "tools", "klut_table_set.hpp")
MANIFEST_NAME = "Klut_Manifest.txt"
CROSSINGS = range(3, 14)
LFS_HEADER = b"version https://git-lfs.github.com/spec/"

HEADER_TEMPLATE = """\
#pragma once

// The KLUT table set this knoodleidentify is built for: the `set` line of
// data/Klut/Klut_Manifest.txt. Written by scripts/klut-manifest.py; do not edit
// by hand. knoodleidentify refuses tables from another set unless given
// --allow-other-tables, since knot numbering can differ between table sets.
// test/klut_table_check.py checks this matches the manifest.

#include <string_view>

inline constexpr std::string_view kKlutTableSet = "{set_id}";
"""


def table_files():
    for c in CROSSINGS:
        yield c, f"Klut_Keys_{c:02d}.bin", f"Klut_Values_{c:02d}.tsv"


def values_key_count(data, name):
    total = 0
    for i, line in enumerate(data.decode("ascii").split("\n")[:-1], 1):
        name_part, count = line.split("\t")
        if not re.fullmatch(r"[1-9][0-9]{0,8}", count):
            raise ValueError(f"{name} line {i}: bad count {count!r}")
        total += int(count)
    return total


def file_lines(klut_dir, crossings=CROSSINGS):
    """The manifest's file lines for the tables in klut_dir."""
    lines = []
    for c, keys, values in table_files():
        if c not in crossings:
            continue
        for name in (keys, values):
            path = os.path.join(klut_dir, name)
            with open(path, "rb") as f:
                data = f.read()
            if data.startswith(LFS_HEADER):
                raise ValueError(f"{path} is a Git LFS pointer, not a table: run `git lfs pull`")
            if name == keys:
                if len(data) % c:
                    raise ValueError(f"{name}: {len(data)} bytes is not a multiple of {c}")
                nkeys = len(data) // c
            else:
                nkeys = values_key_count(data, name)
            lines.append(f"{name} {len(data)} {nkeys} {hashlib.sha256(data).hexdigest()}")
    return lines


def set_id(lines):
    return hashlib.sha256("".join(l + "\n" for l in lines).encode()).hexdigest()[:16]


def manifest_text(lines):
    return ("# KLUT manifest: each table file's name, size in bytes, number of keys,\n"
            "# and SHA-256. Written by scripts/klut-manifest.py; do not edit by hand.\n"
            "klut-manifest 1\n"
            f"set {set_id(lines)}\n"
            + "".join(l + "\n" for l in lines))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true",
                    help="verify instead of writing; exit 1 on any difference")
    args = ap.parse_args()

    try:
        lines = file_lines(KLUT)
    except (OSError, ValueError) as e:
        print(f"klut-manifest: {e}", file=sys.stderr)
        return 1
    text = manifest_text(lines)
    header = HEADER_TEMPLATE.format(set_id=set_id(lines))
    manifest_path = os.path.join(KLUT, MANIFEST_NAME)

    if args.check:
        ok = True
        for path, want, what in ((manifest_path, text, "the manifest"),
                                 (HEADER, header, "tools/klut_table_set.hpp")):
            try:
                with open(path, newline="") as f:
                    have = f.read()
            except OSError as e:
                print(f"klut-manifest: cannot read {what}: {e}", file=sys.stderr)
                ok = False
                continue
            if have != want:
                print(f"klut-manifest: {what} does not match data/Klut. "
                      "Run scripts/klut-manifest.py to rewrite it.", file=sys.stderr)
                ok = False
        if ok:
            print(f"klut-manifest: data/Klut matches the manifest (table set {set_id(lines)}, "
                  f"{len(lines)} files)")
        return 0 if ok else 1

    with open(manifest_path, "w", newline="") as f:
        f.write(text)
    with open(HEADER, "w", newline="") as f:
        f.write(header)
    print(f"klut-manifest: wrote {os.path.relpath(manifest_path, REPO)} and "
          f"{os.path.relpath(HEADER, REPO)} (table set {set_id(lines)}, {len(lines)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
