/**
 * @file knoodleidentify.cpp
 * @brief knoodleidentify - identify knot diagrams via the KLUT (Knot LookUp Table).
 *
 * Reads RAW knot/link diagrams (PD codes or 3D polygons, as knoodlesimplify reads
 * them from a stream; a .kndlxyz file reads as it is) from stdin or files, runs the KLUT identify protocol on each
 * (decompose -> simplify -> escalate with Reapr -> look each prime summand up),
 * and emits one identification per input knot.
 *
 * Usage: generator | knoodleidentify        (parallel to: generator | knoodlesimplify)
 *
 * Identification is self-contained: this tool simplifies internally (it does NOT
 * consume knoodlesimplify output). It pass-reduces each diagram and looks it up;
 * a non-minimal result is escalated with Reapr until it reduces to a table key.
 *
 * Output (default): per knot, a Wolfram Language association from each distinct
 * prime summand to its multiplicity, e.g.
 *   <| KnotSymbol[3,1,True,"e/r"] -> 2, KnotSymbol[4,1,True,"e/m/r/mr"] -> 1 |>
 * Parses directly via ToExpression. An unknot yields <||>.
 *
 * Non-table summands appear as: Unidentified[N] (N above the table range: 13,
 * or --max-crossings), NotFound[N] (within the range yet unresolved even after
 * Reapr -- suspicious), and the whole input as Link[N] if it is multi-component
 * (the KLUT is knots-only). See --help for the option list.
 */

#include "knoodle_io.hpp"
#include "klut_identify.hpp"
#include "klut_table_set.hpp"
#include "sha256.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

// Platform primitive for "path to my own executable" (std::filesystem has none).
// These CLI tools target POSIX: macOS, Linux, and Linux-under-WSL2. Native Windows
// is deliberately NOT special-cased -- we don't pull in <windows.h>. The only context
// that runs the binary under native Windows is the Mathematica paclet, which always
// passes --data-dir explicitly, so exe-relative discovery is never needed there;
// SelfExecutablePath() just returns {} and resolution falls through to the overrides.
#if defined(__APPLE__)
    #include <mach-o/dyld.h>   // _NSGetExecutablePath
    #include <cstdint>         // std::uint32_t
#endif
// Linux/WSL2 reads /proc/self/exe via std::filesystem::read_symlink -- no <unistd.h>.

//==============================================================================
// Configuration
//==============================================================================

