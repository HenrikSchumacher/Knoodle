#!/usr/bin/env python3
"""run_tier -- run every test in the manifest at one tier's settings.

    ./run_tier.py --tier=light     # what `make check` runs, budget 30 s
    ./run_tier.py --tier=heavy     # what `make check-full` runs, hours are fine
    ./run_tier.py --list           # show what would run, and with what arguments

THE LIGHT TIER REDUCES, IT NEVER SKIPS. Every kind=test and kind=script row runs
in both tiers; only the arguments differ. Skipping is how `all` came to build 2
of 23 binaries and 21 targets rotted unnoticed, and a tier that skips would
reintroduce exactly that, just one level up.

kind=tool and kind=bench are not run by either tier -- a tool needs arguments to
do anything and a bench measures rather than asserts -- but they ARE built by
`make all`, so they cannot rot either.

EVERY RUN IS KEPT. Each test's full stdout and stderr go to
logs/<tier>/<target>.log (overwritten by the next run of that tier), and the
names of the tests that did not pass go to logs/<tier>/failed.txt, one per line
-- empty when the tier is green. The pre-push hook reads that file instead of
scraping this script's output. A failing test's entry in the report shows the
lines where it SAYS it failed, not its last lines; see `excerpt`.
"""

import argparse
import re
import concurrent.futures
import os
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MANIFEST = os.path.join(HERE, "manifest.tsv")

RUNNABLE = {"test", "script"}

# Git exports these to hooks. A child process that inherits them and runs git
# from another directory sees the WRONG worktree: with GIT_DIR set and no
# GIT_WORK_TREE, git takes the current directory as the top of the tree. Inside
# the pre-push hook that made `git status` from test/ report every tracked file
# outside test/ as deleted, so the light tier never stamped and every push
# re-ran it. Stripped once, here, and used for this script's own git calls AND
# for every test it runs -- a test must behave the same inside the hook as out.
GIT_LOCATION_VARS = ("GIT_DIR", "GIT_WORK_TREE", "GIT_INDEX_FILE", "GIT_COMMON_DIR",
                     "GIT_OBJECT_DIRECTORY", "GIT_ALTERNATE_OBJECT_DIRECTORIES",
                     "GIT_PREFIX")
CLEAN_ENV = {k: v for k, v in os.environ.items() if k not in GIT_LOCATION_VARS}


def read_manifest(path):
    rows = []
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            target, kind, needs, source, light, heavy, work = line.rstrip("\n").split("\t")
            rows.append({
                "target": target,
                "kind": kind,
                "needs": needs,
                "source": source if source != "-" else target + ".cpp",
                "light": [] if light == "-" else light.split(),
                "heavy": [] if heavy == "-" else heavy.split(),
                "work": None if work == "-" else work,
            })
    return rows


def command_for(row, tier, base):
    args = row[tier]
    if row["kind"] == "script":
        return [sys.executable, os.path.join(base, row["source"])] + args
    return [os.path.join(base, row["target"])] + args


def describe_exit(rc):
    """`exit 1`, or `killed by SIGSEGV` -- never a bare negative number."""
    if rc is None:
        return "not built"
    if rc < 0:
        try:
            return f"killed by {signal.Signals(-rc).name}"
        except ValueError:
            return f"killed by signal {-rc}"
    return f"exit {rc}"


def show_path(path):
    """A log path the way a reader wants it: relative to the repo when inside it."""
    rel = os.path.relpath(path, REPO)
    return path if rel.startswith("..") else rel


def write_log(log_dir, res, tier, cmd):
    path = os.path.join(log_dir, res["row"]["target"] + ".log")
    with open(path, "w") as f:
        f.write(f"# target:  {res['row']['target']} ({res['row']['kind']}), {tier} tier\n")
        f.write(f"# command: {' '.join(cmd)}\n")
        f.write(f"# result:  {describe_exit(res['rc'])}, {res['secs']:.2f}s\n")
        f.write("\n==== stdout ====\n")
        f.write(res["out"] or "")
        f.write("\n==== stderr ====\n")
        f.write(res["err"] or "")
    return path


