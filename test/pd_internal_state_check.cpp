// Round-trip test for PlanarDiagram's internal-state serialization
// (WriteToOutString / FromInString, and the file pair WriteToFile / FromFile).
//
// What it has to demonstrate, beyond "it parses":
//   1. the text round trip is exact on an ordinary diagram;
//   2. INACTIVE crossing/arc slots and the max_* vs active counts survive --
//      this is the whole reason the format exists, since PD codes drop them;
//   3. the same state survives a trip through an actual file;
//   4. malformed input yields an invalid diagram rather than a crash or a
//      silently empty one.
//
// NOTE ON THE FORMAT. This is a deliberately literal parser: the leading line
// is ClassName() (which carries the Int type, so PD_T::Uninitialized can be
// interpreted), separators are exact, and stray whitespace is a parse error.
// It is a debugging format whose only contract is that the reader and writer
// in the SAME release agree -- hence a round-trip test rather than a test
// against a checked-in golden string.

#include "../Knoodle.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

using Int  = std::int64_t;
using PD_T = Knoodle::PlanarDiagram<Int>;

using CrossingState_T = Knoodle::CrossingState_T;
using ArcState_T      = Knoodle::ArcState_T;

static bool ok = true;

static void check( bool passedQ, const char * what )
{
    std::printf("  %-58s %s\n", what, passedQ ? "OK" : "FAILED");
    if( !passedQ ) { ok = false; }
}

/*!@brief Serialize `pd` to a `std::string` via `WriteToOutString`. */

static std::string Serialize( const PD_T & pd )
{
    Tools::OutString s;

    if( !pd.WriteToOutString(s) ) { return std::string(); }

    return std::string( s.begin(), static_cast<std::size_t>(s.Size()) );
}

/*!@brief Rebuild a diagram from `text` via `FromInString`. */

static PD_T Deserialize( const std::string & text )
{
    Tools::InString s ( text );

    return PD_T::FromInString(s);
}

int main()
{
    // ---- 1. exact round trip on a right-hand trefoil --------------------
    {
        Int code[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
        PD_T pd = PD_T::FromSignedPDCode(&code[0], Int(3), false, false);

        const std::string a   = Serialize(pd);
        PD_T              pd2 = Deserialize(a);
        const std::string b   = Serialize(pd2);

        check(!a.empty(),        "trefoil: WriteToOutString produced output");
        check(a == b,            "trefoil: text round trip is exact");
        check(pd2.CheckAll(),    "trefoil: rebuilt diagram passes CheckAll");
        check(pd2.CrossingCount() == pd.CrossingCount(),
                                 "trefoil: crossing count preserved");
        check(pd2.ArcCount()      == pd.ArcCount(),
                                 "trefoil: arc count preserved");
    }

    // ---- 2. inactive slots and label preservation -----------------------
    // A trefoil living in arrays sized for four crossings: slot 3 and arcs
    // 6, 7 are inactive. A PD code cannot express this at all -- Traverse
    // renumbers and drops the dead slots -- so we build it through the
    // construct-from-internal-data constructor with compressQ = false, which
    // is the only way to keep the padding.
    {
        const Int C_arcs [16] = {
            1, 4,  3, 0,
            3, 0,  5, 2,
            5, 2,  1, 4,
            PD_T::Uninitialized, PD_T::Uninitialized,
            PD_T::Uninitialized, PD_T::Uninitialized
        };

        const CrossingState_T C_state [4] = {
            CrossingState_T::RightHanded,
            CrossingState_T::RightHanded,
            CrossingState_T::RightHanded,
            CrossingState_T::Inactive
        };

        const Int A_cross [16] = {
            1, 0,   0, 2,   2, 1,
            1, 0,   0, 2,   2, 1,
            PD_T::Uninitialized, PD_T::Uninitialized,
            PD_T::Uninitialized, PD_T::Uninitialized
        };

        const ArcState_T A_state [8] = {
            ArcState_T::Active,   ArcState_T::Active,   ArcState_T::Active,
            ArcState_T::Active,   ArcState_T::Active,   ArcState_T::Active,
            ArcState_T::Inactive, ArcState_T::Inactive
        };

        const Int A_color [8] = { 0, 0, 0, 0, 0, 0, -1, -1 };

        PD_T pd (
            Int(4),
            &C_arcs[0],  &C_state[0],
            &A_cross[0], &A_state[0],
            &A_color[0], Int(-1),
            false,      // proven_minimalQ
            false       // compressQ: keep the padding we just built
        );

        check(pd.MaxCrossingCount() == Int(4),
              "padded source: max_crossing_count is 4");
        check(pd.CrossingCount() == Int(3),
              "padded source: active crossing count is 3");

        const std::string a   = Serialize(pd);
        PD_T              pd2 = Deserialize(a);

        check(pd2.MaxCrossingCount() == Int(4),
              "inactive slots: max_crossing_count kept at 4");
        check(pd2.CrossingCount() == Int(3),
              "inactive slots: active crossing count is 3");
        check(pd2.MaxArcCount() == Int(8),
              "inactive slots: max_arc_count kept at 8");
        check(pd2.ArcCount() == Int(6),
              "inactive slots: active arc count is 6");
        check(pd2.CheckAll(), "inactive slots: CheckAll passes");
        check(Serialize(pd2) == a,
              "inactive slots: re-serializes to the identical text");
    }

    // ---- 3. the same state survives a trip through a file ---------------
    {
        Int code[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
        PD_T pd = PD_T::FromSignedPDCode(&code[0], Int(3), false, false);

        const std::filesystem::path file
            = std::filesystem::temp_directory_path()
              / "knoodle_pd_internal_state_check.txt";

        const bool wroteQ = pd.WriteToFile(file);
        check(wroteQ, "file: WriteToFile succeeded");

        if( wroteQ )
        {
            PD_T pd2 = PD_T::FromFile(file);

            check(pd2.CheckAll(),  "file: rebuilt diagram passes CheckAll");
            check(Serialize(pd2) == Serialize(pd),
                  "file: recovers the same state");
        }

        std::error_code ec;
        std::filesystem::remove(file,ec);
    }

    // ---- 4. malformed input is refused, not guessed ---------------------
    // The parser is literal, so each of these fails at a different point:
    // the class-name line, the first field, and a truncated array.
    {
        PD_T bad1 = Deserialize("");
        check(bad1.InvalidQ(), "malformed: empty input -> invalid diagram");

        PD_T bad2 = Deserialize("NotAPlanarDiagram\n");
        check(bad2.InvalidQ(), "malformed: wrong class name -> invalid diagram");

        PD_T bad3 = Deserialize(PD_T::ClassName() + "\n");
        check(bad3.InvalidQ(),
              "malformed: no max_crossing_count -> invalid diagram");

        // A well-formed header whose arrays stop early.
        Int code[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
        PD_T pd = PD_T::FromSignedPDCode(&code[0], Int(3), false, false);

        const std::string full = Serialize(pd);
        PD_T bad4 = Deserialize(full.substr(0, full.size() / 2));
        check(bad4.InvalidQ(), "malformed: truncated body -> invalid diagram");
    }

    std::printf(ok ? "CASE OK\n" : "CASE FAILED\n");
    return ok ? 0 : 1;
}