namespace {

using Klut = Knoodle::Klut;
namespace ki = klut_identify;

/**
 * @brief Configuration parsed from command-line arguments.
 */
struct Config
{
    std::optional<std::string> data_dir;     ///< KLUT data directory (if given)
    Int  max_crossings = static_cast<Int>(Klut::max_crossing_count);
    bool expanded       = false;             ///< '#'-joined per-summand output
    bool tsv            = false;             ///< Per-summand TSV output
    bool quiet          = false;             ///< Suppress stderr summary/warnings
    InputOptions input_options;              ///< --projection, --exact-input
    std::optional<std::uint64_t> seed;       ///< --seed (else one is drawn at startup)
    std::optional<Energy_T> reapr_energy;    ///< --reapr-energy (else Reapr's default)
    std::optional<double>   reapr_scaling;   ///< --reapr-scaling (else Reapr's default)
    std::optional<Int>      reapr_z_grid_size; ///< --reapr-z-grid-size (else Reapr's scaled levels)
    bool verify_tables  = false;             ///< Check the tables in full, report, read no input
    bool allow_other_tables = false;         ///< Accept a table set other than kKlutTableSet
    ki::Size_T escalation_rounds = ki::IdentifyParams{}.cap;      ///< Reapr escalation rounds per candidate
    Int        escalation_band   = ki::IdentifyParams{}.deep_cx;  ///< deep rounds only while stalled <= this
    ki::Size_T rotation_trials   = ki::IdentifyParams{}.rot;      ///< reprojections per embedding
    std::vector<std::string> input_files;    ///< Input file paths (empty = stdin)
    bool help_requested = false;
};

//==============================================================================
// Usage
//==============================================================================

void PrintUsage()
{
    std::cout <<
        "knoodleidentify - identify knot diagrams via the KLUT\n"
        "\n"
        "Usage: knoodleidentify [options] [input_files...]\n"
        "\n"
        "Examples:\n"
        "  generator | knoodleidentify        # identify a piped stream of diagrams\n"
        "  knoodleidentify diagrams.tsv       # identify diagrams from a file\n"
        "  knoodleidentify                    # read diagrams from the terminal (Ctrl-D)\n"
        "\n"
        "Reads RAW knot/link diagrams from stdin or the given files, 'k' between\n"
        "diagrams: PD codes (4-7 columns, as knoodlesimplify reads them) or 3D\n"
        "polygons (3 columns; a blank line starts a new link component, so a\n"
        ".kndlxyz file reads as it is). It runs the KLUT identify protocol on\n"
        "each (decompose, simplify, escalate with Reapr, look each prime summand\n"
        "up), and writes one line per knot.\n"
        "\n"
        "Self-contained: it simplifies internally -- feed it the SAME stream you\n"
        "would feed knoodlesimplify (generator | knoodleidentify), not the output\n"
        "of knoodlesimplify.\n"
        "\n"
        "Default output (one line per knot) is a Wolfram Language association from\n"
        "each distinct knot summand to its multiplicity, e.g.\n"
        "  <| KnotSymbol[3,1,True,\"e/r\"] -> 2, KnotSymbol[5,1,True,\"m/mr\"] -> 1 |>\n"
        "This parses directly via ToExpression. An unknot yields <||>. A summand we\n"
        "cannot place carries its PD code for offline analysis, e.g.\n"
        "  <| Unidentified[15, {{1,2,3,4,-1}, ...}] -> 1 |>\n"
        "\n"
        "Options:\n"
        "  --data-dir=PATH     KLUT data directory containing Klut_Keys_NN.bin\n"
        "                      and Klut_Values_NN.tsv. If this or $KNOODLE_KLUT_DIR\n"
        "                      is given, it is the only place looked (a wrong one\n"
        "                      is an error). Otherwise: ../share/knoodle/Klut,\n"
        "                      then ../data/Klut, both relative to the directory\n"
        "                      holding the real executable (symlinks resolved),\n"
        "                      then ./data/Klut with a warning. The stderr summary\n"
        "                      names the directory used. The tables are checked\n"
        "                      before any input is read: missing, unreadable,\n"
        "                      truncated or malformed files, Git LFS pointer\n"
        "                      files (run `git lfs pull` in a checkout), and files\n"
        "                      that disagree with Klut_Manifest.txt stop the run\n"
        "                      with exit status 1 and nothing on stdout.\n"
        "  --max-crossings=N   Use subtables up to N crossings (3-13, default 13).\n"
        "                      A knot with more crossings is reported as\n"
        "                      Unidentified, as it is above 13 by default.\n"
        "  --verify-tables     Check the KLUT tables in full, including the\n"
        "                      SHA-256 of every file against Klut_Manifest.txt,\n"
        "                      print one line saying so, and exit (0 if they\n"
        "                      pass). Reads no input. For install checks.\n"
        "  --allow-other-tables  Use tables from a different table set than the\n"
        "                      one this build expects (Klut_Manifest.txt names\n"
        "                      the set). Knot numbering can differ between sets.\n"
        "  --escalation-rounds=N  Max Reapr escalation rounds per candidate\n"
        "                      (default 4). Each round doubles embedding trials.\n"
        "                      The first 2 rounds always run; deeper rounds are\n"
        "                      banded (see --escalation-band), so a high N does\n"
        "                      not tax genuinely irreducible diagrams.\n"
        "  --escalation-band=C Rounds beyond the first 2 run only while the\n"
        "                      stalled diagram has <= C crossings (default 16 =\n"
        "                      table range + 3): a fixpoint near table range\n"
        "                      plausibly hides a table knot, one far above it\n"
        "                      does not. Set large to disable banding.\n"
        "  --reapr-rotation-trials=N  Reprojections per embedding during escalation\n"
        "                      (default 5). --rotation-trials=N is the same flag.\n"
        "  --reapr-energy=E    Reapr's energy for the escalation embeddings:\n"
        "                      " + ValidEnergies() + " (default: TV)\n"
        "  --reapr-scaling=X   Reapr's 3D grid scaling (default: 1.0)\n"
        "  --reapr-z-grid-size=N  Put consecutive levels of Reapr's embeddings N\n"
        "                      units apart (z = N*level; its x and y grid is 20\n"
        "                      units) instead of scaling them. Not with\n"
        "                      --reapr-scaling.\n"
        "  --expanded          One line per knot, summands joined by ' # ' (uses\n"
        "                      the raw K[...] table names).\n"
        "  --tsv               Per-summand output: knot_index, summand_index,\n"
        "                      crossing_count, name (tab-separated).\n"
        "  -q, --quiet         Suppress the stderr summary, the per-summand warnings\n"
        "                      and the reading-from-the-terminal notice. Errors\n"
        "                      still print, and a NotFound still makes the exit\n"
        "                      status nonzero.\n"
        "  --projection=P      How 3D input is projected (default: z): z, x or y\n"
        "                      projects down that axis (exact); random after a\n"
        "                      uniformly random rotation (exact for integer\n"
        "                      coordinates). 3D input only: a PD code is\n"
        "                      already a diagram, and is never embedded and\n"
        "                      re-viewed from another side (for that, pipe\n"
        "                      knoodledraw --embedding into --projection=x or y).\n"
        "  --randomize-projection  Same as --projection=random. (3-column input\n"
        "                      used to get a random shear instead, an oblique view\n"
        "                      at most ~35 degrees off the z-axis; it now gets a\n"
        "                      full rotation.)\n"
        "  --exact-input       Refuse 3D input that cannot be projected exactly:\n"
        "                      non-integer coordinates that do not land exactly on\n"
        "                      Knoodle's integer grid, or that --projection=random\n"
        "                      would have to rotate.\n"
        "  --seed=N            Seed every random choice (N in [0, 2^64)): the\n"
        "                      random projection and Reapr's escalation. The same\n"
        "                      seed repeats a run on the same build, except for\n"
        "                      Reapr's internal bend randomization.\n"
        "  -h, --help          Show this help.\n"
        "\n"
        "Knot symbols (default / --tsv); c=crossings, i=index, the third field is the\n"
        "alternating flag, last is the symmetry coset:\n"
        "  KnotSymbol[c,i,a,\"sym\"]  identified knot; a (alternating) is True/False\n"
        "  Unidentified[N,PD]       N above the table range (13, or --max-crossings);\n"
        "                           PD = signed PD code of the unresolved diagram\n"
        "  NotFound[N,PD]           N within the table range yet unresolved after\n"
        "                           Reapr (suspicious!); PD too; exit status 1\n"
        "  Link[N]                  multi-component input (the table is knots-only)\n"
        "  Invalid[]                invalid diagram / internal error\n"
        "  (unknot summands are the connect-sum identity and are omitted)\n"
        "\n"
        "Markers in --expanded mode (j is the same alternating flag, as 0/1):\n"
        "  K[c,i,j,\"sym\"], Unknot, <unidentified:N PD=...>, <notfound:N PD=...>,\n"
        "  <link:N>, <invalid>.\n";
}

//==============================================================================
// Argument Parsing
//==============================================================================

/**
 * @brief Parse a non-negative integer from a string view.
 */
std::optional<Int> ParseInt(std::string_view s)
{
    if (s.empty()) return std::nullopt;

    Int value = 0;
    for (char ch : s)
    {
        if (ch < '0' || ch > '9') return std::nullopt;
        value = value * 10 + (ch - '0');
    }
    return value;
}

// Every flag knoodleidentify accepts, in the preferred spelling PrintUsage()
// documents. CanonicalizeFlagSpelling maps case and separator variants onto
// these. A flag added to the parser belongs here and in PrintUsage();
// test/cli_contract_check.py checks all three agree.
constexpr std::string_view kKnownFlags[] = {
    "--help",
    "--data-dir",
    "--max-crossings",
    "--escalation-rounds",
    "--escalation-band",
    "--rotation-trials",
    "--reapr-rotation-trials",
    "--reapr-energy",
    "--reapr-scaling",
    "--reapr-z-grid-size",
    "--expanded",
    "--tsv",
    "--quiet",
    "--projection",
    "--randomize-projection",
    "--exact-input",
    "--seed",
    "--verify-tables",
    "--allow-other-tables",
};

std::optional<Config> ParseArguments(int argc, char* argv[])
{
    Config config;

    for (int i = 1; i < argc; ++i)
    {
        // Serve any spelling of a documented flag that differs only in case or
        // in '-'/'_' separators, by rewriting it to the spelling --help names.
        // Values, short flags and filenames pass through untouched.
        const std::string arg_text = CanonicalizeFlagSpelling(
            std::string_view(argv[i]), kKnownFlags);
        std::string_view arg(arg_text);

        if (arg == "-h" || arg == "--help")
        {
            config.help_requested = true;
            return config;
        }
        else if (arg.starts_with("--data-dir="))
        {
            config.data_dir = std::string(arg.substr(11));
        }
        else if (arg.starts_with("--escalation-rounds="))
        {
            auto parsed = ParseInt(arg.substr(20));
            if (!parsed || *parsed < 1 || *parsed > 64)
            {
                LogError("Invalid --escalation-rounds (expected 1-64)");
                return std::nullopt;
            }
            config.escalation_rounds = static_cast<ki::Size_T>(*parsed);
        }
        else if (arg.starts_with("--escalation-band="))
        {
            auto parsed = ParseInt(arg.substr(18));
            if (!parsed || *parsed < 0)
            {
                LogError("Invalid --escalation-band (expected >= 0)");
                return std::nullopt;
            }
            config.escalation_band = *parsed;
        }
        else if (arg.starts_with("--rotation-trials=")
                 || arg.starts_with("--reapr-rotation-trials="))
        {
            const std::string_view value = arg.substr(arg.find('=') + 1);
            auto parsed = ParseInt(value);
            if (!parsed || *parsed < 1 || *parsed > 1024)
            {
                LogError("Invalid --reapr-rotation-trials (expected 1-1024)");
                return std::nullopt;
            }
            config.rotation_trials = static_cast<ki::Size_T>(*parsed);
        }
        else if (arg.starts_with("--reapr-energy="))
        {
            const auto e = ParseEnergy(arg.substr(15));
            if (!e)
            {
                LogError("Unknown --reapr-energy value: '" + std::string(arg.substr(15))
                         + "' (valid: " + ValidEnergies() + ")");
                return std::nullopt;
            }
            config.reapr_energy = *e;
        }
        else if (arg.starts_with("--reapr-scaling="))
        {
            const std::string value(arg.substr(16));
            std::size_t used = 0;
            double x = 0;
            try { x = std::stod(value, &used); } catch (const std::exception&) { used = 0; }
            if (used == 0 || used != value.size() || !(x > 0) || !std::isfinite(x))
            {
                LogError("Invalid --reapr-scaling (expected a positive number): " + value);
                return std::nullopt;
            }
            config.reapr_scaling = x;
        }
        else if (arg.starts_with("--reapr-z-grid-size="))
        {
            const auto n = ParsePositiveInt(arg.substr(20));
            if (!n)
            {
                LogError("Invalid --reapr-z-grid-size (expected a positive integer): "
                         + std::string(arg.substr(20)));
                return std::nullopt;
            }
            config.reapr_z_grid_size = *n;
        }
        else if (arg.starts_with("--max-crossings="))
        {
            auto parsed = ParseInt(arg.substr(16));
            if (!parsed || *parsed < 3 ||
                *parsed > static_cast<Int>(Klut::max_crossing_count))
            {
                LogError("Invalid --max-crossings (expected 3-" +
                         std::to_string(Klut::max_crossing_count) + "): " +
                         std::string(arg));
                return std::nullopt;
            }
            config.max_crossings = *parsed;
        }
        else if (arg == "--expanded")
        {
            config.expanded = true;
        }
        else if (arg == "--tsv")
        {
            config.tsv = true;
        }
        else if (arg == "--quiet" || arg == "-q")
        {
            config.quiet = true;
        }
        else if (arg.starts_with("--projection="))
        {
            const auto p = ParseProjection(arg.substr(13));
            if (!p)
            {
                LogError("Unknown --projection value: '" + std::string(arg.substr(13))
                         + "' (valid: z, x, y, random)");
                return std::nullopt;
            }
            config.input_options.projection = *p;
        }
        else if (arg == "--randomize-projection")
        {
            config.input_options.projection = Projection_T::Random;
        }
        else if (arg == "--exact-input")
        {
            config.input_options.exact_input = true;
        }
        else if (arg.starts_with("--seed="))
        {
            const auto seed = ParseSeed(arg.substr(7));
            if (!seed)
            {
                LogError("Invalid --seed value: '" + std::string(arg.substr(7))
                         + "' (a decimal integer from 0 to 2^64 - 1)");
                return std::nullopt;
            }
            config.seed = *seed;
        }
        else if (arg == "--verify-tables")
        {
            config.verify_tables = true;
        }
        else if (arg == "--allow-other-tables")
        {
            config.allow_other_tables = true;
        }
        else if (arg.starts_with("-") && arg.size() > 1)
        {
            LogError("Unknown option: " + std::string(arg));
            LogError("Use --help for usage information");
            return std::nullopt;
        }
        else
        {
            config.input_files.push_back(std::string(arg));
        }
    }

    if (config.reapr_z_grid_size && config.reapr_scaling)
    {
        LogError("--reapr-z-grid-size and --reapr-scaling both set Reapr's z spacing;"
                 " use one or the other");
        return std::nullopt;
    }
    if (config.verify_tables && !config.input_files.empty())
    {
        LogError("--verify-tables checks the KLUT tables and reads no input; "
                 "run it without input files.");
        return std::nullopt;
    }

    return config;
}

//==============================================================================
// Data Directory Resolution
//==============================================================================

/**
 * @brief Absolute, canonical path to the running executable.
 *
 * std::filesystem has no portable "path to my own executable" primitive, and
 * argv[0] is unreliable: when the tool is launched by bare name off $PATH (the
 * normal installed case) argv[0] has no directory part, so canonicalizing it
 * resolves against the CWD rather than the real binary. Ask the OS instead.
 * Returns an empty path if the platform call fails.
 */
std::filesystem::path SelfExecutablePath()
{
    namespace fs = std::filesystem;
    std::error_code ec;

#if defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);          // query required buffer size
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0) { return {}; }
    // _NSGetExecutablePath may return a non-canonical path (symlinks, ..).
    fs::path p = fs::weakly_canonical(fs::path(buf.c_str()), ec);
    return ec ? fs::path(buf.c_str()) : p;
#elif defined(__linux__)
    // Native Linux and WSL2. read_symlink is pure std::filesystem, no <unistd.h>.
    fs::path link = fs::read_symlink("/proc/self/exe", ec);
    if (ec) { return {}; }
    fs::path p = fs::weakly_canonical(link, ec);
    return ec ? link : p;
#else
    // No exe-path primitive on this platform (e.g. native Windows). Resolution relies
    // on --data-dir / $KNOODLE_KLUT_DIR instead; the paclet always supplies --data-dir.
    (void)ec;
    return {};
#endif
}

