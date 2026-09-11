// simplify_config_check -- knoodlesimplify's flags must reach the library call.
//
// Clause 2 of the tool contract: "be a faithful CLI for the library call".
// Nothing here computes a knot. The only question is whether a command line
// becomes the PDC_T::Simplify_Args_T it claims to.
//
// WHY THIS IS A STRUCT COMPARISON AND NOT AN OUTPUT COMPARISON. The path from
// argv to arguments is two pure functions,
//
//     ParseArguments(argc, argv)  ->  std::optional<Config>
//     BuildSimplifyArgs(config)   ->  PDC_T::Simplify_Args_T
//
// so this file calls them and compares the resulting struct field by field.
// Inferring the call from the tool's output would be strictly weaker: it
// conflates this contract with the library's, it can only see fields somebody
// thought to print, and it cannot see a field that was set when it should not
// have been. The "--local-opt-level=2 must not perturb rerouteQ" class of bug
// is invisible from the outside and obvious from here.
//
// The ladder in `Presets` below is the thing most worth pinning:
// docs/simplify-level-wiring recorded that -s levels 1/2/3 were silently
// no-ops once, and a table is how that stays fixed.
//
// Build: `make simplify_config_check` in test/.

#include "../tools/knoodlesimplify_config.hpp"

#include <cstdio>
#include <string>
#include <vector>

static int fails = 0;
static int checks = 0;

static void check( bool ok, const std::string & what )
{
    ++checks;
    if( ok ) { return; }
    std::printf("  FAILED  %s\n", what.c_str());
    ++fails;
}

static void section( const char * s ) { std::printf("\n=== %s ===\n", s); }

/// Run the real parser on a real argv.
static std::optional<Config> Parse( std::vector<std::string> args )
{
    std::vector<char*> argv;
    std::string prog = "knoodlesimplify";
    argv.push_back(prog.data());
    for( auto & a : args ) { argv.push_back(a.data()); }
    return ParseArguments(static_cast<int>(argv.size()), argv.data());
}

/// argv -> Simplify_Args_T, the whole clause-2 path in one call.
static std::optional<PDC_T::Simplify_Args_T> ArgsOf( std::vector<std::string> a )
{
    auto c = Parse(std::move(a));
    if( !c ) { return std::nullopt; }
    return BuildSimplifyArgs(*c);
}

// ---------------------------------------------------------------------------

static void TestPresetLadder()
{
    section("--simplify-level presets");

    // The ladder, straight from BuildSimplifyArgs. Two regimes that are NOT a
    // superset chain: levels 1-3 are local-only diagnostics, levels 4+ are the
    // production pipeline and deliberately keep local_opt_level = 0 because the
    // local pass does not pay once rerouting is engaged.
    struct Preset { int level; int local; bool reroute; bool disconnect; };
    static const Preset kPresets[] = {
        { 0, 0, false, false },
        { 1, 1, false, false },
        { 2, 2, false, false },
        { 3, 4, false, false },
        { 4, 0, true,  false },
        { 5, 0, true,  true  },
        { 6, 0, true,  true  },
    };

    for( const Preset & p : kPresets )
    {
        const std::string lvl = "-s=" + std::to_string(p.level);
        auto a = ArgsOf({lvl});
        if( !a ) { check(false, lvl + " parses"); continue; }

        check(int(a->local_opt_level) == p.local,
              lvl + " -> local_opt_level=" + std::to_string(p.local)
                  + " (got " + std::to_string(int(a->local_opt_level)) + ")");
        check(a->rerouteQ == p.reroute,
              lvl + " -> rerouteQ=" + (p.reroute ? "true" : "false"));
        check(a->disconnectQ == p.disconnect,
              lvl + " -> disconnectQ=" + (p.disconnect ? "true" : "false"));
    }

    // Level 6 is the Reapr threshold: it is the only preset that asks for
    // embedding trials, and that is what separates it from level 5.
    auto a5 = ArgsOf({"-s=5"});
    auto a6 = ArgsOf({"-s=6"});
    check(a5 && a6 && a6->embedding_trials > a5->embedding_trials,
          "-s=6 requests embedding trials where -s=5 does not (the Reapr threshold)");
    check(a6 && a6->rotation_trials == Knoodle::Size_T(25),
          "-s=6 sets rotation_trials = 25");

    // splitQ is not exposed on the command line and must always be true: it is
    // an internal quality knob, not the --unite/--split output shape.
    for( int lvl = 0; lvl <= 6; ++lvl )
    {
        auto a = ArgsOf({"-s=" + std::to_string(lvl)});
        check(a && a->splitQ, "-s=" + std::to_string(lvl) + " keeps splitQ = true");
    }
}

