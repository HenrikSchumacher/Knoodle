/**
 * @file knoodle_io.hpp
 * @brief Shared input parsing and utilities for knoodlesimplify and knoodledraw.
 *
 * This header is included by each tool's single .cpp translation unit.
 * Everything lives in an anonymous namespace to avoid ODR issues.
 */

#pragma once

#include "../Knoodle.hpp"
#include "../src/OrthoDraw.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <system_error>
#include <utility>

#ifndef _WIN32
#include <sys/utsname.h>   // runtime machine identity for the failure report
#endif
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Interactive-terminal detection: POSIX isatty/fileno vs Windows _isatty/_fileno.
// Centralized here so the three tools build unchanged on macOS, Linux, and Windows.
#include <cstdio>       // stdin, fileno
#ifdef _WIN32
#  include <io.h>       // _isatty, _fileno
#  include <process.h>  // _getpid
#  include <stdlib.h>   // _putenv_s
#else
#  include <unistd.h>   // isatty, fileno, getpid
#endif

#include <set>          // bundle bookkeeping in the diagnostics helpers

//==============================================================================
// Type Aliases
//==============================================================================

using Int         = std::int64_t;
using Real        = double;
using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
using Energy_T    = PDC_T::Energy_T;
using Reapr_T     = Knoodle::Reapr<Real, Int>;
// Floating-point 3D input. Since the LinkEmbedding promotion this is
// LinkEmbedding_Int on Prosector_Int<Int64>: the coordinates are scaled by a
// power of 2, rounded onto an integer grid, and projected with exact
// predicates and simulation of simplicity (src/LinkEmbedding.hpp).
using LinkEmb_T   = Reapr_T::LinkEmbedding_T;
// Integer-coordinate (lattice) input skips the scaling and rounding: the same
// class with integer coordinates, projected exactly as given. The same
// projector knoodleprove trusts (trace_verify.hpp, ProjectLattice).
using LatticeEmb_T = Knoodle::LinkEmbedding<std::int64_t, Int>;

// The timing aliases live in a named namespace rather than at global scope.
// Apple's <MacTypes.h> (pulled in transitively by <Accelerate/Accelerate.h>,
// which the UMFPACK/Alexander path force-includes) declares `typedef SInt32
// Duration;` at global scope. A global `using Duration = ...` here is then a
// conflicting typedef redefinition. Downstream tools that combine this shared
// header with the Accelerate backend are the first TU to meet both, so keep
// these two aliases scoped. The vocabulary aliases above stay global on purpose.
// Where the version text comes from. A tool built by tools/Makefile is compiled
// with KNOODLE_VERSION_LINKED and linked against knoodle_version.cpp, which
// holds the version and the build time; that keeps the version off the tool's
// own compile line, so a new commit relinks a tool instead of recompiling it.
// Anything else that includes this header gets the GIT_VERSION macro if its
// build defines one, exactly as before.
// (Declared here, at file scope: inside the unnamed namespace below they
// would name a different, internal variable and the link would fail.)
#if defined(KNOODLE_VERSION_LINKED)
extern const char * const knoodle_linked_version;
extern const char * const knoodle_linked_build_time;
#endif

namespace knoodle_io {
    using Clock    = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double>;
} // namespace knoodle_io