/// The KLUT directory in use, and how it was chosen (for messages and the summary).
struct DataDir
{
    std::filesystem::path path;
    std::string           source;   ///< e.g. "--data-dir", "next to the executable"
};

/// What a candidate directory turned out to be.
enum class Probe { Found, Absent, Unreadable };

/**
 * @brief Does `dir` hold a KLUT? Never throws.
 *
 * The throwing std::filesystem::exists() turns "permission denied" into an
 * uncaught filesystem_error, i.e. abort() -- a crash, and a crash report on
 * macOS -- so use the error_code overload and report the error instead.
 * Not-a-directory and does-not-exist both come back as Absent.
 */
Probe ProbeDataDir(const std::filesystem::path& dir, std::string& why)
{
    std::error_code ec;
    const bool there = std::filesystem::exists(dir / "Klut_Values_03.tsv", ec);
    if (ec) { why = ec.message(); return Probe::Unreadable; }
    return there ? Probe::Found : Probe::Absent;
}

/**
 * @brief Resolve the KLUT data directory.
 *
 * An explicit choice is the only candidate, so a wrong one is an error rather
 * than a quiet switch to some other table set:
 *   1. --data-dir=PATH
 *   2. $KNOODLE_KLUT_DIR                       (if set and non-empty)
 * Otherwise, first hit wins:
 *   3. <exe_dir>/../share/knoodle/Klut         (FHS install: bin/ <-> ../share/)
 *   4. <exe_dir>/../data/Klut                  (in-tree build: tools/ <-> ../data/)
 *   5. KNOODLE_KLUT_DIR compile-time default   (baked-in prefix, if defined)
 *   6. ./data/Klut                             (CWD, dev convenience; warns)
 *
 * Candidates 3-4 are anchored to the *real* executable path, not argv[0] or the
 * CWD, so a from-$PATH invocation of an installed binary resolves correctly. A
 * candidate that exists but cannot be read stops the search with an error:
 * it means a broken install, not an absent one.
 */
std::optional<DataDir> ResolveDataDir(const Config& config)
{
    namespace fs = std::filesystem;

    std::string why;

    const auto only = [&why](const fs::path& dir, const std::string& source)
        -> std::optional<DataDir>
    {
        switch (ProbeDataDir(dir, why))
        {
            case Probe::Found:
                return DataDir{dir, source};
            case Probe::Unreadable:
                LogError("Cannot read the KLUT directory " + dir.string()
                         + " (given by " + source + "): " + why + ".");
                return std::nullopt;
            case Probe::Absent:
                LogError("Could not find KLUT tables in " + dir.string()
                         + " (given by " + source + "): there is no "
                           "Klut_Values_03.tsv there.");
                LogError("Point " + source + " at a directory holding "
                         "Klut_Keys_NN.bin and Klut_Values_NN.tsv (data/Klut "
                         "in a Knoodle checkout).");
                return std::nullopt;
        }
        return std::nullopt;
    };

    if (config.data_dir)
    {
        return only(*config.data_dir, "--data-dir");
    }
    if (const char* env = std::getenv("KNOODLE_KLUT_DIR"); env != nullptr && *env != '\0')
    {
        return only(env, "$KNOODLE_KLUT_DIR");
    }

    std::vector<DataDir> candidates;

    fs::path exe = SelfExecutablePath();
    if (!exe.empty() && exe.has_parent_path())
    {
        const fs::path exe_dir = exe.parent_path();
        // Installed layout: <prefix>/bin/knoodleidentify next to
        // <prefix>/share/knoodle/Klut (Homebrew keg, make install PREFIX=...).
        candidates.push_back({exe_dir.parent_path() / "share" / "knoodle" / "Klut",
                              "next to the executable"});
        // In-tree build: tools/knoodleidentify next to <repo>/data/Klut.
        candidates.push_back({exe_dir.parent_path() / "data" / "Klut",
                              "next to the executable"});
    }

#ifdef KNOODLE_KLUT_DIR
    // Optional build-time default (e.g. -DKNOODLE_KLUT_DIR=/usr/local/share/...).
    candidates.push_back({KNOODLE_KLUT_DIR, "the compiled-in default"});
#endif

    // Last-ditch dev convenience: relative to the current directory.
    candidates.push_back({fs::path("data") / "Klut", "the current directory"});

    for (const DataDir& cand : candidates)
    {
        switch (ProbeDataDir(cand.path, why))
        {
            case Probe::Found:
                if (cand.source == "the current directory")
                {
                    // Not an error, but an answer from whatever data/Klut happens
                    // to sit in the CWD (an old checkout, say) should not pass
                    // unannounced, so this prints even under --quiet.
                    std::error_code ec;
                    const fs::path abs = fs::absolute(cand.path, ec);
                    Log("knoodleidentify: warning: no KLUT next to this executable; "
                        "using " + (ec ? cand.path : abs).string()
                        + " from the current directory.");
                    return DataDir{ec ? cand.path : abs, cand.source};
                }
                return cand;
            case Probe::Unreadable:
            {
                std::error_code ec;
                const fs::path abs = fs::absolute(cand.path, ec);
                LogError("Cannot read the KLUT directory " + (ec ? cand.path : abs).string()
                         + " (" + cand.source + "): " + why + ".");
                return std::nullopt;
            }
            case Probe::Absent:
                break;
        }
    }

    LogError("Could not find KLUT data directory. Tried:");
    for (const DataDir& cand : candidates)
    {
        LogError("  " + cand.path.string());
    }
    LogError("Specify one with --data-dir=PATH or $KNOODLE_KLUT_DIR.");
    return std::nullopt;
}

//==============================================================================
// Table check: refuse damaged tables before reading any input
//==============================================================================
//
// Klut trusts its files. Its reader (src/Klut/Subtable.hpp) does not notice a
// missing or unreadable file until the first diagram of that size arrives, a
// count that fails to parse silently ends the table, and a key file that is one
// key short or has keys left over passes unremarked. Damage of that kind does
// not come back as an error: it comes back as NotFound, or as a wrong knot name
// -- a one-bit flip in one count shifts every later knot's keys and misnames
// 33,813 of them (docs/klut-table-failure-audit.md). So before anything is
// identified, check what can be checked without loading the tables: every file
// is there and readable, each values file parses strictly, and each keys file is
// exactly as long as its values file says. That costs a read of the ~0.85 MB of
// values files and a stat of each keys file.
//
// This does not catch damage that keeps the structure intact (a flipped digit
// in a knot's index, a flipped bit inside a key, a values file regenerated in a
// different order). Only a checksum can see those.

/**
 * @brief Is `name` a well-formed table name for crossing number `c`?
 *
 * K[c,i,j,"coset"]: c this subtable's crossing number; i >= 1 with no leading
 * zero; j the alternating flag, 0 or 1; coset a '/'-joined set of distinct
 * symmetries from e, r, m, mr.
 */
bool ValidKlutName(std::string_view name, Int c)
{
    const std::string head = "K[" + std::to_string(c) + ",";
    if (!name.starts_with(head) || !name.ends_with("\"]")) { return false; }

    std::string_view rest = name.substr(head.size(), name.size() - head.size() - 2);

    const std::size_t comma = rest.find(',');
    if (comma == std::string_view::npos) { return false; }
    const std::string_view index = rest.substr(0, comma);
    if (index.empty() || index.size() > 9 || index[0] == '0') { return false; }
    for (char ch : index) { if (ch < '0' || ch > '9') { return false; } }

    rest = rest.substr(comma + 1);                       // j,"coset
    if (rest.size() < 4 || (rest[0] != '0' && rest[0] != '1')
        || rest[1] != ',' || rest[2] != '"') { return false; }

    std::string_view coset = rest.substr(3);
    std::vector<std::string_view> seen;
    while (true)
    {
        const std::size_t slash = coset.find('/');
        const std::string_view part = coset.substr(0, slash);
        if (part != "e" && part != "r" && part != "m" && part != "mr") { return false; }
        if (std::find(seen.begin(), seen.end(), part) != seen.end()) { return false; }
        seen.push_back(part);
        if (slash == std::string_view::npos) { break; }
        coset = coset.substr(slash + 1);
    }
    return true;
}

