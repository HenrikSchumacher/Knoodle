"""concurrent_checks -- run a script's test functions side by side, report in order.

WHY. cli_contract_check and lattice_input_check are a few hundred launches of
the real tools, one after another. About 190 of those are knoodleidentify, and
each spends 0.1 s loading the 12- and 13-crossing KLUT tables (with
--max-crossings=8 the same launch takes no measurable time). That was 19 of the
two scripts' 21 seconds, all of it waiting on a child process -- which threads
do perfectly well. Nothing about what is launched, or checked, changes.

HOW. `run_in_order(tests)` runs each callable on a small thread pool. While a
test runs, everything it prints is held in a buffer of its own; when all have
finished, the buffers are printed in the order the tests were listed. So the
output is byte-for-byte what a sequential run prints, and a FAILED line still
sits under its own `=== section ===`.

WHAT A TEST MUST NOT DO: share a scratch file with another test (use tempfile),
or update a module global without a lock. The scripts' `check()` counters take
`LOCK` for that reason.

An exception in a test is re-raised after the output is printed, so a crash
reads the same as before: a traceback and a nonzero exit.
"""

import concurrent.futures
import io
import sys
import threading

# For the callers' shared counters (`checks += 1` is not atomic across threads).
LOCK = threading.Lock()

# Enough to overlap the waiting; few enough that the tier runner, which runs
# several tests at once itself, does not drown a 4-core CI runner.
DEFAULT_JOBS = 4

_local = threading.local()


class _PerThreadStdout:
    """Stands in for sys.stdout: a thread with a buffer writes there instead."""

    def __init__(self, real):
        self._real = real

    def write(self, text):
        buf = getattr(_local, "buffer", None)
        return (buf if buf is not None else self._real).write(text)

    def flush(self):
        self._real.flush()

    def __getattr__(self, name):
        return getattr(self._real, name)


def _run_buffered(test):
    _local.buffer = io.StringIO()
    try:
        test()
        return _local.buffer.getvalue(), None
    except BaseException as e:      # reported, in order, by run_in_order
        return _local.buffer.getvalue(), e
    finally:
        # Pool threads are never reused across nesting levels within one call,
        # so there is no outer buffer of this thread's to put back.
        _local.buffer = None


def run_in_order(tests, jobs=DEFAULT_JOBS):
    """Run the callables in `tests` concurrently; print their output in list order."""
    # A test may itself call run_in_order (one slow test, split over its
    # fixtures). The stand-in is installed once, by the outermost call; a nested
    # call's output then lands in its caller's buffer, in order, like any print.
    outermost = not isinstance(sys.stdout, _PerThreadStdout)
    real = sys.stdout
    if outermost:
        sys.stdout = _PerThreadStdout(real)
    out = sys.stdout
    try:
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            results = list(pool.map(_run_buffered, tests))
    finally:
        if outermost:
            sys.stdout = real
    first_error = None
    for text, error in results:
        out.write(text)
        if error is not None and first_error is None:
            first_error = error
    out.flush()
    if first_error is not None:
        raise first_error
