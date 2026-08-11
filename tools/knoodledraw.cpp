/**
 * @file knoodledraw.cpp
 * @brief knoodledraw - A command-line tool for drawing knot and link diagrams as ASCII art.
 *
 * This tool reads knot/link diagrams (as PD codes, 3D geometry, or .kndlxyz files)
 * and produces ASCII art drawings using OrthoDraw.
 *
 * Designed as a Unix filter: reads from stdin by default, writes to stdout.
 * Pipe from knoodlesimplify for simplified diagrams:
 *
 *   knoodlesimplify --streaming-mode < input.tsv | knoodledraw
 *   knoodledraw diagram.tsv
 *   knoodledraw --x-grid-size=12 diagram.tsv
 *
 * See --help for full option list.
 */

// Jason, you can let the compiler have the following flags defined
//#define KNOODLE_USE_UMFPACK  // Support for the "Dirichlet" and "Bending" energies in Reapr.
//#define KNOODLE_USE_CLP      // Support some LP problems in Reapr and OrthoDraw; inferior to MCF.
//#define KNOODLE_USE_BOOST_UNORDERED // Support for faster associative containers in Reapr.

#include "knoodle_io.hpp"

#include <charconv>     // ParsePassMove
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#include <string_view>  // ParsePassMove

//==============================================================================
// Configuration
//==============================================================================

namespace {

/**
 * @brief Configuration parsed from command-line arguments.
 */
struct Config
{
    Int  x_grid_size    = -1;  // -1 = auto (depends on label count)
    Int  y_grid_size    = -1;  // -1 = auto (depends on label count)
    Int  x_gap_size     = 1;
    Int  y_gap_size     = 1;
    std::vector<std::string> input_files;
    std::vector<std::string> highlight_specs;  // raw "--highlight=..." values
    bool randomize_projection = false;
    bool ascii_mode           = false;
    bool mono_mode            = false;  // --mono : no ANSI; weight carries meaning
    bool wolfram_mode         = false;  // --format=wl : emit WL geometry association
    bool help_requested       = false;
    bool label_crossings      = false;
    bool label_arcs           = false;
    bool label_faces          = false;
    bool label_components     = false;

    // Quality preset
    std::optional<std::string> quality_preset;  // "fast", "default", "best", "best-clp"

    // Algorithm selection
    std::optional<OrthoDraw_T::BendMethod_T>      bend_method;
    std::optional<OrthoDraw_T::CompactionMethod_T> compaction_method;

    // Integer options
    std::optional<Int> x_rounding_radius;
    std::optional<Int> y_rounding_radius;
    std::optional<int> randomize_bends;
    std::optional<Int> exterior_face;  // which face OrthoDraw lays out as unbounded (nullopt = auto)

    // Boolean options (nullopt = use default/preset)
    std::optional<bool> redistribute_bends;
    std::optional<bool> turn_regularize;
    std::optional<bool> saturate_regions;
    std::optional<bool> saturate_exterior;
    std::optional<bool> filter_saturating_edges;
    std::optional<bool> randomize_virtual_edges;
    std::optional<bool> dual_simplex;

    bool checkerboard_coloring = false;

    // Pass-move overlay (docs/move-descriptor.md): descriptor text from
    // --move=..., e.g. "kind=pass strand=11 depart=7 cross=7:u land=1".
    std::optional<std::string> move_spec;

