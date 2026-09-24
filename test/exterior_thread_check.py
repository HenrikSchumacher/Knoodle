#!/usr/bin/env python3
"""knoodleprove --thread-exterior / --check-exterior: one exterior face
threaded through a move trace (docs/move-descriptor.md, "Exterior faces across
a trace").

Build: a kind=script row in test/manifest.tsv (runs the real binaries).

What is protected:

  * THE THREADER'S OUTPUT CHECKS. Every committed trace, threaded, passes
    --check-exterior with no MISMATCH; the thread really is continuous.
  * IT CHANGES ONLY `#view`. Strip the `#view` lines from input and output and
    the streams are identical; every record with a diagram gets exactly one;
    threading twice changes nothing; the drawing and proof claims of the
    threaded stream are those of the original.
  * THE CHECK CAN FAIL. test/exterior_sticky_example.trace is middlestrands'
    raw output for ts-hard-010 (--no-cascade --r1-sweep --uniform-pass) with
    its own "sticky" `#view` lines, which follow an ARC rather than a face and
    carry darc numbers across a relabelling. Checked, it fails three ways:
    a jump, a cut exterior with no `outside=`, and an unflagged `behind`.
    Tampering with a good thread (flip `outside=`, add or drop `behind`,
    misspell a token) must also fail; declaring `seam` must not.
  * THE RENDERER HONOURS `outside=`. Where the corridor cuts the drawn
    exterior, knoodledraw routes it round the drawing whichever way leaves side
    `outside` unbounded -- for both values, on every cut record of the threaded
    sticky trace, and the two pictures differ. knoodledraw reads the side back
    off its own picture and warns when it cannot draw the one asked for, so no
    warning means the picture was checked. Where the corridor does not cut the
    exterior, a false `outside=` gets that warning instead.
  * HONEST GAPS. A corridor that crosses W (Proposition C') still falls back
    to a declared seam, with the reason on stderr.
"""

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PROVE = os.path.join(HERE, "..", "tools", "knoodleprove")
DRAW = os.path.join(HERE, "..", "tools", "knoodledraw")

FIXTURES = {
    "sticky":      os.path.join(HERE, "exterior_sticky_example.trace"),
    "pass_wcross": os.path.join(HERE, "pass_wcross_example.trace"),
    "healed_curl": os.path.join(HERE, "middlepass_healed_curl.trace"),
    "r1_trace":    os.path.join(HERE, "r1_trace_example.txt"),
    "trace":       os.path.join(HERE, "trace_example.txt"),
    "link_result": os.path.join(HERE, "link_result_example.trace"),
}

checks = 0
fails = 0


def check(condQ, label, detail=""):
    global checks, fails
    checks += 1
    if not condQ:
        fails += 1
        print(f"FAIL: {label}")
        if detail:
            for line in str(detail).rstrip("\n").split("\n")[:12]:
                print(f"      {line}")
    return condQ


def run(args, stdin_text=None, binary=PROVE):
    p = subprocess.run([binary] + args, input=stdin_text,
                       capture_output=True, text=True, timeout=300)
    return p.returncode, p.stdout, p.stderr


def read(path):
    with open(path) as f:
        return f.read()


def strip_views(text):
    return [ln for ln in text.split("\n") if not ln.startswith("#view ")]


def exterior_lines(text):
    return [ln for ln in text.split("\n") if " exterior: " in ln]


def views(text):
    return [ln for ln in text.split("\n") if ln.startswith("#view ")]


threaded = {}

# -- every fixture threads, and the thread checks ---------------------------

