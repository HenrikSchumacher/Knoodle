// ordering_probe -- is the FindShortestPath ordering regularity a property of
// legal pass moves, or only of that one producer?
//
// BACKGROUND. `PassSimplifier::Reroute` recycles the label `a_1` at each
// interior crossing it processes. Its `path` was captured before any mutation,
// so a LATER corridor step naming an EARLIER-consumed label operates on the
// wrong arc and yields a planar, CheckAll-clean diagram of a different knot
// (handoff/reroute-arc-label-aliasing, Knoodle PR #30, closed 2026-08-12).
//
// Write x_i for the i-th interior crossing of the strand (1-based), t_i for
// its transversal, and j for the corridor step naming an arc of t_i (1-based).
// The aliasing needs j > i -- the corridor LAGS the strand. Over 4,612,650
// corridors emitted by `FindShortestPath`, j > i never occurred; j < i
// occurred 95,783 times. Nobody knows why. Two candidate explanations are
// already dead: it is not shortness (3,612,148 of those corridors were
// non-improving and still never lagged) and it is not a property of legal
// corridors (the PR #30 reproducer is a valid dual walk that lags, i=2 j=4).
//
// So the open question is whether a lagging corridor exists that production
// would actually accept:
//
//     strictly shortening (k < L-1), well-formed, and lagging
//
// A hit means the bug is reachable in-contract and PR #30 should be reopened.
//
// THIS FILE IS PHASE A: the classifier, plus its validation against the known
// positive control. A zero from a search is meaningless unless the detector is
// known to fire on the one case we are certain about, so the search (phase B)
// is gated on the control passing here.

#include "../Knoodle.hpp"
#include "pass_fixtures.hpp"

#include <cstdio>
#include <map>
#include <string>
#include <vector>

using Int    = std::int64_t;
using PD_T   = Knoodle::PlanarDiagram<Int>;
using Desc_T = Knoodle::PassDescriptor<Int>;

static bool ok = true;

static void check( bool passedQ, const std::string & what )
{
    std::printf("  %-62s %s\n", what.c_str(), passedQ ? "OK" : "FAILED");
    if( !passedQ ) { ok = false; }
}

//==========================================================================
// The classifier
//==========================================================================

/*!@brief One (crossing, corridor step) incidence: corridor step `j` crosses an
 * arc that is a transversal half at the strand's `i`-th interior crossing.
 */
struct Incidence
{
    Int i;      // interior crossing index along the strand, 1-based
    Int j;      // corridor step index, 1-based
    Int arc;    // the transversal half named
    Int cx;     // the crossing x_i
};

/*!@brief The two transversal arcs at interior crossing `c`, i.e. the two of
 * its four arc slots not occupied by the strand arcs `w_in` and `w_out`.
 */
static std::vector<Int> TransversalArcs(
    Knoodle::cref<PD_T> pd, const Int c, const Int w_in, const Int w_out )
{
    std::vector<Int> t;

    for( bool io : { PD_T::Out, PD_T::In } )
    {
        for( bool side : { PD_T::Left, PD_T::Right } )
        {
            const Int a = pd.Crossings()(c, io, side);
            if( (a == w_in) || (a == w_out) ) { continue; }
            t.push_back(a);
        }
    }
    return t;
}

/*!@brief Every (i,j) incidence between the descriptor's corridor and the
 * transversals of its strand. Indices are 1-based so that they match the
 * loop counter `p` in `Reroute`, which pairs corridor step p with the p-th
 * interior crossing it processes.
 */