    // Trace mode (--trace): input is a move-trace stream of
    // #step/#move/#view-headed PD records per docs/move-descriptor.md.
    bool trace_mode = false;
};

// Strand = the arcs of a --move descriptor's rerouted strand W. It is a
// highlight rather than an overlay: those arcs already exist in the diagram,
// we only need to tell them apart from the corridor that replaces them.
enum class HighlightType : uint8_t {
    None = 0, Arc = 1, Crossing = 2, Face = 3, Pass = 4, Strand = 5
};

struct HighlightSet {
    std::set<Int> arcs;
    std::set<Int> crossings;
    std::set<Int> faces;
    bool empty() const { return arcs.empty() && crossings.empty() && faces.empty(); }
};

//==============================================================================
// Command-Line Parsing
//==============================================================================

/**
 * @brief Print usage information to stderr.
 */
void PrintUsage()
{
    // Help goes to stderr so stdout stays clean for piping
    std::cerr << "Usage: knoodledraw [options] [input_files...]\n";
    std::cerr << "\n";
    std::cerr << "Reads knot/link diagrams and produces ASCII art drawings.\n";
    std::cerr << "If no input files are given, reads from stdin (Unix filter mode).\n";
    std::cerr << "\n";
    std::cerr << "Quality presets:\n";
    std::cerr << "  --quality=PRESET            fast, default, best, debug\n";
    std::cerr << "\n";
    std::cerr << "Grid and spacing:\n";
    std::cerr << "  --x-grid-size=N             Horizontal grid size (default: 4/6/8 for 0/1/2+ labels)\n";
    std::cerr << "  --y-grid-size=N             Vertical grid size (default: 2/3/4 for 0/1/2+ labels)\n";
    std::cerr << "  --x-gap-size=N              Horizontal gap size (default: 1)\n";
    std::cerr << "  --y-gap-size=N              Vertical gap size (default: 1)\n";
    std::cerr << "  --x-rounding-radius=N       Horizontal rounding radius (default: 4)\n";
    std::cerr << "  --y-rounding-radius=N       Vertical rounding radius (default: 4)\n";
    std::cerr << "\n";
    std::cerr << "Label options (combinable, e.g. -calf):\n";
    std::cerr << "  -c, --label-crossings       Show crossing numbers (0-based)\n";
    std::cerr << "  -a, --label-arcs            Show arc numbers (0-based)\n";
    std::cerr << "  -f, --label-faces           Show face numbers (0-based)\n";
    std::cerr << "  -l, --label-components      Color/label link components\n";
    std::cerr << "\n";
    std::cerr << "Output options:\n";
    std::cerr << "  --ascii                     Use plain ASCII output (default: Unicode box-drawing)\n";
    std::cerr << "  --mono                      No ANSI color: the rerouted strand W and the new\n";
    std::cerr << "                              corridor are drawn in heavy strokes and labelled\n";
    std::cerr << "                              'w' / 'p' instead (for files, docs, and anything\n";
    std::cerr << "                              that strips escape codes)\n";
    std::cerr << "  --highlight=ELEMENTS        Highlight elements (a=arc, c=crossing, f=face)\n";
    std::cerr << "                              e.g. --highlight=\"a0,c3,f2\"; multiple flags allowed\n";
    std::cerr << "  --checkerboard-coloring     Highlight alternating faces (checkerboard pattern)\n";
    std::cerr << "  --move=DESCRIPTOR           Overlay a pass move (docs/move-descriptor.md):\n";
    std::cerr << "                              \"strand=DA[,DA..] depart=DA [cross=DA:u|o,..] land=DA\"\n";
    std::cerr << "                              darc DA = 2*arc+d (Tail=0/Head=1); corridor drawn in\n";
    std::cerr << "                              heavy gold strokes, anchors in red; rejected\n";
    std::cerr << "                              descriptors report the failed check and exit nonzero\n";
    std::cerr << "  --trace                     Input is a move-trace stream (#step/#move/#view headed\n";
    std::cerr << "                              PD records, docs/move-descriptor.md): each record is\n";
    std::cerr << "                              drawn under its echoed headers, pass moves as overlays\n";
    std::cerr << "\n";
    std::cerr << "Algorithm options:\n";
    std::cerr << "  --bend-method=METHOD        mcf (default), clp\n";
    std::cerr << "  --compaction=METHOD         topo-number, topo-order, length-mcf (default),\n";
    std::cerr << "                              length-clp, area-clp\n";
    std::cerr << "  --randomize-bends=N         [experimental] Randomization rounds (default: 0)\n";
    std::cerr << "  --exterior-face=N           Face to lay out as the unbounded exterior region\n";
    std::cerr << "                              (0-based; default: auto, the largest face)\n";
    std::cerr << "\n";
    std::cerr << "Boolean tuning (--flag / --no-flag):\n";
    std::cerr << "  redistribute-bends (on)     Redistribute bends after optimization\n";
    std::cerr << "  turn-regularize (on)        Regularize turns\n";
    std::cerr << "  saturate-regions (on)       Saturate regions\n";
    std::cerr << "  saturate-exterior (on)      Saturate exterior region\n";
    std::cerr << "  filter-saturating-edges (on) Filter saturating edges\n";
    std::cerr << "  randomize-virtual-edges (off) Randomize virtual edges\n";
    std::cerr << "  dual-simplex (off)          Use dual simplex method\n";
    std::cerr << "\n";
    std::cerr << "Input formats:\n";
    std::cerr << "  4 columns: unsigned PD code (4 arc labels per crossing)\n";
    std::cerr << "  5 columns: signed PD code (4 arcs + handedness)\n";
    std::cerr << "  6 columns: unsigned PD code + link component colors\n";
    std::cerr << "  7 columns: signed PD code + link component colors\n";
    std::cerr << "  3 columns: 3D geometry (x, y, z coordinates)\n";
    std::cerr << "  .kndlxyz:  multi-component 3D link embedding\n";
    std::cerr << "\n";
    std::cerr << "Input options:\n";
    std::cerr << "  --randomize-projection      Apply random shear to 3D geometry projection\n";
    std::cerr << "\n";
    std::cerr << "Other:\n";
    std::cerr << "  -h, --help                  Show this help message\n";
    std::cerr << "\n";
    std::cerr << "Examples:\n";
    std::cerr << "  knoodlesimplify --streaming-mode < input.tsv | knoodledraw\n";
    std::cerr << "  knoodledraw diagram.tsv\n";
    std::cerr << "  knoodledraw --quality=fast --ascii diagram.tsv\n";
    std::cerr << "  knoodledraw --quality=debug --ascii diagram.tsv\n";
}

/**
 * @brief Parse a positive integer from a string.
 */
std::optional<Int> ParsePositiveInt(std::string_view s)
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
 * @brief Parse a non-negative integer from a string (accepts 0).
 */
std::optional<int> ParseNonNegativeInt(std::string_view s)
{
    try
    {
        std::size_t pos = 0;
        long long val = std::stoll(std::string(s), &pos);
        if (pos != s.size() || val < 0) return std::nullopt;
        return static_cast<int>(val);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

/**
 * @brief Match a boolean flag pair: --name / --no-name.
 * @return +1 for --name, -1 for --no-name, 0 for no match.
 */
int MatchBoolFlag(std::string_view arg, std::string_view name)
{
    // Check --name
    if (arg.size() == 2 + name.size()
        && arg.starts_with("--")
        && arg.substr(2) == name)
    {
        return +1;
    }
    // Check --no-name
    if (arg.size() == 5 + name.size()
        && arg.starts_with("--no-")
        && arg.substr(5) == name)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief Parse command-line arguments into a Config struct.
 */
std::optional<Config> ParseArguments(int argc, char* argv[])
{
    Config config;

    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg(argv[i]);

        // Help
        if (arg == "-h" || arg == "--help")
        {
            config.help_requested = true;
            return config;
        }

        // Grid size options
        if (arg.starts_with("--x-grid-size="))
        {
            auto val = ParsePositiveInt(arg.substr(14));
            if (!val) { std::cerr << "Error: Invalid x-grid-size value\n"; return std::nullopt; }
            config.x_grid_size = *val;
        }
        else if (arg.starts_with("--y-grid-size="))
        {
            auto val = ParsePositiveInt(arg.substr(14));
            if (!val) { std::cerr << "Error: Invalid y-grid-size value\n"; return std::nullopt; }
            config.y_grid_size = *val;
        }
        else if (arg.starts_with("--x-gap-size="))
        {
            auto val = ParsePositiveInt(arg.substr(13));
            if (!val) { std::cerr << "Error: Invalid x-gap-size value\n"; return std::nullopt; }
            config.x_gap_size = *val;
        }
        else if (arg.starts_with("--y-gap-size="))
        {
            auto val = ParsePositiveInt(arg.substr(13));
            if (!val) { std::cerr << "Error: Invalid y-gap-size value\n"; return std::nullopt; }
            config.y_gap_size = *val;
        }
        // ASCII mode (disable Unicode box-drawing)
        else if (arg == "--ascii")
        {
            config.ascii_mode = true;
        }
        else if (arg == "--mono")
        {
            config.mono_mode = true;
        }
        else if (arg == "--trace")
        {
            config.trace_mode = true;
        }
        // Output format: unicode (default), ascii, or wl (Wolfram geometry association)
        else if (arg.starts_with("--format="))
        {
            std::string val(arg.substr(9));
            if (val == "wl" || val == "wolfram") { config.wolfram_mode = true; }
            else if (val == "ascii")             { config.ascii_mode = true; }
            else if (val == "unicode")           { /* default (Unicode box-drawing) */ }
            else
            {
                std::cerr << "Error: Unknown --format value: " << val << "\n";
                std::cerr << "  Valid: unicode, ascii, wl\n";
                return std::nullopt;
            }
        }
        // Randomize projection
        else if (arg == "--randomize-projection")
        {
            config.randomize_projection = true;
        }
        // Long-form label options
        else if (arg == "--label-crossings")
        {
            config.label_crossings = true;
        }
        else if (arg == "--label-arcs")
        {
            config.label_arcs = true;
        }
        else if (arg == "--label-faces")
        {
            config.label_faces = true;
        }
        else if (arg == "--label-components")
        {
            config.label_components = true;
        }
        // Quality preset
        else if (arg.starts_with("--quality="))
        {
            std::string val(arg.substr(10));
            if (val != "fast" && val != "default" && val != "best" && val != "debug")
            {
                std::cerr << "Error: Unknown quality preset: " << val << "\n";
                std::cerr << "  Valid presets: fast, default, best, debug\n";
                return std::nullopt;
            }
            config.quality_preset = val;
        }
        // Bend method
        else if (arg.starts_with("--bend-method="))
        {
            std::string val(arg.substr(14));
            if (val == "mcf")
                config.bend_method = OrthoDraw_T::BendMethod_T::Bends_MCF;
            else if (val == "clp")
                config.bend_method = OrthoDraw_T::BendMethod_T::Bends_CLP;
            else
            {
                std::cerr << "Error: Unknown bend method: " << val << "\n";
                std::cerr << "  Valid methods: mcf, clp\n";
                return std::nullopt;
            }
        }
        // Compaction method
        else if (arg.starts_with("--compaction="))
        {
            std::string val(arg.substr(13));
            if (val == "topo-number")
                config.compaction_method = OrthoDraw_T::CompactionMethod_T::TopologicalNumbering;
            else if (val == "topo-order")
                config.compaction_method = OrthoDraw_T::CompactionMethod_T::TopologicalOrdering;
            else if (val == "length-mcf")
                config.compaction_method = OrthoDraw_T::CompactionMethod_T::Length_MCF;
            else if (val == "length-clp")
                config.compaction_method = OrthoDraw_T::CompactionMethod_T::Length_CLP;
            else if (val == "area-clp")
                config.compaction_method = OrthoDraw_T::CompactionMethod_T::AreaAndLength_CLP;
            else
            {
                std::cerr << "Error: Unknown compaction method: " << val << "\n";
                std::cerr << "  Valid methods: topo-number, topo-order, length-mcf, length-clp, area-clp\n";
                return std::nullopt;
            }
        }
        // Randomize bends (non-negative integer)
        else if (arg.starts_with("--randomize-bends="))
        {
            auto val = ParseNonNegativeInt(arg.substr(18));
            if (!val) { std::cerr << "Error: Invalid randomize-bends value\n"; return std::nullopt; }
            config.randomize_bends = *val;
        }
        // Rounding radii
        else if (arg.starts_with("--x-rounding-radius="))
        {
            auto val = ParsePositiveInt(arg.substr(20));
            if (!val) { std::cerr << "Error: Invalid x-rounding-radius value\n"; return std::nullopt; }
            config.x_rounding_radius = *val;
        }
        else if (arg.starts_with("--y-rounding-radius="))
        {
            auto val = ParsePositiveInt(arg.substr(20));
            if (!val) { std::cerr << "Error: Invalid y-rounding-radius value\n"; return std::nullopt; }
            config.y_rounding_radius = *val;
        }
        // Exterior face
        else if (arg.starts_with("--exterior-face="))
        {
            auto val = ParseNonNegativeInt(arg.substr(16));
            if (!val) { std::cerr << "Error: Invalid exterior-face value\n"; return std::nullopt; }
            config.exterior_face = static_cast<Int>(*val);
        }
        // Checkerboard coloring
        else if (arg == "--checkerboard-coloring" || arg == "--checkerboardcoloring")
        {
            config.checkerboard_coloring = true;
        }
        // Highlight specification
        else if (arg.starts_with("--highlight="))
        {
            config.highlight_specs.emplace_back(arg.substr(12));
        }
        // Pass-move overlay descriptor
        else if (arg.starts_with("--move="))
        {
            config.move_spec = arg.substr(7);
        }
        // Short combinable flags: -c, -a, -f, -caf, -ac, etc.
        else if (arg.starts_with("-") && !arg.starts_with("--") && arg.size() > 1)
        {
            bool valid = true;
            for (std::size_t j = 1; j < arg.size(); ++j)
            {
                switch (arg[j])
                {
                    case 'c': config.label_crossings  = true; break;
                    case 'a': config.label_arcs       = true; break;
                    case 'f': config.label_faces      = true; break;
                    case 'l': config.label_components = true; break;
                    default:
                        valid = false;
                        break;
                }
                if (!valid) break;
            }
            if (!valid)
            {
                std::cerr << "Error: Unknown option: " << arg << "\n";
                PrintUsage();
                return std::nullopt;
            }
        }
        // Boolean flag pairs (--flag / --no-flag)
        else if (arg.starts_with("--"))
        {
            int m;
            bool matched = false;

            if ((m = MatchBoolFlag(arg, "redistribute-bends")) != 0)
            { config.redistribute_bends = (m > 0); matched = true; }
            else if ((m = MatchBoolFlag(arg, "turn-regularize")) != 0)
            { config.turn_regularize = (m > 0); matched = true; }
            else if ((m = MatchBoolFlag(arg, "saturate-regions")) != 0)
            { config.saturate_regions = (m > 0); matched = true; }
            else if ((m = MatchBoolFlag(arg, "saturate-exterior")) != 0)
            { config.saturate_exterior = (m > 0); matched = true; }
            else if ((m = MatchBoolFlag(arg, "filter-saturating-edges")) != 0)
            { config.filter_saturating_edges = (m > 0); matched = true; }
            else if ((m = MatchBoolFlag(arg, "randomize-virtual-edges")) != 0)
            { config.randomize_virtual_edges = (m > 0); matched = true; }
            else if ((m = MatchBoolFlag(arg, "dual-simplex")) != 0)
            { config.dual_simplex = (m > 0); matched = true; }

            if (!matched)
            {
                std::cerr << "Error: Unknown option: " << arg << "\n";
                PrintUsage();
                return std::nullopt;
            }
        }
        // Unknown short option
        else if (arg.starts_with("-"))
        {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            PrintUsage();
            return std::nullopt;
        }
        // Positional argument = input file
        else
        {
            config.input_files.emplace_back(arg);
        }
    }

    // Debug preset enables all labels (before auto-sizing so grid adapts)
    if (config.quality_preset && *config.quality_preset == "debug")
    {
        config.label_crossings  = true;
        config.label_arcs       = true;
        config.label_faces      = true;
        config.label_components = true;
    }

    // Apply default grid sizes based on label count (if not explicitly set)
    int label_count = int(config.label_crossings)
                    + int(config.label_arcs)
                    + int(config.label_faces)
                    + int(config.label_components);
    if (config.x_grid_size < 0)
        config.x_grid_size = (label_count >= 2) ? 8 : (label_count == 1) ? 6 : 4;
    if (config.y_grid_size < 0)
        config.y_grid_size = (label_count >= 2) ? 4 : (label_count == 1) ? 3 : 2;

    return config;
}

//==============================================================================
// Unicode Post-Processing
//==============================================================================

/**
 * @brief Test whether a character is a horizontal connector (connects left/right).
 */
bool ConnectsHorizontally(char c)
{
    return c == '-' || c == '+' || c == '<' || c == '>';
}

/**
 * @brief Test whether a character is a vertical connector (connects up/down).
 */
bool ConnectsVertically(char c)
{
    return c == '|' || c == '+' || c == '^' || c == 'v';
}

/**
 * @brief Convert an ASCII knot diagram to Unicode box-drawing characters.
 *
 * Replaces -, |, +, <, >, ^, v, and . with their Unicode equivalents.
 * For + characters, examines neighbors to choose the correct corner/junction glyph.
 *
 * When a highlight mask is provided, highlighted characters are wrapped with
 * ANSI 256-color escape sequences (Modus Vivendi palette).
 */
std::string UnicodeifyDiagram(const std::string& ascii,
                               const std::vector<HighlightType>* hl_mask = nullptr,
                               Int n_x = 0,
                               const std::vector<Int>* comp_map = nullptr,
                               bool mono = false)
{
    // Parse into lines
    std::vector<std::string> lines;
    {
        std::istringstream iss(ascii);
        std::string line;
        while (std::getline(iss, line))
        {
            lines.push_back(line);
        }
    }

    // Find max width for bounds checking
    std::size_t max_width = 0;
    for (const auto& line : lines)
    {
        max_width = std::max(max_width, line.size());
    }

    auto cell = [&](std::size_t row, std::size_t col) -> char
    {
        if (row >= lines.size() || col >= lines[row].size()) return ' ';
        return lines[row][col];
    };

    // ANSI 256-color escapes (Modus Vivendi palette)
    static const char* COLOR_ARC      = "\033[38;5;111m";  // blue-warmer
    static const char* COLOR_CROSSING = "\033[38;5;203m";  // red
    static const char* COLOR_FACE     = "\033[48;5;234m";  // bg-dim
    static const char* COLOR_PASS     = "\033[38;5;220m";  // gold (new corridor)
    static const char* COLOR_STRAND   = "\033[38;5;80m";   // cyan (strand W)
    static const char* COLOR_RESET    = "\033[0m";

    // Component palette (Modus Vivendi distinct colors)
    static constexpr int COMPONENT_COLORS[] = {
        204,  // red-warmer (#ff5f87)
        111,  // blue-warmer (#87afff)
         78,  // green (#5fd787)
        176,  // magenta (#d787d7)
        180,  // gold (#d7af87)
         80,  // cyan (#5fd7d7)
        215,  // orange (#ffaf5f)
        141,  // purple (#af87ff)
    };
    static constexpr int NUM_COMPONENT_COLORS = 8;

    std::string result;
    result.reserve(ascii.size() * 4);

    for (std::size_t r = 0; r < lines.size(); ++r)
    {
        for (std::size_t c = 0; c < lines[r].size(); ++c)
        {
            char ch = lines[r][c];
            std::string char_out;

            // Which of the two rerouting actors, if either, owns this cell.
            // In --mono there is no colour to say so, and the difference is
            // carried by stroke weight instead: W and the corridor are drawn
            // heavy, the rest of the diagram light. In colour mode everything
            // is drawn with the diagram's own light strokes and the colour
            // does the talking.
            HighlightType ht_here = HighlightType::None;
            if ((n_x > 0) && hl_mask)
            {
                auto m = r * static_cast<std::size_t>(n_x) + c;
                if (m < hl_mask->size()) { ht_here = (*hl_mask)[m]; }
            }
            const bool heavyQ = mono
                && ((ht_here == HighlightType::Strand)
                 || (ht_here == HighlightType::Pass));

            switch (ch)
            {
                case '-': char_out = heavyQ ? "\xe2\x94\x81"  // ━ U+2501
                                            : "\xe2\x94\x80"; // ─ U+2500
                          break;
                case '|': char_out = heavyQ ? "\xe2\x94\x83"  // ┃ U+2503
                                            : "\xe2\x94\x82"; // │ U+2502
                          break;
                case '<': char_out = "\xe2\x86\x90"; break; // ← U+2190
                case '>': char_out = "\xe2\x86\x92"; break; // → U+2192
                case '^': char_out = "\xe2\x86\x91"; break; // ↑ U+2191
                case 'v': char_out = "\xe2\x86\x93"; break; // ↓ U+2193
                case '.': char_out = "\xc2\xb7";     break; // · U+00B7

                // Pass-move corridor strokes, stamped by StampPassOverlay and
                // never emitted by DiagramString itself. They take the same
                // light glyphs as the diagram (colour tells them apart) unless
                // --mono asked for weight instead.
                case '=': char_out = mono ? "\xe2\x94\x81"  // ━
                                          : "\xe2\x94\x80"; // ─
                          break;
                case ';': char_out = mono ? "\xe2\x94\x83"  // ┃
                                          : "\xe2\x94\x82"; // │
                          break;
                case '{': char_out = mono ? "\xe2\x94\x97"  // ┗ (N+E)
                                          : "\xe2\x95\xb0"; // ╰
                          break;
                case '}': char_out = mono ? "\xe2\x94\x9b"  // ┛ (N+W)
                                          : "\xe2\x95\xaf"; // ╯
                          break;
                case '[': char_out = mono ? "\xe2\x94\x8f"  // ┏ (S+E)
                                          : "\xe2\x95\xad"; // ╭
                          break;
                case ']': char_out = mono ? "\xe2\x94\x93"  // ┓ (S+W)
                                          : "\xe2\x95\xae"; // ╮
                          break;
                case '*': char_out = "\xe2\x97\x8f"; break; // ● U+25CF (junction)

                case '+':
                {
                    bool left  = (c > 0) && ConnectsHorizontally(cell(r, c - 1));
                    bool right = (c + 1 < lines[r].size()) && ConnectsHorizontally(cell(r, c + 1));
                    bool up    = (r > 0) && ConnectsVertically(cell(r - 1, c));
                    bool down  = (r + 1 < lines.size()) && ConnectsVertically(cell(r + 1, c));

                    int bits = (left ? 8 : 0) | (right ? 4 : 0) | (up ? 2 : 0) | (down ? 1 : 0);

                    if (heavyQ)
                    {
                        switch (bits)
                        {
                            case 0b0101: char_out = "\xe2\x94\x8f"; break; // ┏ right+down
                            case 0b1001: char_out = "\xe2\x94\x93"; break; // ┓ left+down
                            case 0b0110: char_out = "\xe2\x94\x97"; break; // ┗ right+up
                            case 0b1010: char_out = "\xe2\x94\x9b"; break; // ┛ left+up
                            case 0b1101: char_out = "\xe2\x94\xb3"; break; // ┳ left+right+down
                            case 0b1110: char_out = "\xe2\x94\xbb"; break; // ┻ left+right+up
                            case 0b0111: char_out = "\xe2\x94\xa3"; break; // ┣ up+down+right
                            case 0b1011: char_out = "\xe2\x94\xab"; break; // ┫ up+down+left
                            case 0b1111: char_out = "\xe2\x95\x8b"; break; // ╋ all four
                            case 0b1100: char_out = "\xe2\x94\x81"; break; // ━ left+right
                            case 0b0011: char_out = "\xe2\x94\x83"; break; // ┃ up+down
                            default:     char_out = "+";            break; // fallback
                        }
                    }
                    else
                    {
                        switch (bits)
                        {
                            case 0b0101: char_out = "\xe2\x95\xad"; break; // ╭ right+down
                            case 0b1001: char_out = "\xe2\x95\xae"; break; // ╮ left+down
                            case 0b0110: char_out = "\xe2\x95\xb0"; break; // ╰ right+up
                            case 0b1010: char_out = "\xe2\x95\xaf"; break; // ╯ left+up
                            case 0b1101: char_out = "\xe2\x94\xac"; break; // ┬ left+right+down
                            case 0b1110: char_out = "\xe2\x94\xb4"; break; // ┴ left+right+up
                            case 0b0111: char_out = "\xe2\x94\x9c"; break; // ├ up+down+right
                            case 0b1011: char_out = "\xe2\x94\xa4"; break; // ┤ up+down+left
                            case 0b1111: char_out = "\xe2\x94\xbc"; break; // ┼ all four
                            case 0b1100: char_out = "\xe2\x94\x80"; break; // ─ left+right
                            case 0b0011: char_out = "\xe2\x94\x82"; break; // │ up+down
                            default:     char_out = "+";            break; // fallback
                        }
                    }
                    break;
                }

                default:
                    char_out = ch;
                    break;
            }

            // Apply coloring: highlight takes priority over component color.
            // --mono emits no escape codes at all: weight and the w/p markers
            // carry everything colour would have.
            if ((n_x > 0) && !mono)
            {
                auto mi = r * static_cast<std::size_t>(n_x) + c;

                // Priority 1: highlight color
                if (hl_mask && mi < hl_mask->size()
                    && (*hl_mask)[mi] != HighlightType::None)
                {
                    auto ht = (*hl_mask)[mi];

                    // Face highlight is a background color — compose it
                    // with the foreground component color if available,
                    // so arc labels in highlighted faces keep their color.
                    if (ht == HighlightType::Face
                        && comp_map && mi < comp_map->size()
                        && (*comp_map)[mi] >= 0)
                    {
                        int lc = static_cast<int>((*comp_map)[mi]);
                        int fg = COMPONENT_COLORS[lc % NUM_COMPONENT_COLORS];
                        result += COLOR_FACE;
                        result += "\033[38;5;";
                        result += std::to_string(fg);
                        result += "m";
                        result += char_out;
                        result += COLOR_RESET;
                        continue;
                    }

                    const char* color = "";
                    switch (ht)
                    {
                        case HighlightType::Arc:      color = COLOR_ARC;      break;
                        case HighlightType::Crossing: color = COLOR_CROSSING; break;
                        case HighlightType::Face:     color = COLOR_FACE;     break;
                        case HighlightType::Pass:     color = COLOR_PASS;     break;
                        case HighlightType::Strand:   color = COLOR_STRAND;   break;
                        default: break;
                    }
                    result += color;
                    result += char_out;
                    result += COLOR_RESET;
                    continue;
                }

                // Priority 2: component color
                if (comp_map && mi < comp_map->size()
                    && (*comp_map)[mi] >= 0)
                {
                    int lc = static_cast<int>((*comp_map)[mi]);
                    int ci = COMPONENT_COLORS[lc % NUM_COMPONENT_COLORS];
                    result += "\033[38;5;";
                    result += std::to_string(ci);
                    result += "m";
                    result += char_out;
                    result += COLOR_RESET;
                    continue;
                }
            }

            result += char_out;
        }
        result += '\n';
    }

    // Remove trailing newline if original didn't end with one
    if (!ascii.empty() && ascii.back() != '\n' && !result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }

    return result;
}

//==============================================================================
// Label Infrastructure
//==============================================================================

/**
 * @brief Place crossing labels at diagonal positions relative to each crossing.
 *
 * At each crossing (cx, cy), the vertical strand passes through (cx, cy-1) and
 * (cx, cy+1). We detect which side has the incoming arrow (^ or v) and prefer
 * placing the label diagonally adjacent to the incoming side first.
 * The four diagonal positions (cx±1, cy±1) are always spaces in a valid
 * orthogonal drawing.
 */
void PlaceCrossingLabels(std::string& diagram, Int n_x, Int n_y,
                         OrthoDraw_T& H, const std::string& prefix,
                         std::vector<HighlightType>* mask = nullptr,
                         const HighlightSet* highlights = nullptr)
{
    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    auto char_at = [&](Int x, Int y) -> char {
        if (x < 0 || x >= n_x - 1 || y < 0 || y >= n_y) return '\0';
        return diagram[idx(x, y)];
    };

    auto is_label_char = [](char c) -> bool {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z');
    };

    auto fits = [&](Int x, Int y, Int len) -> bool {
        if (y < 0 || y >= n_y) return false;
        if (x < 0 || x + len >= n_x - 1) return false;
        for (Int i = 0; i < len; ++i)
        {
            if (diagram[idx(x + i, y)] != ' ') return false;
        }
        // 1-char margin so labels don't touch other labels
        // (diagram characters like |, ^, v, - are fine to be adjacent)
        if (x - 1 >= 0 && is_label_char(diagram[idx(x - 1, y)])) return false;
        if (x + len < n_x - 1 && is_label_char(diagram[idx(x + len, y)])) return false;
        return true;
    };

    const auto& V_coords = H.VertexCoordinates();

    for (Int c = 0; c < H.MaxCrossingCount(); ++c)
    {
        if (!H.VertexActiveQ(c)) continue;

        std::string label = prefix + std::to_string(c);
        Int label_len = static_cast<Int>(label.size());

        Int cx = V_coords(c, 0);
        Int cy = V_coords(c, 1);

        // Detect incoming arrow on the vertical strand
        Int incoming_y, outgoing_y;
        char above = char_at(cx, cy + 1); // cy+1 is visually above in math coords
        char below = char_at(cx, cy - 1);

        if (below == '^')
        {
            // Arrow at cy-1 points up toward crossing: incoming from below
            incoming_y = cy - 1;
            outgoing_y = cy + 1;
        }
        else if (above == 'v')
        {
            // Arrow at cy+1 points down toward crossing: incoming from above
            incoming_y = cy + 1;
            outgoing_y = cy - 1;
        }
        else
        {
            // Fallback: treat cy-1 as incoming
            incoming_y = cy - 1;
            outgoing_y = cy + 1;
        }

        // Try placements in priority order:
        // 1. Right of incoming arrow
        // 2. Left of incoming arrow
        // 3. Right of outgoing side
        // 4. Left of outgoing side
        struct { Int x, y; } positions[] = {
            { cx + 1,         incoming_y },
            { cx - label_len, incoming_y },
            { cx + 1,         outgoing_y },
            { cx - label_len, outgoing_y },
        };

        for (const auto& pos : positions)
        {
            if (fits(pos.x, pos.y, label_len))
            {
                for (Int i = 0; i < label_len; ++i)
                {
                    auto ci = idx(pos.x + i, pos.y);
                    diagram[ci] = label[static_cast<std::size_t>(i)];
                    if (mask && highlights
                        && highlights->crossings.count(c))
                    {
                        (*mask)[ci] = HighlightType::Crossing;
                    }
                }
                break;
            }
        }
    }
}

/**
 * @brief Place arc labels directly onto diagram characters.
 *
 * For arcs with horizontal segments: overwrites '-' characters with the label,
 * centered in the longest contiguous run of dashes (the "safe zone").
 * For entirely-vertical arcs: overwrites a '|' with 'a' and places digits beside it.
 */
void PlaceArcLabels(std::string& diagram, Int n_x, Int n_y,
                    OrthoDraw_T& H, const std::string& prefix,
                    std::vector<HighlightType>* mask = nullptr,
                    const HighlightSet* highlights = nullptr,
                    std::vector<Int>* comp_map = nullptr)
{
    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    const auto& A_lines = H.ArcLines();

    for (Int a = 0; a < H.MaxArcCount(); ++a)
    {
        if (!H.EdgeActiveQ(a)) continue;

        auto sublist = A_lines[a];
        Int point_count = static_cast<Int>(sublist.end() - sublist.begin());
        if (point_count < 2) continue;

        // Collect horizontal and vertical segments from the polyline
        struct Segment { Int x0, y0, x1, y1; };
        std::vector<Segment> h_segs, v_segs;

        {
            auto it = sublist.begin();
            auto prev = it;
            ++it;
            for (; it != sublist.end(); prev = it, ++it)
            {
                Int x0 = (*prev)[0], y0 = (*prev)[1];
                Int x1 = (*it)[0],   y1 = (*it)[1];
                if (y0 == y1 && x0 != x1)
                    h_segs.push_back({x0, y0, x1, y1});
                else if (x0 == x1 && y0 != y1)
                    v_segs.push_back({x0, y0, x1, y1});
            }
        }

        bool placed = false;

        // Case 1: Arc has horizontal segment(s) — always preferred
        if (!h_segs.empty())
        {
            std::string label = prefix + std::to_string(a);
            Int label_len = static_cast<Int>(label.size());

            // Find the longest contiguous run of '-' across all horizontal segments
            Int best_run_len = 0;
            Int best_run_start = -1;
            Int best_run_y = -1;

            for (const auto& seg : h_segs)
            {
                Int lo = std::min(seg.x0, seg.x1);
                Int hi = std::max(seg.x0, seg.x1);
                Int y  = seg.y0;

                Int run_start = -1;
                Int run_len   = 0;

                for (Int x = lo; x <= hi; ++x)
                {
                    if (x >= 0 && x < n_x - 1 && y >= 0 && y < n_y
                        && diagram[idx(x, y)] == '-')
                    {
                        if (run_len == 0) run_start = x;
                        ++run_len;
                    }
                    else
                    {
                        if (run_len > best_run_len)
                        {
                            best_run_len   = run_len;
                            best_run_start = run_start;
                            best_run_y     = y;
                        }
                        run_len = 0;
                    }
                }
                if (run_len > best_run_len)
                {
                    best_run_len   = run_len;
                    best_run_start = run_start;
                    best_run_y     = y;
                }
            }

            if (best_run_len >= label_len)
            {
                // Center the label within the safe zone
                Int offset  = (best_run_len - label_len) / 2;
                Int start_x = best_run_start + offset;
                // Read component color from the first dash cell
                Int h_arc_color = Int(-1);
                if (comp_map)
                {
                    auto first_ci = idx(start_x, best_run_y);
                    if (first_ci < comp_map->size())
                        h_arc_color = (*comp_map)[first_ci];
                }

                for (Int i = 0; i < label_len; ++i)
                {
                    auto ci = idx(start_x + i, best_run_y);
                    diagram[ci] = label[static_cast<std::size_t>(i)];
                    if (mask && highlights
                        && highlights->arcs.count(a))
                    {
                        (*mask)[ci] = HighlightType::Arc;
                    }
                    if (comp_map && h_arc_color >= 0)
                        (*comp_map)[ci] = h_arc_color;
                }
                placed = true;
            }
        }

        // Case 2: Entirely vertical arc (no horizontal segments)
        if (!placed && h_segs.empty() && !v_segs.empty())
        {
            // Find the vertical segment with the most '|' characters
            Int best_pipe_count = 0;
            Int best_center_x   = -1;
            Int best_center_y   = -1;

            for (const auto& seg : v_segs)
            {
                Int x  = seg.x0;
                Int lo = std::min(seg.y0, seg.y1);
                Int hi = std::max(seg.y0, seg.y1);

                std::vector<Int> pipes;
                for (Int y = lo; y <= hi; ++y)
                {
                    if (x >= 0 && x < n_x - 1 && y >= 0 && y < n_y
                        && diagram[idx(x, y)] == '|')
                    {
                        pipes.push_back(y);
                    }
                }

                if (static_cast<Int>(pipes.size()) > best_pipe_count)
                {
                    best_pipe_count = static_cast<Int>(pipes.size());
                    best_center_x   = x;

                    // Compute the segment's visual center.
                    // When the crossing at one endpoint has a '|' character
                    // (vertical overstrand), it gets counted as a pipe,
                    // skewing the distribution. Detect this and round up
                    // to compensate, keeping the label at the true midpoint.
                    bool pipe_at_lo = (lo >= 0 && lo < n_y
                                       && diagram[idx(x, lo)] == '|');
                    Int seg_center = pipe_at_lo
                        ? (lo + hi + 1) / 2
                        : (lo + hi) / 2;
                    best_center_y = pipes[0];
                    for (Int py : pipes)
                    {
                        if (std::abs(py - seg_center) < std::abs(best_center_y - seg_center))
                            best_center_y = py;
                    }
                }
            }

            if (best_pipe_count > 0)
            {
                // For vertical arcs, overwrite the central '|' with the
                // first character of the label and place the rest beside it.
                // When prefix is non-empty (e.g. "a"), the pipe becomes 'a'
                // and digits go to the side. When prefix is empty, the pipe
                // becomes the first digit and remaining digits go beside it.
                std::string full_label = prefix + std::to_string(a);
                char pipe_char = full_label[0];
                std::string digits = full_label.substr(1);
                Int digit_len = static_cast<Int>(digits.size());

                // Overwrite '|' with first label character
                auto a_ci = idx(best_center_x, best_center_y);
                Int arc_color = (comp_map && a_ci < comp_map->size()
                                 && (*comp_map)[a_ci] >= 0)
                                ? (*comp_map)[a_ci] : Int(-1);
                diagram[a_ci] = pipe_char;
                bool arc_hl = mask && highlights
                              && highlights->arcs.count(a);
                if (arc_hl)
                {
                    (*mask)[a_ci] = HighlightType::Arc;
                }

                // Try placing digits to the right in space cells
                bool right_fits = true;
                for (Int i = 0; i < digit_len; ++i)
                {
                    Int x = best_center_x + 1 + i;
                    if (x >= n_x - 1 || diagram[idx(x, best_center_y)] != ' ')
                    {
                        right_fits = false;
                        break;
                    }
                }

                if (right_fits)
                {
                    for (Int i = 0; i < digit_len; ++i)
                    {
                        auto ci = idx(best_center_x + 1 + i, best_center_y);
                        diagram[ci] = digits[static_cast<std::size_t>(i)];
                        if (arc_hl) (*mask)[ci] = HighlightType::Arc;
                        if (comp_map && arc_color >= 0) (*comp_map)[ci] = arc_color;
                    }
                    placed = true;
                }
                else
                {
                    // Try digits to the left
                    bool left_fits = true;
                    for (Int i = 0; i < digit_len; ++i)
                    {
                        Int x = best_center_x - digit_len + i;
                        if (x < 0 || diagram[idx(x, best_center_y)] != ' ')
                        {
                            left_fits = false;
                            break;
                        }
                    }

                    if (left_fits)
                    {
                        for (Int i = 0; i < digit_len; ++i)
                        {
                            auto ci = idx(best_center_x - digit_len + i, best_center_y);
                            diagram[ci] = digits[static_cast<std::size_t>(i)];
                            if (arc_hl) (*mask)[ci] = HighlightType::Arc;
                            if (comp_map && arc_color >= 0) (*comp_map)[ci] = arc_color;
                        }
                        placed = true;
                    }
                }
            }
        }

        (void)placed;
    }
}

/**
 * @brief Build a face ownership map for the diagram grid.
 *
 * For each cell in the grid, determines which face (if any) it belongs to.
 * Returns a vector indexed by the same flat layout as the diagram string.
 * Values: face index (>= 0), -1 (unassigned space), -2 (occupied by diagram).
 *
 * Algorithm: for each face, find a seed point by stepping perpendicular from
 * a boundary arc into the face interior (using OrthoDraw's directed arc
 * convention), then flood-fill to claim all connected space cells.
 */
std::vector<Int> BuildFaceMap(OrthoDraw_T& H, const std::string& diagram,
                              Int n_x, Int n_y)
{
    std::vector<Int> face_map(static_cast<std::size_t>(n_x * n_y), Int(-1));

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    // Mark occupied cells (non-space characters)
    for (Int y = 0; y < n_y; ++y)
    {
        for (Int x = 0; x < n_x - 1; ++x) // n_x-1 is the newline column
        {
            if (diagram[idx(x, y)] != ' ')
                face_map[idx(x, y)] = Int(-2);
        }
        // Mark the newline column as occupied
        face_map[idx(n_x - 1, y)] = Int(-2);
    }

    const auto& F_dA = H.FaceDarcs();
    auto& A_lines = H.ArcLines();

    // Direction offsets: East=0, North=1, West=2, South=3
    static const Int step_dx[] = {1, 0, -1, 0};
    static const Int step_dy[] = {0, 1, 0, -1};

    for (Int f = 0; f < H.FaceCount(); ++f)
    {
        auto darcs = F_dA[f];

        // Find a seed point by stepping perpendicular from a boundary arc
        Int seed_x = -1, seed_y = -1;

        for (auto darc_it = darcs.begin(); darc_it != darcs.end(); ++darc_it)
        {
            Int da = *darc_it;
            Int a = da / Int(2);
            Int d = da % Int(2); // 0 = Tail (forward), 1 = Head (backward)

            if (!H.EdgeActiveQ(a)) continue;

            auto sublist = A_lines[a];
            Int point_count = static_cast<Int>(sublist.end() - sublist.begin());
            if (point_count < 2) continue;

            // Pick the middle segment of the polyline
            Int mid = point_count / 2;
            auto it0 = sublist.begin();
            auto it1 = sublist.begin();
            std::advance(it0, mid - 1);
            std::advance(it1, mid);

            Int x0 = (*it0)[0], y0 = (*it0)[1];
            Int x1 = (*it1)[0], y1 = (*it1)[1];

            // Segment midpoint
            Int mx = (x0 + x1) / 2;
            Int my = (y0 + y1) / 2;

            // Compute segment direction from coordinates
            int seg_dir;
            if      (x1 > x0) seg_dir = 0; // East
            else if (y1 > y0) seg_dir = 1; // North
            else if (x1 < x0) seg_dir = 2; // West
            else               seg_dir = 3; // South

            // Step perpendicular toward face interior.
            // Convention: dA_F[ToDarc(a,Tail)] = right face,
            //             dA_F[ToDarc(a,Head)] = left face.
            // d=0 (face is RIGHT of forward): step (seg_dir + 3) % 4
            // d=1 (face is LEFT  of forward): step (seg_dir + 1) % 4
            int face_dir = (d == 0) ? (seg_dir + 3) % 4 : (seg_dir + 1) % 4;

            Int sx = mx + step_dx[face_dir];
            Int sy = my + step_dy[face_dir];

            // Verify it's a valid, unassigned space cell
            if (sx >= 0 && sx < n_x - 1 && sy >= 0 && sy < n_y
                && face_map[idx(sx, sy)] == Int(-1))
            {
                seed_x = sx;
                seed_y = sy;
                break;
            }
        }

        if (seed_x < 0) continue; // No valid seed found

        // Flood fill from seed
        std::vector<std::pair<Int, Int>> stack;
        stack.push_back({seed_x, seed_y});
        face_map[idx(seed_x, seed_y)] = f;

        while (!stack.empty())
        {
            auto [cx, cy] = stack.back();
            stack.pop_back();

            for (int dir = 0; dir < 4; ++dir)
            {
                Int nx = cx + step_dx[dir];
                Int ny = cy + step_dy[dir];

                if (nx < 0 || nx >= n_x - 1 || ny < 0 || ny >= n_y) continue;

                auto ni = idx(nx, ny);
                if (face_map[ni] == Int(-1))
                {
                    face_map[ni] = f;
                    stack.push_back({nx, ny});
                }
            }
        }
    }

    return face_map;
}

/**
 * @brief Build a per-cell component color map for the diagram grid.
 *
 * Returns a vector parallel to the diagram string. Each cell stores
 * the component color index (from pd.A_color) or -1 if not part of an arc.
 * Uses the same edge-walk pattern as BuildHighlightMask.
 */
std::vector<Int> BuildComponentMap(
    OrthoDraw_T& H, const PD_T& pd,
    const std::string& diagram, Int n_x, Int n_y)
{
    std::vector<Int> comp_map(
        static_cast<std::size_t>(n_x * n_y), Int(-1));

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return x >= 0 && x < n_x - 1 && y >= 0 && y < n_y;
    };

    auto is_body_char = [](char c) -> bool {
        return c == '-' || c == '|' || c == '<' || c == '>'
            || c == '^' || c == 'v';
    };

    const auto& A_E = H.ArcEdges();
    const auto& A_V = H.ArcVertices();
    const auto& E_V = H.Edges();
    const auto& V_coords = H.VertexCoordinates();
    const auto& E_dir = H.EdgeDirections();

    // Mark edge body cells for each arc
    for (Int a = 0; a < H.MaxArcCount(); ++a)
    {
        if (!H.EdgeActiveQ(a)) continue;

        Int color = pd.ArcColors()[a];

        // Walk the edges of this arc
        auto edges = A_E[a];
        for (auto it = edges.begin(); it != edges.end(); ++it)
        {
            Int e = *it;
            if (!H.EdgeActiveQ(e)) continue;

            Int v_0 = E_V(e, 0);
            Int v_1 = E_V(e, 1);
            Int x0 = V_coords(v_0, 0), y0 = V_coords(v_0, 1);
            Int x1 = V_coords(v_1, 0), y1 = V_coords(v_1, 1);

            auto dir = E_dir[e];

            if (dir == OrthoDraw_T::East || dir == OrthoDraw_T::West)
            {
                Int lo_x = std::min(x0, x1);
                Int hi_x = std::max(x0, x1);
                Int y = y0;
                for (Int x = lo_x + 1; x <= hi_x - 1; ++x)
                {
                    if (!in_bounds(x, y)) continue;
                    auto i = idx(x, y);
                    if (is_body_char(diagram[i]))
                        comp_map[i] = color;
                }
            }
            else if (dir == OrthoDraw_T::North || dir == OrthoDraw_T::South)
            {
                Int lo_y = std::min(y0, y1);
                Int hi_y = std::max(y0, y1);
                Int x = x0;
                for (Int y = lo_y + 1; y <= hi_y - 1; ++y)
                {
                    if (!in_bounds(x, y)) continue;
                    auto i = idx(x, y);
                    if (is_body_char(diagram[i]))
                        comp_map[i] = color;
                }
            }
        }

        // Mark corner vertices ('+' characters)
        auto verts = A_V[a];
        for (auto it = verts.begin(); it != verts.end(); ++it)
        {
            Int v = *it;
            Int x = V_coords(v, 0);
            Int y = V_coords(v, 1);
            if (!in_bounds(x, y)) continue;
            auto i = idx(x, y);
            if (diagram[i] == '+')
                comp_map[i] = color;
        }
    }

    // Fix crossing centers: the overstrand character ('-' or '|') belongs
    // to the arc that passes over. Copy color from adjacent cell in the
    // overstrand direction.
    for (Int c = 0; c < H.MaxCrossingCount(); ++c)
    {
        if (!H.VertexActiveQ(c)) continue;

        Int cx = V_coords(c, 0);
        Int cy = V_coords(c, 1);
        if (!in_bounds(cx, cy)) continue;

        auto ci = idx(cx, cy);
        char ch = diagram[ci];

        if (ch == '-')
        {
            // Horizontal overstrand: copy from left or right neighbor
            if (in_bounds(cx - 1, cy) && comp_map[idx(cx - 1, cy)] >= 0)
                comp_map[ci] = comp_map[idx(cx - 1, cy)];
            else if (in_bounds(cx + 1, cy) && comp_map[idx(cx + 1, cy)] >= 0)
                comp_map[ci] = comp_map[idx(cx + 1, cy)];
        }
        else if (ch == '|')
        {
            // Vertical overstrand: copy from above or below neighbor
            if (in_bounds(cx, cy - 1) && comp_map[idx(cx, cy - 1)] >= 0)
                comp_map[ci] = comp_map[idx(cx, cy - 1)];
            else if (in_bounds(cx, cy + 1) && comp_map[idx(cx, cy + 1)] >= 0)
                comp_map[ci] = comp_map[idx(cx, cy + 1)];
        }
    }

    return comp_map;
}

/**
 * @brief Place face labels into diagram, optimizing for maximum clearance.
 *
 * For each face, finds an initial position near the face centroid where the
 * label fits, then hill-climbs one cell at a time to maximize "open distance"
 * — the minimum squared visual Euclidean distance from any label character
 * to the nearest non-whitespace diagram character.  Visual distance accounts
 * for the ~2:1 aspect ratio of monospace characters:
 * dist²((x,y),(z,w)) = (x-z)² + 4·(y-w)².
 *
 * @param face_map  Pre-computed face ownership map (from pristine diagram
 *                  before crossing/arc labels were placed).
 */
void PlaceFaceLabels(std::string& diagram, Int n_x, Int n_y,
                     OrthoDraw_T& H, const std::string& prefix,
                     const std::vector<Int>& face_map)
{
    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    // --- Squared visual Euclidean distance transform ---
    // Computes squared visual distance to the nearest non-whitespace character:
    //   dist(x,y) = min over obstacles (z,w) of { (x-z)^2 + 4*(y-w)^2 }
    // Accounts for the ~2:1 aspect ratio of monospace characters.
    // Uses the Felzenszwalb-Huttenlocher separable algorithm (no sqrt needed
    // since we only compare distances).

    const double INF = 1e18;
    std::vector<double> dist(static_cast<std::size_t>(n_x * n_y));

    // Initialize: 0 at obstacles, INF at free cells
    for (std::size_t i = 0; i < dist.size(); ++i)
        dist[i] = (diagram[i] != ' ') ? 0.0 : INF;

    // 1D weighted squared distance transform (lower envelope of parabolas).
    // Computes d[i] = min_j { f[j] + weight * (i - j)^2 }
    auto dt1d = [](double* f, double* d, int n, double weight,
                   std::vector<int>& v, std::vector<double>& z) {
        v[0] = 0;
        z[0] = -1e20;
        z[1] = 1e20;
        int k = 0;

        for (int q = 1; q < n; ++q)
        {
            for (;;)
            {
                double s = (f[q] + weight * q * q
                          - f[v[k]] - weight * v[k] * v[k])
                         / (2.0 * weight * (q - v[k]));
                if (s > z[k]) { ++k; v[k] = q; z[k] = s; z[k+1] = 1e20; break; }
                --k;
            }
        }

        k = 0;
        for (int q = 0; q < n; ++q)
        {
            while (z[k+1] < q) ++k;
            d[q] = f[v[k]] + weight * (q - v[k]) * (q - v[k]);
        }
    };

    // Scratch buffers for dt1d
    int max_dim = std::max(static_cast<int>(n_x), static_cast<int>(n_y));
    std::vector<int>    env_v(max_dim);
    std::vector<double> env_z(max_dim + 1);
    std::vector<double> buf_in(max_dim), buf_out(max_dim);

    // Pass 1: column-wise (vertical), weight = 4
    for (Int c = 0; c < n_x; ++c)
    {
        for (Int r = 0; r < n_y; ++r)
            buf_in[r] = dist[static_cast<std::size_t>(c + n_x * r)];
        dt1d(buf_in.data(), buf_out.data(), static_cast<int>(n_y), 4.0,
             env_v, env_z);
        for (Int r = 0; r < n_y; ++r)
            dist[static_cast<std::size_t>(c + n_x * r)] = buf_out[r];
    }

    // Pass 2: row-wise (horizontal), weight = 1
    for (Int r = 0; r < n_y; ++r)
    {
        for (Int c = 0; c < n_x; ++c)
            buf_in[c] = dist[static_cast<std::size_t>(c + n_x * r)];
        dt1d(buf_in.data(), buf_out.data(), static_cast<int>(n_x), 1.0,
             env_v, env_z);
        for (Int c = 0; c < n_x; ++c)
            dist[static_cast<std::size_t>(c + n_x * r)] = buf_out[c];
    }

    // Open distance: minimum squared visual distance across all label characters
    auto open_dist = [&](Int x, Int y, Int len) -> double {
        double min_d = INF;
        for (Int i = 0; i < len; ++i)
            min_d = std::min(min_d, dist[idx(x + i, y)]);
        return min_d;
    };

    // Check if label fits: all characters within bounds and are spaces
    auto fits = [&](Int x, Int y, Int len) -> bool {
        if (y < 0 || y >= n_y) return false;
        if (x < 0 || x + len > n_x - 1) return false;
        for (Int i = 0; i < len; ++i)
        {
            if (diagram[idx(x + i, y)] != ' ') return false;
        }
        return true;
    };

    // --- Place each face label ---
    for (Int f = 0; f < H.FaceCount(); ++f)
    {
        std::string label = prefix + std::to_string(f);
        Int label_len = static_cast<Int>(label.size());

        // Compute centroid of face cells
        double sum_x = 0.0, sum_y = 0.0;
        Int count = 0;
        for (Int y = 0; y < n_y; ++y)
        {
            for (Int x = 0; x < n_x - 1; ++x)
            {
                if (face_map[idx(x, y)] == f)
                {
                    sum_x += static_cast<double>(x);
                    sum_y += static_cast<double>(y);
                    ++count;
                }
            }
        }
        if (count == 0) continue;

        double centroid_x = sum_x / count;
        double centroid_y = sum_y / count;

        // Find initial position: closest to centroid, first char in face, label fits
        Int best_x = -1, best_y = -1;
        double best_eucl = std::numeric_limits<double>::max();

        for (Int y = 0; y < n_y; ++y)
        {
            for (Int x = 0; x < n_x - 1; ++x)
            {
                if (face_map[idx(x, y)] != f) continue;
                if (!fits(x, y, label_len)) continue;

                double dx = static_cast<double>(x) - centroid_x;
                double dy = static_cast<double>(y) - centroid_y;
                double d = dx * dx + dy * dy;
                if (d < best_eucl)
                {
                    best_eucl = d;
                    best_x = x;
                    best_y = y;
                }
            }
        }

        if (best_x < 0) continue;

        // Hill-climb to maximize open distance (move one cell at a time)
        for (;;)
        {
            double current_od = open_dist(best_x, best_y, label_len);
            Int next_x = best_x, next_y = best_y;
            double next_od = current_od;

            static const Int step_dx[] = {1, -1, 0, 0};
            static const Int step_dy[] = {0, 0, 1, -1};

            for (int dir = 0; dir < 4; ++dir)
            {
                Int tx = best_x + step_dx[dir];
                Int ty = best_y + step_dy[dir];
                if (!fits(tx, ty, label_len)) continue;
                // Keep first character anchored in this face
                if (face_map[idx(tx, ty)] != f) continue;

                double od = open_dist(tx, ty, label_len);
                if (od > next_od)
                {
                    next_od = od;
                    next_x = tx;
                    next_y = ty;
                }
            }

            if (next_od <= current_od) break;
            best_x = next_x;
            best_y = next_y;
        }

        // Write label into diagram
        for (Int i = 0; i < label_len; ++i)
        {
            diagram[idx(best_x + i, best_y)] =
                label[static_cast<std::size_t>(i)];
        }
    }
}

/**
 * @brief Place component labels ("comp0", "comp1", ...) in ASCII mode.
 *
 * One label per component color, placed on the topmost horizontal edge
 * segment long enough to hold the label text. Called after PlaceArcLabels
 * to avoid obscuring arc labels.
 */
void PlaceComponentLabels(
    std::string& diagram, Int n_x, Int n_y,
    OrthoDraw_T& H, const PD_T& pd)
{
    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    const auto& A_lines = H.ArcLines();

    // Group arcs by color
    std::map<Int, std::vector<Int>> color_arcs;
    for (Int a = 0; a < H.MaxArcCount(); ++a)
    {
        if (!H.EdgeActiveQ(a)) continue;
        color_arcs[pd.ArcColors()[a]].push_back(a);
    }

    for (const auto& [color, arcs] : color_arcs)
    {
        std::string label = "comp" + std::to_string(color);
        Int label_len = static_cast<Int>(label.size());

        // Collect all horizontal segments from all arcs in this component
        struct HRun { Int x_start; Int y; Int length; };
        std::vector<HRun> runs;

        for (Int a : arcs)
        {
            auto sublist = A_lines[a];
            Int point_count = static_cast<Int>(sublist.end() - sublist.begin());
            if (point_count < 2) continue;

            auto it = sublist.begin();
            auto prev = it;
            ++it;
            for (; it != sublist.end(); prev = it, ++it)
            {
                Int x0 = (*prev)[0], y0 = (*prev)[1];
                Int x1 = (*it)[0],   y1 = (*it)[1];

                if (y0 != y1 || x0 == x1) continue; // not horizontal

                Int lo = std::min(x0, x1);
                Int hi = std::max(x0, x1);
                Int y  = y0;

                // Find contiguous runs of '-' in this segment
                Int run_start = -1;
                Int run_len   = 0;

                for (Int x = lo; x <= hi; ++x)
                {
                    if (x >= 0 && x < n_x - 1 && y >= 0 && y < n_y
                        && diagram[idx(x, y)] == '-')
                    {
                        if (run_len == 0) run_start = x;
                        ++run_len;
                    }
                    else
                    {
                        if (run_len >= label_len)
                            runs.push_back({run_start, y, run_len});
                        run_len = 0;
                    }
                }
                if (run_len >= label_len)
                    runs.push_back({run_start, y, run_len});
            }
        }

        if (runs.empty()) continue;

        // Pick the topmost run (highest y in math coords).
        // Break ties by longest run.
        auto best = std::max_element(runs.begin(), runs.end(),
            [](const HRun& a, const HRun& b) {
                if (a.y != b.y) return a.y < b.y;
                return a.length < b.length;
            });

        // Center the label within the run
        Int offset  = (best->length - label_len) / 2;
        Int start_x = best->x_start + offset;
        for (Int i = 0; i < label_len; ++i)
        {
            diagram[idx(start_x + i, best->y)] =
                label[static_cast<std::size_t>(i)];
        }
    }
}

//==============================================================================
// Highlight Infrastructure
//==============================================================================

/**
 * @brief Parse highlight spec strings into a HighlightSet.
 *
 * Each spec is tokenized on comma/space/tab. Tokens start with 'a', 'c', or 'f'
 * followed by a non-negative integer index. Out-of-range or malformed tokens
 * produce warnings on stderr.
 */
HighlightSet ParseHighlightSpecs(const std::vector<std::string>& specs,
                                  Int max_arc, Int max_crossing, Int max_face)
{
    HighlightSet hs;

    for (const auto& spec : specs)
    {
        std::size_t pos = 0;
        while (pos < spec.size())
        {
            // Skip delimiters
            while (pos < spec.size()
                   && (spec[pos] == ',' || spec[pos] == ' ' || spec[pos] == '\t'))
                ++pos;
            if (pos >= spec.size()) break;

            char type_char = spec[pos];
            if (type_char != 'a' && type_char != 'c' && type_char != 'f')
            {
                std::cerr << "Warning: Unknown highlight type '"
                          << type_char << "' (expected a/c/f)\n";
                while (pos < spec.size()
                       && spec[pos] != ',' && spec[pos] != ' ' && spec[pos] != '\t')
                    ++pos;
                continue;
            }
            ++pos;

            // Parse integer index
            std::size_t num_start = pos;
            while (pos < spec.size() && spec[pos] >= '0' && spec[pos] <= '9')
                ++pos;

            if (pos == num_start)
            {
                std::cerr << "Warning: Missing index after '"
                          << type_char << "' in highlight spec\n";
                continue;
            }

            Int index;
            try {
                index = static_cast<Int>(
                    std::stoll(spec.substr(num_start, pos - num_start)));
            } catch (...) {
                std::cerr << "Warning: Invalid index in highlight spec\n";
                continue;
            }

            switch (type_char)
            {
                case 'a':
                    if (index < 0 || index >= max_arc)
                        std::cerr << "Warning: Arc index " << index
                                  << " out of range [0, " << max_arc - 1 << "]\n";
                    else
                        hs.arcs.insert(index);
                    break;
                case 'c':
                    if (index < 0 || index >= max_crossing)
                        std::cerr << "Warning: Crossing index " << index
                                  << " out of range [0, " << max_crossing - 1 << "]\n";
                    else
                        hs.crossings.insert(index);
                    break;
                case 'f':
                    if (index < 0 || index >= max_face)
                        std::cerr << "Warning: Face index " << index
                                  << " out of range [0, " << max_face - 1 << "]\n";
                    else
                        hs.faces.insert(index);
                    break;
            }
        }
    }

    return hs;
}

/**
 * @brief Build a per-cell highlight mask for the diagram grid.
 *
 * The mask has the same flat layout as the diagram string (n_x * n_y).
 * Built from the pristine diagram (before labels are placed).
 *
 * - Arcs: walks each edge of highlighted arcs, marking cells with edge chars.
 * - Crossings: marks the crossing cell and adjacent body chars (- or |).
 * - Faces: marks space cells owned by highlighted faces.
 */
/**
 * @brief Mark the cells drawn for a set of arcs with a highlight type.
 *
 * Shared by `--highlight=a<i>` (type Arc) and by a `--move` descriptor's
 * strand W (type Strand). When `marker` is non-zero the midpoint of every
 * sufficiently long straight run is overwritten with it in `diagram`, which
 * is how W is labelled in `--mono` output (no colour to distinguish it).
 */
void MarkArcCells(
    OrthoDraw_T& H, std::string& diagram,
    Int n_x, Int n_y,
    const std::vector<Int>& arcs,
    std::vector<HighlightType>& mask,
    HighlightType type,
    char marker = '\0')
{
    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return x >= 0 && x < n_x - 1 && y >= 0 && y < n_y;
    };

    auto is_edge_char = [](char c) -> bool {
        return c == '-' || c == '|' || c == '<' || c == '>'
            || c == '^' || c == 'v';
    };

    const auto& A_E = H.ArcEdges();
    const auto& A_V = H.ArcVertices();
    const auto& E_V = H.Edges();
    const auto& V_coords = H.VertexCoordinates();
    const auto& E_dir = H.EdgeDirections();

    // A marker is placed at most once per arc, on the longest straight run,
    // so a short arc does not lose its only stroke to a label. If no arc of the
    // set had a long enough run, the best cell seen anywhere gets one, so W is
    // never left entirely unlabelled.
    constexpr Int min_run_for_marker = 3;

    Int overall_len = 0;
    std::size_t overall_cell = 0;
    bool overall_have = false;
    bool placed_any = false;

    for (Int a : arcs)
    {
        if (!H.EdgeActiveQ(a)) continue;

        Int best_len = 0;
        std::size_t best_cell = 0;
        bool have_cell = false;

        auto consider = [&](Int len, Int x, Int y)
        {
            if ((len < Int(1)) || !in_bounds(x, y)) { return; }

            if ((len >= min_run_for_marker) && (len > best_len))
            {
                best_len  = len;
                best_cell = idx(x, y);
                have_cell = true;
            }
            if (len > overall_len)
            {
                overall_len  = len;
                overall_cell = idx(x, y);
                overall_have = true;
            }
        };

        auto edges = A_E[a];
        for (auto it = edges.begin(); it != edges.end(); ++it)
        {
            Int e = *it;
            if (!H.EdgeActiveQ(e)) continue;

            Int v_0 = E_V(e, 0);
            Int v_1 = E_V(e, 1);
            Int x0 = V_coords(v_0, 0), y0 = V_coords(v_0, 1);
            Int x1 = V_coords(v_1, 0), y1 = V_coords(v_1, 1);

            auto dir = E_dir[e];

            if (dir == OrthoDraw_T::East || dir == OrthoDraw_T::West)
            {
                Int lo_x = std::min(x0, x1);
                Int hi_x = std::max(x0, x1);
                Int y = y0;
                for (Int x = lo_x + 1; x <= hi_x - 1; ++x)
                {
                    if (!in_bounds(x, y)) continue;
                    auto i = idx(x, y);
                    if (is_edge_char(diagram[i])) mask[i] = type;
                }
                consider(hi_x - lo_x - 1, (lo_x + hi_x) / 2, y);
            }
            else if (dir == OrthoDraw_T::North || dir == OrthoDraw_T::South)
            {
                Int lo_y = std::min(y0, y1);
                Int hi_y = std::max(y0, y1);
                Int x = x0;
                for (Int y = lo_y + 1; y <= hi_y - 1; ++y)
                {
                    if (!in_bounds(x, y)) continue;
                    auto i = idx(x, y);
                    if (is_edge_char(diagram[i])) mask[i] = type;
                }
                consider(hi_y - lo_y - 1, x, (lo_y + hi_y) / 2);
            }
        }

        auto verts = A_V[a];
        for (auto it = verts.begin(); it != verts.end(); ++it)
        {
            Int v = *it;
            Int x = V_coords(v, 0);
            Int y = V_coords(v, 1);
            if (!in_bounds(x, y)) continue;
            auto i = idx(x, y);
            if (diagram[i] == '+') mask[i] = type;
        }

        if (marker && have_cell && (mask[best_cell] == type))
        {
            diagram[best_cell] = marker;
            placed_any = true;
        }
    }

    if (marker && !placed_any && overall_have && (mask[overall_cell] == type))
    {
        diagram[overall_cell] = marker;
    }
}

std::vector<HighlightType> BuildHighlightMask(
    OrthoDraw_T& H, const std::string& diagram,
    Int n_x, Int n_y,
    const HighlightSet& highlights,
    const std::vector<Int>& face_map)
{
    std::vector<HighlightType> mask(
        static_cast<std::size_t>(n_x * n_y), HighlightType::None);

    auto idx = [n_x, n_y](Int x, Int y) -> std::size_t {
        return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y));
    };

    auto in_bounds = [n_x, n_y](Int x, Int y) -> bool {
        return x >= 0 && x < n_x - 1 && y >= 0 && y < n_y;
    };

    auto is_edge_char = [](char c) -> bool {
        return c == '-' || c == '|' || c == '<' || c == '>'
            || c == '^' || c == 'v';
    };

    const auto& A_E = H.ArcEdges();
    const auto& A_V = H.ArcVertices();
    const auto& E_V = H.Edges();
    const auto& V_coords = H.VertexCoordinates();
    const auto& E_dir = H.EdgeDirections();

    // --- Arc highlighting ---
    for (Int a : highlights.arcs)
    {
        if (!H.EdgeActiveQ(a)) continue;

        // Mark edge body cells
        auto edges = A_E[a];
        for (auto it = edges.begin(); it != edges.end(); ++it)
        {
            Int e = *it;
            if (!H.EdgeActiveQ(e)) continue;

            Int v_0 = E_V(e, 0);
            Int v_1 = E_V(e, 1);
            Int x0 = V_coords(v_0, 0), y0 = V_coords(v_0, 1);
            Int x1 = V_coords(v_1, 0), y1 = V_coords(v_1, 1);

            auto dir = E_dir[e];

            if (dir == OrthoDraw_T::East || dir == OrthoDraw_T::West)
            {
                Int lo_x = std::min(x0, x1);
                Int hi_x = std::max(x0, x1);
                Int y = y0;
                for (Int x = lo_x + 1; x <= hi_x - 1; ++x)
                {
                    if (!in_bounds(x, y)) continue;
                    auto i = idx(x, y);
                    if (is_edge_char(diagram[i]))
                        mask[i] = HighlightType::Arc;
                }
            }
            else if (dir == OrthoDraw_T::North || dir == OrthoDraw_T::South)
            {
                Int lo_y = std::min(y0, y1);
                Int hi_y = std::max(y0, y1);
                Int x = x0;
                for (Int y = lo_y + 1; y <= hi_y - 1; ++y)
                {
                    if (!in_bounds(x, y)) continue;
                    auto i = idx(x, y);
                    if (is_edge_char(diagram[i]))
                        mask[i] = HighlightType::Arc;
                }
            }
        }

        // Mark corner vertices ('+' characters)
        auto verts = A_V[a];
        for (auto it = verts.begin(); it != verts.end(); ++it)
        {
            Int v = *it;
            Int x = V_coords(v, 0);
            Int y = V_coords(v, 1);
            if (!in_bounds(x, y)) continue;
            auto i = idx(x, y);
            if (diagram[i] == '+')
                mask[i] = HighlightType::Arc;
        }
    }

    // Mark crossing centers where both sides of the overstrand are
    // highlighted arcs (makes consecutive highlighted arcs visually
    // continuous through the overstrand).
    // A '-' overstrand is highlighted iff both left and right neighbors
    // are Arc-highlighted (#-# pattern). A '|' overstrand likewise
    // requires both above and below to be Arc-highlighted.
    if (!highlights.arcs.empty())
    {
        for (Int c = 0; c < H.MaxCrossingCount(); ++c)
        {
            if (!H.VertexActiveQ(c)) continue;

            Int cx = V_coords(c, 0);
            Int cy = V_coords(c, 1);
            if (!in_bounds(cx, cy)) continue;

            auto ci = idx(cx, cy);
            char ch = diagram[ci];

            if (ch == '-')
            {
                // Horizontal overstrand: both left and right must be Arc
                if (in_bounds(cx - 1, cy) && in_bounds(cx + 1, cy)
                    && mask[idx(cx - 1, cy)] == HighlightType::Arc
                    && mask[idx(cx + 1, cy)] == HighlightType::Arc)
                {
                    mask[ci] = HighlightType::Arc;
                }
            }
            else if (ch == '|')
            {
                // Vertical overstrand: both above and below must be Arc
                if (in_bounds(cx, cy - 1) && in_bounds(cx, cy + 1)
                    && mask[idx(cx, cy - 1)] == HighlightType::Arc
                    && mask[idx(cx, cy + 1)] == HighlightType::Arc)
                {
                    mask[ci] = HighlightType::Arc;
                }
            }
        }
    }

    // --- Crossing highlighting ---
    for (Int c : highlights.crossings)
    {
        if (!H.VertexActiveQ(c)) continue;

        Int cx = V_coords(c, 0);
        Int cy = V_coords(c, 1);

        // Mark the crossing cell itself
        if (in_bounds(cx, cy))
        {
            mask[idx(cx, cy)] = HighlightType::Crossing;
        }

        // Mark 4 adjacent cells that are part of the crossing's visual
        // (body chars and arrowheads pointing into/out of the crossing)
        static const Int dx[] = {1, -1, 0, 0};
        static const Int dy[] = {0, 0, 1, -1};
        for (int d = 0; d < 4; ++d)
        {
            Int nx = cx + dx[d];
            Int ny = cy + dy[d];
            if (!in_bounds(nx, ny)) continue;
            auto i = idx(nx, ny);
            if (is_edge_char(diagram[i]))
                mask[i] = HighlightType::Crossing;
        }
    }

    // --- Face highlighting ---
    if (!highlights.faces.empty() && !face_map.empty())
    {
        // The exterior face can have disconnected grid regions (the flood-fill
        // seed only reaches one connected component). Unassigned space cells
        // (face_map == -1) are topologically part of the exterior face.
        Int ext_face = H.ExteriorFace();
        bool highlight_exterior = highlights.faces.count(ext_face) > 0;

        for (Int y = 0; y < n_y; ++y)
        {
            for (Int x = 0; x < n_x - 1; ++x)
            {
                auto i = idx(x, y);
                if (diagram[i] != ' ') continue;
                Int face = face_map[i];
                if (face >= 0 && highlights.faces.count(face))
                    mask[i] = HighlightType::Face;
                else if (face == Int(-1) && highlight_exterior)
                    mask[i] = HighlightType::Face;
            }
        }
    }

    return mask;
}

