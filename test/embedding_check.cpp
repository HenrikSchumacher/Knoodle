/**
 * @file embedding_check.cpp
 * @brief Degeneracy and performance test for the LinkEmbedding family.
 *
 * `LinkEmbedding` (floating-point backend) and `LinkEmbedding2/3/4` (exact
 * integer backends over `Prosector2/3/4`) turn 3D polygonal coordinates into the
 * crossing data that `PlanarDiagram` is assembled from. The 2/3/4 line claims
 * robustness to projection degeneracies -- vertices projecting onto vertices, a
 * vertex projecting into a segment, segments projecting to a point, several
 * segments through one projected point, collinear overlaps, zero-length edges.
 * Lattice links present all of these at once and are the motivating input.
 *
 * This driver checks that claim, at four levels of strictness:
 *
 *   census     Independently classify each fixture's projected edge pairs in
 *              exact integer arithmetic, and check it against the degeneracies
 *              the fixture declares. This is what makes "we exercised the corner
 *              cases" verifiable rather than assumed. Also pins that no fixture
 *              contains a genuine 3D intersection, the one case no perturbation
 *              can repair.
 *
 *   reader     The library's own file reader agrees with an independent parse.
 *
 *   exact      The sharp one. `Prosector2/3/4` all document the same symbolic
 *              perturbation -- project along `{eps,eps^3,1}`, let `eps -> 0+`.
 *              That limit is exactly computable by an integer shear (see
 *              `Shear` in embedding_fixtures.hpp), so the degenerate projection
 *              must reproduce the sheared, *generic* projection crossing for
 *              crossing. Not "same knot" -- the same crossings. N is escalated
 *              until the sheared answer stabilizes, so the test certifies rather
 *              than assumes that N was large enough.
 *
 *   cross      Sharing the perturbation contract, LinkEmbedding2, 3 and 4 must
 *              agree with each other crossing for crossing on every input.
 *
 *   invariant  Knot type is unchanged between the degenerate projection and
 *              random generic rotations: HOMFLY where the simplified diagram is
 *              small enough, the Alexander |det| fingerprint above that, and the
 *              link-component count always.
 *
 * `--bench` times the whole read -> intersect -> build-PD pipeline for each
 * class, on both the degenerate projection and a generic rotation of the same
 * curve -- the second row is the only fair comparison against LinkEmbedding,
 * which cannot run the first one at all.
 *
 * Build: see the `embedding_check` target in test/Makefile (needs UMFPACK +
 * BLAS/LAPACK for the Alexander fingerprint and the vendored libhomfly objects).
 */

#include "embedding_fixtures.hpp"
#include "homfly_invariance.hpp"
#include "link_alexander.hpp"

#include <chrono>
#include <limits>
#include <sys/wait.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace kt = knoodle_test;

// `Int`, `PD_T`, `PDC_T`, `Polynomial`, `Delta`, `Multiply`, `PolyToString`,
// `SimplifyAndMeasure`, `KnoodleJenkins` and `HomflyOfPossiblySplit` all come
// from homfly_invariance.hpp's internal-linkage namespace; declaring our own
// aliases for the first three here would make every unqualified use ambiguous.

/// The Alexander |det| fingerprint, instantiated once for this driver.
using Alexander_T = kt::LinkAlexander<std::complex<double>,std::int64_t>;

// ===========================================================================
//  The classes under test
// ===========================================================================
//
// `LinkEmbedding` is float-only (its template constrains Real_ with FloatQ), so
// the coordinate axis for it is f64/f32 alone.
//
// Two coordinate configurations advertised by the class documentation do not
// compile today. Both are kept behind macros so this test picks them up the
// moment they land, and both are *reported* at startup rather than silently
// skipped -- silence here would read as coverage that is not there.
//
//   i64  Integral `Real_`. src/LinkEmbedding2/EdgeCoordinates.hpp says
//        "Let's handle only case 1.2.a) for now" and then
//        `static_assert(FloatQ<Real>)`, so the "Case 2. `Real` is an integral
//        type / then we can simply copy" branch is unwritten.
//        Buildable since 4d2d0624; in the default set as of 2026-08-14.
//        Henrik speaking: This should resolved by now (2026-08-17).
//
//   i32  The 32-bit backend (`IReal_ = Int32`), which the class docs recommend
//        pairing with `Real_ = float`. Did not compile until 2026-08 (upstream
//        issue 4: ambiguous `Sign` in Prosector2, "excess elements in array
//        initializer" in WideInt for Prosector3/4); in the default set as of
//        2026-08-25. Its Real_ is float, so it inherits every @f32 marker.
//        Henrik speaking: This should resolved by now (2026-08-17).
//
// The integral-coordinate *path* is nevertheless exercised by default:
// `ReadVertexCoordinates` detects all-integral input and sets
// `scaling_exponent = 0`, so integer-valued doubles take the exact, unscaled,
// zero-rounding-error route through the same code. Every fixture here except
// the deliberately rotated ones is integral.

using LE1_f64 = Knoodle::LinkEmbedding <double,std::int64_t,float>;
using LE2_f64 = Knoodle::LinkEmbedding2<double,std::int64_t,std::int64_t>;
using LE3_f64 = Knoodle::LinkEmbedding3<double,std::int64_t,std::int64_t>;
using LE4_f64 = Knoodle::LinkEmbedding4<double,std::int64_t,std::int64_t>;

// Float coordinates against the 64-bit backend: a genuine narrowing of the
// input, so `RoundingError()` becomes informative.
using LE1_f32 = Knoodle::LinkEmbedding <float,std::int64_t,float>;
using LE2_f32 = Knoodle::LinkEmbedding2<float,std::int64_t,std::int64_t>;
using LE3_f32 = Knoodle::LinkEmbedding3<float,std::int64_t,std::int64_t>;
using LE4_f32 = Knoodle::LinkEmbedding4<float,std::int64_t,std::int64_t>;

using LE1_i32 = Knoodle::LinkEmbedding <float,std::int64_t,float>;
using LE2_i32 = Knoodle::LinkEmbedding2<float,std::int64_t,std::int32_t>;
using LE3_i32 = Knoodle::LinkEmbedding3<float,std::int64_t,std::int32_t>;
using LE4_i32 = Knoodle::LinkEmbedding4<float,std::int64_t,std::int32_t>;

// Integral `Real_`. Unbuildable until 4d2d0624 implemented it (finding C of
// docs/embedding-check.md); no longer gated.
using LE2_i64 = Knoodle::LinkEmbedding2<std::int64_t,std::int64_t,std::int64_t>;
using LE3_i64 = Knoodle::LinkEmbedding3<std::int64_t,std::int64_t,std::int64_t>;
using LE4_i64 = Knoodle::LinkEmbedding4<std::int64_t,std::int64_t,std::int64_t>;

enum class Coords { f64, f32, i32, i64 };

static const char * CoordsName( Coords c )
{
    switch( c )
    {
        case Coords::f64: return "f64";
        case Coords::f32: return "f32";
        case Coords::i32: return "i32";
        case Coords::i64: return "i64";
    }
    return "?";
}

static std::string ClassName( int cls, Coords c )
{
    return "LinkEmbedding" + (cls == 1 ? std::string("") : std::to_string(cls))
         + "<" + CoordsName(c) + ">";
}

/// Is this (class, coordinate type) combination buildable in this binary?
static bool SupportedQ( int cls, Coords c, std::string & why )
{
    if( c == Coords::i64 )
    {
        if( cls == 1 )
        {
            why = "LinkEmbedding is float-only (its Real_ is constrained by FloatQ)";
            return false;
        }
    }
    (void)cls;
    return true;
}

/// Call `fn.template operator()<LE_T>()` for the selected class. Returns false
/// if the combination is not buildable.
template<class Fn>
static bool WithClass( int cls, Coords c, Fn && fn )
{
    std::string why;
    if( !SupportedQ(cls,c,why) ) { return false; }

    if( c == Coords::f64 )
    {
        switch( cls )
        {
            case 1: fn.template operator()<LE1_f64>(); return true;
            case 2: fn.template operator()<LE2_f64>(); return true;
            case 3: fn.template operator()<LE3_f64>(); return true;
            case 4: fn.template operator()<LE4_f64>(); return true;
            default: return false;
        }
    }
    if( c == Coords::f32 )
    {
        switch( cls )
        {
            case 1: fn.template operator()<LE1_f32>(); return true;
            case 2: fn.template operator()<LE2_f32>(); return true;
            case 3: fn.template operator()<LE3_f32>(); return true;
            case 4: fn.template operator()<LE4_f32>(); return true;
            default: return false;
        }
    }
    if( c == Coords::i32 )
    {
        switch( cls )
        {
            case 1: fn.template operator()<LE1_i32>(); return true;
            case 2: fn.template operator()<LE2_i32>(); return true;
            case 3: fn.template operator()<LE3_i32>(); return true;
            case 4: fn.template operator()<LE4_i32>(); return true;
            default: return false;
        }
    }
    if( c == Coords::i64 )
    {
        switch( cls )
        {
            case 2: fn.template operator()<LE2_i64>(); return true;
            case 3: fn.template operator()<LE3_i64>(); return true;
            case 4: fn.template operator()<LE4_i64>(); return true;
            default: return false;
        }
    }
    return false;
}

// ===========================================================================
//  Result bookkeeping
// ===========================================================================