/**
 * @brief Strictly parse one Klut_Values_NN.tsv.
 *
 * Every line is `name<TAB>count<LF>`, printable ASCII, with a valid name for
 * crossing number `c`, a positive count, and no name twice. On success fills
 * `names` and `counts` (in file order, so names[id] is knot id's name and
 * counts[id] the number of its keys) and `total` (the sum of the counts, i.e.
 * the number of keys the keys file must hold).
 *
 * @return "" on success, else what is wrong, with the line number.
 */
std::string ParseKlutValues(const std::string& text, const std::string& file, Int c,
                            std::vector<std::string>& names,
                            std::vector<std::uint64_t>& counts, std::uint64_t& total)
{
    if (text.empty()) { return file + ": empty"; }
    if (text.back() != '\n') { return file + ": does not end with a newline (truncated?)"; }

    std::map<std::string_view, std::size_t> first_line;
    std::size_t pos = 0;
    std::size_t line = 0;
    while (pos < text.size())
    {
        ++line;
        const std::size_t nl = text.find('\n', pos);
        const std::string_view l(text.data() + pos, nl - pos);
        pos = nl + 1;

        const std::string where = file + " line " + std::to_string(line) + ": ";

        for (std::size_t k = 0; k < l.size(); ++k)
        {
            const auto ch = static_cast<unsigned char>(l[k]);
            if ((ch < 0x21 || ch > 0x7e) && ch != '\t')
            {
                char hex[8];
                std::snprintf(hex, sizeof hex, "0x%02x", ch);
                return where + "unexpected byte " + hex + " at column "
                     + std::to_string(k + 1);
            }
        }

        const std::size_t tab = l.find('\t');
        if (tab == std::string_view::npos || l.find('\t', tab + 1) != std::string_view::npos)
        {
            return where + "expected `name<TAB>count`";
        }
        const std::string_view name  = l.substr(0, tab);
        const std::string_view count = l.substr(tab + 1);

        if (!ValidKlutName(name, c))
        {
            return where + "`" + std::string(name) + "` is not a K[" + std::to_string(c)
                 + ",i,j,\"coset\"] name";
        }

        bool count_ok = !count.empty() && count.size() <= 9 && count[0] != '0';
        for (char ch : count) { count_ok = count_ok && ch >= '0' && ch <= '9'; }
        if (!count_ok)
        {
            return where + "`" + std::string(count) + "` is not a key count";
        }

        if (const auto [it, fresh] = first_line.emplace(name, line); !fresh)
        {
            return where + std::string(name) + " also appears on line "
                 + std::to_string(it->second);
        }

        names.emplace_back(name);
        counts.push_back(std::stoull(std::string(count)));
        total += counts.back();
    }
    return "";
}

/// What the structural check learned about one subtable, for the later checks.
struct CheckedSubtable
{
    std::vector<std::string>   names;         ///< names[id], in file order
    std::vector<std::uint64_t> counts;        ///< counts[id]: keys listed for knot id
    std::uint64_t              keys        = 0;  ///< the sum of the counts
    std::uintmax_t             key_bytes   = 0;
    std::uintmax_t             value_bytes = 0;
    std::string                value_sha256;  ///< of the values file, lowercase hex
};
using CheckedTables = std::map<Int, CheckedSubtable>;

/// The advice that ends every refusal of a table directory.
void LogTableAdvice()
{
    LogError("Nothing will be identified from them. Reinstall Knoodle, or point "
             "--data-dir (or $KNOODLE_KLUT_DIR) at a complete copy of the tables "
             "(data/Klut in a Knoodle checkout, after `git lfs pull`).");
}

/// "The KLUT tables in DIR <what> (N problems):" and at most 12 of them.
void LogTableProblems(const std::filesystem::path& dir, const std::string& what,
                      const std::vector<std::string>& problems)
{
    LogError("The KLUT tables in " + dir.string() + " " + what + " ("
             + std::to_string(problems.size()) + " problem"
             + (problems.size() == 1 ? "" : "s") + "):");
    constexpr std::size_t shown = 12;
    for (std::size_t i = 0; i < problems.size() && i < shown; ++i)
    {
        LogError("  " + problems[i]);
    }
    if (problems.size() > shown)
    {
        LogError("  ... and " + std::to_string(problems.size() - shown) + " more");
    }
    LogTableAdvice();
}

/**
 * @brief Refuse a KLUT directory whose tables are missing, unreadable,
 *        Git LFS pointers, or structurally damaged.
 *
 * Checks the subtables up to max_crossings, the ones Klut will read. Runs
 * before any input is read, so a refusal leaves stdout empty.
 *
 * @param checked  On success, what was learned about each subtable, by
 *                 crossing number: the validated names (NameBook compares
 *                 Klut's own reading against them), the per-knot key counts
 *                 (the eager load checks Klut kept every key), the file sizes
 *                 and the values file's SHA-256 (the manifest check).
 * @return true if the tables pass; otherwise false, after saying which
 *         directory, which files, and what to do.
 */
bool KlutTablesAreSound(const std::filesystem::path& dir, Int max_crossings,
                        CheckedTables& checked)
{
    namespace fs = std::filesystem;

    // A checkout made without Git LFS (no git-lfs installed, GIT_LFS_SKIP_SMUDGE=1,
    // actions/checkout with lfs: false) holds ~130-byte text stubs in place of the
    // tables, each starting with this line. It is the commonest way to get bad
    // tables, so it gets its own message.
    static constexpr std::string_view lfs_header = "version https://git-lfs.github.com/spec/";

    std::vector<fs::path>    pointers;
    std::vector<std::string> problems;
    Int opened = 0;

    // Open one table file. Returns false (after noting why) unless it is a
    // readable regular file that is not an LFS pointer. `whole` reads all of it.
    const auto open = [&](const fs::path& p, bool whole, std::string& bytes,
                          std::uintmax_t& size) -> bool
    {
        const std::string file = p.filename().string();
        std::error_code ec;
        const fs::file_status st = fs::status(p, ec);
        if (st.type() == fs::file_type::not_found)
        {
            problems.push_back(file + ": missing");
            return false;
        }
        if (ec)
        {
            problems.push_back(file + ": cannot be examined: " + ec.message());
            return false;
        }
        if (!fs::is_regular_file(st))
        {
            problems.push_back(file + ": not a regular file");
            return false;
        }
        std::ifstream in(p, std::ios::binary);
        if (!in)
        {
            problems.push_back(file + ": cannot be opened for reading (check its permissions)");
            return false;
        }
        ++opened;
        size = fs::file_size(p, ec);
        if (ec)
        {
            problems.push_back(file + ": cannot get its size: " + ec.message());
            return false;
        }
        const std::size_t want = whole ? static_cast<std::size_t>(size) : lfs_header.size();
        bytes.assign(want, '\0');
        in.read(bytes.data(), static_cast<std::streamsize>(want));
        bytes.resize(static_cast<std::size_t>(in.gcount()));
        if (bytes.starts_with(lfs_header))
        {
            pointers.push_back(p);
            return false;
        }
        if (whole && bytes.size() != want)
        {
            problems.push_back(file + ": read only " + std::to_string(bytes.size())
                               + " of its " + std::to_string(want) + " bytes");
            return false;
        }
        return true;
    };

    for (Int c = 3; c <= max_crossings; ++c)
    {
        const std::string cc = (c < 10 ? "0" : "") + std::to_string(c);
        const fs::path keys   = dir / ("Klut_Keys_"   + cc + ".bin");
        const fs::path values = dir / ("Klut_Values_" + cc + ".tsv");

        std::string    key_head, value_text;
        std::uintmax_t key_size = 0, value_size = 0;
        const bool keys_ok   = open(keys,   false, key_head,   key_size);
        const bool values_ok = open(values, true,  value_text, value_size);
        if (!keys_ok || !values_ok) { continue; }

        std::vector<std::string> v;
        std::vector<std::uint64_t> counts;
        std::uint64_t total = 0;
        const std::string bad = ParseKlutValues(value_text, values.filename().string(), c,
                                                v, counts, total);
        if (!bad.empty())
        {
            problems.push_back(bad);
            continue;
        }

        const std::uint64_t want = total * static_cast<std::uint64_t>(c);
        if (key_size != want)
        {
            problems.push_back(keys.filename().string() + " holds " + std::to_string(key_size)
                + " bytes, but " + values.filename().string() + " lists " + std::to_string(total)
                + " keys of " + std::to_string(c) + " bytes (" + std::to_string(want)
                + " bytes): the pair is truncated or does not belong together");
            continue;
        }

        knoodle_sha256::Sha256 sha;
        sha.Update(value_text);
        checked[c] = CheckedSubtable{std::move(v), std::move(counts), total,
                                     key_size, value_size, sha.HexDigest()};
    }

    if (!pointers.empty())
    {
        LogError("The KLUT tables in " + dir.string() + " are Git LFS pointer files, "
                 "not tables (" + std::to_string(pointers.size()) + " of "
                 + std::to_string(opened) + " files"
                 + (pointers.size() == 1 ? ": " : ", e.g. ")
                 + pointers.front().filename().string() + ").");
        LogError("That happens when Knoodle is cloned without Git LFS. Install "
                 "git-lfs and run `git lfs pull` in the repository, or point "
                 "--data-dir (or $KNOODLE_KLUT_DIR) at a directory holding the "
                 "real tables.");
        return false;
    }

    if (problems.empty()) { return true; }

    LogTableProblems(dir, "are damaged or incomplete", problems);
    return false;
}