/**
 * @brief Apply ASCII-mode highlighting to a diagram string.
 *
 * For Arc/Crossing cells: replaces '-' or '|' with '#'.
 * For Face cells: replaces ' ' with '.'.
 * Arrowheads, corners, and label text are left unchanged.
 */
void ApplyHighlightASCII(std::string& diagram,
                          const std::vector<HighlightType>& mask)
{
    for (std::size_t i = 0; i < mask.size() && i < diagram.size(); ++i)
    {
        switch (mask[i])
        {
            case HighlightType::Arc:
            case HighlightType::Crossing:
                if (diagram[i] == '-' || diagram[i] == '|')
                    diagram[i] = '#';
                break;
            case HighlightType::Face:
                if (diagram[i] == ' ')
                    diagram[i] = '.';
                break;
            default:
                break;
        }
    }
}

//==============================================================================
// Settings Construction
//==============================================================================

/**
 * @brief Build OrthoDraw settings from config: library defaults → preset → overrides.
 */
OrthoDraw_T::Settings_T BuildSettings(const Config& config)
{
    OrthoDraw_T::Settings_T s{};

    // Layer 1: Library defaults (soften_virtual_edgesQ left at library default)

    // Layer 2: Quality preset
    if (config.quality_preset)
    {
        const auto& preset = *config.quality_preset;

        if (preset == "fast")
        {
            s.compaction_method    = OrthoDraw_T::CompactionMethod_T::TopologicalOrdering;
            s.redistribute_bendsQ  = false;
            s.turn_regularizeQ     = false;
        }
        // "best", "debug", "default": no changes from library defaults
    }

    // Layer 3: Individual overrides
    if (config.bend_method)              s.bend_method              = *config.bend_method;
    if (config.compaction_method)        s.compaction_method        = *config.compaction_method;
    if (config.randomize_bends)          s.randomize_bends          = *config.randomize_bends;
    if (config.x_rounding_radius)        s.x_rounding_radius        = *config.x_rounding_radius;
    if (config.y_rounding_radius)        s.y_rounding_radius        = *config.y_rounding_radius;
    if (config.redistribute_bends)       s.redistribute_bendsQ      = *config.redistribute_bends;
    if (config.turn_regularize)          s.turn_regularizeQ         = *config.turn_regularize;
    if (config.saturate_regions)         s.saturate_regionsQ        = *config.saturate_regions;
    if (config.saturate_exterior)        s.saturate_exterior_regionQ = *config.saturate_exterior;
    if (config.filter_saturating_edges)  s.filter_saturating_edgesQ = *config.filter_saturating_edges;
    if (config.randomize_virtual_edges)  s.randomize_virtual_edgesQ = *config.randomize_virtual_edges;
    if (config.dual_simplex)             s.use_dual_simplexQ        = *config.dual_simplex;

    // Grid/gap sizes always come from Config (with auto-sizing logic applied in ParseArguments)
    s.x_grid_size = config.x_grid_size;
    s.y_grid_size = config.y_grid_size;
    s.x_gap_size  = config.x_gap_size;
    s.y_gap_size  = config.y_gap_size;

    return s;
}