static void TestOverridesWin()
{
    section("explicit flags override the preset");

    // The has_value() cascade at the end of BuildSimplifyArgs exists so an
    // explicit flag beats the level preset. Each of these picks a level whose
    // preset is the OPPOSITE of the flag, so a passing check cannot be an
    // accident of them agreeing.
    {
        auto a = ArgsOf({"-s=6", "--local-opt-level=3"});
        check(a && int(a->local_opt_level) == 3,
              "--local-opt-level=3 beats -s=6's preset of 0");
    }
    {
        auto a = ArgsOf({"-s=6", "--no-reroute"});
        check(a && !a->rerouteQ, "--no-reroute beats -s=6's preset of true");
    }
    {
        auto a = ArgsOf({"-s=0", "--reroute"});
        check(a && a->rerouteQ, "--reroute beats -s=0's preset of false");
    }
    {
        auto a = ArgsOf({"-s=6", "--no-disconnect"});
        check(a && !a->disconnectQ, "--no-disconnect beats -s=6's preset of true");
    }
    {
        auto a = ArgsOf({"-s=0", "--disconnect"});
        check(a && a->disconnectQ, "--disconnect beats -s=0's preset of false");
    }
    {
        auto a = ArgsOf({"-s=6", "--reapr-rotation-trials=7"});
        check(a && a->rotation_trials == Knoodle::Size_T(7),
              "--reapr-rotation-trials=7 beats -s=6's preset of 25");
    }
}

static void TestNoCollateralDamage()
{
    section("a flag changes ONLY its own field");

    // The bug this catches is invisible from outside the process: a flag that
    // also perturbs a field it has no business touching. Compare the whole
    // struct against the bare preset and require exactly one field to move.
    auto base = ArgsOf({"-s=0"});
    if( !base ) { check(false, "-s=0 parses"); return; }

    auto same_except = [&](const std::string & flag,
                           const std::string & moved,
                           auto && differs) -> void
    {
        auto a = ArgsOf({"-s=0", flag});
        if( !a ) { check(false, flag + " parses"); return; }

        check(differs(*a), flag + " changes " + moved);

        // Everything else must be untouched.
        if( moved != "local_opt_level" )
        { check(a->local_opt_level == base->local_opt_level, flag + " leaves local_opt_level alone"); }
        if( moved != "rerouteQ" )
        { check(a->rerouteQ == base->rerouteQ, flag + " leaves rerouteQ alone"); }
        if( moved != "disconnectQ" )
        { check(a->disconnectQ == base->disconnectQ, flag + " leaves disconnectQ alone"); }
        if( moved != "compressQ" )
        { check(a->compressQ == base->compressQ, flag + " leaves compressQ alone"); }
        if( moved != "compression_threshold" )
        { check(a->compression_threshold == base->compression_threshold, flag + " leaves compression_threshold alone"); }
        if( moved != "rotation_trials" )
        { check(a->rotation_trials == base->rotation_trials, flag + " leaves rotation_trials alone"); }
        if( moved != "embedding_trials" )
        { check(a->embedding_trials == base->embedding_trials, flag + " leaves embedding_trials alone"); }
        if( moved != "strategy" )
        { check(a->strategy == base->strategy, flag + " leaves strategy alone"); }
        if( moved != "canonicalizeQ" )
        { check(a->canonicalizeQ == base->canonicalizeQ, flag + " leaves canonicalizeQ alone"); }
        check(a->splitQ == base->splitQ, flag + " leaves splitQ alone");
    };

    same_except("--local-opt-level=2", "local_opt_level",
                [&](auto & a){ return int(a.local_opt_level) == 2; });
    same_except("--reroute", "rerouteQ",
                [&](auto & a){ return a.rerouteQ; });
    same_except("--disconnect", "disconnectQ",
                [&](auto & a){ return a.disconnectQ; });
    same_except("--compression-threshold=9", "compression_threshold",
                [&](auto & a){ return a.compression_threshold == Int(9); });
    same_except("--reapr-rotation-trials=3", "rotation_trials",
                [&](auto & a){ return a.rotation_trials == Knoodle::Size_T(3); });
    same_except("--canonicalize", "canonicalizeQ",
                [&](auto & a){ return a.canonicalizeQ; });
    same_except("--dijkstra-strategy=bidirectional", "strategy",
                [&](auto & a){ return a.strategy == Knoodle::DijkstraStrategy_T::Bidirectional; });
}

