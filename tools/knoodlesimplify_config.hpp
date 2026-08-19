#pragma once

/**
 * @file knoodlesimplify_config.hpp
 * @brief knoodlesimplify's command line: the Config struct, the argv parser,
 *        and the translation from Config to PDC_T::Simplify_Args_T.
 *
 * Split out of knoodlesimplify.cpp so it can be TESTED. This is clause 2 of the
 * tool contract -- "be a faithful CLI for the library call" -- and it happens to
 * be two pure functions:
 *
 *     ParseArguments(argc, argv)  ->  std::optional<Config>
 *     BuildSimplifyArgs(config)   ->  PDC_T::Simplify_Args_T
 *
 * so it can be checked by calling them and comparing structs, rather than by
 * running the tool and inferring from its output what call it must have made.
 * That distinction matters: inferring from output conflates this contract with
 * the library's, and can only see the fields somebody thought to print.
 *
 * The anonymous namespace is deliberate and matches knoodle_io.hpp: it is
 * per-translation-unit, so both knoodlesimplify.cpp and test/simplify_config_check
 * get their own copy and there is nothing to link against or collide with.
 *
 * Tested by test/simplify_config_check.
 */

#include "knoodle_io.hpp"

#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <ostream>
#include <vector>

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

    // Simplify_Args_T overrides (src/PlanarDiagramComplex/Simplify.hpp). All
    // optional: unset means "whatever --simplify-level's preset already
    // chose" (see BuildSimplifyArgs). splitQ is deliberately NOT exposed here
    // -- it is an internal simplification-quality knob (splitQ=false makes
    // Reapr skip multi-component diagrams outright, Simplify.hpp:313-324), not
    // the output-shape choice "unite"/"split" below control.
    std::optional<bool>     compress_initial;
    std::optional<int>      local_opt_level;          ///< 0-4
    std::optional<Knoodle::DijkstraStrategy_T> dijkstra_strategy;
    std::optional<Int>      start_max_dist;
    std::optional<Int>      final_max_dist;
    std::optional<bool>     reroute;                  ///< overrides the level>=4 preset
    std::optional<bool>     disconnect;                ///< overrides the level>=5 preset
    std::optional<bool>     compress;
    std::optional<Int>      compression_threshold;
    std::optional<Knoodle::Size_T> rotation_trials;
    std::optional<bool>     reapr_permute_random;
    std::optional<double>   reapr_scaling;
    std::optional<int>      randomize_bends;
    std::optional<bool>     randomize_virtual_edges;
    std::optional<PDC_T::Compaction_T> compaction_method;  ///< supersedes no_compaction if both given
    std::optional<bool>     canonicalize;

    // Output shape: split (default, matching Simplify's natural splitQ=true
    // output -- one diagram per diagrammatically-prime factor, same-colored
    // factors belonging to the same original component) vs. unite (connect-
    // sums same-colored factors back together via PDC::Unite, so the result
    // is one diagram per physically split component -- the natural form for
    // a single PD code per component, e.g. for KnotTheory/Regina).
    bool unite                = false;

    // Input options
    std::vector<std::string> input_files;    ///< Input file paths
    bool streaming_mode       = false;       ///< Read from stdin, write to stdout
    bool randomize_projection = false;       ///< Apply random shear to 3D projection

    // Output options
    std::optional<std::string> output_file;  ///< Single output file (if specified)
    bool quiet                = false;       ///< Suppress per-knot reports, show counter only
    bool pdc_format           = false;       ///< --format=pdc: PlanarDiagramComplex's own
                                              ///< native serialization (colors, including for
                                              ///< unknot summands, round-trip exactly)

    /// --debug-print-simplify-args: dump the Simplify_Args_T to stderr
    /// immediately before it is handed to Simplify, then carry on. Hidden
    /// (absent from --help) because it is a debugging aid, not an interface.
    bool debug_print_args     = false;

    // Derived state
    bool help_requested       = false;       ///< User requested help
};

/// Minimum valid simplification level.
constexpr int kMinSimplifyLevel = 0;

