// link_color_roundtrip — regression guard for LinkEmbedding's colored .kndlxyz
// round trip.
//
// LinkEmbedding::WriteToFile( ..., colorQ = true ) emits a "#color <int>" header
// before each component, but FromInString used to read three Reals per line and
// nothing else -- so it choked on the leading '#' and rejected every file the
// writer produced under colorQ. Writing a file that your own reader refuses is
// the bug this guards against.
//
// Checks, on a 2-component embedding with non-default colors:
//   1. colorQ = true  round-trips vertex counts, component count, AND colors.
//   2. colorQ = false still round-trips geometry, with colors defaulting to the
//      component index (the historical behavior, unchanged).
//   3. Unknown '#' lines are skipped as comments.
// Exit 0 = all good.
#include "../Knoodle.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using Int    = std::int64_t;
using Real   = double;
using Link_T = Knoodle::LinkEmbedding<Real, Int, float>;

namespace
{

int failures = 0;

void check(bool ok, const std::string& what)
{
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << "\n";
    if (!ok) { ++failures; }
}

// Two disjoint triangles: 6 vertices, 2 components, deliberately non-default
// colors (7 and 3) so a reader that quietly falls back to iota is caught.
constexpr Int kColor0 = 7;
constexpr Int kColor1 = 3;

Link_T MakeTwoComponentLink()
{
    Knoodle::Tensor1<Int,Int> ptr    ( Int(3) );
    Knoodle::Tensor1<Int,Int> colors ( Int(2) );

    ptr[0] = 0; ptr[1] = 3; ptr[2] = 6;
    colors[0] = kColor0; colors[1] = kColor1;

    Link_T link ( std::move(ptr), std::move(colors) );

    const Real coords[18] = {
        0.0, 0.0, 0.0,   1.0, 0.0, 0.0,   0.0, 1.0, 0.0,      // component 0
        5.0, 5.0, 1.0,   6.0, 5.0, 1.0,   5.0, 6.0, 1.0,      // component 1
    };

    link.template ReadVertexCoordinates<false,true>( &coords[0] );

    return link;
}

} // namespace

