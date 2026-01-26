/**
 * @file main.cpp
 * @brief knoodletool - A command-line tool for knot diagram simplification.
 *
 * This tool reads knot diagrams (as PD codes or 3D geometry), simplifies them
 * using various algorithms, and outputs the simplified diagrams.
 *
 * Usage: knoodletool [options] [input_files...]
 *
 * Input formats:
 *   - PD code: Lines with 4 or 5 tab-separated integers per crossing
 *   - 3D geometry: Lines with 3 tab-separated numbers (coordinates)
 *
 * Output format (knoodletool format):
 *   k           <- knot separator
 *   s           <- summand separator
 *   1  2  3  4  1   <- crossing (5 integers: 4 arcs + handedness)
 *   ...
 *
 * See --help for full option list.
 */

// Jason, you can let the compiler have the following flags defined
//#define KNOODLE_USE_UMFPACK  // Support for the "Dirichlet" and "Bending" energies in Reapr.
//#define KNOODLE_USE_CLP      // Support some LP problems in Reapr and OrthoDraw; inferior to MCF.
//#define KNOODLE_USE_BOOST_UNORDERED // Support for faster associative containers in Reapr.

// Only KNOODLE_USE_BOOST_UNORDERED might be of some interest here because OrthoDraw and Reapr uses some of the containers provided by this a little. However, the containers sizes should be small in practive, so the corresponding fall back containers of the STL will be good enough. I doubt that anybody will measure some difference, but you are free to do so.

#include "../Knoodle.hpp"
#include "../src/OrthoDraw.hpp"
#include "../Reapr.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

//==============================================================================
// Type Aliases
//==============================================================================

using Int         = std::int64_t;
using Real        = double;
using PD_T        = Knoodle::PlanarDiagram<Int>;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
using Reapr_T     = Knoodle::Reapr<Real, Int>;
using Link_T      = Knoodle::Link_2D<Real, Int, Real>;
using Clock       = std::chrono::steady_clock;
using Duration    = std::chrono::duration<double>;
using Flag_T      = Reapr_T::EnergyFlag_T;

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
    int  simplify_level      = 6;            ///< Simplification level (3, 4, 5, or 6+)
    Int  max_reapr_attempts  = 25;           ///< Max iterations for Reapr::Rattle
    bool no_compaction       = false;        ///< Skip compaction in OrthoDraw (Reapr only)
    std::optional<Flag_T> reapr_energy;      ///< Energy flag for Reapr (if set)
    
    // Input options
    std::vector<std::string> input_files;    ///< Input file paths
    bool streaming_mode      = false;        ///< Read from stdin, write to stdout
    bool randomize_projection = false;       ///< Apply random shear to 3D projection
    
    // Output options
    std::optional<std::string> output_file;  ///< Single output file (if specified)
    bool output_ascii_drawing = false;       ///< Generate ASCII art drawings
    bool quiet               = false;        ///< Suppress per-knot reports, show counter only
    
    // Derived state
    bool help_requested      = false;  ///< User requested help
};

/// Minimum valid simplification level.
constexpr int kMinSimplifyLevel = 0;

/// Threshold at or above which Reapr is used instead of SimplifyN.
constexpr int kReaprThreshold = 6;

//==============================================================================
// Timing Utilities
//==============================================================================

/**
 * @brief Simple RAII timer that records elapsed time to a Duration reference.
 */
class ScopedTimer
{
public:
    explicit ScopedTimer(Duration& target)
        : target_(target), start_(Clock::now()) {}
    
    ~ScopedTimer()
    {
        target_ = Clock::now() - start_;
    }
    
private:
    Duration& target_;
    Clock::time_point start_;
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
    
    // For 3D geometry input
    bool              had_3d_geometry = false;
    Int               vertex_count_3d = 0;
    
    std::string       source_description;  ///< File name or "stdin"
};

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
// Logging
//==============================================================================

/// Output stream for user messages (may be redirected to file in streaming mode)
std::ostream* g_log_stream = &std::cerr;

/// Log file for streaming mode
std::ofstream g_log_file;

/**
 * @brief Initialize logging based on mode.
 */
bool InitLogging(const Config& config)
{
    if (config.streaming_mode)
    {
        g_log_file.open("knoodletool.log");
        if (!g_log_file)
        {
            std::cerr << "Error: Failed to open knoodletool.log for writing.\n";
            return false;
        }
        g_log_stream = &g_log_file;
    }
    return true;
}

/**
 * @brief Log a message to the appropriate stream.
 */