static std::vector<Incidence> Classify( Knoodle::cref<PD_T> pd, Knoodle::cref<Desc_T> d )
{
    std::vector<Incidence> out;

    const std::size_t L = d.strand.size();
    if( L < 2 ) { return out; }

    // arc -> the interior-crossing indices at which it is a transversal half.
    // An arc can be a transversal at more than one crossing of the same
    // strand, so this is a multimap in spirit.
    std::map<Int,std::vector<Int>> t_of;

    for( std::size_t m = 1; m < L; ++m )
    {
        const Int i     = static_cast<Int>(m);       // 1-based crossing index
        const Int w_in  = Desc_T::ArcOf(d.strand[m-1]);
        const Int w_out = Desc_T::ArcOf(d.strand[m]);
        const Int c     = Desc_T::DarcHeadCrossing(pd, d.strand[m-1]);

        for( Int a : TransversalArcs(pd, c, w_in, w_out) )
        {
            t_of[a].push_back(i);
        }
    }

    for( std::size_t s = 0; s < d.cross.size(); ++s )
    {
        const Int j = static_cast<Int>(s) + Int(1);  // 1-based corridor step
        const Int a = Desc_T::ArcOf(d.cross[s]);

        auto it = t_of.find(a);
        if( it == t_of.end() ) { continue; }

        for( Int i : it->second )
        {
            const Int c = Desc_T::DarcHeadCrossing(pd, d.strand[static_cast<std::size_t>(i)-1]);
            out.push_back( Incidence{ i, j, a, c } );
        }
    }

    return out;
}

/*!@brief Does this descriptor LAG -- i.e. does any corridor step name the
 * transversal of an already-processed crossing? That is the aliasing trigger.
 */
static bool LagsQ( Knoodle::cref<std::vector<Incidence>> inc )
{
    for( Knoodle::cref<Incidence> e : inc ) { if( e.j > e.i ) { return true; } }
    return false;
}

//==========================================================================
// Phase B: the search
//==========================================================================

/*!@brief Dual-graph BFS distance from every face to `target`. Used to prune
 * the corridor DFS: a partial corridor with `r` steps left cannot reach the
 * target if it is further than `r` away.
 */
static std::vector<Int> DualDistances( Knoodle::cref<PD_T> pd, const Int target )
{
    const auto & F_dA = pd.FaceDarcs();
    const Int    nf   = pd.FaceCount();

    std::vector<Int> dist( static_cast<std::size_t>(nf), Int(-1) );
    if( (target < Int(0)) || (target >= nf) ) { return dist; }

    std::vector<Int> queue { target };
    dist[static_cast<std::size_t>(target)] = Int(0);

    for( std::size_t head = 0; head < queue.size(); ++head )
    {
        const Int f = queue[head];
        for( Int da : F_dA[f] )
        {
            const Int g = Desc_T::LeftFace(pd, da ^ Int(1));
            if( (g < Int(0)) || (g >= nf) ) { continue; }
            if( dist[static_cast<std::size_t>(g)] >= Int(0) ) { continue; }
            dist[static_cast<std::size_t>(g)] = dist[static_cast<std::size_t>(f)] + Int(1);
            queue.push_back(g);
        }
    }
    return dist;
}

struct Stats
{
    long long strands      = 0;
    long long corridors    = 0;   // well-formed, strictly shortening
    long long with_inc     = 0;   // ... that touch a transversal at all
    long long j_lt_i       = 0;   // incidences, by bucket
    long long j_eq_i       = 0;
    long long j_gt_i       = 0;
    long long lagging      = 0;   // corridors with at least one j > i
    long long plain_pass   = 0;   // ... of which kind=pass
    long long middlepass   = 0;   // ... of which kind=middlepass

    // Lagging descriptors kept for phase C (sign surgery). Capped: we only
    // need enough to convert, not the whole population.
    std::vector<Desc_T> hits;
    static constexpr std::size_t hit_cap = 400;
};

/*!@brief Enumerate every strictly shortening, well-formed corridor for one
 * (strand, depart, land) triple and classify it. Emits at most `report_cap`
 * lagging descriptors to stdout.
 */
struct Searcher
{
    Knoodle::cref<PD_T>     pd;
    const std::vector<Int> & strand;   // darcs
    Int                      depart;
    Int                      land;
    Int                      target;
    std::vector<Int>         dist;
    std::vector<bool>        arc_used;
    std::vector<Int>         corridor;
    Stats &                  st;
    int &                    reported;
    int                      report_cap;

