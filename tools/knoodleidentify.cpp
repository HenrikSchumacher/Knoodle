/**
 * @file knoodleidentify.cpp
 * @brief knoodleidentify - A command-line tool for identifying simplified
 *        knot diagrams via the KLUT (Knot LookUp Table).
 *
 * Reads the output of knoodlesimplify (the 'k'/'s'-delimited PD code stream),
 * looks each connect-sum summand up in the KLUT, and emits one identification
 * line per input knot.
 *
 * Usage: knoodlesimplify --streaming-mode < diagrams.tsv | knoodleidentify
 *
 * IMPORTANT: the KLUT stores MacLeod keys of *simplified, canonicalized*
 * diagrams only. Simplification is part of the query; this tool does not
 * simplify. Feed it knoodlesimplify output, not raw diagrams.
 *
 * Output (default): one line per knot, summand names joined by " # ", e.g.
 *   K[3,1,1,"e/r"] # K[4,1,1,"e/m/r/mr"]
 *   Unknot
 * Non-table summands appear as markers: <unidentified:15> (over table range),
 * <notfound:9> (in range but no key match — flagged on stderr), <link:4>
 * (multi-component; the KLUT is knots-only).
 *
 * Output (--tsv): one line per summand:
 *   knot_index <tab> summand_index <tab> crossing_count <tab> name
 *
 * See --help for the full option list.
 */

#include "knoodle_io.hpp"

#include <filesystem>
#include <map>
#include <tuple>

//==============================================================================
// Configuration
//==============================================================================

namespace {

using Klut = Knoodle::Klut;

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
    std::vector<std::string> input_files;    ///< Input file paths (empty = stdin)
    bool help_requested = false;
};

//==============================================================================
// Usage
//==============================================================================

void PrintUsage()
{
    std::cout <<
        "knoodleidentify - identify simplified knot diagrams via the KLUT\n"
        "\n"
        "Usage: knoodleidentify [options] [input_files...]\n"
        "\n"
        "Reads knoodlesimplify output (PD codes, 'k' separates knots, 's'\n"
        "separates connect-sum summands) from stdin or the given files, looks\n"
        "each summand up in the Knot LookUp Table, and writes one line per\n"
        "knot to stdout.\n"
        "\n"
        "The table indexes simplified, canonicalized diagrams only — pipe\n"
        "knoodlesimplify output into this tool; raw diagrams will not match.\n"
        "\n"
        "Default output (one line per knot) is a Wolfram Language association\n"
        "from each distinct knot summand to its multiplicity, e.g.\n"
        "  <| KnotSymbol[3,1,True,\"e/r\"] -> 42, KnotSymbol[5,1,True,\"m/mr\"] -> 1 |>\n"
        "This parses directly via ToExpression. An all-unknot knot yields <||>.\n"
        "\n"
        "Options:\n"
        "  --data-dir=PATH     KLUT data directory containing Klut_Keys_NN.bin\n"
        "                      and Klut_Values_NN.tsv. Default: $KNOODLE_KLUT_DIR,\n"
        "                      else data/Klut next to this executable's parent,\n"
        "                      else ./data/Klut.\n"
        "  --max-crossings=N   Load subtables up to N crossings (3-13, default 13).\n"
        "  --expanded          One line per knot, summands joined by ' # ' in\n"
        "                      arrival order (uses the raw K[...] table names).\n"
        "  --tsv               Per-summand output: knot_index, summand_index,\n"
        "                      crossing_count, name (tab-separated).\n"
        "  --quiet             Suppress the stderr summary and per-summand warnings.\n"
        "  -h, --help          Show this help.\n"
        "\n"
        "Knot symbols (default / --tsv):\n"
        "  KnotSymbol[c,i,a,\"sym\"]  identified knot; a (amphichiral) is True/False\n"
        "  Unidentified[N]          N crossings, over the table range\n"
        "  NotFound[N]              within range but not in the table (suspicious!)\n"
        "  Link[N]                  multi-component diagram (table is knots-only)\n"
        "  Invalid[]                invalid diagram\n"
        "  (unknot summands are the connect-sum identity and are omitted)\n"
        "\n"
        "Markers in --expanded mode: K[c,i,j,\"sym\"], Unknot, <unidentified:N>,\n"
        "<notfound:N>, <link:N>, <invalid>.\n";
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

std::optional<Config> ParseArguments(int argc, char* argv[])
{
    Config config;

    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);

        if (arg == "-h" || arg == "--help")
        {
            config.help_requested = true;
            return config;
        }
        else if (arg.starts_with("--data-dir="))
        {
            config.data_dir = std::string(arg.substr(11));
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
        else if (arg == "--quiet")
        {
            config.quiet = true;
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

    return config;
}

//==============================================================================
// Data Directory Resolution
//==============================================================================

/**
 * @brief Resolve the KLUT data directory.
 *
 * Order: --data-dir, $KNOODLE_KLUT_DIR, <exe_dir>/../data/Klut, ./data/Klut.
 */
std::optional<std::filesystem::path> ResolveDataDir(const Config& config,
                                                    const char* argv0)
{
    namespace fs = std::filesystem;

    std::vector<fs::path> candidates;

    if (config.data_dir)
    {
        candidates.emplace_back(*config.data_dir);
    }
    else
    {
        if (const char* env = std::getenv("KNOODLE_KLUT_DIR"))
        {
            candidates.emplace_back(env);
        }

        std::error_code ec;
        fs::path exe = fs::weakly_canonical(fs::path(argv0), ec);
        if (!ec && exe.has_parent_path())
        {
            // tools/ sits next to data/, so try <exe_dir>/../data/Klut
            candidates.push_back(exe.parent_path().parent_path() / "data" / "Klut");
        }

        candidates.push_back(fs::path("data") / "Klut");
    }

    for (const fs::path& dir : candidates)
    {
        // The 3-crossing subtable always exists in a valid KLUT directory.
        if (fs::exists(dir / "Klut_Values_03.tsv"))
        {
            return dir;
        }
    }

    LogError("Could not find KLUT data directory. Tried:");
    for (const fs::path& dir : candidates)
    {
        LogError("  " + dir.string());
    }
    LogError("Specify one with --data-dir=PATH or $KNOODLE_KLUT_DIR.");
    return std::nullopt;
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
    Kind        kind       = Kind::Invalid;
    Int         crossings  = 0;

    // Valid only when kind == Identified (parsed from the K[c,i,j,"sym"] name):
    Int         knot_index = 0;     ///< i
    bool        amphichiral = false; ///< j (0/1 -> False/True)
    std::string coset;              ///< "sym" *including* the surrounding quotes
    std::string raw_name;           ///< the original K[...] string from FindName
};

/**
 * @brief Parse a raw table name "K[c,i,j,\"sym\"]" into the fields of a
 *        Summand. Leaves identified-only fields at defaults if the shape is
 *        unexpected (defensive; should not happen for real table names).
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
    s.amphichiral = (f_amp == "1");
    s.coset = inner.substr(c3 + 1);  // keeps the surrounding quotes
}

/**
 * @brief Classify one summand and update the running stats.
 */
Summand ClassifySummand(PD_T& pd, Klut& klut, Stats& stats)
{
    ++stats.summands;

    Summand s;

    if (pd.InvalidQ())
    {
        ++stats.invalid;
        s.kind = Kind::Invalid;
        return s;
    }

    s.crossings = pd.CrossingCount();

    if (s.crossings == 0)
    {
        ++stats.unknots;
        s.kind = Kind::Unknot;
        return s;
    }

    if (pd.LinkComponentCount() > Int(1))
    {
        ++stats.links;
        s.kind = Kind::Link;
        return s;
    }

    if (s.crossings > static_cast<Int>(klut.CrossingCount()))
    {
        ++stats.over_range;
        s.kind = Kind::OverRange;
        return s;
    }

    const std::string name = klut.FindName(pd);

    if (name == "NotFound")
    {
        ++stats.not_found;
        s.kind = Kind::NotFound;
        return s;
    }
    if (name == "Invalid" || name == "Error")
    {
        ++stats.invalid;
        s.kind = Kind::Invalid;
        return s;
    }

    ++stats.identified;
    s.kind = Kind::Identified;
    ParseRawName(name, s);
    return s;
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
                   (s.amphichiral ? "True" : "False") + "," + s.coset + "]";
        case Kind::Unknot:    return "";
        case Kind::OverRange: return "Unidentified[" + std::to_string(s.crossings) + "]";
        case Kind::NotFound:  return "NotFound[" + std::to_string(s.crossings) + "]";
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
        case Kind::OverRange:  return "<unidentified:" + std::to_string(s.crossings) + ">";
        case Kind::NotFound:   return "<notfound:" + std::to_string(s.crossings) + ">";
        case Kind::Link:       return "<link:" + std::to_string(s.crossings) + ">";
        case Kind::Invalid:    return "<invalid>";
    }
    return "<invalid>";
}

/// Deterministic ordering of distinct summand kinds within a knot's multiset:
/// identified knots first (by crossing, index, amphichirality, coset), then
/// the non-knot categories grouped after.
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

    return std::tie(ga, a.crossings, a.knot_index, a.amphichiral, a.coset)
         < std::tie(gb, b.crossings, b.knot_index, b.amphichiral, b.coset);
}