void Log(const std::string& msg)
{
    *g_log_stream << msg << "\n";
}

/**
 * @brief Log an error message.
 */
void LogError(const std::string& msg)
{
    *g_log_stream << "Error: " << msg << "\n";
}

//==============================================================================
// String Utilities
//==============================================================================

/**
 * @brief Convert a string to lowercase.
 */
std::string ToLower(std::string_view s)
{
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
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
    values.clear(); // Clearing the container without erasing it; values.size() will return 0.
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
// Command-Line Parsing
//==============================================================================

std::string ValidEnergies()
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
 * @brief Print usage information.
 */
void PrintUsage()
{
    
    
    Log("Usage: knoodletool [options] [input_files...]");
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
    Log("Input options:");
    Log("  --input=FILE                Specify input file (can use multiple times)");
    Log("  --streaming-mode            Read from stdin, write to stdout");
    Log("  --randomize-projection      Apply random shear to 3D geometry projection");
    Log("");
    Log("Output options:");
    Log("  --output=FILE               Write all output to FILE");
    Log("  --output-ascii-drawing      Generate ASCII art diagrams");
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
                config.reapr_energy = Flag_T::TV;
            }
            else if (energy_str == "dirichlet")
            {
                config.reapr_energy = Flag_T::Dirichlet;
            }
            else if (energy_str == "bending")
            {
                config.reapr_energy = Flag_T::Bending;
            }
            else if (energy_str == "height")
            {
                config.reapr_energy = Flag_T::Height;
            }
            else if (energy_str == "tv_clp")
            {
                config.reapr_energy = Flag_T::TV_CLP;
            }
            else if (energy_str == "tv_mcf")
            {
                config.reapr_energy = Flag_T::TV_MCF;
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
        // ASCII drawing output
        else if (arg == "--output-ascii-drawing")
        {
            config.output_ascii_drawing = true;
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
    
    if (config.streaming_mode && config.output_ascii_drawing)
    {
        LogError("--streaming-mode and --output-ascii-drawing are incompatible");
        return std::nullopt;
    }
    
    return config;
}

//==============================================================================
// Input Parsing
//==============================================================================

/**
 * @brief Detect input format from a data line.
 *
 * @return 3 for 3D geometry, 4 for unsigned PD code, 5 for signed PD code, 0 for unknown.
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
    return 0;  // Unknown
}

/**
 * @brief Create a PlanarDiagram from 3D vertex coordinates.
 *
 * @param vertices Flat array of 3D coordinates (x, y, z, x, y, z, ...).
 * @param config Configuration (for randomize_projection).
 * @param rng Random number generator.
 * @return The constructed PlanarDiagram, or an invalid diagram on error.
 */
PD_T CreateDiagramFrom3D(const std::vector<Real>& vertices,
                         const Config& config,
                         Knoodle::PRNG_T& rng)
{
    Int vertex_count = static_cast<Int>(vertices.size() / 3);
    
    if (vertex_count < 3)
    {
        LogError("3D geometry requires at least 3 vertices");
        return PD_T();
    }
    
    // Check if last vertex equals first (closed polygon marker)
    const Real* first = vertices.data();
    const Real* last  = vertices.data() + 3 * (vertex_count - 1);
    
    constexpr Real eps = 1e-10;
    bool closed_marker = (std::abs(first[0] - last[0]) < eps &&
                          std::abs(first[1] - last[1]) < eps &&
                          std::abs(first[2] - last[2]) < eps);
    
    if (closed_marker)
    {
        --vertex_count;  // Don't include duplicate vertex
    }
    
    // Create transformed coordinates if needed
    std::vector<Real> coords(vertices.begin(), vertices.begin() + 3 * vertex_count);
    
    if (config.randomize_projection)
    {
        // Apply shear: (x, y, z) -> (x + eps_x * z, y + eps_y * z, z)
        std::uniform_real_distribution<Real> dist(-0.5, 0.5);
        Real eps_x = dist(rng);
        Real eps_y = dist(rng);
        
        for (Int i = 0; i < vertex_count; ++i)
        {
            Real z = coords[3 * i + 2];
            coords[3 * i + 0] += eps_x * z;
            coords[3 * i + 1] += eps_y * z;
        }
    }
    
    // Create Link_2D and find intersections
    Link_T link(vertex_count);
    link.ReadVertexCoordinates(coords.data());
    
    int err = link.template FindIntersections<true>();
    if (err != 0)
    {
        LogError("FindIntersections failed with error code " + std::to_string(err));
        return PD_T();
    }
    
    // Construct PlanarDiagram from the link
    return PD_T(link);
}

/**
 * @brief Create a PlanarDiagram from PD code data.
 *
 * @param crossings Flat array of crossing data.
 * @param crossing_count Number of crossings.
 * @param format 4 for unsigned, 5 for signed PD code.
 * @return The constructed PlanarDiagram.
 */
PD_T CreateDiagramFromPDCode(const std::vector<Int>& crossings,
                              Int crossing_count,
                              int format)
{
    if (format == 5)
    {
        return PD_T::FromSignedPDCode(
            crossings.data(),
            crossing_count,
            Int(0),  // unlinks
            true,    // compress
            false    // not proven minimal
        );
    }
    else
    {
        return PD_T::FromUnsignedPDCode(
            crossings.data(),
            crossing_count,
            Int(0),
            true,
            false
        );
    }
}

/**
 * @brief Read a single knot from an input stream.
 *
 * Reads until EOF, a 'k' line, or stream error.
 *
 * @param input The input stream.
 * @param config Configuration options.
 * @param rng Random number generator.
 * @param source_name Description of the source (filename or "stdin").
 * @param[out] reached_eof Set to true if we hit EOF.
 * @return The parsed InputKnot, or nullopt on error.
 */
std::optional<InputKnot> ReadKnot(std::istream& input,
                                   const Config& config,
                                   Knoodle::PRNG_T& rng,
                                   const std::string& source_name,
                                   bool& reached_eof)
{
    InputKnot result;
    result.source_description = source_name;
    reached_eof = false;
    
    // Current summand data
    std::vector<Real>  geometry_vertices;
    std::vector<Int>   pd_crossings;
    Int                pd_crossing_count = 0;
    int                detected_format = 0;  // 0=unknown, 3=geometry, 4/5=PD code
    bool               in_summand = false;
    bool               first_data_seen = false;
    
    // Lambda to finalize current summand
    auto finalize_summand = [&]() -> bool
    {
        if (!in_summand) return true;
        
        if (detected_format == 3)
        {
            // 3D geometry
            if (!geometry_vertices.empty())
            {
                if (!result.summands.empty())
                {
                    LogError("3D geometry input cannot have multiple summands");
                    return false;
                }
                
                result.had_3d_geometry = true;
                result.vertex_count_3d = static_cast<Int>(geometry_vertices.size() / 3);
                
                PD_T pd = CreateDiagramFrom3D(geometry_vertices, config, rng);
                if (!pd.ValidQ())
                {
                    LogError("Failed to create diagram from 3D geometry");
                    return false;
                }
                
                result.summands.push_back(std::move(pd));
                geometry_vertices.clear();
            }
        }
        else if (detected_format == 4 || detected_format == 5)
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
    values.reserve(5); // We expect at most 5 elements per line, so we can allocate for that purpose.
    // The container will be expanded automatically, if that does not suffice.
    
    while (std::getline(input, line))
    {
        std::string cleaned = CleanLine(line);
        
        // Skip empty lines
        if (cleaned.empty()) continue;
        
        // Check for markers
        if (cleaned == "k" || cleaned == "K")
        {
            if (first_data_seen)
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
            // Start of new summand
            if (!finalize_summand()) return std::nullopt;
            in_summand = true;
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
            LogError("Unrecognized format (expected 3, 4, or 5 values): " + cleaned);
            return std::nullopt;
        }
        
        if (!first_data_seen)
        {
            detected_format = line_format;
            first_data_seen = true;
            in_summand = true;
        }
        else if (line_format != detected_format)
        {
            // Format mismatch within same summand
            if (line_format == 3 || detected_format == 3)
            {
                LogError("Cannot mix 3D geometry and PD code formats");
                return std::nullopt;
            }
            // 4 vs 5 integers - keep using detected format
            if (static_cast<int>(values.size()) != detected_format)
            {
                LogError("Inconsistent PD code format (expected " + 
                         std::to_string(detected_format) + " values)");
                return std::nullopt;
            }
        }
        
        // Store data
        if (detected_format == 3)
        {
            for (Real v : values)
            {
                geometry_vertices.push_back(v);
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
    
    // Check if we got any data
    if (result.summands.empty())
    {
        return std::nullopt;  // No data found
    }
    
    // Compute crossing counts
    for (const auto& pd : result.summands)
    {
        Int cc = pd.CrossingCount();
        result.crossing_counts.push_back(cc);
        result.total_crossings += cc;
    }
    
    return result;
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
            else if (config.simplify_level == 3)
            {
                PD_T pd(pd_in);
                pd.Simplify3();
                result.summands.push_back(std::move(pd));
            }
            else if (config.simplify_level == 4)
            {
                PD_T pd(pd_in);
                pd.Simplify4();
                result.summands.push_back(std::move(pd));
            }
            else if (config.simplify_level == 5)
            {
                PD_T pd(pd_in);
                std::vector<PD_T> pd_list;
                pd.Simplify5(pd_list);
                pd_list.push_back(std::move(pd));
                
                for (auto& p : pd_list)
                {
                    result.summands.push_back(std::move(p));
                }
            }
            else  // Reapr
            {
                Reapr_T reapr;
                
                if (config.no_compaction)
                {
                    auto settings = reapr.OrthoDrawSettings();
                    settings.compaction_method = OrthoDraw_T::CompactionMethod_T::Unknown;
                    reapr.SetOrthoDrawSettings(settings);
                }
                
                if (config.reapr_energy.has_value())
                {
                    reapr.SetEnergyFlag(*config.reapr_energy);
                }
                
                std::vector<PD_T> pd_list = reapr.Rattle(pd_in, config.max_reapr_attempts);
                
                if (pd_list.empty())
                {
                    // Rattle simplified to an unknot (0 crossings, discarded)
                    ++result.unknot_count;
                }
                else
                {
                    for (auto& p : pd_list)
                    {
                        result.summands.push_back(std::move(p));
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
 * @brief Write a simplified knot in knoodletool format.
 *
 * @param knot The simplified knot (non-const because PDCode() modifies internal buffers).
 * @param output The output stream.
 * @param include_k_marker Whether to include the leading 'k' marker.
 */
void WriteKnot(SimplifiedKnot& knot, std::ostream& output, bool include_k_marker)
{
    if (include_k_marker)
    {
        output << "k\n";
    }
    
    for (std::size_t i = 0; i < knot.summands.size(); ++i)
    {
        output << "s\n";
        
        auto& pd = knot.summands[i];
        auto pd_code = pd.PDCode();
        
        Int crossing_count = pd.CrossingCount();
        
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

/**
 * @brief Write ASCII art diagrams for a simplified knot.
 *
 * @param knot The simplified knot.
 * @param filepath Output file path.
 * @return true on success.
 */
bool WriteAsciiDrawings(SimplifiedKnot& knot, const std::filesystem::path& filepath)
{
    std::ofstream file(filepath);
    if (!file)
    {
        LogError("Failed to open " + filepath.string() + " for writing");
        return false;
    }
    
    OrthoDraw_T::Settings_T settings{
        .x_grid_size = 8,
        .y_grid_size = 4,
        .x_gap_size  = 1,
        .y_gap_size  = 1
    };
    
    for (std::size_t i = 0; i < knot.summands.size(); ++i)
    {
        if (i > 0)
        {
            file << "s\n";
        }
        
        OrthoDraw_T H(knot.summands[i], Int(-1), settings);
        file << H.DiagramString() << "\n";
    }
    
    return true;
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
            // I alreay created an overload of the ToString function for this purpose.
            level_str += Knoodle::ToString(*config.reapr_energy);
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
            // I alreay created an overload of the ToString function for this purpose.
            level_str += Knoodle::ToString(*config.reapr_energy);
            
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
 * @brief Generate drawing filename from input filename.
 */
std::filesystem::path GetDrawingFilename(const std::filesystem::path& input_path)
{
    auto stem = input_path.stem().string();
    return input_path.parent_path() / (stem + "_simplified_drawing.txt");
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
            input_knot = ReadKnot(input, config, rng, source_name, reached_eof);
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
        
        // Output phase
        Duration output_time{0};
        
        {
            ScopedTimer timer(output_time);
            
            if (output_stream)
            {
                // Writing to shared output stream
                WriteKnot(simplified, *output_stream, !first_knot_in_output || !config.streaming_mode);
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
                
                WriteKnot(simplified, file, true);
                
                if (config.output_ascii_drawing)
                {
                    std::filesystem::path drawing_path = GetDrawingFilename(input_path);
                    WriteAsciiDrawings(simplified, drawing_path);
                }
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
    if (!InitLogging(config))
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
        Log("knoodletool in stream mode, expect input from stdin, will direct output to stdout");
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
            std::ifstream file(filename);
            if (!file)
            {
                LogError("Failed to open input file: " + filename);
                success = false;
                continue;
            }
            
            // Add file separator to combined output
            if (output_stream && !first_knot_in_output && config.input_files.size() > 1)
            {
                *output_stream << "%file " << filename << "\n";
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