    void Emit()
    {
        if( corridor.empty() ) { return; }

        // Try, in order:
        //   kind=pass       -- check 5 forces the tags to W's own uniform
        //                      role, so try both and keep what the library
        //                      accepts (at most one can pass);
        //   kind=middlepass -- check 5 dropped, per-crossing tags allowed.
        //
        // The middlepass tier is the one that matters here: a uniform
        // over/under strand is a ~2^-(L-1) event, so plain passes are far too
        // rare to search, AND the live exposure is `ReroutePrescribed`, which
        // is exactly the middlepass applier. Counted separately so the two
        // tiers never get conflated.
        struct Attempt { bool mpQ; bool overval; };

        for( Attempt at : { Attempt{false,false}, Attempt{false,true},
                            Attempt{true ,false} } )
        {
            Desc_T d;
            d.strand      = strand;
            d.depart      = depart;
            d.cross       = corridor;
            d.over.assign(corridor.size(), at.overval);
            d.land        = land;
            d.middlepassQ = at.mpQ;

            std::string why;
            if( !d.WellFormedQ(pd, why) ) { continue; }

            ++st.corridors;
            if( at.mpQ ) { ++st.middlepass; } else { ++st.plain_pass; }

            const std::vector<Incidence> inc = Classify(pd, d);
            if( !inc.empty() ) { ++st.with_inc; }

            bool lagQ = false;
            for( Knoodle::cref<Incidence> e : inc )
            {
                if     ( e.j <  e.i ) { ++st.j_lt_i; }
                else if( e.j == e.i ) { ++st.j_eq_i; }
                else                  { ++st.j_gt_i; lagQ = true; }
            }

            if( lagQ )
            {
                ++st.lagging;
                if( st.hits.size() < Stats::hit_cap ) { st.hits.push_back(d); }
                if( reported < report_cap )
                {
                    ++reported;
                    std::printf("\n  *** LAGGING, STRICTLY SHORTENING corridor ***\n");
                    std::printf("      kind=%s L=%lld k=%lld\n",
                        at.mpQ ? "middlepass" : "pass",
                        (long long)strand.size(), (long long)corridor.size());
                    std::printf("      %s\n", d.ToString().c_str());
                    for( Knoodle::cref<Incidence> e : inc )
                    {
                        if( e.j > e.i )
                        {
                            std::printf("      lag: i=%lld j=%lld arc=%lld x=%lld"
                                "   [x row: %lld %lld %lld %lld]"
                                "  strand arcs at x: %lld,%lld\n",
                                (long long)e.i, (long long)e.j,
                                (long long)e.arc, (long long)e.cx,
                                (long long)pd.Crossings()(e.cx,PD_T::Out,PD_T::Left),
                                (long long)pd.Crossings()(e.cx,PD_T::Out,PD_T::Right),
                                (long long)pd.Crossings()(e.cx,PD_T::In ,PD_T::Left),
                                (long long)pd.Crossings()(e.cx,PD_T::In ,PD_T::Right),
                                (long long)Desc_T::ArcOf(strand[(std::size_t)e.i-1]),
                                (long long)Desc_T::ArcOf(strand[(std::size_t)e.i]));
                        }
                    }
                }
            }
            break;   // one tag assignment accepted; do not double-count
        }
    }

    void Dfs( const Int f, const Int depth_left )
    {
        if( f == target ) { Emit(); }
        if( depth_left <= Int(0) ) { return; }

        const Int d_here = dist[static_cast<std::size_t>(f)];
        if( (d_here < Int(0)) || (d_here > depth_left) ) { return; }

        for( Int da : pd.FaceDarcs()[f] )
        {
            const Int a = Desc_T::ArcOf(da);
            if( arc_used[static_cast<std::size_t>(a)] ) { continue; }

            const Int g = Desc_T::LeftFace(pd, da ^ Int(1));
            if( g < Int(0) ) { continue; }

            arc_used[static_cast<std::size_t>(a)] = true;
            corridor.push_back(da);

            Dfs(g, depth_left - Int(1));

            corridor.pop_back();
            arc_used[static_cast<std::size_t>(a)] = false;
        }
    }
};

