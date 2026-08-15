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
//#define TOOLS_USE_BOOST_UNORDERED // Support for faster associative containers in Reapr.

// Only TOOLS_USE_BOOST_UNORDERED might be of some interest here because OrthoDraw and Reapr uses some of the containers provided by this a little. However, the containers sizes should be small in practive, so the corresponding fall back containers of the STL will be good enough. I doubt that anybody will measure some difference, but you are free to do so.

#include "knoodle_io.hpp"

// The command line -- Config, ParseArguments, BuildSimplifyArgs -- lives in
// its own header so test/simplify_config_check can call it directly.
#include "knoodlesimplify_config.hpp"

#include <cstdio>
#include <filesystem>
#include <limits>

//==============================================================================
// Configuration
//==============================================================================

namespace {

// The timing aliases moved into namespace knoodle_io (see knoodle_io.hpp) to
// avoid a global-scope clash with Apple's <MacTypes.h> `Duration`. This tool
// never pulls in the Accelerate backend, so bringing the alias local to this
// anonymous namespace is safe and keeps the timing fields below unqualified.
using knoodle_io::Duration;

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



/**
 * @brief Simplify a knot using the configured algorithm.
 *
 * @param input The input knot with its summands.
 * @param config Configuration options.
 * @param output_pdc If non-null, every resulting diagram (trivial or not) is
 *        ALSO pushed here, for --format=pdc output -- unlike `result`, which
 *        the standard TSV writer treats losslessly only up to the point where
 *        a summand becomes trivial (see knoodle_io.hpp's unknot_colors doc
 *        comment), this preserves full color fidelity via
 *        PlanarDiagramComplex's own native serialization.
 * @return The simplified knot with timing information.
 */
SimplifiedKnot SimplifyKnot(const InputKnot& input, const Config& config, PDC_T* output_pdc = nullptr)
{
    SimplifiedKnot result;

    // PDC-native format ('u <color>') can only represent a *colored* Anello:
    // PD_T::InvalidQ() is true for a 0-crossing diagram with an uninitialized
    // color, and WriteToFile silently skips invalid diagrams -- so an unknot
    // summand with no known color (colorless input, e.g. a bare 's' line or
    // an uncolored 4/5-column PD) would otherwise vanish from --format=pdc
    // output entirely. A synthetic color from a high base (astronomically
    // unlikely to collide with any real, user-supplied color) keeps it in
    // the output instead; distinct summands get distinct synthetic colors.
    Int next_synthetic_color = (std::numeric_limits<Int>::max)() / 2;
    auto validColor = [&next_synthetic_color](Int color) -> Int
    {
        return (color == PD_T::Uninitialized) ? next_synthetic_color++ : color;
    };

    // Every resulting diagram (trivial or not, across all input summands) is
    // gathered here first, so --unite (see below) can be applied uniformly
    // before result.summands/unknot_count/output_pdc are derived from it --
    // regardless of whether --format=pdc was also requested.
    PDC_T all_pdc;

    // Push()/Clear() are lock-guarded: on a locked complex they do nothing and
    // warn, because they cannot verify that the caller's arc colors stay
    // consistent. Assembling diagrams we own and have already colored ourselves
    // is the sanctioned use, so unlock for the duration. Without this every
    // Push() below silently fails, all_pdc stays empty, and the tool reports
    // every input -- trefoil included -- as a 0-crossing unknot.
    all_pdc.Unlock();

    // Unknot summands that arrived as bare 's' lines pass through.
    for (Int color : input.unknot_colors)
    {
        all_pdc.Push(PD_T::Unknot(validColor(color)));
    }

    const PDC_T::Simplify_Args_T args = BuildSimplifyArgs(config);

    // Printed from the very object about to be passed, so there is no
    // second code path to drift. See PrintSimplifyArgs.
    if (config.debug_print_args) { PrintSimplifyArgs(args, std::cerr); }

    // 4- and 5-column PD codes carry no colors, so every arc arrives as
    // PD_T::Uninitialized. Simplification then splits the diagram into summands,
    // and the record of WHICH summands were once a single closed curve -- the
    // thing that distinguishes a connected sum from a split link -- exists
    // nowhere but the colors. Assign them up front, per link component, so that
    // record survives the split and can be written out below. Colored input
    // (6/7-column, PDC-native) and 3D input (colored by FromCoordinates) already
    // carry meaningful colors and must keep them.
    const bool assign_colors = (input.input_column_count == 4)
                            || (input.input_column_count == 5);

    auto colorize = [assign_colors](PD_T&& pd) -> PD_T
    {
        if (assign_colors)
        {
            // ComputeArcColors() is lock-guarded: it can change arc colors, which
            // in general breaks the class's invariants. Here it only fills in
            // colors that were never set, on a diagram we own outright.
            pd.Unlock();
            pd.ComputeArcColors();
            pd.Lock();
        }
        return std::move(pd);
    };

    {
        ScopedTimer timer(result.simplify_time);

        for (const PD_T& pd_in : input.summands)
        {
            if (config.simplify_level == 0)
            {
                // No simplification - just copy the PD
                all_pdc.Push(colorize(PD_T(pd_in)));
            }
            else
            {
                // Use PlanarDiagramComplex for all simplification levels
                PD_T pd_copy = colorize(PD_T(pd_in));
                PDC_T pdc(std::move(pd_copy));

                pdc.Simplify(args);

                if (pdc.DiagramCount() == 0)
                {
                    // Nothing (not even a trivial done-diagram) survived in
                    // pdc itself to read a color off of; fall back to the
                    // color the original, un-simplified summand carried.
                    all_pdc.Push(PD_T::Unknot(validColor(pd_in.ArcColors()[0])));
                }
                else
                {
                    for (Int i = 0; i < pdc.DiagramCount(); ++i)
                    {
                        PD_T pd(pdc.Diagram(i));
                        if (pd.CrossingCount() == 0)
                        {
                            all_pdc.Push(PD_T::Unknot(validColor(pd.FirstColor())));
                        }
                        else
                        {
                            all_pdc.Push(std::move(pd));
                        }
                    }
                }
            }
        }
    }

    // --unite: connect-sum same-colored diagrams back together, so the
    // result is one diagram per physically split link component instead of
    // one per diagrammatically-prime factor -- e.g. for exporting a single
    // PD code per component to KnotTheory/Regina.
    //
    // PDC::Unite/Union() is NOT the right tool for this, despite the name:
    // it just packs multiple diagrams' crossing/arc data into one PD_T's
    // arrays side by side (offset indices), without actually splicing any
    // arcs together -- the result is still, topologically, several
    // disconnected diagram components bundled in one PD_T. Verified this the
    // hard way: OrthoDraw correctly refuses it ("Input planar diagram has
    // more than one diagram components", a crash prior to that check being
    // hit defensively here). The real connect-sum operation is
    // PDC::Connect()/ConnectedSum() (Connect.hpp) -- it groups by color, then
    // performs actual arc surgery (PD_T::Connect(a,b)) between a
    // representative diagram and every other same-colored one, and already
    // handles Anelli exactly as intended (a color with any non-trivial
    // diagram absorbs that color's unknots -- a no-op connect-sum; a color
    // with only unknots keeps exactly one).
    if (config.unite)
    {
        all_pdc.Connect();
    }

    // Derive result.summands/unknot_count (and output_pdc, if requested)
    // from the final (possibly united) diagram list.
    for (Int i = 0; i < all_pdc.DiagramCount(); ++i)
    {
        PD_T pd(all_pdc.Diagram(i));

        // Same lock caveat as all_pdc above; the caller hands us a fresh
        // (locked) complex, so unlock before the first Push into it.
        if (output_pdc) { output_pdc->Unlock(); output_pdc->Push(PD_T(pd)); }

        if (pd.CrossingCount() == 0)
        {
            ++result.unknot_count;
        }
        else
        {
            result.summands.push_back(std::move(pd));
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

    // Unknot summands are written as bare 's' lines (0-crossing diagrams).
    for (Int i = 0; i < knot.unknot_count; ++i)
    {
        output << "s\n";
    }

    for (std::size_t i = 0; i < knot.summands.size(); ++i)
    {
        output << "s\n";

        auto& pd = knot.summands[i];
        Int crossing_count = pd.CrossingCount();

        if (colored_output)
        {
            auto pd_code = pd.template PDCode<Int, {.signQ = true, .colorQ = true}>();

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
            auto pd_code = pd.template PDCode<Int, {.signQ = true, .colorQ = false}>();

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

/**
 * @brief Write a simplified knot, choosing --format=pdc (PlanarDiagramComplex's
 *        own native serialization, full color fidelity) over the usual TSV
 *        writer when output_pdc is non-null.
 */
bool WriteSimplified(SimplifiedKnot& knot, PDC_T* output_pdc, std::ostream& output,
                      bool include_k_marker, bool colored_output)
{
    if (output_pdc)
    {
        return WritePdcNativeFormat(*output_pdc, output, include_k_marker);
    }
    WriteKnot(knot, output, include_k_marker, colored_output);
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
    else if (config.simplify_level == 1) level_str = "R1 only (local)";
    else if (config.simplify_level == 2) level_str = "R1 + R2 (local)";
    else if (config.simplify_level == 3) level_str = "All local Reidemeister moves";
    else if (config.simplify_level == 4) level_str = "Path rerouting";
    else if (config.simplify_level == 5) level_str = "Rerouting + summand detection";
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
    else if (config.simplify_level == 1) level_str = "R1 only (local)";
    else if (config.simplify_level == 2) level_str = "R1 + R2 (local)";
    else if (config.simplify_level == 3) level_str = "All local Reidemeister moves";
    else if (config.simplify_level == 4) level_str = "Path rerouting";
    else if (config.simplify_level == 5) level_str = "Rerouting + summand detection";
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
        LinkEmb_T link = LinkEmb_T::FromFile(std::filesystem::path(filepath));

        if (config.randomize_projection)
        {
            // Rotate the whole embedding at once (not each component
            // independently, which would distort the link's actual geometric
            // arrangement) with a proper random rotation -- the same mechanism
            // already used elsewhere (PlanarDiagramComplex/Simplify.hpp:
            // emb.Transform(reapr.RandomRotation())).
            Reapr_T reapr;
            link.Transform(reapr.RandomRotation());
        }

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
    PDC_T output_pdc;
    SimplifiedKnot simplified = SimplifyKnot(input_knot, config, config.pdc_format ? &output_pdc : nullptr);

    // Output phase (always 7-column / colored for .kndlxyz)
    Duration output_time{0};

    {
        ScopedTimer timer(output_time);

        if (output_stream)
        {
            WriteSimplified(simplified, config.pdc_format ? &output_pdc : nullptr, *output_stream,
                             !first_knot_in_output, true);
            first_knot_in_output = false;
        }
        else
        {
            std::filesystem::path input_path(filepath);
            std::filesystem::path output_path = GetSimplifiedFilename(input_path);

            // Staged: committed only if nothing went wrong producing this knot.
            const long errors_before = ErrorTotal();

            AtomicOutFile file(output_path);
            if (!file.Good())
            {
                LogError("Failed to open " + output_path.string() + " for writing");
                return false;
            }

            WriteSimplified(simplified, config.pdc_format ? &output_pdc : nullptr, file.Stream(), true, true);

            if (ErrorTotal() != errors_before)
            {
                file.Abort();
                *g_log_stream << "Refusing to write " << output_path.string()
                              << ": the library reported an error while producing it.\n";
                return false;
            }
            if (!file.Commit())
            {
                LogError("Failed to move output into place: " + output_path.string());
                return false;
            }
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
        PDC_T output_pdc;
        SimplifiedKnot simplified = SimplifyKnot(*input_knot, config, config.pdc_format ? &output_pdc : nullptr);

        // Determine output format based on input column count.
        //
        // Colors are also written whenever the result has more than one summand,
        // even for uncolored input. Splitting a diagram is exactly when the color
        // stops being redundant: it is the only thing recording that two summands
        // were once one closed curve (a connected sum) rather than two (a split
        // link), and a consumer such as `knoodledraw --embedding` cannot rebuild
        // the correct link type without it. SimplifyKnot has already given
        // uncolored input real per-link-component colors for this purpose.
        const bool split_into_summands =
            (simplified.summands.size() + static_cast<std::size_t>(simplified.unknot_count)) > 1;

        bool colored_output = (input_knot->input_column_count >= 6) || split_into_summands;

        // Output phase
        Duration output_time{0};

        {
            ScopedTimer timer(output_time);

            if (output_stream)
            {
                // Writing to shared output stream
                WriteSimplified(simplified, config.pdc_format ? &output_pdc : nullptr, *output_stream,
                                 !first_knot_in_output || !config.streaming_mode,
                                 colored_output);
                first_knot_in_output = false;
            }
            else if (!config.streaming_mode)
            {
                // Per-file output. Staged, and committed only if nothing went
                // wrong while producing this knot (see AtomicOutFile).
                std::filesystem::path input_path(source_name);
                std::filesystem::path output_path = GetSimplifiedFilename(input_path);

                const long errors_before = ErrorTotal();

                AtomicOutFile file(output_path);
                if (!file.Good())
                {
                    LogError("Failed to open " + output_path.string() + " for writing");
                    return false;
                }

                WriteSimplified(simplified, config.pdc_format ? &output_pdc : nullptr, file.Stream(), true, colored_output);

                if (ErrorTotal() != errors_before)
                {
                    file.Abort();
                    *g_log_stream << "Refusing to write " << output_path.string()
                                  << ": the library reported an error while producing it.\n";
                    return false;
                }
                if (!file.Commit())
                {
                    LogError("Failed to move output into place: " + output_path.string());
                    return false;
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
    // Watch std::cerr for the library's "ERROR: " lines for the whole run. Must
    // outlive every write below, since the commit decision at the end depends on
    // what it counted. See knoodle_io.hpp for why tapping the stream is the only
    // handle we have on Tools' eprint.
    CerrErrorTap cerr_tap;
    g_cerr_tap = &cerr_tap;

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
    // Decide where diagnostics go before anything else can write one: the log
    // opens in InitLogging just below, and the library reads KNOODLE_DUMP_DIR at
    // the moment Rattle's projection fails (its default is the user's home
    // directory). Alongside the output file when we have one, else a per-process
    // temp directory. Snapshot what is already there so we only report our own.
    const std::filesystem::path diag_dir =
        ChooseDiagnosticDir("knoodlesimplify", config.streaming_mode, config.output_file);
    const std::set<std::string> bundles_before = ListDiagnosticBundles(diag_dir);

    if (!InitLogging(config.streaming_mode, "knoodlesimplify.log"))
    {
        return EXIT_FAILURE;
    }

    // Initialize random number generator
    Knoodle::PRNG_T rng = Knoodle::InitializedRandomEngine<Knoodle::PRNG_T>();

    // Statistics accumulator
    ProcessingStats stats;
    bool first_knot_in_output = true;

    // Prepare output stream if single output file specified.
    //
    // The file is staged as "<name>.partial" and only renamed into place if the
    // run finishes without the library or this tool reporting an error -- see
    // AtomicOutFile and the commit decision at the end of main(). Results are
    // written knot by knot, long before we know whether a later knot will fail,
    // so staging is the only way to honour "no output on error".
    std::optional<AtomicOutFile> output_file;
    std::ostream* output_stream = nullptr;

    if (config.streaming_mode)
    {
        Log("knoodlesimplify in stream mode, expect input from stdin, will direct output to stdout");
        output_stream = &std::cout;
    }
    else if (config.output_file)
    {
        output_file.emplace(*config.output_file);
        if (!output_file->Good())
        {
            LogError("Failed to open output file: " + *config.output_file);
            return EXIT_FAILURE;
        }
        output_stream = &output_file->Stream();
    }

    // Process inputs
    bool success = true;

    if (config.streaming_mode)
    {
        // Read from stdin. In streaming mode Log is redirected to a file, so write
        // the interactive-tty notice straight to stderr (else a bare invocation of
        // `knoodlesimplify --streaming-mode` just looks hung).
        if (StdinIsInteractive())
        {
            std::cerr << "knoodlesimplify: reading diagrams from stdin (Ctrl-D to end). "
                         "Pipe a stream or pass a file; --help for usage.\n";
        }
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

    // Fail loudly. The core library signals unrecoverable trouble by calling
    // eprint and then handing back a diagram it has already disclaimed ("Returning
    // an invalid diagram. Check your results carefully."), and the PDC writers skip
    // invalid diagrams silently -- so without this the run would write a quietly
    // truncated file and exit 0. Refuse the output instead, and say why.
    if (ErrorsSeen())
    {
        std::string notice;
        if (output_file)
        {
            output_file->Abort();
            notice = "\nRefusing to write " + output_file->FinalPath().string()
                   + ": " + ErrorSummary() + " during this run.\n"
                     "The output would be unreliable (the library discards diagrams it"
                     " has flagged as invalid), so no file was produced.\n";
        }
        else if (config.streaming_mode)
        {
            // Already-piped bytes cannot be recalled; the exit code is the contract.
            notice = "\nknoodlesimplify: " + ErrorSummary()
                   + " during this run -- output already written to stdout is"
                     " UNRELIABLE and should be discarded.\n";
        }
        else
        {
            notice = "\nknoodlesimplify: " + ErrorSummary()
                   + " during this run; per-file outputs were withheld.\n";
        }

        // Always reach the terminal. In streaming mode g_log_stream is the log
        // FILE, which is precisely where someone piping output would never look.
        std::cerr << notice;
        if (g_log_stream != &std::cerr) { *g_log_stream << notice; }

        // Drop everything needed to reproduce into one forwardable file. The
        // Simplify args matter most: the failures we have chased so far only
        // appear at high --max-reapr-attempts / --reapr-rotation-trials, and a
        // pasted stderr tail never carries them.
        std::string invocation;
        for (int i = 0; i < argc; ++i)
        {
            invocation += (i ? " " : "");
            invocation += argv[i];
        }

        std::string inputs;
        if (config.streaming_mode)
        {
            inputs = "  (stdin, --streaming-mode)\n";
        }
        for (const auto& f : config.input_files)
        {
            std::error_code ec;
            const auto size = std::filesystem::file_size(f, ec);
            inputs += "  " + f + (ec ? "  (size unknown)"
                                     : "  (" + std::to_string(size) + " bytes)") + "\n";
        }

        const auto report = WriteDiagnosticReport("knoodlesimplify", {
            { "command line",     "  " + invocation + "\n" },
            { "input files",      inputs },
            { "simplify options", "  " + ToString(BuildSimplifyArgs(config)) + "\n" },
            { "what to send",
              "  This file, plus the input file(s) listed above.\n"
              "  The failing intermediate diagram and its 3D embedding live inside\n"
              "  the library and are not reachable from here; if a core-side dump is\n"
              "  available in your version, its files will be named alongside this one.\n" },
        });

        if (!report.empty())
        {
            std::cerr << "Wrote a diagnostic report to " << report.string()
                      << " -- please send it with any bug report.\n";
        }

        FinishDiagnostics(diag_dir, bundles_before, "knoodlesimplify", true);

        return EXIT_FAILURE;
    }

    if (output_file && !output_file->Commit())
    {
        LogError("Failed to move output into place: " + output_file->FinalPath().string());
        FinishDiagnostics(diag_dir, bundles_before, "knoodlesimplify", true);
        return EXIT_FAILURE;
    }

    FinishDiagnostics(diag_dir, bundles_before, "knoodlesimplify", !success);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