/**
 * @brief Validate that CLP methods are available if requested. Returns true if valid.
 */
bool ValidateCLPSettings(const OrthoDraw_T::Settings_T& settings)
{
#ifndef KNOODLE_USE_CLP
    if (settings.bend_method == OrthoDraw_T::BendMethod_T::Bends_CLP)
    {
        std::cerr << "Error: --bend-method=clp requires CLP support (compile with -DKNOODLE_USE_CLP)\n";
        return false;
    }
    if (settings.compaction_method == OrthoDraw_T::CompactionMethod_T::Length_CLP
     || settings.compaction_method == OrthoDraw_T::CompactionMethod_T::AreaAndLength_CLP)
    {
        std::cerr << "Error: CLP compaction methods require CLP support (compile with -DKNOODLE_USE_CLP)\n";
        return false;
    }
#endif
    (void)settings;
    return true;
}

/**
 * @brief Validate that settings don't combine options known to produce
 *        incorrect drawings. Returns true if valid.
 */
bool ValidateSettingsCombinations(const OrthoDraw_T::Settings_T& settings)
{
    if (!settings.redistribute_bendsQ && !settings.saturate_regionsQ)
    {
        std::cerr
            << "Error: --no-redistribute-bends and --no-saturate-regions cannot be used together.\n"
            << "Without bend redistribution, the MCF solver's raw bend placement can route\n"
            << "non-adjacent edges close together. Without region saturation, the constraint\n"
            << "graph lacks the spacing constraints to keep those edges apart during compaction,\n"
            << "which produces false visual crossings in the output.\n"
            << "Re-enable at least one of these options.\n";
        return false;
    }
    return true;
}