for name, path in FIXTURES.items():
    original = read(path)
    rc, out, err = run(["--thread-exterior", path])
    check(rc == 0, f"{name}: --thread-exterior exits 0", err)
    threaded[name] = out

    check(strip_views(out) == strip_views(original),
          f"{name}: threading changes nothing but `#view` lines")

    n_state = len(re.findall(r"^#step ", original, re.M))
    check(len(views(out)) == n_state,
          f"{name}: one `#view` per record ({n_state})",
          "\n".join(views(out)))

    rc2, out2, _ = run(["--thread-exterior"], out)
    check(rc2 == 0 and out2 == out, f"{name}: threading is idempotent")

    rc, chk, _ = run(["--check-exterior"], out)
    bad = [ln for ln in exterior_lines(chk) if "MISMATCH" in ln]
    check(rc == 0 and not bad, f"{name}: the threaded stream checks clean",
          "\n".join(bad) or chk)

    # The other claims are untouched by the thread.
    _, plain_before, _ = run([path])
    _, plain_after, _ = run([], out)
    check(plain_before == plain_after,
          f"{name}: knoodleprove's own claims are unchanged by threading")
    check(not exterior_lines(plain_after),
          f"{name}: no `exterior:` claims without --check-exterior")

# Continuity is actually exercised, not just vacuously passed.
_, chk, _ = run(["--check-exterior"], threaded["sticky"])
verified = [ln for ln in exterior_lines(chk) if "-> face" in ln]
check(len(verified) >= 3, "sticky: at least 3 continuity claims VERIFIED", chk)
cut = [ln for ln in exterior_lines(chk) if "corridor cuts it" in ln]
check(len(cut) >= 1, "sticky: a cut exterior is threaded (outside= binds)", chk)

_, chk, _ = run(["--check-exterior"], threaded["r1_trace"])
check(any("exterior: VERIFIED (face" in ln for ln in exterior_lines(chk)),
      "r1_trace: an r1's face map carries the thread", chk)

# Drawings of a threaded stream still verify.
rc, dout, derr = run(["--trace", "--verify", "--ascii"], threaded["sticky"],
                     binary=DRAW)
dv = [ln for ln in (dout + derr).split("\n") if ln.startswith("#verify")]
check(rc == 0 and dv and all("VERIFIED" in ln for ln in dv),
      "sticky: knoodledraw --verify accepts the threaded stream", derr)

# -- the check can fail ------------------------------------------------------

rc, chk, _ = run(["--check-exterior", FIXTURES["sticky"]])
ext = "\n".join(exterior_lines(chk))
check(rc == 1, "sticky views: --check-exterior exits 1", chk)
check("the next record's exterior is face" in ext,
      "sticky views: a jump across a relabelling is caught", ext)
check("must say outside=0|1" in ext,
      "sticky views: a cut exterior without outside= is caught", ext)
check("'behind' is missing" in ext,
      "sticky views: an exterior on the swept side needs `behind`", ext)


def tamper(text, pattern, repl, count=1):
    new, n = re.subn(pattern, repl, text, count=count, flags=re.M)
    return new, n


good = threaded["sticky"]

# Flip outside= on the record whose exterior the corridor cuts.
_, chk, _ = run(["--check-exterior"], good)
cut_steps = [int(m.group(1)) for m in
             re.finditer(r"#verify step (\d+) exterior: VERIFIED[^\n]*corridor cuts it", chk)]
check(bool(cut_steps), "sticky: found a cut step to tamper with", chk)
if cut_steps:
    step = cut_steps[0]
    blocks = good.split("#step n=")
    idx = next(i for i, b in enumerate(blocks) if b.startswith(f"{step} "))
    b = blocks[idx]
    b2 = re.sub(r"outside=(\d)", lambda m: "outside=" + str(1 - int(m.group(1))), b, count=1)
    bad_stream = "#step n=".join(blocks[:idx] + [b2] + blocks[idx + 1:])
    rc, chk2, _ = run(["--check-exterior"], bad_stream)
    check(rc == 1 and f"#verify step {step} exterior: MISMATCH" in chk2,
          "flipping outside= on a cut exterior breaks continuity", chk2)

