"""Generate Klut_Keys_NN.bin / Klut_Values_NN.tsv prime-knot lookup tables.

Pipeline per crossing count c:
  1. plantri_shadows.knot_shadows: enumerate prime reduced knot shadows.
  2. knoodle.sieve_shadows: enumerate over/under assignments, simplify with
     REAPR, keep the MacLeod codes of diagrams that stay at c crossings
     (proven-minimal and unproven alike -- unproven survivors are typically
     minimal non-alternating diagrams, and stuck diagrams of simpler knots
     also make useful lookup keys).
  3. name_knots.identify_name: name each code's representative diagram.
  4. Emit the table files and validate the number of distinct names whose
     crossing number equals c against the knot census.

Run inside tablegen/.venv (needs SnapPy):
  tablegen/.venv/bin/python3 tablegen/make_tables.py \
      --plantri /path/to/plantri --out /path/to/tables --max-crossings 8
"""

import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from knoodle import _knoodle as kn
from plantri_shadows import knot_shadows
from name_knots import identify_name

# Prime knots per crossing number.
CENSUS = {3: 1, 4: 1, 5: 2, 6: 3, 7: 7, 8: 21, 9: 49,
          10: 165, 11: 552, 12: 2176, 13: 9988}


def name_crossing_number(name):
    m = re.fullmatch(r"(\d+)_\d+", name) or re.fullmatch(r"(\d+)[an]\d+", name)
    return int(m.group(1)) if m else None


def build_level(c, plantri, rattle_iter, threads):
    shadows = knot_shadows(c, plantri)
    flat = [x for s in shadows for x in s]
    print(f"c={c}: {len(shadows)} knot shadows, sieving ...", flush=True)
    minimal, other = kn.sieve_shadows(flat, c, rattle_iter, threads)
    print(f"c={c}: {len(minimal)} proven-minimal codes, "
          f"{len(other)} unproven codes, naming ...", flush=True)

    names = {}
    unidentified = 0
    for code, rep in minimal + other:
        name = identify_name(rep)
        if name is None:
            unidentified += 1
            continue
        names[code] = name
    return names, unidentified


def write_level(c, names, out_dir):
    groups = collections.defaultdict(list)
    for code, name in names.items():
        groups[name].append(code)

    values_path = os.path.join(out_dir, f"Klut_Values_{c:02d}.tsv")
    keys_path = os.path.join(out_dir, f"Klut_Keys_{c:02d}.bin")
    with open(values_path, "w") as values, open(keys_path, "wb") as keys:
        for name in sorted(groups):
            codes = sorted(groups[name])
            values.write(f"{name} {len(codes)}\n")
            for code in codes:
                assert len(code) == c
                keys.write(code)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", required=True, help="output directory")
    ap.add_argument("--plantri", default="plantri", help="plantri binary")
    ap.add_argument("--min-crossings", type=int, default=3)
    ap.add_argument("--max-crossings", type=int, default=8)
    ap.add_argument("--rattle-iter", type=int, default=25)
    ap.add_argument("--threads", type=int, default=0)
    args = ap.parse_args()

    os.makedirs(args.out, exist_ok=True)

    ok = True
    for c in range(args.min_crossings, args.max_crossings + 1):
        names, unidentified = build_level(
            c, args.plantri, args.rattle_iter, args.threads)
        write_level(c, names, args.out)

        at_level = {n for n in set(names.values())
                    if name_crossing_number(n) == c}
        expected = CENSUS.get(c)
        status = "OK" if expected == len(at_level) else "MISMATCH"
        if expected != len(at_level):
            ok = False
        print(f"c={c}: {len(names)} codes, "
              f"{len(set(names.values()))} distinct names, "
              f"{len(at_level)} knots with crossing number {c} "
              f"(census: {expected}) {status}"
              + (f", {unidentified} unidentified" if unidentified else ""),
              flush=True)

    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