//==============================================================================
// Drawing
//==============================================================================

/**
 * @brief Emit one diagram's OrthoDraw layout as a Wolfram Language association:
 *        <| "BoundingBox"->{w,h},
 *           "Arcs"->{ <|"Id"->a,"Component"->c,"Color"->k,"Points"->{{x,y},..}|>, .. },
 *           "Crossings"->{ <|"Id"->c,"Pos"->{x,y}|>, .. },
 *           "Faces"->{ <|"Id"->f,"Exterior"->True|False,"Color"->±1,"Boundary"->{{x,y},..}|>, .. } |>
 *
 * Arcs: each is its routed polyline of integer grid points. The over/under gaps
 * are already baked into ArcLines() (the under-strand is inset at each
 * crossing), so rendering the polylines directly yields correct
 * broken-under-strand crossings. "Component" is a per-summand topological
 * renumbering (ArcLinkComponents, restarts at 0 each summand); "Color" is the
 * raw wire color (ArcColors) identifying the physical link component across all
 * summands of a drawing call (uncolored input defaults to 0; the key is present
 * for any real color and omitted only for the Uninitialized sentinel).
 *
 * Crossings: grid position of each active crossing (crossings are vertices
 * [0, MaxCrossingCount()) in VertexCoordinates()).
 *
 * Faces: boundary of EVERY face, including the exterior one (flagged
 * "Exterior"->True; its boundary is the outer silhouette of the whole
 * diagram, needed for exterior-face label placement), as a closed point
 * cycle, plus its two-coloring from CheckerBoardColoring(). Boundaries are
 * built from ArcVertices() (the RAW, un-gapped vertex-index path per arc) and
 * VertexCoordinates() rather than from ArcLines() -- ArcLines()'s under-strand
 * gap inset is correct for drawing broken strands, but would leave a notch in
 * a filled face polygon at every crossing.
 *
 * Parses via ToExpression.
 */