bad_stream, n = tamper(good, r"^(#view exterior=\d+ outside=\d)$", r"\1 behind")
check(n == 1, "found a view to add `behind` to")
rc, chk2, _ = run(["--check-exterior"], bad_stream)
check(rc == 1 and "'behind' is set" in chk2, "a false `behind` is caught", chk2)

bad_stream, n = tamper(good, r"^(#view exterior=\d+ outside=\d)$", r"\1 behnd")
rc, chk2, _ = run(["--check-exterior"], bad_stream)
check(rc == 1 and "unknown '#view' token 'behnd'" in chk2,
      "a misspelt `#view` token is a MISMATCH, not silently ignored", chk2)

# Declaring a seam on the second record withdraws the continuity claim into it.
lines = good.split("\n")
vidx = [i for i, ln in enumerate(lines) if ln.startswith("#view ")]
lines[vidx[1]] = re.sub(r"exterior=\d+", "exterior=0", lines[vidx[1]]) + " seam"
rc, chk2, _ = run(["--check-exterior"], "\n".join(lines))
check("#verify step 0 exterior: UNCHECKED (the next record declares a seam)" in chk2,
      "a declared seam withdraws the continuity claim", chk2)

# -- the renderer honours outside= -------------------------------------------

_, chk, _ = run(["--check-exterior"], good)
# Only a continuity claim says whether the corridor cuts the exterior; the
# last record and candidates say neither, and only their true outside= is
# asked to draw quietly.
cut_set = {int(m.group(1)) for m in
           re.finditer(r"#verify step (\d+) exterior: VERIFIED[^\n]*corridor cuts it", chk)}
uncut_set = {int(m.group(1)) for m in
             re.finditer(r"#verify step (\d+) exterior: VERIFIED \(face[^\n]*outside=\d\)", chk)}
blocks = good.split("#step n=")
head, recs = blocks[0], blocks[1:]
n_cut = n_uncut = 0

for rec in recs:
    step = int(rec.split(" ", 1)[0])
    if not re.search(r"^#view .*outside=\d", rec, re.M):
        continue
    flipped = re.sub(r"outside=(\d)",
                     lambda m: "outside=" + str(1 - int(m.group(1))), rec, count=1)
    pics = []
    for label, r in (("as threaded", rec), ("flipped", flipped)):
        rc, dout, derr = run(["--trace", "--verify", "--ascii"],
                             head + "#step n=" + r, binary=DRAW)
        warnQ = "cannot draw outside=" in derr
        dv = [ln for ln in dout.split("\n") if ln.startswith("#verify")]
        check(rc == 0 and dv and all("VERIFIED" in ln for ln in dv),
              f"step {step} ({label}): the drawing verifies", dout + derr)
        if step in cut_set:
            check(not warnQ, f"step {step} ({label}): cut exterior, "
                  "the corridor goes round the way outside= asks", derr)
        elif label == "flipped":
            if step not in uncut_set:
                continue
            check(warnQ and "does not run through the drawn exterior" in derr,
                  f"step {step}: a false outside= on an uncut exterior is"
                  " reported, not drawn", derr)
        else:
            check(not warnQ, f"step {step}: a true outside= draws quietly", derr)
        pics.append(dout)
    if step in cut_set:
        n_cut += 1
        check(len(pics) == 2 and pics[0] != pics[1],
              f"step {step}: flipping outside= changes the corridor")
    elif step in uncut_set:
        n_uncut += 1

check(n_cut >= 2 and n_uncut >= 1,
      f"outside= rendering exercised ({n_cut} cut, {n_uncut} uncut records)")

# -- honest gaps -------------------------------------------------------------

rc, out, err = run(["--thread-exterior", FIXTURES["pass_wcross"]])
check(any(v.endswith(" seam") for v in views(out)),
      "pass_wcross: the corridor-crosses-W move falls back to a declared seam",
      "\n".join(views(out)))
check("Proposition C'" in err, "pass_wcross: stderr says why", err)

print(f"exterior_thread_check: ({checks} checks, {fails} failed)")
sys.exit(1 if fails else 0)
