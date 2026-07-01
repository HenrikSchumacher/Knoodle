#!/usr/bin/env bash
# gen_bench_sets.sh — (re)generate the invariant-throughput benchmark datasets
# from a FIXED seed. The datasets themselves are large and never committed; this
# script is the committed, reproducible recipe for them.
#
# Writes ONE FILE PER CROSSING NUMBER: $OUTDIR/bench_set_cNN.txt, for n in
# [C_MIN, C_MAX]. Each file is a standalone dataset (its own header + count), and
# they concatenate cleanly (`cat $OUTDIR/bench_set_c*.txt`) into a single 3..13
# dataset because bench_io accepts multiple count blocks. Splitting by crossing
# number lets the slow high-n files (n=12 ~1.5 min, n=13 ~12 min: plantri emits
# ~1.4e9 quadrangulations at V=15) be generated / regenerated independently.
#
# Reproducibility: make_bench_set derives its per-crossing PRNG from
# SplitMix64(SEED + n), so every bench_set_cNN.txt depends only on (plantri
# binary, SEED, n) -- independent of the crossing range and of each other. Rerun
# with the same SEED to get byte-identical files.
#
# Configure via environment (all optional):
#   OUTDIR         output dir (default ./bench_data; gitignored)
#   SEED           master seed                         (default 20260701)
#   C_MIN, C_MAX   crossing-number range               (default 3 .. 13)
#   PER_CROSSING   knot-shadows kept per crossing no.   (default 20000)
#   RESERVOIR_MULT raw-graph reservoir = MULT*PER       (default 3)
#   PLANTRI_MODE   simple | no-r1 | everything          (default everything)
#   FORCE          1 = regenerate even if the file exists (default 0 = skip)
#
# See docs/klut-bench-ivt.md.
set -euo pipefail
cd "$(dirname "$0")"

OUTDIR="${OUTDIR:-./bench_data}"
SEED="${SEED:-20260701}"
C_MIN="${C_MIN:-3}"
C_MAX="${C_MAX:-13}"
PER_CROSSING="${PER_CROSSING:-20000}"
RESERVOIR_MULT="${RESERVOIR_MULT:-3}"
PLANTRI_MODE="${PLANTRI_MODE:-everything}"
FORCE="${FORCE:-0}"

mkdir -p "$OUTDIR"
echo "=== building make_bench_set ==="
make make_bench_set

echo "=== generating datasets (seed=$SEED, mode=$PLANTRI_MODE, per_crossing=$PER_CROSSING) ==="
for (( n = C_MIN; n <= C_MAX; n++ )); do
    printf -v out "%s/bench_set_c%02d.txt" "$OUTDIR" "$n"
    if [ "$FORCE" != "1" ] && [ -f "$out" ]; then
        echo "  n=$n: $out exists, skipping (FORCE=1 to regenerate)"
        continue
    fi
    echo "  n=$n: generating $out ..."
    ./make_bench_set --out="$out" --from-crossing="$n" --up-to-crossing="$n" \
        --seed="$SEED" --per-crossing="$PER_CROSSING" \
        --reservoir-mult="$RESERVOIR_MULT" --plantri-mode="$PLANTRI_MODE"
done

echo "=== done. datasets in $OUTDIR ==="
echo "Combined 3..13 dataset for the drivers:  cat $OUTDIR/bench_set_c*.txt > $OUTDIR/bench_set_all.txt"
