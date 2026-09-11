// knoodle_io_check -- unit tests for tools/knoodle_io.hpp, the input parser
// shared by knoodlesimplify, knoodledraw and knoodleidentify.
//
// This is clause 1 of the TOOL contract -- "parse input correctly" -- and it is
// a different thing from testing the library. Nothing here computes a knot
// type. The questions are only: does this text become the diagram it describes,
// and does text that describes nothing get refused rather than quietly becoming
// something else.
//
// No refactor was needed to write this. knoodle_io.hpp is a header and its
// anonymous namespace is per translation unit, so including it here gives this
// file its own copy of everything and every function is directly callable.
//
// Build: `make knoodle_io_check` in test/.

#include "../tools/knoodle_io.hpp"

#include <cmath>
#include <cstdio>
#include <sstream>
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

// ---------------------------------------------------------------------------

static void TestTrim()
{
    section("Trim");
    check(Trim("abc")           == "abc", "Trim leaves a clean string alone");
    check(Trim("  abc  ")       == "abc", "Trim strips spaces both ends");
    check(Trim("\t\r\nabc\n\r\t")== "abc", "Trim strips tabs, CR and LF");
    check(Trim("a b")           == "a b", "Trim does not touch interior spaces");
    check(Trim("")              == "",    "Trim of empty is empty");
    check(Trim("   ")           == "",    "Trim of all-whitespace is empty");
    // CR matters: a file authored on Windows arrives with '\r' at line end, and
    // a surviving '\r' would make the last numeric token unparseable.
    check(Trim("1 2 3\r")       == "1 2 3", "Trim removes a trailing CR (CRLF input)");
}

static void TestCleanLine()
{
    section("CleanLine");
    check(CleanLine("1 2 3")         == "1 2 3", "a plain data line is unchanged");
    check(CleanLine("1 2 3 % note")  == "1 2 3", "'%' starts a comment");
    check(CleanLine("% whole line")  == "",      "a whole-line comment becomes empty");
    check(CleanLine("  % indented")  == "",      "an indented comment becomes empty");
    check(CleanLine("1,2,3")         == "1 2 3", "commas become separators");
    // Each brace and comma becomes ITS OWN space, so "{1, 2, 3}" comes back
    // double-spaced. Harmless -- the tokenizer splits on runs of whitespace --
    // but worth pinning, because it means CleanLine is not a normalizer and
    // string-comparing its output to a canonical form will surprise you.
    check(CleanLine("{1, 2, 3}")     == "1  2  3",
          "braces become separators (and runs of space are not collapsed)");
    check(CleanLine("{{1,2},{3,4}}") == "1 2   3 4",
          "nested braces separate but do not collapse runs of space");
    check(CleanLine("")              == "",      "empty stays empty");
    // '%' inside what would otherwise be data still starts a comment: there is
    // no escaping, which is worth knowing rather than discovering.
    check(CleanLine("1 2%3")         == "1 2",   "'%' needs no surrounding space");
}

static void TestParseNumericLine()
{
    section("ParseNumericLine");
    std::vector<Real> v;
    bool f = false;

    check(ParseNumericLine("1 2 3", v, f) && v.size() == 3 && !f,
          "three integers parse, has_float false");
    check(v.size() == 3 && v[0] == 1.0 && v[2] == 3.0, "values are in order");

    check(ParseNumericLine("1.5 2 3", v, f) && f,
          "a decimal point sets has_float");
    check(ParseNumericLine("-1 -2.5", v, f) && v.size() == 2 && v[0] == -1.0,
          "negatives parse");
    check(ParseNumericLine("", v, f) && v.empty(),
          "an empty line parses to no values (not an error)");
    check(ParseNumericLine("   ", v, f) && v.empty(),
          "whitespace parses to no values");
    check(!ParseNumericLine("abc", v, f),
          "a non-numeric token is an error");
    check(!ParseNumericLine("1 2 oops", v, f),
          "one bad token fails the whole line");

    // TWO LENIENCIES, pinned rather than endorsed. Both follow from using
    // std::stod plus a literal search for '.', and both can silently change how
    // a line is CLASSIFIED, since DetectFormat keys on the value count and on
    // has_float.
    //
    // 1. std::stod parses a leading number and ignores the rest, so "5abc" is
    //    accepted as 5. A typo in a PD code is read as a valid crossing.
    if( ParseNumericLine("5abc", v, f) )
    {
        check(v.size() == 1 && v[0] == 5.0,
              "NOTE: '5abc' is accepted as 5 -- std::stod stops at the first "
              "non-numeric character");
    }
    else
    {
        check(false, "expected '5abc' to be accepted (std::stod leniency); "
                     "if this now fails, the leniency was fixed -- update this");
    }

    // 2. has_float is a search for '.', so exponent notation is not seen as
    //    floating point. "1e5" is a Real but leaves has_float false, and a
    //    five-column line containing one is therefore offered to DetectFormat
    //    as a signed PD CODE rather than rejected.
    ParseNumericLine("1e5 2 3 4 5", v, f);
    check(v.size() == 5 && v[0] == 100000.0,
          "NOTE: '1e5' parses to 100000");
    check(!f, "NOTE: '1e5' does NOT set has_float -- has_float looks for '.' only");
    check(DetectFormat(v, f) == 5,
          "NOTE: consequence -- a line with 1e5 in it is classified as a signed "
          "PD code, not rejected");
}

