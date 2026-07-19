#!/usr/bin/env bash
#
# sweep.sh -- drive a sharded plantri_check sweep on a SLURM cluster.
#
# Site-agnostic: every cluster-specific value (partition, account, modules,
# scratch path, ...) comes from a PRIVATE config that is never committed --
# this script and the files beside it contain nothing site-specific. See
# cluster.conf.example. Run it from the cluster's login node after you have
# authenticated manually; it only ever calls local sbatch/squeue/sacct.
#
# Usage:
#   ./sweep.sh build              # module load + make plantri_check
#   ./sweep.sh submit             # sbatch the array + a dependent reduce job
#   ./sweep.sh status             # how many shards have finished
#   ./sweep.sh recover [--auto]   # resubmit shards that produced no result
#   ./sweep.sh reduce             # sum the SUMMARY lines -> PASS/FAIL
#   ./sweep.sh config             # print the resolved configuration
#   DRYRUN=1 ./sweep.sh submit    # print what submit would do, don't sbatch
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTDIR="$(cd "$HERE/.." && pwd)"
REPO="$(cd "$TESTDIR/.." && pwd)"

# ---------------------------------------------------------------------------
# Configuration: source the first config that exists, then fall back to the
# generic defaults below for anything it didn't set.
# ---------------------------------------------------------------------------
resolve_conf() {
    if [ -n "${KNOODLE_CLUSTER_CONF:-}" ];        then echo "$KNOODLE_CLUSTER_CONF"; return; fi
    if [ -f "$HOME/.config/knoodle/cluster.conf" ]; then echo "$HOME/.config/knoodle/cluster.conf"; return; fi
    if [ -f "$HERE/cluster.conf" ];               then echo "$HERE/cluster.conf"; return; fi
    echo ""
}
CONF="$(resolve_conf)"

# Refuse to run if a repo-local cluster.conf is tracked by git: it may hold
# site secrets and must never be committed.
if [ "$CONF" = "$HERE/cluster.conf" ] \
   && git -C "$HERE" ls-files --error-unmatch cluster.conf >/dev/null 2>&1; then
    echo "ERROR: $CONF is tracked by git -- it may contain site values." >&2
    echo "       Untrack it (git rm --cached cluster.conf) before running." >&2
    exit 2
fi

# Generic, site-agnostic defaults.
MODULES="" ; CXX="clang++" ; MARCH="native"
PARTITION="" ; ACCOUNT="" ; QOS="" ; TIME="04:00:00" ; MEM="2G" ; CPUS="1"
MOD="" ; FROM="3" ; UPTO="9" ; MODE="everything"
HOMFLY="0" ; KLUT="0" ; KNOTS_ONLY="0" ; CROSSING_ASSIGNMENTS="all" ; EXTRA=""
OUT="${SCRATCH:-$HOME/scratch}/knoodle_sweep"

# shellcheck disable=SC1090
[ -n "$CONF" ] && source "$CONF"

DRYRUN="${DRYRUN:-0}"
run() { if [ "$DRYRUN" = "1" ]; then echo "+ $*"; else "$@"; fi; }
load_modules() { [ -n "$MODULES" ] && module load $MODULES || true; }

# plantri_check flags built from the config.
check_args() {
    local a="--plantri-mode=$MODE --from-crossing=$FROM --up-to-crossing=$UPTO"
    a+=" --crossing-assignments=$CROSSING_ASSIGNMENTS"
    [ "$HOMFLY" = "1" ]     || a+=" --no-homfly"
    [ "$KLUT" = "1" ]       && a+=" --klut" || true
    [ "$KNOTS_ONLY" = "1" ] && a+=" --knots-only" || true
    [ -n "$EXTRA" ]         && a+=" $EXTRA" || true
    echo "$a"
}

# Scheduler flags that are set (blank => omit => site default). Populates the
# global array SCHED_FLAGS (avoids bash-4 mapfile, so this runs on old bash too).
SCHED_FLAGS=()
sched_flags() {
    SCHED_FLAGS=(--ntasks=1 --cpus-per-task="$CPUS" --time="$TIME" --mem="$MEM")
    [ -n "$PARTITION" ] && SCHED_FLAGS+=(--partition="$PARTITION") || true
    [ -n "$ACCOUNT" ]   && SCHED_FLAGS+=(--account="$ACCOUNT")     || true
    [ -n "$QOS" ]       && SCHED_FLAGS+=(--qos="$QOS")             || true
}

state_file() { echo "$OUT/.sweep_state"; }
load_state() { local s; s="$(state_file)"; [ -f "$s" ] && source "$s" \
                 || { echo "no sweep state at $s -- run 'submit' first" >&2; exit 2; }; }

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------
cmd_config() {
    echo "config file : ${CONF:-<none, using defaults>}"
    for v in MODULES CXX MARCH PARTITION ACCOUNT QOS TIME MEM CPUS MOD \
             FROM UPTO MODE HOMFLY KLUT KNOTS_ONLY CROSSING_ASSIGNMENTS EXTRA OUT; do
        printf '  %-22s = %s\n' "$v" "${!v}"
    done
    echo "  plantri_check args     = $(check_args)"
}

