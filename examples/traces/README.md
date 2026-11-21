# Example traces

A *trace* is the record a simplifier writes of what it did to a diagram: the
diagram it started from and, for each move, the move and the diagram it left.
The format is in `docs/move-descriptor.md`.

Two tools read a trace, and every file here goes through both:

- **`knoodleprove`** checks it. Each claim in the trace gets a `#verify` line,
  and the exit status is 0 only if nothing was found wrong.
- **the animator** (`src/Animate/`, driven here by `knoodlemovie`) draws it:
  every move becomes a piece of animation.

| File | What it shows | Moves | Animation |
|---|---|---|---|
| `pass_lasso.trace` | The smallest whole story: one pass move on a trefoil-shaped unknot, then one curl removed. | 1 pass, 1 r1 | 13 s |
| `fhw_unlink.trace` | The Freedman–He–Wang hard unknot (32 crossings) taken all the way to nothing. One pass needs the outside face changed first, so the sphere is turned. | 11 pass, 4 r1 | 108 s |
| `middlepass_unlink.trace` | Middle passes: the strand goes over some of what it crosses and under the rest, with the witness that makes that sound. | 4 middlepass, 8 r1 | 65 s |
| `redraw_example.trace` | A redraw: the same curve in space, seen from another side. | 1 redraw | 11 s |
| `redraw_link_example.trace` | A redraw of a two-component link (the Hopf link), with the components' colours kept. | 1 redraw | 25 s |

These are copies of files in `test/`, where they are the fixtures of the
tests; `animate_build_check` fails if a copy differs from its original.

## Running them

From the top of the repository:

```bash
make -C tools knoodleprove knoodlemovie
```

Check a trace:

```bash
tools/knoodleprove examples/traces/pass_lasso.trace
```

The last line should read `#verify stream unlink: VERIFIED (...)`: from this
trace alone, the input is an unknot.

Animate it, as a file for the web player:

```bash
tools/knoodlemovie examples/traces/pass_lasso.trace --json=pass_lasso.json
```

Open `players/knoodle-player.html` in a browser and drop `pass_lasso.json` on
it (or use its Open button). The player is one file with no dependencies.

Or as a movie (this needs `ffmpeg`):

```bash
tools/knoodlemovie examples/traces/pass_lasso.trace --size=1280x720 --fps=30 | ffmpeg -f rawvideo -pix_fmt rgb24 -s 1280x720 -r 30 -i - -pix_fmt yuv420p pass_lasso.mp4
```

Or one frame, as a PPM picture, at a time in seconds:

```bash
tools/knoodlemovie examples/traces/pass_lasso.trace --frame=4.0 > frame.ppm
```

`--style=talk` (the default), `light` or `dark` picks the colours.

## Where traces come from

Nothing in this repository writes a trace yet. These were recorded by the
simplifier of the *middlestrands* project, which is built on Knoodle; the
`#comment` lines at the top of each file say how.