static Stats RunSearch( Knoodle::cref<PD_T> pd, const Int max_L, const int report_cap )
{
    Stats st;
    int   reported = 0;

    const Int m_arcs = pd.MaxArcCount();

    for( Int a0 = 0; a0 < m_arcs; ++a0 )
    {
        if( !pd.ArcActiveQ(a0) ) { continue; }

        // Strands are runs of consecutive arcs along the orientation. A plain
        // pass needs k >= 1 and k <= L-2 (strictly shortening), so L >= 3.
        std::vector<Int> arcs { a0 };

        for( Int L = 2; L <= max_L; ++L )
        {
            const Int nxt = pd.NextArc(arcs.back(), PD_T::Head);
            if( !pd.ArcActiveQ(nxt) ) { break; }

            bool repeatQ = false;
            for( Int x : arcs ) { if( x == nxt ) { repeatQ = true; break; } }
            if( repeatQ ) { break; }          // strand must be simple

            arcs.push_back(nxt);
            if( L < 3 ) { continue; }

            ++st.strands;

            std::vector<Int> strand;
            strand.reserve(arcs.size());
            for( Int x : arcs ) { strand.push_back( Int(2) * x + Int(1) ); }

            const Int max_k = L - Int(2);     // STRICTLY shortening

            for( Int dep_d : { Int(2)*arcs.front(), Int(2)*arcs.front()+Int(1) } )
            {
                for( Int lnd_d : { Int(2)*arcs.back(), Int(2)*arcs.back()+Int(1) } )
                {
                    const Int f0 = Desc_T::LeftFace(pd, dep_d);
                    const Int ft = Desc_T::LeftFace(pd, lnd_d);
                    if( (f0 < Int(0)) || (ft < Int(0)) ) { continue; }

                    Searcher s {
                        pd, strand, dep_d, lnd_d, ft,
                        DualDistances(pd, ft),
                        std::vector<bool>(static_cast<std::size_t>(m_arcs), false),
                        {}, st, reported, report_cap
                    };

                    // The corridor may not cross the strand's own arcs.
                    for( Int x : arcs ) { s.arc_used[static_cast<std::size_t>(x)] = true; }

                    s.Dfs(f0, max_k);
                }
            }
        }
    }

    std::printf("\n=== search: strictly shortening, well-formed corridors ===\n");
    std::printf("  strands examined                      %lld\n", st.strands);
    std::printf("  well-formed corridors (k <= L-2)      %lld\n", st.corridors);
    std::printf("    of which kind=pass                  %lld\n", st.plain_pass);
    std::printf("    of which kind=middlepass            %lld\n", st.middlepass);
    std::printf("  ... touching a transversal at all     %lld\n", st.with_inc);
    std::printf("  incidences at j <  i (runs ahead)     %lld\n", st.j_lt_i);
    std::printf("  incidences at j == i (guarded case)   %lld\n", st.j_eq_i);
    std::printf("  incidences at j >  i (THE TRIGGER)    %lld\n", st.j_gt_i);
    std::printf("  corridors that LAG                    %lld\n", st.lagging);
    return st;
}

//==========================================================================
// Phase C: sign surgery -- turn a lagging middlepass hit into a plain pass
//==========================================================================

/*!@brief Does the strand run UNDER at crossing `x`, entering on `w_in`?
 * The understrand enters at (In,Right) for a right-handed crossing and
 * (In,Left) for a left-handed one. Validated against the fixture, whose
 * strand 40..48 is a known genuine underpass at all 8 interior crossings.
 */
static bool StrandUnderQ( Knoodle::cref<PD_T> pd, const Int x, const Int w_in )
{
    const bool rhQ =
        (pd.CrossingStates()[x] == Knoodle::CrossingState_T::RightHanded);
    return pd.Crossings()(x, PD_T::In, rhQ ? PD_T::Right : PD_T::Left) == w_in;
}

