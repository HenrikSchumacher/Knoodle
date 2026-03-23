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
using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
using Energy_T    = PDC_T::Energy_T;
using LinkEmb_T   = Knoodle::LinkEmbedding<Real, Int, float>;
using Clock       = std::chrono::steady_clock;
using Duration    = std::chrono::duration<double>;

namespace {

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

/**
 * @brief Initialize logging based on mode.
 *
 * @param streaming_mode If true, redirect log output to a file.
 * @param log_filename The filename for the log file (used only in streaming mode).
 * @return true on success.
 */
bool InitLogging(bool streaming_mode, const std::string& log_filename)
{
    if (streaming_mode)
    {
        g_log_file.open(log_filename);
        if (!g_log_file)
        {
            std::cerr << "Error: Failed to open " << log_filename << " for writing.\n";
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

std::string EnergyName(Energy_T e)
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

/**
 * @brief Create a PlanarDiagram from 3D vertex coordinates.
 *
 * @param vertices Flat array of 3D coordinates (x, y, z, x, y, z, ...).
 * @param randomize_projection Whether to apply a random shear to the projection.
 * @param rng Random number generator.
 * @return The constructed PlanarDiagram, or an invalid diagram on error.
 */
PD_T CreateDiagramFrom3D(const std::vector<Real>& vertices,
                         bool randomize_projection,
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

    if (randomize_projection)
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

    // Create PlanarDiagram from 3D coordinates using static factory
    auto [pd, unlinks] = PD_T::FromKnotEmbedding(coords.data(), vertex_count);

    if (!pd.ValidQ())
    {
        LogError("FromKnotEmbedding failed to create a valid diagram");
        return PD_T();
    }

    return pd;
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
            return PD_T::template FromPDCode<false, true>(
                crossings.data(), crossing_count, false, true
            );
        case 7:
            return PD_T::template FromPDCode<true, true>(
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
 * @param input The input stream.
 * @param randomize_projection Whether to apply random shear to 3D projection.
 * @param rng Random number generator.
 * @param source_name Description of the source (filename or "stdin").
 * @param[out] reached_eof Set to true if we hit EOF.
 * @return The parsed InputKnot, or nullopt on error.
 */
std::optional<InputKnot> ReadKnot(std::istream& input,
                                   bool randomize_projection,
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

                PD_T pd = CreateDiagramFrom3D(geometry_vertices, randomize_projection, rng);
                if (!pd.ValidQ())
                {
                    LogError("Failed to create diagram from 3D geometry");
                    return false;
                }

                result.summands.push_back(std::move(pd));
                geometry_vertices.clear();
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
            LogError("Unrecognized format (expected 3, 4, 5, 6, or 7 values): " + cleaned);
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
