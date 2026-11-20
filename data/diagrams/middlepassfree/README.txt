middlepassfree -- 126 unknot diagrams on which middle-strand rerouting has no move (2026-09-27)

Source.  The certificate pipeline of the middlestrands project (r1 sweep,
uniform pass / lasso, SG middle pass; middlepass --no-cascade --uniform-pass
--r1-sweep, lasso on) was run on all 2,623,203 diagrams of largehardunknots
(knotdiagramsimplification/data/diagrams/largehardunknots/hard_unknots.tsv).
2,622,984 reached 0 crossings with every applied move verified by
knoodleprove; 219 stalled.  Up to relabeling, mirror image and orientation
reversal the 219 finals are these 126 diagrams (15 crossings x16,
16 x1, 17 x48, 18 x16, 19 x23, 20 x5, 21 x4, 22 x6, 23 x4, 25 x1, 29 x2).
One representative per class, verbatim; MANIFEST.tsv gives each class's
size among the 219, the largehard indices of its members, and which of the
five stuck veryhardunknots census finals (juhasz-veryhard-86, -195, -22 =
-22_simplified, -148) it coincides with.

What is certified for each (middlestrands, diagnostics/largehard_full_2026-09-26):
  * no Reidemeister I curl;
  * no shortening uniform pass of any maximal over/under strand, maximal
    NSI2 window, or lasso;
  * no SG-feasible, face-simple, strictly shortening middle pass for ANY
    distinct-crossing strand interval (L* = n+1, route enumeration complete,
    no cap reached), both sides, k = 0 included.
Not covered: face-revisiting or W-crossing corridors, non-shortening moves,
Knoodle's cascade.  Each is thus a local minimum for that move set, on its
own component -- not a hard-unknot theorem.

Unknots.  det(-1) = 1 for all, and stock knoodlesimplify (full Reapr
pipeline, defaults) takes every one to 0 crossings on every one of 4 runs.
These are the first diagrams found on which Knoodle's re-embedding does
something middle-strand rerouting (as of 2026-09-27) cannot.

Format.  One diagram per file, tab-separated rows a b c d s: a->c is the
under strand (a incoming), the over strand runs d->b for s = +1 and b->d for
s = -1 (Knoodle / KnotTheory PD convention).  Arc labels are 0..2n-1 but
NOT necessarily consecutive along the orientation: they are as the
simplifier left them.
