/**
 * @file knoodlesimplify.cpp
 * @brief knoodlesimplify - A command-line tool for knot and link diagram simplification.
 *
 * This tool reads knot/link diagrams (as PD codes, 3D geometry, or .kndlxyz files),
 * simplifies them using various algorithms, and outputs the simplified diagrams.
 *
 * Usage: knoodlesimplify [options] [input_files...]
 *
 * Input formats:
 *   - PD code: Lines with 4, 5, 6, or 7 tab-separated integers per crossing
 *   - 3D geometry: Lines with 3 tab-separated numbers (coordinates)
 *   - .kndlxyz: Multi-component 3D link embedding (blank lines separate components)
 *
 * Output format (knoodlesimplify format):
 *   k           <- knot/link separator
 *   s           <- summand separator
 *   1  2  3  4  1           <- crossing (5 integers: 4 arcs + handedness)
 *   1  2  3  4  1  0  1     <- crossing (7 integers: 4 arcs + handedness + 2 colors)
 *   ...
 *
 * See --help for full option list.
 */

// Jason, you can let the compiler have the following flags defined
//#define KNOODLE_USE_UMFPACK  // Support for the "Dirichlet" and "Bending" energies in Reapr.
//#define KNOODLE_USE_CLP      // Support some LP problems in Reapr and OrthoDraw; inferior to MCF.
//#define KNOODLE_USE_BOOST_UNORDERED // Support for faster associative containers in Reapr.

// Only KNOODLE_USE_BOOST_UNORDERED might be of some interest here because OrthoDraw and Reapr uses some of the containers provided by this a little. However, the containers sizes should be small in practive, so the corresponding fall back containers of the STL will be good enough. I doubt that anybody will measure some difference, but you are free to do so.

#include "knoodle_io.hpp"

#include <filesystem>

//==============================================================================
// Configuration
//==============================================================================

namespace {

/**
 * @brief Configuration parsed from command-line arguments.
 */
struct Config
{
    // Simplification options
    int  simplify_level       = 6;           ///< Simplification level (3, 4, 5, or 6+)
    Int  max_reapr_attempts   = 25;          ///< Max iterations for Reapr::Rattle
    bool no_compaction        = false;       ///< Skip compaction in OrthoDraw (Reapr only)
    std::optional<Energy_T> reapr_energy;    ///< Energy flag for Reapr (if set)

    // Input options
    std::vector<std::string> input_files;    ///< Input file paths
    bool streaming_mode       = false;       ///< Read from stdin, write to stdout
    bool randomize_projection = false;       ///< Apply random shear to 3D projection

    // Output options
    std::optional<std::string> output_file;  ///< Single output file (if specified)
    bool quiet                = false;       ///< Suppress per-knot reports, show counter only

    // Derived state
    bool help_requested       = false;       ///< User requested help
};

/// Minimum valid simplification level.
constexpr int kMinSimplifyLevel = 0;

/// Threshold at or above which Reapr is used instead of SimplifyN.
constexpr int kReaprThreshold = 6;

//==============================================================================
// Input/Output Data Structures (simplify-specific)
//==============================================================================

/**
 * @brief Represents a simplified knot with timing information.
 */
struct SimplifiedKnot
{
    std::vector<PD_T> summands;           ///< Simplified prime summands
    std::vector<Int>  crossing_counts;    ///< Crossing count per summand
    Int               total_crossings = 0;

    Int               unknot_count = 0;       ///< Number of summands that simplified to unknots (Reapr only)
    Int               proven_minimal_count = 0;  ///< Number of summands with ProvenMinimalQ() == true

    Duration          simplify_time{0};   ///< Time spent simplifying

    /// Returns total number of summands (including unknots)
    Int TotalSummandCount() const
    {
        return static_cast<Int>(summands.size()) + unknot_count;
    }

    /// Returns total proven minimal count (unknots are trivially minimal)
    Int TotalProvenMinimalCount() const
    {
        return proven_minimal_count + unknot_count;
    }