static void TestDetectFormat()
{
    section("DetectFormat");
    auto ints = [](std::size_t n){ return std::vector<Real>(n, 1.0); };

    check(DetectFormat(ints(3), false) == 3, "3 values -> 3D geometry");
    check(DetectFormat(ints(4), false) == 4, "4 integers -> unsigned PD code");
    check(DetectFormat(ints(5), false) == 5, "5 integers -> signed PD code");
    check(DetectFormat(ints(6), false) == 6, "6 integers -> unsigned PD + colors");
    check(DetectFormat(ints(7), false) == 7, "7 integers -> signed PD + colors");

    // 3D is the only format that may contain floats; every PD shape requires
    // integers, so a decimal point demotes the line to unrecognized.
    check(DetectFormat(ints(3), true) == 3, "3 floats are still 3D geometry");
    check(DetectFormat(ints(4), true) == 0, "4 values with a float are not a PD code");
    check(DetectFormat(ints(5), true) == 0, "5 values with a float are not a PD code");
    check(DetectFormat(ints(6), true) == 0, "6 values with a float are not a PD code");
    check(DetectFormat(ints(7), true) == 0, "7 values with a float are not a PD code");

    check(DetectFormat(ints(0), false) == 0, "0 values is unrecognized");
    check(DetectFormat(ints(1), false) == 0, "1 value is unrecognized");
    check(DetectFormat(ints(2), false) == 0, "2 values is unrecognized");
    check(DetectFormat(ints(8), false) == 0, "8 values is unrecognized");

    // 7 is the widest understood row. This is the trap that cost a day on
    // 2026-08-14: PDCode<signQ=true> is SEVEN columns, not five, and reading
    // seven-column output as five-column silently yields a different diagram.
    check(DetectFormat(ints(9), false) == 0, "9 values is unrecognized");
}

// ---------------------------------------------------------------------------
// ReadKnot: the whole parser, driven from a string.

static std::optional<InputKnot> Read( const std::string & text, bool & eof )
{
    std::istringstream in (text);
    Knoodle::PRNG_T rng (12345);
    return ReadKnot(in, /*randomize_projection=*/false, rng, "test", eof);
}

// A trefoil as a 5-column signed PD code.
static const char * kTrefoil =
    "2 0 3 5 1\n"
    "0 4 1 3 1\n"
    "4 2 5 1 1\n";

