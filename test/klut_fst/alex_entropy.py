#!/usr/bin/env python3
"""Size determinant / Alexander-evaluation cascade stages against v2 and HOMFLY.

Reads per-key invariant values (alexanderval --at=-1,2 output and the v2
file, both in pairs-file order), checks each candidate feature for constancy
across the keys of each type (a feature that varies within a type cannot be
used for bucketing), then reports the key-weighted residual bits
log2(types per bucket) for each feature set.

usage: alex_entropy.py <pairs.bin> <key_len> <alex.txt> <v2.txt> [reps.tsv...]
       (reps.tsv = optional homfly shard files for the joint comparison)
"""
import math
import struct
import sys
from collections import defaultdict
from pathlib import Path


def residual(pairs_ids, key_count, feature_of_type):
    """Key-weighted log2(bucket size in types) for a per-type feature."""
    buckets = defaultdict(set)
    for t, f in feature_of_type.items():
        buckets[f].add(t)
    bits = 0.0
    n = 0
    for t, kc in key_count.items():
        bits += kc * math.log2(len(buckets[feature_of_type[t]]))
        n += kc
    return bits / n, len(buckets)


def main() -> None:
    pairs_path, n = Path(sys.argv[1]), int(sys.argv[2])
    alex_lines = Path(sys.argv[3]).read_text().splitlines()
    v2_vals = [int(x) for x in Path(sys.argv[4]).read_text().split()]

    ids = []
    blob = pairs_path.read_bytes()
    rec = n + 4
    for i in range(0, len(blob), rec):
        ids.append(struct.unpack_from("<I", blob, i + n)[0])
    assert len(ids) == len(alex_lines) == len(v2_vals)

    key_count = defaultdict(int)
    for t in ids:
        key_count[t] += 1

    # Parse per-key features. det -> exact integer; |Delta(2)| -> rounded to
    # 9 significant digits (floating LU noise) for the constancy test.
    det_of, a2_of = {}, {}
    det_bad, a2_bad = set(), set()
    for t, line in zip(ids, alex_lines):
        d_str, a_str = line.split("\t")
        det = round(float(d_str))
        a2 = float(f"{float(a_str):.9g}")
        if t in det_of:
            if det_of[t] != det:
                det_bad.add(t)
            if a2_of[t] != a2:
                a2_bad.add(t)
        else:
            det_of[t] = det
            a2_of[t] = a2

    n_types = len(det_of)
    print(f"types {n_types}, keys {len(ids)}")
    print(f"determinant: {'CONSTANT on all types' if not det_bad else f'VARIES on {len(det_bad)} types'};"
          f" {len(set(det_of.values()))} distinct values")
    print(f"|Delta(2)|:  {'CONSTANT on all types' if not a2_bad else f'VARIES on {len(a2_bad)} types (unit t^k drift)'};"
          f" {len(set(a2_of.values()))} distinct values (9 sig figs)")

    v2_of = {}
    for t, v in zip(ids, v2_vals):
        v2_of[t] = v

    uncond = math.log2(n_types)
    print(f"\nkey-weighted residual bits (unconditional {uncond:.2f}):")

    def report(name, feat):
        bits, nb = residual(ids, key_count, feat)
        print(f"  {name:<22} {bits:7.3f} bits  ({nb} buckets)")

    report("det", det_of)
    report("v2", v2_of)
    report("v2 + det", {t: (v2_of[t], det_of[t]) for t in det_of})
    if not a2_bad:
        report("v2 + det + |D(2)|",
               {t: (v2_of[t], det_of[t], a2_of[t]) for t in det_of})

    # optional: joint with HOMFLY shards for the ceiling comparison
    if len(sys.argv) > 5:
        poly_of = {}
        for shard in sys.argv[5:]:
            for line in Path(shard).read_text().splitlines():
                type_id, p = line.split("\t", 1)
                poly_of[int(type_id)] = p
        report("homfly", poly_of)
        report("homfly + det", {t: (poly_of[t], det_of[t]) for t in det_of})


if __name__ == "__main__":
    main()
