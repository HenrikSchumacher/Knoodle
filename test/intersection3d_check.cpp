// intersection3d_check -- the introspection contract, from the failing side.
//
// LinkEmbedding2/3/4 expose IntersectionCount3D(), RoundingError() and
// RelativeRoundingError(). Everything else in this suite feeds them curves that
// ARE embeddings, so those accessors are only ever exercised on their trivial
// answers. This test does the opposite: it feeds curves that genuinely
// self-intersect in 3-space and requires the count to be right.
//
// A test designed to fail, in JHC's phrase. The negative half -- "a projection
// that reports success must report zero 3-space intersections" -- lives in
// `RequireIntersectionsOK` in embedding_fixtures.hpp, so it is enforced on
// every fixture, class, coordinate type and tier of embedding_check rather than
// only here.
//
// THE ORACLE IS INDEPENDENT. Each curve's expected count comes from
// `kt::TakeCensus`, which classifies every edge pair in exact integer
// arithmetic with no help from the classes under test. So this compares two
// implementations rather than checking the library against itself, and a
// disagreement names which one to distrust: the census has no floating point in
// it at all.
//
// Build: `make intersection3d_check` in test/.

#include "embedding_fixtures.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace kt = knoodle_test;

using Int = std::int64_t;

static bool ok = true;

/// FIXED 2026-08-15 in b53a9ecb, "Fixed a bug in Prosector classes that allowed
/// 3D intersections to go unnoticed" -- this test reported the 24 XPASS that
/// said so. Flag kept, set false, because it is the switch to flip if the
/// contract ever regresses.
///
/// It was: detecting a transversal 3-space intersection failed in two different
/// ways depending on the backend, so the whole contract was a known failure. Recorded as XFAIL rather than left red, and
/// reported as XPASS if it starts working -- that is the signal to delete this.
/// See GitHub issue for the analysis; in short, Prosector's IntersectionType()
/// does not return Flag_T::Error for a genuine 3D crossing, which Prosector3
/// and Prosector4 detect internally and throw over, while Prosector2 carries on
/// and builds a diagram for a curve that has none.
static bool kDetection3DIsBroken = false;

static void check( bool passedQ, const std::string & what, bool known_broken = false )
{
    if( known_broken )
    {
        std::printf("  %-66s %s\n", what.c_str(), passedQ ? "XPASS" : "XFAIL");
        if( passedQ )
        {
            std::printf("        ^ this is fixed now; drop kDetection3DIsBroken\n");
            ok = false;
        }
        return;
    }
    std::printf("  %-66s %s\n", what.c_str(), passedQ ? "OK" : "FAILED");
    if( !passedQ ) { ok = false; }
}

static kt::Curve MakeCurve( const std::string & name,
                            const std::vector<std::vector<std::array<Int,3>>> & comps )
{
    kt::Curve c;
    c.name = name;
    c.comp_ptr.push_back(0);
    Int n = 0;
    for( const auto & comp : comps )
    {
        for( const auto & p : comp )
        {
            c.v.push_back(double(p[0]));
            c.v.push_back(double(p[1]));
            c.v.push_back(double(p[2]));
            ++n;
        }
        c.comp_ptr.push_back(n);
    }
    return c;
}

/// Run one class on one curve and report what it says about itself.
template<class LE_T>
static void Probe( const kt::Curve & c, const char * label, Int expected3D,
                   bool refuses_correctlyQ )
{
    kt::RunResult r = kt::RunEmbedding<LE_T>(c,/*want_pd=*/false);

    const std::string base = c.name + " " + label;

    // A curve that self-intersects in 3-space is not an embedding, so the class
    // must refuse. Succeeding here would be the serious failure: it would mean
    // a diagram was built for a curve that has no diagram.
    // All classes refuse since b53a9ecb. Before that, Prosector3/4 refused by
    // throwing out of their own consistency check while Prosector2 succeeded and
    // handed back a diagram for a curve that has none, so the two had to be
    // marked differently. The parameter is kept because that distinction is the
    // one worth noticing again if it comes back.
    check(!r.ok, base + ": refuses a curve that is not an embedding",
          !refuses_correctlyQ);

    // And the refusal has to be backed by the count. This is the part that
    // would silently rot: the error code says "not an embedding", the accessor
    // is what says how badly.
    kt::Curve copy = c;
    std::vector<typename LE_T::Real> coords(copy.v.size());
    for( std::size_t i = 0; i < copy.v.size(); ++i )
    { coords[i] = static_cast<typename LE_T::Real>(copy.v[i]); }

    Knoodle::Tensor1<Int,Int> cp ( copy.comp_ptr.data(), Int(copy.comp_ptr.size()) );
    Knoodle::Tensor1<Int,Int> col = Knoodle::iota<Int,Int>(copy.ComponentCount());
    LE_T L ( std::move(cp), std::move(col) );
    L.template ReadVertexCoordinates<false>( coords.data() );

    // Prosector3 THROWS on some of these inputs rather than returning, so the
    // accessor has to be reached through a guard or one case takes the run down.
    Int got = -1;
    try
    {
        (void)L.RequireIntersections();
        got = Int(L.IntersectionCount3D());
    }
    catch( const std::exception & e )
    {
        std::printf("      (threw: %.110s...)\n", e.what());
        check(false, base + ": threw instead of reporting a 3D intersection",
              kDetection3DIsBroken);
        return;
    }

    check(got == expected3D,
          base + ": IntersectionCount3D() = " + std::to_string(got)
               + ", census says " + std::to_string(expected3D),
          kDetection3DIsBroken);

    // Integral input takes the unscaled path, so it must be exact.
    if( c.IntegralQ() && std::is_integral_v<typename LE_T::Real> )
    {
        check(double(L.RoundingError()) == 0.0,
              base + ": RoundingError() is 0 on integral input");
    }
}