static void TestReadKnot()
{
    section("ReadKnot");
    bool eof = false;

    {
        auto k = Read(kTrefoil, eof);
        check(k.has_value(), "a 5-column signed PD code reads");
        if( k )
        {
            check(k->summands.size() == 1, "one summand");
            check(k->input_column_count == 5, "column count reported as 5");
            check(!k->had_3d_geometry, "not flagged as 3D geometry");
            if( !k->summands.empty() )
            {
                check(k->summands[0].CrossingCount() == Int(3), "3 crossings");
            }
        }
    }

    {
        // Blank lines, comments and indentation are noise, not structure.
        std::string noisy =
            "% a leading comment\n"
            "\n"
            "   2 0 3 5 1   % trailing comment\n"
            "\n"
            "0 4 1 3 1\n"
            "4 2 5 1 1\n"
            "\n";
        auto k = Read(noisy, eof);
        check(k.has_value(), "comments, blank lines and indentation are skipped");
        if( k && !k->summands.empty() )
        {
            check(k->summands[0].CrossingCount() == Int(3),
                  "the noisy version yields the same 3 crossings");
        }
    }

    {
        // A bare 's' is how the tools write a 0-crossing summand.
        auto k = Read("s\n", eof);
        check(k.has_value(), "a bare 's' line reads");
        if( k )
        {
            check(k->unknot_colors.size() == 1, "a bare 's' is one unknot summand");
            check(k->summands.empty(), "and contributes no crossing diagram");
        }
    }

    {
        auto k = Read("s\ns\ns\n", eof);
        check(k.has_value() && k->unknot_colors.size() == 3,
              "three bare 's' lines are three unknot summands");
    }

    {
        // 'k' terminates a knot; a leading 'k' is optional and ignored.
        auto k = Read(std::string("k\n") + kTrefoil + "k\n", eof);
        check(k.has_value(), "a leading 'k' is optional");
        if( k ) { check(k->summands.size() == 1, "and does not add a summand"); }
    }

    // --- refusals. Each of these describes nothing coherent, so the parser
    // must decline rather than invent a reading.
    {
        auto k = Read("2 0 3 5 1\n1 2 3 4\n", eof);
        check(!k.has_value(), "mixing 5-column and 4-column PD rows is refused");
    }
    {
        auto k = Read("0 0 0\n1 0 0\n2 0 3 5 1\n", eof);
        check(!k.has_value(), "mixing 3D geometry and a PD code is refused");
    }
    {
        auto k = Read("1 2\n", eof);
        check(!k.has_value(), "a 2-column line is refused (no such format)");
    }
    {
        auto k = Read("1 2 3 4 5 6 7 8 9\n", eof);
        check(!k.has_value(), "a 9-column line is refused");
    }
    {
        auto k = Read("2 0 3 5 1\nnot a number\n", eof);
        check(!k.has_value(), "an unparseable line is refused");
    }
    {
        auto k = Read("1.5 2 3 4 5\n", eof);
        check(!k.has_value(),
              "a 5-column line with a decimal point is refused (PD codes are integral)");
    }

    {
        // Empty input is not an error, it is the end.
        auto k = Read("", eof);
        check(eof, "empty input sets reached_eof");
    }
    {
        auto k = Read("\n\n% just a comment\n\n", eof);
        check(eof, "input with no data sets reached_eof");
    }
}

static void TestReadKnot3D()
{
    section("ReadKnot -- 3D geometry");
    bool eof = false;

    // A trefoil as a closed polygon: 3-column float input is the other half of
    // what these tools accept, and it takes a completely different path
    // (CreateDiagramFrom3D) from the PD-code path above.
    //
    // Sampled from the standard parametrisation rather than written by hand.
    // A hand-placed hexagon put two vertices in degenerate position -- edges
    // meeting corner-to-corner -- and FindIntersections declined it, which
    // would have made this a test of my arithmetic rather than of the parser.
    std::ostringstream t;
    t.precision(10);
    for( int i = 0; i < 24; ++i )
    {
        const double a = 2.0 * M_PI * double(i) / 24.0;
        t << ( std::sin(a) + 2.0 * std::sin(2.0*a) ) << " "
          << ( std::cos(a) - 2.0 * std::cos(2.0*a) ) << " "
          << ( -std::sin(3.0*a) ) << "\n";
    }

    auto k = Read(t.str(), eof);
    check(k.has_value(), "3-column float input reads as 3D geometry");
    if( k )
    {
        check(k->had_3d_geometry, "had_3d_geometry is set");
        check(k->vertex_count_3d == Int(24), "all 24 vertices are counted");
        check(k->input_column_count == 3, "column count reported as 3");
        // The projection is deterministic here (randomize_projection = false),
        // so the standard trefoil's 3 crossings are a fixed expectation.
        check(k->summands.size() == 1 && k->summands[0].CrossingCount() == Int(3),
              "the standard trefoil projects to 3 crossings");
    }

    // Integer 3-column input is geometry too -- DetectFormat keys on the value
    // count before it looks at has_float, so a lattice curve is not mistaken
    // for a PD code. A planar square has no crossings, which is a perfectly
    // good diagram and must not be treated as a failure to parse.
    auto k2 = Read("0 0 0\n1 0 0\n1 1 0\n0 1 0\n", eof);
    check(k2.has_value(), "3-column INTEGER input is also 3D geometry");
    if( k2 )
    {
        check(k2->had_3d_geometry, "and is flagged as such");
        check(k2->vertex_count_3d == Int(4), "4 vertices");
        check(!k2->summands.empty() && k2->summands[0].CrossingCount() == Int(0),
              "a planar square is a valid 0-crossing diagram, not a parse error");
    }
}

int main()
{
    std::printf("knoodle_io_check -- tools/knoodle_io.hpp input parsing\n");

    TestTrim();
    TestCleanLine();
    TestParseNumericLine();
    TestDetectFormat();
    TestReadKnot();
    TestReadKnot3D();

    std::printf("\n%s (%d checks, %d failed)\n",
                fails == 0 ? "KNOODLE IO CHECK OK" : "KNOODLE IO CHECK FAILED",
                checks, fails);
    return fails == 0 ? 0 : 1;
}