struct Tally
{
    Int passed = 0;
    Int failed = 0;
    Int skipped = 0;

    std::vector<std::string> failures;
    std::vector<std::string> skips;

    void Pass( const std::string & what, bool verbose )
    {
        ++passed;
        if( verbose ) { std::printf("    PASS  %s\n", what.c_str()); }
    }
    void Fail( const std::string & what, const std::string & why )
    {
        ++failed;
        failures.push_back(what + ": " + why);
        std::printf("    FAIL  %s\n          %s\n", what.c_str(), why.c_str());
    }
    void Skip( const std::string & what, const std::string & why, bool verbose )
    {
        ++skipped;
        skips.push_back(what + ": " + why);
        if( verbose ) { std::printf("    SKIP  %s  (%s)\n", what.c_str(), why.c_str()); }
    }
    /// Record a result that the fixture may have declared as a known failure.
    ///
    /// An `xfail_<tier>` marker means "this is a defect in the code under test,
    /// not in the test". The check still runs and is still printed, but it does
    /// not fail the suite. If it *passes*, that is an XPASS and does fail --
    /// otherwise a stale marker would quietly mask a later regression.
    void Verdict( const std::string & what, bool ok, const std::string & why,
                  const std::string & xfail_reason, bool verbose )
    {
        if( xfail_reason.empty() )
        {
            if( ok ) { Pass(what,verbose); } else { Fail(what,why); }
            return;
        }
        if( ok )
        {
            ++failed;
            failures.push_back(what + ": XPASS -- marked xfail (" + xfail_reason
                               + ") but it passed; remove the marker");
            std::printf("    XPASS %s\n          marked xfail (%s) but it passed;"
                        " remove the marker\n", what.c_str(), xfail_reason.c_str());
            return;
        }
        ++xfailed;
        xfails.push_back(what + ": " + xfail_reason + "  [" + why + "]");
        std::printf("    XFAIL %s\n          known: %s\n          got:   %s\n",
                    what.c_str(), xfail_reason.c_str(), why.c_str());
    }

    Int xfailed = 0;
    std::vector<std::string> xfails;
};

/// Is this class supposed to survive a degenerate projection at all?
///
/// `LinkEmbedding` computes intersections in floating point with no symbolic
/// perturbation, so it *correctly* refuses a degenerate projection (error code 5
/// for an interval/corner degeneracy). That refusal is the reason
/// LinkEmbedding2/3/4 exist; it is not a defect, and the test must not score it
/// as one. LinkEmbedding is therefore held only to the generic-projection
/// contract: on random rotations it must agree with everyone else.
static bool ResolvesDegeneraciesQ( int cls ) { return cls != 1; }

/// Is this coordinate type integral?
static bool IntegralCoordsValueQ( Coords c ) { return c == Coords::i64; }

/// Can transforms of this coordinate type be COMPOSED -- applied repeatedly to
/// the running coordinates rather than to the original each time?
///
/// Not for integral coordinates. A random *rotation* is unusable there to begin
/// with (a rotation of a lattice point is not a lattice point, and the rotation
/// matrix itself truncates to something singular), which is why integral types
/// get a unimodular integer matrix from `kt::GenericImage` instead. Those are
/// exact and knot-type-preserving -- determinant > 0, so the map is isotopic to
/// the identity -- but coordinates grow geometrically under composition, and the
/// exact path holds only while `scaling_exponent >= 0`. So each integer image
/// must be taken from the ORIGINAL curve.
///
/// That rules out exactly one thing: the rotation tier's `composed` path, whose
/// entire purpose is to accumulate. Everything else re-derives from the original
/// and runs normally.
[[maybe_unused]] static bool ComposableTransformsQ( Coords c ) { return !IntegralCoordsValueQ(c); }

/// Is this class's coordinate type integral? Derived from the class rather than
/// threaded through, so a new integral instantiation cannot forget to say so.
template<class LE_T>
inline constexpr bool IntegralCoordsQ = std::is_integral_v<typename LE_T::Real>;

/// The reason, for the skip message.
static const char * NotComposableWhy()
{
    return "integral coordinates cannot be transformed repeatedly: each image "
           "must come from the original curve, or the coordinates grow "
           "geometrically and leave the exact range";
}

/// The coordinate tag out of a class label.
///
/// Labels are generated by `ClassName`, which is exactly
/// "LinkEmbedding" + N + "<" + CoordsName(c) + ">", so the tag is what sits
/// between the angle brackets. Kept as one function so that coupling has a
/// single home: change `ClassName` and this is the only other place to look.
static std::string CoordsTagOf( const std::string & label )
{
    const std::size_t a = label.find('<');
    const std::size_t b = label.rfind('>');
    if( a == std::string::npos || b == std::string::npos || b <= a ) { return {}; }
    return label.substr(a+1,b-a-1);
}

/// The reason `tier` is expected to fail for this class, or "" if it should pass.
template<class FixtureT>
static std::string XFailFor( const FixtureT & f, const char * tier, int cls,
                             const std::string & label )
{
    return f.expect.XFail(tier,cls,CoordsTagOf(label));
}

// ===========================================================================
//  Invariants
// ===========================================================================

/// What a projection says the link is. Two projections of the same curve must
/// agree on `components`, and on `value` whenever both sides were computable.
struct LinkClass
{
    bool        ok             = false;
    std::string unsupported;              ///< why `value` is unavailable
    Int         components     = 0;       ///< link components, free unknots included
    Int         crossings_in   = 0;
    Int         crossings_out  = 0;
    std::string kind;                     ///< "homfly" | "alexander"
    std::string value;
    Alexander_T::Value alex;              ///< kept so the tolerant comparator can be used
};

/// Simplify, then fingerprint the result with the strongest oracle that fits.
///
/// `unlinks` are the crossing-free components `FromLinkEmbedding_Raw` reports
/// separately; they are genuine split unknots, so they are folded into the
/// component count and (for HOMFLY) into the polynomial via the split-union
/// delta rule.
static LinkClass Classify( const PD_T & pd, Int unlinks, Int curve_components,
                           Int homfly_cap )
{
    LinkClass lc;

    if( !pd.ValidQ() || pd.CrossingCount() <= Int(0) )
    {
        // No crossings at all: a split union of `curve_components` unknots.
        lc.ok            = true;
        lc.components    = curve_components;
        lc.crossings_in  = 0;
        lc.crossings_out = 0;
        lc.kind          = "homfly";

        auto H = Polynomial{ {{0,0},1} };
        for( Int i = 1; i < curve_components; ++i ) { H = Multiply(H,Delta); }
        lc.value = PolyToString(H);
        return lc;
    }

    lc.crossings_in = pd.CrossingCount();

    auto m = SimplifyAndMeasure(pd, /*need_single=*/true);

    lc.components = m.comp_after + unlinks;

    if( m.reduced_to_unknot )
    {
        lc.ok            = true;
        lc.crossings_out = 0;
        lc.kind          = "homfly";

        auto H = Polynomial{ {{0,0},1} };
        for( Int i = 0; i < unlinks; ++i ) { H = Multiply(H,Delta); }
        lc.value = PolyToString(H);
        return lc;
    }

    if( m.reassembly_failed || !m.single_after.ValidQ() )
    {
        lc.unsupported = "ToSingleDiagram could not reassemble the simplified complex";
        return lc;
    }

    lc.crossings_out = m.single_after.CrossingCount();

    if( lc.crossings_out <= homfly_cap )
    {
        bool ok = true;
        Polynomial H = HomflyOfPossiblySplit(KnoodleJenkins(m.single_after), ok);
        if( ok )
        {
            for( Int i = 0; i < unlinks; ++i ) { H = Multiply(H,Delta); }
            lc.ok    = true;
            lc.kind  = "homfly";
            lc.value = PolyToString(H);
            return lc;
        }
        lc.unsupported = "libhomfly rejected the Jenkins code";
        return lc;
    }

    // Too big for HOMFLY: the single-variable Alexander |det| fingerprint,
    // sampled on the unit circle. It declines split diagrams (det = 0 there),
    // in which case we are left with the component count alone.
    Alexander_T alex;
    auto v = alex(m.single_after);

    if( !v.ok )
    {
        lc.unsupported = "Alexander |det| unavailable (split diagram or singular matrix)";
        return lc;
    }
    lc.ok      = true;
    lc.kind    = "alexander";
    lc.value   = Alexander_T::ToString(v);
    lc.alex    = std::move(v);
    return lc;
}

/// Compare two projections' verdicts. Component count is always required to
/// match; the polynomial only when both sides produced one (a projection that
/// could not be classified is reported as a skip, never as a pass).
static bool SameLink( const LinkClass & a, const LinkClass & b, std::string & why )
{
    if( a.components != b.components )
    {
        why = "link component count " + std::to_string(a.components)
            + " vs " + std::to_string(b.components);
        return false;
    }
    if( !a.ok || !b.ok )
    {
        why = "not comparable (" + (a.ok ? b.unsupported : a.unsupported) + ")";
        return false;
    }
    if( a.kind != b.kind )
    {
        why = "different oracles (" + a.kind + " vs " + b.kind + "); "
              "one diagram fell on the far side of the HOMFLY cap";
        return false;
    }
    if( a.kind == "alexander" )
    {
        // log10|det| is a floating-point quantity at large scale; compare with
        // the tolerant relative comparator the fingerprint was designed with,
        // not by string equality.
        if( !Alexander_T::Equal(a.alex,b.alex) )
        {
            why = "Alexander |det| " + a.value + " vs " + b.value;
            return false;
        }
        return true;
    }
    if( a.value != b.value )
    {
        why = a.kind + " " + a.value + " vs " + b.value;
        return false;
    }
    return true;
}

