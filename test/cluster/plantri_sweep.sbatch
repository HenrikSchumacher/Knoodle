#!/bin/bash
# SLURM array template: fan a plantri_check sweep across MOD jobs, one
# plantri res/mod shard each. Each task is independent (no communication);
# results are reassembled afterward by plantri_reduce.py over the SUMMARY lines.
#
#   sbatch --array=0-165 plantri_sweep.sbatch          # 166 shards, 9 crossings
#   UPTO=10 MOD=512 sbatch --array=0-511 plantri_sweep.sbatch
#
# After the array finishes:
#   python3 plantri_reduce.py "$OUT"/summary/job_*.out
#
#SBATCH --job-name=plantri_sweep
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --time=04:00:00
#SBATCH --mem=2G
#SBATCH --output=/dev/null            # real output goes to $OUT/summary below

set -euo pipefail

# --- knobs (override via environment) ----------------------------------------
MOD="${MOD:-166}"                      # total shards; must match --array size
UPTO="${UPTO:-9}"                      # up-to crossing number
FROM="${FROM:-3}"
MODE="${MODE:-everything}"
OUT="${OUT:-$SLURM_SUBMIT_DIR/sweep_${UPTO}cx}"   # on shared scratch
HOMFLY="${HOMFLY:-0}"                  # 1 = full per-sublink HOMFLY (much slower)
BIN="${BIN:-$SLURM_SUBMIT_DIR/plantri_check}"
# -----------------------------------------------------------------------------

RES="${SLURM_ARRAY_TASK_ID:?run me as a job array}"
mkdir -p "$OUT/dump" "$OUT/summary"

ARGS=(--plantri-mode="$MODE" --from-crossing="$FROM" --up-to-crossing="$UPTO"
      --shard="$RES/$MOD" --dump-changed="$OUT/dump")
[ "$HOMFLY" = "1" ] || ARGS+=(--no-homfly)

# Exit code: 0 clean, 1 bug found in this shard. Both are recorded; the array
# keeps running other shards regardless.
srun "$BIN" "${ARGS[@]}" > "$OUT/summary/job_${RES}.out" 2>&1 || true