cmd_build() {
    # Submodules need network access (login/xfer nodes have it; compute nodes
    # often do not), so fetch them here. Then compile on a COMPUTE node -- many
    # clusters forbid compiling on the login node -- via the current allocation
    # if we are in one, else a one-shot srun. `bash -lc` sources the login
    # profile so Lmod's `module` function is available in the build shell.
    run bash -c "cd '$REPO' && git submodule update --init --recursive"
    local ml=""; [ -n "$MODULES" ] && ml="module load $MODULES && "
    local make_cmd="${ml}make -C '$TESTDIR' CXX='$CXX' MARCH='$MARCH' plantri_check"
    if [ -n "${SLURM_JOB_ID:-}" ]; then
        echo "building in the current allocation..."
        run bash -lc "$make_cmd"
    elif command -v srun >/dev/null 2>&1; then
        echo "compiling on a compute node via srun (login-node builds are disallowed)..."
        sched_flags
        run srun "${SCHED_FLAGS[@]}" bash -lc "$make_cmd"
    else
        echo "no SLURM detected; building locally..."
        run make -C "$TESTDIR" CXX="$CXX" MARCH="$MARCH" plantri_check
    fi
    echo "built $TESTDIR/plantri_check"
}

# Submit an array over the given comma-list (or 0-(MOD-1) if empty). Echoes the
# array job id.
submit_array() {
    local arr_spec="$1"
    mkdir -p "$OUT/summary" "$OUT/dump" "$OUT/log"
    sched_flags
    local export="ALL,KN_OUT=$OUT,KN_MOD=$MOD,KN_MODULES=$MODULES"
    export+=",KN_BIN=$TESTDIR/plantri_check,KN_PLANTRI=$TESTDIR/vendor/plantri/plantri"
    export+=",KN_ARGS=$(check_args)"
    if [ "$DRYRUN" = "1" ]; then
        echo "+ sbatch --parsable --job-name=plantri_sweep ${SCHED_FLAGS[*]} --array=$arr_spec" \
             "--output=$OUT/log/job_%a.slurm --export=<env> $HERE/plantri_sweep.sbatch" >&2
        echo "DRYRUN"; return
    fi
    sbatch --parsable --job-name=plantri_sweep "${SCHED_FLAGS[@]}" --array="$arr_spec" \
        --output="$OUT/log/job_%a.slurm" --export="$export" "$HERE/plantri_sweep.sbatch"
}

cmd_submit() {
    [ -n "$MOD" ] || { echo "set MOD (shard count) in your cluster.conf" >&2; exit 2; }
    local arr; arr="$(submit_array "0-$((MOD-1))")"
    echo "array job: $arr  ($MOD shards -> $OUT)"
    # Dependent reduce (runs whether or not shards failed): writes $OUT/RESULT.
    sched_flags
    local red="DRYRUN"
    if [ "$DRYRUN" = "1" ]; then
        echo "+ sbatch --dependency=afterany:$arr --wrap 'sweep.sh reduce > $OUT/RESULT'" >&2
    else
        red=$(sbatch --parsable --job-name=plantri_reduce "${SCHED_FLAGS[@]}" \
            --dependency="afterany:$arr" --output="$OUT/log/reduce.slurm" \
            --wrap "KNOODLE_CLUSTER_CONF='${CONF}' '$HERE/sweep.sh' reduce > '$OUT/RESULT' 2>&1; cat '$OUT/RESULT'")
        echo "reduce job: $red  (afterany:$arr -> $OUT/RESULT)"
    fi
    { echo "ARRAY=$arr"; echo "REDUCE=$red"; echo "MOD=$MOD"; echo "OUT=$OUT"; } > "$(state_file)" 2>/dev/null || true
}

# Shards that produced no SUMMARY line (i.e. did not finish their work).
missing_shards() {
    local r miss=()
    for r in $(seq 0 $((MOD-1))); do
        grep -q '^SUMMARY' "$OUT/summary/job_$r.out" 2>/dev/null || miss+=("$r")
    done
    printf '%s\n' "${miss[@]}"
}

cmd_status() {
    load_state
    echo "array job $ARRAY, $MOD shards, OUT=$OUT"
    squeue -j "$ARRAY" 2>/dev/null | tail -n +1 || echo "(array no longer in the queue)"
    local done; done=$(( MOD - $(missing_shards | grep -c . || true) ))
    echo "shards finished (have SUMMARY): $done / $MOD"
}

cmd_reduce() {
    load_state
    ( load_modules; python3 "$HERE/plantri_reduce.py" --dump "$OUT/dump" "$OUT"/summary/job_*.out )
}

cmd_recover() {
    load_state
    local auto=0; [ "${1:-}" = "--auto" ] && auto=1
    local miss=() r
    while IFS= read -r r; do [ -n "$r" ] && miss+=("$r"); done < <(missing_shards)
    if [ "${#miss[@]}" -eq 0 ]; then echo "no missing shards."; cmd_reduce; return; fi
    # Don't resubmit shards still in the queue.
    if squeue -j "$ARRAY" -h >/dev/null 2>&1 && [ -n "$(squeue -j "$ARRAY" -h 2>/dev/null)" ]; then
        echo "array $ARRAY still has tasks in the queue; wait for it to drain, then recover." >&2
        [ "$auto" = "0" ] && return
    fi
    local list; list="$(IFS=,; echo "${miss[*]}")"
    echo "resubmitting ${#miss[@]} missing shard(s): $list"
    local arr; arr="$(submit_array "$list")"
    echo "recovery array job: $arr"
    sed -i.bak "s/^ARRAY=.*/ARRAY=$arr/" "$(state_file)" 2>/dev/null || true
}

case "${1:-}" in
    build)   cmd_build ;;
    submit)  cmd_submit ;;
    status)  cmd_status ;;
    reduce)  cmd_reduce ;;
    recover) shift; cmd_recover "${1:-}" ;;
    config)  cmd_config ;;
    *) echo "usage: $0 {build|submit|status|recover [--auto]|reduce|config}" >&2; exit 2 ;;
esac
