#!/usr/bin/env python3
"""Invariant-throughput driver #3 of 3 (Regina).

Reads the frozen benchmark dataset (make_bench_set) and, for each diagram, times:
    construct - regina.Link.fromPD(...) from the stored signed PD code
    compute   - link.homflyLM()   (Regina reduces internally, then computes)
and emits one per-diagram result row (bench_io schema):
    index  nc  construct_ns  compute_ns  result_key

result_key is regina's homflyLM().str() -- an opaque, deterministic token (a
within-engine sanity check). The authoritative cross-engine correctness check
(homfly_xcheck-style: libhomfly (L,M) == Regina homflyLM(l,m)) lives in the
cross-check script, which parses libhomfly's canonical term string into a
regina.Laurent2 and compares with ==.

Needs Python + Regina (the test/venv). Single item at a time (apples-to-apples
with the C++ drivers). Use --limit to run Regina on a subsample of a large set.

Usage:
    venv/bin/python regina_ivt_bench.py --dataset=PATH [--out=PATH] [--limit=N] [--warmup=N]
"""

import argparse
import sys
from time import perf_counter_ns

import regina

import bench_io


def regina_pd(crossings):
    """0-based Knoodle crossings -> Regina 1-based PD string (cols 0-3)."""
    rows = [f"[{r[0] + 1},{r[1] + 1},{r[2] + 1},{r[3] + 1}]" for r in crossings]
    return "[" + ",".join(rows) + "]"


def homfly_one(crossings):
    """Return (construct_ns, compute_ns, result_key)."""
    t0 = perf_counter_ns()
    link = regina.Link.fromPD(regina_pd(crossings))
    t1 = perf_counter_ns()
    poly = link.homflyLM()
    t2 = perf_counter_ns()
    key = poly.str().replace("\t", " ").replace("\n", " ")
    return t1 - t0, t2 - t1, key


def main():
    ap = argparse.ArgumentParser(description="Regina invariant-throughput driver.")
    ap.add_argument("--dataset", required=True, help="frozen benchmark dataset")
    ap.add_argument("--out", default=None, help="per-diagram result TSV (default: stdout)")
    ap.add_argument("--warmup", type=int, default=200, help="untimed warmup computations")
    ap.add_argument("--limit", type=int, default=0, help="process only first N diagrams (0 = all)")
    args = ap.parse_args()

    _, diagrams = bench_io.load_dataset(args.dataset)
    n = len(diagrams)
    if args.limit and args.limit < n:
        n = args.limit
    print(f"regina_ivt_bench: {n} diagrams from {args.dataset}", file=sys.stderr)

    for i in range(min(args.warmup, n)):
        homfly_one(diagrams[i % n][1])

    out = open(args.out, "w") if args.out else sys.stdout
    try:
        out.write(bench_io.RESULT_HEADER + "\n")
        total_compute = 0
        w0 = perf_counter_ns()
        for i in range(n):
            nc, crossings = diagrams[i]
            construct_ns, compute_ns, key = homfly_one(crossings)
            total_compute += compute_ns
            out.write(f"{i}\t{nc}\t{construct_ns}\t{compute_ns}\t{key}\n")
        w1 = perf_counter_ns()
    finally:
        if args.out:
            out.close()

    mean = total_compute / n if n else 0.0
    tput = n / (total_compute * 1e-9) if total_compute else 0.0
    print(
        f"regina_ivt_bench: homfly {mean:.1f} ns/item (mean), {tput:.1f} items/s "
        f"(compute-only); wall {(w1 - w0) * 1e-9:.4f} s",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