namespace {

//==============================================================================
// Terminal Utilities
//==============================================================================

/**
 * @brief Is stdin connected to an interactive terminal?
 *
 * Wraps POSIX isatty/fileno and their Windows _isatty/_fileno equivalents so
 * the tools' "reading from stdin…" hint behaves identically on both platforms.
 */
inline bool StdinIsInteractive()
{
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

//==============================================================================
// Timing Utilities
//==============================================================================

/**
 * @brief Simple RAII timer that records elapsed time to a Duration reference.
 */
class ScopedTimer
{
public:
    explicit ScopedTimer(knoodle_io::Duration& target)
        : target_(target), start_(knoodle_io::Clock::now()) {}

    ~ScopedTimer()
    {
        target_ = knoodle_io::Clock::now() - start_;
    }

private:
    knoodle_io::Duration& target_;
    knoodle_io::Clock::time_point start_;
};

//==============================================================================
// Input/Output Data Structures
//==============================================================================

/**
 * @brief Represents an input knot with its summands and metadata.
 */
struct InputKnot
{
    std::vector<PD_T> summands;           ///< Prime summands of the knot
    std::vector<Int>  crossing_counts;    ///< Crossing count per summand
    Int               total_crossings = 0;

    /// One entry per unknot (0-crossing) summand -- knoodlesimplify emits these
    /// as bare 's' lines (color unknown, PD_T::Uninitialized here) in its usual
    /// TSV output, or as 'u <color>' markers (color known) in PlanarDiagramComplex's
    /// native WriteToFile/FromInString format (see ReadKnot's PDC-native
    /// delegation). Old call sites that just want the count can use .size().
    std::vector<Int>  unknot_colors;

    // For 3D geometry input
    bool              had_3d_geometry = false;
    Int               vertex_count_3d = 0;
    Int               component_count_3d = 0;  ///< blank-line-separated components

    /// 3D input whose colors carry information: several components, or an
    /// explicit '#color' line. knoodlesimplify writes such input with colors.
    bool              colored_3d = false;

    int               input_column_count = 0; ///< Detected column format (3/4/5/6/7)

    std::string       source_description;  ///< File name or "stdin"
};

//==============================================================================
// Logging
//==============================================================================

/// Output stream for user messages (may be redirected to file in streaming mode)
std::ostream* g_log_stream = &std::cerr;

/// Log file for streaming mode
std::ofstream g_log_file;

/// Where that log file ended up, so we can tell the user (rule 3). Empty unless
/// streaming mode actually opened one.
std::filesystem::path g_log_path;

//==============================================================================
// Error detection (fail loudly rather than emit silently-wrong output)
//==============================================================================
//
// The core library reports unrecoverable trouble by calling Tools' eprint() and
// carrying on with a diagram it has already told you not to trust -- e.g.
// PlanarDiagramComplex::Rattle, when FindIntersections keeps failing:
//
//   ERROR: ...Rattle: ...returned invalid status flag for 10 random rotation
//          matrices. Something must be wrong. Returning an invalid diagram.
//          Check your results carefully.
//
// PlanarDiagramComplex's writers then *skip* invalid diagrams silently, so the
// run finishes, writes a short file, and (before this change) exited 0. That is
// the worst possible outcome: corrupt output indistinguishable from success.
//
// There is no error counter in Tools to query, and eprint() is not a hook we can
// register with, but it does write every message to std::cerr with a fixed
// "ERROR: " prefix. So we tap std::cerr, pass everything through untouched, and
// count matching lines. Tools serializes its own prints behind cerr_mutex, so
// the counting stays coherent for library output; our own LogError bumps the
// counter directly, since in streaming mode it goes to the log file, not cerr.

/// Number of errors reported by this tool's own LogError.
long g_tool_error_count = 0;

/// Counts "ERROR: " lines written to std::cerr (i.e. Tools' eprint) while
/// forwarding every byte to the original stream. RAII: restores cerr on scope
/// exit. One instance at a time, installed in main().
class CerrErrorTap
{
public:
    CerrErrorTap()
    {
        buf_.sink = std::cerr.rdbuf();
        std::cerr.rdbuf(&buf_);
    }

    ~CerrErrorTap()
    {
        std::cerr.rdbuf(buf_.sink);
    }

    CerrErrorTap(const CerrErrorTap&)            = delete;
    CerrErrorTap& operator=(const CerrErrorTap&) = delete;

    long Count() const { return buf_.errors; }

    /// The error lines themselves, for the diagnostic report (bounded, so a
    /// pathological run cannot eat memory).
    const std::vector<std::string>& Messages() const { return buf_.messages; }

private:
    class Buf : public std::streambuf
    {
    public:
        std::streambuf*          sink   = nullptr;
        long                     errors = 0;
        std::vector<std::string> messages;

        static constexpr std::size_t max_messages   = 50;
        static constexpr std::size_t max_line_bytes = 2000;

    protected:
        int overflow(int c) override
        {
            if (c == traits_type::eof()) { return traits_type::not_eof(c); }
            const char ch = static_cast<char>(c);
            if (sink) { sink->sputc(ch); }
            if (ch == '\n')
            {
                if (line_.starts_with("ERROR:"))
                {
                    ++errors;
                    if (messages.size() < max_messages) { messages.push_back(line_); }
                }
                line_.clear();
            }
            else if (line_.size() < max_line_bytes)
            {
                // Whole line retained now (not just the prefix) so the report can
                // quote what the library actually said.
                line_.push_back(ch);
            }
            return c;
        }

        int sync() override { return sink ? sink->pubsync() : 0; }

    private:
        std::string line_;
    };

    Buf buf_;
};

/// The single tap installed by main(), if any. Tools query it via ErrorsSeen().
CerrErrorTap* g_cerr_tap = nullptr;

/// Total errors so far, library + tool. Snapshot this before writing an output
/// unit and compare after, to decide whether that unit is trustworthy.
[[maybe_unused]] long ErrorTotal()
{
    return g_tool_error_count
         + ((g_cerr_tap != nullptr) ? g_cerr_tap->Count() : 0);
}

/// True if the library or this tool has reported any error during this run.
[[maybe_unused]] bool ErrorsSeen()
{
    return ErrorTotal() > 0;
}

/// Human-readable tally, for the failure message.
[[maybe_unused]] std::string ErrorSummary()
{
    const long lib = (g_cerr_tap != nullptr) ? g_cerr_tap->Count() : 0;
    return std::to_string(lib) + " library error(s), "
         + std::to_string(g_tool_error_count) + " tool error(s)";
}

//==============================================================================
// Diagnostic report
//==============================================================================
//
// When a run fails there is usually nobody watching, and a bug report reaches us
// as a pasted stderr tail with the interesting parts missing (which build? which
// options? which diagram?). So on failure the tools drop a single self-contained
// file next to the output, and print its path. The aim is that forwarding that
// one file is enough for us to reproduce, without a round trip asking for the
// version and the input.
//
// Build identity is recorded twice on purpose. The COMPILE-TIME target says what
// the binary was built for; the RUNTIME uname says what it is executing on. They
// differ for an x86_64 build under Rosetta on Apple silicon -- and since the
// degeneracy this was written for turns on exact floating-point coincidence,
// that distinction is exactly the sort of thing that decides whether a failure
// reproduces.

/// The version text as the build gave it, without the quotes the Makefile's
/// -DGIT_VERSION=\"...\" leaves in; empty when the build gave none.
[[maybe_unused]] std::string RawVersionText()
{
#if defined(KNOODLE_VERSION_LINKED)
    std::string v = knoodle_linked_version;
#elif defined(GIT_VERSION)
    std::string v = GIT_VERSION;
#else
    std::string v;
#endif
    std::erase(v, '"');
    return v;
}

/// The version this binary was built from, as --version and --help print it:
/// "1.2.1" for a release, "1.2.0-38-ge4c4486c" (git describe: nearest release,
/// commits since, hash) for a build between releases, "unversioned-e4c4486c"
/// where no release tag is reachable, "unknown" when the build gave us nothing.
/// The Makefile hands over the full 40-digit hash for the failure reports
/// (BuildIdentity keeps it); here it is cut to 8 digits.
[[maybe_unused]] std::string VersionString()
{
    std::string v = RawVersionText();
    if( v.empty() ) { return "unknown"; }

    std::string dirty;
    if( v.ends_with("-dirty") )
    {
        dirty = "-dirty";
        v.resize(v.size() - dirty.size());
    }

    const auto hexQ = []( std::string_view s )
    {
        return !s.empty() && s.find_first_not_of("0123456789abcdef") == s.npos;
    };

    if( (v.size() >= 7) && hexQ(v) )
    {
        // A bare hash: git describe found no tag.
        return "unversioned-" + v.substr(0, 8) + dirty;
    }

    if( const auto g = v.rfind("-g"); (g != v.npos) && hexQ(std::string_view(v).substr(g + 2)) )
    {
        v.resize(std::min(v.size(), g + 2 + 8));
    }

    if( (v.size() > 1) && (v[0] == 'v') && (v[1] >= '0') && (v[1] <= '9') )
    {
        v.erase(0, 1);   // tags are v1.2.1; the version is 1.2.1
    }
    return v + dirty;
}

/// First line of --version and of --help, in the GNU Coding Standards' form
/// (program, package in parentheses, version after the last space).
[[maybe_unused]] std::string VersionLine( std::string_view tool )
{
    return std::string(tool) + " (Knoodle) " + VersionString();
}

/// What --version prints: the version line, then the licence (the repository's
/// LICENSE is MIT). No copyright line, by JHC's decision.
[[maybe_unused]] std::string VersionText( std::string_view tool )
{
    return VersionLine(tool) + "\n"
        "License: MIT. There is NO WARRANTY, to the extent permitted by law.\n";
}

/// Compile-time build identity.
[[maybe_unused]] std::string BuildIdentity()
{
    std::string s;

    {
        const std::string v = RawVersionText();
        s += "  knoodle version : " + (v.empty() ? std::string("(not compiled in)") : v) + "\n";
    }

#if   defined(__aarch64__) || defined(__arm64__)
    s += "  built for arch  : arm64\n";
#elif defined(__x86_64__)
    s += "  built for arch  : x86_64\n";
#else
    s += "  built for arch  : (unknown)\n";
#endif

#if   defined(__APPLE__)
    s += "  built for OS    : macOS\n";
#elif defined(__linux__)
    s += "  built for OS    : Linux\n";
#else
    s += "  built for OS    : (unknown)\n";
#endif

#if defined(__VERSION__)
    s += "  compiler        : " __VERSION__ "\n";
#endif

#if defined(KNOODLE_VERSION_LINKED)
    s += "  built           : " + std::string(knoodle_linked_build_time) + "\n";
#else
    s += "  built           : " __DATE__ " " __TIME__ "\n";
#endif
    return s;
}

/// Runtime machine identity (see BuildIdentity for why both are recorded).
[[maybe_unused]] std::string RuntimeIdentity()
{
    std::string s;
#ifdef _WIN32
    // No uname() on Windows (the Mathematica paclet's build); the build
    // identity above already names the target.
    s += "  running on      : Windows\n";
#else
    struct utsname u;
    if (uname(&u) == 0)
    {
        s += std::string("  running on      : ") + u.sysname + " " + u.release
           + " (" + u.machine + ")\n";
    }
    else
    {
        s += "  running on      : (uname failed)\n";
    }
#endif
    return s;
}

/// Process id, for a per-run directory name. POSIX `getpid` vs Windows
/// `_getpid`, same split as the isatty handling above.
inline long long CurrentProcessId()
{
#ifdef _WIN32
    return static_cast<long long>(_getpid());
#else
    return static_cast<long long>(getpid());
#endif
}

/// Set an environment variable for this process. POSIX `setenv` vs Windows
/// `_putenv_s`; the library reads `KNOODLE_DUMP_DIR` with plain `std::getenv`,
/// so either is fine on the reading side.
inline void SetEnvVar(const char* name, const std::string& value)
{
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    ::setenv(name, value.c_str(), 1);
#endif
}

/// Where this run puts anything a user might need to forward to us: the failure
/// report written here, and the bundles the core library drops when Rattle's
/// projection fails (see PlanarDiagramComplex/Simplify.hpp, DumpRattleFailure).
/// Set once by ChooseDiagnosticDir; empty until then.
std::filesystem::path g_diagnostic_dir;

/// True only when we created g_diagnostic_dir ourselves as a per-process temp
/// directory, i.e. when it is ours to remove again. Comparing paths after the
/// fact is not reliable -- std::filesystem::temp_directory_path() keeps its
/// trailing separator on macOS, so parent_path() never compares equal to it.
bool g_diagnostic_dir_is_temp = false;

/// The seed this run uses (--seed=N, or drawn at startup; see SeededEngine).
/// Diagnostic reports name it, so that a failing run can be repeated.
std::uint64_t g_run_seed = 0;

/**
 * @brief Decide where this run's diagnostic artifacts belong, and tell the
 *        library about it.
 *
 * Three rules:
 *
 *  1. Writing output to a file -> that file's own directory. If the run fails,
 *     the user finds the diagnostics sitting where they expected their result,
 *     rather than having to be told where to look.
 *  2. Streaming to stdout -> a per-process directory under the system temp
 *     directory. Streaming means we are probably a subshell of something else
 *     (the Mathematica paclet works this way), so the working directory is not
 *     ours to litter and may not even be writable. The risk accepted here is
 *     that the system may purge the temp directory before anyone collects the
 *     bundle.
 *  3. Either way the user is told, by ReportDiagnosticBundles below.
 *
 * An explicit `KNOODLE_DUMP_DIR` from the environment always wins: someone who
 * has chosen a location has out-ranked all of the above.
 *
 * The library reads `KNOODLE_DUMP_DIR` at the point of failure, so this must run
 * before any call into Simplify.
 *
 * @return The directory chosen. Best-effort: on any filesystem error this falls
 *         back to the current directory, which is the historical behaviour.
 */
[[maybe_unused]] std::filesystem::path ChooseDiagnosticDir(
    const std::string& tool,
    bool streaming_mode,
    const std::optional<std::string>& output_file)
{
    namespace fs = std::filesystem;

    std::error_code ec;

    if (const char* forced = std::getenv("KNOODLE_DUMP_DIR"))
    {
        if (*forced != '\0') { g_diagnostic_dir = fs::path(forced); return g_diagnostic_dir; }
    }

    fs::path dir;

    if (!streaming_mode && output_file.has_value())
    {
        // Rule 1. parent_path() is empty for a bare filename, which means "here".
        dir = fs::path(*output_file).parent_path();
        if (dir.empty()) { dir = fs::path("."); }
    }
    else
    {
        // Rule 2. Per-process, so two concurrent runs cannot overwrite each
        // other's bundles -- the library numbers them from zero in every process.
        dir = fs::temp_directory_path(ec);
        if (ec) { dir = fs::path("."); }
        else
        {
            dir /= ("knoodle-" + tool + "-" + std::to_string(CurrentProcessId()));
            fs::create_directories(dir, ec);
            if (ec) { dir = fs::path("."); }
            else    { g_diagnostic_dir_is_temp = true; }
        }
    }

    g_diagnostic_dir = dir;

    // The library only consults the environment, so this is how it learns.
    SetEnvVar("KNOODLE_DUMP_DIR", dir.string());

    return dir;
}

/// Names of the library's bundle files currently in `dir`. Used to tell this
/// run's bundles apart from ones an earlier run left behind.
[[maybe_unused]] std::set<std::string> ListDiagnosticBundles(
    const std::filesystem::path& dir)
{
    namespace fs = std::filesystem;

    std::set<std::string> out;
    std::error_code ec;

    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
    {
        const std::string name = it->path().filename().string();
        if (name.rfind("rattle-failure-", 0) == 0) { out.insert(name); }
    }
    return out;
}

/**
 * @brief Tell the user about any bundle this run produced, and what it is for.
 *
 * Deliberately conditional on whether the run actually failed. Since PR #26 the
 * library dumps on the FIRST failed projection rather than after Rattle has
 * exhausted its retries, and Rattle normally recovers on the next rotation --
 * so a completely successful run can leave a bundle behind. Telling such a user
 * to file a bug report would be wrong and would bury us in noise.
 *
 * Writes to std::cerr directly rather than through eprint: this is information,
 * not an error, and eprint would be counted by the cerr tap and turn a healthy
 * run into a nonzero exit.
 */
[[maybe_unused]] void ReportDiagnosticBundles(
    const std::filesystem::path& dir,
    const std::set<std::string>& before,
    const std::string& tool,
    bool run_failed)
{
    std::set<std::string> now = ListDiagnosticBundles(dir);

    std::vector<std::string> fresh;
    for (const auto& n : now)
    {
        if (before.find(n) == before.end()) { fresh.push_back(n); }
    }
    if (fresh.empty()) { return; }

    std::cerr << tool << ": wrote " << fresh.size()
              << " diagnostic file(s) to " << dir.string() << "\n";
    for (const auto& n : fresh) { std::cerr << "    " << n << "\n"; }

    if (run_failed)
    {
        std::cerr <<
            "  These describe the failure above. Please attach them to a report at\n"
            "  https://github.com/HenrikSchumacher/Knoodle/issues -- they contain the\n"
            "  exact diagram and geometry needed to reproduce it. This run's seed was\n"
            "  --seed=" << g_run_seed << ".\n";
    }
    else
    {
        std::cerr <<
            "  This run SUCCEEDED and its output is valid: the projection step hit a\n"
            "  degenerate viewing angle, and Knoodle retried and recovered. The files\n"
            "  are kept because that case is still worth fixing -- sending them to\n"
            "  https://github.com/HenrikSchumacher/Knoodle/issues helps us make it stop\n"
            "  happening at all.\n";
    }
}

/**
 * @brief Tell the user where the streaming-mode log went, if it has content.
 *
 * Rule 3 applied to the log: in streaming mode it is the only record of what
 * happened, and it no longer sits in the working directory. Silent when the log
 * is empty, so a clean run of a tool the paclet drives thousands of times says
 * nothing at all.
 */
[[maybe_unused]] void ReportLogLocation()
{
    if (g_log_path.empty()) { return; }

    g_log_file.flush();

    std::error_code ec;
    const auto size = std::filesystem::file_size(g_log_path, ec);
    if (ec || size == 0) { return; }

    std::cerr << "  (run log: " << g_log_path.string() << ")\n";
}

/**
 * @brief Drop the streaming-mode log when the run went cleanly.
 *
 * In streaming mode the log always has content -- it carries the per-knot report
 * that would otherwise corrupt stdout -- so the per-process temp directory would
 * never be empty and never be removed. A caller that runs the tool thousands of
 * times (the paclet does) would accumulate one directory per run.
 *
 * On a clean run that report is reproducible by running the tool again, so it is
 * not worth a directory each time. On a failing run it is evidence and is kept.
 *
 * Only ever touches a log inside a directory we created ourselves: if the user
 * pointed KNOODLE_DUMP_DIR somewhere, they asked for the file to be there.
 */
[[maybe_unused]] void DiscardLogIfClean(bool run_failed)
{
    if (run_failed || !g_diagnostic_dir_is_temp || g_log_path.empty()) { return; }

    // Nothing may write to the log after this point.
    g_log_stream = &std::cerr;
    g_log_file.close();

    std::error_code ec;
    std::filesystem::remove(g_log_path, ec);
    if (!ec) { g_log_path.clear(); }
}

/// Remove the per-process temp directory if we made one and nothing used it.
/// Keeps rule 2 from leaving an empty directory behind on every clean run.
[[maybe_unused]] void CleanupDiagnosticDir()
{
    namespace fs = std::filesystem;

    // Only ever remove a directory we created ourselves.
    if (!g_diagnostic_dir_is_temp || g_diagnostic_dir.empty()) { return; }

    // The log lives in here too; close it first so the emptiness test is honest
    // and Windows will let the directory go.
    if (g_log_file.is_open()) { g_log_file.flush(); }

    std::error_code ec;
    if (fs::is_empty(g_diagnostic_dir, ec) && !ec)
    {
        fs::remove(g_diagnostic_dir, ec);
    }
}

/**
 * @brief Everything that has to happen on the way out, in the right order.
 *
 * Report bundles first (while they are all still there), then drop the log if
 * the run was clean, then report the log only if it survived, then remove the
 * directory if that left it empty.
 */
[[maybe_unused]] void FinishDiagnostics(
    const std::filesystem::path& dir,
    const std::set<std::string>& before,
    const std::string& tool,
    bool run_failed)
{
    ReportDiagnosticBundles(dir, before, tool, run_failed);
    DiscardLogIfClean(run_failed);
    ReportLogLocation();
    CleanupDiagnosticDir();
}

/**
 * @brief Write a self-contained failure report the user can forward verbatim.
 *
 * @param tool        Tool name, used for the filename and the header.
 * @param sections    Ordered (title, body) pairs: the caller's options, input
 *                    description, parsed diagram, and so on.
 * @return The path written, or an empty path if it could not be written.
 *
 * Goes in the same directory as the library's bundles (see ChooseDiagnosticDir),
 * so everything a user needs to send is in one place. Previously this was a bare
 * relative path, i.e. the working directory, which for a streaming run inside
 * another process is nobody's idea of a useful location.
 *
 * Never throws and never fails the run further: this is best-effort diagnostics
 * on a path that is already failing.
 */
[[maybe_unused]] std::filesystem::path WriteDiagnosticReport(
    const std::string& tool,
    const std::vector<std::pair<std::string,std::string>>& sections)
{
    const std::filesystem::path dir =
        g_diagnostic_dir.empty() ? std::filesystem::path(".") : g_diagnostic_dir;

    const std::filesystem::path path = dir / (tool + "-error-report.txt");

    std::ofstream out(path);
    if (!out) { return {}; }

    out << "================================================================\n"
        << tool << " error report\n"
        << "================================================================\n\n"
        << "Something went wrong that makes this run's output untrustworthy.\n"
        << "Please send this whole file with your bug report -- it contains the\n"
        << "build, the options, and the input needed to reproduce.\n\n"
        << "-- build ------------------------------------------------------\n"
        << BuildIdentity()
        << RuntimeIdentity()
        << "\n"
        << "-- random seed ------------------------------------------------\n"
        << "  --seed=" << g_run_seed << "\n"
        << "  (rerunning with this flag repeats the run's random choices on this\n"
        << "   build, except OrthoDraw's inside Reapr; see the tool's --help)\n\n";

    out << "-- errors reported --------------------------------------------\n";
    if ((g_cerr_tap != nullptr) && !g_cerr_tap->Messages().empty())
    {
        for (const auto& m : g_cerr_tap->Messages()) { out << "  " << m << "\n"; }
        const long extra = g_cerr_tap->Count()
                         - static_cast<long>(g_cerr_tap->Messages().size());
        if (extra > 0) { out << "  ... and " << extra << " more\n"; }
    }
    else
    {
        out << "  (no library errors; the failure was reported by the tool itself --\n"
               "   see the tool's own message on stderr, e.g. a malformed input line)\n";
    }
    out << "  (" << ErrorSummary() << ")\n\n";

    for (const auto& [title, body] : sections)
    {
        out << "-- " << title << " "
            << std::string(std::max<std::size_t>(4, 62 - title.size()), '-') << "\n"
            << body;
        if (!body.empty() && body.back() != '\n') { out << "\n"; }
        out << "\n";
    }

    out.flush();
    return out ? path : std::filesystem::path{};
}

//==============================================================================
// Atomic file output
//==============================================================================

/**
 * @brief An output file that only becomes visible if the run succeeds.
 *
 * Writes go to "<path>.partial"; Commit() renames it into place, and anything
 * else (explicit Abort, or destruction without Commit) removes it. This is what
 * lets us honour "refuse to produce file output" even though results are written
 * incrementally, knot by knot, long before we know whether the run went wrong.
 *
 * Note this cannot apply to stdout (--streaming-mode): bytes already written to
 * a pipe cannot be recalled, so there the contract is only the nonzero exit.
 */
class AtomicOutFile
{
public:
    explicit AtomicOutFile(const std::filesystem::path& final_path)
    :   final_path_ { final_path }
    ,   temp_path_  { final_path.string() + ".partial" }
    {
        stream_.open(temp_path_);
    }

    ~AtomicOutFile()
    {
        if (!committed_) { Abort(); }
    }

    AtomicOutFile(const AtomicOutFile&)            = delete;
    AtomicOutFile& operator=(const AtomicOutFile&) = delete;

    bool Good() const { return static_cast<bool>(stream_); }

    std::ofstream& Stream() { return stream_; }

    /// Move the finished file into place. Returns false if the rename failed.
    bool Commit()
    {
        if (committed_) { return true; }
        stream_.flush();
        stream_.close();
        std::error_code ec;
        std::filesystem::rename(temp_path_, final_path_, ec);
        if (ec)
        {
            std::filesystem::remove(temp_path_, ec);
            return false;
        }
        committed_ = true;
        return true;
    }

    /// Discard the partial file, leaving any previous output file untouched.
    void Abort()
    {
        stream_.close();
        std::error_code ec;
        std::filesystem::remove(temp_path_, ec);
    }

    const std::filesystem::path& FinalPath() const { return final_path_; }

private:
    std::filesystem::path final_path_;
    std::filesystem::path temp_path_;
    std::ofstream         stream_;
    bool                  committed_ = false;
};

/**
 * @brief Initialize logging based on mode.
 *
 * @param streaming_mode If true, redirect log output to a file.
 * @param log_filename The filename for the log file (used only in streaming mode).
 * @return true on success.
 */
[[maybe_unused]] bool InitLogging(bool streaming_mode, const std::string& log_filename)
{
    if (streaming_mode)
    {
        // Same three rules as the failure report and the library's bundles: in
        // streaming mode this used to be a bare relative path, i.e. the working
        // directory of whatever process happens to be driving us. Call
        // ChooseDiagnosticDir before this; the fallback keeps the old behaviour
        // if some caller has not.
        const std::filesystem::path dir =
            g_diagnostic_dir.empty() ? std::filesystem::path(".") : g_diagnostic_dir;

        g_log_path = dir / log_filename;

        g_log_file.open(g_log_path);
        if (!g_log_file)
        {
            std::cerr << "Error: Failed to open " << g_log_path.string() << " for writing.\n";
            return false;
        }
        g_log_stream = &g_log_file;
    }
    return true;
}

/**
 * @brief Log a message to the appropriate stream.
 */
[[maybe_unused]] void Log(const std::string& msg)
{
    *g_log_stream << msg << "\n";
}

/**
 * @brief Log an error message.
 */
void LogError(const std::string& msg)
{
    ++g_tool_error_count;   // see ErrorsSeen(): in streaming mode this line goes
                            // to the log file, so the tap on cerr won't see it.
    *g_log_stream << "Error: " << msg << "\n";
}

//==============================================================================
// String Utilities
//==============================================================================

/**
 * @brief Convert a string to lowercase.
 */
[[maybe_unused]] std::string ToLower(std::string_view s)
{
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * @brief Canonical form of a command-line token, for tolerant matching.
 *
 * Lowercase, with '-' and '_' dropped, so that every spelling that differs only
 * in case or separators folds together:
 *
 *   --checkerboard-coloring  --CheckerboardColoring  --checkerboard_coloring
 *       all canonicalize to  "checkerboardcoloring"
 *
 * A user who types any of those has said unambiguously what they want, so the
 * tools serve them rather than complain about the spelling.
 *
 * Used for flag NAMES (CanonicalizeFlagSpelling) and for the values of flags
 * with a fixed vocabulary (TokenIs / MatchToken) -- --format=WL and
 * --compaction=LENGTH_MCF are as clear as their preferred spellings. It is
 * never applied to open-ended values: dropping '-' from a path, a PD
 * descriptor or a negative number would change what they mean.
 */
[[maybe_unused]] std::string CanonicalToken(std::string_view name)
{
    std::string result;
    result.reserve(name.size());
    for (char c : name)
    {
        if (c == '-' || c == '_') { continue; }
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

/**
 * @brief Rewrite one argv entry to the PREFERRED spelling of the flag it names.
 *
 * `known` lists a tool's documented flags in their preferred spelling (the one
 * and only form --help describes). If `arg` names one of them in any spelling
 * that canonicalizes the same, the preferred spelling is substituted; anything
 * else is handed back untouched. Run over argv before matching, this buys the
 * whole parser variant-spelling tolerance without a single comparison site
 * having to know about it -- and because the substituted name has the preferred
 * spelling's exact length, offsets into the rewritten argument stay valid.
 *
 * Only the flag NAME is folded. Everything right of the first '=' is a value --
 * a path, a PD descriptor, a highlight list -- where case and punctuation carry
 * meaning, so it is copied verbatim. Single-dash arguments are left alone too:
 * they are short flags (-calf) or filenames, not long-flag spellings.
 *
 * Deliberately NOT accepted: abbreviations (--checker) and single-dash long
 * flags (-format=wl). Every accepted spelling maps to exactly one documented
 * flag, so the mapping is unambiguous today and adding a flag tomorrow cannot
 * change how an existing spelling parses. An unknown flag comes back verbatim,
 * so the parser's own error message quotes what the user actually typed.
 */
template<typename FlagList>
[[maybe_unused]] std::string CanonicalizeFlagSpelling(
    std::string_view arg, const FlagList & known)
{
    if (!arg.starts_with("--")) { return std::string(arg); }

    const auto eq = arg.find('=');
    const std::string canon = CanonicalToken(arg.substr(0, eq));

    for (std::string_view flag : known)
    {
        if (CanonicalToken(flag) != canon) { continue; }

        std::string result(flag);
        if (eq != std::string_view::npos) { result += std::string(arg.substr(eq)); }
        return result;
    }

    return std::string(arg);
}

/**
 * @brief Does `value` name the vocabulary token `token`, in any spelling?
 *
 * For flags whose value is drawn from a fixed vocabulary (--format=wl,
 * --bend-method=mcf, --compaction=length-mcf). Case and '-'/'_' separators are
 * ignored, so --compaction=LENGTH_MCF and --reapr-energy=TV-CLP are served.
 *
 * Only ever call this where the vocabulary IS fixed. A path, a PD descriptor
 * or a number must be compared verbatim.
 */
[[maybe_unused]] bool TokenIs(std::string_view value, std::string_view token)
{
    return CanonicalToken(value) == CanonicalToken(token);
}

/**
 * @brief Find the preferred spelling of the vocabulary token `value` names.
 *
 * Returns the matching entry of `tokens` -- the preferred spelling, as --help
 * documents it -- or an empty view if `value` names none of them. Use this
 * rather than TokenIs where the value is STORED and compared again later, so
 * that what gets stored is the canonical token and every downstream comparison
 * keeps working unchanged.
 */
template<typename TokenList>
[[maybe_unused]] std::string_view MatchToken(
    std::string_view value, const TokenList & tokens)
{
    for (std::string_view token : tokens)
    {
        if (TokenIs(value, token)) { return token; }
    }
    return std::string_view{};
}

/**
 * @brief Trim leading and trailing whitespace from a string.
 */
std::string Trim(std::string_view s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(start, end - start + 1));
}

/**
 * @brief Remove comments (% to end of line) and clean up a line.
 *
 * Also removes commas and braces for compatibility with various input formats.
 */
std::string CleanLine(std::string_view line)
{
    // Remove comment (everything from % to end)
    auto comment_pos = line.find('%');
    if (comment_pos != std::string_view::npos)
    {
        line = line.substr(0, comment_pos);
    }

    std::string result;
    result.reserve(line.size());

    for (char c : line)
    {
        // Skip commas and braces
        if (c == ',' || c == '{' || c == '}')
        {
            result += ' ';  // Replace with space to preserve token separation
        }
        else
        {
            result += c;
        }
    }

    return Trim(result);
}

/**
 * @brief Parse a line into numeric tokens.
 *
 * @param line The cleaned input line.
 * @param[out] values Vector to store parsed values.
 * @param[out] has_float Set to true if any value contains a decimal point.
 * @return true if parsing succeeded, false otherwise.
 */
bool ParseNumericLine(const std::string& line,
                      std::vector<Real>& values,
                      bool& has_float)
{
    values.clear();
    has_float = false;

    std::istringstream iss(line);
    std::string token;

    while (iss >> token)
    {
        // Check if token contains a decimal point
        if (token.find('.') != std::string::npos)
        {
            has_float = true;
        }

        try
        {
            Real val = std::stod(token);
            values.push_back(val);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    return true;
}

//==============================================================================
// Energy Utilities
//==============================================================================

[[maybe_unused]] std::string EnergyName(Energy_T e)
{
    switch (e)
    {
        case Energy_T::TV:        return "TV";
        case Energy_T::Dirichlet: return "Dirichlet";
        case Energy_T::Bending:   return "Bending";
        case Energy_T::Height:    return "Height";
        case Energy_T::TV_CLP:    return "TV_CLP";
        case Energy_T::TV_MCF:    return "TV_MCF";
        default:                  return "Unknown";
    }
}

[[maybe_unused]] std::string ValidEnergies()
{
    return std::string("TV, ")
#ifdef KNOODLE_USE_UMFPACK
    + "Dirichlet, "
    + "Bending, "
#endif
    + "Height, "
#ifdef KNOODLE_USE_CLP
    + "TV_CLP, "
#endif
    + "TV_MCF";
}

/**
 * @brief Parse a positive integer from a string: a grid size, a count.
 *        Shared by the tools' integer flags (e.g. knoodledraw's --x-grid-size
 *        and --z-grid-size, the others' --reapr-z-grid-size).
 */
[[maybe_unused]] std::optional<Int> ParsePositiveInt(std::string_view s)
{
    try
    {
        std::size_t pos = 0;
        long long val = std::stoll(std::string(s), &pos);
        if (pos != s.size() || val < 1) return std::nullopt;
        return static_cast<Int>(val);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

/**
 * @brief Parse the value of --reapr-energy (fixed vocabulary, so case and
 *        separators are forgiven).
 *
 * Every name parses whatever this build supports; ValidEnergies() is the list
 * the build can actually run.
 */
[[maybe_unused]] std::optional<Energy_T> ParseEnergy(std::string_view value)
{
    if (TokenIs(value, "tv"))        { return Energy_T::TV;        }
    if (TokenIs(value, "dirichlet")) { return Energy_T::Dirichlet; }
    if (TokenIs(value, "bending"))   { return Energy_T::Bending;   }
    if (TokenIs(value, "height"))    { return Energy_T::Height;    }
    if (TokenIs(value, "tv_clp"))    { return Energy_T::TV_CLP;    }
    if (TokenIs(value, "tv_mcf"))    { return Energy_T::TV_MCF;    }
    return std::nullopt;
}

//==============================================================================
// PDC-Native Format I/O
//==============================================================================

/**
 * @brief Write a PlanarDiagramComplex to an arbitrary output stream using its
 *        own native serialization (PDC_T::WriteToFile -- 'u <color>' for
 *        colored unknot summands, 's <flag>' + colored PD rows otherwise; see
 *        src/PlanarDiagramComplex/WriteToFile.hpp).
 *
 * WriteToFile only writes to a real file, so this round-trips through a
 * unique scratch file rather than reimplementing its logic -- callers (e.g.
 * knoodlesimplify's --format=pdc) get Henrik's exact, unmodified output
 * regardless of whether their own destination is a file or stdout.
 */
[[maybe_unused]] bool WritePdcNativeFormat(PDC_T& pdc, std::ostream& output, bool leading_kQ = true)
{
    static std::atomic<unsigned long long> counter{0};

    auto scratch_path = std::filesystem::temp_directory_path() / (
        "knoodle_pdc_"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
        + "_" + std::to_string(counter.fetch_add(1)) + ".tmp"
    );

    if (!pdc.WriteToFile(scratch_path, leading_kQ))
    {
        LogError("WritePdcNativeFormat: PlanarDiagramComplex::WriteToFile failed for "
                 + scratch_path.string());
        return false;
    }

    std::ifstream scratch(scratch_path);
    if (!scratch)
    {
        LogError("WritePdcNativeFormat: failed to reopen scratch file " + scratch_path.string());
        std::filesystem::remove(scratch_path);
        return false;
    }

    output << scratch.rdbuf();
    scratch.close();
    std::filesystem::remove(scratch_path);
    return true;
}

//==============================================================================
// Input Parsing
//==============================================================================

/**
 * @brief Detect input format from a data line.
 *
 * @return 3 for 3D geometry, 4 for unsigned PD code, 5 for signed PD code,
 *         6 for unsigned PD + colors, 7 for signed PD + colors, 0 for unknown.
 */
int DetectFormat(const std::vector<Real>& values, bool has_float)
{
    if (values.size() == 3)
    {
        return 3;  // 3D geometry
    }
    else if (values.size() == 4 && !has_float)
    {
        return 4;  // Unsigned PD code
    }
    else if (values.size() == 5 && !has_float)
    {
        return 5;  // Signed PD code
    }
    else if (values.size() == 6 && !has_float)
    {
        return 6;  // Unsigned PD code + link component colors
    }
    else if (values.size() == 7 && !has_float)
    {
        return 7;  // Signed PD code + link component colors
    }
    return 0;  // Unknown
}

//==============================================================================
// Random numbers (--seed)
//==============================================================================
//
// Every random choice a tool makes follows from one 64-bit seed: the tool's own
// engine is seeded from it (SeededEngine), and every Reapr the tool builds gets
// an engine drawn from that one (DerivedEngine). So --seed=N repeats a run --
// on the same build: std::normal_distribution and friends are not specified
// bit for bit, so libc++ and libstdc++ builds draw different numbers from the
// same seed.
//
// One source stays out of reach: the OrthoDraw that Reapr builds inside
// Embedding() seeds itself from std::random_device, and its bend and virtual-
// edge randomization (on by default) is not repeated. A run that re-embeds
// repeats exactly only with --randomize-bends=0 --no-randomize-virtual-edges.

/// A seed nobody chose, for a run without --seed.
[[maybe_unused]] std::uint64_t FreshSeed()
{
    std::random_device rd;
    return (static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd());
}

/// An engine seeded from one 64-bit number. std::seed_seq's mixing is fully
/// specified, so this is the same engine on every platform.
[[maybe_unused]] Knoodle::PRNG_T SeededEngine(std::uint64_t seed)
{
    std::seed_seq seq {
        static_cast<std::uint32_t>(seed), static_cast<std::uint32_t>(seed >> 32)
    };
    return Knoodle::PRNG_T(seq);
}

/// A fresh engine for a library object (a Reapr), drawn from the tool's own,
/// so that the library's random choices follow from --seed too.
[[maybe_unused]] Knoodle::PRNG_T DerivedEngine(Knoodle::PRNG_T& rng)
{
    std::array<std::uint32_t, 8> words;
    for (std::size_t i = 0; i < words.size(); i += 2)
    {
        const std::uint64_t r = rng();
        words[i]     = static_cast<std::uint32_t>(r);
        words[i + 1] = static_cast<std::uint32_t>(r >> 32);
    }
    std::seed_seq seq(words.begin(), words.end());
    return Knoodle::PRNG_T(seq);
}

/**
 * @brief Parse the N of --seed=N: a decimal integer in [0, 2^64).
 *
 * Verbatim, like every number: no sign, no spaces, nothing after the digits.
 */
[[maybe_unused]] std::optional<std::uint64_t> ParseSeed(std::string_view value)
{
    std::uint64_t seed = 0;
    const char* first = value.data();
    const char* last  = value.data() + value.size();

    if (value.empty() || !std::isdigit(static_cast<unsigned char>(value.front())))
    {
        return std::nullopt;
    }
    auto [ptr, ec] = std::from_chars(first, last, seed);
    if (ec != std::errc() || ptr != last) { return std::nullopt; }
    return seed;
}

//==============================================================================
// 3D input
//==============================================================================
//
// 3D input comes in one shape, read by ReadKnot from a file or from stdin:
// three numbers per vertex, a blank line between link components, and an
// optional '#color' line before each component. A .kndlxyz file is exactly
// that, so it needs no route of its own.
//
// All of it is projected by ProjectGeometry: exactly, through LatticeEmb_T,
// when every coordinate is an integer, and through the floating-point
// LinkEmb_T (which scales and rounds onto an integer grid) otherwise.

/// 3D input as read: the coordinates, where each component starts, and its color.
struct Geometry3D
{
    std::vector<Real> xyz;                    ///< x, y, z per vertex
    std::vector<Int>  ptr { 0 };              ///< component k is vertices [ptr[k], ptr[k+1])
    std::vector<Int>  colors;                 ///< one per component
    bool              colors_declared = false;///< colors came from '#color' lines

    Int ComponentCount() const { return static_cast<Int>(ptr.size()) - 1; }
    Int VertexCount()    const { return static_cast<Int>(xyz.size() / 3); }
};

/// Which way 3D input is projected (--projection).
enum class Projection_T
{
    Z,       ///< down the z-axis: the diagram is (x, y), z decides over and under
    X,       ///< down the x-axis: (y, z), x decides over and under
    Y,       ///< down the y-axis: (z, x), y decides over and under
    Random   ///< after a uniformly random rotation of the whole link
};

/// How 3D input is read and projected; the same for every tool and every route.
struct InputOptions
{
    Projection_T projection  = Projection_T::Z;  ///< --projection
    bool         exact_input = false;            ///< --exact-input
};

/// The values --projection takes, preferred spellings first.
constexpr std::array<std::string_view, 4> kProjectionNames { "z", "x", "y", "random" };

/**
 * @brief Parse the value of --projection (fixed vocabulary, so case and
 *        separators are forgiven).
 */
[[maybe_unused]] std::optional<Projection_T> ParseProjection(std::string_view value)
{
    const std::string_view name = MatchToken(value, kProjectionNames);
    if (name == "z")      { return Projection_T::Z; }
    if (name == "x")      { return Projection_T::X; }
    if (name == "y")      { return Projection_T::Y; }
    if (name == "random") { return Projection_T::Random; }
    return std::nullopt;
}

/// The preferred spelling of a projection, as --help documents it.
[[maybe_unused]] std::string ProjectionName(Projection_T p)
{
    return std::string(kProjectionNames[static_cast<std::size_t>(p)]);
}

/// What projecting 3D input gave.
struct Projection3D
{
    std::vector<PD_T> diagrams;       ///< diagrams with at least one crossing
    std::vector<Int>  unknot_colors;  ///< one per crossingless diagram, its color
};

using LatticeMatrix_T = std::array<std::array<std::int64_t, 3>, 3>;

/**
 * @brief Is every coordinate an integer that a double holds exactly?
 *
 * The test is on VALUES, not on how they were spelled: "1.0 2.0 3.0" is a
 * lattice vertex too. Beyond 2^53 a parsed double no longer pins down the
 * integer that was written, so such input stays on the floating-point path.
 */
bool LatticeCoordinatesQ(const Real* x, std::size_t n)
{
    constexpr Real limit = 9007199254740992.0;  // 2^53

    for (std::size_t i = 0; i < n; ++i)
    {
        // Written so that NaN fails too.
        if (!(std::abs(x[i]) < limit) || std::trunc(x[i]) != x[i]) return false;
    }
    return true;
}

/**
 * @brief Multiply every vertex by the integer matrix M, exactly.
 *
 * @return false if a coordinate would leave Prosector_Int's headroom (60 bits
 *         for Int64; we stop at 58, as redraw_view.hpp does).
 */
bool TransformLattice(std::vector<std::int64_t>& xyz, const LatticeMatrix_T& M)
{
    using W_T = __int128;
    constexpr W_T lim = W_T(1) << 58;

    for (std::size_t v = 0; v + 2 < xyz.size(); v += 3)
    {
        const W_T x[3] = {xyz[v], xyz[v + 1], xyz[v + 2]};
        W_T y[3];

        for (int i = 0; i < 3; ++i)
        {
            y[i] = W_T(M[i][0]) * x[0] + W_T(M[i][1]) * x[1] + W_T(M[i][2]) * x[2];
            if (y[i] >= lim || y[i] <= -lim) return false;
        }
        for (int i = 0; i < 3; ++i) xyz[v + i] = static_cast<std::int64_t>(y[i]);
    }
    return true;
}

/**
 * @brief A uniformly random unit quaternion (w, x, y, z): a Gaussian vector
 *        in R^4, normalized. q and -q give the same rotation, and the
 *        rotations it gives are uniform (Haar) on SO(3).
 */
std::array<Real, 4> RandomUnitQuaternion(Knoodle::PRNG_T& rng)
{
    std::normal_distribution<Real> gaussian(0, 1);
    std::array<Real, 4> g;
    Real norm = 0;
    do
    {
        norm = 0;
        for (Real& gi : g) { gi = gaussian(rng); norm += gi * gi; }
    }
    while (norm == 0);
    norm = std::sqrt(norm);
    for (Real& gi : g) { gi /= norm; }
    return g;
}

/**
 * @brief A uniformly random rotation, as an integer matrix.
 *
 * For an integer quaternion q = (w, x, y, z), |q|^2 R(q/|q|) has integer
 * entries, so the rotated curve is still a lattice curve, exactly; scaling by
 * |q|^2 does not change the diagram. q is a uniform direction on S^3 rounded
 * at radius 256: within ~1/256 radian of a uniform rotation, with coordinates
 * growing by |q|^2 ~ 2^16. det = |q|^6 > 0, so the link is not mirrored.
 */
LatticeMatrix_T RandomLatticeRotation(Knoodle::PRNG_T& rng)
{
    constexpr Real radius = 256;

    const std::array<Real, 4> u = RandomUnitQuaternion(rng);

    // Some |u_i| >= 1/2, so q != 0.
    std::int64_t q[4];
    for (int i = 0; i < 4; ++i) q[i] = std::llround(radius * u[i]);

    const std::int64_t w = q[0], x = q[1], y = q[2], z = q[3];

    return {{
        { w*w + x*x - y*y - z*z, 2 * (x*y - w*z),         2 * (x*z + w*y)         },
        { 2 * (x*y + w*z),       w*w - x*x + y*y - z*z,   2 * (y*z - w*x)         },
        { 2 * (x*z - w*y),       2 * (y*z + w*x),         w*w - x*x - y*y + z*z   }
    }};
}

/**
 * @brief Rotate floating-point coordinates by a uniformly random rotation.
 *
 * The same quaternion draw as RandomLatticeRotation, unrounded. (Reapr's own
 * RandomRotation picks a uniform axis and a uniform angle, which is not
 * uniform on SO(3); small angles come up too often.)
 */
void RandomRotateFloat(std::vector<Real>& xyz, Knoodle::PRNG_T& rng)
{
    const auto [w, x, y, z] = RandomUnitQuaternion(rng);

    const Real R[3][3] = {
        { 1 - 2 * (y*y + z*z), 2 * (x*y - w*z),     2 * (x*z + w*y)     },
        { 2 * (x*y + w*z),     1 - 2 * (x*x + z*z), 2 * (y*z - w*x)     },
        { 2 * (x*z - w*y),     2 * (y*z + w*x),     1 - 2 * (x*x + y*y) }
    };

    for (std::size_t v = 0; v + 2 < xyz.size(); v += 3)
    {
        const Real p[3] = {xyz[v], xyz[v + 1], xyz[v + 2]};
        for (int i = 0; i < 3; ++i)
        {
            xyz[v + i] = R[i][0] * p[0] + R[i][1] * p[1] + R[i][2] * p[2];
        }
    }
}

/**
 * @brief Turn the coordinates so that --projection=x or y becomes a projection
 *        down the z-axis: (x, y, z) -> (y, z, x) or (z, x, y).
 *
 * Both are cyclic permutations (det +1, so nothing is mirrored), and both are
 * exact.
 */
void PermuteAxes(std::vector<Real>& xyz, Projection_T p)
{
    if (p != Projection_T::X && p != Projection_T::Y) { return; }

    for (std::size_t v = 0; v + 2 < xyz.size(); v += 3)
    {
        const Real x = xyz[v], y = xyz[v + 1], z = xyz[v + 2];
        if (p == Projection_T::X) { xyz[v] = y; xyz[v + 1] = z; xyz[v + 2] = x; }
        else                      { xyz[v] = z; xyz[v + 1] = x; xyz[v + 2] = y; }
    }
}

/**
 * @brief Drop each vertex that repeats the one before it (cyclically, within
 *        each component), so no edge has length zero.
 *
 * The polygon is the same point set, so this is exact, for integer and
 * floating-point coordinates alike (the comparison is ==, not a tolerance).
 * It matters because Prosector_Int mishandles zero-length edges: it rejects
 * them as 3D self-intersections, or trips an assertion in LinesColinearTest
 * (test/embeddings/deg_zero_length and deg_stacked_points; upstream issue 11).
 *
 * @param xyz Coordinates, 3 per vertex; compacted in place.
 * @param ptr Component pointers into the vertices; rewritten to match.
 */
template<typename T>
void DropRepeatedVertices(std::vector<T>& xyz, std::vector<Int>& ptr)
{
    auto same = [&xyz](std::size_t u, std::size_t v)
    {
        return xyz[3 * u] == xyz[3 * v] && xyz[3 * u + 1] == xyz[3 * v + 1]
            && xyz[3 * u + 2] == xyz[3 * v + 2];
    };

    std::vector<Int> new_ptr{0};
    std::size_t out = 0;  // vertices kept so far; never ahead of v below

    for (std::size_t c = 0; c + 1 < ptr.size(); ++c)
    {
        const std::size_t begin = out;

        for (auto v = static_cast<std::size_t>(ptr[c]); v < static_cast<std::size_t>(ptr[c + 1]); ++v)
        {
            if (out > begin && same(out - 1, v)) continue;
            for (std::size_t k = 0; k < 3; ++k) xyz[3 * out + k] = xyz[3 * v + k];
            ++out;
        }
        // The closing edge, last vertex back to the first.
        while (out - begin > 1 && same(out - 1, begin)) --out;

        new_ptr.push_back(static_cast<Int>(out));
    }

    xyz.resize(3 * out);
    ptr = std::move(new_ptr);
}

/**
 * @brief Project a link embedding and sort what comes out into diagrams with
 *        crossings and crossingless (unknot) components.
 *
 * @return nullopt if the embedding is not one (it meets itself in 3-space);
 *         the library has already said why, on stderr.
 */
template<typename Emb_T>
std::optional<Projection3D> ProjectEmbedding(Emb_T& L)
{
    // An invalid pd with a list of unlink colors is the all-crossingless case,
    // not a failure; PDC_T's constructor turns those colors into anelli.
    PDC_T pdc(PD_T::FromLinkEmbedding(L));

    if (pdc.DiagramCount() == Int(0)) return std::nullopt;

    // A split link projects to ONE diagram with several diagram components,
    // which OrthoDraw refuses -- and knoodledraw, reaching it anyway,
    // segfaulted. Give each diagram component its own diagram. The pieces
    // never share a color, so nothing downstream mistakes them for summands
    // of one component.
    pdc.Split();

    Projection3D result;
    for (Int i = 0; i < pdc.DiagramCount(); ++i)
    {
        Knoodle::cref<PD_T> pd = pdc.Diagram(i);
        if (pd.CrossingCount() > Int(0)) { result.diagrams.push_back(PD_T(pd)); }
        else                             { result.unknot_colors.push_back(pd.FirstColor()); }
    }
    return result;
}

/**
 * @brief The power of two s that LinkEmb_T will scale these coordinates by,
 *        and its exponent, chosen exactly as
 *        LinkEmbedding_Int::ComputeEdgeCoordinates chooses them: from the
 *        extent of the bounding box, 2^(60 - ceil(log2 extent)), and no
 *        scaling at all for integer input that would only be scaled up.
 */
std::pair<Real, int> PredictedGridScale(const std::vector<Real>& xyz)
{
    constexpr int m = 60;  // bits LinkEmbedding_Int allows Int64 coordinates

    Real lo[3] = { std::numeric_limits<Real>::infinity(),
                   std::numeric_limits<Real>::infinity(),
                   std::numeric_limits<Real>::infinity() };
    Real hi[3] = { -lo[0], -lo[1], -lo[2] };
    bool integral = true;

    for (std::size_t v = 0; v + 2 < xyz.size(); v += 3)
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            lo[i] = std::min(lo[i], xyz[v + i]);
            hi[i] = std::max(hi[i], xyz[v + i]);
            integral = integral && (std::trunc(xyz[v + i]) == xyz[v + i]);
        }
    }
    const Real extent = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
    const int  e      = m - static_cast<int>(std::ceil(std::log2(extent)));

    if (integral && e >= 0) { return { Real(1), 0 }; }
    return { std::pow(Real(2), e), e };
}

/**
 * @brief Why these floating-point coordinates would not land exactly on
 *        LinkEmb_T's integer grid, or nullopt if they would (--exact-input).
 *
 * LinkEmb_T scales the coordinates by a power of two s (PredictedGridScale)
 * and rounds them to integers. The projection is exact iff no coordinate
 * moves (what RoundingError() reports afterwards) and every s x fits in the
 * 60 bits Prosector_Int's determinants have room for. The second test is the
 * one RoundingError() does not make: s comes from the spread of the
 * coordinates, not their size, so coordinates far from the origin for their
 * spread overflow (GH #50). Checked BEFORE projecting, because an overflowed
 * grid makes the library report nonsense (edges meeting in 3D, say).
 */
std::optional<std::string> InexactGrid(const std::vector<Real>& xyz)
{
    const auto [s, e] = PredictedGridScale(xyz);

    Real err = 0;
    Real biggest = 0;
    for (Real x : xyz)
    {
        const Real y = std::nearbyint(x * s);
        err     = std::max(err, std::abs(std::fma(-s, x, y)));
        biggest = std::max(biggest, std::abs(x));
    }

    constexpr Real lim = 1152921504606846976.0;  // 2^60

    if (biggest * s > lim)
    {
        std::ostringstream why;
        why << "--exact-input: the coordinates are too far from the origin for "
               "their spread (|x| up to " << biggest << " at scale 2^" << e
            << " overflows 60 bits; see "
               "https://github.com/HenrikSchumacher/Knoodle/issues/50). Translate "
               "the link to the origin, or write integer coordinates.";
        return why.str();
    }
    if (err > Real(0))
    {
        std::ostringstream why;
        why << "--exact-input: rounding the coordinates onto Knoodle's integer grid "
               "(scale 2^" << e << ") would move a vertex by up to " << err
            << " grid units. Write integer coordinates instead, scaled up as far "
               "as needed.";
        return why.str();
    }
    return std::nullopt;
}

/**
 * @brief Project 3D input to diagrams.
 *
 * If every coordinate is an integer (after --projection=x|y permutes the axes,
 * which is exact), the link is projected exactly through LatticeEmb_T, and
 * --projection=random is an exact integer rotation (RandomLatticeRotation).
 * Otherwise it goes through the floating-point LinkEmb_T, rotated in floating
 * point (RandomRotateFloat); with --exact-input that path must not round.
 *
 * Either way, a vertex that repeats the one before it is dropped first
 * (DropRepeatedVertices), which is exact.
 *
 * @param g    The input; a component may end with a copy of its first vertex
 *             (a closing marker), which is dropped.
 * @param opt  The projection, and whether rounding is allowed.
 * @param rng  The tool's engine (--seed), for --projection=random.
 * @return The diagrams, or nullopt (after LogError) on failure.
 */
std::optional<Projection3D> ProjectGeometry(Geometry3D g,
                                            const InputOptions& opt,
                                            Knoodle::PRNG_T& rng)
{
    using T_T = Knoodle::Tensor1<Int, Int>;

    if (g.ComponentCount() < 1)
    {
        LogError("3D geometry has no vertices");
        return std::nullopt;
    }

    // A closing marker: a last vertex within 1e-10 of the first. Dropping it is
    // not quite exact (DropRepeatedVertices below catches the exact repeats);
    // it is the historical rule, kept for input written with a near-copy.
    {
        constexpr Real eps = 1e-10;
        std::vector<Int> keep { 0 };
        std::vector<Real> xyz;
        xyz.reserve(g.xyz.size());

        for (Int c = 0; c < g.ComponentCount(); ++c)
        {
            Int b = g.ptr[c];
            Int e = g.ptr[c + 1];
            if (e - b > 1)
            {
                const Real* first = &g.xyz[3 * b];
                const Real* last  = &g.xyz[3 * (e - 1)];
                if (std::abs(first[0] - last[0]) < eps
                    && std::abs(first[1] - last[1]) < eps
                    && std::abs(first[2] - last[2]) < eps)
                {
                    --e;
                }
            }
            xyz.insert(xyz.end(), g.xyz.begin() + 3 * b, g.xyz.begin() + 3 * e);
            keep.push_back(static_cast<Int>(xyz.size() / 3));
        }
        g.xyz = std::move(xyz);
        g.ptr = std::move(keep);
    }

    auto check_sizes = [](const std::vector<Int>& ptr) -> bool
    {
        for (std::size_t c = 0; c + 1 < ptr.size(); ++c)
        {
            if (ptr[c + 1] - ptr[c] < 3)
            {
                LogError("3D geometry: component " + std::to_string(c)
                         + " has fewer than 3 distinct vertices");
                return false;
            }
        }
        return true;
    };

    PermuteAxes(g.xyz, opt.projection);

    if (LatticeCoordinatesQ(g.xyz.data(), g.xyz.size()))
    {
        // Integral doubles below 2^53: the conversion is exact.
        std::vector<std::int64_t> xyz(g.xyz.begin(), g.xyz.end());

        if (opt.projection == Projection_T::Random)
        {
            if (!TransformLattice(xyz, RandomLatticeRotation(rng)))
            {
                LogError("Lattice coordinates too large for an exact random rotation "
                         "(it scales them by about 2^16, and they must stay below 2^58)");
                return std::nullopt;
            }
        }

        DropRepeatedVertices(xyz, g.ptr);
        if (!check_sizes(g.ptr)) return std::nullopt;

        LatticeEmb_T L(T_T(g.ptr.data(), static_cast<Int>(g.ptr.size())),
                       T_T(g.colors.data(), static_cast<Int>(g.colors.size())));
        L.ReadVertexCoordinates(xyz.data());
        auto result = ProjectEmbedding(L);
        if (!result) LogError("3D geometry is not an embedding (it meets itself in 3-space)");
        return result;
    }

    if (opt.projection == Projection_T::Random)
    {
        if (opt.exact_input)
        {
            LogError("--exact-input: a random rotation of non-integer coordinates "
                     "cannot be exact. Write integer coordinates (they rotate "
                     "exactly), or use --projection=z, x or y.");
            return std::nullopt;
        }
        // The whole link at once, never each component on its own.
        RandomRotateFloat(g.xyz, rng);
    }

    DropRepeatedVertices(g.xyz, g.ptr);
    if (!check_sizes(g.ptr)) return std::nullopt;

    if (opt.exact_input)
    {
        if (auto why = InexactGrid(g.xyz))
        {
            LogError(*why);
            return std::nullopt;
        }
    }

    LinkEmb_T L(T_T(g.ptr.data(), static_cast<Int>(g.ptr.size())),
                T_T(g.colors.data(), static_cast<Int>(g.colors.size())));
    L.ReadVertexCoordinates(g.xyz.data());

    auto result = ProjectEmbedding(L);

    // The library's own account, which has to agree with the prediction that
    // InexactGrid was based on. If it ever does not, the library changed how
    // it picks its scale, and the check above has to follow it.
    if (opt.exact_input
        && (L.RoundingError() > Real(0)
            || L.ScalingFactor() != PredictedGridScale(g.xyz).first))
    {
        LogError("--exact-input: internal error: LinkEmbedding scaled by 2^"
                 + std::to_string(L.ScalingExponent()) + " with rounding error "
                 + std::to_string(L.RoundingError())
                 + ", not as predicted (tools/knoodle_io.hpp, PredictedGridScale)");
        return std::nullopt;
    }

    if (!result) LogError("3D geometry is not an embedding (it meets itself in 3-space)");
    return result;
}

/**
 * @brief Fill an InputKnot's 3D fields from the input and its projection.
 */
void StoreProjection(InputKnot& knot, const Geometry3D& g, Projection3D&& p)
{
    knot.had_3d_geometry    = true;
    knot.vertex_count_3d    = g.VertexCount();
    knot.component_count_3d = g.ComponentCount();
    knot.colored_3d         = (g.ComponentCount() > 1) || g.colors_declared;

    for (PD_T& pd : p.diagrams) { knot.summands.push_back(std::move(pd)); }
    knot.unknot_colors.insert(knot.unknot_colors.end(),
                              p.unknot_colors.begin(), p.unknot_colors.end());
}

/**
 * @brief Create a PlanarDiagram from PD code data.
 *
 * @param crossings Flat array of crossing data.
 * @param crossing_count Number of crossings.
 * @param format 4 for unsigned, 5 for signed, 6 for unsigned+colors, 7 for signed+colors.
 * @return The constructed PlanarDiagram.
 */
PD_T CreateDiagramFromPDCode(const std::vector<Int>& crossings,
                              Int crossing_count,
                              int format)
{
    switch (format)
    {
        case 4:
            return PD_T::FromUnsignedPDCode(
                crossings.data(), crossing_count, false, true
            );
        case 5:
            return PD_T::FromSignedPDCode(
                crossings.data(), crossing_count, false, true
            );
        case 6:
            return PD_T::template FromPDCode<{.signQ = false, .colorQ = true}>(
                crossings.data(), crossing_count, false, true
            );
        case 7:
            return PD_T::template FromPDCode<{.signQ = true, .colorQ = true}>(
                crossings.data(), crossing_count, false, true
            );
        default:
            return PD_T();
    }
}

/**
 * @brief Read a single knot from an input stream.
 *
 * Reads until EOF, a 'k' line, or stream error.
 *
 * In 3-column (3D) data a blank line ends a component, as in a .kndlxyz file,
 * and a '#color <int>' line at the start of a component sets its color (for
 * every component or for none; by default component k has color k). Any other
 * line starting with '#' is a comment, as is everything from a '%' on. In the
 * PD-code formats blank lines carry no meaning.
 *
 * @param input The input stream.
 * @param options How to project 3D input (--projection, --exact-input).
 * @param rng The tool's engine (--seed), for --projection=random.
 * @param source_name Description of the source (filename or "stdin").
 * @param[out] reached_eof Set to true if we hit EOF.
 * @return The parsed InputKnot, or nullopt on error.
 */
[[maybe_unused]] std::optional<InputKnot> ReadKnot(std::istream& input,
                                   const InputOptions& options,
                                   Knoodle::PRNG_T& rng,
                                   const std::string& source_name,
                                   bool& reached_eof)
{
    InputKnot result;
    result.source_description = source_name;
    reached_eof = false;

    // Current summand data
    Geometry3D         geometry;
    std::optional<Int> pending_color;            // from '#color', for the next component
    bool               undeclared_color = false; // some component had no '#color'
    std::vector<Int>   pd_crossings;
    Int                pd_crossing_count = 0;
    int                detected_format = 0;  // 0=unknown, 3=geometry, 4/5=PD code
    bool               in_summand = false;
    bool               first_data_seen = false;  // first *data* line (sets format)
    bool               saw_content = false;      // any 's' marker or data line

    // Vertices read since the last component ended.
    auto open_component_size = [&geometry]() -> Int
    {
        return geometry.VertexCount() - geometry.ptr.back();
    };

    auto close_component = [&]()
    {
        if (open_component_size() > 0)
        {
            geometry.ptr.push_back(geometry.VertexCount());
        }
    };

    // Lambda to finalize current summand
    auto finalize_summand = [&]() -> bool
    {
        if (!in_summand) return true;

        // An 's' marker with no data lines is an unknot summand
        // (knoodlesimplify writes unknots as bare 's' lines).
        if (geometry.xyz.empty() && pd_crossing_count == 0)
        {
            result.unknot_colors.push_back(PD_T::Uninitialized);
            in_summand = false;
            return true;
        }

        if (detected_format == 3)
        {
            // 3D geometry
            if (!geometry.xyz.empty())
            {
                if (result.had_3d_geometry || !result.summands.empty())
                {
                    LogError("3D geometry input cannot have multiple summands");
                    return false;
                }

                close_component();

                if (pending_color)
                {
                    LogError("'#color' line with no component after it");
                    return false;
                }
                if (geometry.colors_declared && undeclared_color)
                {
                    LogError("'#color' must be given for every component or for none");
                    return false;
                }

                auto projected = ProjectGeometry(geometry, options, rng);
                if (!projected)
                {
                    LogError("Failed to create diagram from 3D geometry");
                    return false;
                }

                StoreProjection(result, geometry, std::move(*projected));
                geometry = Geometry3D();
            }
        }
        else if (detected_format == 4 || detected_format == 5 ||
                 detected_format == 6 || detected_format == 7)
        {
            // PD code
            if (pd_crossing_count > 0)
            {
                PD_T pd = CreateDiagramFromPDCode(pd_crossings, pd_crossing_count, detected_format);
                if (!pd.ValidQ())
                {
                    LogError("Failed to create diagram from PD code");
                    return false;
                }

                result.summands.push_back(std::move(pd));
                pd_crossings.clear();
                pd_crossing_count = 0;
            }
        }

        in_summand = false;
        return true;
    };

    std::string line;

    std::vector<Real> values;
    values.reserve(7);

    while (std::getline(input, line))
    {
        // A truly blank line ends a 3D component (not a comment-only line,
        // which CleanLine also empties). Elsewhere blank lines mean nothing.
        if (Trim(line).empty())
        {
            if (detected_format == 3) { close_component(); }
            continue;
        }

        std::string cleaned = CleanLine(line);

        // Skip comment-only lines
        if (cleaned.empty()) continue;

        // '#color <int>' sets the color of the 3D component that follows, as
        // in a .kndlxyz file; any other '#' line is a comment.
        if (cleaned[0] == '#')
        {
            if (!cleaned.starts_with("#color")) continue;

            if (first_data_seen && detected_format != 3)
            {
                LogError("'#color' applies only to 3D geometry: " + cleaned);
                return std::nullopt;
            }
            if (open_component_size() > 0)
            {
                LogError("'#color' must start a component (after a blank line): " + cleaned);
                return std::nullopt;
            }
            if (pending_color)
            {
                LogError("Two '#color' lines for one component: " + cleaned);
                return std::nullopt;
            }

            const std::string value = Trim(std::string_view(cleaned).substr(6));
            try
            {
                std::size_t pos = 0;
                const long long c = std::stoll(value, &pos);
                if (pos != value.size()) { throw std::invalid_argument(value); }
                pending_color = static_cast<Int>(c);
            }
            catch (const std::exception&)
            {
                LogError("Malformed '#color' line: " + cleaned);
                return std::nullopt;
            }
            continue;
        }

        // Check for markers
        if (cleaned == "k" || cleaned == "K")
        {
            if (saw_content)
            {
                // End of this knot, finalize and return
                if (!finalize_summand()) return std::nullopt;
                break;
            }
            // First 'k' is optional, just continue
            continue;
        }

        if (cleaned == "s" || cleaned == "S")
        {
            // Start of new summand. This counts as knot content even if no
            // data lines follow (a bare 's' is an unknot summand), so a
            // later 'k' must terminate this knot rather than be skipped as
            // an optional leading marker.
            if (!finalize_summand()) return std::nullopt;
            in_summand = true;
            saw_content = true;
            continue;
        }

        // PlanarDiagramComplex's own native serialization -- 'u <color>' for a
        // colored unknot summand, 's <flag>' for a proven-minimal-tagged
        // summand (see src/PlanarDiagramComplex/{Write,Read}FromFile.hpp) --
        // is detected here (distinguished from the bare 's' marker above by
        // the trailing token) and delegated wholesale to PDC_T::FromInString
        // rather than reimplemented: colors -- including for unknot summands,
        // which the tools' own bare-'s' convention cannot represent -- round-
        // trip exactly as Henrik's reader/writer pair already defines them.
        if ((!cleaned.empty() && (cleaned[0] == 'u' || cleaned[0] == 'U')) ||
            cleaned.starts_with("s ") || cleaned.starts_with("S "))
        {
            if (!finalize_summand()) return std::nullopt;

            std::string pdc_text = cleaned + "\n";
            std::string more_line;
            bool saw_terminating_k = false;

            while (std::getline(input, more_line))
            {
                std::string more_cleaned = CleanLine(more_line);
                if (more_cleaned == "k" || more_cleaned == "K")
                {
                    saw_terminating_k = true;
                    break;
                }
                if (more_cleaned.empty()) continue;
                pdc_text += more_cleaned + "\n";
            }

            Tools::InString in_str(pdc_text);
            PDC_T pdc = PDC_T::FromInString(in_str);

            for (Int i = 0; i < pdc.DiagramCount(); ++i)
            {
                Knoodle::cref<PD_T> pd = pdc.Diagram(i);
                if (pd.AnelloQ())
                {
                    result.unknot_colors.push_back(pd.FirstColor());
                }
                else
                {
                    result.summands.push_back(PD_T(pd));
                }
            }

            // Equivalent richness to signed+colored PD code, for output-format
            // decisions downstream (e.g. knoodlesimplify's colored_output).
            detected_format = 7;
            first_data_seen = true;
            saw_content = true;
            in_summand = false;

            if (saw_terminating_k) { break; }
            continue;
        }

        // Parse as data line
        bool has_float = false;

        if (!ParseNumericLine(cleaned, values, has_float))
        {
            LogError("Failed to parse line: " + cleaned);
            return std::nullopt;
        }

        if (values.empty()) continue;

        // Detect format on first data line
        int line_format = DetectFormat(values, has_float);
        if (line_format == 0)
        {
            LogError("Unrecognized format (expected 3, 4, 5, 6, or 7 values): " + cleaned);
            return std::nullopt;
        }

        if (!first_data_seen)
        {
            detected_format = line_format;
            first_data_seen = true;
            saw_content = true;
            in_summand = true;
        }
        else if (line_format != detected_format)
        {
            // Format mismatch within same knot
            if (line_format == 3 || detected_format == 3)
            {
                LogError("Cannot mix 3D geometry and PD code formats");
                return std::nullopt;
            }
            // All PD lines must use the same column count
            LogError("Inconsistent PD code format (expected " +
                     std::to_string(detected_format) + " values per line, got " +
                     std::to_string(line_format) + ")");
            return std::nullopt;
        }

        // Store data
        if (detected_format == 3)
        {
            if (open_component_size() == 0)
            {
                // First vertex of a component: settle its color.
                if (pending_color)
                {
                    geometry.colors.push_back(*pending_color);
                    geometry.colors_declared = true;
                    pending_color.reset();
                }
                else
                {
                    geometry.colors.push_back(geometry.ComponentCount());
                    undeclared_color = true;
                }
            }
            for (Real v : values)
            {
                geometry.xyz.push_back(v);
            }
        }
        else
        {
            for (Real v : values)
            {
                pd_crossings.push_back(static_cast<Int>(v));
            }
            ++pd_crossing_count;
        }
    }

    // Check for EOF
    if (input.eof())
    {
        reached_eof = true;
    }

    // Finalize last summand
    if (!finalize_summand()) return std::nullopt;

    // Check if we got any data (a knot consisting only of unknot
    // summands — bare 's' lines — still counts as data)
    if (result.summands.empty() && result.unknot_colors.empty())
    {
        return std::nullopt;  // No data found
    }

    // Record input column format for output format decisions
    result.input_column_count = detected_format;

    // Compute crossing counts
    for (const auto& pd : result.summands)
    {
        Int cc = pd.CrossingCount();
        result.crossing_counts.push_back(cc);
        result.total_crossings += cc;
    }

    return result;
}

} // anonymous namespace