// ===========================================================================
//  Fixtures
// ===========================================================================

struct Fixture
{
    std::string  path;
    std::string  name;
    kt::Curve    curve;
    kt::Census   census;
    kt::Expectations expect;
};

static bool CollectFixtures( const std::string & dir, const std::string & only,
                             std::vector<Fixture> & out, std::string & error )
{
    namespace fs = std::filesystem;

    if( !fs::exists(dir) )
    {
        error = "fixture directory '" + dir + "' does not exist";
        return false;
    }

    std::vector<std::string> paths;
    for( const auto & e : fs::directory_iterator(dir) )
    {
        if( e.path().extension() == ".crd" ) { paths.push_back(e.path().string()); }
    }
    std::sort(paths.begin(),paths.end());

    // Markers that apply to every fixture (class-level defects) live here.
    kt::Expectations defaults;
    if( !kt::LoadExpectations((fs::path(dir)/"DEFAULT.expect").string(),defaults,error) )
    {
        return false;
    }

    // Prefer an exact stem match when one exists: --only is substring matching
    // for interactive convenience, but the isolated runner addresses a single
    // fixture by name and must not pick up a second one that contains it.
    bool exact_exists = false;
    for( const auto & p : paths )
    {
        if( fs::path(p).stem().string() == only ) { exact_exists = true; break; }
    }

    for( const auto & p : paths )
    {
        const std::string stem = fs::path(p).stem().string();
        if( !only.empty() )
        {
            if( exact_exists ) { if( stem != only ) { continue; } }
            else if( stem.find(only) == std::string::npos ) { continue; }
        }

        Fixture f;
        f.path = p;
        f.name = stem;

        if( !kt::LoadCurve(p,f.curve,error) ) { return false; }

        const std::string xp = fs::path(p).replace_extension(".expect").string();
        if( !kt::LoadExpectations(xp,f.expect,error) ) { return false; }
        kt::InheritDefaults(f.expect,defaults);

        out.push_back(std::move(f));
    }

    if( out.empty() )
    {
        error = "no fixtures matched in '" + dir + "'";
        return false;
    }
    return true;
}

/// Handle an `xcrash_<tier>` marker: a combination that takes the process down.
///
/// In the normal (in-process) run there is nothing to score -- a segfault ends
/// the run -- so the combination is skipped with its reason printed. Under
/// `--isolate` each combination runs in a child process, so the crash can be
/// observed and scored, and this returns false to let it proceed.
static bool SkipKnownCrash( const Fixture & f, const char * tier, int cls,
                            const std::string & label, bool isolate,
                            Tally & t, bool verbose )
{
    const std::string reason = f.expect.XCrash(tier,cls,CoordsTagOf(label));
    if( reason.empty() || isolate ) { return false; }   // `isolate` is set in the child

    t.Skip(std::string(tier) + " " + f.name + " " + label,
           "known crash, skipped to keep the run alive (use --isolate to score it): "
           + reason, verbose);
    return true;
}

// ===========================================================================
//  Tiers
// ===========================================================================

// --- census -----------------------------------------------------------------

static void TierCensus( std::vector<Fixture> & fx, Tally & t, bool verbose )
{
    std::printf("\n=== census: fixtures are as degenerate as they claim ===\n");

    for( auto & f : fx )
    {
        f.census = kt::TakeCensus(f.curve);

        const std::string what = "census " + f.name;

        if( f.census.edges == Int(0) )
        {
            t.Skip(what,"too large for the pair scan",verbose);
            continue;
        }

        if( verbose )
        {
            std::printf("    %-24s %s\n", f.name.c_str(), f.census.ToString().c_str());
        }

        // A genuine 3D intersection is the one degeneracy no symbolic
        // perturbation can repair, so a fixture must never contain one --
        // otherwise a legitimate error return would look like a test failure.
        if( f.census.spatial != Int(0) )
        {
            t.Fail(what, "fixture contains " + std::to_string(f.census.spatial)
                       + " genuine 3D intersection(s); no perturbation can resolve these");
            continue;
        }

        auto failures = kt::CheckExpectations(f.census,f.expect);
        if( !failures.empty() )
        {
            std::string why = failures[0];
            for( std::size_t i = 1; i < failures.size(); ++i ) { why += "; " + failures[i]; }
            t.Fail(what,why);
            continue;
        }
        t.Pass(what,verbose);
    }
}

// --- reader -----------------------------------------------------------------

/// The library's own reader must see the same curve an independent parse sees.
/// Cheap, and it pins the file format the rest of the fixtures rely on.
template<class LE_T>
static void ReaderCheckOne( const Fixture & f, const std::string & label,
                            Tally & t, bool verbose )
{
    const std::string what = "reader " + f.name + " " + label;

    try
    {
        LE_T L = kt::LoadWithLibraryReader<LE_T>( std::filesystem::path(f.path) );

        if( Int(L.EdgeCount()) != f.curve.VertexCount() )
        {
            t.Fail(what, "ReadFromFile saw " + std::to_string(Int(L.EdgeCount()))
                       + " edges, the file has " + std::to_string(f.curve.VertexCount())
                       + " vertices");
            return;
        }
        if( Int(L.ComponentCount()) != f.curve.ComponentCount() )
        {
            t.Fail(what, "ReadFromFile saw " + std::to_string(Int(L.ComponentCount()))
                       + " components, the file has "
                       + std::to_string(f.curve.ComponentCount()));
            return;
        }
        t.Pass(what,verbose);
    }
    catch( const std::exception & e )
    {
        t.Fail(what, std::string("ReadFromFile threw: ") + e.what());
    }
}

// --- exact ------------------------------------------------------------------

/// The shear tier for one class on one fixture.
///
/// Escalates N along the ladder until two consecutive shears agree, which is
/// what certifies N was large enough; then requires the raw (degenerate)
/// projection to equal that stable answer exactly.
template<class LE_T>
static void ExactCheckOne( const Fixture & f, const std::string & label, int cls,
                           Tally & t, bool verbose )
{
    const std::string what  = "exact " + f.name + " " + label;
    const std::string xfail = XFailFor(f,"exact",cls,label);

    if( !f.curve.IntegralQ() )
    {
        t.Skip(what,"fixture is not integral, so the shear is not exact",verbose);
        return;
    }

    auto raw = kt::RunEmbedding<LE_T>(f.curve);
    if( !raw.ok )
    {
        t.Verdict(what,false,"raw projection failed -- " + raw.message,xfail,verbose);
        return;
    }

    kt::Fingerprint stable;
    bool            have_stable = false;
    double          stable_N    = 0.0;
    std::string     last_note;

    kt::Fingerprint prev;
    bool            have_prev = false;

    for( double N : kt::ShearLadder() )
    {
        // Stay inside the exactly-representable range; a silently rounded shear
        // would be comparing against the wrong curve.
        if( kt::ShearMagnitude(f.curve,N) > 9.0e15 )
        {
            last_note = "shear by N=" + std::to_string(std::llround(N))
                      + " would leave exact-integer range";
            break;
        }

        auto sh = kt::RunEmbedding<LE_T>( kt::Shear(f.curve,N) );
        if( !sh.ok )
        {
            last_note = "shear N=" + std::to_string(std::llround(N)) + " failed -- " + sh.message;
            continue;
        }

        if( have_prev && sh.fp == prev )
        {
            stable      = sh.fp;
            have_stable = true;
            stable_N    = N;
            break;
        }
        prev      = sh.fp;
        have_prev = true;
    }

    if( !have_stable )
    {
        t.Skip(what, "sheared projection never stabilized"
                     + (last_note.empty() ? std::string() : " (" + last_note + ")"), verbose);
        return;
    }

    if( raw.fp != stable )
    {
        t.Verdict(what,false,
                  "symbolic perturbation disagrees with the explicit shear (stable at N="
                  + std::to_string(std::llround(stable_N)) + "): "
                  + raw.fp.DiffAgainst(stable), xfail, verbose);
        return;
    }

    if( !xfail.empty() ) { t.Verdict(what,true,"",xfail,verbose); return; }

    if( verbose )
    {
        std::printf("    PASS  %s  (%lld crossings, stable at N=%lld)\n",
                    what.c_str(), (long long)raw.crossing_count,
                    (long long)std::llround(stable_N));
        ++t.passed;
    }
    else { t.Pass(what,false); }
}

// --- rotation ---------------------------------------------------------------