void EmitWolframGeometry(OrthoDraw_T& H, const PD_T& pd, std::ostream& out)
{
    const auto& A_lines  = H.ArcLines();
    const auto& A_verts  = H.ArcVertices();
    const auto& V_coords = H.VertexCoordinates();
    const Int*  comp     = pd.ArcLinkComponents().data();

    out << "<|\"BoundingBox\"->{" << H.Width() << "," << H.Height() << "},\"Arcs\"->{";

    bool first_arc = true;
    for (Int a = 0; a < H.MaxArcCount(); ++a)
    {
        if (!H.EdgeActiveQ(a)) { continue; }

        auto sublist = A_lines[a];
        if (sublist.end() - sublist.begin() < 2) { continue; }

        out << (first_arc ? "" : ",")
            << "<|\"Id\"->" << a
            << ",\"Component\"->" << comp[a];
        // Raw wire color (ArcColors) identifies which physical link component
        // the arc belongs to -- unlike "Component" (ArcLinkComponents), a
        // per-summand topological renumbering that restarts at 0 in every
        // summand, so connect-sum factors of a one-component knot all read
        // "Component"->0 yet share one wire color. Active arcs normally carry a
        // real color (uncolored input still defaults to 0), so the key is
        // effectively always present; the guard only suppresses the
        // Uninitialized sentinel (-1) so it is never emitted as a bogus color
        // class -- faces legitimately use -1 for their checkerboard sign.
        // Absence of the key therefore means an older binary; downstream falls
        // back to per-summand "Component" runs in that case.
        const Int wire_color = pd.ArcColors()[a];
        if (wire_color != PD_T::Uninitialized) { out << ",\"Color\"->" << wire_color; }
        out << ",\"Points\"->{";
        first_arc = false;

        bool first_pt = true;
        for (auto it = sublist.begin(); it != sublist.end(); ++it)
        {
            out << (first_pt ? "" : ",") << "{" << (*it)[0] << "," << (*it)[1] << "}";
            first_pt = false;
        }
        out << "}|>";
    }
    out << "},\"Crossings\"->{";

    bool first_c = true;
    for (Int c = 0; c < H.MaxCrossingCount(); ++c)
    {
        if (!pd.CrossingActiveQ(c)) { continue; }
        out << (first_c ? "" : ",")
            << "<|\"Id\"->" << c
            << ",\"Pos\"->{" << V_coords(c,0) << "," << V_coords(c,1) << "}|>";
        first_c = false;
    }
    out << "},\"Faces\"->{";

    // CheckerBoardColoring() mutates cache state and needs a non-const PD_T, so
    // work on a copy; face ids match H's (OrthoDraw is built directly from pd).
    PD_T pd_copy = pd;
    auto coloring = pd_copy.CheckerBoardColoring();
    const bool have_color = (coloring.Dimension(0) == H.FaceCount());

    const auto& F_dA = H.FaceDarcs();
    const Int   ext  = H.ExteriorFace();

    bool first_f = true;
    for (Int f = 0; f < H.FaceCount(); ++f)
    {
        out << (first_f ? "" : ",")
            << "<|\"Id\"->" << f
            << ",\"Exterior\"->" << (f == ext ? "True" : "False")
            << ",\"Color\"->" << (have_color ? int(coloring[f]) : 0)
            << ",\"Boundary\"->{";
        first_f = false;

        std::vector<Int> darc_list(F_dA[f].begin(), F_dA[f].end());
        const Int m = static_cast<Int>(darc_list.size());

        bool first_pt = true;
        Int  prev_end_vertex = Int(-1);   // vertex the previous darc's walk ended on

        for (Int i = 0; i < m; ++i)
        {
            const Int da = darc_list[i];
            const Int a  = da / Int(2);
            const Int d  = da % Int(2);

            auto vsub = A_verts[a];
            std::vector<Int> vidx(vsub.begin(), vsub.end());
            const Int n = static_cast<Int>(vidx.size());
            if (n < 1) { continue; }

            auto emit_vertex = [&](Int v)
            {
                out << (first_pt ? "" : ",")
                    << "{" << V_coords(v,0) << "," << V_coords(v,1) << "}";
                first_pt = false;
            };

            // Orient this arc's raw vertex path so it continues from wherever the
            // previous darc's walk left off, verified by vertex-index equality --
            // rather than trusting an inferred Tail/Head "forward means stored
            // order" convention (that produced spurious jumps across faces in
            // testing: a face's darc order need not alternate Tail/Head in a way
            // that maps to a fixed forward/backward rule).
            bool forwardQ;
            if (prev_end_vertex >= Int(0))
            {
                if      (vidx.front() == prev_end_vertex) { forwardQ = true;  }
                else if (vidx.back()  == prev_end_vertex) { forwardQ = false; }
                else
                {
                    Knoodle::eprint("EmitWolframGeometry: face " + Knoodle::ToString(f)
                         + " darc for arc " + Knoodle::ToString(a)
                         + " does not connect to the previous vertex "
                         + Knoodle::ToString(prev_end_vertex) + "; face boundary may be wrong.");
                    forwardQ = (d == Int(PD_T::Tail));
                }
            }
            else if (i + 1 < m)
            {
                // First darc of a multi-darc face: nothing to connect to yet, so
                // orient by which endpoint is shared with the NEXT darc's arc.
                auto vsub_next = A_verts[darc_list[i + 1] / Int(2)];
                std::vector<Int> vnext(vsub_next.begin(), vsub_next.end());
                auto sharesQ = [&](Int v) {
                    return !vnext.empty() && (vnext.front() == v || vnext.back() == v);
                };
                if      (sharesQ(vidx.back()))  { forwardQ = true;  }
                else if (sharesQ(vidx.front())) { forwardQ = false; }
                else                            { forwardQ = (d == Int(PD_T::Tail)); } // shouldn't happen
            }
            else
            {
                // Single-darc face (one arc's own loop bounds it): direction is
                // irrelevant to the resulting polygon.
                forwardQ = (d == Int(PD_T::Tail));
            }

            if (forwardQ)
            {
                for (Int k = (first_pt ? 0 : 1); k < n; ++k) { emit_vertex(vidx[k]); }
                prev_end_vertex = vidx[n - 1];
            }
            else
            {
                for (Int k = (first_pt ? n - 1 : n - 2); k >= 0; --k) { emit_vertex(vidx[k]); }
                prev_end_vertex = vidx[0];
            }
        }
        out << "}|>";
    }
    out << "}|>\n";
}

//==============================================================================
// Pass-Move Overlay (--move=..., docs/move-descriptor.md)
//==============================================================================

using Deco_T = Knoodle::OrthoDecorate<PD_T>;

/**
 * @brief Parse a pass-move descriptor per docs/move-descriptor.md.
 *
 * Accepts the payload of a `#move` header line (a leading "#move" token and
 * a "kind=pass" token are both optional; any other kind is an error):
 *
 *     [#move] [kind=pass] strand=DA[,DA...] depart=DA
 *             [cross=DA:u|o[,DA:u|o...]] land=DA
 *
 * All references are darcs (da = 2*arc + d, Tail=0/Head=1) against the
 * diagram being drawn. Returns false with a message in `err` on bad syntax.
 */