int main()
{
    const std::filesystem::path dir = std::filesystem::temp_directory_path();
    const std::filesystem::path colored   = dir / "knoodle_link_color_roundtrip_c.kndlxyz";
    const std::filesystem::path uncolored = dir / "knoodle_link_color_roundtrip_u.kndlxyz";
    const std::filesystem::path commented = dir / "knoodle_link_color_roundtrip_x.kndlxyz";
    const std::filesystem::path blanks    = dir / "knoodle_link_color_roundtrip_b.kndlxyz";
    const std::filesystem::path mixed     = dir / "knoodle_link_color_roundtrip_m.kndlxyz";

    Link_T src = MakeTwoComponentLink();

    std::cout << "=== colorQ = true (the regression) ===\n";
    {
        check(src.WriteToFile(colored, true), "WriteToFile(colorQ=true) succeeded");

        Link_T back = Link_T::FromFile(colored);

        check(back.ValidQ(), "colored file reads back as a valid object");
        check(back.ComponentCount() == Int(2), "component count round-trips (2)");
        check(back.EdgeCount() == Int(6), "vertex count round-trips (6)");

        if (back.ValidQ() && (back.ComponentCount() == Int(2)))
        {
            const auto& c = back.ComponentColors();
            check(c[0] == kColor0 && c[1] == kColor1,
                  "colors round-trip (" + std::to_string(kColor0) + ","
                                        + std::to_string(kColor1) + ")");
        }
        else { check(false, "colors round-trip"); }
    }

    std::cout << "=== colorQ = false (historical default preserved) ===\n";
    {
        check(src.WriteToFile(uncolored, false), "WriteToFile(colorQ=false) succeeded");

        Link_T back = Link_T::FromFile(uncolored);

        check(back.ValidQ(), "uncolored file reads back as a valid object");
        check(back.ComponentCount() == Int(2), "component count round-trips (2)");

        if (back.ValidQ() && (back.ComponentCount() == Int(2)))
        {
            const auto& c = back.ComponentColors();
            check(c[0] == Int(0) && c[1] == Int(1),
                  "colors default to the component index");
        }
        else { check(false, "colors default to the component index"); }
    }

    std::cout << "=== '#' comment lines are skipped ===\n";
    {
        {
            std::ofstream f (commented);
            f << "# a hand-written note\n"
              << "#color 42\n"
              << "0 0 0\n1 0 0\n0 1 0\n";
        }

        Link_T back = Link_T::FromFile(commented);

        check(back.ValidQ(), "commented file reads back as a valid object");
        check(back.EdgeCount() == Int(3), "vertex count is 3 (comments not counted)");

        if (back.ValidQ() && (back.ComponentCount() >= Int(1)))
        {
            check(back.ComponentColors()[0] == Int(42), "'#color 42' is honored");
        }
        else { check(false, "'#color 42' is honored"); }
    }

    // A blank line ends a component. Two in a row, or one at either end of the
    // file, used to end a component that had not started -- manufacturing an
    // empty phantom component the user cannot see anywhere in the file. Easy to
    // trip by hand, and invisible until something downstream counts components.
    std::cout << "=== stray blank lines do not manufacture components ===\n";
    {
        const std::string c0  = "0 0 0\n1 0 0\n0 1 0";
        const std::string c1  = "5 5 1\n6 5 1\n5 6 1";
        const std::string two = c0 + "\n\n" + c1;

        auto read_back = [&](const std::string& body)
        {
            { std::ofstream f (blanks); f << body; }
            return Link_T::ReadFromFile(blanks);
        };

        for (const auto& [body, what] : {
                 std::pair{two,                std::string("no stray blank lines")},
                 std::pair{two + "\n",         std::string("trailing newline")},
                 std::pair{two + "\n\n",       std::string("trailing blank line")},
                 std::pair{"\n" + two,         std::string("leading blank line")},
                 std::pair{c0 + "\n\n\n" + c1, std::string("two blank lines between components")},
             })
        {
            Link_T back = read_back(body);
            check(back.ValidQ() && (back.ComponentCount() == Int(2))
                                && (back.EdgeCount() == Int(6)),
                  what + " -> 2 components, 6 vertices");
        }
    }

    // A '#color' tag on some components but not others is malformed. An untagged
    // component takes its index as its color, which can silently collide with a
    // color declared for another component -- an untagged component 0 beside a
    // '#color 0' component 1 used to yield the colors `0 0`. There is no reading
    // of such a file that is obviously the intended one, so it is refused.
    std::cout << "=== mixed '#color' tagging is rejected ===\n";
    {
        // Exactly the layout WriteToFile emits: coordinate lines joined by '\n',
        // components separated by a blank line.
        const std::string c0  = "0 0 0\n1 0 0\n0 1 0";
        const std::string c1  = "5 5 1\n6 5 1\n5 6 1";
        const std::string sep = "\n\n";

        auto read_back = [&](const std::string& body)
        {
            { std::ofstream f (mixed); f << body; }
            return Link_T::ReadFromFile(mixed);
        };

        check(!read_back(c0 + sep + "#color 0\n" + c1).ValidQ(),
              "first component untagged, second tagged -> rejected");

        check(!read_back("#color 0\n" + c0 + sep + c1).ValidQ(),
              "first component tagged, second untagged -> rejected");

        // The two uniform cases stay legal.
        check(read_back("#color 7\n" + c0 + sep + "#color 3\n" + c1).ValidQ(),
              "all components tagged -> accepted");

        check(read_back(c0 + sep + c1).ValidQ(),
              "no component tagged -> accepted");

        // A lone component is uniform either way.
        check(read_back("#color 9\n" + c0).ValidQ(), "single tagged component -> accepted");
        check(read_back(c0).ValidQ(),                "single untagged component -> accepted");
    }

    for (const auto& p : {colored, uncolored, commented, blanks, mixed})
    {
        std::error_code ec;
        std::filesystem::remove(p, ec);
    }

    std::cout << (failures ? "\nFAILED\n" : "\nAll link color round-trip checks passed.\n");

    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