/*!@brief Crossing change on one PD-code row: the over-strand becomes the
 * under-strand. A row must begin at the INCOMING under-arc, so the tuple is
 * re-rooted at the incoming OVER-arc -- which is X[3] for a right-handed
 * crossing and X[1] for a left-handed one (the convention recorded in
 * handoff/reroute-arc-label-aliasing/analyze.py) -- and the sign negates.
 *
 * Merely negating the sign does NOT work: the sign decides which of X[1],X[3]
 * is the OUTGOING over-arc, so flipping it alone leaves an arc with two tails
 * and the code no longer parses.
 *
 * Verified: valid diagram, face count preserved (so the corridor survives),
 * role flipped, and applying it twice is the identity.
 */
static void ChangeCrossing( std::vector<Int> & code, const Int x )
{
    Int * r = &code[static_cast<std::size_t>(5*x)];
    const Int a = r[0], b = r[1], c = r[2], d = r[3], s = r[4];
    if( s > Int(0) ) { r[0]=d; r[1]=a; r[2]=b; r[3]=c; }   // over ran d -> b
    else             { r[0]=b; r[1]=c; r[2]=d; r[3]=a; }   // over ran b -> d
    r[4] = -s;
}

/*!@brief For each lagging hit, flip the crossings where W runs over so that W
 * becomes a uniform UNDERpass, and check the descriptor is then a well-formed
 * plain `kind=pass` on the modified diagram -- with the lag intact.
 *
 * This is sound because handedness does not touch the planar graph: checks
 * 1-4 and 6 are pure face combinatorics and cannot be disturbed. Only check 5
 * depends on the signs, which is exactly what we are fixing.
 */
static void ConvertHits(
    Knoodle::cref<PD_T> pd0, Knoodle::cref<std::vector<Int>> code0,
    Knoodle::cref<std::vector<Desc_T>> hits, const int report_cap )
{
    long long tried = 0, converted = 0, lag_kept = 0, faces_kept = 0;
    int reported = 0;

    for( Knoodle::cref<Desc_T> d : hits )
    {
        ++tried;

        std::vector<Int> code = code0;
        for( std::size_t m = 1; m < d.strand.size(); ++m )
        {
            const Int w_in = Desc_T::ArcOf(d.strand[m-1]);
            const Int x    = Desc_T::DarcHeadCrossing(pd0, d.strand[m-1]);
            if( !StrandUnderQ(pd0, x, w_in) ) { ChangeCrossing(code, x); }
        }

        PD_T pd = PD_T::FromSignedPDCode(
            &code[0], static_cast<Int>(code.size()/5), false, false );

        if( !pd.CheckAll() ) { continue; }
        if( pd.FaceCount() == pd0.FaceCount() ) { ++faces_kept; }

        Desc_T e = d;
        e.middlepassQ = false;
        e.over.assign(d.cross.size(), false);   // W runs under everywhere

        std::string why;
        if( !e.WellFormedQ(pd, why) ) { continue; }
        ++converted;

        const std::vector<Incidence> inc = Classify(pd, e);
        if( !LagsQ(inc) ) { continue; }
        ++lag_kept;

        if( reported < report_cap )
        {
            ++reported;
            std::printf("\n  *** PLAIN kind=pass, STRICTLY SHORTENING, LAGGING ***\n");
            std::printf("      L=%lld k=%lld\n",
                (long long)e.strand.size(), (long long)e.cross.size());
            std::printf("      %s\n", e.ToString().c_str());
            for( Knoodle::cref<Incidence> z : inc )
            {
                if( z.j > z.i )
                {
                    std::printf("      lag: i=%lld j=%lld arc=%lld x=%lld\n",
                        (long long)z.i, (long long)z.j,
                        (long long)z.arc, (long long)z.cx);
                }
            }
        }
    }

    std::printf("\n=== phase C: sign surgery -> plain kind=pass ===\n");
    std::printf("  lagging hits tried                    %lld\n", tried);
    std::printf("  rebuilt with face count preserved     %lld\n", faces_kept);
    std::printf("  well-formed as kind=pass afterwards   %lld\n", converted);
    std::printf("  ... and STILL LAGGING                 %lld\n", lag_kept);
}

