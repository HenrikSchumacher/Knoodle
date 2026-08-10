#!/usr/bin/env python3
"""Residual bits with the unit-circle Alexander fingerprint as context.

Bucketing by |Delta| at many |t|=1 points == bucketing by the full Alexander
polynomial (to float resolution), so this is the ceiling for Alexander-based
cascade stages: residual(any unit-circle evals) >= residual(full Delta) >=
residual(HOMFLY). Reports the sweep over 1..5 points plus joints with
det / v2 (both functions of Delta -- they can only tighten the fingerprint
toward the true Delta-partition, and SHOULD add ~nothing if 5 points already
resolve it).

usage: circle_entropy.py <pairs.bin> <key_len> <circle.tsv> <alex.txt> <v2.txt> [homfly shards...]
"""
import math
import struct
import sys
from collections import defaultdict
from pathlib import Path


def main() -> None:
    pairs_path, n = Path(sys.argv[1]), int(sys.argv[2])
    circle_path, alex_path, v2_path = (Path(p) for p in sys.argv[3:6])

    ids = []
    blob = pairs_path.read_bytes()
    rec = n + 4
    for i in range(0, len(blob), rec):
        ids.append(struct.unpack_from("<I", blob, i + n)[0])
    key_count = defaultdict(int)
    for t in ids:
        key_count[t] += 1

    fp_of = {}  # type -> tuple of 5 rounded log10|det| values
    for line in circle_path.read_text().splitlines():
        parts = line.split("\t")
        # round to 6 decimals in log10: far above LU noise (~1e-10), far below
        # genuine cross-polynomial separations
        fp_of[int(parts[0])] = tuple(round(float(x), 6) for x in parts[1:])

    det_of, v2_of = {}, {}
    for t, line in zip(ids, alex_path.read_text().splitlines()):
        if t not in det_of:
            det_of[t] = round(float(line.split("\t")[0]))
    for t, v in zip(ids, (int(x) for x in v2_path.read_text().split())):
        if t not in v2_of:
            v2_of[t] = v

    n_types = len(fp_of)
    n_keys = len(ids)
    missing = [t for t in det_of if t not in fp_of]
    if missing:
        print(f"WARNING: {len(missing)} types lack fingerprints; excluded")

    def residual(feat):
        buckets = defaultdict(set)
        for t, f in feat.items():
            buckets[f].add(t)
        bits = 0.0
        for t, kc in key_count.items():
            if t in feat:
                bits += kc * math.log2(len(buckets[feat[t]]))
        return bits / n_keys, len(buckets)

    print(f"types {n_types}, keys {n_keys}, unconditional {math.log2(len(det_of)):.2f} bits")

    def report(name, feat):
        bits, nb = residual(feat)
        print(f"  {name:<26} {bits:7.3f} bits  ({nb} buckets)")

    for k in range(1, 6):
        report(f"circle x{k}", {t: f[:k] for t, f in fp_of.items()})
    report("circle x5 + det", {t: (f, det_of[t]) for t, f in fp_of.items()})
    report("circle x5 + det + v2", {t: (f, det_of[t], v2_of[t]) for t, f in fp_of.items()})

    if len(sys.argv) > 6:
        poly_of = {}
        for shard in sys.argv[6:]:
            for line in Path(shard).read_text().splitlines():
                tid, p = line.split("\t", 1)
                poly_of[int(tid)] = p
        report("homfly (ceiling)", poly_of)
        report("circle x5 + homfly", {t: (f, poly_of[t]) for t, f in fp_of.items()})


if __name__ == "__main__":
    main()