def run_one(row, tier, base, log_dir):
    cmd = command_for(row, tier, base)
    if row["kind"] == "test" and not os.path.exists(cmd[0]):
        res = {"row": row, "rc": None, "secs": 0.0, "out": "", "err": "",
               "missing": True}
    else:
        t0 = time.monotonic()
        try:
            p = subprocess.run(cmd, cwd=base, capture_output=True, text=True,
                               errors="replace", env=CLEAN_ENV)
            rc, out, err = p.returncode, p.stdout, p.stderr
        except OSError as e:
            rc, out, err = 127, "", str(e)
        res = {"row": row, "rc": rc, "secs": time.monotonic() - t0,
               "out": out, "err": err, "missing": False}
    # Written as each test finishes, so an interrupted run still leaves the
    # logs of everything that completed.
    res["log"] = write_log(log_dir, res, tier, cmd)
    return res


# What a failing test's entry shows. NOT its last lines: a test that ends with a
# summary -- or, like embedding_check, with its list of KNOWN failures -- puts
# exactly the wrong thing there. (2026-11-06: a red push showed three XFAIL
# lines and hid the four real failures printed above them.) Instead:
#
#   1. every line where the test says it failed, with up to two more-indented
#      continuation lines (the "first difference at ..." under a FAIL), capped;
#   2. the end of stderr -- where crashes, assertions and tracebacks land, and
#      which was never shown at all before;
#   3. the tail of stdout ONLY if the test printed no failure line.
#
# The word must stand alone: XFAIL (a known failure) does not match, XPASS (a
# stale known-failure marker, which does fail the run) does. Lower-case
# "0 failed" in a passing summary does not match either.
FAILURE_LINE = re.compile(
    r"(?<![A-Za-z])(?:FAIL(?:ED|URES?)?|XPASS|ERROR)(?![A-Za-z])"
    r"|\berror:|Assertion|assertion failed|terminate called|terminating"
    r"|Segmentation fault")
MAX_FAILURE_LINES = 12
TAIL_LINES = 5


def _indent(s):
    return len(s) - len(s.lstrip())


def excerpt(out, err):
    """The lines worth printing for a failed test, already dedented."""
    lines = (out or "").splitlines()
    shown, seen, markers = [], set(), 0

    for i, ln in enumerate(lines):
        if not FAILURE_LINE.search(ln):
            continue
        markers += 1
        if markers > MAX_FAILURE_LINES:
            continue
        block = [ln]
        for nxt in lines[i + 1:i + 3]:
            if nxt.strip() and _indent(nxt) > _indent(ln) and not FAILURE_LINE.search(nxt):
                block.append(nxt)
            else:
                break
        for b in block:
            if b.strip() not in seen:
                seen.add(b.strip())
                shown.append(b.rstrip())

    out_part = shown
    note = None
    if markers > MAX_FAILURE_LINES:
        note = f"... and {markers - MAX_FAILURE_LINES} more failure lines; see the log"
    if not shown:
        tail = [ln.rstrip() for ln in lines if ln.strip()][-TAIL_LINES:]
        out_part = tail
        note = "(no FAIL line in stdout; these are its last lines)" if tail else "(no stdout)"

    err_part = [ln.rstrip() for ln in (err or "").splitlines()
                if ln.strip() and ln.strip() not in seen][-TAIL_LINES:]

    def dedent(block):
        if not block:
            return block
        cut = min(_indent(b) for b in block)
        return [b[cut:] for b in block]

    result = dedent(out_part)
    if note:
        result.append(note)
    if err_part:
        result.append("stderr:")
        result.extend("  " + b for b in dedent(err_part))
    return result


STAMPS = os.path.join(HERE, ".stamps")


def tree_state():
    """(HEAD sha, uncommitted changes) for this checkout; ("", "") without git."""
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=HERE, env=CLEAN_ENV,
                             capture_output=True, text=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain", "--untracked-files=no"],
                               cwd=HERE, env=CLEAN_ENV,
                               capture_output=True, text=True).stdout.strip()
    except OSError:
        return "", ""
    return sha, dirty