/// Repeatedly re-aiming the projection must not make the knot grow.
///
/// Rotating a *fixed* curve changes which projection you look at, so the crossing
/// count legitimately varies from one rotation to the next. What must not happen
/// is a *trend*: rotations are drawn uniformly from SO(3), so the projections are
/// exchangeable and the crossing count has no reason to drift in either
/// direction. If it climbs, the embedding is degrading rather than being re-aimed.
///
/// Two paths are run, and the difference between them is the whole point:
///
///   fresh      each step rotates the *original* coordinates by an independent
///              random matrix. Nothing accumulates, so this is the control: it
///              measures the honest spread of crossing counts over projections.
///
///   composed   one embedding object is held and `Transform()` is called on it
///              repeatedly, each rotation landing on the output of the last.
///              This is where error, and any translation folded into something
///              advertised as a rotation, accumulates. It is the path Rattle used
///              to take, and the one 00a8407 had to route around for
///              `LinkEmbedding` -- see test/embedding_inflation_check.cpp, which
///              pins the geometry for that class. Here it is checked through the
///              generic harness, so LinkEmbedding2/3/4 are covered too, and the
///              *diagram* is checked rather than only the coordinates.
///
/// Three assertions per path: the knot type never changes; the radius of gyration
/// is conserved (exactly invariant under any rigid motion, and translation-
/// invariant so the internal Sterbenz shift does not register as motion); and the
/// crossing count does not trend upward from the first quarter to the last.
template<class LE_T>
static void RotationCheckOne( const Fixture & f, const std::string & label, int cls,
                              std::int64_t steps, std::int64_t homfly_cap,
                              Tally & t, bool verbose )
{
    using Real_T = typename LE_T::Real;

    // Anchor on a generic rotation: a class without symbolic perturbation cannot
    // start from the degenerate projection, and the question here is about
    // re-aiming, not about degeneracy.
    const kt::Curve start = kt::GenericImage(f.curve, 0x0A7A7EULL,
                                             IntegralCoordsQ<LE_T>);

    // ---- reference verdict -------------------------------------------------
    LinkClass reference;
    {
        auto r = kt::RunEmbedding<LE_T>(start,/*want_pd=*/true);
        if( !r.ok )
        {
            t.Verdict("rotation " + f.name + " " + label, false,
                      "the anchoring projection failed -- " + r.message,
                      XFailFor(f,"rotation",cls,label), verbose);
            return;
        }
        reference = Classify(r.pd, r.unlink_count, start.ComponentCount(), homfly_cap);
    }

    // ---- the two paths -----------------------------------------------------
    struct Track
    {
        std::vector<std::int64_t> crossings;
        std::vector<double>       gyration;
        std::vector<kt::Fingerprint> prints;
        std::string               failure;
        std::string               declined;  ///< a legitimate mid-sequence refusal
        LinkClass                 last;
    };

    /// Did the class DECLINE to answer, rather than answer wrongly?
    ///
    /// `LinkEmbedding` returns error code 8 when two intersection times along an
    /// edge come within `intersection_time_tolerance` of each other
    /// (`src/LinkEmbedding/FindIntersections.hpp`): it cannot order them, so it
    /// refuses instead of guessing. For some rotations at some precisions that
    /// ordering is genuinely not well defined -- f32 on the larger lattices hits
    /// it -- and refusing is the correct behaviour, not a defect. This tier asks
    /// whether re-aiming makes the knot GROW, and a class that declines to
    /// answer has not grown anything, so the sequence stops there and the steps
    /// already collected are still checked.
    ///
    /// Only `LinkEmbedding` can produce it. Code 8 belongs to the floating-point
    /// path, which is the only one that has to order intersection times it
    /// cannot separate; the `LinkEmbedding_Int` family (2/3/4) computes exactly
    /// and returns 0 on success or 1/2/3 for "no coordinates", "unknown", and
    /// "self-intersects in 3-space" (`src/LinkEmbedding_Int/Intersections.hpp`).
    /// So a refusal from 2/3/4 never looks like a decline here and stays a
    /// failure -- which is right, since they claim to resolve degeneracies.
    /// That is why this keys on the code alone and needs no class test.
    ///
    /// (Both families return `int` as of `1d8761c7`, which unified a convention
    /// split that used to have 2/3/4 returning `bool`. If the `_Int` codes ever
    /// grow an 8, this needs a class test.)
    auto declined_orderingQ = []( int err ) { return err == 8; };

    auto run_fresh = [&]() -> Track
    {
        Track tr;
        for( std::int64_t k = 0; k < steps; ++k )
        {
            const kt::Curve c = kt::GenericImage(f.curve, 0x5EED5EEDULL + std::uint64_t(k),
                                                IntegralCoordsQ<LE_T>);
            auto r = kt::RunEmbedding<LE_T>(c, /*want_pd=*/(k+1 == steps));
            if( !r.ok )
            {
                if( declined_orderingQ(r.err) )
                {
                    tr.declined = "step " + std::to_string(k) + ": " + r.message;
                    break;
                }
                tr.failure = "step " + std::to_string(k) + ": " + r.message;
                return tr;
            }

            tr.crossings.push_back(r.crossing_count);
            tr.gyration.push_back(kt::RadiusOfGyration(c.v));
            tr.prints.push_back(r.fp);
            if( r.pd_built )
            {
                tr.last = Classify(r.pd, r.unlink_count, c.ComponentCount(), homfly_cap);
            }
        }
        return tr;
    };

    auto run_composed = [&]() -> Track
    {
        Track tr;

        Knoodle::Tensor1<Int,Int> cp ( start.comp_ptr.data(), Int(start.comp_ptr.size()) );
        Knoodle::Tensor1<Int,Int> col =
            Knoodle::iota<Int,Int>( start.comp_ptr.size() - std::size_t(1) );

        LE_T L ( std::move(cp), std::move(col) );

        std::vector<Real_T> coords ( start.v.size() );
        for( std::size_t i = 0; i < start.v.size(); ++i )
        {
            coords[i] = static_cast<Real_T>(start.v[i]);
        }
        L.template ReadVertexCoordinates<false>( coords.data() );

        std::vector<Real_T> out ( start.v.size() );

        for( std::int64_t k = 0; k < steps; ++k )
        {
            if( k > 0 )
            {
                // Compose the next rotation onto the coordinates already in place.
                const auto R = kt::RandomRotationMatrix( 0xC0FFEEULL + std::uint64_t(k) );

                Real_T flat[9];
                for( int i = 0; i < 9; ++i ) { flat[i] = static_cast<Real_T>(R[std::size_t(i)]); }

                typename LE_T::Matrix3x3_T A;
                A.Read(flat);

                try { L.Transform(A); }
                catch( const std::exception & e )
                {
                    tr.failure = "step " + std::to_string(k) + ": Transform threw: " + e.what();
                    return tr;
                }
            }

            std::string msg;
            int err = 0;
            if( !kt::RequireIntersectionsOK(L,msg,err) )
            {
                if( declined_orderingQ(err) )
                {
                    tr.declined = "step " + std::to_string(k) + ": " + msg;
                    break;
                }
                tr.failure = "step " + std::to_string(k) + ": " + msg;
                return tr;
            }

            tr.crossings.push_back( std::int64_t(L.IntersectionCount()) );
            tr.prints.push_back( kt::TakeFingerprint(L) );

            L.WriteVertexCoordinates( out.data() );
            tr.gyration.push_back( kt::RadiusOfGyration(out) );

            if( k+1 == steps )
            {
                auto [pd,unlinks] = kt::PD_T::FromLinkEmbedding(L);
                tr.last = Classify(pd, Int(unlinks.Size()), start.ComponentCount(), homfly_cap);
            }
        }
        return tr;
    };

    // ---- score -------------------------------------------------------------
    auto score = [&]( const char * path_name, const Track & tr )
    {
        const std::string what = "rotation " + f.name + " " + label + " " + path_name;

        // Markers on this tier describe `Transform`, which only the composed path
        // exercises. Holding the fresh path to them too would make every marker
        // XPASS against a path it was never about.
        const std::string xfail = (std::strcmp(path_name,"composed") == 0)
                                ? XFailFor(f,"rotation",cls,label) : std::string();

        if( !tr.failure.empty() )
        {
            t.Verdict(what,false,tr.failure,xfail,verbose);
            return;
        }
        if( tr.crossings.empty() )
        {
            t.Skip(what,
                   tr.declined.empty()
                       ? std::string("no steps ran")
                       : "declined at the first step (" + tr.declined + ")",
                   verbose);
            return;
        }

        // Accepted, but say so: a shortened sequence is weaker evidence than a
        // full one, and a decline that starts happening earlier and earlier is
        // worth a human noticing even though it is not a failure.
        if( !tr.declined.empty() )
        {
            std::printf("    NOTE  %s\n"
                        "          declined and was accepted: %s\n"
                        "          (%lld of %lld steps still checked for growth;"
                        " knot type not compared)\n",
                        what.c_str(), tr.declined.c_str(),
                        static_cast<long long>(tr.crossings.size()),
                        static_cast<long long>(steps));
        }

        // (a) re-aiming must actually re-aim.
        //
        // Twenty-odd uniformly random rotations of a curve cannot all produce
        // the identical crossing set; if they do, the rotation is not reaching
        // the computation. That is exactly what a stale cache looks like from
        // outside, and it is the failure mode this catch was added for --
        // `LinkEmbedding::Transform` resets neither `intersections_computedQ`
        // nor `bounding_boxes_computedQ`, so every projection after the first
        // returns the first one's answer.
        if( tr.prints.size() >= 4 )
        {
            bool all_same = true;
            for( std::size_t i = 1; i < tr.prints.size(); ++i )
            {
                if( tr.prints[i] != tr.prints[0] ) { all_same = false; break; }
            }
            if( all_same )
            {
                t.Verdict(what,false,
                          "the projection never changed over " + std::to_string(steps)
                          + " random rotations -- the crossing set is bit-identical every"
                          " time, so the rotation is not reaching the intersection"
                          " computation (stale cache)", xfail, verbose);
                return;
            }
        }

        // (b) the knot must be the same one throughout.
        //
        // Only a sequence that ran to the end has a final classification: the PD
        // is built on the last step, so a run that declined earlier (above) has
        // no `tr.last` to compare and must not be scored on a default-constructed
        // one -- that reads as "component count 1 vs 0" and is pure artefact.
        // The growth checks above are the tier's actual claim and still applied.
        std::string why;
        if( !tr.declined.empty() )
        {
            t.Pass(what,verbose);
            return;
        }
        if( !SameLink(reference,tr.last,why) )
        {
            if( why.rfind("not comparable",0) == 0 || why.rfind("different oracles",0) == 0 )
            {
                t.Skip(what,why,verbose);
            }
            else
            {
                t.Verdict(what,false,"the knot changed over " + std::to_string(steps)
                          + " rotations: " + why, xfail, verbose);
            }
            return;
        }

        // (c) the shape must not inflate or collapse
        const double g0  = tr.gyration.front();
        double g_lo = g0, g_hi = g0;
        for( double g : tr.gyration ) { g_lo = std::min(g_lo,g); g_hi = std::max(g_hi,g); }

        // The tolerance has to scale with the working precision: composing k
        // rotations accumulates roundoff like sqrt(k)*eps, which is ~1e-15 for
        // double but ~6e-7 for float, so a single fixed threshold would either
        // be blind at f64 or cry wolf at f32. 256*sqrt(k)*eps leaves a wide
        // margin over that random walk and still sits orders of magnitude below
        // any real drift -- the inflation embedding_inflation_check documents
        // moves the coordinates by a factor of ~4000.
        const double eps = double(std::numeric_limits<Real_T>::epsilon());
        const double tol = std::max( 1.0e-12,
                                     256.0 * std::sqrt(double(steps)) * eps );

        // R_g measures a RIGID motion. Integral coordinates are re-aimed by a
        // unimodular integer matrix instead, which preserves orientation and
        // therefore knot type -- determinant +1, hence isotopic to the identity
        // -- but is not an isometry and moves distances freely. So this
        // assertion is about rotations specifically, and does not apply there.
        // The other three (re-aiming re-aims, knot type is constant, no upward
        // trend) all still do, and they are the ones that matter.
        const bool rigidQ = !IntegralCoordsQ<LE_T>;

        if( rigidQ && g0 > 0.0 && (g_hi - g_lo) / g0 > tol )
        {
            char buf[256];
            std::snprintf(buf,sizeof(buf),
                          "radius of gyration is not conserved over %lld rotations: "
                          "%.17g .. %.17g (relative spread %.3e, tolerance %.3e)",
                          (long long)steps, g_lo, g_hi, (g_hi-g_lo)/g0, tol);
            t.Verdict(what,false,buf,xfail,verbose);
            return;
        }

        // (d) the crossing count must not trend upward
        const std::size_t n = tr.crossings.size();
        const std::size_t q = std::max<std::size_t>(1, n/4);

        auto mean = [&](std::size_t b, std::size_t e)
        {
            double s = 0.0;
            for( std::size_t i = b; i < e; ++i ) { s += double(tr.crossings[i]); }
            return s / double(e-b);
        };

        const double first_q = mean(0,q);
        const double last_q  = mean(n-q,n);

        // Generous on purpose: this is looking for a trend, not policing the
        // honest sampling spread of projections of one curve. A 25% climb in the
        // mean over dozens of uniform rotations is not sampling noise.
        if( n >= 8 && last_q > 1.25*first_q + 2.0 )
        {
            t.Verdict(what,false,
                      "crossing count trends upward over " + std::to_string(steps)
                      + " rotations: first quarter mean " + std::to_string(first_q)
                      + ", last quarter mean " + std::to_string(last_q), xfail, verbose);
            return;
        }

        if( verbose )
        {
            const auto [lo,hi] = std::minmax_element(tr.crossings.begin(),tr.crossings.end());
            std::printf("    PASS  %s  (crossings %lld..%lld, first/last quarter mean "
                        "%.1f/%.1f; R_g %.6g stable)\n",
                        what.c_str(), (long long)*lo, (long long)*hi, first_q, last_q, g0);
            ++t.passed;
        }
        else { t.Verdict(what,true,"",xfail,verbose); }
    };

    score("fresh",    run_fresh());

    // `composed` is the one path that accumulates, so it is the one path
    // integral coordinates cannot take. See ComposableTransformsQ.
    if constexpr ( IntegralCoordsQ<LE_T> )
    {
        t.Skip("rotation " + f.name + " " + label + " composed",
               NotComposableWhy(), verbose);
    }
    else
    {
        score("composed", run_composed());
    }
}

