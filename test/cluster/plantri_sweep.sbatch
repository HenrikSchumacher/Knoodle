#!/bin/bash
# plantri_check array worker. One SLURM array task = one plantri res/mod shard.
# Every parameter arrives through the environment (KN_*), exported by sweep.sh;
# this file contains NO site-specific values. Submit it via `sweep.sh submit`,
# not by hand.
#
# Scheduler flags (partition/account/time/mem/array) are passed by sweep.sh on
# the sbatch command line, so they aren't hard-coded here.
#SBATCH --job-name=plantri_sweep
#SBATCH --ntasks=1

set -uo pipefail

# Load the clang/LLVM runtime modules (same ones used to build), if any.
if [ -n "${KN_MODULES:-}" ]; then
    module load ${KN_MODULES} || true
fi

RES="${SLURM_ARRAY_TASK_ID:?must run as a SLURM job array}"

# KN_ARGS is intentionally word-split (it is a list of flags we built ourselves).
# Each shard's stdout/stderr -> its own summary file; the final SUMMARY line in
# that file is what the reducer sums and what `recover` uses to tell a finished
# shard from one that needs resubmitting. Exit code is recorded but not fatal:
# a shard that finds bugs exits 1 yet is "done".
"${KN_BIN}" --plantri="${KN_PLANTRI}" --shard="${RES}/${KN_MOD}" \
    --dump-changed="${KN_OUT}/dump" ${KN_ARGS} \
    > "${KN_OUT}/summary/job_${RES}.out" 2>&1 || true