    /// Returns true if all summands are proven minimal
    bool FullySimplifiedQ() const
    {
        return TotalProvenMinimalCount() == TotalSummandCount();
    }
};

/**
 * @brief Aggregate statistics for reporting.
 */
struct ProcessingStats
{
    Int      input_crossings  = 0;
    Int      output_crossings = 0;
    Int      total_knots      = 0;              ///< Total knots across all files
    Int      total_summands   = 0;              ///< Total summands across all knots
    Int      proven_minimal_summands = 0;       ///< Summands with ProvenMinimalQ() == true
    Int      fully_simplified_knots = 0;        ///< Knots where all summands are proven minimal
    Duration input_time{0};
    Duration simplify_time{0};
    Duration output_time{0};
    int      files_processed  = 0;              ///< Number of files successfully processed
};

//==============================================================================
// Command-Line Parsing
//==============================================================================

/**
 * @brief Print usage information.
 */
void PrintUsage()
{
    Log("Usage: knoodlesimplify [options] [input_files...]");
    Log("");
    Log("Simplification options:");
    Log("  -s=N, --simplify-level=N    Set simplification level:");
    Log("                                0          No simplification (PD code only)");
    Log("                                3          Simplify3 (Reidemeister I + II)");
    Log("                                4          Simplify4 (+ path rerouting)");
    Log("                                5          Simplify5 (+ summand detection)");
    Log("                                6+/max/full/reapr");
    Log("                                           Full Reapr pipeline (default)");
    Log("  --max-reapr-attempts=K      Maximum iterations for Reapr (default: 25)");
    Log("  --no-compaction             Skip compaction in OrthoDraw (Reapr only)");
    Log("  --reapr-energy=E            Set Reapr energy function (Reapr only):");
    Log("                                " + ValidEnergies());
    Log("");
    Log("Input formats:");
    Log("  4 columns: unsigned PD code (4 arc labels per crossing)");
    Log("  5 columns: signed PD code (4 arcs + handedness)");
    Log("  6 columns: unsigned PD code + link component colors");
    Log("  7 columns: signed PD code + link component colors");
    Log("  3 columns: 3D geometry (x, y, z coordinates)");
    Log("  .kndlxyz:  multi-component 3D link embedding");
    Log("");
    Log("Input options:");
    Log("  --input=FILE                Specify input file (can use multiple times)");
    Log("  --streaming-mode            Read from stdin, write to stdout");
    Log("  --randomize-projection      Apply random shear to 3D geometry projection");
    Log("");
    Log("Output options:");
    Log("  --output=FILE               Write all output to FILE");
    Log("  -q, --quiet                 Suppress per-knot reports, show counter only");
    Log("");
    Log("Other:");
    Log("  -h, --help                  Show this help message");
}

/**
 * @brief Parse a simplification level from a string.
 */
std::optional<int> ParseSimplifyLevel(std::string_view arg)
{
    std::string lower = ToLower(arg);

    if (lower == "max" || lower == "full" || lower == "reapr")
    {
        return kReaprThreshold;
    }

    try
    {
        std::size_t pos = 0;
        int level = std::stoi(std::string(arg), &pos);
        if (pos != arg.size()) return std::nullopt;
        return level;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

/**
 * @brief Parse command-line arguments into a Config struct.
 */
std::optional<Config> ParseArguments(int argc, char* argv[])
{
    Config config;
    int output_count = 0;

    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);

        // Help
        if (arg == "-h" || arg == "--help")
        {
            config.help_requested = true;
            return config;
        }

        // Simplify level: -s=N or --simplify-level=N
        if (arg.starts_with("-s="))
        {
            auto parsed = ParseSimplifyLevel(arg.substr(3));
            if (!parsed || *parsed < kMinSimplifyLevel)
            {
                LogError("Invalid simplify level: '" + std::string(arg.substr(3)) + "'");
                return std::nullopt;
            }
            config.simplify_level = *parsed;
        }
        else if (arg.starts_with("--simplify-level="))
        {
            auto parsed = ParseSimplifyLevel(arg.substr(17));
            if (!parsed || *parsed < kMinSimplifyLevel)
            {
                LogError("Invalid simplify level: '" + std::string(arg.substr(17)) + "'");
                return std::nullopt;
            }
            config.simplify_level = *parsed;
        }
        // Max Reapr attempts
        else if (arg.starts_with("--max-reapr-attempts="))
        {
            try
            {
                config.max_reapr_attempts = std::stoll(std::string(arg.substr(21)));
                if (config.max_reapr_attempts < 1)
                {
                    LogError("max-reapr-attempts must be at least 1");
                    return std::nullopt;
                }
            }
            catch (const std::exception&)
            {
                LogError("Invalid max-reapr-attempts value");
                return std::nullopt;
            }
        }
        // No compaction (Reapr only)
        else if (arg == "--no-compaction")
        {
            config.no_compaction = true;
        }
        // Reapr energy flag
        else if (arg.starts_with("--reapr-energy="))
        {
            std::string energy_str = ToLower(arg.substr(15));

            if (energy_str == "tv")
            {
                config.reapr_energy = Energy_T::TV;
            }
            else if (energy_str == "dirichlet")
            {
                config.reapr_energy = Energy_T::Dirichlet;
            }
            else if (energy_str == "bending")
            {
                config.reapr_energy = Energy_T::Bending;
            }
            else if (energy_str == "height")
            {
                config.reapr_energy = Energy_T::Height;
            }
            else if (energy_str == "tv_clp")
            {
                config.reapr_energy = Energy_T::TV_CLP;
            }
            else if (energy_str == "tv_mcf")
            {
                config.reapr_energy = Energy_T::TV_MCF;
            }
            else
            {
                LogError("Unknown reapr energy: '" + std::string(arg.substr(15)) + "'");
                LogError("Valid options: " + ValidEnergies());
                return std::nullopt;
            }
        }
        // Input file
        else if (arg.starts_with("--input="))
        {
            config.input_files.emplace_back(arg.substr(8));
        }
        // Streaming mode
        else if (arg == "--streaming-mode")
        {
            config.streaming_mode = true;
        }
        // Randomize projection
        else if (arg == "--randomize-projection")
        {
            config.randomize_projection = true;
        }
        // Output file
        else if (arg.starts_with("--output="))
        {
            ++output_count;
            if (output_count > 1)
            {
                LogError("--output can only be specified once");
                return std::nullopt;
            }
            config.output_file = std::string(arg.substr(9));
        }
        // Quiet mode
        else if (arg == "--quiet" || arg == "-q")
        {
            config.quiet = true;
        }
        // Unknown option
        else if (arg.starts_with("-"))
        {
            LogError("Unknown option: " + std::string(arg));
            PrintUsage();
            return std::nullopt;
        }
        // Naked argument = input file
        else
        {
            config.input_files.emplace_back(arg);
        }
    }

    // Validate option combinations
    if (config.streaming_mode && !config.input_files.empty())
    {
        LogError("Cannot use --streaming-mode with input files");
        return std::nullopt;
    }

    return config;
}

//==============================================================================
// Simplification
//==============================================================================

/**
 * @brief Simplify a knot using the configured algorithm.
 *
 * @param input The input knot with its summands.
 * @param config Configuration options.
 * @return The simplified knot with timing information.
 */
SimplifiedKnot SimplifyKnot(const InputKnot& input, const Config& config)
{
    SimplifiedKnot result;

    {
        ScopedTimer timer(result.simplify_time);

        for (const PD_T& pd_in : input.summands)
        {
            if (config.simplify_level == 0)
            {
                // No simplification - just copy the PD
                PD_T pd(pd_in);
                result.summands.push_back(std::move(pd));
            }
            else
            {
                // Use PlanarDiagramComplex for all simplification levels
                PD_T pd_copy(pd_in);
                PDC_T pdc(std::move(pd_copy));

                PDC_T::Simplify_Args_T args;
                args.splitQ = true;

                if (config.simplify_level >= 4)
                    args.rerouteQ = true;
                else
                    args.rerouteQ = false;

                if (config.simplify_level >= 5)
                    args.disconnectQ = true;
                else
                    args.disconnectQ = false;

                if (config.simplify_level >= kReaprThreshold)
                {
                    args.embedding_trials = static_cast<Knoodle::Size_T>(config.max_reapr_attempts);
                    args.rotation_trials = 25;
                }

                if (config.reapr_energy.has_value())
                    args.energy = *config.reapr_energy;

                if (config.no_compaction)
                    args.compaction_method = PDC_T::Compaction_T::Unknown;

                pdc.Simplify(args);

                // Extract results
                if (pdc.DiagramCount() == 0)
                {
                    ++result.unknot_count;
                }
                else
                {
                    for (Int i = 0; i < pdc.DiagramCount(); ++i)
                    {
                        result.summands.push_back(PD_T(pdc.Diagram(i)));
                    }
                }
            }
        }
    }

    // Compute crossing counts and proven minimal count
    for (const auto& pd : result.summands)
    {
        Int cc = pd.CrossingCount();
        result.crossing_counts.push_back(cc);
        result.total_crossings += cc;

        if (pd.ProvenMinimalQ())
        {
            ++result.proven_minimal_count;
        }
    }

    return result;
}

//==============================================================================
// Output
//==============================================================================

/**
 * @brief Write a simplified knot in knoodlesimplify format.
 *
 * @param knot The simplified knot (non-const because PDCode() modifies internal buffers).
 * @param output The output stream.
 * @param include_k_marker Whether to include the leading 'k' marker.
 * @param colored_output If true, write 7-column format (with link component colors);
 *                       otherwise write 5-column format.
 */
void WriteKnot(SimplifiedKnot& knot, std::ostream& output,
               bool include_k_marker, bool colored_output)
{
    if (include_k_marker)
    {
        output << "k\n";
    }

    for (std::size_t i = 0; i < knot.summands.size(); ++i)
    {
        output << "s\n";

        auto& pd = knot.summands[i];
        Int crossing_count = pd.CrossingCount();

        if (colored_output)
        {
            auto pd_code = pd.template PDCode<Int, true, true>();

            for (Int c = 0; c < crossing_count; ++c)
            {
                output << pd_code(c, 0);
                for (Int j = 1; j < 7; ++j)
                {
                    output << '\t' << pd_code(c, j);
                }
                output << '\n';
            }
        }
        else
        {
            auto pd_code = pd.template PDCode<Int, true, false>();

            for (Int c = 0; c < crossing_count; ++c)
            {
                output << pd_code(c, 0);
                for (Int j = 1; j < 5; ++j)
                {
                    output << '\t' << pd_code(c, j);
                }
                output << '\n';
            }
        }
    }
}

//==============================================================================
// Reporting
//==============================================================================

/**
 * @brief Write the report for a single knot processing.
 */
void WriteKnotReport(const InputKnot& input,
                     const SimplifiedKnot& simplified,
                     const Config& config,
                     Duration input_time,
                     Duration output_time)
{
    Log("");
    Log("=== Processing Report: " + input.source_description + " ===");
    Log("");

    // Simplification settings
    std::string level_str;
    if (config.simplify_level == 0) level_str = "None (PD code only)";
    else if (config.simplify_level == 3) level_str = "Simplify3";
    else if (config.simplify_level == 4) level_str = "Simplify4";
    else if (config.simplify_level == 5) level_str = "Simplify5";
    else
    {
        level_str = "Reapr (max attempts: " + std::to_string(config.max_reapr_attempts);

        if (config.reapr_energy.has_value())
        {
            level_str += ", energy: ";
            level_str += EnergyName(*config.reapr_energy);
        }

        level_str += ")";
    }

    Log("Simplification: " + level_str);

    // Input statistics
    Log("");
    Log("Input:");
    Log("  Total crossings: " + std::to_string(input.total_crossings));

    if (input.summands.size() > 1)
    {
        std::string summand_list = "  By summand: ";
        for (std::size_t i = 0; i < input.crossing_counts.size(); ++i)
        {
            if (i > 0) summand_list += ", ";
            summand_list += std::to_string(input.crossing_counts[i]);
        }
        Log(summand_list);
    }

    if (input.had_3d_geometry)
    {
        Log("  3D vertices: " + std::to_string(input.vertex_count_3d));
    }

    // Output statistics
    Log("");
    Log("Output:");
    Log("  Total crossings: " + std::to_string(simplified.total_crossings));
    Log("  Summands: " + std::to_string(simplified.TotalSummandCount()));

    if (simplified.unknot_count > 0)
    {
        Log("  Unknots: " + std::to_string(simplified.unknot_count));
    }

    if (simplified.summands.size() > 1)
    {
        std::string summand_list = "  By summand: ";
        for (std::size_t i = 0; i < simplified.crossing_counts.size(); ++i)
        {
            if (i > 0) summand_list += ", ";
            summand_list += std::to_string(simplified.crossing_counts[i]);
        }
        Log(summand_list);
    }

    // Proven minimal fraction
    {
        Int p = simplified.TotalProvenMinimalCount();
        Int q = simplified.TotalSummandCount();
        double pct = (q > 0) ? (100.0 * static_cast<double>(p) / static_cast<double>(q)) : 100.0;
        std::ostringstream oss;
        oss << "  Proven minimal: " << p << "/" << q << " (";
        oss << std::fixed << std::setprecision(1) << pct << " %)";
        Log(oss.str());
    }

    // Simplification factor
    if (simplified.total_crossings > 0)
    {
        double factor = static_cast<double>(input.total_crossings) /
                        static_cast<double>(simplified.total_crossings);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << factor;
        Log("  Simplification factor: " + oss.str() + "x");
    }
    else if (input.total_crossings > 0)
    {
        // Reduced to unknot(s)
        Log("  Simplification factor: infinite (reduced to unknot)");
    }

    // Timing
    Log("");
    Log("Timing:");

    auto format_time = [](Duration d) {
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(4) << d.count();
        return oss.str();
    };

    Log("  Input:          " + format_time(input_time) + " s");
    Log("  Simplification: " + format_time(simplified.simplify_time) + " s");
    Log("  Output:         " + format_time(output_time) + " s");
}

/**
 * @brief Write the final aggregate report for multiple files.
 */
void WriteFinalReport(const ProcessingStats& stats, const Config& config)
{
    Log("");
    Log("========================================");
    Log("=== Final Report ===");
    Log("========================================");
    Log("");

    // Simplification settings
    std::string level_str;
    if (config.simplify_level == 0) level_str = "None (PD code only)";
    else if (config.simplify_level == 3) level_str = "Simplify3";
    else if (config.simplify_level == 4) level_str = "Simplify4";
    else if (config.simplify_level == 5) level_str = "Simplify5";
    else
    {
        level_str = "Reapr (max attempts: " + std::to_string(config.max_reapr_attempts);

        if (config.reapr_energy.has_value())
        {
            level_str += ", energy: ";
            level_str += EnergyName(*config.reapr_energy);
        }

        level_str += ")";
    }

    Log("Simplification: " + level_str);
    Log("");
    Log("Files processed: " + std::to_string(stats.files_processed));
    Log("Knots processed: " + std::to_string(stats.total_knots));
    Log("Summands processed: " + std::to_string(stats.total_summands));

    // Average input crossing number per summand
    if (stats.total_summands > 0)
    {
        double avg_crossings = static_cast<double>(stats.input_crossings) /
                               static_cast<double>(stats.total_summands);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << avg_crossings;
        Log("Average input summand crossing number: " + oss.str());
    }

    auto format_time = [](Duration d) {
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(4) << d.count();
        return oss.str();
    };

    Log("");
    Log("Total timing:");
    Log("  Input:          " + format_time(stats.input_time) + " s");
    Log("  Simplification: " + format_time(stats.simplify_time) + " s");
    Log("  Output:         " + format_time(stats.output_time) + " s");

    // Average simplification time per summand
    if (stats.total_summands > 0)
    {
        double avg_simplify_time = stats.simplify_time.count() /
                                   static_cast<double>(stats.total_summands);
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(4) << avg_simplify_time;
        Log("  Average summand simplification time: " + oss.str() + " s");
    }

    if (stats.output_crossings > 0)
    {
        double factor = static_cast<double>(stats.input_crossings) /
                        static_cast<double>(stats.output_crossings);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << factor;
        Log("");
        Log("Overall simplification factor: " + oss.str() + "x");
        Log("  (" + std::to_string(stats.input_crossings) + " -> " +
            std::to_string(stats.output_crossings) + " crossings)");
    }

    // Proven minimal statistics
    Log("");
    {
        Int p = stats.proven_minimal_summands;
        Int q = stats.total_summands;
        double pct = (q > 0) ? (100.0 * static_cast<double>(p) / static_cast<double>(q)) : 0.0;
        std::ostringstream oss;
        oss << "Fraction of summands proven minimal after simplification: ";
        oss << p << "/" << q << " (";
        oss << std::fixed << std::setprecision(1) << pct << " %)";
        Log(oss.str());
    }
    {
        Int p = stats.fully_simplified_knots;
        Int q = stats.total_knots;
        double pct = (q > 0) ? (100.0 * static_cast<double>(p) / static_cast<double>(q)) : 0.0;
        std::ostringstream oss;
        oss << "Fully simplified knots: " << p << "/" << q << " (";
        oss << std::fixed << std::setprecision(1) << pct << " %)";
        Log(oss.str());
    }
}

//==============================================================================
// File Processing
//==============================================================================

/**
 * @brief Generate output filename from input filename.
 */
std::filesystem::path GetSimplifiedFilename(const std::filesystem::path& input_path)
{
    auto stem = input_path.stem().string();
    auto ext  = input_path.extension().string();
    if (ext.empty()) ext = ".tsv";

    return input_path.parent_path() / (stem + "_simplified" + ext);
}

/**
 * @brief Process a .kndlxyz file (multi-component 3D link embedding).
 *
 * @param filepath Path to the .kndlxyz file.
 * @param output_stream Optional output stream (nullptr for per-file output).
 * @param config Configuration.
 * @param stats Statistics accumulator.
 * @param first_knot_in_output Whether this is the first knot being written.
 * @return true on success.
 */
bool ProcessXYZFile(const std::string& filepath,
                    std::ostream* output_stream,
                    const Config& config,
                    ProcessingStats& stats,
                    bool& first_knot_in_output)
{
    Duration input_time{0};
    PDC_T pdc;

    {
        ScopedTimer timer(input_time);
        LinkEmb_T link = LinkEmb_T::ReadFromFile(std::filesystem::path(filepath));
        // PDC constructor from LinkEmbedding calls FindIntersections internally
        pdc = PDC_T(std::move(link));
    }

    if (pdc.DiagramCount() == 0)
    {
        LogError("Failed to create diagram from .kndlxyz file: " + filepath);
        return false;
    }

    // Create an InputKnot for reporting
    InputKnot input_knot;
    input_knot.source_description = filepath;
    input_knot.had_3d_geometry = true;
    input_knot.input_column_count = 7;  // .kndlxyz always outputs colored format

    // Record input crossing counts before simplification
    for (Int i = 0; i < pdc.DiagramCount(); ++i)
    {
        const auto& pd = pdc.Diagram(i);
        Int cc = pd.CrossingCount();
        input_knot.summands.push_back(PD_T(pd));
        input_knot.crossing_counts.push_back(cc);
        input_knot.total_crossings += cc;
    }

    // Simplification phase
    SimplifiedKnot simplified = SimplifyKnot(input_knot, config);

    // Output phase (always 7-column / colored for .kndlxyz)
    Duration output_time{0};

    {
        ScopedTimer timer(output_time);

        if (output_stream)
        {
            WriteKnot(simplified, *output_stream,
                      !first_knot_in_output, true);
            first_knot_in_output = false;
        }
        else
        {
            std::filesystem::path input_path(filepath);
            std::filesystem::path output_path = GetSimplifiedFilename(input_path);

            std::ofstream file(output_path);
            if (!file)
            {
                LogError("Failed to open " + output_path.string() + " for writing");
                return false;
            }

            WriteKnot(simplified, file, true, true);
        }
    }

    // Reporting phase
    if (config.quiet)
    {
        *g_log_stream << "\r" << (stats.total_knots + 1) << " links processed" << std::flush;
    }
    else
    {
        WriteKnotReport(input_knot, simplified, config, input_time, output_time);
    }

    // Update stats
    stats.input_crossings  += input_knot.total_crossings;
    stats.output_crossings += simplified.total_crossings;
    stats.total_summands   += simplified.TotalSummandCount();
    stats.proven_minimal_summands += simplified.TotalProvenMinimalCount();
    if (simplified.FullySimplifiedQ())
    {
        ++stats.fully_simplified_knots;
    }
    stats.input_time       += input_time;
    stats.simplify_time    += simplified.simplify_time;
    stats.output_time      += output_time;
    ++stats.total_knots;

    return true;
}

/**
 * @brief Process a single input source (file or stdin).
 *
 * @param input The input stream.
 * @param source_name Description of the source.
 * @param output_stream Optional output stream (nullptr for per-file output).
 * @param config Configuration.
 * @param rng Random number generator.
 * @param stats Statistics accumulator.
 * @param first_knot_in_output Whether this is the first knot being written.
 * @return true on success.
 */
bool ProcessSource(std::istream& input,
                   const std::string& source_name,
                   std::ostream* output_stream,
                   const Config& config,
                   Knoodle::PRNG_T& rng,
                   ProcessingStats& stats,
                   bool& first_knot_in_output)
{
    bool reached_eof = false;

    while (!reached_eof)
    {
        // Input phase
        Duration input_time{0};
        std::optional<InputKnot> input_knot;

        {
            ScopedTimer timer(input_time);
            input_knot = ReadKnot(input, config.randomize_projection, rng, source_name, reached_eof);
        }

        if (!input_knot)
        {
            if (reached_eof && stats.total_knots == 0)
            {
                // No data at all
                continue;
            }
            if (!reached_eof)
            {
                return false;  // Parse error
            }
            continue;
        }

        // Simplification phase
        SimplifiedKnot simplified = SimplifyKnot(*input_knot, config);

        // Determine output format based on input column count
        bool colored_output = (input_knot->input_column_count >= 6);

        // Output phase
        Duration output_time{0};

        {
            ScopedTimer timer(output_time);

            if (output_stream)
            {
                // Writing to shared output stream
                WriteKnot(simplified, *output_stream,
                          !first_knot_in_output || !config.streaming_mode,
                          colored_output);
                first_knot_in_output = false;
            }
            else if (!config.streaming_mode)
            {
                // Per-file output
                std::filesystem::path input_path(source_name);
                std::filesystem::path output_path = GetSimplifiedFilename(input_path);

                std::ofstream file(output_path);
                if (!file)
                {
                    LogError("Failed to open " + output_path.string() + " for writing");
                    return false;
                }

                WriteKnot(simplified, file, true, colored_output);
            }
        }

        // Reporting phase
        if (config.quiet)
        {
            // In quiet mode, just print a counter that updates in place
            *g_log_stream << "\r" << (stats.total_knots + 1) << " knots processed" << std::flush;
        }
        else
        {
            WriteKnotReport(*input_knot, simplified, config, input_time, output_time);
        }

        // Update stats
        stats.input_crossings  += input_knot->total_crossings;
        stats.output_crossings += simplified.total_crossings;
        stats.total_summands   += simplified.TotalSummandCount();
        stats.proven_minimal_summands += simplified.TotalProvenMinimalCount();
        if (simplified.FullySimplifiedQ())
        {
            ++stats.fully_simplified_knots;
        }
        stats.input_time       += input_time;
        stats.simplify_time    += simplified.simplify_time;
        stats.output_time      += output_time;
        ++stats.total_knots;
    }

    return true;
}

} // anonymous namespace

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[])
{
    // Parse command line
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

    // Initialize logging
    if (!InitLogging(config.streaming_mode, "knoodlesimplify.log"))
    {
        return EXIT_FAILURE;
    }

    // Initialize random number generator
    Knoodle::PRNG_T rng = Knoodle::InitializedRandomEngine<Knoodle::PRNG_T>();

    // Statistics accumulator
    ProcessingStats stats;
    bool first_knot_in_output = true;

    // Prepare output stream if single output file specified
    std::ofstream output_file;
    std::ostream* output_stream = nullptr;

    if (config.streaming_mode)
    {
        Log("knoodlesimplify in stream mode, expect input from stdin, will direct output to stdout");
        output_stream = &std::cout;
    }
    else if (config.output_file)
    {
        output_file.open(*config.output_file);
        if (!output_file)
        {
            LogError("Failed to open output file: " + *config.output_file);
            return EXIT_FAILURE;
        }
        output_stream = &output_file;
    }

    // Process inputs
    bool success = true;

    if (config.streaming_mode)
    {
        // Read from stdin
        success = ProcessSource(std::cin, "stdin", output_stream, config, rng,
                                stats, first_knot_in_output);
        if (success)
        {
            ++stats.files_processed;
        }
    }
    else if (config.input_files.empty())
    {
        // No input files specified - display help
        PrintUsage();
        return EXIT_SUCCESS;
    }
    else
    {
        // Process each input file
        for (const auto& filename : config.input_files)
        {
            // Add file separator to combined output
            if (output_stream && !first_knot_in_output && config.input_files.size() > 1)
            {
                *output_stream << "%file " << filename << "\n";
            }

            // Route .kndlxyz files to the specialized handler
            std::filesystem::path fpath(filename);
            if (fpath.extension() == ".kndlxyz")
            {
                if (!ProcessXYZFile(filename, output_stream, config,
                                    stats, first_knot_in_output))
                {
                    success = false;
                }
                else
                {
                    ++stats.files_processed;
                }
                continue;
            }

            std::ifstream file(filename);
            if (!file)
            {
                LogError("Failed to open input file: " + filename);
                success = false;
                continue;
            }

            if (!ProcessSource(file, filename, output_stream, config, rng,
                               stats, first_knot_in_output))
            {
                success = false;
            }
            else
            {
                ++stats.files_processed;
            }
        }
    }

    // Final report for multiple files
    if (config.quiet && stats.total_knots > 0)
    {
        // End the counter line before final report
        *g_log_stream << "\n";
    }

    if (stats.total_knots > 1)
    {
        WriteFinalReport(stats, config);
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