bool ParsePassMove(const std::string& spec, Deco_T::PassMove_T& mv,
                   std::string& err)
{
    bool have_strand = false, have_depart = false, have_land = false;

    auto parse_int = [&](std::string_view tok, Int& out) -> bool
    {
        std::int64_t v = 0;
        auto [p, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), v);
        if (ec != std::errc{} || p != tok.data() + tok.size()) return false;
        out = static_cast<Int>(v);
        return true;
    };

    auto for_each_item = [](std::string_view list, auto&& f) -> bool
    {
        while (!list.empty())
        {
            auto comma = list.find(',');
            std::string_view item = list.substr(0, comma);
            if (!f(item)) return false;
            if (comma == std::string_view::npos) break;
            list.remove_prefix(comma + 1);
        }
        return true;
    };

    std::istringstream iss(spec);
    std::string tok;
    while (iss >> tok)
    {
        if (tok == "#move") continue;

        auto eq = tok.find('=');
        if (eq == std::string::npos)
        {
            err = "expected key=value, got '" + tok + "'";
            return false;
        }
        std::string key = tok.substr(0, eq);
        std::string val = tok.substr(eq + 1);

        if (key == "kind")
        {
            if (val == "pass")            { mv.middlepassQ = false; }
            else if (val == "middlepass") { mv.middlepassQ = true;  }
            else
            {
                err = "unsupported kind=" + val
                    + " (want pass or middlepass)";
                return false;
            }
        }
        else if (key == "strand")
        {
            have_strand = true;
            if (!for_each_item(val, [&](std::string_view item)
                {
                    Int da;
                    if (!parse_int(item, da)) return false;
                    mv.strand.push_back(da);
                    return true;
                }))
            {
                err = "bad strand darc list '" + val + "'";
                return false;
            }
        }
        else if (key == "depart")
        {
            have_depart = true;
            if (!parse_int(val, mv.depart))
            {
                err = "bad depart darc '" + val + "'";
                return false;
            }
        }
        else if (key == "cross")
        {
            if (!for_each_item(val, [&](std::string_view item)
                {
                    auto colon = item.find(':');
                    if (colon == std::string_view::npos) return false;
                    Int da;
                    if (!parse_int(item.substr(0, colon), da)) return false;
                    std::string_view tag = item.substr(colon + 1);
                    if (tag != "u" && tag != "o") return false;
                    mv.cross.push_back(da);
                    mv.over.push_back(tag == "o");
                    return true;
                }))
            {
                err = "bad cross list '" + val + "' (want DA:u or DA:o)";
                return false;
            }
        }
        else if (key == "land")
        {
            have_land = true;
            if (!parse_int(val, mv.land))
            {
                err = "bad land darc '" + val + "'";
                return false;
            }
        }
        else
        {
            err = "unknown key '" + key + "'";
            return false;
        }
    }

    if (!have_strand || !have_depart || !have_land)
    {
        err = "descriptor needs strand=, depart= and land=";
        return false;
    }
    return true;
}

/**
 * @brief Pad the drawing canvas (diagram string + parallel cell maps) by
 * `m` blank cells on every side, updating n_x/n_y.
 *
 * OrthoDecorate is given a margin so corridors through the drawn exterior
 * face can route AROUND the diagram (the unpadded exterior clip is
 * disconnected corner pockets); its route coordinates include that margin,
 * so the canvas must grow to match before stamping.
 */
void PadCanvas(std::string& diagram, std::vector<HighlightType>& mask,
               std::vector<Int>& comp_map, Int& n_x, Int& n_y, Int m)
{
    const Int old_nx = n_x, old_ny = n_y;
    const Int new_nx = n_x + 2 * m, new_ny = n_y + 2 * m;

    std::vector<std::string> lines;
    {
        std::istringstream iss(diagram);
        std::string line;
        while (std::getline(iss, line)) lines.push_back(line);
    }
    lines.resize(static_cast<std::size_t>(old_ny));

    const std::string blank(static_cast<std::size_t>(new_nx - 1), ' ');
    std::string out;
    out.reserve(static_cast<std::size_t>(new_nx * new_ny));

    for (Int r = 0; r < m; ++r) { out += blank; out += '\n'; }
    for (Int r = 0; r < old_ny; ++r)
    {
        std::string& row = lines[static_cast<std::size_t>(r)];
        row.resize(static_cast<std::size_t>(old_nx - 1), ' ');
        out.append(static_cast<std::size_t>(m), ' ');
        out += row;
        out.append(static_cast<std::size_t>(m), ' ');
        out += '\n';
    }
    for (Int r = 0; r < m; ++r) { out += blank; out += '\n'; }

    auto pad_map = [&](auto& map, auto fill)
    {
        if (map.empty()) return;
        std::decay_t<decltype(map)> padded(
            static_cast<std::size_t>(new_nx * new_ny), fill);
        for (Int r = 0; r < old_ny; ++r)
        {
            for (Int c = 0; c < old_nx - 1; ++c)
            {
                padded[static_cast<std::size_t>((r + m) * new_nx + (c + m))] =
                    map[static_cast<std::size_t>(r * old_nx + c)];
            }
        }
        map = std::move(padded);
    };

    pad_map(mask, HighlightType::None);
    pad_map(comp_map, Int(-1));

    diagram = std::move(out);
    n_x = new_nx;
    n_y = new_ny;
}

/**
 * @brief Stamp a rendered pass route into the ASCII diagram + highlight mask.
 *
 * Overlay strokes use placeholder characters that UnicodeifyDiagram maps to
 * heavy box-drawing glyphs (=;{}[]* — in --ascii mode they are printed as
 * is). CrossUnder cells keep the crossed arc's own glyph and color (the
 * corridor visibly breaks); Anchor cells keep their glyph but get crossing
 * emphasis.
 */
void StampPassOverlay(std::string& diagram, std::vector<HighlightType>& mask,
                      Int n_x, Int n_y,
                      const std::vector<Deco_T::OverlayCell_T>& cells,
                      char marker = '\0')
{
    if (mask.empty())
    {
        mask.assign(static_cast<std::size_t>(n_x * n_y), HighlightType::None);
    }

    // In --mono the corridor is labelled instead of coloured: drop a marker
    // every `marker_period` plain strokes, far enough apart to stay readable
    // and never on an arrow, corner or crossing cell. A corridor too short to
    // reach the period still gets one, at its middle plain cell -- otherwise a
    // short corridor would be indistinguishable from W, which is also heavy.
    constexpr int marker_period = 9;

    std::vector<std::size_t> plain_cells;
    if (marker)
    {
        for (std::size_t k = 0; k < cells.size(); ++k)
        {
            if ((cells[k].kind == Deco_T::OverlayKind::Horizontal)
             || (cells[k].kind == Deco_T::OverlayKind::Vertical))
            {
                plain_cells.push_back(k);
            }
        }
    }

    std::set<std::size_t> marked;
    if (plain_cells.size() >= static_cast<std::size_t>(marker_period))
    {
        for (std::size_t k = marker_period / 2; k < plain_cells.size();
             k += marker_period)
        {
            marked.insert(plain_cells[k]);
        }
    }
    else if (!plain_cells.empty())
    {
        marked.insert(plain_cells[plain_cells.size() / 2]);
    }

    std::size_t cell_index = 0;

    for (const auto& cell : cells)
    {
        const std::size_t this_cell = cell_index++;

        if (cell.x < 0 || cell.x >= n_x - 1 || cell.y < 0 || cell.y >= n_y)
            continue;

        auto idx = static_cast<std::size_t>(
            (n_y - Int(1) - cell.y) * n_x + cell.x);
        if (idx >= diagram.size() || idx >= mask.size()) continue;

        char ch = 0;
        switch (cell.kind)
        {
            case Deco_T::OverlayKind::Horizontal: ch = '='; break;
            case Deco_T::OverlayKind::Vertical:   ch = ';'; break;
            case Deco_T::OverlayKind::CornerNE:   ch = '{'; break;
            case Deco_T::OverlayKind::CornerNW:   ch = '}'; break;
            case Deco_T::OverlayKind::CornerSE:   ch = '['; break;
            case Deco_T::OverlayKind::CornerSW:   ch = ']'; break;
            case Deco_T::OverlayKind::Junction:   ch = '*'; break;
            case Deco_T::OverlayKind::CrossOverH: ch = '='; break;
            case Deco_T::OverlayKind::CrossOverV: ch = ';'; break;
            // Orientation arrows on the corridor. These reuse the diagram's own
            // arrow glyphs: unambiguous in context, since they only ever sit
            // inside a run of corridor strokes (and UnicodeifyDiagram already
            // maps them to the right arrows).
            case Deco_T::OverlayKind::ArrowN:     ch = '^'; break;
            case Deco_T::OverlayKind::ArrowE:     ch = '>'; break;
            case Deco_T::OverlayKind::ArrowS:     ch = 'v'; break;
            case Deco_T::OverlayKind::ArrowW:     ch = '<'; break;
            case Deco_T::OverlayKind::CrossUnder:
                continue;  // arc's glyph and color stay: corridor breaks
            case Deco_T::OverlayKind::Anchor:
                mask[idx] = HighlightType::Crossing;
                continue;  // emphasis only, glyph unchanged
        }

        if (marker && marked.count(this_cell)) { ch = marker; }

        diagram[idx] = ch;
        mask[idx]    = HighlightType::Pass;
    }
}

/**
 * @brief Draw all summands of a knot to stdout.
 *
 * unknot_colors has one entry per bare unknot (0-crossing) summand that
 * accompanied `summands` in the input -- ReadKnot() strips these out of the
 * PD_T vector entirely (there is nothing to build an OrthoDraw diagram from),
 * tracking only their color (PD_T::Uninitialized if unknown -- the plain
 * bare-'s' TSV convention cannot represent one; a real color comes from the
 * PDC-native 'u <color>' input format). In --format=wl mode we still owe the
 * caller one line per summand, trivial or not, so each is emitted as a
 * <|"Unknot"->True|> marker (plus "Component"->color when known) after the
 * drawn summands. Order relative to the non-trivial summands isn't preserved
 * by the parser (nor is it topologically meaningful for a connect-sum/split
 * decomposition).
 */
bool DrawKnot(const std::vector<PD_T>& summands, const Config& config,
              const std::vector<Int>& unknot_colors = {})
{
    OrthoDraw_T::Settings_T settings = BuildSettings(config);

    if (!ValidateCLPSettings(settings)) return false;
    if (!ValidateSettingsCombinations(settings)) return false;

    bool has_labels = config.label_crossings || config.label_arcs
                   || config.label_faces || config.label_components;
    bool has_highlights = !config.highlight_specs.empty() || config.checkerboard_coloring;

    // Pass-move overlay: parse the descriptor once; it is then tried against
    // each summand (darc references only validate on the one it belongs to).
    Deco_T::PassMove_T move;
    bool move_requested = config.move_spec.has_value();
    bool move_applied   = false;
    std::string move_why;

    if (move_requested)
    {
        std::string perr;
        if (!ParsePassMove(*config.move_spec, move, perr))
        {
            std::cerr << "knoodledraw: bad --move descriptor: " << perr << "\n";
            return false;
        }
    }

    for (std::size_t i = 0; i < summands.size(); ++i)
    {
        if (i > 0 && !config.wolfram_mode)
        {
            std::cout << "s\n";
        }

        OrthoDraw_T H(summands[i], config.exterior_face ? *config.exterior_face : Int(-1), settings);

        if (config.wolfram_mode)
        {
            EmitWolframGeometry(H, summands[i], std::cout);
            continue;
        }

        std::string diagram = H.DiagramString();

        // Virtual edges are drawn as '.' by DiagramString().
        // Hide them unless we're in debug mode.
        bool debug_mode = config.quality_preset && *config.quality_preset == "debug";
        if (!debug_mode)
        {
            std::replace(diagram.begin(), diagram.end(), '.', ' ');
        }

        std::vector<HighlightType> mask;
        std::vector<Int> component_map;
        Int n_x = 0, n_y = 0;

        if (has_labels || has_highlights)
        {
            n_x = H.Width()  * config.x_grid_size + 2;
            n_y = H.Height() * config.y_grid_size + 1;

            // Parse highlight specs
            HighlightSet highlights;
            if (!config.highlight_specs.empty())
            {
                highlights = ParseHighlightSpecs(
                    config.highlight_specs,
                    H.MaxArcCount(), H.MaxCrossingCount(), H.FaceCount());
            }

            // Add checkerboard face highlights
            if (config.checkerboard_coloring)
            {
                PD_T pd_copy = summands[i];
                auto coloring = pd_copy.CheckerBoardColoring();
                if (coloring.Dimension(0) > 0)
                {
                    for (Int f = 0; f < coloring.Dimension(0); ++f)
                    {
                        if (coloring[f] == 1)
                        {
                            highlights.faces.insert(f);
                        }
                    }
                }
            }

            // Build face map if needed (for face labels OR face highlights)
            std::vector<Int> face_map;
            if (config.label_faces || !highlights.faces.empty())
            {
                face_map = BuildFaceMap(H, diagram, n_x, n_y);
            }

            // Build highlight mask from pristine diagram (before labels)
            if (!highlights.empty())
            {
                mask = BuildHighlightMask(H, diagram, n_x, n_y,
                                          highlights, face_map);
            }

            // Build component map from pristine diagram (before labels)
            if (config.label_components)
            {
                component_map = BuildComponentMap(
                    H, summands[i], diagram, n_x, n_y);
            }

            // Place labels
            if (has_labels)
            {
                int active_types = int(config.label_crossings)
                                 + int(config.label_arcs)
                                 + int(config.label_faces)
                                 + int(config.label_components);
                bool use_prefix = (active_types > 1);

                // Pass mask+highlights in Unicode mode so labels of
                // highlighted elements get marked directly at placement time.
                std::vector<HighlightType>* m_ptr =
                    (!config.ascii_mode && !mask.empty()) ? &mask : nullptr;
                const HighlightSet* h_ptr =
                    (!config.ascii_mode && !highlights.empty()) ? &highlights : nullptr;

                if (config.label_crossings)
                {
                    PlaceCrossingLabels(diagram, n_x, n_y, H,
                                        use_prefix ? "c" : "",
                                        m_ptr, h_ptr);
                }
                if (config.label_arcs)
                {
                    std::vector<Int>* c_ptr =
                        (!component_map.empty()) ? &component_map : nullptr;
                    PlaceArcLabels(diagram, n_x, n_y, H,
                                   use_prefix ? "a" : "",
                                   m_ptr, h_ptr, c_ptr);
                }
                if (config.label_components && config.ascii_mode)
                {
                    PlaceComponentLabels(diagram, n_x, n_y, H, summands[i]);
                }
                if (config.label_faces)
                {
                    PlaceFaceLabels(diagram, n_x, n_y, H,
                                    use_prefix ? "f" : "", face_map);
                }
            }

            // Apply ASCII highlighting after labels
            if (!mask.empty() && config.ascii_mode)
            {
                ApplyHighlightASCII(diagram, mask);
            }
        }

        // Pass-move overlay: stamped after labels so the corridor wins any
        // cell conflicts. The descriptor is tried on every summand; its darc
        // references only validate on the summand they belong to.
        if (move_requested)
        {
            if (n_x == 0)
            {
                n_x = H.Width()  * config.x_grid_size + 2;
                n_y = H.Height() * config.y_grid_size + 1;
            }

            // Margin lets corridors through the drawn exterior face route
            // around the diagram (see PadCanvas).
            constexpr Int move_margin = 2;

            Deco_T deco(H, move_margin);
            auto pass_route = deco.RoutePassMove(move);
            if (pass_route.validQ)
            {
                // Mark the strand W before padding, while the mask and the
                // drawing still share OrthoDraw's coordinates. W keeps the
                // diagram's own strokes; only its colour (or, in --mono, its
                // weight and a 'w' marker) sets it apart from the corridor.
                {
                    std::vector<Int> strand_arcs;
                    strand_arcs.reserve(move.strand.size());
                    for (Int da : move.strand)
                    {
                        strand_arcs.push_back(PD_T::ArcOfDarc(da));
                    }

                    if (mask.empty())
                    {
                        mask.assign(static_cast<std::size_t>(n_x * n_y),
                                    HighlightType::None);
                    }

                    // --ascii has no colour either, so it gets the markers
                    // too; it just has no heavy strokes to go with them.
                    const bool no_colorQ = config.mono_mode || config.ascii_mode;

                    MarkArcCells(H, diagram, n_x, n_y, strand_arcs, mask,
                                 HighlightType::Strand,
                                 no_colorQ ? 'w' : '\0');
                }

                PadCanvas(diagram, mask, component_map, n_x, n_y, move_margin);
                StampPassOverlay(diagram, mask, n_x, n_y,
                                 deco.RenderPassRoute(pass_route),
                                 (config.mono_mode || config.ascii_mode)
                                     ? 'p' : '\0');
                move_applied = true;
            }
            else
            {
                move_why = pass_route.why;
            }
        }

        if (!config.ascii_mode)
        {
            const std::vector<HighlightType>* m_ptr =
                !mask.empty() ? &mask : nullptr;
            const std::vector<Int>* c_ptr =
                !component_map.empty() ? &component_map : nullptr;

            if (m_ptr || c_ptr)
                diagram = UnicodeifyDiagram(diagram, m_ptr, n_x, c_ptr,
                                            config.mono_mode);
            else
                diagram = UnicodeifyDiagram(diagram);
        }

        std::cout << diagram << "\n";
    }

    if (config.wolfram_mode)
    {
        for (Int color : unknot_colors)
        {
            std::cout << "<|\"Unknot\"->True";
            if (color != PD_T::Uninitialized)
            {
                // `color` here is the raw wire color, so emit it under "Color"
                // too -- this makes the unknot marker agree with arc records on
                // what "Color" means, while "Component" stays for older
                // consumers that keyed off it.
                std::cout << ",\"Component\"->" << color
                          << ",\"Color\"->" << color;
            }
            std::cout << "|>\n";
        }
    }

    // Fail loud if a requested pass-move overlay applied to no summand: a
    // rejected descriptor is exactly what this mode exists to diagnose.
    if (move_requested && !move_applied)
    {
        std::cerr << "knoodledraw: --move descriptor rejected: "
                  << (move_why.empty() ? "no drawable summands" : move_why)
                  << "\n";
        return false;
    }

    return true;
}

