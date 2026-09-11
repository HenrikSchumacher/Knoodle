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
"""

import argparse
import re
import concurrent.futures
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HERE, "manifest.tsv")

RUNNABLE = {"test", "script"}


def read_manifest():
    rows = []
    with open(MANIFEST) as f:
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


def command_for(row, tier):
    args = row[tier]
    if row["kind"] == "script":
        return [sys.executable, os.path.join(HERE, row["source"])] + args
    return [os.path.join(HERE, row["target"])] + args


def run_one(row, tier):
    cmd = command_for(row, tier)
    if row["kind"] == "test" and not os.path.exists(cmd[0]):
        return {"row": row, "rc": None, "secs": 0.0, "out": "", "err": "",
                "missing": True}
    t0 = time.monotonic()
    try:
        p = subprocess.run(cmd, cwd=HERE, capture_output=True, text=True)
        rc, out, err = p.returncode, p.stdout, p.stderr
    except OSError as e:
        rc, out, err = 127, "", str(e)
    return {"row": row, "rc": rc, "secs": time.monotonic() - t0,
            "out": out, "err": err, "missing": False}


STAMPS = os.path.join(HERE, ".stamps")


def write_stamp(tier):
    """Record that `tier` passed cleanly at this exact commit.

    The pre-push hook reads these to avoid re-running a tier it already has an
    answer for, and the release gate reads the heavy one to refuse a version tag
    that has never been through it.

    Two conditions, both necessary for the stamp to mean anything:

      * the worktree must be CLEAN. A stamp for HEAD when the tree has
        uncommitted edits would certify code that was never run.
      * the run must have skipped nothing. A platform-limited subset covered
        less than the tier claims, so it cannot stand in for it.
    """
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=HERE,
                             capture_output=True, text=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain", "--untracked-files=no"],
                               cwd=HERE, capture_output=True, text=True).stdout.strip()
    except OSError:
        return
    if not sha or dirty:
        if dirty:
            print("\n(no stamp written: the worktree has uncommitted changes)")
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
                    help="comma-separated target names, for debugging one test")
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
    args = ap.parse_args()

    rows = [r for r in read_manifest() if r["kind"] in RUNNABLE]

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

    print(f"run_tier: {args.tier} tier, {len(rows)} targets, {args.jobs} at a time\n")

    t0 = time.monotonic()
    results = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_one, r, args.tier): r["target"] for r in rows}
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
            print(f"  - {name} (exit {res['rc']})")
            tail = [ln for ln in (res["out"] or "").splitlines() if ln.strip()][-3:]
            for ln in tail:
                print(f"      {ln}")

    if missing:
        print("\nnot built: " + ", ".join(missing))

    if no_work:
        print("\nEXITED 0 BUT DID NO WORK -- arguments reduced too far, or the\n"
              "work counter changed shape. A test that examines nothing is worse\n"
              "than one that fails, because it reads as coverage:")
        for name in no_work:
            row = next(r for r in rows if r["target"] == name)
            print(f"  - {name}: stdout never matched /{row['work']}/")

    ok = not (failed or missing or no_work)
    if ok and not skipped:
        write_stamp(args.tier)

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
