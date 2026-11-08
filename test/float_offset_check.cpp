// float_offset_check -- float input placed away from the origin (GH #50).
//
// `LinkEmbedding<double,...>` rounds float coordinates onto an Int64 grid in
// `ComputeEdgeCoordinates`. It scales by a power of two chosen from
// `MaxModulus()` and does NOT translate first; the class comment in
// src/LinkEmbedding_Int.hpp says that is deliberate (it avoids cancellation).
// So the cast to Int64 is safe only if `MaxModulus()` bounds every |coordinate|.
// Since 158f580c it returns the extent of the bounding box,
// `(global_hi - global_lo).MaxNorm()`. For a curve that sits further from the
// origin than its own size, that is too small: the scale comes out too big,
// the cast overflows, and the curve comes back either "self-intersecting"
// (error 3) or as the assertion in Prosector's `LinesColinearTest`.
// `RoundingError()` reports 0 either way, since it measures the rounding in
// double, not the cast.
//
//   https://github.com/HenrikSchumacher/Knoodle/issues/50  (filed 2026-11-07)
//   docs/upstream-issues.md, issue 17
//
// src/ is Henrik's, and so is the fix: the issue gives two readings of what
// `MaxModulus()` is meant to be. This test does not care which one he picks.
// It checks only the answer, which is the same for every translation.
//
// THE CASES. test/embeddings/arc_trefoil.crd: one component, 15 vertices, 8
// crossings in the z-projection, bounding box x in [-2,1], y in [-2,2],
// z in [0,4]. Each case translates it and requires 8 crossings, in both Morton
// modes. Every coordinate is a small integer and every shift is a multiple of
// 1/4, so the translated coordinates are exact in double and the projection is
// the same curve. 8 is the right answer every time.
//
//   double, as given                -- control. Scaled by 2^58 on main, and safe
//                                      because the curve sits at the origin.
//                                      Shows the scaling path itself works.
//   double, +0.25 on all axes       -- control. The same, with non-integral
//                                      values. On main every double input is
//                                      scaled (`input_integralQ` is just
//                                      `IntQ<Real>`), so this duplicates the
//                                      case above today. It is here so that a
//                                      scaled near-origin control survives if
//                                      integral-valued doubles ever skip the
//                                      scaling, as the `scaleQ` test in
//                                      ComputeEdgeCoordinates allows.
//   double, +/-1000.25 on all axes  -- failed on main 09aa3339: error 3
//   double, +/-1000.25 in x only    -- failed on main 09aa3339: assertion abort
//                                      (x only, -1000.25 also failed before
//                                      158f580c). Fixed by reading 1 of #50.
//   Int64,  as given / +/-1000      -- control. Integral Real is never scaled,
//                                      so the same translations must pass. This
//                                      shows the failures come from the float
//                                      scaling, not from where the curve sits.
//
// A failing control is a real failure, not an XFAIL. Each line also says
// whether the coordinates were scaled (`ScalingFactor()`), not the exponent:
// `ScalingExponent()` is computed even when it is not applied, so it reads 58
// on every case here, scaled or not.
//
// KNOWN FAILURES are reported as XFAIL. A known failure that starts passing is
// reported as XPASS and fails the run: that is the signal that #50 is fixed.
// Then set kIssue50Open to false and keep the cases, which become the
// regression test for the fix.
//
// EACH CASE RUNS IN A FORKED CHILD. Half the failures are an assert() inside
// src/, which no try/catch can stop. The child installs handlers for SIGABRT
// and the other crash signals that call _exit, so the parent records the
// failure and carries on. A process that exits this way also does not file a
// macOS crash report, which a process killed by SIGABRT does (see
// docs/test-architecture.md, section 7a). The child's own stdout and stderr go
// into a pipe; under the case the parent prints the assertion if there was
// one, otherwise the first line.
//
// Build: `make float_offset_check` in test/. Run from test/ (it reads
// embeddings/arc_trefoil.crd); takes well under a second.

#include "embedding_fixtures.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <string>
#include <vector>

#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace kt = knoodle_test;

using Int = std::int64_t;

/// GH #50 was open on main 09aa3339, where every +/-1000.25 double case below
/// failed. It is fixed, so those cases must now pass like the controls.
static constexpr bool kIssue50Open = false;

static constexpr Int  kExpectedCrossings = 8;
static constexpr int  kCaughtSignalBase  = 100;  // child exit code = 100 + signal
static constexpr int  kChildTimeoutSecs  = 20;

static int  checks = 0;
static int  failed = 0;
static int  xfail  = 0;

