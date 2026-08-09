#!/usr/bin/env python3
"""Export (MacLeod key, knot-type ID) pairs from a Klut subtable.

Reads Klut_Keys_NN.bin (raw concatenation of NN-byte short MacLeod codes,
grouped by knot name in Klut_Values_NN.tsv order) and writes a binary
pair file sorted lexicographically by key, ready for the FST builder:

    repeated records of [NN bytes key][4 bytes little-endian type ID]

Also prints alphabet statistics (distinct byte values used), which decide
which packing variants are worth running.
"""
import sys
import struct
from pathlib import Path


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(f"usage: {sys.argv[0]} <data_dir> <crossings> <out.bin>")
    data_dir = Path(sys.argv[1])
    n = int(sys.argv[2])
    out_path = Path(sys.argv[3])

    keys_file = data_dir / f"Klut_Keys_{n:02d}.bin"
    values_file = data_dir / f"Klut_Values_{n:02d}.tsv"

    # values file: whitespace-separated (name, count) pairs, in key-file order
    groups = []  # (name, count)
    tokens = values_file.read_text().split()
    for i in range(0, len(tokens), 2):
        groups.append((tokens[i], int(tokens[i + 1])))

    blob = keys_file.read_bytes()
    total = sum(c for _, c in groups)
    assert len(blob) == total * n, (
        f"key file size {len(blob)} != {total} keys * {n} bytes"
    )

    pairs = []
    seen_bytes = set()
    off = 0
    for type_id, (_, count) in enumerate(groups):
        for _ in range(count):
            key = blob[off : off + n]
            seen_bytes.update(key)
            pairs.append((key, type_id))
            off += n

    pairs.sort(key=lambda p: p[0])
    # keys must be unique for an FST map
    for a, b in zip(pairs, pairs[1:]):
        assert a[0] != b[0], f"duplicate key {a[0].hex()}"

    with out_path.open("wb") as f:
        for key, type_id in pairs:
            f.write(key)
            f.write(struct.pack("<I", type_id))

    print(f"crossings:      {n}")
    print(f"keys:           {total}")
    print(f"types:          {len(groups)}")
    print(f"key bytes:      {total * n} ({n} B/key on disk)")
    print(f"alphabet size:  {len(seen_bytes)}")
    print(f"byte range:     {min(seen_bytes)}..{max(seen_bytes)}")


if __name__ == "__main__":
    main()
