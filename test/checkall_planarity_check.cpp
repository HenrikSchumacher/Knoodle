// checkall_planarity_check -- CheckAll passes on an object that is not a
// planar diagram.
//
// THE BLIND SPOT. `PlanarDiagram::CheckAll` is
//
//     CheckAllCrossings && CheckAllArcs && CheckVertexDegrees
//     && CheckArcDegrees && CheckArcColors
//
// Every one of those is a LOCAL incidence check: indices in bounds, arcs
// attached to active crossings are themselves active, `A_cross` and `C_arcs`
// agree with each other, degrees are right, colors are consistent. Nothing in
// it is global. In particular nothing checks that the combinatorial map is
// planar, i.e. that
//
//     V - E + F == 2
//
// and it is perfectly possible for a 4-valent map to be locally impeccable and
// live on a surface of higher genus. The `PassSimplifier::Reroute` arc-label
// aliasing (handoff/reroute-arc-label-aliasing, PR #30, closed) produces
// exactly that: rerouting through a corridor that names an already-recycled
// label rewires the map onto a genus-2 surface. `CheckAll` returns true.
//
// WHY IT MATTERS BEYOND THAT BUG. Once the object is non-planar, "the
// determinant" is not well defined on it, and different ways of reading the
// same object disagree -- we measured 115 from the live `C_arcs` slots and 247
// from that same object's own `PDCode()` traversal. Both are meaningless.
// Reporting "CheckAll PASS" alongside either number invites the reader to
// believe a knot was computed. So this is not only a missing check; it is a
// missing check whose absence makes downstream numbers look trustworthy.
//
// The Euler test is cheap: `FaceCount()` is already computed and cached, so
// this is a subtraction. It is a natural candidate for `CheckAll` itself, or
// for a `CheckPlanarity()` beside it.
//
// The corrupted diagram is REGENERATED here rather than read from a checked-in
// blob. The label-preserving internal-state format is explicitly documented as
// debugging-only with no cross-version guarantee, so a committed fixture in it
// would rot; regenerating keeps the test honest. Pass a path as argv[1] to
// write the internal-state dump for sharing (that IS the right form to hand
// upstream -- a PD-code dump would not do, because `Traverse` rebuilds a
// self-consistent diagram and the corruption would be lost in transit).

#include "../Knoodle.hpp"
#include "pass_oracle.hpp"
#include "pass_fixtures.hpp"

#include <cstdio>
#include <string>
#include <vector>

using Int    = std::int64_t;
using PD_T   = Knoodle::PlanarDiagram<Int>;
using PDC_T  = Knoodle::PlanarDiagramComplex<Int>;
using PS_T   = Knoodle::PassSimplifier<Int>;
using Oracle = Knoodle::PassOracle<Int>;

static bool ok = true;

static void check( bool passedQ, const std::string & what )
{
    std::printf("  %-58s %s\n", what.c_str(), passedQ ? "OK" : "FAILED");
    if( !passedQ ) { ok = false; }
}

static PD_T Fixture()
{
    std::vector<Int> c( zf061098_underpass.begin(), zf061098_underpass.end() );
    return PD_T::FromSignedPDCode(&c[0], static_cast<Int>(c.size()/5), false, false);
}

/*!@brief V - E + F. Two for a sphere; anything else means the map is not
 * planar, whatever the local checks say.
 */
static Int EulerCharacteristic( Knoodle::mref<PD_T> pd )
{
    return pd.CrossingCount() - pd.ArcCount() + pd.FaceCount();
}

/*!@brief Reroute the strand arcs 40..48 along an explicitly supplied corridor.
 * The two corridors below differ in ONE entry.
 */
static PD_T Reroute( Knoodle::cref<std::vector<Int>> raw )
{
    PD_T  pd = Fixture();
    PDC_T pdc { PD_T(pd) };
    PS_T  ps(pdc, Knoodle::DijkstraStrategy_T::Bidirectional);

    Oracle::LoadDiagram(ps, pd);

    typename PS_T::Pass_T pass;
    pass.first     = Int(40);
    pass.last      = Int(48);
    pass.overQ     = false;
    pass.activeQ   = true;
    pass.arc_count = Int(9);

    typename PS_T::Path_T path( static_cast<Int>(raw.size()) + Int(4) );
    for( std::size_t i = 0; i < raw.size(); ++i )
    {
        path[static_cast<Int>(i)] = raw[i];
    }
    path.size = static_cast<Int>(raw.size());

    Oracle::Reroute(ps, pass, path);
    return pd;
}

int main( int argc, char ** argv )
{
    // The corridor's 5th entry is the whole story: 230 names arc 115, which
    // the loop recycles at its second interior crossing; 228 names arc 114,
    // the label 115's span was spliced into. One number apart.
    const std::vector<Int> verbatim { 80,7,42,8,230,131,281,260,290,96 };
    const std::vector<Int> control  { 80,7,42,8,228,131,281,260,290,96 };

    {
        PD_T pd = Fixture();
        std::printf("input:     V=%lld E=%lld F=%lld  V-E+F=%lld\n",
            (long long)pd.CrossingCount(), (long long)pd.ArcCount(),
            (long long)pd.FaceCount(), (long long)EulerCharacteristic(pd));
        check(pd.CheckAll(),                    "input: CheckAll passes");
        check(EulerCharacteristic(pd) == Int(2),"input: is planar (V-E+F = 2)");
    }

    {
        PD_T pd = Reroute(control);
        std::printf("control:   V=%lld E=%lld F=%lld  V-E+F=%lld\n",
            (long long)pd.CrossingCount(), (long long)pd.ArcCount(),
            (long long)pd.FaceCount(), (long long)EulerCharacteristic(pd));
        check(pd.CheckAll(),                     "control: CheckAll passes");
        check(EulerCharacteristic(pd) == Int(2), "control: is planar (V-E+F = 2)");
    }

    {
        PD_T pd = Reroute(verbatim);
        const Int chi = EulerCharacteristic(pd);
        std::printf("corrupted: V=%lld E=%lld F=%lld  V-E+F=%lld\n",
            (long long)pd.CrossingCount(), (long long)pd.ArcCount(),
            (long long)pd.FaceCount(), (long long)chi);

        // The blind spot, stated as two assertions that are both true.
        check(pd.CheckAll(),
              "corrupted: CheckAll PASSES (the blind spot)");
        check(chi != Int(2),
              "corrupted: is NOT planar (V-E+F != 2)");
        check(chi == Int(-2),
              "corrupted: lives on a genus-2 surface (V-E+F = -2)");

        if( argc > 1 )
        {
            Tools::OutString s;
            if( pd.WriteToOutString(s) )
            {
                std::FILE * f = std::fopen(argv[1],"w");
                if( f != nullptr )
                {
                    std::fwrite(s.begin(), 1,
                        static_cast<std::size_t>(s.Size()), f);
                    std::fclose(f);
                    std::printf("\n  wrote the corrupted diagram's internal "
                                "state to %s\n", argv[1]);
                    std::printf("  (label-preserving; a PD-code dump would "
                                "lose the corruption)\n");
                }
            }
        }
    }

    std::printf(ok ? "CASE OK\n" : "CASE FAILED\n");
    return ok ? 0 : 1;
}
