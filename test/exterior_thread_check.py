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
    exterior, a false `outside=` is a hard stop (nonzero exit): it is a bug
    in whatever wrote the `#view` line. The threader cannot write one: it
    runs its own output through --check-exterior's checker and emits nothing
    if that fails -- passing on every fixture, and refusing when the test hook
    KNOODLEPROVE_TEST_CORRUPT_VIEW corrupts a view (a false outside=, the
    wrong part of a cut exterior, a false `behind`).
  * C' AND LASSOS. A corridor that crosses W (Proposition C') threads with
    no seam: its two sides are the checkerboard classes of the complement of
    W* + corridor (middlestrands ROUND-24 §6(b)), and knoodledraw honours
    outside= on it, cut and uncut, as on any other move. So does a lasso.
  * HONEST GAPS. A move that frees a crossingless loop is UNCHECKED.
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
    "fhw":         os.path.join(HERE, "fhw_unlink.trace"),
    "lasso_corridor": os.path.join(HERE, "pass_lasso_corridor.trace"),
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


def run(args, stdin_text=None, binary=PROVE, env=None):
    full_env = None
    if env:
        full_env = dict(os.environ)
        full_env.update(env)
    p = subprocess.run([binary] + args, input=stdin_text, env=full_env,
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

def crosses_w(rec):
    """Does this record's pass corridor cross its own strand (C')?"""
    m = re.search(r"^#move kind=pass strand=([\d,]+) \S+ cross=(\S*)", rec, re.M)
    if not m or not m.group(2):
        return False
    w = {int(d) // 2 for d in m.group(1).split(",")}
    return any(int(x.split(":")[0]) // 2 in w for x in m.group(2).split(","))


def renderer_honours(good, name):
    """Draw every record of a threaded stream as threaded and with outside=
    flipped. Returns (cut, uncut, C'-cut, C'-uncut) record counts."""
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
    n_cut = n_uncut = c_cut = c_uncut = 0

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
            if label == "flipped" and step not in cut_set:
                if step in uncut_set:
                    check(rc != 0 and "is a false claim" in derr,
                          f"{name} step {step}: a false outside= on an uncut exterior"
                          " is a hard stop", derr)
                continue
            dv = [ln for ln in dout.split("\n") if ln.startswith("#verify")]
            check(rc == 0 and dv and all("VERIFIED" in ln for ln in dv),
                  f"{name} step {step} ({label}): the drawing verifies", dout + derr)
            if step in cut_set:
                check(not warnQ, f"{name} step {step} ({label}): cut exterior, "
                      "the corridor goes round the way outside= asks", derr)
            else:
                check(not warnQ, f"{name} step {step}: a true outside= draws quietly", derr)
            pics.append(dout)
        if step in cut_set:
            n_cut += 1
            c_cut += crosses_w(rec)
            check(len(pics) == 2 and pics[0] != pics[1],
                  f"{name} step {step}: flipping outside= changes the corridor")
        elif step in uncut_set:
            n_uncut += 1
            c_uncut += crosses_w(rec)

    return n_cut, n_uncut, c_cut, c_uncut


n_cut, n_uncut, _, _ = renderer_honours(good, "sticky")
check(n_cut >= 2 and n_uncut >= 1,
      f"outside= rendering exercised ({n_cut} cut, {n_uncut} uncut records)")

# The same on corridors that cross W (Proposition C'), whose loop has double
# points: its sides are the checkerboard classes (ROUND-24 §6(b)), and
# knoodledraw reads them off the picture by ray parity. Both kinds of record
# must be covered: a cut exterior (the corridor must go round the way
# outside= asks) and an uncut one (a flipped outside= is a hard stop).
cc = cu = 0
for name in ("pass_wcross", "fhw", "lasso_corridor"):
    rc, threaded, err = run(["--thread-exterior", FIXTURES[name]])
    check(rc == 0 and not any(v.endswith(" seam") for v in views(threaded)),
          f"{name}: threads with no seam (C' and lasso moves have two sides)",
          err + "\n".join(views(threaded)))
    _, _, x_c, x_u = renderer_honours(threaded, name)
    cc += x_c
    cu += x_u
check(cc >= 1 and cu >= 1,
      f"C' corridors exercised on both kinds of exterior ({cc} cut, {cu} uncut)")

# -- the threader refuses to emit a claim its own checker rejects -------------
#
# KNOODLEPROVE_TEST_CORRUPT_VIEW corrupts one view after threading and before
# the self-check, so the refusal path runs end to end: MISMATCH lines on
# stderr, NOTHING on stdout, exit 1. Record indices count every record.

sticky = read(FIXTURES["sticky"])
corruptions = [
    # record 0: the exterior lies wholly on one side -- a false outside=
    ("0:outside", "lies wholly on side",  "a false outside= on an uncut exterior"),
    # record 1: the corridor cuts it -- the other part is not the next exterior
    ("1:outside", "the next record's exterior is face", "the wrong part of a cut exterior"),
    # record 0: a uniform/unswept move is never behind
    ("0:behind",  "'behind' is set",      "a false `behind`"),
]
for spec, needle, what in corruptions:
    rc, out, err = run(["--thread-exterior"], sticky,
                       env={"KNOODLEPROVE_TEST_CORRUPT_VIEW": spec})
    check(rc == 1 and out == "" and "fails its own check" in err and needle in err,
          f"threader self-check refuses {what} ({spec}): exit 1, nothing emitted",
          f"rc={rc}, {len(out)} bytes on stdout\n{err}")

rc, out, err = run(["--thread-exterior"], sticky,
                   env={"KNOODLEPROVE_TEST_CORRUPT_VIEW": "99:outside"})
check(rc != 0 and out == "" and "names no view" in err,
      "the test hook refuses a spec it cannot apply", err)

# -- honest gaps -------------------------------------------------------------

# A move that frees a crossingless loop still has no face correspondence: the
# freed component has no face to go to. FHW's last r1 frees its last loop.
rc, out, err = run(["--check-exterior"], run(["--thread-exterior", FIXTURES["fhw"]])[1])
check("exterior: UNCHECKED (the curl's whole component comes free)" in out,
      "fhw: the r1 that frees the last loop is UNCHECKED, and says why", out)

print(f"exterior_thread_check: ({checks} checks, {fails} failed)")
sys.exit(1 if fails else 0)
