# plantri (vendored)

`plantri` generates non-isomorphic planar graphs. We use it to enumerate
planar **quadrangulations**, whose duals are 4-valent graphs — i.e. knot/link
diagram *shadows*. Vendored so the plantri-based test (`test/plantri_check.cpp`)
is self-contained and reproducible without a network download.

## Version & provenance

- **Version 5.8 — March 4, 2026** (`#define VERSION` at the top of `plantri.c`).
- Upstream: <https://users.cecs.anu.edu.au/~bdm/plantri/> (distributed as
  `plantri58.tar.gz`).
- Authors: Gunnar Brinkmann (University of Gent) and Brendan McKay (Australian
  National University); copyright jointly held by the authors.

## License

**Apache License 2.0** — see `LICENSE-2.0.txt` (the full text shipped with the
plantri distribution) and Appendix G of plantri's own `plantri-guide.txt`.
Earlier plantri releases used a more restrictive custom license; the March 2026
(5.8) release relicensed to Apache 2.0, which permits redistribution with
attribution. The copyright/author notice is preserved in `plantri.c`.

## What we vendored

Only `plantri.c` — the entire `plantri` program is a single translation unit
(`cc -o plantri plantri.c`), depending only on the C standard library (the
optional `PLUGIN` facility is off by default). The other files in the upstream
tarball build the separate `fullgen` tool and are not needed here.

**Unmodified.** `plantri.c` is byte-for-byte as shipped in plantri 5.8; we add
no patches. It is built by the `plantri` target in `test/Makefile`.

## How the test invokes it

`plantri_check` runs plantri as a subprocess, capturing the binary
`edge_code` output (the `-E` flag) on stdout. For crossing number `n` it
requests `V = n + 2` vertices:

| mode        | command              | meaning                                  |
|-------------|----------------------|------------------------------------------|
| `simple`    | `plantri -q -E V`    | simple quadrangulations (V even, ≥ 8)    |
| `no-r1`     | `plantri -Q -c2 -E V`| all quadrangulations, no length-2 cycles |
| `everything`| `plantri -Q -E V`    | all quadrangulations                     |

The dual of each quadrangulation is the knot-shadow; over/under assignments of
its crossings yield signed PD codes (see `test/plantri_check.cpp`).
