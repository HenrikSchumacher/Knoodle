#!/usr/bin/env python3
"""Build v2-prefixed / ID-remapped variants of a KLUT pairs file.

Inputs: a pairs file from export_pairs.py (records [n bytes key][4 B LE id],
sorted by key) and a v2 file with one integer per record, same order.

Verifies v2 is constant on each type ID (it is a knot invariant; a violation
means a table or v2 bug), then writes four variants for the FST benchmark:

  <stem>_remap.bin    key unchanged (n bytes);   IDs renumbered in key order
  <stem>_v2.bin       key = [v2+bias] + key;     original IDs
  <stem>_v2remap.bin  key = [v2+bias] + key;     IDs renumbered in new key order
  (the plain file itself is the fourth cell of the 2x2)

"IDs renumbered in key order" = first-appearance order along the sorted key
stream, which makes the FST's output values correlate with key order.
"""
import struct
import sys
from pathlib import Path


def load_pairs(path: Path, n: int):
    blob = path.read_bytes()
    rec = n + 4
    assert len(blob) % rec == 0
    return [
        (blob[i : i + n], struct.unpack_from("<I", blob, i + n)[0])
        for i in range(0, len(blob), rec)
    ]


def write_pairs(path: Path, pairs):
    pairs = sorted(pairs, key=lambda p: p[0])
    with path.open("wb") as f:
        for key, type_id in pairs:
            f.write(key)
            f.write(struct.pack("<I", type_id))
    print(f"wrote {path.name}: {len(pairs)} records, key_len {len(pairs[0][0])}")


def remap_ids(pairs):
    """Renumber IDs by first appearance along the sorted key stream."""
    pairs = sorted(pairs, key=lambda p: p[0])
    new_id = {}
    out = []
    for key, type_id in pairs:
        if type_id not in new_id:
            new_id[type_id] = len(new_id)
        out.append((key, new_id[type_id]))
    return out


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(f"usage: {sys.argv[0]} <pairs.bin> <key_len> <v2.txt>")
    pairs_path = Path(sys.argv[1])
    n = int(sys.argv[2])
    v2s = [int(line) for line in Path(sys.argv[3]).read_text().split()]

    pairs = load_pairs(pairs_path, n)
    assert len(pairs) == len(v2s), f"{len(pairs)} pairs vs {len(v2s)} v2 values"

    # v2 must be constant on each type
    by_type = {}
    for (key, type_id), v2 in zip(pairs, v2s):
        if type_id in by_type:
            assert by_type[type_id] == v2, (
                f"type {type_id}: v2 {by_type[type_id]} vs {v2} on key {key.hex()}"
            )
        else:
            by_type[type_id] = v2
    lo, hi = min(v2s), max(v2s)
    print(f"v2 constant on all {len(by_type)} types; range [{lo}, {hi}], "
          f"{len(set(v2s))} distinct values")
    assert hi - lo < 256, "v2 does not fit in one biased byte"
    bias = -lo

    stem = pairs_path.with_suffix("")
    write_pairs(Path(f"{stem}_remap.bin"), remap_ids(pairs))

    v2_pairs = [
        (bytes([v2 + bias]) + key, type_id)
        for (key, type_id), v2 in zip(pairs, v2s)
    ]
    write_pairs(Path(f"{stem}_v2.bin"), v2_pairs)
    write_pairs(Path(f"{stem}_v2remap.bin"), remap_ids(v2_pairs))


if __name__ == "__main__":
    main()
