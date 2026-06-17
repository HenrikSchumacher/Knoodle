/**
 * @file knoodleidentify.cpp
 * @brief knoodleidentify - identify knot diagrams via the KLUT (Knot LookUp Table).
 *
 * Reads RAW knot/link diagrams (PD codes or 3D embeddings, same input formats as
 * knoodlesimplify) from stdin or files, runs the KLUT identify protocol on each
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
 * Non-table summands appear as: Unidentified[N] (reduced below the table range
 * is impossible, so this is N>13), NotFound[N] (<=13 yet unresolved even after
 * Reapr -- suspicious), and the whole input as Link[N] if it is multi-component
 * (the KLUT is knots-only). See --help for the option list.
 */

#include "knoodle_io.hpp"
#include "klut_identify.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <unistd.h>   // isatty / fileno -- notice when reading from an interactive tty

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
        "Reads RAW knot/link diagrams (PD codes or 3D embeddings, same formats as\n"
        "knoodlesimplify; 'k' separates diagrams) from stdin or the given files,\n"
        "runs the KLUT identify protocol on each (decompose, simplify, escalate\n"
        "with Reapr, look each prime summand up), and writes one line per knot.\n"
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
        "                      and Klut_Values_NN.tsv. Default: $KNOODLE_KLUT_DIR,\n"
        "                      else data/Klut next to this executable's parent,\n"
        "                      else ./data/Klut.\n"
        "  --max-crossings=N   Use subtables up to N crossings (3-13, default 13).\n"
        "  --expanded          One line per knot, summands joined by ' # ' (uses\n"
        "                      the raw K[...] table names).\n"
        "  --tsv               Per-summand output: knot_index, summand_index,\n"
        "                      crossing_count, name (tab-separated).\n"
        "  --quiet             Suppress the stderr summary and per-summand warnings.\n"
        "  -h, --help          Show this help.\n"
        "\n"
        "Knot symbols (default / --tsv); c=crossings, i=index, the third field is the\n"
        "alternating flag, last is the symmetry coset:\n"
        "  KnotSymbol[c,i,a,\"sym\"]  identified knot; a (alternating) is True/False\n"
        "  Unidentified[N,PD]       N>13 crossings, over the table range; PD = signed\n"
        "                           PD code of the unresolved diagram (for analysis)\n"
        "  NotFound[N,PD]           <=13 yet unresolved after Reapr (suspicious!); PD too\n"
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
// Name resolution: (crossing_count, id) -> K[c,i,j,"sym"]
//==============================================================================

/// Identify returns a compact (crossing_count, subtable id). The id is the knot's
/// 0-based index within crossing-number subtable c, i.e. the id-th line of
/// Klut_Values_cc.tsv. Load those name lists once for human-readable rendering.
std::map<Int, std::vector<std::string>>
LoadNames(const std::filesystem::path& dir, Int max_crossings)
{
    std::map<Int, std::vector<std::string>> names;
    for (Int c = 3; c <= max_crossings; ++c)
    {
        const std::string cc = (c < 10 ? "0" : "") + std::to_string(c);
        std::ifstream vf(dir / ("Klut_Values_" + cc + ".tsv"));
        if (!vf) { continue; }
        std::vector<std::string> v;
        std::string name; long count;
        while (vf >> name >> count) { v.push_back(name); }
        names[c] = std::move(v);
    }
    return names;
}

std::string NameOf(const std::map<Int, std::vector<std::string>>& names,
                   Int c, Klut::ID_T id)
{
    auto it = names.find(c);
    if (it != names.end() && static_cast<std::size_t>(id) < it->second.size())
    {
        return it->second[id];
    }
    return "K[" + std::to_string(c) + ",?,?,\"?\"]";  // defensive; should not occur
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
Summand ConvertSummand(const ki::Summand& rs,
                       const std::map<Int, std::vector<std::string>>& names,
                       Stats& stats)
{
    Summand s;
    s.crossings = rs.crossings;
    switch (rs.kind)
    {
        case ki::Summand::Kind::Identified:
            s.kind = Kind::Identified;
            ParseRawName(NameOf(names, rs.crossings, rs.id), s);
            ++stats.identified;
            break;
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

/**
 * @brief Process one input stream; writes identifications to stdout.
 */
bool ProcessStream(std::istream& input, const std::string& source_name,
                   const Config& config, Klut& klut,
                   const std::map<Int, std::vector<std::string>>& names,
                   ki::Reapr_T& reapr, Stats& stats, Knoodle::PRNG_T& rng)
{
    bool reached_eof = false;

    while (!reached_eof)
    {
        auto input_knot = ReadKnot(input, false, rng, source_name, reached_eof);

        if (!input_knot)
        {
            if (reached_eof) { continue; }
            return false;  // Parse error
        }

        ++stats.knots;

        // Build a PDC from the raw input diagram(s). Raw input is normally a
        // single diagram; if pre-split summands arrive, Identify re-decomposes
        // their union. Bare unknot summands carry no diagram and are the identity.
        ki::PDC_T pdc;
        for (PD_T& pd : input_knot->summands)
        {
            if (pd.ValidQ()) { pdc.Push(std::move(pd)); }
        }
        const Int input_crossings =
            (pdc.DiagramCount() > Int(0)) ? pdc.CrossingCount() : Int(0);

        ki::IdentifyResult res = ki::Identify(klut, std::move(pdc), reapr);

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
                summands.push_back(ConvertSummand(rs, names, stats));
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

        if (!config.quiet)
        {
            for (std::size_t i = 0; i < summands.size(); ++i)
            {
                if (summands[i].kind == Kind::NotFound)
                {
                    LogError("knot " + std::to_string(stats.knots) +
                             ", summand " + std::to_string(i + 1) + ": " +
                             std::to_string(summands[i].crossings) +
                             " crossings, <=13 yet unresolved after Reapr — "
                             "table gap or a hard simplification case?");
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
    auto names = LoadNames(*data_dir, config.max_crossings);

    ki::Reapr_T reapr{};
    Knoodle::PRNG_T rng = Knoodle::InitializedRandomEngine<Knoodle::PRNG_T>();

    Stats stats;
    bool success = true;

    if (config.input_files.empty())
    {
        // Unix filter: no files -> read stdin. If stdin is an interactive terminal
        // (no pipe/redirect), say so, so a bare invocation does not just look hung.
        if (isatty(fileno(stdin)))
        {
            Log("knoodleidentify: reading diagrams from stdin (Ctrl-D to end). "
                "Pipe a stream or pass a file; --help for usage.");
        }
        success = ProcessStream(std::cin, "stdin", config, klut, names, reapr, stats, rng);
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
            if (!ProcessStream(file, filename, config, klut, names, reapr, stats, rng))
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
