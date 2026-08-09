#!/usr/bin/env python3
"""Size the HOMFLY stage of an invariant cascade over a KLUT table.

Reads the per-type polynomials from homfly_reps_probe shards plus the pairs
file (for key counts per type) and the values TSV (for names), groups types
by polynomial, and reports how much of the type-ID storage HOMFLY-as-context
erases: the key-weighted residual bits log2(types per HOMFLY class), the
size of a concrete exception ledger, and the surviving collision classes.

usage: homfly_entropy.py <pairs.bin> <key_len> <values.tsv> <reps.tsv...>
"""
import math
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path


def main() -> None:
    pairs_path, n, values_path = Path(sys.argv[1]), int(sys.argv[2]), Path(sys.argv[3])

    # keys per type
    key_count = Counter()
    blob = pairs_path.read_bytes()
    rec = n + 4
    for i in range(0, len(blob), rec):
        key_count[struct.unpack_from("<I", blob, i + n)[0]] += 1

    # type id -> name (values file order = id order)
    tokens = values_path.read_text().split()
    names = [tokens[i] for i in range(0, len(tokens), 2)]

    # type id -> polynomial
    poly = {}
    for shard in sys.argv[4:]:
        for line in Path(shard).read_text().splitlines():
            type_id, p = line.split("\t", 1)
            poly[int(type_id)] = p
    assert len(poly) == len(names) == len(key_count), (
        f"{len(poly)} polys, {len(names)} names, {len(key_count)} key groups"
    )

    classes = defaultdict(list)  # polynomial -> [type_id]
    for type_id, p in poly.items():
        classes[p].append(type_id)

    n_types = len(poly)
    n_keys = sum(key_count.values())
    n_classes = len(classes)
    sizes = Counter(len(v) for v in classes.values())

    print(f"types {n_types}, keys {n_keys}, distinct HOMFLY polynomials {n_classes}")
    print(f"class-size census (size: #classes): "
          + ", ".join(f"{s}: {c}" for s, c in sorted(sizes.items())))

    # key-weighted residual information (same measure as the v2 analysis)
    resid_bits = 0.0
    colliding_keys = 0
    ledger_bits = 0  # ceil(log2 k) per key in a size-k class, k > 1
    for members in classes.values():
        k = len(members)
        keys_here = sum(key_count[t] for t in members)
        resid_bits += keys_here * math.log2(k)
        if k > 1:
            colliding_keys += keys_here
            ledger_bits += keys_here * math.ceil(math.log2(k))
    resid_bits /= n_keys

    uncond = math.log2(n_types)
    print(f"\nkey-weighted residual log2(types per class): {resid_bits:.4f} bits"
          f" (unconditional {uncond:.2f}; v2 alone left 11.41)")
    print(f"keys in colliding classes: {colliding_keys} of {n_keys}"
          f" ({100.0 * colliding_keys / n_keys:.2f}%)")
    print(f"exception ledger at ceil(log2 k) bits/colliding key: "
          f"{ledger_bits / 8:.0f} B total ({ledger_bits / 8 / n_keys:.4f} B/key)")

    print("\nlargest surviving collision classes:")
    biggest = sorted(classes.values(), key=len, reverse=True)[:10]
    for members in biggest:
        if len(members) == 1:
            break
        keys_here = sum(key_count[t] for t in members)
        print(f"  k={len(members)} ({keys_here} keys): "
              + " ".join(names[t] for t in sorted(members)))


if __name__ == "__main__":
    main()
