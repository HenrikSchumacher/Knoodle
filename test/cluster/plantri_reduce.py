#!/usr/bin/env python3
"""Reassemble a sharded plantri_check sweep.

Sums the `SUMMARY` lines emitted by each shard (counts are additive because the
plantri res/mod shards partition the graphs disjointly and exhaustively), and
inventories the dumped reproducer diagrams.

    python3 plantri_reduce.py sweep_9cx/summary/job_*.out
    python3 plantri_reduce.py --dump sweep_9cx/dump sweep_9cx/summary/job_*.out

Exit code: 0 if no shard reported a bug and all expected shards are present,
1 if any bug was found, 2 if shards are missing (incomplete sweep).
"""
import sys, glob

INT_FIELDS = ["tested", "knots", "links", "comp_changed",
              "homfly_knot", "homfly_link", "skipped", "recon_fail", "bugs",
              "klut_found", "klut_notfound", "klut_overrange"]


def main(argv):
    dump_dir = None
    paths = []
    it = iter(argv)
    for a in it:
        if a == "--dump":
            dump_dir = next(it)
        else:
            paths.extend(glob.glob(a))

    total = {k: 0 for k in INT_FIELDS}
    shards_seen, mods, max_seconds = set(), set(), 0.0
    files = 0
    for path in paths:
        got_summary = False
        for line in open(path):
            if not line.startswith("SUMMARY"):
                continue
            got_summary = True
            fields = dict(kv.split("=", 1) for kv in line.rstrip("\n").split("\t")[1:])
            for k in INT_FIELDS:
                total[k] += int(fields.get(k, 0))
            res, _, mod = fields.get("shard", "/").partition("/")
            shards_seen.add((res, mod))
            if mod:
                mods.add(mod)
            max_seconds = max(max_seconds, float(fields.get("seconds", 0)))
        files += 1
        if not got_summary:
            print(f"WARNING: no SUMMARY line in {path}", file=sys.stderr)

    print(f"shard outputs read : {files}")
    for k in INT_FIELDS:
        print(f"  {k:14s}: {total[k]:,}")
    print(f"  slowest shard : {max_seconds:.1f}s")

    incomplete = False
    if len(mods) == 1:
        mod = int(next(iter(mods)))
        present = {int(r) for r, _ in shards_seen}
        missing = sorted(set(range(mod)) - present)
        if missing:
            incomplete = True
            head = ", ".join(map(str, missing[:20])) + (" ..." if len(missing) > 20 else "")
            print(f"  MISSING {len(missing)}/{mod} shards: {head}")
        else:
            print(f"  all {mod} shards present")
    elif len(mods) > 1:
        print(f"  WARNING: mixed mod values {sorted(mods)} -- counts may not be a clean partition")

    if dump_dir is not None:
        dumps = glob.glob(f"{dump_dir}/*.tsv")
        print(f"  reproducers in {dump_dir}: {len(dumps)}")

    if total["klut_found"] or total["klut_notfound"]:
        print(f"  KLUT coverage : {total['klut_found']:,} pieces found, "
              f"{total['klut_notfound']:,} NOT in table, "
              f"{total['klut_overrange']:,} over 13 crossings")

    if total["bugs"] > 0 or total["klut_notfound"] > 0:
        if total["bugs"]:
            print(f"RESULT: FAIL -- {total['bugs']:,} simplification bug(s) "
                  f"({total['comp_changed']:,} component-count, "
                  f"{total['homfly_knot'] + total['homfly_link']:,} HOMFLY)")
        if total["klut_notfound"]:
            print(f"RESULT: FAIL -- {total['klut_notfound']:,} prime piece(s) missing from KLUT")
        return 1
    if incomplete:
        print("RESULT: INCOMPLETE -- no bugs in the shards that ran, but some are missing")
        return 2
    print("RESULT: PASS -- every shard clean")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