/// Threshold at or above which Reapr is used instead of SimplifyN.
constexpr int kReaprThreshold = 6;

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
    Log("                              local-only diagnostic tiers (no rerouting):");
    Log("                                1          Reidemeister I only");
    Log("                                2          Reidemeister I + II");
    Log("                                3          all local moves (incl. R_Ia/R_IIa)");
    Log("                              production pipeline (no local pass; tuned):");
    Log("                                4          path rerouting");
    Log("                                5          rerouting + summand detection");
    Log("                                6+/max/full/reapr");
    Log("                                           full Reapr pipeline (default)");
    Log("  --max-reapr-attempts=K      Maximum iterations for Reapr (default: 25)");
    Log("  --no-compaction             Skip compaction in OrthoDraw (Reapr only)");
    Log("  --reapr-energy=E            Set Reapr energy function (Reapr only):");
    Log("                                " + ValidEnergies());
    Log("");
    Log("Output shape:");
    Log("  --split                     One diagram per diagrammatically-prime factor,");
    Log("                                same-colored factors sharing a link component");
    Log("                                (default; natural input for knoodleidentify)");
    Log("  --unite                     Connect-sum same-colored factors back together,");
    Log("                                one diagram per physically split component");
    Log("                                (natural PD-code-per-component form for");
    Log("                                KnotTheory/Regina)");
    Log("");
    Log("Simplify_Args_T fine-tuning (all optional; unset = --simplify-level's own");
    Log("preset -- see src/PlanarDiagramComplex/Simplify.hpp):");
    Log("  --compress-initial / --no-compress-initial   Compress input before simplifying");
    Log("  --local-opt-level=N (0-4)   Local pattern optimization intensity");
    Log("  --dijkstra-strategy=S       unidirectional, alternating, bidirectional");
    Log("  --start-max-dist=N          Initial Dijkstra max search distance");
    Log("  --final-max-dist=N          Final Dijkstra max search distance");
    Log("  --reroute / --no-reroute    Enable rerouting passes");
    Log("  --disconnect / --no-disconnect   Enable disconnect simplification");
    Log("  --compress / --no-compress  Compress diagrams during simplification");
    Log("  --compression-threshold=N   Crossing-count threshold for compression");
    Log("  --reapr-rotation-trials=N   Random rotations tried per Reapr embedding (default: 25)");
    Log("  --reapr-permute-random / --no-reapr-permute-random");
    Log("                              Randomize arc permutation in Reapr");
    Log("  --reapr-scaling=X           3D grid scaling in Reapr (default: 1.0)");
    Log("  --randomize-bends=N         Bend randomization iterations (default: 4)");
    Log("  --randomize-virtual-edges / --no-randomize-virtual-edges");
    Log("                              Randomize virtual edges in OrthoDraw");
    Log("  --compaction-method=M       unknown, topological-numbering, topological-ordering,");
    Log("                                length-mcf (default), length-clp, area-length-clp");
    Log("  --canonicalize / --no-canonicalize   Canonicalize after simplification");
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
    Log("  --format=pdc                PlanarDiagramComplex's own native serialization");
    Log("                                (WriteToFile/FromInString) instead of the usual");
    Log("                                TSV -- colors, including for unknot summands,");
    Log("                                round-trip exactly");
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
        // Simplify_Args_T overrides (unset = --simplify-level's own preset)
        else if (arg == "--compress-initial")   { config.compress_initial = true; }
        else if (arg == "--no-compress-initial"){ config.compress_initial = false; }
        else if (arg.starts_with("--local-opt-level="))
        {
            try
            {
                int v = std::stoi(std::string(arg.substr(18)));
                if (v < 0 || v > 4) { LogError("local-opt-level must be 0-4"); return std::nullopt; }
                config.local_opt_level = v;
            }
            catch (const std::exception&) { LogError("Invalid local-opt-level value"); return std::nullopt; }
        }
        else if (arg.starts_with("--dijkstra-strategy="))
        {
            std::string v = ToLower(arg.substr(20));
            if      (v == "unidirectional") config.dijkstra_strategy = Knoodle::DijkstraStrategy_T::Unidirectional;
            else if (v == "alternating")    config.dijkstra_strategy = Knoodle::DijkstraStrategy_T::Alternating;
            else if (v == "bidirectional")  config.dijkstra_strategy = Knoodle::DijkstraStrategy_T::Bidirectional;
            else
            {
                LogError("Unknown dijkstra-strategy: '" + std::string(arg.substr(20)) + "'");
                LogError("Valid options: unidirectional, alternating, bidirectional");
                return std::nullopt;
            }
        }
        else if (arg.starts_with("--start-max-dist="))
        {
            try { config.start_max_dist = std::stoll(std::string(arg.substr(17))); }
            catch (const std::exception&) { LogError("Invalid start-max-dist value"); return std::nullopt; }
        }
        else if (arg.starts_with("--final-max-dist="))
        {
            try { config.final_max_dist = std::stoll(std::string(arg.substr(17))); }
            catch (const std::exception&) { LogError("Invalid final-max-dist value"); return std::nullopt; }
        }
        else if (arg == "--reroute")    { config.reroute = true; }
        else if (arg == "--no-reroute") { config.reroute = false; }
        else if (arg == "--disconnect")    { config.disconnect = true; }
        else if (arg == "--no-disconnect") { config.disconnect = false; }
        else if (arg == "--compress")    { config.compress = true; }
        else if (arg == "--no-compress") { config.compress = false; }
        else if (arg.starts_with("--compression-threshold="))
        {
            try { config.compression_threshold = std::stoll(std::string(arg.substr(24))); }
            catch (const std::exception&) { LogError("Invalid compression-threshold value"); return std::nullopt; }
        }
        else if (arg.starts_with("--reapr-rotation-trials="))
        {
            try
            {
                Int v = std::stoll(std::string(arg.substr(24)));
                if (v < 0) { LogError("reapr-rotation-trials must be non-negative"); return std::nullopt; }
                config.rotation_trials = static_cast<Knoodle::Size_T>(v);
            }
            catch (const std::exception&) { LogError("Invalid reapr-rotation-trials value"); return std::nullopt; }
        }
        else if (arg == "--reapr-permute-random")    { config.reapr_permute_random = true; }
        else if (arg == "--no-reapr-permute-random") { config.reapr_permute_random = false; }
        else if (arg.starts_with("--reapr-scaling="))
        {
            try { config.reapr_scaling = std::stod(std::string(arg.substr(16))); }
            catch (const std::exception&) { LogError("Invalid reapr-scaling value"); return std::nullopt; }
        }
        else if (arg.starts_with("--randomize-bends="))
        {
            try { config.randomize_bends = std::stoi(std::string(arg.substr(18))); }
            catch (const std::exception&) { LogError("Invalid randomize-bends value"); return std::nullopt; }
        }
        else if (arg == "--randomize-virtual-edges")    { config.randomize_virtual_edges = true; }
        else if (arg == "--no-randomize-virtual-edges") { config.randomize_virtual_edges = false; }
        else if (arg.starts_with("--compaction-method="))
        {
            std::string v = ToLower(arg.substr(20));
            if      (v == "unknown")               config.compaction_method = PDC_T::Compaction_T::Unknown;
            else if (v == "topological-numbering") config.compaction_method = PDC_T::Compaction_T::TopologicalNumbering;
            else if (v == "topological-ordering")  config.compaction_method = PDC_T::Compaction_T::TopologicalOrdering;
            else if (v == "length-mcf")            config.compaction_method = PDC_T::Compaction_T::Length_MCF;
            else if (v == "length-clp")            config.compaction_method = PDC_T::Compaction_T::Length_CLP;
            else if (v == "area-length-clp")       config.compaction_method = PDC_T::Compaction_T::AreaAndLength_CLP;
            else
            {
                LogError("Unknown compaction-method: '" + std::string(arg.substr(20)) + "'");
                LogError("Valid options: unknown, topological-numbering, topological-ordering, "
                         "length-mcf, length-clp, area-length-clp");
                return std::nullopt;
            }
        }
        else if (arg == "--canonicalize")    { config.canonicalize = true; }
        else if (arg == "--no-canonicalize") { config.canonicalize = false; }
        // Output shape: prime-factored (default) vs. connect-summed by color
        else if (arg == "--split") { config.unite = false; }
        else if (arg == "--unite") { config.unite = true; }
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
        else if (arg == "--debug-print-simplify-args")
        {
            config.debug_print_args = true;
        }
        else if (arg == "--quiet" || arg == "-q")
        {
            config.quiet = true;
        }
        // Output format
        else if (arg.starts_with("--format="))
        {
            std::string val(arg.substr(9));
            if (val != "pdc")
            {
                std::cerr << "Error: Unknown --format value: " << val << "\n";
                std::cerr << "  Valid: pdc (PlanarDiagramComplex's own native serialization,\n";
                std::cerr << "         colors round-trip exactly, including for unknot summands)\n";
                return std::nullopt;
            }
            config.pdc_format = true;
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
 * @brief Build a PDC_T::Simplify_Args_T from --simplify-level's coarse preset,
 *        with any explicit Simplify_Args_T-field flags applied on top. splitQ
 *        is always true -- it is an internal simplification-quality knob
 *        (splitQ=false makes Reapr skip multi-component diagrams outright),
 *        not the output-shape choice --unite/--split controls.
 */
PDC_T::Simplify_Args_T BuildSimplifyArgs(const Config& config)
{
    PDC_T::Simplify_Args_T args;
    args.splitQ = true;

    // Coarse preset from --simplify-level. Two regimes:
    //
    //   Levels 1-3 are LOCAL-ONLY diagnostic tiers (no rerouting), driven by
    //   local_opt_level -- the gate for the ArcSimplifier patterns
    //   (ArcSimplifier<...,level>): 1 = Reidemeister I only, 2 = + Reidemeister
    //   II, 4 = all local patterns (incl. assisted R_Ia/R_IIa). Useful for
    //   benchmarking other simplifiers against a pure R1 / R1+R2 / full-local
    //   pass.
    //
    //   Levels 4-6 are the tuned PRODUCTION pipeline (reroute/disconnect/Reapr)
    //   and deliberately keep local_opt_level = 0: Henrik's performance testing
    //   found the local pass does not help (slightly hurts) once rerouting is
    //   engaged on large diagrams, so the default path leaves it off. Local and
    //   reroute are orthogonal, so the 3->4 boundary is intentionally not a
    //   strict superset.
    args.local_opt_level = static_cast<Knoodle::UInt8>(
        config.simplify_level == 1 ? 1 :
        config.simplify_level == 2 ? 2 :
        config.simplify_level == 3 ? 4 : 0);
    args.rerouteQ    = (config.simplify_level >= 4);
    args.disconnectQ = (config.simplify_level >= 5);

    if (config.simplify_level >= kReaprThreshold)
    {
        args.embedding_trials = static_cast<Knoodle::Size_T>(config.max_reapr_attempts);
        args.rotation_trials = 25;
    }

    if (config.reapr_energy.has_value())     args.energy = *config.reapr_energy;
    if (config.no_compaction)                args.compaction_method = PDC_T::Compaction_T::Unknown;

    // Explicit overrides, applied last so they win over the level preset.
    if (config.compress_initial.has_value())     args.compress_initialQ = *config.compress_initial;
    if (config.local_opt_level.has_value())       args.local_opt_level = static_cast<Knoodle::UInt8>(*config.local_opt_level);
    if (config.dijkstra_strategy.has_value())      args.strategy = *config.dijkstra_strategy;
    if (config.start_max_dist.has_value())         args.start_max_dist = *config.start_max_dist;
    if (config.final_max_dist.has_value())         args.final_max_dist = *config.final_max_dist;
    if (config.reroute.has_value())                args.rerouteQ = *config.reroute;
    if (config.disconnect.has_value())             args.disconnectQ = *config.disconnect;
    if (config.compress.has_value())               args.compressQ = *config.compress;
    if (config.compression_threshold.has_value())  args.compression_threshold = *config.compression_threshold;
    if (config.rotation_trials.has_value())        args.rotation_trials = *config.rotation_trials;
    if (config.reapr_permute_random.has_value())   args.permute_randomQ = *config.reapr_permute_random;
    if (config.reapr_scaling.has_value())          args.scaling = *config.reapr_scaling;
    if (config.randomize_bends.has_value())        args.randomize_bends = *config.randomize_bends;
    if (config.randomize_virtual_edges.has_value())args.randomize_virtual_edgesQ = *config.randomize_virtual_edges;
    if (config.compaction_method.has_value())      args.compaction_method = *config.compaction_method;
    if (config.canonicalize.has_value())           args.canonicalizeQ = *config.canonicalize;

    return args;
}

/**
 * @brief Write a Simplify_Args_T to `out`, one field per line.
 *
 * Used by --debug-print-simplify-args, which prints the struct IMMEDIATELY
 * BEFORE it is passed to Simplify -- from the same object, so there is one code
 * path and no second construction that could drift from the real one.
 *
 * test/simplify_config_check covers argv -> Simplify_Args_T by calling
 * ParseArguments and BuildSimplifyArgs directly, which is a stronger check than
 * reading this output. What this adds is the last link those pure functions
 * cannot see: that `main` really hands Simplify the struct BuildSimplifyArgs
 * returned, unmodified. The variable is already const, so this is
 * belt-and-braces -- and a useful thing to be able to ask the tool.
 */
[[maybe_unused]] void PrintSimplifyArgs(const PDC_T::Simplify_Args_T& a, std::ostream& out)
{
    out << "simplify-args:"
        << "\n  local_opt_level          = " << int(a.local_opt_level)
        << "\n  rerouteQ                 = " << (a.rerouteQ    ? "true" : "false")
        << "\n  disconnectQ              = " << (a.disconnectQ ? "true" : "false")
        << "\n  compressQ                = " << (a.compressQ   ? "true" : "false")
        << "\n  compress_initialQ        = " << (a.compress_initialQ ? "true" : "false")
        << "\n  splitQ                   = " << (a.splitQ      ? "true" : "false")
        << "\n  canonicalizeQ            = " << (a.canonicalizeQ ? "true" : "false")
        << "\n  compression_threshold    = " << a.compression_threshold
        << "\n  start_max_dist           = " << a.start_max_dist
        << "\n  final_max_dist           = " << a.final_max_dist
        << "\n  embedding_trials         = " << a.embedding_trials
        << "\n  rotation_trials          = " << a.rotation_trials
        << "\n  randomize_bends          = " << a.randomize_bends
        << "\n  permute_randomQ          = " << (a.permute_randomQ ? "true" : "false")
        << "\n  randomize_virtual_edgesQ = " << (a.randomize_virtual_edgesQ ? "true" : "false")
        << "\n  scaling                  = " << a.scaling
        << "\n  energy                   = " << EnergyName(a.energy)
        << "\n";
}

} // anonymous namespace