/**
 * @brief Process one input stream; writes identifications to stdout.
 */
bool ProcessStream(std::istream& input, const std::string& source_name,
                   const Config& config, Klut& klut, Stats& stats,
                   Knoodle::PRNG_T& rng)
{
    bool reached_eof = false;

    while (!reached_eof)
    {
        auto input_knot = ReadKnot(input, false, rng, source_name, reached_eof);

        if (!input_knot)
        {
            if (reached_eof)
            {
                continue;
            }
            return false;  // Parse error
        }

        ++stats.knots;

        std::vector<Summand> summands;

        // Unknot summands arrive as bare 's' lines (no diagram constructed).
        for (Int i = 0; i < input_knot->empty_summand_count; ++i)
        {
            ++stats.summands;
            ++stats.unknots;
            summands.push_back(Summand{.kind = Kind::Unknot});
        }

        for (PD_T& pd : input_knot->summands)
        {
            summands.push_back(ClassifySummand(pd, klut, stats));
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

        if (!config.quiet)
        {
            for (std::size_t i = 0; i < summands.size(); ++i)
            {
                if (summands[i].kind == Kind::NotFound)
                {
                    LogError("knot " + std::to_string(stats.knots) +
                             ", summand " + std::to_string(i + 1) + ": " +
                             std::to_string(summands[i].crossings) +
                             " crossings, within table range but not found — " +
                             "unsimplified input or table gap?");
                }
            }
        }
    }

    return true;
}

} // anonymous namespace

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[])
{
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

    auto data_dir = ResolveDataDir(config, argv[0]);
    if (!data_dir)
    {
        return EXIT_FAILURE;
    }

    Klut klut(*data_dir, static_cast<Knoodle::Size_T>(config.max_crossings));

    Knoodle::PRNG_T rng = Knoodle::InitializedRandomEngine<Knoodle::PRNG_T>();

    Stats stats;
    bool success = true;

    if (config.input_files.empty())
    {
        success = ProcessStream(std::cin, "stdin", config, klut, stats, rng);
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
            if (!ProcessStream(file, filename, config, klut, stats, rng))
            {
                success = false;
            }
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
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