/// What the child reports back, through a page shared across the fork.
struct Result
{
    int       done              = 0;    ///< the child got as far as writing this
    int       err               = 0;    ///< RequireIntersections() return value
    int       scaledQ           = 0;    ///< ScalingFactor() != 1
    int       scaling_exponent  = 0;    ///< meaningful only when scaledQ
    long long crossings         = -1;
};

extern "C" void OnCrashSignal( int sig ) { _exit(kCaughtSignalBase + sig); }

static const char * SignalName( int sig )
{
    switch( sig )
    {
        case SIGABRT: return "SIGABRT";
        case SIGSEGV: return "SIGSEGV";
        case SIGBUS:  return "SIGBUS";
        case SIGFPE:  return "SIGFPE";
        case SIGILL:  return "SIGILL";
        case SIGALRM: return "SIGALRM";
        default:      return "signal";
    }
}

template<typename Real, bool mortonQ>
static void RunInChild( const std::vector<Real> & coords, Result * out )
{
    using LE_T = Knoodle::LinkEmbedding<Real,Int,Int,mortonQ>;

    LE_T L ( Int(coords.size() / 3) );
    L.ReadVertexCoordinates( coords.data() );

    const int err = L.RequireIntersections();

    out->err              = err;
    out->scaledQ          = (static_cast<double>(L.ScalingFactor()) != 1.0);
    out->scaling_exponent = L.ScalingExponent();
    out->crossings        = err ? -1 : (long long)L.IntersectionCount();
    out->done             = 1;
}

/// One line of what the child printed: the assertion if there was one (it is
/// the line that names the failure), otherwise the first line.
static std::string SaidLine( const std::string & s )
{
    std::size_t a = s.find("Assertion failed");
    if( a == std::string::npos ) { a = s.find_first_not_of(" \t\r\n"); }
    if( a == std::string::npos ) { return {}; }
    std::size_t b = s.find('\n', a);
    std::string line = s.substr(a, b == std::string::npos ? std::string::npos : b - a);
    if( line.size() > 120 ) { line = line.substr(0,117) + "..."; }
    return line;
}

/// Run one case in a forked child. Returns true if it gave the right answer;
/// `what` says what happened, `said` is the child's first line of output.
template<typename Real, bool mortonQ>
static bool RunCase( const std::vector<Real> & coords, std::string & what, std::string & said )
{
    auto * res = static_cast<Result *>( mmap( nullptr, sizeof(Result),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0 ) );
    if( res == MAP_FAILED ) { what = "mmap failed"; return false; }
    *res = Result{};

    int fds[2];
    if( pipe(fds) != 0 ) { what = "pipe failed"; munmap(res,sizeof(Result)); return false; }

    std::fflush(stdout);
    std::fflush(stderr);

    const pid_t pid = fork();
    if( pid < 0 )
    {
        what = "fork failed";
        close(fds[0]); close(fds[1]); munmap(res,sizeof(Result));
        return false;
    }
    if( pid == 0 )
    {
        for( int sig : { SIGABRT, SIGSEGV, SIGBUS, SIGFPE, SIGILL } )
        {
            std::signal(sig, OnCrashSignal);
        }
        alarm(kChildTimeoutSecs);   // default action: terminate, no crash report
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[1]);
        RunInChild<Real,mortonQ>(coords, res);
        std::fflush(stdout);
        std::fflush(stderr);
        _exit(0);
    }

    close(fds[1]);
    std::string output;
    char buf[4096];
    for( ssize_t n; (n = read(fds[0], buf, sizeof buf)) != 0; )
    {
        if( n < 0 ) { if( errno == EINTR ) { continue; } break; }
        output.append(buf, std::size_t(n));
    }
    close(fds[0]);

    int status = 0;
    while( waitpid(pid, &status, 0) < 0 && errno == EINTR ) {}

    const Result r = *res;
    munmap(res, sizeof(Result));
    said = SaidLine(output);

    if( WIFSIGNALED(status) )
    {
        const int sig = WTERMSIG(status);
        what = (sig == SIGALRM)
             ? "timed out after " + std::to_string(kChildTimeoutSecs) + " s"
             : std::string("killed by ") + SignalName(sig);
        return false;
    }
    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if( code > kCaughtSignalBase )
    {
        const int sig = code - kCaughtSignalBase;
        what = (sig == SIGABRT) ? "assertion abort (SIGABRT caught)"
                                : std::string(SignalName(sig)) + " caught";
        return false;
    }
    if( code != 0 || !r.done )
    {
        what = "child exited " + std::to_string(code) + " without a result";
        return false;
    }
    const std::string scaling = r.scaledQ
        ? " (scaled by 2^" + std::to_string(r.scaling_exponent) + ")"
        : std::string(" (unscaled)");
    if( r.err != 0 )
    {
        what = "error " + std::to_string(r.err) + scaling;
        return false;
    }

    what = std::to_string(r.crossings) + " crossings" + scaling;
    if( r.crossings != kExpectedCrossings )
    {
        what += ", expected " + std::to_string(kExpectedCrossings);
        return false;
    }
    return true;
}