//==============================================================================
// Manifest check: the tables are the ones this build was made for
//==============================================================================
//
// The structural check above cannot see damage that keeps the structure: a
// changed digit in a knot's index, an alternating flag flipped, names
// reordered, a flipped bit in a key, or a whole table set regenerated with
// different numbering. data/Klut/Klut_Manifest.txt (scripts/klut-manifest.py)
// lists each table file's size, key count and SHA-256, and names the table
// set. Every run checks the sizes and counts, and the SHA-256 of the values
// files, which hold every name and count (0.85 MB, a few ms). --verify-tables
// also hashes the keys files (23 MB). And the set must be the one this build
// was made for (klut_table_set.hpp), unless --allow-other-tables.
//
// Tables without a manifest (from before Knoodle 1.1.1, or copied without it)
// get a warning, not an error, for now: the paclet's staged copies lack it
// until its build copies it too.

constexpr std::string_view kManifestName = "Klut_Manifest.txt";

struct ManifestEntry
{
    std::uintmax_t bytes = 0;
    std::uint64_t  keys  = 0;
    std::string    sha256;
};

struct Manifest
{
    std::string set_id;
    std::map<std::string, ManifestEntry> files;
};

std::string KeysFileName(Int c)
{
    return std::string("Klut_Keys_") + (c < 10 ? "0" : "") + std::to_string(c) + ".bin";
}

std::string ValuesFileName(Int c)
{
    return std::string("Klut_Values_") + (c < 10 ? "0" : "") + std::to_string(c) + ".tsv";
}

bool IsLowerHex(std::string_view s, std::size_t n)
{
    if (s.size() != n) { return false; }
    for (char ch : s)
    {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) { return false; }
    }
    return true;
}

/// A decimal count with no sign and no leading zero (except "0" itself).
std::optional<std::uint64_t> ParseCount(std::string_view s)
{
    if (s.empty() || s.size() > 18 || (s.size() > 1 && s[0] == '0')) { return std::nullopt; }
    std::uint64_t v = 0;
    for (char ch : s)
    {
        if (ch < '0' || ch > '9') { return std::nullopt; }
        v = v * 10 + static_cast<std::uint64_t>(ch - '0');
    }
    return v;
}

/**
 * @brief Strictly parse Klut_Manifest.txt (format 1; see scripts/klut-manifest.py).
 * @return "" on success, else what is wrong.
 */
std::string ParseManifest(const std::string& text, Manifest& m)
{
    if (text.empty()) { return "it is empty"; }
    if (text.back() != '\n') { return "it does not end with a newline (truncated?)"; }

    std::set<std::string> table_names;
    for (Int c = 3; c <= static_cast<Int>(Klut::max_crossing_count); ++c)
    {
        table_names.insert(KeysFileName(c));
        table_names.insert(ValuesFileName(c));
    }

    bool have_version = false;
    bool have_set     = false;
    knoodle_sha256::Sha256 file_lines;   // the set id is the hash of these
    std::size_t pos  = 0;
    std::size_t line = 0;
    while (pos < text.size())
    {
        ++line;
        const std::size_t nl = text.find('\n', pos);
        const std::string_view l(text.data() + pos, nl - pos);
        pos = nl + 1;
        if (!l.empty() && l[0] == '#') { continue; }

        const std::string where = "line " + std::to_string(line) + ": ";
        std::vector<std::string_view> f;
        for (std::size_t a = 0; a <= l.size();)
        {
            const std::size_t b = std::min(l.find(' ', a), l.size());
            f.push_back(l.substr(a, b - a));
            a = b + 1;
        }
        for (std::string_view x : f)
        {
            if (x.empty()) { return where + "fields must be separated by single spaces"; }
        }

        if (!have_version)
        {
            if (f.size() != 2 || f[0] != "klut-manifest" || !ParseCount(f[1]))
            {
                return where + "expected `klut-manifest 1`";
            }
            if (f[1] != "1")
            {
                return where + "it is format " + std::string(f[1]) + ", and this "
                       "knoodleidentify reads format 1 (it is older than the tables)";
            }
            have_version = true;
            continue;
        }
        if (!have_set)
        {
            if (f.size() != 2 || f[0] != "set" || !IsLowerHex(f[1], 16))
            {
                return where + "expected `set` and 16 hex digits";
            }
            m.set_id = std::string(f[1]);
            have_set = true;
            continue;
        }

        const auto bytes = (f.size() == 4) ? ParseCount(f[1]) : std::nullopt;
        const auto keys  = (f.size() == 4) ? ParseCount(f[2]) : std::nullopt;
        if (!bytes || !keys || !IsLowerHex(f[3], 64))
        {
            return where + "expected `<file> <bytes> <keys> <sha256>`";
        }
        const std::string name(f[0]);
        if (!table_names.contains(name))
        {
            return where + "`" + name + "` is not a KLUT table file name";
        }
        if (!m.files.emplace(name, ManifestEntry{*bytes, *keys, std::string(f[3])}).second)
        {
            return where + name + " is listed twice";
        }
        file_lines.Update(l);
        file_lines.Update("\n");
    }
    if (!have_version) { return "there is no `klut-manifest` line"; }
    if (!have_set)     { return "there is no `set` line"; }
    if (file_lines.HexDigest().substr(0, 16) != m.set_id)
    {
        return "its `set` line does not match its file lines (edited by hand?)";
    }
    return "";
}

/**
 * @brief Check the tables against Klut_Manifest.txt, if there is one.
 *
 * @param set_id  Set to the manifest's table set, or left empty without one.
 * @return true to go on; false after explaining why not.
 */