static void TestEnumsAndValues()
{
    section("enum and value flags");

    struct S { const char * name; Knoodle::DijkstraStrategy_T value; };
    static const S kStrategies[] = {
        { "unidirectional", Knoodle::DijkstraStrategy_T::Unidirectional },
        { "alternating",    Knoodle::DijkstraStrategy_T::Alternating    },
        { "bidirectional",  Knoodle::DijkstraStrategy_T::Bidirectional  },
    };
    for( const S & s : kStrategies )
    {
        auto a = ArgsOf({std::string("--dijkstra-strategy=") + s.name});
        check(a && a->strategy == s.value,
              std::string("--dijkstra-strategy=") + s.name + " reaches args.strategy");
    }

    // Case-insensitive: ToLower runs before the comparison.
    {
        auto a = ArgsOf({"--dijkstra-strategy=BIDIRECTIONAL"});
        check(a && a->strategy == Knoodle::DijkstraStrategy_T::Bidirectional,
              "--dijkstra-strategy is case-insensitive");
    }

    {
        auto a = ArgsOf({"--start-max-dist=11", "--final-max-dist=13"});
        check(a && a->start_max_dist == Int(11), "--start-max-dist reaches args");
        check(a && a->final_max_dist == Int(13), "--final-max-dist reaches args");
    }
    {
        auto a = ArgsOf({"--reapr-scaling=2.5"});
        check(a && double(a->scaling) == 2.5, "--reapr-scaling reaches args.scaling");
    }
    {
        auto a = ArgsOf({"--randomize-bends=3"});
        check(a && a->randomize_bends == 3, "--randomize-bends reaches args");
    }
    {
        auto a = ArgsOf({"--no-compaction"});
        check(a && a->compaction_method == PDC_T::Compaction_T::Unknown,
              "--no-compaction sets compaction_method = Unknown");
    }
}

static void TestRejections()
{
    section("bad input is refused, not clamped");

    // Refusal matters as much as acceptance: a value silently clamped into
    // range is a command line that did something other than what it said.
    check(!Parse({"--local-opt-level=7"}), "--local-opt-level=7 is refused (range is 0-4)");
    check(!Parse({"--local-opt-level=-1"}), "--local-opt-level=-1 is refused");
    check(!Parse({"--local-opt-level=abc"}), "--local-opt-level=abc is refused");
    check(!Parse({"--dijkstra-strategy=sideways"}), "an unknown dijkstra strategy is refused");
    check(!Parse({"--reapr-energy=nonsense"}), "an unknown reapr energy is refused");
    check(!Parse({"--simplify-level=notanumber"}), "a non-numeric simplify level is refused");

    // And the boundary is where it says it is.
    check(Parse({"--local-opt-level=0"}).has_value(), "--local-opt-level=0 is accepted");
    check(Parse({"--local-opt-level=4"}).has_value(), "--local-opt-level=4 is accepted");
}

static void TestConfigLevel()
{
    section("parsing into Config (not yet the library call)");

    {
        auto c = Parse({"--help"});
        check(c && c->help_requested, "--help sets help_requested");
    }
    {
        auto c = Parse({"--streaming-mode"});
        check(c && c->streaming_mode, "--streaming-mode is recorded");
    }
    {
        auto c = Parse({"--unite"});
        check(c && c->unite, "--unite is recorded");
    }
    {
        auto c = Parse({"--format=pdc"});
        check(c && c->pdc_format, "--format=pdc is recorded");
    }
    {
        auto c = Parse({"a.tsv", "b.tsv"});
        check(c && c->input_files.size() == 2, "positional arguments are input files");
    }
    {
        // The default matters: it is what a user gets with no flags at all.
        auto c = Parse({});
        check(c && c->simplify_level == 6, "the default simplify level is 6");
    }
}

int main()
{
    std::printf("simplify_config_check -- knoodlesimplify argv -> Simplify_Args_T\n");

    TestPresetLadder();
    TestOverridesWin();
    TestNoCollateralDamage();
    TestEnumsAndValues();
    TestRejections();
    TestConfigLevel();

    std::printf("\n%s (%d checks, %d failed)\n",
                fails == 0 ? "SIMPLIFY CONFIG CHECK OK" : "SIMPLIFY CONFIG CHECK FAILED",
                checks, fails);
    return fails == 0 ? 0 : 1;
}