static void Report( bool passedQ, const std::string & label, const std::string & what,
                    const std::string & said, bool known_broken )
{
    ++checks;
    const char * verdict = known_broken ? (passedQ ? "XPASS" : "XFAIL")
                                        : (passedQ ? "OK"    : "FAILED");
    std::printf("  %-34s %-48s %s\n", label.c_str(), what.c_str(), verdict);
    if( !said.empty() ) { std::printf("        said: %s\n", said.c_str()); }

    if( known_broken && passedQ )
    {
        std::printf("        ^ GH #50 looks fixed; set kIssue50Open = false\n");
        ++failed;
    }
    else if( known_broken )  { ++xfail; }
    else if( !passedQ )      { ++failed; }
}

/// One translation. `issue50` marks the cases #50 breaks on main 09aa3339;
/// they are XFAIL while kIssue50Open is true, and must pass once it is false.
struct Shift { const char * name; double dx, dy, dz; bool issue50; };

template<typename Real, bool mortonQ>
static void RunShifts( const kt::Curve & c, const char * type_name,
                       const std::vector<Shift> & shifts )
{
    for( const Shift & s : shifts )
    {
        std::vector<Real> y ( c.v.size() );
        for( std::size_t i = 0; i < c.v.size(); i += 3 )
        {
            y[i  ] = static_cast<Real>(c.v[i  ] + s.dx);
            y[i+1] = static_cast<Real>(c.v[i+1] + s.dy);
            y[i+2] = static_cast<Real>(c.v[i+2] + s.dz);
        }

        const bool known_broken = kIssue50Open && s.issue50;

        std::string what, said;
        const bool passedQ = RunCase<Real,mortonQ>(y, what, said);

        const std::string label = std::string(type_name)
            + " mortonQ=" + (mortonQ ? "1" : "0") + " " + s.name;
        Report(passedQ, label, what, said, known_broken);
    }
}

int main()
{
    const std::string path = "embeddings/arc_trefoil.crd";
    kt::Curve c;
    std::string error;
    if( !kt::LoadCurve(path, c, error) )
    {
        std::printf("FAILED to load fixture: %s (run from test/)\n", error.c_str());
        return 1;
    }
    if( c.ComponentCount() != 1 || c.VertexCount() != 15 || !c.IntegralQ() )
    {
        std::printf("FAILED: %s is no longer the 15-vertex integral trefoil this test "
                    "assumes (components %lld, vertices %lld)\n", path.c_str(),
                    (long long)c.ComponentCount(), (long long)c.VertexCount());
        return 1;
    }

    const std::vector<Shift> float_shifts = {
        { "as given",          0.0,      0.0,      0.0,      false },
        { "+0.25 all axes",    0.25,     0.25,     0.25,     false },
        { "+1000.25 all axes", 1000.25,  1000.25,  1000.25,  true  },
        { "-1000.25 all axes", -1000.25, -1000.25, -1000.25, true  },
        { "+1000.25 x only",   1000.25,  0.0,      0.0,      true  },
        { "-1000.25 x only",   -1000.25, 0.0,      0.0,      true  },
    };
    const std::vector<Shift> int_shifts = {
        { "as given",          0.0,      0.0,      0.0,      false },
        { "+1000 all axes",    1000.0,   1000.0,   1000.0,   false },
        { "-1000 all axes",    -1000.0,  -1000.0,  -1000.0,  false },
        { "+1000 x only",      1000.0,   0.0,      0.0,      false },
        { "-1000 x only",      -1000.0,  0.0,      0.0,      false },
    };

    std::printf("float_offset_check: %s translated, expecting %lld crossings each time "
                "(GH #50 %s)\n\n", path.c_str(), (long long)kExpectedCrossings,
                kIssue50Open ? "open: the +/-1000.25 double cases are XFAIL" : "fixed");

    RunShifts<double,false>(c, "double", float_shifts);
    RunShifts<double,true >(c, "double", float_shifts);
    RunShifts<Int,   false>(c, "Int64 ", int_shifts  );
    RunShifts<Int,   true >(c, "Int64 ", int_shifts  );

    const bool ok = (failed == 0);
    std::printf("\n%s (%d checks, %d failed; %d XFAIL)\n",
                ok ? "FLOAT OFFSET CHECK OK" : "FLOAT OFFSET CHECK FAILED",
                checks, failed, xfail);
    return ok ? 0 : 1;
}