bool KlutTablesMatchManifest(const std::filesystem::path& dir, Int max_crossings,
                             const CheckedTables& checked, const Config& config,
                             std::string& set_id)
{
    namespace fs = std::filesystem;

    const fs::path path = dir / kManifestName;
    std::error_code ec;
    const fs::file_status st = fs::status(path, ec);
    if (st.type() == fs::file_type::not_found)
    {
        if (config.verify_tables)
        {
            LogError("Cannot verify the KLUT tables in " + dir.string() + ": there is no "
                     + std::string(kManifestName) + " to check them against. (Tables "
                     "from before Knoodle 1.1.1 have none; data/Klut in a current "
                     "Knoodle checkout or release does.)");
            return false;
        }
        if (!config.quiet)
        {
            Log("knoodleidentify: warning: there is no " + std::string(kManifestName)
                + " in " + dir.string() + ", so only the tables' structure was "
                "checked, not their contents or version.");
        }
        return true;
    }

    std::string text;
    {
        std::ifstream in(path, std::ios::binary);
        if (ec || !fs::is_regular_file(st) || !in)
        {
            LogError("Cannot read the KLUT manifest " + path.string()
                     + (ec ? ": " + ec.message() : std::string()) + ".");
            LogTableAdvice();
            return false;
        }
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    Manifest m;
    if (const std::string bad = ParseManifest(text, m); !bad.empty())
    {
        LogError("The KLUT manifest " + path.string() + " is damaged: " + bad + ".");
        LogTableAdvice();
        return false;
    }

    std::vector<std::string> problems;
    const auto compare = [&](const std::string& name, std::uintmax_t bytes,
                             std::uint64_t keys, const std::string& sha)
    {
        const auto it = m.files.find(name);
        if (it == m.files.end())
        {
            problems.push_back(name + ": not listed in the manifest");
            return;
        }
        const ManifestEntry& e = it->second;
        if (bytes != e.bytes)
        {
            problems.push_back(name + ": " + std::to_string(bytes) + " bytes, but the "
                               "manifest says " + std::to_string(e.bytes));
        }
        else if (keys != e.keys)
        {
            problems.push_back(name + ": " + std::to_string(keys) + " keys, but the "
                               "manifest says " + std::to_string(e.keys));
        }
        else if (!sha.empty() && sha != e.sha256)
        {
            problems.push_back(name + ": its contents are not the ones the manifest "
                               "lists (SHA-256 differs)");
        }
    };

    for (Int c = 3; c <= max_crossings; ++c)
    {
        const CheckedSubtable& t = checked.at(c);
        std::string key_sha;
        if (config.verify_tables)
        {
            std::string err;
            const auto hex = knoodle_sha256::FileHexDigest(dir / KeysFileName(c), err);
            if (!hex)
            {
                problems.push_back(KeysFileName(c) + ": could not be read in full: " + err);
                continue;
            }
            key_sha = *hex;
        }
        compare(KeysFileName(c),   t.key_bytes,   t.key_bytes / static_cast<std::uintmax_t>(c), key_sha);
        compare(ValuesFileName(c), t.value_bytes, t.keys, t.value_sha256);
    }
    if (!problems.empty())
    {
        LogTableProblems(dir, "do not match their manifest, " + std::string(kManifestName),
                         problems);
        return false;
    }

    set_id = m.set_id;
    if (m.set_id != kKlutTableSet)
    {
        if (!config.allow_other_tables)
        {
            LogError("The KLUT tables in " + dir.string() + " are table set " + m.set_id
                     + ", but this knoodleidentify was built for table set "
                     + std::string(kKlutTableSet) + ".");
            LogError("Knot numbering can differ between table sets, so names from these "
                     "tables could be wrong. Use the tables from the same Knoodle release "
                     "as this program, or pass --allow-other-tables if you know they are "
                     "compatible.");
            return false;
        }
        if (!config.quiet)
        {
            Log("knoodleidentify: warning: using KLUT table set " + m.set_id
                + " (this build expects " + std::string(kKlutTableSet)
                + "), as --allow-other-tables asks.");
        }
    }
    return true;
}

//==============================================================================
// Name resolution: (crossing_count, id) -> K[c,i,j,"sym"]
//==============================================================================

/**
 * @brief Turns Identify's (crossing_count, subtable id) into the table's name.
 *
 * The id is the knot's 0-based line in Klut_Values_cc.tsv as Klut read it, so
 * the names come from that same reading (Klut::SubtableNames), never from a
 * second parse of the file. Klut reads a subtable only when it is first used,
 * possibly long after KlutTablesAreSound checked it; the first time each
 * subtable is used, its names are compared with the checked ones, and a
 * difference -- the files changed while we ran -- is a table error.
 */
class NameBook
{
public:
    NameBook(Klut& klut, const CheckedTables& checked, std::string dir)
    :   klut_(klut), checked_(checked), dir_(std::move(dir))
    {}

    /// Does the c-crossing subtable Klut read still match the one checked at
    /// startup? Loads it if need be. Logs and returns false if not.
    bool Verify(Int c)
    {
        // Outside the loaded range there is nothing to compare (and
        // Klut::SubtableNames does not bounds-check).
        if (c < Int(3) || static_cast<Knoodle::Size_T>(c) > klut_.CrossingCount()) { return true; }
        if (loaded_.contains(c)) { return true; }

        std::vector<std::string> read = klut_.SubtableNames(c);
        const auto chk = checked_.find(c);
        if (chk == checked_.end() || read != chk->second.names)
        {
            LogError("The " + std::to_string(c) + "-crossing KLUT table in " + dir_
                     + " changed after knoodleidentify started: its names no longer "
                       "match the ones checked at startup.");
            return false;
        }
        loaded_.emplace(c, std::move(read));
        return true;
    }

    /// The name of knot `id` among the c-crossing knots, or nullopt after
    /// logging a table error.
    std::optional<std::string> Name(Int c, Klut::ID_T id)
    {
        if (!Verify(c)) { return std::nullopt; }
        const auto it = loaded_.find(c);
        if (it == loaded_.end() || static_cast<std::size_t>(id) >= it->second.size())
        {
            LogError("Internal error: the " + std::to_string(c) + "-crossing KLUT table "
                     "returned id " + std::to_string(id) + ", which it does not list.");
            return std::nullopt;
        }
        return it->second[id];
    }

    const std::string& Dir() const { return dir_; }

private:
    Klut& klut_;
    const CheckedTables& checked_;
    std::map<Int, std::vector<std::string>> loaded_;
    std::string dir_;
};

/**
 * @brief Load every subtable now, and check that Klut kept every key.
 *
 * Klut reads a subtable on first use. Loading them all here, before any input
 * is read, means a load failure can never cut the output off halfway. It also
 * catches what no file-level check sees: a key listed twice, under one knot or
 * under two. Subtable::Read refuses that itself now (an ERROR naming both
 * knots); before, it kept the first key and dropped the second, which shrank
 * the table and could name a knot as the wrong chirality. The per-knot count
 * below stays as a cross-check that does not depend on the library: every knot
 * must keep exactly the number of keys its line in the values file lists.
 * Costs about 55 ms for the full table.
 */
bool KlutTablesLoad(Klut& klut, const std::filesystem::path& dir,
                    const CheckedTables& checked, NameBook& names)
{
    const long errors_before = (g_cerr_tap != nullptr) ? g_cerr_tap->Count() : 0;
    klut.RequireSubtables();
    if (g_cerr_tap != nullptr && g_cerr_tap->Count() > errors_before)
    {
        LogError("The KLUT tables in " + dir.string() + " are damaged: Klut refused "
                 "to load them (see the ERROR lines above).");
        LogTableAdvice();
        return false;
    }

    std::vector<std::string> problems;
    for (const auto& [c, t] : checked)
    {
        if (!names.Verify(c)) { return false; }

        const auto ids = klut.SubtableIDs(c);
        std::vector<std::uint64_t> kept(t.names.size(), 0);
        bool stray = false;
        for (Knoodle::Size_T i = 0; i < ids.Size(); ++i)
        {
            const auto id = static_cast<std::size_t>(ids[i]);
            if (id < kept.size()) { ++kept[id]; } else { stray = true; }
        }
        if (stray)
        {
            problems.push_back(KeysFileName(c) + ": Klut read a knot id that "
                               + ValuesFileName(c) + " does not list");
            continue;
        }
        const auto held = static_cast<std::uint64_t>(ids.Size());
        if (held == t.keys) { continue; }

        std::string who;
        std::size_t shown = 0;
        for (std::size_t id = 0; id < kept.size(); ++id)
        {
            if (kept[id] == t.counts[id]) { continue; }
            if (shown < 3)
            {
                who += (shown ? ", " : "") + t.names[id] + " kept " + std::to_string(kept[id])
                     + " of " + std::to_string(t.counts[id]);
            }
            ++shown;
        }
        const std::uint64_t lost = t.keys - held;
        problems.push_back(KeysFileName(c) + ": " + std::to_string(lost) + " of its "
                           + std::to_string(t.keys) + " keys "
                           + (lost == 1 ? "duplicates an earlier key" : "duplicate earlier keys")
                           + ", and Klut keeps only the first (" + who
                           + (shown > 3 ? ", ..." : "") + ")");
    }
    if (!problems.empty())
    {
        LogTableProblems(dir, "are damaged", problems);
        return false;
    }
    return true;
}

//==============================================================================
// Identification
//==============================================================================

/**
 * @brief Tallies across all processed knots, for the stderr summary.
 */
struct Stats
{
    Int knots        = 0;
    Int summands     = 0;
    Int identified   = 0;
    Int unknots      = 0;
    Int not_found    = 0;
    Int over_range   = 0;
    Int links        = 0;
    Int invalid      = 0;
};

/// What a single summand resolved to.
enum class Kind { Identified, Unknot, OverRange, NotFound, Link, Invalid };

/**
 * @brief Classification of one summand: enough to render any output form and
 *        to sort it deterministically within a knot's multiset.
 */
struct Summand
{
    Kind        kind        = Kind::Invalid;
    Int         crossings   = 0;

    // Valid only when kind == Identified (parsed from the K[c,i,j,"sym"] name):
    Int         knot_index  = 0;     ///< i
    bool        alternating = false; ///< j: alternating flag (0/1 -> False/True)
    std::string coset;               ///< "sym" *including* the surrounding quotes
    std::string raw_name;            ///< the original K[...] string

    // Valid only when kind == OverRange / NotFound: the flattened signed PD code
    // (5 cols/crossing) of the diagram we could not identify, so a failure is
    // recoverable for offline analysis. PD code is standard and survives any size.
    std::vector<Int> pd_code;
};

/// Format a flattened signed PD code as a Wolfram nested list { {a,b,c,d,e}, ... }.
std::string PDCodeToWL(const std::vector<Int>& pd)
{
    std::string out = "{";
    const std::size_t rows = pd.size() / 5;
    for (std::size_t r = 0; r < rows; ++r)
    {
        out += (r ? ",{" : "{");
        for (int j = 0; j < 5; ++j)
        {
            if (j) { out += ","; }
            out += std::to_string(pd[5 * r + j]);
        }
        out += "}";
    }
    out += "}";
    return out;
}

/**
 * @brief Parse a raw table name "K[c,i,j,\"sym\"]" into the fields of a Summand.
 */
void ParseRawName(const std::string& raw, Summand& s)
{
    s.raw_name = raw;

    if (!raw.starts_with("K[") || raw.back() != ']') { return; }

    const std::string inner = raw.substr(2, raw.size() - 3);  // c,i,j,"sym"

    const std::size_t c1 = inner.find(',');
    const std::size_t c2 = (c1 == std::string::npos) ? c1 : inner.find(',', c1 + 1);
    const std::size_t c3 = (c2 == std::string::npos) ? c2 : inner.find(',', c2 + 1);
    if (c3 == std::string::npos) { return; }

    const std::string f_idx = inner.substr(c1 + 1, c2 - c1 - 1);
    const std::string f_amp = inner.substr(c2 + 1, c3 - c2 - 1);

    if (auto v = ParseInt(f_idx)) { s.knot_index = *v; }
    s.alternating = (f_amp == "1");
    s.coset = inner.substr(c3 + 1);  // keeps the surrounding quotes
}

/**
 * @brief Wolfram Language symbol for a summand (default and --tsv output).
 *        Unknots have no symbol (connect-sum identity); returns "" for them.
 */
std::string WLSymbol(const Summand& s)
{
    switch (s.kind)
    {
        case Kind::Identified:
            return "KnotSymbol[" + std::to_string(s.crossings) + "," +
                   std::to_string(s.knot_index) + "," +
                   (s.alternating ? "True" : "False") + "," + s.coset + "]";
        case Kind::Unknot:    return "";
        case Kind::OverRange: return s.pd_code.empty()
                                     ? "Unidentified[" + std::to_string(s.crossings) + "]"
                                     : "Unidentified[" + std::to_string(s.crossings) + ", "
                                       + PDCodeToWL(s.pd_code) + "]";
        case Kind::NotFound:  return s.pd_code.empty()
                                     ? "NotFound[" + std::to_string(s.crossings) + "]"
                                     : "NotFound[" + std::to_string(s.crossings) + ", "
                                       + PDCodeToWL(s.pd_code) + "]";
        case Kind::Link:      return "Link[" + std::to_string(s.crossings) + "]";
        case Kind::Invalid:   return "Invalid[]";
    }
    return "Invalid[]";
}

/**
 * @brief Human marker for a summand (--expanded output).
 */
std::string ExpandedSymbol(const Summand& s)
{
    switch (s.kind)
    {
        case Kind::Identified: return s.raw_name;
        case Kind::Unknot:     return "Unknot";
        case Kind::OverRange:  return "<unidentified:" + std::to_string(s.crossings)
                                    + (s.pd_code.empty() ? "" : " PD=" + PDCodeToWL(s.pd_code)) + ">";
        case Kind::NotFound:   return "<notfound:" + std::to_string(s.crossings)
                                    + (s.pd_code.empty() ? "" : " PD=" + PDCodeToWL(s.pd_code)) + ">";
        case Kind::Link:       return "<link:" + std::to_string(s.crossings) + ">";
        case Kind::Invalid:    return "<invalid>";
    }
    return "<invalid>";
}

/// Deterministic ordering of distinct summand kinds within a knot's multiset.
bool SummandLess(const Summand& a, const Summand& b)
{
    auto group = [](const Summand& s) -> int
    {
        switch (s.kind)
        {
            case Kind::Identified: return 0;
            case Kind::OverRange:  return 1;
            case Kind::NotFound:   return 2;
            case Kind::Link:       return 3;
            case Kind::Invalid:    return 4;
            case Kind::Unknot:     return 5;  // never aggregated, but define it
        }
        return 6;
    };

    const int ga = group(a);
    const int gb = group(b);

    // pd_code last: distinct failed diagrams (same crossing count) stay distinct
    // keys in the multiset rather than collapsing and losing one diagram's code.
    return std::tie(ga, a.crossings, a.knot_index, a.alternating, a.coset, a.pd_code)
         < std::tie(gb, b.crossings, b.knot_index, b.alternating, b.coset, b.pd_code);
}

/// Convert one Identify result-summand to a render Summand, resolving its name.
/// nullopt means the name could not be had from the table (already logged).
std::optional<Summand> ConvertSummand(const ki::Summand& rs, NameBook& names, Stats& stats)
{
    Summand s;
    s.crossings = rs.crossings;
    switch (rs.kind)
    {
        case ki::Summand::Kind::Identified:
        {
            const std::optional<std::string> name = names.Name(rs.crossings, rs.id);
            if (!name) { return std::nullopt; }
            s.kind = Kind::Identified;
            ParseRawName(*name, s);
            ++stats.identified;
            break;
        }
        case ki::Summand::Kind::Unidentified:
            s.kind = Kind::OverRange;
            s.pd_code = rs.pd_code;
            ++stats.over_range;
            break;
        case ki::Summand::Kind::Error:
            s.kind = Kind::NotFound;
            s.pd_code = rs.pd_code;
            ++stats.not_found;
            break;
    }
    return s;
}

/// How ProcessStream ended.
enum class StreamEnd
{
    Done,        ///< read to the end
    ParseError,  ///< bad input; the caller moves on to the next file
    TableError,  ///< the KLUT failed mid-run; the caller stops everything
};

/**
 * @brief Process one input stream; writes identifications to stdout.
 */
StreamEnd ProcessStream(std::istream& input, const std::string& source_name,
                        const Config& config, Klut& klut, NameBook& names,
                        ki::Reapr_T& reapr, Stats& stats, Knoodle::PRNG_T& rng)
{
    bool reached_eof = false;

    while (!reached_eof)
    {
        auto input_knot = ReadKnot(input, config.input_options, rng, source_name, reached_eof);

        if (!input_knot)
        {
            if (reached_eof) { continue; }
            return StreamEnd::ParseError;
        }

        ++stats.knots;

        // Build a PDC from the raw input diagram(s). Raw input is normally a
        // single diagram; if pre-split summands arrive, Identify re-decomposes
        // their union. Bare unknot summands carry no diagram and are the identity.
        ki::PDC_T pdc;
        // Push() is lock-guarded and silently does nothing on a locked complex,
        // which would leave pdc empty and make every input look like an unknot.
        // Feeding it diagrams that ReadKnot just produced from one input record
        // is the sanctioned use, so unlock for the duration.
        pdc.Unlock();
        for (PD_T& pd : input_knot->summands)
        {
            if (pd.ValidQ()) { pdc.Push(std::move(pd)); }
        }
        const Int input_crossings =
            (pdc.DiagramCount() > Int(0)) ? pdc.CrossingCount() : Int(0);

        // 3D input knows its component count exactly. A split link -- a knot
        // beside a distant circle, say -- projects to the knot's diagram plus
        // a crossingless component, and the crossingless one carries no
        // diagram for Identify to count, so it would pass for the knot.
        const bool link_3d = input_knot->had_3d_geometry
                          && input_knot->component_count_3d > Int(1);

        ki::IdentifyResult res;

        if (link_3d)
        {
            res.status = ki::IdentifyResult::Status::LinkOutOfScope;
        }
        else
        {
            ki::IdentifyParams params;
            params.cap     = config.escalation_rounds;
            params.deep_cx = config.escalation_band;
            params.rot     = config.rotation_trials;
            // A knot above --max-crossings is over this run's table range, so it is
            // Unidentified, not a "table gap" NotFound.
            params.max_cx  = config.max_crossings;

            res = ki::Identify(klut, std::move(pdc), reapr, params);
        }

        // A subtable that would not load is the table's fault, not the
        // diagram's: report it as such rather than as NotFound, and print
        // nothing more, since every later answer would come from the same
        // broken table.
        if (res.table_error)
        {
            LogError("The " + std::to_string(res.table_error_crossings)
                     + "-crossing KLUT table in " + names.Dir() + " failed to load "
                       "(see above) while identifying knot " + std::to_string(stats.knots)
                     + ". Stopping.");
            return StreamEnd::TableError;
        }

        std::vector<Summand> summands;

        if (res.status == ki::IdentifyResult::Status::LinkOutOfScope)
        {
            Summand s; s.kind = Kind::Link; s.crossings = input_crossings;
            summands.push_back(s);
            ++stats.summands; ++stats.links;
        }
        else
        {
            if (res.component_error && !config.quiet)
            {
                LogError("knot " + std::to_string(stats.knots) +
                         ": Simplify changed the link-component count (a bug) — "
                         "identification suspect");
            }
            for (const ki::Summand& rs : res.summands)
            {
                std::optional<Summand> s = ConvertSummand(rs, names, stats);
                // A miss is only a miss if the table is still the one checked
                // at startup: one truncated mid-run would also miss.
                if (s && s->kind == Kind::NotFound && !names.Verify(s->crossings))
                {
                    s.reset();
                }
                if (!s)
                {
                    LogError("Stopping at knot " + std::to_string(stats.knots) + ".");
                    return StreamEnd::TableError;
                }
                summands.push_back(std::move(*s));
                ++stats.summands;
            }
            // Empty (and not a link) means everything reduced away: the unknot.
            if (summands.empty())
            {
                summands.push_back(Summand{.kind = Kind::Unknot});
                ++stats.summands; ++stats.unknots;
            }
        }

        if (config.tsv)
        {
            for (std::size_t i = 0; i < summands.size(); ++i)
            {
                const Summand& s = summands[i];
                const std::string sym =
                    (s.kind == Kind::Unknot) ? "Unknot" : WLSymbol(s);
                std::cout << stats.knots << '\t' << (i + 1) << '\t'
                          << s.crossings << '\t' << sym << '\n';
            }
        }
        else if (config.expanded)
        {
            // '#'-joined, arrival order. Unknot is the connect-sum identity:
            // show it only when there is nothing else.
            std::vector<std::string> shown;
            for (const Summand& s : summands)
            {
                if (s.kind != Kind::Unknot) { shown.push_back(ExpandedSymbol(s)); }
            }
            if (shown.empty()) { shown.push_back("Unknot"); }

            for (std::size_t i = 0; i < shown.size(); ++i)
            {
                if (i > 0) { std::cout << " # "; }
                std::cout << shown[i];
            }
            std::cout << '\n';
        }
        else
        {
            // Default: a Wolfram Language association mapping each distinct
            // (non-unknot) summand to its multiplicity, keys sorted.
            std::map<Summand, Int, bool(*)(const Summand&, const Summand&)>
                multiset(SummandLess);

            for (const Summand& s : summands)
            {
                if (s.kind == Kind::Unknot) { continue; }  // identity
                ++multiset[s];
            }

            std::cout << "<|";
            bool first = true;
            for (const auto& [s, count] : multiset)
            {
                std::cout << (first ? " " : ", ")
                          << WLSymbol(s) << " -> " << count;
                first = false;
            }
            std::cout << (first ? "|>" : " |>") << '\n';
        }

        std::cout << std::flush;

        for (std::size_t i = 0; i < summands.size(); ++i)
        {
            if (summands[i].kind != Kind::NotFound) { continue; }
            if (config.quiet)
            {
                // --quiet hides the message, not the verdict: NotFound still
                // makes the exit status nonzero.
                ++g_tool_error_count;
                continue;
            }
            LogError("knot " + std::to_string(stats.knots) +
                     ", summand " + std::to_string(i + 1) + ": " +
                     std::to_string(summands[i].crossings) +
                     " crossings, <=" + std::to_string(config.max_crossings) +
                     " yet unresolved after Reapr — "
                     "table gap or a hard simplification case?");
        }
    }

    return StreamEnd::Done;
}

} // anonymous namespace

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[])
{
    // Count the library's "ERROR: " lines for the whole run, so an identification
    // made from a diagram the core has disclaimed cannot be reported as success.
    // knoodleidentify writes only to stdout, so the nonzero exit and the notice
    // below are the whole contract -- there is no file to withhold.
    CerrErrorTap cerr_tap;
    g_cerr_tap = &cerr_tap;

    auto config_opt = ParseArguments(argc, argv);
    if (!config_opt)
    {
        return EXIT_FAILURE;
    }

    Config config = *config_opt;

    if (config.help_requested)
    {
        PrintUsage();
        return EXIT_SUCCESS;
    }

    // Find the tables and check them before reading any input, so that a table
    // problem is an error with empty stdout, never an answer.
    const std::optional<DataDir> data_dir = ResolveDataDir(config);
    CheckedTables checked;
    std::string   table_set;
    if (!data_dir
        || !KlutTablesAreSound(data_dir->path, config.max_crossings, checked)
        || !KlutTablesMatchManifest(data_dir->path, config.max_crossings, checked, config,
                                    table_set))
    {
        return EXIT_FAILURE;
    }

    Klut klut(data_dir->path, static_cast<Knoodle::Size_T>(config.max_crossings));
    NameBook names(klut, checked, data_dir->path.string());
    if (!KlutTablesLoad(klut, data_dir->path, checked, names))
    {
        return EXIT_FAILURE;
    }

    if (config.verify_tables)
    {
        std::uint64_t keys = 0;
        for (const auto& [c, t] : checked) { keys += t.keys; }
        std::cout << "KLUT tables in " << data_dir->path.string() << " verified: table set "
                  << table_set << ", " << 2 * checked.size() << " files, " << keys
                  << " keys, 3 to " << config.max_crossings << " crossings\n";
        return EXIT_SUCCESS;
    }

    // knoodleidentify always writes its results to stdout, so rule 1 (put the
    // diagnostics beside the output file) never applies -- it is rule 2 every
    // time: a per-process directory under the system temp dir. This has to
    // happen before any call into Simplify, because ki::Identify escalates with
    // embedding_trials > 0 (klut_identify.hpp) and can therefore reach Rattle,
    // whose failure bundles would otherwise land in the user's home directory.
    const std::filesystem::path diag_dir =
        ChooseDiagnosticDir("knoodleidentify", /*streaming_mode=*/true, std::nullopt);
    const std::set<std::string> bundles_before = ListDiagnosticBundles(diag_dir);

    // Every random choice follows from this one seed (see SeededEngine): the
    // random projection draws from rng, and Reapr from an engine drawn from it.
    g_run_seed = config.seed.value_or(FreshSeed());
    Knoodle::PRNG_T rng = SeededEngine(g_run_seed);

    ki::Reapr_T::Settings_T reapr_settings;
    if (config.reapr_energy)  { reapr_settings.energy  = *config.reapr_energy; }
    if (config.reapr_scaling) { reapr_settings.scaling = static_cast<Real>(*config.reapr_scaling); }
    if (config.reapr_z_grid_size) { reapr_settings.z_grid_size = *config.reapr_z_grid_size; }
    ki::Reapr_T reapr(reapr_settings);
    reapr.RandomEngine() = DerivedEngine(rng);

    Stats stats;
    bool success = true;
    bool table_failed = false;

    if (config.input_files.empty())
    {
        // Unix filter: no files -> read stdin. If stdin is an interactive terminal
        // (no pipe/redirect), say so, so a bare invocation does not just look hung.
        if (StdinIsInteractive() && !config.quiet)
        {
            Log("knoodleidentify: reading diagrams from stdin (Ctrl-D to end). "
                "Pipe a stream or pass a file; --help for usage.");
        }
        const StreamEnd end = ProcessStream(std::cin, "stdin", config, klut, names, reapr, stats, rng);
        success      = (end == StreamEnd::Done);
        table_failed = (end == StreamEnd::TableError);
    }
    else
    {
        for (const std::string& filename : config.input_files)
        {
            std::ifstream file(filename);
            if (!file)
            {
                LogError("Failed to open " + filename);
                success = false;
                continue;
            }
            const StreamEnd end = ProcessStream(file, filename, config, klut, names, reapr, stats, rng);
            if (end != StreamEnd::Done) { success = false; }
            if (end == StreamEnd::TableError) { table_failed = true; break; }
        }
    }

    if (!config.quiet)
    {
        Log("knoodleidentify: " + std::to_string(stats.knots) + " knots, " +
            std::to_string(stats.summands) + " summands (" +
            std::to_string(stats.identified) + " identified, " +
            std::to_string(stats.unknots) + " unknots, " +
            std::to_string(stats.not_found) + " not found, " +
            std::to_string(stats.over_range) + " over table range, " +
            std::to_string(stats.links) + " links, " +
            std::to_string(stats.invalid) + " invalid)");
        // Which table set answered: with several ways to choose one, this is
        // the only way to tell afterwards.
        Log("knoodleidentify: KLUT tables from " + data_dir->path.string()
            + " (" + data_dir->source + "), "
            + (table_set.empty() ? std::string("no manifest") : "table set " + table_set));
    }

    if (ErrorsSeen())
    {
        const long library_errors = (g_cerr_tap != nullptr) ? g_cerr_tap->Count() : 0;
        if (table_failed)
        {
            std::cerr << "\nknoodleidentify: stopped at knot " << stats.knots
                      << " because a KLUT table failed (see above). Lines printed"
                         " before that came from subtables that loaded; nothing"
                         " after it was identified.\n";
        }
        else if (library_errors == 0 && g_tool_error_count == stats.not_found)
        {
            std::cerr << "\nknoodleidentify: " << stats.not_found << " summand"
                      << (stats.not_found == 1 ? "" : "s") << " came back NotFound"
                      << " (" << config.max_crossings << " or fewer crossings, yet"
                         " unresolved after Reapr), so the exit status is nonzero.\n";
        }
        else
        {
            std::cerr << "\nknoodleidentify: " << ErrorSummary()
                      << " during this run -- the identifications above are UNRELIABLE"
                         " (the library discards diagrams it has flagged as invalid).\n";
        }

        std::string invocation;
        for (int i = 0; i < argc; ++i)
        {
            invocation += (i ? " " : "");
            invocation += argv[i];
        }

        const auto report = WriteDiagnosticReport("knoodleidentify", {
            { "command line", "  " + invocation + "\n" },
            { "what to send", "  This file, plus the input that produced it.\n" },
        });

        if (!report.empty())
        {
            std::cerr << "Wrote a diagnostic report to " << report.string()
                      << " -- please send it with any bug report.\n";
        }

        FinishDiagnostics(diag_dir, bundles_before, "knoodleidentify", true);
        return EXIT_FAILURE;
    }

    FinishDiagnostics(diag_dir, bundles_before, "knoodleidentify", !success);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
