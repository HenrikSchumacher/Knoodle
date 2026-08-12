// Round-trip test for PlanarDiagram's internal-state serialization
// (WriteToOutString / FromInString).
//
// What it has to demonstrate, beyond "it parses":
//   1. the text round trip is exact on an ordinary diagram;
//   2. INACTIVE crossing/arc slots and the max_* vs active counts survive --
//      this is the whole reason the format exists, since PD codes drop them;
//   3. a dump lifted out of a log file parses unedited, surrounding log noise
//      and the extra CacheKeys() field included;
//   4. malformed input yields an invalid diagram rather than a crash or a
//      silently empty one.

#include "../Knoodle.hpp"

#include <cstdio>
#include <string>

using Int  = std::int64_t;
using PD_T = Knoodle::PlanarDiagram<Int>;

static bool ok = true;

static void check( bool passedQ, const char * what )
{
    std::printf("  %-58s %s\n", what, passedQ ? "OK" : "FAILED");
    if( !passedQ ) { ok = false; }
}

int main()
{
    // ---- 1. exact round trip on a right-hand trefoil --------------------
    {
        Int code[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
        PD_T pd = PD_T::FromSignedPDCode(&code[0], Int(3), false, false);

        const std::string a  = pd.ToInternalStateString();
        PD_T              pd2 = PD_T::FromInternalStateString(a);
        const std::string b  = pd2.ToInternalStateString();

        check(a == b,            "trefoil: text round trip is exact");
        check(pd2.CheckAll(),    "trefoil: rebuilt diagram passes CheckAll");
        check(pd2.CrossingCount() == pd.CrossingCount(),
                                 "trefoil: crossing count preserved");
        check(pd2.ArcCount()      == pd.ArcCount(),
                                 "trefoil: arc count preserved");
    }

    // ---- 2. inactive slots and label preservation -----------------------
    // A trefoil living in arrays sized for four crossings: slot 3 and arcs
    // 6, 7 are inactive. A PD code cannot express this at all; the point of
    // the format is that it comes back unchanged.
    {
        const std::string padded =
            "PlanarDiagram\n"
            "max_crossing_count = 4\n"
            "crossing_count = 3\n"
            "max_arc_count = 8\n"
            "arc_count = 6\n"
            "C_arcs = {\n"
            " { { 1, 4 }, { 3, 0 } },\n"
            " { { 3, 0 }, { 5, 2 } },\n"
            " { { 5, 2 }, { 1, 4 } },\n"
            " { { -1, -1 }, { -1, -1 } }\n"
            "}\n"
            "C_state = { RightHanded, RightHanded, RightHanded, Inactive }\n"
            "A_cross = {\n"
            " { 1, 0 },\n { 0, 2 },\n { 2, 1 },\n"
            " { 1, 0 },\n { 0, 2 },\n { 2, 1 },\n"
            " { -1, -1 },\n { -1, -1 }\n"
            "}\n"
            "A_state = { Active, Active, Active, Active, Active, Active,"
            " Inactive, Inactive }\n"
            "A_color = { 0, 0, 0, 0, 0, 0, -1, -1 }\n"
            "last_color_deactivated = -1\n"
            "proven_minimalQ = 0\n";

        PD_T pd = PD_T::FromInternalStateString(padded);

        check(pd.MaxCrossingCount() == Int(4),
              "inactive slots: max_crossing_count kept at 4");
        check(pd.CrossingCount() == Int(3),
              "inactive slots: active crossing count is 3");
        check(pd.MaxArcCount() == Int(8),
              "inactive slots: max_arc_count kept at 8");
        check(pd.ArcCount() == Int(6),
              "inactive slots: active arc count is 6");
        check(pd.CheckAll(), "inactive slots: CheckAll passes");
        check(pd.ToInternalStateString() == padded,
              "inactive slots: re-serializes to the identical text");
    }

    // ---- 3. tolerance: a dump lifted out of a log file -------------------
    {
        Int code[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
        PD_T pd = PD_T::FromSignedPDCode(&code[0], Int(3), false, false);

        const std::string body = pd.ToInternalStateString();

        std::string logged =
            "PlanarDiagram<I64>::PrintInfo -- begin\n"
            + body +
            "this->CacheKeys() = { ArcLeftDarcs, DiagramComponents }\n"
            "PlanarDiagram<I64>::PrintInfo -- end\n";

        PD_T pd2 = PD_T::FromInternalStateString(logged);

        check(pd2.CheckAll(), "log dump: parses with surrounding log lines");
        check(pd2.ToInternalStateString() == body,
              "log dump: recovers the same state");
    }

    // ---- 4. malformed input is refused, not guessed ---------------------
    {
        PD_T bad1 = PD_T::FromInternalStateString("PlanarDiagram\n");
        check(bad1.InvalidQ(), "malformed: empty body -> invalid diagram");

        PD_T bad2 = PD_T::FromInternalStateString(
            "max_crossing_count = 3\nmax_arc_count = 5\n");
        check(bad2.InvalidQ(),
              "malformed: max_arc_count != 2 * max_crossing_count -> invalid");

        PD_T bad3 = PD_T::FromInternalStateString(
            "max_crossing_count = 1\nmax_arc_count = 2\n"
            "C_arcs = { { { 1, 0 }, { 1, 0 } } }\n"
            "C_state = { Sideways }\n"
            "A_cross = { { 0, 0 }, { 0, 0 } }\n"
            "A_state = { Active, Active }\n"
            "A_color = { 0, 0 }\n");
        check(bad3.InvalidQ(), "malformed: unknown crossing state -> invalid");
    }

    std::printf(ok ? "CASE OK\n" : "CASE FAILED\n");
    return ok ? 0 : 1;
}