def write_stamp(tier, at_start):
    """Record that `tier` passed cleanly at this exact commit.

    The pre-push hook reads these to avoid re-running a tier it already has an
    answer for, and the release gate reads the heavy one to refuse a version tag
    that has never been through it.

    Four conditions, all necessary for the stamp to mean anything:

      * the worktree must be CLEAN. A stamp for HEAD when the tree has
        uncommitted edits would certify code that was never run.
      * it must be the SAME commit, still clean, that the run STARTED at
        (`at_start`, from tree_state()). On 2026-11-06 a 12-minute heavy run
        stamped a commit made 8 minutes into it -- certifying code the run
        never saw -- because HEAD was read only at the end.
      * the run must have skipped nothing. A platform-limited subset covered
        less than the tier claims, so it cannot stand in for it.
      * it must have been the whole tier, from the real manifest -- not an
        --only subset, and not a test manifest (main() checks both).
    """
    start_sha, start_dirty = at_start
    sha, dirty = tree_state()
    if not sha or not start_sha:
        return
    if dirty or start_dirty:
        print("\n(no stamp written: the worktree has uncommitted changes)")
        return
    if sha != start_sha:
        print(f"\n(no stamp written: HEAD moved from {start_sha[:12]} to {sha[:12]} "
              f"during the run, so this run did not test {sha[:12]})")
        return
    os.makedirs(STAMPS, exist_ok=True)
    with open(os.path.join(STAMPS, f"{tier}-{sha}"), "w") as f:
        f.write(f"{tier} tier passed at {sha}\n")
    print(f"\nstamped: {tier} tier passed at {sha[:12]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tier", choices=("light", "heavy"), default="light")
    ap.add_argument("--jobs", type=int, default=4,
                    help="run this many tests concurrently (default 4). Results "
                         "are always reported in manifest order.")
    ap.add_argument("--only", default="",
                    help="comma-separated target names, for debugging one test "
                         "(never stamps: it is not the whole tier)")
    ap.add_argument("--list", action="store_true",
                    help="print what would run and exit")
    ap.add_argument("--exclude-needs", default="",
                    help="comma-separated link shapes to skip, e.g. "
                         "umfpack,homfly_umfpack,boost_umfpack on a platform "
                         "without UMFPACK/BLAS. Skipped targets are REPORTED, "
                         "so a subset run never reads as a full one.")
    ap.add_argument("--exclude", default="",
                    help="comma-separated target names to skip, for runtime "
                         "requirements the manifest cannot express (e.g. tests "
                         "needing the Git-LFS KLUT data). Also reported.")
    ap.add_argument("--budget", type=float, default=30.0,
                    help="light-tier run-time budget in seconds (default 30); "
                         "exceeding it is reported, not a failure")
    ap.add_argument("--manifest", default=MANIFEST,
                    help="manifest to run (default: the real one). Binaries and "
                         "scripts are found next to it. Only the real manifest "
                         "stamps; run_tier_check uses this for its fake suite.")
    ap.add_argument("--log-dir", default="",
                    help="where the per-test logs and failed.txt go "
                         "(default: logs/<tier> next to the manifest)")
    args = ap.parse_args()

    manifest = os.path.abspath(args.manifest)
    base = os.path.dirname(manifest)
    log_dir = os.path.abspath(args.log_dir or os.path.join(base, "logs", args.tier))

    rows = [r for r in read_manifest(manifest) if r["kind"] in RUNNABLE]

    # Exclusions are for what a PLATFORM cannot run, never for trimming the tier.
    # Every one is listed in the summary: a run that quietly covered less than it
    # claimed would be worse than one that failed outright.
    drop_needs = set(filter(None, args.exclude_needs.split(",")))
    drop_names = set(filter(None, args.exclude.split(",")))

    skipped, keep = [], []
    for r in rows:
        if r["needs"] in drop_needs:
            skipped.append((r["target"], f"needs {r['needs']}"))
        elif r["target"] in drop_names:
            skipped.append((r["target"], "excluded for this platform"))
        else:
            keep.append(r)
    rows = keep

    if args.only:
        want = set(args.only.split(","))
        rows = [r for r in rows if r["target"] in want]
        missing = want - {r["target"] for r in rows}
        if missing:
            print(f"error: not runnable targets in the manifest: {', '.join(sorted(missing))}")
            return 2

    if args.list:
        print(f"{args.tier} tier: {len(rows)} runnable targets\n")
        for r in rows:
            argv = " ".join(r[args.tier])
            print(f"  {r['target']:<30} {r['kind']:<7} {argv}")
        return 0

    # Clear the previous run's results for this tier, so a stale log or a stale
    # failed.txt can never be read as this run's. Only our own files: anything
    # else in the directory is left alone.
    os.makedirs(log_dir, exist_ok=True)
    for name in os.listdir(log_dir):
        if name.endswith(".log") or name == "failed.txt":
            os.remove(os.path.join(log_dir, name))

    print(f"run_tier: {args.tier} tier, {len(rows)} targets, {args.jobs} at a time\n")

    # What is being tested: the stamp at the end must name THIS commit, so a
    # commit made while the tier runs is not certified by it.
    at_start = tree_state()
    t0 = time.monotonic()
    results = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_one, r, args.tier, base, log_dir): r["target"]
                   for r in rows}
        for fut in concurrent.futures.as_completed(futures):
            res = fut.result()
            results[res["row"]["target"]] = res
    wall = time.monotonic() - t0

    # Report in manifest order, so the output is stable regardless of --jobs.
    failed, missing, no_work, cpu = [], [], [], 0.0
    for r in rows:
        res = results[r["target"]]
        cpu += res["secs"]
        if res["missing"]:
            print(f"  {'MISSING':<8} {r['target']:<30} not built -- run `make all`")
            missing.append(r["target"])
            continue
        ok = res["rc"] == 0
        # THE NO-OP GUARD. Exiting 0 is not enough: a test whose arguments were
        # reduced too far still exits 0 having examined nothing, and that reads
        # as coverage. The pattern must match a count of work actually done.
        if ok and r["work"] and not re.search(r["work"], res["out"]):
            ok = False
            no_work.append(r["target"])
        if not ok and r["target"] not in no_work:
            failed.append(r["target"])
        argv = " ".join(r[args.tier])
        print(f"  {'ok' if ok else 'FAIL':<8} {r['target']:<30} {res['secs']:7.2f}s"
              + (f"  {argv}" if argv else ""))

    # For the pre-push hook (and anyone else): the names, one per line, in
    # manifest order. Written on every run, so an empty file means green.
    with open(os.path.join(log_dir, "failed.txt"), "w") as f:
        for name in [r["target"] for r in rows
                     if r["target"] in failed or r["target"] in no_work
                     or r["target"] in missing]:
            f.write(name + "\n")

    print("\n" + "-" * 70)
    print(f"{len(rows) - len(failed) - len(missing) - len(no_work)} passed, "
          f"{len(failed)} failed, {len(no_work)} did no work, {len(missing)} not built"
          + (f", {len(skipped)} skipped" if skipped else ""))
    if skipped:
        print("\nSKIPPED on this platform (this run covered less than a full tier):")
        for name, why in sorted(skipped):
            print(f"  - {name:<30} {why}")
    print(f"wall {wall:.1f}s, cpu {cpu:.1f}s across {args.jobs} workers")

    if args.tier == "light" and wall > args.budget:
        print(f"\nNOTE: the light tier took {wall:.1f}s, over its {args.budget:.0f}s "
              f"budget.\n      It runs on every push, so it is the budget that keeps "
              f"it from being dodged.")

    if failed:
        print("\nfailed:")
        for name in failed:
            res = results[name]
            print(f"  - {name}: {describe_exit(res['rc'])}   "
                  f"full output: {show_path(res['log'])}")
            for ln in excerpt(res["out"], res["err"]):
                print(f"      {ln}")

    if missing:
        print("\nnot built: " + ", ".join(missing))

    if no_work:
        print("\nEXITED 0 BUT DID NO WORK -- arguments reduced too far, or the\n"
              "work counter changed shape. A test that examines nothing is worse\n"
              "than one that fails, because it reads as coverage:")
        for name in no_work:
            row = next(r for r in rows if r["target"] == name)
            print(f"  - {name}: stdout never matched /{row['work']}/   "
                  f"full output: {show_path(results[name]['log'])}")

    ok = not (failed or missing or no_work)
    if ok and not skipped and not args.only and manifest == os.path.abspath(MANIFEST):
        write_stamp(args.tier, at_start)

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
