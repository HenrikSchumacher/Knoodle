#!/usr/bin/env python3
"""hooks_check -- the versioned git hooks, especially the Git LFS chain.

WHY THIS EXISTS. Pointing core.hooksPath at scripts/githooks DISABLES the four
hooks git-lfs installed in .git/hooks. If scripts/githooks does not carry them
too, LFS stops working -- and it stops working SILENTLY: objects simply are not
uploaded, and you find out when someone clones and gets pointer files.

The subtle one is stdin. A pre-push hook is handed one line per ref:

    <local ref> <local sha> <remote ref> <remote sha>

`git lfs pre-push` READS that stream. If it ran first in a pipeline the policy
would see an empty stdin, conclude nothing was being pushed, and pass -- every
time, silently. The hook captures stdin once and replays it to both, and the
behavioural test below is what keeps that true: it runs the real hook with a
fake `git` on PATH and asserts BOTH consumers received the refs.

Run: python3 hooks_check.py
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
HOOKS = os.path.join(REPO, "scripts", "githooks")

# What `git lfs install` writes, and therefore what we must carry.
LFS_HOOKS = {
    "pre-push": "pre-push",
    "post-commit": "post-commit",
    "post-merge": "post-merge",
    "post-checkout": "post-checkout",
}

checks = 0
fails = 0


def check(ok, what):
    global checks, fails
    checks += 1
    if ok:
        return
    print(f"  FAILED  {what}")
    fails += 1


def section(s):
    print(f"\n=== {s} ===")


def test_lfs_chain_present():
    section("every git-lfs hook is carried")

    for name, subcommand in sorted(LFS_HOOKS.items()):
        path = os.path.join(HOOKS, name)
        check(os.path.exists(path), f"{name} exists in scripts/githooks")
        if not os.path.exists(path):
            continue
        check(os.access(path, os.X_OK), f"{name} is executable")
        body = open(path).read()
        check(f"git lfs {subcommand}" in body,
              f"{name} invokes `git lfs {subcommand}`")
        check("command -v git-lfs" in body,
              f"{name} keeps the 'git-lfs not installed' guard")


def test_stdin_is_captured_before_lfs():
    section("pre-push captures stdin before handing it to LFS")

    # Comments are stripped first: the hook's own header EXPLAINS that
    # `git lfs pre-push` reads stdin, and that explanation naturally appears
    # above the capture. Comparing raw positions matches the prose instead of
    # the code, which is how this check first failed against a correct hook.
    code = "\n".join(ln for ln in open(os.path.join(HOOKS, "pre-push"))
                      if not ln.lstrip().startswith("#"))
    cap = code.find("$(cat)")
    lfs = code.find("git lfs pre-push")
    check(cap != -1, "pre-push captures stdin into a variable")
    check(lfs != -1, "pre-push invokes `git lfs pre-push`")
    check(cap != -1 and lfs != -1 and cap < lfs,
          "stdin is captured BEFORE `git lfs pre-push` could consume it")


def test_stdin_reaches_both_consumers():
    """The behavioural version of the above, with a fake git on PATH."""
    section("both consumers actually receive the ref list")

    refs = ("refs/heads/topic aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa "
            "refs/heads/topic bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n")

    with tempfile.TemporaryDirectory() as tmp:
        seen_lfs = os.path.join(tmp, "lfs-saw")
        seen_pol = os.path.join(tmp, "policy-saw")

        # A `git` that records what `git lfs pre-push` was fed, and answers the
        # handful of queries the policy makes.
        git = os.path.join(tmp, "git")
        with open(git, "w") as f:
            f.write(f'''#!/bin/sh
if [ "$1" = "lfs" ] && [ "$2" = "pre-push" ]; then cat > "{seen_lfs}"; exit 0; fi
if [ "$1" = "rev-parse" ] && [ "$2" = "--show-toplevel" ]; then echo "{REPO}"; exit 0; fi
if [ "$1" = "rev-parse" ] && [ "$2" = "HEAD" ]; then echo cafebabe; exit 0; fi
if [ "$1" = "status" ]; then exit 0; fi
if [ "$1" = "log" ]; then exit 0; fi
exit 0
''')
        os.chmod(git, 0o755)

        # The RPM CI containers (fedora, rocky) install git but not git-lfs,
        # and the hook's first act is `command -v git-lfs || exit 2` -- which
        # exits before stdin is ever read, failing this test for the wrong
        # reason. The fixture already fakes `git`; fake `git-lfs` too (the
        # real `git lfs pre-push` call routes through the fake git anyway),
        # so the stdin plumbing is exercised on hosts without git-lfs.
        lfs_stub = os.path.join(tmp, "git-lfs")
        with open(lfs_stub, "w") as f:
            f.write("#!/bin/sh\nexit 0\n")
        os.chmod(lfs_stub, 0o755)

        # A policy stand-in that records its own stdin, so this test is about
        # the plumbing and not about the policy's decisions.
        policy = os.path.join(HOOKS, "knoodle-push-policy")
        backup = policy + ".hooks_check_backup"
        os.rename(policy, backup)
        try:
            with open(policy, "w") as f:
                f.write(f'#!/bin/sh\ncat > "{seen_pol}"\nexit 0\n')
            os.chmod(policy, 0o755)

            env = dict(os.environ, PATH=tmp + os.pathsep + os.environ["PATH"])
            p = subprocess.run([os.path.join(HOOKS, "pre-push"), "origin", "url"],
                               input=refs, capture_output=True, text=True, env=env)
            check(p.returncode == 0, f"pre-push exits 0 (got {p.returncode})")

            for who, path in (("git lfs pre-push", seen_lfs),
                              ("the push policy", seen_pol)):
                got = open(path).read() if os.path.exists(path) else ""
                check("refs/heads/topic" in got,
                      f"{who} received the ref list"
                      + ("" if got else " (received NOTHING -- stdin was consumed)"))
        finally:
            if os.path.exists(policy):
                os.remove(policy)
            os.rename(backup, policy)


def run_policy(refs, env_extra, args=("origin", "url"), cwd=REPO):
    env = dict(os.environ, **env_extra)
    env.pop("KNOODLE_PUSH_RED", None)
    if "KNOODLE_PUSH_RED" in env_extra:
        env["KNOODLE_PUSH_RED"] = env_extra["KNOODLE_PUSH_RED"]
    # Never let the policy read (or write) the real test/logs: unless a test
    # supplies its own, every run gets an empty throwaway log directory.
    with tempfile.TemporaryDirectory() as empty_logs:
        env.setdefault("KNOODLE_PUSH_LOG_DIR", empty_logs)
        p = subprocess.run([os.path.join(HOOKS, "knoodle-push-policy"), *args],
                           input=refs, capture_output=True, text=True, env=env, cwd=cwd)
    return p.returncode, p.stdout + p.stderr


def test_annotated_tag_gate():
    section("the release gate looks through an annotated tag to its commit")

    # Releases are cut with `git tag -a`, and pre-push is handed the TAG
    # OBJECT's sha, not the commit's. The stamp is keyed on the commit, so a
    # gate comparing the raw sha refused every real release even after a clean
    # heavy run. The fake shas above cannot catch this: it takes a real tag
    # object, so build one in a throwaway repository.
    git = ["git", "-c", "user.name=hooks_check", "-c", "user.email=hooks@example.invalid",
           "-c", "commit.gpgsign=false", "-c", "tag.gpgsign=false", "-c", "core.hooksPath=/dev/null"]
    with tempfile.TemporaryDirectory() as repo:
        def g(*a):
            return subprocess.run([*git, *a], cwd=repo, capture_output=True, text=True,
                                  check=True).stdout.strip()
        g("init", "-q")
        g("commit", "-q", "--allow-empty", "-m", "release")
        commit = g("rev-parse", "HEAD")
        g("tag", "-a", "v9.9.9", "-m", "v9.9.9")
        tag_object = g("rev-parse", "v9.9.9")
        check(tag_object != commit, "(setup) the annotated tag is its own object")

        tag_ref = f"refs/tags/v9.9.9 {tag_object} refs/tags/v9.9.9 {'0'*40}\n"
        green = {"KNOODLE_PUSH_TIER_RESULT": "0"}

        rc, out = run_policy(tag_ref, green, cwd=repo)
        check(rc != 0, "annotated v* tag, no heavy stamp -> refused")
        check(commit[:12] in out, "the refusal names the COMMIT, not the tag object")

        os.makedirs(os.path.join(repo, "test", ".stamps"))
        open(os.path.join(repo, "test", ".stamps", f"heavy-{commit}"), "w").close()

        rc, out = run_policy(tag_ref, green, cwd=repo)
        check(rc == 0, "annotated v* tag + heavy stamp on its commit -> allowed")
        check("heavy tier verified" in out, "the gate says the heavy tier was verified")

        light_ref = f"refs/tags/v9.9.9 {commit} refs/tags/v9.9.9 {'0'*40}\n"
        rc, out = run_policy(light_ref, green, cwd=repo)
        check(rc == 0, "lightweight v* tag + heavy stamp -> allowed")


def test_failure_names_come_from_the_run():
    section("the policy names failures from the run's own failed.txt")

    sha = "a" * 40
    main_ref = f"refs/heads/main {sha} refs/heads/main {'b'*40}\n"
    red = {"KNOODLE_PUSH_TIER_RESULT": "1"}

    with tempfile.TemporaryDirectory() as logs:
        os.makedirs(os.path.join(logs, "light"))
        with open(os.path.join(logs, "light", "failed.txt"), "w") as f:
            f.write("alpha_check\nbeta_check\n")
        rc, out = run_policy(main_ref, dict(red, KNOODLE_PUSH_LOG_DIR=logs))
        check(rc != 0, "RED + main -> refused (names from failed.txt)")
        check("alpha_check beta_check" in out,
              "the refusal names every test listed in failed.txt")

    # No failed.txt at all means run_tier.py never ran: `make check` stopped at
    # the build or the lints. Say so -- the old "(see the output above)" is how
    # a build failure used to read in a commit message.
    with tempfile.TemporaryDirectory() as logs:
        rc, out = run_policy(main_ref, dict(red, KNOODLE_PUSH_LOG_DIR=logs))
        check(rc != 0, "RED + main with no failed.txt -> refused")
        check("before any test ran" in out,
              "with no failed.txt the refusal says the build or lint failed first")

    # The names must come from the run that just happened. Re-running the tier
    # to scrape them cost another 15s and could disagree with the first run.
    code = "\n".join(ln for ln in open(os.path.join(HOOKS, "knoodle-push-policy"))
                      if not ln.lstrip().startswith("#"))
    check("run_tier.py" not in code,
          "the policy never runs run_tier.py itself (one run per push)")
    check("rm -f /tmp/knoodle-prepush" not in code and "pre-push.log" in code,
          "the policy keeps the run's transcript instead of deleting it")


def test_policy_decisions():
    section("the push policy's four cases")

    sha = "a" * 40
    main_ref = f"refs/heads/main {sha} refs/heads/main {'b'*40}\n"
    branch_ref = f"refs/heads/topic {sha} refs/heads/topic {'b'*40}\n"
    tag_ref = f"refs/tags/v9.9.9 {sha} refs/tags/v9.9.9 {'0'*40}\n"

    green = {"KNOODLE_PUSH_TIER_RESULT": "0"}
    red = {"KNOODLE_PUSH_TIER_RESULT": "1", "KNOODLE_PUSH_TIER_FAILED": "widget_check"}

    rc, out = run_policy(main_ref, green)
    check(rc == 0, "green + main -> allowed")

    rc, out = run_policy(branch_ref, green)
    check(rc == 0, "green + branch -> allowed")

    rc, out = run_policy(main_ref, red)
    check(rc != 0, "RED + main -> refused")
    check("widget_check" in out, "the refusal names the failing test")
    check("KNOODLE_PUSH_RED=1" in out, "the refusal gives the override command")
    check("refs/heads/<branch-name>" in out,
          "the refusal gives the push-to-a-branch-instead command")

    rc, out = run_policy(main_ref, dict(red, KNOODLE_PUSH_RED="1"))
    check(rc == 0, "RED + main + override -> allowed")

    # A version tag with no heavy stamp for that commit.
    rc, out = run_policy(tag_ref, green)
    check(rc != 0, "v* tag with no heavy-tier stamp -> refused")
    check("check-full" in out, "the tag refusal says how to run the heavy tier")
    # pre-push is handed <remote name> <url>; the hint used to expand "$*"
    # into `git push origin url v9.9.9`, which is not a command.
    check("KNOODLE_PUSH_RED=1 git push origin v9.9.9" in out,
          "the tag refusal's command names the remote, not the remote and its url")

    rc, out = run_policy(tag_ref, dict(green, KNOODLE_PUSH_RED="1"))
    check(rc == 0, "v* tag + override -> allowed")

    # Deleting a branch pushes an all-zero local sha and must not run anything.
    delete_ref = f"(delete) {'0'*40} refs/heads/topic {'b'*40}\n"
    rc, out = run_policy(delete_ref, red)
    check(rc == 0, "deleting a ref runs no tests and is allowed")


def main():
    print("hooks_check -- versioned hooks, the LFS chain, and the push policy")

    if not os.path.isdir(HOOKS):
        print(f"error: {HOOKS} does not exist")
        return 2

    test_lfs_chain_present()
    test_stdin_is_captured_before_lfs()
    test_stdin_reaches_both_consumers()
    test_policy_decisions()
    test_annotated_tag_gate()
    test_failure_names_come_from_the_run()

    print(f"\n{'HOOKS CHECK OK' if fails == 0 else 'HOOKS CHECK FAILED'}"
          f" ({checks} checks, {fails} failed)")
    return 0 if fails == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
