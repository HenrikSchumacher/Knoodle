#!/usr/bin/env bash
# run_ivt_bench.sh — end-to-end driver for the invariant-throughput benchmark
# (KLUT vs libhomfly vs Regina) over one frozen dataset. Builds the tools,
# generates (or reuses) the dataset, runs the three drivers, then prints the
# stratified throughput report and the correctness cross-check.
#
# Configure via environment variables (all optional):
#   OUTDIR         where to write the dataset + result TSVs (default: ./ivt_out)
#   DATASET        dataset file (default: $OUTDIR/bench_set.txt; generated if absent)
#   C_MIN, C_MAX   crossing-number range for generation      (default 3 .. 13)
#   PER_CROSSING   reservoir size per crossing number         (default 20000)
#   SEED           generator seed                             (default 20260701)
#   PLANTRI_MODE   simple | no-r1 | everything                (default everything)
#   REGINA_LIMIT   run Regina on only the first N diagrams    (default 0 = all)
#   PYTHON         python interpreter with Regina             (default: venv/bin/python)
#
# See docs/klut-bench-ivt.md for the methodology.
set -euo pipefail
cd "$(dirname "$0")"

OUTDIR="${OUTDIR:-./ivt_out}"
DATASET="${DATASET:-$OUTDIR/bench_set.txt}"
C_MIN="${C_MIN:-3}"
C_MAX="${C_MAX:-13}"
PER_CROSSING="${PER_CROSSING:-20000}"
SEED="${SEED:-20260701}"
PLANTRI_MODE="${PLANTRI_MODE:-everything}"
REGINA_LIMIT="${REGINA_LIMIT:-0}"
PYTHON="${PYTHON:-venv/bin/python}"

mkdir -p "$OUTDIR"

echo "=== building tools ==="
make make_bench_set klut_ivt_bench libhomfly_ivt_bench

if [ ! -f "$DATASET" ]; then
    echo "=== generating dataset -> $DATASET ==="
    ./make_bench_set --out="$DATASET" --from-crossing="$C_MIN" --up-to-crossing="$C_MAX" \
        --per-crossing="$PER_CROSSING" --seed="$SEED" --plantri-mode="$PLANTRI_MODE"
else
    echo "=== reusing existing dataset $DATASET ==="
fi

KLUT_TSV="$OUTDIR/klut.tsv"
LIB_TSV="$OUTDIR/libhomfly.tsv"
REG_TSV="$OUTDIR/regina.tsv"

echo "=== KLUT driver ==="
./klut_ivt_bench --dataset="$DATASET" --out="$KLUT_TSV"

echo "=== libhomfly driver ==="
./libhomfly_ivt_bench --dataset="$DATASET" --out="$LIB_TSV"

REGINA_OK=0
if [ -x "$PYTHON" ] && "$PYTHON" -c "import regina" 2>/dev/null; then
    echo "=== Regina driver ==="
    reg_args=(--dataset="$DATASET" --out="$REG_TSV")
    [ "$REGINA_LIMIT" -gt 0 ] && reg_args+=(--limit="$REGINA_LIMIT")
    "$PYTHON" regina_ivt_bench.py "${reg_args[@]}"
    REGINA_OK=1
else
    echo "=== Regina driver SKIPPED (no '$PYTHON' with regina) ==="
fi

echo
echo "=== REPORT ==="
report_args=(--klut="$KLUT_TSV" --libhomfly="$LIB_TSV")
[ "$REGINA_OK" -eq 1 ] && report_args+=(--regina="$REG_TSV")
python3 bench_report.py "${report_args[@]}"

if [ "$REGINA_OK" -eq 1 ]; then
    echo
    echo "=== CROSS-CHECK ==="
    "$PYTHON" bench_xcheck.py --klut="$KLUT_TSV" --libhomfly="$LIB_TSV" --regina="$REG_TSV"
fi