int main()
{
    PD_T pd = PD_T::FromSignedPDCode(
        &zf061098_underpass[0],
        static_cast<Int>(zf061098_underpass.size() / 5),
        false, false );

    std::printf("=== diagram: zf061098 underpass ===\n");
    check(pd.CheckAll(),            "fixture: CheckAll passes");
    check(pd.CrossingCount() == Int(73), "fixture: 73 crossings");

    // ---- the positive control ------------------------------------------
    // The PR #30 reproducer. ROUND-1-RESPONSE established, independently of
    // Knoodle, that crossing 48 has row [41,115,42,114,+1]; it sits between
    // strand arcs 41 and 42, so it is the strand's 2nd interior crossing, and
    // corridor step 4 names arc 115 -- one of its two transversal halves.
    // Hence i = 2, j = 4. If the classifier does not reproduce exactly that,
    // nothing it reports about any other descriptor is worth reading.
    {
        // NOTE `land=96`, not the `land=19` that appears in the PR #30 body:
        // the same move, respelled when the descriptor normal form landed
        // (depart/land must be darcs OF the strand's first/last arc, and
        // 96/2 = 48 is the last arc). The old spelling now fails check 4.
        const std::string spec =
            "strand=81,83,85,87,89,91,93,95,97 depart=80 "
            "cross=7:u,42:u,8:u,230:u,131:u,281:u,260:u,290:u land=96";

        Desc_T d;
        std::string err;
        const bool parsedQ = Desc_T::Parse(spec, d, err);
        check(parsedQ, "control: descriptor parses" + (parsedQ ? "" : " -- " + err));

        if( parsedQ )
        {
            std::string why;
            const bool wfQ = d.WellFormedQ(pd, why);
            check(wfQ, "control: well-formed" + (wfQ ? "" : " -- " + why));

            const std::vector<Incidence> inc = Classify(pd, d);

            std::printf("  control incidences (i, j, arc, crossing):\n");
            for( Knoodle::cref<Incidence> e : inc )
            {
                std::printf("    i=%lld  j=%lld  arc=%lld  x=%lld  %s\n",
                    (long long)e.i, (long long)e.j,
                    (long long)e.arc, (long long)e.cx,
                    (e.j > e.i) ? "<-- LAG" : "");
            }

            bool found_2_4 = false;
            for( Knoodle::cref<Incidence> e : inc )
            {
                if( (e.i == Int(2)) && (e.j == Int(4)) && (e.arc == Int(115)) )
                {
                    found_2_4 = true;
                }
            }

            check(found_2_4,
                  "control: detector fires at i=2, j=4 on arc 115");
            check(LagsQ(inc), "control: descriptor is classified as lagging");

            // The strand has 9 arcs and the corridor 8 crossings, so this is
            // the saving-zero case Henrik calls out of scope. Recording it
            // here makes the contrast with a search hit explicit.
            const Int L = static_cast<Int>(d.strand.size());
            const Int k = static_cast<Int>(d.cross.size());
            std::printf("  control: L=%lld, k=%lld, k < L-1 (strictly shortening)? %s\n",
                (long long)L, (long long)k, (k < L - Int(1)) ? "YES" : "NO");
            check(k == L - Int(1),
                  "control: is saving-zero (k == L-1), i.e. out of contract");
        }
    }

    // Phase B is gated on the control: a zero from the search means nothing
    // unless the detector is known to fire on the case we are sure about.
    if( !ok )
    {
        std::printf("PHASE A FAILED -- not running the search\n");
        return 1;
    }
    std::printf("PHASE A OK\n");

    const Stats st = RunSearch(pd, /*max_L=*/Int(10), /*report_cap=*/3);

    std::vector<Int> code0( zf061098_underpass.begin(), zf061098_underpass.end() );
    ConvertHits(pd, code0, st.hits, /*report_cap=*/3);

    return ok ? 0 : 1;
}