// --- cross-class ------------------------------------------------------------

/// LinkEmbedding2/3/4 share one perturbation contract, so on any input their
/// crossing sets must be identical. LinkEmbedding (float, no perturbation) is
/// compared only on generic input, where every backend should agree.
static void TierCross( const std::vector<Fixture> & fx, Coords coords,
                       Int rotations, bool in_child, Tally & t, bool verbose )
{
    std::printf("\n=== cross-class <%s>: LinkEmbedding2/3/4 agree crossing for crossing "
                "(and LinkEmbedding too, on generic input) ===\n", CoordsName(coords));

    for( const auto & f : fx )
    {
        // This tier compares the backends pairwise, so it runs them all in one
        // unit -- which means a class that CRASHES takes the whole unit with it,
        // and with it the rest of the run. Honour a crash marker on any
        // participating class, the same way the xfail lookup below does.
        {
            bool skipped = false;
            for( int cls : {1,2,3,4} )
            {
                const std::string lbl = "cross " + f.name + " " + ClassName(cls,coords);
                if( SkipKnownCrash(f,"cross",cls,lbl,in_child,t,verbose) )
                { skipped = true; break; }
            }
            if( skipped ) { continue; }
        }

        // `generic` distinguishes a random rotation (degeneracy-free with
        // probability 1, so every backend including LinkEmbedding must agree)
        // from the raw fixture (degenerate, so only 2/3/4 are in scope).
        auto compare = [&]( const kt::Curve & c, const std::string & tag, bool generic )
        {
            kt::RunResult r1, r2, r3, r4;
            bool built = true;

            built &= WithClass(2,coords,[&]<class LE>(){ r2 = kt::RunEmbedding<LE>(c); });
            built &= WithClass(3,coords,[&]<class LE>(){ r3 = kt::RunEmbedding<LE>(c); });
            built &= WithClass(4,coords,[&]<class LE>(){ r4 = kt::RunEmbedding<LE>(c); });

            if( generic )
            {
                built &= WithClass(1,coords,[&]<class LE>(){ r1 = kt::RunEmbedding<LE>(c); });
            }

            if( !built ) { return; }

            // This tier compares backends PAIRWISE, so a marker has to be
            // resolved per pair, not once for the whole unit. A defect in one
            // class must not make the comparisons that exclude it expect
            // failure -- marking `xfail_cross = 1` once for the unit made
            // LE2-vs-LE3 and LE3-vs-LE4 XPASS, which is how this was found
            // (nearmiss_sphere, where only LinkEmbedding fails).
            auto class_of = []( const char * n ) -> int
            {
                // names are "LE1".."LE4"
                return (n[2] >= '1' && n[2] <= '4') ? (n[2] - '0') : 0;
            };

            auto one = [&]( const char * an, const kt::RunResult & a,
                            const char * bn, const kt::RunResult & b )
            {
                std::string xfail;
                for( int cls : { class_of(an), class_of(bn) } )
                {
                    if( cls == 0 ) { continue; }
                    if( xfail.empty() )
                    { xfail = XFailFor(f,"cross",cls,ClassName(cls,coords)); }
                }

                const std::string what = "cross " + f.name + tag + " " + an + " vs " + bn;

                if( !a.ok || !b.ok )
                {
                    if( a.ok != b.ok )
                    {
                        t.Verdict(what,false,
                                  std::string(an) + (a.ok ? " succeeded" : " failed")
                                  + " but " + bn + (b.ok ? " succeeded" : " failed")
                                  + " -- " + (a.ok ? b.message : a.message), xfail, verbose);
                    }
                    else
                    {
                        t.Skip(what,"both failed: " + a.message,verbose);
                    }
                    return;
                }
                t.Verdict(what, a.fp == b.fp, a.fp.DiffAgainst(b.fp), xfail, verbose);
            };

            one("LE2",r2,"LE3",r3);
            one("LE3",r3,"LE4",r4);

            // On a generic projection there are no degeneracies to resolve, so
            // the floating-point backend must land on the same crossing set as
            // the exact ones. A disagreement here is a real bug in one of them.
            if( generic ) { one("LE1",r1,"LE2",r2); }
        };

        compare(f.curve,"",/*generic=*/false);

        for( std::int64_t k = 0; k < rotations; ++k )
        {
            compare( kt::GenericImage(f.curve, std::uint64_t(0x5eed'0000 + k),
                                      IntegralCoordsValueQ(coords)),
                     "[rot" + std::to_string(k) + "]", /*generic=*/true );
        }
    }
}

// --- invariant --------------------------------------------------------------

/// `symmetry` -- the 24 rotations of the cube must all give the same knot.
///
/// Signed permutation matrices of determinant +1 map the cubic lattice onto
/// itself, so a lattice configuration stays a lattice configuration and its
/// projection stays MAXIMALLY DEGENERATE. Measured on `lattice_04`: 96 pairs of
/// vertices share a projected column before the transform and 96 after. That is
/// what makes this worth doing -- it is 24 independent hard cases per fixture
/// rather than one, where a random rotation gives an easy case every time.
///
/// Determinant +1 means orientation-preserving, so every image is the same knot
/// (`GL+(3,R)` is connected, hence the map is isotopic to the identity). The
/// crossing COUNT legitimately differs between images -- they are different
/// projections -- so only the link type is compared.
///
/// Exact for integral coordinates, and exact for the integer-valued floating
/// point ones too, since the entries are 0 and +/-1.
template<class LE_T>
static void SymmetryCheckOne( const Fixture & f, const std::string & label,
                              int cls, bool degeneracy_capable,
                              std::int64_t homfly_cap, Tally & t, bool verbose )
{
    const std::string what  = "symmetry " + f.name + " " + label;
    const std::string xfail = XFailFor(f,"symmetry",cls,label);

    auto classify = [&]( const kt::Curve & c, LinkClass & lc, std::string & err ) -> bool
    {
        auto r = kt::RunEmbedding<LE_T>(c,/*want_pd=*/true);
        if( !r.ok ) { err = r.message; return false; }
        lc = Classify(r.pd, r.unlink_count, c.ComponentCount(), homfly_cap);
        return true;
    };

    const auto & rots = kt::OctahedralRotations();

    LinkClass base;
    std::string err;
    std::size_t first = 0;

    // Anchor on the first image that this class can actually compute. For a
    // class with no symbolic perturbation every lattice image is degenerate and
    // may legitimately be refused; refusing all 24 is not a failure, it is the
    // documented limitation, so the check reports it and stops.
    for( ; first < rots.size(); ++first )
    {
        const kt::Curve c = kt::ApplyMatrix(f.curve,rots[first],
                                            "oct" + std::to_string(first));
        if( classify(c,base,err) ) { break; }
        if( degeneracy_capable )
        {
            t.Verdict(what,false,"image " + std::to_string(first)
                      + " failed -- " + err, xfail, verbose);
            return;
        }
    }

    if( first >= rots.size() )
    {
        t.Skip(what,"this class refused all 24 lattice-symmetric projections, "
                    "which is expected without symbolic perturbation",verbose);
        return;
    }

    std::size_t compared = 0;

    for( std::size_t i = first+1; i < rots.size(); ++i )
    {
        const kt::Curve c = kt::ApplyMatrix(f.curve,rots[i],"oct" + std::to_string(i));

        LinkClass lc;
        if( !classify(c,lc,err) )
        {
            if( degeneracy_capable )
            {
                t.Verdict(what,false,"image " + std::to_string(i) + " failed -- " + err,
                          xfail,verbose);
                return;
            }
            continue;   // a float class may decline a degenerate image
        }

        std::string why;
        if( !SameLink(base,lc,why) )
        {
            if( why.rfind("not comparable",0) == 0
             || why.rfind("different oracles",0) == 0 )
            {
                continue;
            }
            t.Verdict(what,false,"image " + std::to_string(i)
                      + " is a different link: " + why, xfail, verbose);
            return;
        }
        ++compared;
    }

    if( compared == 0 )
    {
        t.Skip(what,"no two images were comparable",verbose);
        return;
    }
    t.Verdict(what,true,"",xfail,verbose);
}

template<class LE_T>
static void InvariantCheckOne( const Fixture & f, const std::string & label,
                               int cls, bool degeneracy_capable,
                               std::int64_t rotations, std::int64_t homfly_cap,
                               Tally & t, bool verbose )
{
    const std::string what  = "invariant " + f.name + " " + label;
    const std::string xfail = XFailFor(f,"invariant",cls,label);

    auto classify = [&]( const kt::Curve & c, LinkClass & lc, std::string & err ) -> bool
    {
        auto r = kt::RunEmbedding<LE_T>(c,/*want_pd=*/true);
        if( !r.ok ) { err = r.message; return false; }
        lc = Classify(r.pd, r.unlink_count, c.ComponentCount(), homfly_cap);
        return true;
    };

    LinkClass base;
    std::string err;

    // A class without symbolic perturbation is not expected to survive the
    // degenerate projection, so it is anchored on a generic rotation instead and
    // compared against further rotations. It still gets a real invariance test;
    // it is simply not held to a contract it never claimed.
    std::int64_t first_rotation = 0;

    if( degeneracy_capable )
    {
        if( !classify(f.curve,base,err) )
        {
            t.Verdict(what,false,"degenerate projection failed -- " + err,xfail,verbose);
            return;
        }
    }
    else
    {
        std::string ignored;
        LinkClass   probe;
        const bool  survived = classify(f.curve,probe,ignored);

        if( verbose )
        {
            std::printf("    note  %s: degenerate projection %s (expected for a class\n"
                        "          with no symbolic perturbation); anchoring on a rotation\n",
                        what.c_str(),
                        survived ? "unexpectedly succeeded" : ("failed -- " + ignored).c_str());
        }

        if( !classify(kt::GenericImage(f.curve,std::uint64_t(0xA11CE),
                                       IntegralCoordsQ<LE_T>),base,err) )
        {
            t.Verdict(what,false,"even a generic rotation failed -- " + err,xfail,verbose);
            return;
        }
        first_rotation = 1;
    }

    std::int64_t agreed = 0;

    for( std::int64_t k = first_rotation; k < rotations; ++k )
    {
        const kt::Curve rot = kt::GenericImage(f.curve, std::uint64_t(0xA11CE + k),
                                               IntegralCoordsQ<LE_T>);

        LinkClass other;
        if( !classify(rot,other,err) )
        {
            t.Skip(what + " rot" + std::to_string(k),
                   "rotated projection failed -- " + err, verbose);
            continue;
        }

        std::string why;
        if( !SameLink(base,other,why) )
        {
            if( why.rfind("not comparable",0) == 0 || why.rfind("different oracles",0) == 0 )
            {
                t.Skip(what + " rot" + std::to_string(k), why, verbose);
                continue;
            }
            t.Verdict(what + " rot" + std::to_string(k), false,
                      "degenerate projection says {" + base.kind + " " + base.value
                      + ", " + std::to_string(base.components) + " components}, "
                      "generic rotation says {" + other.kind + " " + other.value
                      + ", " + std::to_string(other.components) + " components}: " + why,
                      xfail, verbose);
            continue;
        }
        ++agreed;
    }

    if( agreed == std::int64_t(0) )
    {
        t.Skip(what,"no rotation produced a comparable verdict",verbose);
        return;
    }

    if( !xfail.empty() ) { t.Verdict(what,true,"",xfail,verbose); return; }

    if( verbose )
    {
        std::printf("    PASS  %s  (%s, %lld components; %lld/%lld rotations agreed;"
                    " %lld -> %lld crossings)\n",
                    what.c_str(), base.kind.c_str(), (long long)base.components,
                    (long long)agreed, (long long)rotations,
                    (long long)base.crossings_in, (long long)base.crossings_out);
        ++t.passed;
    }
    else { t.Pass(what,false); }
}

// ===========================================================================
//  Benchmark
// ===========================================================================

/// Time one class on one curve. Reported as best-of-reps: we want the machine's
/// floor, not its noise.
template<class LE_T>
static void BenchCurve( const kt::Curve & c, const std::string & fixture,
                        const char * kind, const std::string & label,
                        std::int64_t reps )
{
    using Clock = std::chrono::steady_clock;
    using Millis = std::chrono::duration<double,std::milli>;

    double       best      = 1e300;
    std::int64_t crossings = -1;
    double       rounding  = 0.0;

    for( std::int64_t r = 0; r < reps; ++r )
    {
        const auto t0 = Clock::now();
        auto res = kt::RunEmbedding<LE_T>(c,/*want_pd=*/true);
        const auto t1 = Clock::now();

        if( !res.ok )
        {
            std::printf("  %-14s %-8s %-22s  %s\n",
                        fixture.c_str(), kind, label.c_str(), res.message.c_str());
            return;
        }
        best      = std::min(best, Millis(t1-t0).count());
        crossings = res.crossing_count;
        rounding  = res.rounding_error;
    }

    std::printf("  %-14s %-8s %-22s %9.3f ms  crossings=%-7lld rounding_err=%g\n",
                fixture.c_str(), kind, label.c_str(), best,
                (long long)crossings, rounding);
}

/// Both projections of each fixture are timed: the raw one (degenerate -- the
/// case the exact backends exist for) and a random rotation (generic -- the only
/// case LinkEmbedding can handle, so the only fair comparison against it).
template<class LE_T>
static void BenchOne( const Fixture & f, const std::string & label, std::int64_t reps )
{
    BenchCurve<LE_T>(f.curve, f.name, "degen", label, reps);
    BenchCurve<LE_T>(kt::GenericImage(f.curve,0xB0FFU,IntegralCoordsQ<LE_T>),
                     f.name, "generic", label, reps);
}

// ===========================================================================
//  main
// ===========================================================================

// ===========================================================================
//  Isolated mode
// ===========================================================================
//
// The library under test can take the process down (see the xcrash markers in
// the fixtures). `--isolate` re-executes this same binary once per
// (tier, fixture, class, coords) unit so a crash becomes a reported result
// instead of the end of the run. Slower -- one process per unit -- so it is
// opt-in, and the default run skips the known-crashing combinations instead.

struct ChildResult
{
    bool crashed = false;
    int  signal  = 0;
    Int  passed = 0, failed = 0, skipped = 0, xfailed = 0;
};

static ChildResult RunChild( const std::string & self, const std::string & tier,
                             const std::string & fixture, int cls,
                             const std::string & coords,
                             const std::string & fixtures_dir,
                             std::int64_t rotations, std::int64_t homfly_cap,
                             bool verbose, std::string & output )
{
    ChildResult r;

    std::string cmd = "'" + self + "'"
        + " --tier=" + tier
        + " --only=" + fixture
        + " --class=" + std::to_string(cls)
        + " --coords=" + coords
        + " --fixtures='" + fixtures_dir + "'"
        + " --rotations=" + std::to_string(rotations)
        + " --homfly-cap=" + std::to_string(homfly_cap)
        + " --isolate-child"          // run known-crashing combinations for real
        + (verbose ? " --verbose" : "")
        + " 2>&1";

    FILE * p = popen(cmd.c_str(),"r");
    if( p == nullptr ) { r.crashed = true; return r; }

    char buf[4096];
    while( std::fgets(buf,sizeof(buf),p) != nullptr ) { output += buf; }

    const int status = pclose(p);

    // popen reports the shell's status; a killed child shows up as 128+signal.
    const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if( code > 128 )
    {
        r.crashed = true;
        r.signal  = code - 128;
        return r;
    }

    const std::size_t at = output.rfind("passed ");
    if( at == std::string::npos ) { r.crashed = true; return r; }

    long long a=0,b=0,c=0,d=0;
    if( std::sscanf(output.c_str()+at,
                    "passed %lld, failed %lld, skipped %lld, known-failing %lld",
                    &a,&b,&c,&d) == 4 )
    {
        r.passed = a; r.failed = b; r.skipped = c; r.xfailed = d;
    }
    else { r.crashed = true; }

    return r;
}

/// Load one fixture with the library's own reader and exit. Used as a
/// subprocess probe: the reader can abort the process (see ProbeLibraryReader),
/// so the tier that uses it has to find out from a child first.
template<class LE_T>
static int ReaderProbeMain( const std::string & path )
{
    auto L = kt::LoadWithLibraryReader<LE_T>( std::filesystem::path(path) );
    std::printf("reader-probe ok: %lld edges, %lld components\n",
                (long long)L.EdgeCount(), (long long)L.ComponentCount());
    return 0;
}

/// Is the library's file reader usable in this build at all?
///
/// It is not, at the time of writing: `FromInString` carries two inverted
/// assertions -- `assert(component_ptr_agg.size() != color_agg.size())` and
/// `assert(!coords_may_followQ)` in src/LinkEmbedding2/FromFile.hpp -- that fire
/// on *every* well-formed input, with or without `#color` lines, because one
/// color has always just been recorded for the component being opened. Compiled
/// with -DNDEBUG the same reader parses correctly, so this is the assertions and
/// not the parse.
///
/// Rather than hard-code that as a known failure, probe for it: run the reader
/// in a child process once. When the assertions are fixed the tier simply starts
/// running, with no marker to remember to remove.
static bool ProbeLibraryReader( const std::string & self, const std::string & fixture_path,
                                std::string & why )
{
    const std::string cmd = "'" + self + "' --reader-probe='" + fixture_path + "' 2>&1";

    FILE * p = popen(cmd.c_str(),"r");
    if( p == nullptr ) { why = "could not spawn the probe"; return false; }

    std::string out;
    char buf[1024];
    while( std::fgets(buf,sizeof(buf),p) != nullptr ) { out += buf; }

    const int status = pclose(p);
    const int code   = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if( code == 0 ) { return true; }

    // Trim to the first line: that is the assertion message.
    const std::size_t nl = out.find('\n');
    why = (nl == std::string::npos) ? out : out.substr(0,nl);
    if( why.empty() ) { why = "exited with code " + std::to_string(code); }
    return false;
}

static void Usage()
{
    std::printf(
        "usage: embedding_check [options]\n"
        "\n"
        "  --class=LIST       classes to test, e.g. 1,2,3,4        (default 1,2,3,4)\n"
        "  --coords=LIST      coordinate types: f64,f32,i32,i64    (default f64,f32)\n"
        "  --tier=LIST        census,reader,exact,cross,invariant,symmetry,rotation\n                     (default all)\n"
        "  --fixtures=DIR     fixture directory                    (default ./embeddings)\n"
        "  --only=SUBSTR      only fixtures whose name contains SUBSTR\n"
        "  --rotations=N      random generic projections per fixture (default 3)\n"
        "  --rotation-steps=N successive re-aimings in the rotation tier (default 24)\n"
        "  --homfly-cap=N     max crossings for the HOMFLY oracle  (default 40)\n"
        "  --isolate          run each (tier,fixture,class,coords) in a child process,\n"
        "                     so a crash in the library is reported instead of fatal\n"
        "  --bench            timing mode instead of correctness\n"
        "  --reps=N           repetitions per benchmark point      (default 3)\n"
        "  --list             list fixtures and exit\n"
        "  --verbose          print every check, not just failures\n"
    );
}

static std::vector<std::string> Split( const std::string & s )
{
    std::vector<std::string> out;
    std::string cur;
    for( char ch : s )
    {
        if( ch == ',' ) { if( !cur.empty() ) { out.push_back(cur); } cur.clear(); }
        else            { cur += ch; }
    }
    if( !cur.empty() ) { out.push_back(cur); }
    return out;
}

static bool Arg( const char * a, const char * key, std::string & value )
{
    const std::size_t n = std::strlen(key);
    if( std::strncmp(a,key,n) != 0 ) { return false; }
    value = a+n;
    return true;
}

int main( int argc, char ** argv )
{
    std::setvbuf(stdout,nullptr,_IONBF,0);   // unbuffered: keep output aligned with crashes

    std::vector<int>    classes { 1,2,3,4 };
    std::vector<Coords> coords  { Coords::f64, Coords::f32, Coords::i64 };
    std::vector<std::string> tiers { "census","reader","exact","cross","invariant",
                                     "symmetry","rotation" };

    std::string fixtures_dir = "embeddings";
    std::string only;
    Int  rotations  = 3;
    Int  homfly_cap = 40;
    Int  reps       = 3;
    Int  rot_steps  = 24;
    bool bench      = false;
    bool verbose    = false;
    bool list_only  = false;
    bool isolate    = false;
    bool in_child   = false;
    std::string reader_probe;

    for( int i = 1; i < argc; ++i )
    {
        std::string v;
        if( std::strcmp(argv[i],"--help") == 0 || std::strcmp(argv[i],"-h") == 0 )
        {
            Usage(); return 0;
        }
        else if( std::strcmp(argv[i],"--bench")   == 0 ) { bench = true; }
        else if( std::strcmp(argv[i],"--isolate") == 0 ) { isolate = true; }
        // Set only by the parent on the children it spawns: the child is already
        // in a disposable process, so it must run the known-crashing
        // combinations rather than skip them.
        else if( std::strcmp(argv[i],"--isolate-child") == 0 ) { in_child = true; }
        else if( std::strcmp(argv[i],"--verbose") == 0 ) { verbose = true; }
        else if( std::strcmp(argv[i],"--list")    == 0 ) { list_only = true; }
        else if( Arg(argv[i],"--class=",v) )
        {
            classes.clear();
            for( const auto & s : Split(v) ) { classes.push_back(std::stoi(s)); }
        }
        else if( Arg(argv[i],"--coords=",v) )
        {
            coords.clear();
            for( const auto & s : Split(v) )
            {
                if     ( s == "f64" ) { coords.push_back(Coords::f64); }
                else if( s == "f32" ) { coords.push_back(Coords::f32); }
                else if( s == "i32" ) { coords.push_back(Coords::i32); }
                else if( s == "i64" ) { coords.push_back(Coords::i64); }
                else { std::fprintf(stderr,"unknown coordinate type '%s'\n",s.c_str()); return 2; }
            }
        }
        else if( Arg(argv[i],"--tier=",v) )       { tiers = Split(v); }
        else if( Arg(argv[i],"--fixtures=",v) )   { fixtures_dir = v; }
        else if( Arg(argv[i],"--reader-probe=",v) ) { reader_probe = v; }
        else if( Arg(argv[i],"--only=",v) )       { only = v; }
        else if( Arg(argv[i],"--rotations=",v) )  { rotations = std::stoll(v); }
        else if( Arg(argv[i],"--homfly-cap=",v) ) { homfly_cap = std::stoll(v); }
        else if( Arg(argv[i],"--reps=",v) )       { reps = std::stoll(v); }
        else if( Arg(argv[i],"--rotation-steps=",v) ) { rot_steps = std::stoll(v); }
        else
        {
            std::fprintf(stderr,"unknown option '%s'\n\n",argv[i]);
            Usage();
            return 2;
        }
    }

    if( !reader_probe.empty() )
    {
        return ReaderProbeMain<LE1_f64>(reader_probe);
    }

    auto WantTier = [&](const char * name)
    {
        return std::find(tiers.begin(),tiers.end(),std::string(name)) != tiers.end();
    };

    std::vector<Fixture> fx;
    std::string error;

    if( !CollectFixtures(fixtures_dir,only,fx,error) )
    {
        std::fprintf(stderr,"error: %s\n",error.c_str());
        return 2;
    }

    std::printf("embedding_check: %zu fixture(s) from '%s'\n", fx.size(), fixtures_dir.c_str());

    if( list_only )
    {
        for( auto & f : fx )
        {
            f.census = kt::TakeCensus(f.curve);
            std::printf("  %-26s %5lld vertices  %2lld component(s)  %s  %s\n",
                        f.name.c_str(), (long long)f.curve.VertexCount(),
                        (long long)f.curve.ComponentCount(),
                        f.curve.IntegralQ() ? "integral" : "real    ",
                        f.census.ToString().c_str());
        }
        return 0;
    }

    // --- benchmark mode ---------------------------------------------------
    if( bench )
    {
        std::printf("\n=== benchmark: best of %lld reps, read -> intersect -> build PD ===\n",
                    (long long)reps);

        for( const auto & f : fx )
        {
            for( Coords c : coords )
            {
                for( int cls : classes )
                {
                    std::string why;
                    if( !SupportedQ(cls,c,why) ) { continue; }
                    const std::string label = ClassName(cls,c);
                    WithClass(cls,c,[&]<class LE>(){ BenchOne<LE>(f,label,reps); });
                }
            }
        }
        return 0;
    }

    // --- isolated correctness --------------------------------------------
    if( isolate )
    {
        Tally agg;
        const std::string self = argv[0];

        for( const auto & tier : tiers )
        {
            std::printf("\n=== %s (isolated) ===\n", tier.c_str());

            for( const auto & f : fx )
            {
                for( Coords c : coords )
                {
                    for( int cls : classes )
                    {
                        std::string why;
                        if( !SupportedQ(cls,c,why) ) { continue; }

                        std::string out;
                        auto r = RunChild(self,tier,f.name,cls,CoordsName(c),
                                          fixtures_dir,rotations,homfly_cap,verbose,out);

                        const std::string what =
                            tier + " " + f.name + " " + ClassName(cls,c);

                        if( r.crashed )
                        {
                            const std::string why2 = r.signal
                                ? "crashed with signal " + std::to_string(r.signal)
                                : "child produced no usable result";
                            const std::string xc = f.expect.XCrash(tier.c_str(),cls,CoordsTagOf(ClassName(cls,c)));
                            agg.Verdict(what,false,why2,xc,verbose);
                            continue;
                        }

                        agg.passed  += r.passed;
                        agg.failed  += r.failed;
                        agg.skipped += r.skipped;
                        agg.xfailed += r.xfailed;

                        if( r.failed > Int(0) )
                        {
                            agg.failures.push_back(what + ": see child output below");
                            std::printf("%s", out.c_str());
                        }
                        else if( verbose ) { std::printf("%s", out.c_str()); }

                        // A combination declared as a known crash that did not
                        // crash is news: the marker should come out.
                        const std::string xc = f.expect.XCrash(tier.c_str(),cls,CoordsTagOf(ClassName(cls,c)));
                        if( !xc.empty() )
                        {
                            ++agg.failed;
                            agg.failures.push_back(what + ": XPASS -- marked xcrash ("
                                                   + xc + ") but the child survived;"
                                                   " remove the marker");
                            std::printf("    XPASS %s\n          marked xcrash but survived;"
                                        " remove the marker\n", what.c_str());
                        }
                    }
                }
            }
        }

        std::printf("\n%s\n", std::string(70,'-').c_str());
        std::printf("passed %lld, failed %lld, skipped %lld, known-failing %lld\n",
                    (long long)agg.passed, (long long)agg.failed,
                    (long long)agg.skipped, (long long)agg.xfailed);
        for( const auto & s2 : agg.failures ) { std::printf("  - %s\n", s2.c_str()); }
        if( !agg.xfails.empty() )
        {
            std::printf("\nknown failures:\n");
            for( const auto & s2 : agg.xfails ) { std::printf("  - %s\n", s2.c_str()); }
        }
        return (agg.failed == Int(0)) ? 0 : 1;
    }

    // --- correctness ------------------------------------------------------
    Tally t;

    // Report the combinations that cannot be built, once, up front -- silence
    // here would read as coverage that is not there.
    for( Coords c : coords )
    {
        for( int cls : classes )
        {
            std::string why;
            if( !SupportedQ(cls,c,why) )
            {
                std::printf("  note: %s not built -- %s\n", ClassName(cls,c).c_str(), why.c_str());
            }
        }
    }

    if( WantTier("census") ) { TierCensus(fx,t,verbose); }

    if( WantTier("reader") )
    {
        std::printf("\n=== reader: the library reader agrees with an independent parse ===\n");

        std::string probe_why;
        if( !ProbeLibraryReader(argv[0], fx.front().path, probe_why) )
        {
            ++t.xfailed;
            t.xfails.push_back("reader (whole tier): the library's file reader aborts on "
                               "every input in this build -- " + probe_why);
            std::printf("    XFAIL reader (whole tier)\n"
                        "          the library's file reader aborts on every input:\n"
                        "          %s\n"
                        "          src/LinkEmbedding2/FromFile.hpp has two inverted assertions;\n"
                        "          the parse itself is fine (-DNDEBUG builds read correctly).\n"
                        "          This tier resumes automatically once they are fixed.\n",
                        probe_why.c_str());
            goto reader_done;
        }

        for( const auto & f : fx )
        {
            for( Coords c : coords )
            {
                for( int cls : classes )
                {
                    const std::string label = ClassName(cls,c);
                    WithClass(cls,c,[&]<class LE>(){ ReaderCheckOne<LE>(f,label,t,verbose); });
                }
            }
        }
    }

    reader_done:

    if( WantTier("exact") )
    {
        std::printf("\n=== exact: symbolic perturbation == explicit integer shear ===\n");
        for( const auto & f : fx )
        {
            for( Coords c : coords )
            {
                for( int cls : classes )
                {
                    if( cls == 1 ) { continue; }   // no symbolic perturbation to check
                    const std::string label = ClassName(cls,c);
                    if( SkipKnownCrash(f,"exact",cls,label,in_child,t,verbose) ) { continue; }
                    WithClass(cls,c,[&]<class LE>(){ ExactCheckOne<LE>(f,label,cls,t,verbose); });
                }
            }
        }
    }

    if( WantTier("cross") )
    {
        for( Coords c : coords ) { TierCross(fx,c,rotations,in_child,t,verbose); }
    }

    if( WantTier("invariant") )
    {
        std::printf("\n=== invariant: degenerate projection and generic rotations "
                    "give the same link ===\n");
        for( const auto & f : fx )
        {
            for( Coords c : coords )
            {
                for( int cls : classes )
                {
                    const std::string label = ClassName(cls,c);
                    if( SkipKnownCrash(f,"invariant",cls,label,in_child,t,verbose) ) { continue; }
                    const bool capable = ResolvesDegeneraciesQ(cls);
                    WithClass(cls,c,[&]<class LE>(){
                        InvariantCheckOne<LE>(f,label,cls,capable,rotations,homfly_cap,t,verbose);
                    });
                }
            }
        }
    }

    if( WantTier("symmetry") )
    {
        std::printf("\n=== symmetry: the 24 rotations of the cube give the same knot ===\n");
        for( const auto & f : fx )
        {
            for( Coords c : coords )
            {
                for( int cls : classes )
                {
                    const std::string label = ClassName(cls,c);
                    if( SkipKnownCrash(f,"symmetry",cls,label,in_child,t,verbose) ) { continue; }
                    const bool capable = ResolvesDegeneraciesQ(cls);
                    WithClass(cls,c,[&]<class LE>(){
                        SymmetryCheckOne<LE>(f,label,cls,capable,homfly_cap,t,verbose);
                    });
                }
            }
        }
    }

    if( WantTier("rotation") )
    {
        std::printf("\n=== rotation: repeated re-aiming does not make the knot grow ===\n");
        for( const auto & f : fx )
        {
            for( Coords c : coords )
            {
                for( int cls : classes )
                {
                    const std::string label = ClassName(cls,c);
                    if( SkipKnownCrash(f,"rotation",cls,label,in_child,t,verbose) ) { continue; }
                    WithClass(cls,c,[&]<class LE>(){
                        RotationCheckOne<LE>(f,label,cls,rot_steps,homfly_cap,t,verbose);
                    });
                }
            }
        }
    }

    std::printf("\n%s\n", std::string(70,'-').c_str());
    std::printf("passed %lld, failed %lld, skipped %lld, known-failing %lld\n",
                (long long)t.passed, (long long)t.failed,
                (long long)t.skipped, (long long)t.xfailed);

    if( !t.failures.empty() )
    {
        std::printf("\nfailures:\n");
        for( const auto & s : t.failures ) { std::printf("  - %s\n", s.c_str()); }
    }
    if( !t.xfails.empty() )
    {
        std::printf("\nknown failures (declared xfail in the fixture's .expect file --\n"
                    "these are defects in the code under test, not in the test):\n");
        for( const auto & s : t.xfails ) { std::printf("  - %s\n", s.c_str()); }
    }
    if( verbose && !t.skips.empty() )
    {
        std::printf("\nskipped:\n");
        for( const auto & s : t.skips ) { std::printf("  - %s\n", s.c_str()); }
    }

    return (t.failed == Int(0)) ? 0 : 1;
}