int main()
{
    // Every curve below is built to self-intersect in 3-space, and every one
    // is planar-ish and small enough to check by hand against the census.
    struct Case { kt::Curve curve; std::string why; };
    std::vector<Case> cases;

    // One crossing: a quadrilateral traversed so that two opposite edges cross.
    // (0,0)->(4,4) and (4,0)->(0,4) meet at (2,2), both at z = 0.
    cases.push_back({ MakeCurve("one_X",
        {{ {{0,0,0}}, {{4,4,0}}, {{4,0,0}}, {{0,4,0}} }}),
        "two non-adjacent edges of one component cross at (2,2,0)" });

    // The same shape twice, far apart: the count has to scale, not saturate.
    cases.push_back({ MakeCurve("two_X",
        {{ {{0,0,0}}, {{4,4,0}}, {{4,0,0}}, {{0,4,0}} },
         { {{100,0,0}}, {{104,4,0}}, {{104,0,0}}, {{100,4,0}} }}),
        "two components, one self-crossing each" });

    // Two components piercing each other: a square in z = 0 and a triangle
    // whose first edge runs along x = 5 in the same plane, crossing two of the
    // square's edges.
    cases.push_back({ MakeCurve("pierced_square",
        {{ {{0,0,0}}, {{10,0,0}}, {{10,10,0}}, {{0,10,0}} },
         { {{5,-5,0}}, {{5,15,0}}, {{20,5,7}} }}),
        "an edge of one component crosses two edges of another" });

    // The same single crossing, but in a curve that is NOT planar: only the two
    // crossing edges lie in z = 0, the detour between them goes to z = 9. Any
    // two segments that intersect are necessarily coplanar WITH EACH OTHER, so
    // this separates "the pair is coplanar" (inherent to any 3D intersection)
    // from "the whole curve is flat" (an artifact of one_X).
    cases.push_back({ MakeCurve("nonplanar_X",
        {{ {{0,0,0}}, {{4,4,0}}, {{8,8,9}}, {{8,-4,9}}, {{4,0,0}}, {{0,4,0}} }}),
        "one transversal crossing at (2,2,0) in an otherwise non-planar curve" });

    // DELIBERATELY ABSENT: three concurrent coplanar segments (the non-embedded
    // twin of deg_triple_point). The census scores it at spatial = 6, but it
    // reaches upstream issue 11 and dies on an assert inside LinesColinearTest,
    // which no guard here can catch. It belongs in this file once that is
    // fixed; leaving it in would just make the whole test unrunnable.

    for( auto & c : cases )
    {
        const kt::Census cen = kt::TakeCensus(c.curve);

        std::printf("\n=== %s: %s ===\n", c.curve.name.c_str(), c.why.c_str());
        std::printf("  census (exact integer, independent): spatial = %lld\n",
                    (long long)cen.spatial);

        // The fixture is only useful if it really does self-intersect.
        check(cen.spatial > Int(0),
              c.curve.name + ": the census confirms this curve is not an embedding");
        if( cen.spatial <= Int(0) ) { continue; }

        Probe<Knoodle::LinkEmbedding2<double,Int,Int>>(c.curve,"LinkEmbedding2<f64>",cen.spatial,true);
        Probe<Knoodle::LinkEmbedding2<Int,   Int,Int>>(c.curve,"LinkEmbedding2<i64>",cen.spatial,true);
        Probe<Knoodle::LinkEmbedding3<Int,   Int,Int>>(c.curve,"LinkEmbedding3<i64>",cen.spatial,true);
        Probe<Knoodle::LinkEmbedding4<Int,   Int,Int>>(c.curve,"LinkEmbedding4<i64>",cen.spatial,true);
    }

    std::printf("\n%s\n", ok ? "INTERSECTION 3D CHECK OK" : "INTERSECTION 3D CHECK FAILED");
    return ok ? 0 : 1;
}