//==============================================================================
// Trace Streams (--trace, docs/move-descriptor.md)
//==============================================================================

/**
 * @brief Render a move-trace stream: a sequence of records, each a block of
 * `#`-headed lines followed by a 5-column signed PD snapshot, ended by a
 * blank line.
 *
 * Every header line is echoed above its drawing (the trace is
 * self-captioning). A `#move kind=pass ...` header becomes a corridor
 * overlay on that record's snapshot — the descriptor applies to the diagram
 * in the SAME record, per the spec, so no lookahead is needed. A
 * `#view exterior=<da>` header pins OrthoDraw's exterior face to L(da),
 * resolved via ArcFaces. `#embedding` blocks (redraw witnesses) are skipped
 * with a note; other kinds and `#faces` annotations are echoed unrendered.
 *
 * Fail-loud: malformed records, unresolvable `#view` darcs, and rejected
 * pass descriptors abort with a message and nonzero exit.
 */
bool ProcessTraceStream(std::istream& input, const Config& config)
{
    std::string line;
    std::size_t line_no = 0;
    std::size_t records_drawn = 0;

    std::vector<std::string> headers;
    std::optional<std::string> move_payload;
    std::optional<Int> exterior_da;
    std::vector<Int> code;
    bool in_record = false;

    auto fail = [&](const std::string& msg) -> bool
    {
        std::cerr << "knoodledraw: trace line " << line_no << ": " << msg << "\n";
        return false;
    };

    auto parse_int = [](std::string_view tok, Int& out) -> bool
    {
        std::int64_t v = 0;
        auto [p, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), v);
        if (ec != std::errc{} || p != tok.data() + tok.size()) return false;
        out = static_cast<Int>(v);
        return true;
    };

    // Render and reset the record being accumulated. Returns false on error.
    auto flush = [&]() -> bool
    {
        if (!in_record) return true;

        for (const auto& h : headers) std::cout << h << "\n";

        if (code.empty())
        {
            std::cout << "(0-crossing summand: nothing to draw)\n\n";
        }
        else
        {
            const Int n = static_cast<Int>(code.size()) / Int(5);
            PD_T dia = PD_T::FromSignedPDCode(code.data(), n);

            if (dia.CrossingCount() <= Int(0))
            {
                return fail("PD snapshot did not parse into a valid diagram");
            }

            Config rc = config;

            if (move_payload
                && (move_payload->find("kind=pass") != std::string::npos
                    || move_payload->find("kind=middlepass") != std::string::npos))
            {
                rc.move_spec = *move_payload;
            }

            if (exterior_da)
            {
                const Int da = *exterior_da;
                const Int a  = da / Int(2);
                if (da < 0 || a >= dia.MaxArcCount())
                {
                    return fail("#view exterior darc " + std::to_string(da)
                        + " is out of range");
                }
                // ArcFaces()(a,d) = face left of darc 2a+d
                rc.exterior_face = dia.ArcFaces()(a, da % Int(2));
            }

            std::vector<PD_T> summands;
            summands.push_back(std::move(dia));
            if (!DrawKnot(summands, rc)) return false;
        }

        headers.clear();
        move_payload.reset();
        exterior_da.reset();
        code.clear();
        in_record = false;
        ++records_drawn;
        return true;
    };

    while (std::getline(input, line))
    {
        ++line_no;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
        {
            line.pop_back();
        }

        if (line.empty())
        {
            if (!flush()) return false;
            continue;
        }

        if (line[0] == '#')
        {
            if (line.starts_with("#trace"))
            {
                if (line.find("v=0") == std::string::npos)
                {
                    std::cerr << "knoodledraw: warning: trace version is not"
                                 " v=0; parsing anyway\n";
                }
                continue;  // stream-level header, not part of a record
            }

            in_record = true;

            if (line.starts_with("#embedding"))
            {
                // `#embedding rows=<n>`: a redraw witness. Skip its rows;
                // rendering the lift/rotate/flatten animation is a later
                // backend's job.
                auto pos = line.find("rows=");
                Int rows = 0;
                if (pos == std::string::npos
                    || !parse_int(std::string_view(line).substr(pos + 5), rows)
                    || rows < 0)
                {
                    return fail("bad #embedding header '" + line + "'");
                }
                for (Int r = 0; r < rows; ++r)
                {
                    ++line_no;
                    if (!std::getline(input, line))
                    {
                        return fail("EOF inside #embedding block");
                    }
                }
                headers.push_back("#embedding (" + std::to_string(rows)
                    + " rows, not rendered)");
                continue;
            }

            headers.push_back(line);

            if (line.starts_with("#move "))
            {
                move_payload = line.substr(6);
            }
            else if (line.starts_with("#view "))
            {
                auto pos = line.find("exterior=");
                Int da;
                if (pos == std::string::npos
                    || !parse_int(std::string_view(line).substr(pos + 9), da))
                {
                    return fail("bad #view header '" + line + "'");
                }
                exterior_da = da;
            }
            continue;
        }

        // A 5-column signed PD row.
        {
            in_record = true;
            std::istringstream iss(line);
            std::string tok;
            int count = 0;
            while (iss >> tok)
            {
                Int v;
                if (!parse_int(tok, v))
                {
                    return fail("bad PD entry '" + tok + "'");
                }
                code.push_back(v);
                ++count;
            }
            if (count != 5)
            {
                return fail("PD row has " + std::to_string(count)
                    + " columns, want 5");
            }
        }
    }

    if (!flush()) return false;

    if (records_drawn == 0)
    {
        std::cerr << "knoodledraw: warning: trace stream contained no records\n";
    }
    return true;
}

//==============================================================================
// Stream Processing
//==============================================================================

/**
 * @brief Process a stream, reading knots and drawing each one.
 */
bool ProcessStream(std::istream& input,
                   const std::string& source_name,
                   const Config& config,
                   Knoodle::PRNG_T& rng)
{
    if (config.trace_mode)
    {
        return ProcessTraceStream(input, config);
    }

    bool reached_eof = false;
    bool any_drawn = false;

    while (!reached_eof)
    {
        auto input_knot = ReadKnot(input, config.randomize_projection, rng, source_name, reached_eof);

        if (!input_knot)
        {
            if (reached_eof)
            {
                continue;
            }
            return false;  // Parse error
        }

        if (any_drawn)
        {
            std::cout << "k\n";
        }

        if (!DrawKnot(input_knot->summands, config, input_knot->unknot_colors)) return false;
        any_drawn = true;
    }

    return true;
}

/**
 * @brief Process a .kndlxyz file (multi-component 3D link embedding).
 */
bool ProcessXYZFile(const std::string& filepath, const Config& config)
{
    LinkEmb_T link = LinkEmb_T::ReadFromFile(std::filesystem::path(filepath));

    if (config.randomize_projection)
    {
        // Rotate the whole embedding at once (not each component independently,
        // which would distort the link's actual geometric arrangement) with a
        // proper random rotation -- the same mechanism already used elsewhere
        // (PlanarDiagramComplex/Simplify.hpp: emb.Rotate(reapr.RandomRotation())).
        Reapr_T reapr;
        link.Rotate(reapr.RandomRotation());
    }

    PDC_T pdc(std::move(link));

    if (pdc.DiagramCount() == 0)
    {
        std::cerr << "Error: Failed to create diagram from .kndlxyz file: " << filepath << "\n";
        return false;
    }

    // A trivial (0-crossing, unknot) diagram in the complex must NOT reach
    // OrthoDraw_T's constructor -- unlike the streaming/ReadKnot path (which
    // already separates these into unknot_colors before any diagram is
    // built), pdc.Diagram(i) here can be trivial, and constructing an OrthoDraw
    // from a 0-crossing PlanarDiagram crashes. Filter it out here instead, the
    // same way ReadKnot does -- and since .kndlxyz components are colored
    // automatically (by component order), each trivial diagram's color is
    // recorded rather than discarded.
    std::vector<PD_T> summands;
    std::vector<Int>  unknot_colors;
    for (Int i = 0; i < pdc.DiagramCount(); ++i)
    {
        PD_T pd(pdc.Diagram(i));
        if (pd.CrossingCount() > Int(0))
        {
            summands.push_back(std::move(pd));
        }
        else
        {
            unknot_colors.push_back(pd.FirstColor());
        }
    }

    return DrawKnot(summands, config, unknot_colors);
}

} // anonymous namespace

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[])
{
    // Count the library's "ERROR: " lines for the whole run, so a diagram the
    // core has disclaimed cannot be drawn and reported as a success. knoodledraw
    // writes only to stdout, so unlike knoodlesimplify there is no file to
    // withhold -- the nonzero exit and the notice below are the whole contract.
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

    // knoodledraw writes its drawing to stdout and never to a file, so rule 1
    // (put diagnostics beside the output file) never applies -- it is rule 2
    // every time: a per-process directory under the system temp dir.
    //
    // This tool never calls Simplify, so it cannot produce the library's Rattle
    // bundles. It still needs the directory, because WriteDiagnosticReport reads
    // g_diagnostic_dir and otherwise falls back to the working directory -- which
    // for a Wolfram kernel is the user's notebook directory or home.
    const std::filesystem::path diag_dir =
        ChooseDiagnosticDir("knoodledraw", /*streaming_mode=*/true, std::nullopt);
    const std::set<std::string> bundles_before = ListDiagnosticBundles(diag_dir);

    // Initialize random number generator
    Knoodle::PRNG_T rng = Knoodle::InitializedRandomEngine<Knoodle::PRNG_T>();

    bool success = true;

    if (config.input_files.empty())
    {
        // No input files — read from stdin (Unix filter mode). If stdin is an
        // interactive terminal, say so, so a bare invocation does not look hung.
        if (StdinIsInteractive())
        {
            std::cerr << "knoodledraw: reading diagrams from stdin (Ctrl-D to end). "
                         "Pipe a stream or pass a file; --help for usage.\n";
        }
        success = ProcessStream(std::cin, "stdin", config, rng);
    }
    else
    {
        // Process each input file
        bool first_file = true;

        for (const auto& filename : config.input_files)
        {
            if (!first_file)
            {
                std::cout << "k\n";
            }

            // Route .kndlxyz files to the specialized handler
            std::filesystem::path fpath(filename);
            if (fpath.extension() == ".kndlxyz")
            {
                if (!ProcessXYZFile(filename, config))
                {
                    success = false;
                }
                else
                {
                    first_file = false;
                }
                continue;
            }

            std::ifstream file(filename);
            if (!file)
            {
                std::cerr << "Error: Failed to open input file: " << filename << "\n";
                success = false;
                continue;
            }

            if (!ProcessStream(file, filename, config, rng))
            {
                success = false;
            }
            else
            {
                first_file = false;
            }
        }
    }

    if (ErrorsSeen())
    {
        std::cerr << "\nknoodledraw: " << ErrorSummary()
                  << " during this run -- the drawing above is UNRELIABLE"
                     " (the library discards diagrams it has flagged as invalid).\n";

        std::string invocation;
        for (int i = 0; i < argc; ++i)
        {
            invocation += (i ? " " : "");
            invocation += argv[i];
        }

        const auto report = WriteDiagnosticReport("knoodledraw", {
            { "command line", "  " + invocation + "\n" },
            { "what to send", "  This file, plus the input that produced it.\n" },
        });

        if (!report.empty())
        {
            std::cerr << "Wrote a diagnostic report to " << report.string()
                      << " -- please send it with any bug report.\n";
        }

        FinishDiagnostics(diag_dir, bundles_before, "knoodledraw", true);
        return EXIT_FAILURE;
    }

    FinishDiagnostics(diag_dir, bundles_before, "knoodledraw", !success);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
