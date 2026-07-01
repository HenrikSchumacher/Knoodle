#!/usr/bin/env python3
"""Correctness cross-check for the invariant-throughput benchmark: running all
three engines on one dataset yields a free oracle.

Two assertions over the diagrams the engines share:

  (1) libhomfly HOMFLY == Regina HOMFLY, term for term. libhomfly's (L,M) are
      identical to Regina's homflyLM(l,m) (proven in homfly_xcheck.py), so we
      rebuild libhomfly's canonical term string as a regina.Laurent2 and compare
      its .str() against the Regina driver's result_key (also a Laurent2 .str()).
      No Regina recompute -- it reuses the driver outputs.

  (2) KLUT "unknot" (result_key "0:U") <=> trivial HOMFLY (== 1). A knot KLUT
      reduces to the unknot must have HOMFLY 1; we flag any mismatch.

Needs Python + Regina (only to canonicalize libhomfly's terms into a Laurent2).
Reads the three drivers' result TSVs; if Regina ran on a subsample (--limit), only
the shared indices are checked.

Usage:
    venv/bin/python bench_xcheck.py --klut=K.tsv --libhomfly=L.tsv --regina=R.tsv [--max-report=N]
"""

import argparse
import sys

import regina


def load(path):
    rows = {}
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            idx, nc, cns, kns, key = line.rstrip("\n").split("\t")
            rows[int(idx)] = key
    return rows


def libkey_to_laurent(key):
    """Parse a libhomfly canonical key 'l,m,coef;...' into a regina.Laurent2."""
    poly = regina.Laurent2()
    if key in ("0", "MALFORMED"):
        return poly
    for term in key.split(";"):
        if not term:
            continue
        l, m, c = (int(x) for x in term.split(","))
        poly.set(l, m, c)
    return poly


def main():
    ap = argparse.ArgumentParser(description="Invariant-throughput correctness cross-check.")
    ap.add_argument("--klut", required=True)
    ap.add_argument("--libhomfly", required=True)
    ap.add_argument("--regina", required=True)
    ap.add_argument("--max-report", type=int, default=10, help="max mismatches to print")
    args = ap.parse_args()

    klut = load(args.klut)
    lib = load(args.libhomfly)
    reg = load(args.regina)

    trivial = regina.Laurent2()
    trivial.set(0, 0, 1)
    trivial_str = trivial.str()

    shared = sorted(set(lib) & set(reg))
    n = len(shared)
    lib_reg_ok = 0
    lib_reg_bad = []
    unknot_ok = 0
    unknot_bad = []

    for idx in shared:
        lib_str = libkey_to_laurent(lib[idx]).str()
        if lib_str == reg[idx]:
            lib_reg_ok += 1
        else:
            lib_reg_bad.append((idx, lib_str, reg[idx]))

        if idx in klut and klut[idx].startswith("0:"):
            if reg[idx] == trivial_str:
                unknot_ok += 1
            else:
                unknot_bad.append((idx, klut[idx], reg[idx]))

    print(f"bench_xcheck: {n} shared diagrams (libhomfly ∩ Regina)")
    print(f"  (1) libhomfly == Regina HOMFLY : {lib_reg_ok}/{n} agree, {len(lib_reg_bad)} disagree")
    for idx, ls, rs in lib_reg_bad[: args.max_report]:
        print(f"      idx {idx}: libhomfly={ls!r}  regina={rs!r}")

    n_unknot = unknot_ok + len(unknot_bad)
    print(f"  (2) KLUT-unknot => HOMFLY==1   : {unknot_ok}/{n_unknot} agree, {len(unknot_bad)} disagree")
    for idx, kk, rs in unknot_bad[: args.max_report]:
        print(f"      idx {idx}: klut={kk!r}  regina_homfly={rs!r}")

    # Diagnostic (not a failure): does each KLUT label map to a single HOMFLY?
    # If KLUT distinguishes chirality, yes; if a label maps to >1 poly, it is
    # conflating mirror/distinct knots -- worth eyeballing.
    label_polys = {}
    for idx in set(klut) & set(lib):
        label_polys.setdefault(klut[idx], set()).add(lib[idx])
    ambiguous = {k: v for k, v in label_polys.items() if len(v) > 1}
    print(f"  (diag) KLUT labels mapping to >1 distinct HOMFLY: {len(ambiguous)}")
    for k, v in list(ambiguous.items())[: args.max_report]:
        print(f"      label {k!r}: {len(v)} distinct polys")

    fail = len(lib_reg_bad) > 0 or len(unknot_bad) > 0
    print("RESULT:", "FAIL" if fail else "PASS")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
