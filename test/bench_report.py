#!/usr/bin/env python3
"""Invariant-throughput report: join the three engines' per-diagram result TSVs
(klut_ivt_bench, libhomfly_ivt_bench, regina_ivt_bench) by diagram index and
present throughput stratified by KLUT's reduced crossing number.

The random plantri firehose is dominated by easy knots (mostly the unknot at low
crossing numbers), so a single mean is uninformative. KLUT identifies every
diagram, so we use its result_key ("<reduced_c>:<label>") as the labeling oracle
and bin EVERY engine's timing by that reduced crossing number. Per bin and per
engine we report count, mean, p50, p99, and throughput -- the tail (p99/max) is
what a firehose's wall-clock actually rides on.

Interpretive frame (the reduction spectrum):
  libhomfly = HOMFLY on the RAW diagram (no simplify) -- cost tracks diagram size
  Regina    = simplify internally, then compute the polynomial
  KLUT      = pass-simplify, then table lookup (Reapr escalation only on a miss)

Pure Python (no Regina); reads only the TSVs. Regina is optional (a subsample is
fine -- only the indices it covers are binned for Regina).

Usage:
    python3 bench_report.py --klut=K.tsv --libhomfly=L.tsv [--regina=R.tsv]
"""

import argparse


def load(path):
    """index -> (nc, construct_ns, compute_ns, result_key)."""
    rows = {}
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            idx, nc, cns, kns, key = line.rstrip("\n").split("\t")
            rows[int(idx)] = (int(nc), float(cns), float(kns), key)
    return rows


def pct(sorted_vals, q):
    if not sorted_vals:
        return 0.0
    if len(sorted_vals) == 1:
        return sorted_vals[0]
    pos = q * (len(sorted_vals) - 1)
    lo = int(pos)
    hi = min(lo + 1, len(sorted_vals) - 1)
    frac = pos - lo
    return sorted_vals[lo] * (1 - frac) + sorted_vals[hi] * frac


def stats(vals):
    s = sorted(vals)
    n = len(s)
    total = sum(s)
    mean = total / n if n else 0.0
    tput = n / (total * 1e-9) if total else 0.0
    return {
        "n": n, "mean": mean, "p50": pct(s, 0.50),
        "p99": pct(s, 0.99), "max": s[-1] if s else 0.0, "tput": tput,
    }


def reduced_c(klut_key):
    """Leading integer of a KLUT result_key '<reduced_c>:<label>' (-1 for links)."""
    return int(klut_key.split(":", 1)[0])


def main():
    ap = argparse.ArgumentParser(description="Invariant-throughput report.")
    ap.add_argument("--klut", required=True, help="klut_ivt_bench result TSV (labeling oracle)")
    ap.add_argument("--libhomfly", required=True, help="libhomfly_ivt_bench result TSV")
    ap.add_argument("--regina", default=None, help="regina_ivt_bench result TSV (optional)")
    args = ap.parse_args()

    klut = load(args.klut)
    engines = [("KLUT", klut), ("libhomfly", load(args.libhomfly))]
    if args.regina:
        engines.append(("Regina", load(args.regina)))

    # Bin every engine's compute times by KLUT's reduced crossing number.
    bins = {}  # reduced_c -> {engine_name -> [compute_ns, ...]}
    for idx, (_, _, _, key) in klut.items():
        rc = reduced_c(key)
        slot = bins.setdefault(rc, {name: [] for name, _ in engines})
        for name, rows in engines:
            if idx in rows:
                slot[name].append(rows[idx][2])

    names = [name for name, _ in engines]
    print("Invariant-throughput by reduced crossing number "
          "(compute-only, ns/item unless noted)\n")
    header = f"{'red_c':>6} {'engine':>10} {'n':>8} {'mean':>10} {'p50':>10} {'p99':>10} {'max':>12} {'items/s':>12}"
    print(header)
    print("-" * len(header))
    for rc in sorted(bins):
        for name in names:
            st = stats(bins[rc][name])
            if st["n"] == 0:
                continue
            tag = "unknot" if rc == 0 else ("link" if rc < 0 else f"{rc}cx")
            print(f"{tag:>6} {name:>10} {st['n']:>8} {st['mean']:>10.0f} "
                  f"{st['p50']:>10.0f} {st['p99']:>10.0f} {st['max']:>12.0f} {st['tput']:>12.0f}")
        print()

    print("OVERALL (all diagrams):")
    print(header)
    print("-" * len(header))
    for name, rows in engines:
        st = stats([v[2] for v in rows.values()])
        print(f"{'all':>6} {name:>10} {st['n']:>8} {st['mean']:>10.0f} "
              f"{st['p50']:>10.0f} {st['p99']:>10.0f} {st['max']:>12.0f} {st['tput']:>12.0f}")

    # Mean per-item compute cost relative to KLUT, over the diagrams both cover.
    print("\nMean per-item compute cost relative to KLUT (overall):")
    for name, rows in engines:
        common = [k for k in rows if k in klut]
        e = sum(rows[k][2] for k in common)
        kk = sum(klut[k][2] for k in common)
        ratio = e / kk if kk else float("nan")
        print(f"  {name:>10}: {ratio:6.2f}x  (over {len(common)} shared diagrams)")


if __name__ == "__main__":
    main()
