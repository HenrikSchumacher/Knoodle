// pass_view_check -- the two-deletions contract, checked against the drawing.
//
// docs/move-descriptor.md says a pass-move picture is a superposition of two
// states, each one deletion away:
//
//   delete the corridor (the p-arcs)  ->  an embedding of the diagram we were
//                                         handed;
//   delete the strand W (the w-arcs)  ->  an embedding of the diagram the move
//                                         produces.
//
// Every previous check of that claim was indirect: `--trace --verify` compared
// `AfterDiagram` to the next snapshot by MacLeod code, which sees "wrong knot"
// but never "right knot, wrong picture", localizes nothing, and does not exist
// for links. This test checks the claim where it is actually made -- in the
// drawing. Each view is RENDERED, then PARSED BACK into a diagram by
// tools/drawing_extractor.hpp (which reads structure from the characters and
// never consults the diagram it is checking), and the result is compared
// port-by-port to what that view is supposed to be.
//
// The correspondence handed to the comparison is geometric, so the check is
// much stronger than "isomorphic to": each parsed crossing is matched to the
// crossing whose GRID CELL it was read from -- surviving crossings by their
// drawn position, the corridor's new crossings by their order along the
// corridor (which is how `AfterDiagram` numbers them). A corridor attached to
// the wrong port of an anchor therefore fails even when what it draws is a
// perfectly legal diagram of the right knot.
//
// Build: `make pass_view_check` in tools/.

#include "../Knoodle.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../tools/pass_view.hpp"
#include "../tools/diagram_agreement.hpp"
#include "../tools/drawing_extractor.hpp"
#include "../tools/find_pass.hpp"
#include "pass_fixtures.hpp"

using Int         = std::int64_t;
using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PS_T        = Knoodle::PassSimplifier<Int>;
using PD_T        = Knoodle::PlanarDiagram<Int>;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
using Deco_T      = Knoodle::OrthoDecorate<PD_T>;
using Extract_T   = KnoodleDrawIO::DrawingExtractor<PD_T>;
using View        = KnoodlePassView::PassViewKind;

static bool ok = true;

static constexpr Int margin = Int(2);

static OrthoDraw_T::Settings_T GridSettings( Int xg, Int yg )
{
    OrthoDraw_T::Settings_T s{};
    s.x_grid_size = xg;
    s.y_grid_size = yg;
    s.x_gap_size  = Int(1);
    s.y_gap_size  = Int(1);
    return s;
}

struct Case_T
{
    const char *     name;
    const PD_T *     pd;
    const char *     spec;
};

static void RunCase( const Case_T & kase, Int xg, Int yg )
{
    char tag[128];
    std::snprintf(tag, sizeof tag, "%-18s %2lldx%-2lld",
        kase.name, (long long)xg, (long long)yg);

    Deco_T::PassMove_T mv;
    std::string err;
    if( !Deco_T::PassMove_T::Parse(kase.spec, mv, err) )
    {
        std::printf("  %s  descriptor did not parse: %s\n", tag, err.c_str());
        ok = false;
        return;
    }

    const PD_T & pd = *kase.pd;

    OrthoDraw_T H(pd, Int(-1), GridSettings(xg,yg));
    Deco_T deco(H, margin);

    auto pr = deco.RoutePassMove(pd, mv);
    if( !pr.validQ )
    {
        std::printf("  %s  routing failed: %s\n", tag, pr.why.c_str());
        ok = false;
        return;
    }

    std::string why;
    PD_T after = deco.AfterDiagram(pd, mv, why);
    if( !why.empty() )
    {
        std::printf("  %s  AfterDiagram failed: %s\n", tag, why.c_str());
        ok = false;
        return;
    }

    // Both deletions, through the very routine knoodledraw --verify calls, so
    // the test cannot drift from what the tool checks.
    if( !KnoodlePassView::CheckBothDeletions<PD_T>(
            H, deco, pd, mv, pr, after, margin, why) )
    {
        std::printf("  %s  %s\n", tag, why.c_str());
        ok = false;
        return;
    }

    std::printf("  %s  both deletions OK (k=%zu)\n", tag, mv.cross.size());
}

// The oracle has to be able to FAIL, or it proves nothing. Each corruption
// below is a plausible drawing bug; each must be caught, and caught for the
// right reason.
static void RunNegativeTests( const PD_T & pd, const char * spec )
{
    Deco_T::PassMove_T mv;
    std::string err, why;
    if( !Deco_T::PassMove_T::Parse(spec, mv, err) ) { ok = false; return; }

    OrthoDraw_T H(pd, Int(-1), GridSettings(Int(4),Int(2)));
    Deco_T deco(H, margin);

    auto pr = deco.RoutePassMove(pd, mv);
    if( !pr.validQ ) { ok = false; return; }

    PD_T after = deco.AfterDiagram(pd, mv, why);
    if( !why.empty() ) { ok = false; return; }

    auto expect_caught = [&]( const std::string & canvas, Int n_x, Int n_y,
                              const PD_T & truth, bool afterQ,
                              const char * name )
    {
        auto R = Extract_T::Extract(canvas, n_x, n_y);

        std::string w;
        bool caught = false;
        std::string reason;

        if( !R.okQ ) { caught = true; reason = "parse: " + R.why; }
        else
        {
            std::vector<std::array<Int,2>> seeds;
            const bool seedQ = KnoodlePassView::PassViewSeeds<PD_T>(
                R, pd, H, afterQ ? &pr : nullptr, margin, seeds, w);
            if( !seedQ ) { caught = true; reason = "seeds: " + w; }
            else if( !DiagramsAgreeQ(R.pd, truth, seeds, w) )
            {
                caught = true; reason = "compare: " + w;
            }
        }

        std::printf("  negative %-22s %s%s%s\n", name,
            caught ? "caught" : "*** NOT CAUGHT ***",
            caught ? " -- " : "", caught ? reason.c_str() : "");

        if( !caught ) { ok = false; }
    };

    // 1. --pass-view=both is not a single deletion: the dot is a branch point
    //    there, so the parser must refuse it rather than pick a branch.
    {
        auto c = KnoodlePassView::RenderPassView<PD_T>(
            H, mv, pr, deco, margin, View::Both);
        expect_caught(c.chars, c.n_x, c.n_y, pd, false, "both-view refused");
    }

    // 2. A healed transversal left un-drawn -- the deletion of W erased the
    //    strand but forgot to restore the strand it used to cross.
    {
        auto c = KnoodlePassView::RenderPassView<PD_T>(
            H, mv, pr, deco, margin, View::After);

        // The interior crossing of W is where the strand's first darc ends.
        const Int a = Deco_T::PassMove_T::ArcOf(mv.strand.front());
        const auto & A_V = H.ArcVertices();
        const auto & V    = H.VertexCoordinates();
        auto verts = A_V[a];
        const Int v = *(verts.end() - 1);
        const Int x = V(v,0) + margin, y = V(v,1) + margin;
        const auto i = static_cast<std::size_t>(x + c.n_x * (c.n_y - Int(1) - y));

        std::string bad = c.chars;
        if( i < bad.size() ) { bad[i] = ' '; }
        expect_caught(bad, c.n_x, c.n_y, after, true, "heal patch missing");
    }

    // 3. A corridor crossing drawn with the wrong strand on top. The diagram
    //    stays perfectly legal -- only the handedness changes -- which is
    //    exactly the class of error an invariant-level check would wave past.
    if( !pr.route.crossing_indices.empty() )
    {
        auto c = KnoodlePassView::RenderPassView<PD_T>(
            H, mv, pr, deco, margin, View::After);

        const auto & cell = pr.route.path[static_cast<std::size_t>(
            pr.route.crossing_indices[0])];
        const auto i = static_cast<std::size_t>(
            cell[0] + c.n_x * (c.n_y - Int(1) - cell[1]));

        std::string bad = c.chars;
        if( i < bad.size() )
        {
            // Swap which axis is drawn through the crossing cell.
            const char g = bad[i];
            const bool horizQ = (g == '-') || (g == '=') || (g == '<') || (g == '>');
            bad[i] = horizQ ? '|' : '-';
        }
        expect_caught(bad, c.n_x, c.n_y, after, true, "over/under flipped");
    }
}

// Before any of the pass-move machinery is involved: can the extractor read
// back an ORDINARY drawing? This is the parser's own regression test, and it
// is where the handedness rule and the link handling are pinned down --
// mirrored knots (both crossing signs) and two-component links, neither of
// which the pass-move fixtures cover.
static void RunPlainRoundTrip(
    const char * name, std::vector<Int> code, Int n, Int xg, Int yg )
{
    PD_T pd = PD_T::FromSignedPDCode(code.data(), n);

    OrthoDraw_T H(pd, Int(-1), GridSettings(xg,yg));

    std::string canvas = H.DiagramString();
    std::replace(canvas.begin(), canvas.end(), '.', ' ');

    const Int n_x = H.Width()  * xg + Int(2);
    const Int n_y = H.Height() * yg + Int(1);

    auto R = Extract_T::Extract(canvas, n_x, n_y);

    if( !R.okQ )
    {
        std::printf("  plain %-16s %2lldx%-2lld  parse failed: %s\n",
            name, (long long)xg, (long long)yg, R.why.c_str());
        ok = false;
        return;
    }

    // Same geometric seeding as the views, but with no margin.
    const auto & V = H.VertexCoordinates();
    std::vector<std::array<Int,2>> seeds;
    for( std::size_t i = 0; i < R.crossing_cell.size(); ++i )
    {
        const Int x = R.crossing_cell[i][0], y = R.crossing_cell[i][1];
        Int match = Int(-1);
        for( Int c = 0; c < pd.MaxCrossingCount(); ++c )
        {
            if( !pd.CrossingActiveQ(c) ) { continue; }
            if( (V(c,0) == x) && (V(c,1) == y) ) { match = c; break; }
        }
        if( match < 0 )
        {
            std::printf("  plain %-16s %2lldx%-2lld  crossing at (%lld,%lld)"
                " has no counterpart\n", name, (long long)xg, (long long)yg,
                (long long)x, (long long)y);
            ok = false;
            return;
        }
        seeds.push_back({ static_cast<Int>(i), match });
    }

    std::string why;
    if( !DiagramsAgreeQ(R.pd, pd, seeds, why) )
    {
        std::printf("  plain %-16s %2lldx%-2lld  disagrees: %s\n",
            name, (long long)xg, (long long)yg, why.c_str());
        ok = false;
        return;
    }
    if( !R.pd.CheckAll() )
    {
        std::printf("  plain %-16s %2lldx%-2lld  CheckAll failed\n",
            name, (long long)xg, (long long)yg);
        ok = false;
    }
}

// Per-crossing over/under has nowhere to go in `Pass_T`, which carries ONE
// `overQ` for the whole move. Check 5 does not catch that, because
// `kind=middlepass` exists precisely to drop check 5 -- so `ToPassAndPath`
// has to refuse a mixed-tag corridor itself. Before it did, it read `over[0]`
// and discarded the rest, handing the applier a DIFFERENT move that happened
// to typecheck. That is the exact shape of the bug that produced the zf075886
// incident downstream, so it is worth a test of its own.
static void RunMixedTagTests( const PD_T & pd )
{
    // PassSimplifier's nested Pass_T / Path_T cannot be NAMED until a
    // PassSimplifier has been constructed -- it and PlanarDiagramComplex are
    // mutually dependent, so the nested types are not complete before then.
    // Build PD -> PDC -> PS first, exactly as oracle_vs_reroute does.
    PDC_T pdc { PD_T(pd) };
    PS_T  ps( pdc, Knoodle::DijkstraStrategy_T::Bidirectional );
    (void)ps;

    auto attempt = [&]( const char * spec, bool middlepassQ, bool expect_okQ,
                        const char * name )
    {
        Deco_T::PassMove_T mv;
        std::string err, why;
        if( !Deco_T::PassMove_T::Parse(spec, mv, err) )
        {
            std::printf("  tags %-22s parse failed: %s\n", name, err.c_str());
            ok = false;
            return;
        }
        mv.middlepassQ = middlepassQ;

        // Well-formedness first, so a refusal below is about the conversion
        // and not about the descriptor being bad in some other way.
        if( !mv.WellFormedQ(pd,why) )
        {
            std::printf("  tags %-22s not well-formed: %s\n", name, why.c_str());
            ok = false;
            return;
        }

        typename PS_T::Pass_T pass;
        typename PS_T::Path_T path( pd.MaxArcCount() + Int(4) );

        const bool gotQ = mv.ToPassAndPath(pd, pass, path, why);

        std::printf("  tags %-22s %s\n", name,
            (gotQ == expect_okQ)
                ? (gotQ ? "converted (as expected)" : "REFUSED (as expected)")
                : "*** WRONG ANSWER ***");

        if( gotQ != expect_okQ ) { ok = false; }
        else if( !gotQ ) { std::printf("       reason: %s\n", why.c_str()); }
    };

    // Uniform tags: converts, and must keep converting.
    attempt("strand=1,3,5 depart=0 cross=12:o,17:o land=4", true,  true,
            "middlepass, uniform");
    attempt("strand=1,3,5 depart=0 cross=12:o,17:o land=4", false, true,
            "pass, uniform");

    // Mixed tags. Well-formed as a middlepass (check 5 is dropped), so the
    // refusal has to come from the conversion.
    attempt("strand=1,3,5 depart=0 cross=12:o,17:u land=4", true,  false,
            "middlepass, mixed");
}

// The swept disk: the region the strand covers as it slides over to the
// corridor. W and the corridor share their two endpoints (the dots), so
// between them they close into a loop, and the disk is what that loop bounds.
//
// What is asserted here is the DEFINING property -- that the region really is
// "bounded by arcs in p and arcs in w": every cell next to a disk cell is
// either in the disk or on the loop, so nothing else can be part of its
// boundary. Plus that it touches BOTH W and the corridor, which is what makes
// it the region BETWEEN them rather than some unrelated pocket of the drawing.
static void RunDiskTests( const char * name, const PD_T & pd,
                          const char * spec, Int xg, Int yg )
{
    Deco_T::PassMove_T mv;
    std::string err;
    if( !Deco_T::PassMove_T::Parse(spec, mv, err) ) { ok = false; return; }

    OrthoDraw_T H(pd, Int(-1), GridSettings(xg,yg));
    Deco_T deco(H, margin);

    auto pr = deco.RoutePassMove(pd, mv);
    if( !pr.validQ ) { ok = false; return; }

    const Int n_x = H.Width()  * xg + Int(2) + Int(2) * margin;
    const Int n_y = H.Height() * yg + Int(1) + Int(2) * margin;

    auto disk = KnoodlePassView::PassDiskCells<PD_T>(
        H, mv, pr, margin, n_x, n_y);

    auto idx = [n_x,n_y]( Int x, Int y ) -> std::size_t
    { return static_cast<std::size_t>(x + n_x * (n_y - Int(1) - y)); };

    // Split the loop into its two arcs: the corridor, and W between the dots.
    std::set<std::array<Int,2>> p_cells;
    for( const auto & c : pr.route.path ) { p_cells.insert({c[0],c[1]}); }

    std::set<std::array<Int,2>> loop, w_cells;
    for( const auto & c : KnoodlePassView::PassLoopCells<PD_T>(H,mv,pr,margin) )
    {
        loop.insert({c[0],c[1]});
        if( !p_cells.count({c[0],c[1]}) ) { w_cells.insert({c[0],c[1]}); }
    }

    if( disk.empty() )
    {
        std::printf("  disk %-18s %lldx%-2lld  encloses nothing\n",
            name, (long long)xg, (long long)yg);
        return;
    }

    std::size_t cells = 0;
    bool borderQ = false, on_loopQ = false, leakQ = false;
    bool touch_wQ = false, touch_pQ = false;

    static const Int dx4[] = {1,0,-1,0};
    static const Int dy4[] = {0,1,0,-1};

    for( Int y = 0; y < n_y; ++y )
    for( Int x = 0; x < n_x - Int(1); ++x )
    {
        const auto i = idx(x,y);
        if( i >= disk.size() || !disk[i] ) { continue; }
        ++cells;

        if( loop.count({x,y}) ) { on_loopQ = true; }
        if( (x == 0) || (x == n_x - Int(2)) || (y == 0) || (y == n_y - Int(1)) )
        {
            borderQ = true;
        }

        for( int d = 0; d < 4; ++d )
        {
            const Int qx = x + dx4[d], qy = y + dy4[d];
            if( (qx < 0) || (qx >= n_x - Int(1)) || (qy < 0) || (qy >= n_y) )
            {
                leakQ = true; continue;
            }
            const std::array<Int,2> q{qx,qy};
            if( loop.count(q) )
            {
                if( w_cells.count(q) ) { touch_wQ = true; }
                if( p_cells.count(q) ) { touch_pQ = true; }
                continue;
            }
            const auto j = idx(qx,qy);
            if( (j >= disk.size()) || !disk[j] ) { leakQ = true; }
        }
    }

    const bool goodQ = !borderQ && !on_loopQ && !leakQ && touch_wQ && touch_pQ;

    std::printf("  disk %-18s %lldx%-2lld  %zu cells%s\n",
        name, (long long)xg, (long long)yg, cells,
        goodQ ? ", bounded by w and p"
              : (borderQ  ? "  *** reaches the canvas border ***"
              : (on_loopQ ? "  *** includes loop cells ***"
              : (leakQ    ? "  *** has a boundary cell that is neither disk nor loop ***"
              : (!touch_wQ ? "  *** does not touch the strand ***"
                           : "  *** does not touch the corridor ***")))));

    if( !goodQ ) { ok = false; }
}

// The other way to name a pass move: give the strand's first and last arc and
// let Knoodle's own search supply the corridor (knoodledraw --find-pass=A,B,
// oracle_vs_reroute --find A B). Whatever it returns is inside Reroute's
// contract by construction, so it is the move class that actually matters --
// and its drawings must satisfy the two deletions like any other.
//
// Finding nothing is a legitimate answer, not a failure: FindShortestRerouting
// caps the corridor at L-2, so a strand with no strictly shorter route through
// the faces simply has none. Only what it DOES return is checked here.
static void RunFoundPassTests( const char * name, const PD_T & pd,
                               Int first_arc, Int last_arc )
{
    std::string why;
    Deco_T::PassMove_T mv;

    if( !KnoodleFindPass::FindPassDescriptor<PD_T,PDC_T,PS_T,
            Deco_T::PassMove_T>(pd, first_arc, last_arc, mv, why) )
    {
        std::printf("  found %-16s arcs %lld..%lld: none (%s)\n",
            name, (long long)first_arc, (long long)last_arc, why.c_str());
        return;
    }

    // The corridor came from the shortest-path search, so it must shorten:
    // FindShortestRerouting caps at L-2, i.e. k <= L-2.
    const Int L = static_cast<Int>(mv.strand.size());
    const Int k = static_cast<Int>(mv.cross.size());
    if( k > L - Int(2) )
    {
        std::printf("  found %-16s k=%lld exceeds the L-2 = %lld cap\n",
            name, (long long)k, (long long)(L - Int(2)));
        ok = false;
        return;
    }

    int drawn = 0;
    for( Int xg : {4, 6, 8} )
    {
        for( Int yg : {2, 3, 4} )
        {
            OrthoDraw_T H(pd, Int(-1), GridSettings(xg,yg));
            Deco_T deco(H, margin);

            auto pr = deco.RoutePassMove(pd, mv);
            if( !pr.validQ )
            {
                std::printf("  found %-16s %lldx%lld: routing failed (%s)\n",
                    name, (long long)xg, (long long)yg, pr.why.c_str());
                ok = false;
                continue;
            }

            PD_T after = deco.AfterDiagram(pd, mv, why);
            if( !why.empty() )
            {
                std::printf("  found %-16s AfterDiagram: %s\n", name, why.c_str());
                ok = false;
                continue;
            }

            if( !KnoodlePassView::CheckBothDeletions<PD_T>(
                    H, deco, pd, mv, pr, after, margin, why) )
            {
                std::printf("  found %-16s %lldx%lld: %s\n",
                    name, (long long)xg, (long long)yg, why.c_str());
                ok = false;
                continue;
            }
            ++drawn;
        }
    }

    std::printf("  found %-16s arcs %lld..%lld -> k=%lld, %lld->%lld crossings,"
        " %d grids OK\n", name, (long long)first_arc, (long long)last_arc,
        (long long)k, (long long)pd.CrossingCount(),
        (long long)(pd.CrossingCount() - (L - Int(1)) + k), drawn);
}

// `knoodledraw --trace --verify` leans on DiagramsIsomorphicQ for the one
// comparison that has no shared labelling to appeal to -- a record's snapshot
// against what the previous record's move produced, across a PD code that
// renumbered everything. It replaced the MacLeod code, which does not exist
// for links, so it had better be right about both what it accepts and what it
// refuses.
static void RunIsomorphismTests()
{
    auto build = []( std::vector<Int> code ) -> PD_T
    {
        return PD_T::FromSignedPDCode(
            code.data(), static_cast<Int>(code.size() / 5));
    };

    auto expect = [&]( const PD_T & a, const PD_T & b, bool wantQ,
                       const char * name )
    {
        std::string why;
        const bool got = DiagramsIsomorphicQ(a,b,why);
        std::printf("  iso %-28s %s\n", name,
            (got == wantQ)
                ? (got ? "isomorphic (as expected)" : "refused (as expected)")
                : "*** WRONG ANSWER ***");
        if( got != wantQ ) { ok = false; }
        else if( !got ) { std::printf("      reason: %s\n", why.c_str()); }
    };

    PD_T trefoil = build({ 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 });

    // Same diagram with its rows permuted: a relabelling, nothing more. This
    // is the case the trace check actually meets, since a PD code renumbers.
    PD_T trefoil_perm = build({ 4,2,5,1,1,  0,4,1,3,1,  2,0,3,5,1 });

    // The mirror: identical crossing and arc counts, so the answer has to come
    // from the propagation rather than from a count.
    PD_T trefoil_mirror = build({ 4,1,3,0,-1,  0,3,5,2,-1,  2,5,1,4,-1 });

    PD_T fig8 = build({ 3,1,4,0,1,  7,5,0,4,1,  5,2,6,3,-1,  1,6,2,7,-1 });

    // Links: the whole reason MacLeod had to go.
    PD_T hopf  = build({ 1,2,0,3,-1,  3,0,2,1,-1 });
    PD_T hopf2 = build({ 3,0,2,1,-1,  1,2,0,3,-1 });          // rows swapped
    PD_T hopf_mirror = build({ 2,0,3,1,1,  0,2,1,3,1 });

    expect(trefoil, trefoil,        true,  "trefoil vs itself");
    expect(trefoil, trefoil_perm,   true,  "trefoil vs relabelled");
    expect(trefoil, trefoil_mirror, false, "trefoil vs mirror");
    expect(trefoil, fig8,           false, "trefoil vs figure-eight");
    expect(hopf,    hopf,           true,  "hopf vs itself");
    expect(hopf,    hopf2,          true,  "hopf vs relabelled");
    expect(hopf,    hopf_mirror,    false, "hopf vs mirror");
    expect(hopf,    trefoil,        false, "hopf vs trefoil");
}

int main()
{
    std::vector<Int> trefoil_code = {
        0, 4, 1, 3, 1,
        2, 0, 3, 5, 1,
        4, 2, 5, 1, 1,
    };
    PD_T trefoil = PD_T::FromSignedPDCode(trefoil_code.data(), Int(3));

    PD_T big = PD_T::FromSignedPDCode(
        zf061098_underpass.data(),
        Int(zf061098_underpass.size() / 5), false, false );

    auto link = []( const std::vector<Int> & code ) -> PD_T
    {
        return PD_T::FromSignedPDCode(
            code.data(), Int(code.size() / 5), false, false );
    };

    PD_T l4a1  = link(L4a1_0);
    PD_T l6a1  = link(L6a1_0);
    PD_T l6n1  = link(L6n1_0_0);
    PD_T l7n1  = link(L7n1_0);
    PD_T l10a  = link(L10n104_0_0_0);
    PD_T l10b  = link(L10n104_1_0_0);

    // ---- the parser's own round trip, before any move is involved --------
    std::printf("=== plain drawings read back ===\n");
    {
        // Mirroring a PD code is not a sign flip: slot 0 must stay the
        // incoming UNDER-strand, and mirroring makes the old over-strand the
        // under one, so the row rotates X[a,b,c,d] -> X[b,c,d,a] too.
        std::vector<Int> trefoil_L = {
            4, 1, 3, 0, -1,
            0, 3, 5, 2, -1,
            2, 5, 1, 4, -1,
        };
        std::vector<Int> fig8 = {
            3, 1, 4, 0,  1,
            7, 5, 0, 4,  1,
            5, 2, 6, 3, -1,
            1, 6, 2, 7, -1,
        };
        std::vector<Int> hopf = {           // L2a1_0: two components
            1, 2, 0, 3, -1,
            3, 0, 2, 1, -1,
        };
        std::vector<Int> l6a1 = {           // L6a1_0: mixed signs, two components
            3, 8, 0, 9, -1,
            5, 0, 6, 1, -1,
            1, 4, 2, 5, -1,
            9, 2,10, 3, -1,
           11, 7, 4, 6,  1,
            7,11, 8,10,  1,
        };
        // L6a4_0_0, the Borromean rings: THREE components, so the parser's
        // per-component orientation propagation and colouring have to keep
        // three independent cycles apart rather than two.
        std::vector<Int> borromean = {
            3,11, 0,10,  1,
            5, 0, 6, 1, -1,
            1, 8, 2, 9, -1,
            7, 3, 4, 2,  1,
            9, 4,10, 5, -1,
           11, 7, 8, 6,  1,
        };

        int done = 0;
        for( Int xg : {4, 6, 8, 20} )
        {
            for( Int yg : {2, 3, 4, 20} )
            {
                RunPlainRoundTrip("trefoil (right)", trefoil_code, 3, xg, yg);
                RunPlainRoundTrip("trefoil (left)",  trefoil_L,    3, xg, yg);
                RunPlainRoundTrip("figure-eight",    fig8,         4, xg, yg);
                RunPlainRoundTrip("hopf link",       hopf,         2, xg, yg);
                RunPlainRoundTrip("L6a1_0 link",     l6a1,         6, xg, yg);
                RunPlainRoundTrip("borromean (3cpt)", borromean,   6, xg, yg);
                done += 6;
            }
        }
        std::printf("  %d drawings read back and matched port-by-port\n", done);
    }

    std::printf("=== two deletions, parsed back out of the drawing ===\n");

    const std::vector<Case_T> cases = {
        { "trefoil doc",   &trefoil, "strand=1,3 depart=1 cross=6:u land=3" },
        { "trefoil 2-arc", &trefoil, "strand=11,1 depart=11 cross=7:o land=1" },
        { "big k=2",       &big,     "strand=1,3,5 depart=0 cross=51:o,122:o land=4" },
        { "big k=3",       &big,     "strand=1,3,5,7 depart=0 cross=51:o,122:o,239:o land=6" },
        { "big k=8",       &big,     "strand=81,83,85,87,89,91,93,95,97 depart=80"
                                     " cross=7:u,42:u,8:u,230:u,131:u,281:u,260:u,290:u land=96" },

        // LINKS. In every one of these the corridor belongs to one component
        // and crosses arcs of ANOTHER -- the case no knot can exercise, and
        // the reason the checking had to stop relying on a knot invariant.
        { "L4a1 2cpt k=1",  &l4a1, "strand=1,3 depart=0 cross=12:o land=2" },
        { "L6a1 2cpt k=1u", &l6a1, "strand=11,13 depart=10 cross=1:u land=12" },
        { "L6n1 3cpt k=2",  &l6n1, "strand=1,3,5 depart=0 cross=12:o,17:o land=4" },
        { "L7n1 2cpt k=2",  &l7n1, "strand=1,3,5 depart=0 cross=12:o,22:o land=4" },
        // The corridor crosses one arc of each of the other THREE components.
        { "L10n104 4cpt k=3",  &l10a,
          "strand=15,17,19,9 depart=14 cross=33:o,0:o,22:o land=8" },
        { "L10n104 4cpt k=3u", &l10b,
          "strand=13,15,17,19 depart=12 cross=31:u,2:u,24:u land=18" },
    };

    for( const auto & kase : cases )
    {
        for( Int xg : {4, 6, 8} )
        {
            for( Int yg : {2, 3, 4} )
            {
                RunCase(kase, xg, yg);
            }
        }
    }

    std::printf("=== per-crossing tags have nowhere to go in Pass_T ===\n");
    RunMixedTagTests(l6n1);

    std::printf("=== the swept disk, bounded by w and p ===\n");
    for( Int xg : {4, 8} )
    {
        for( Int yg : {2, 4} )
        {
            RunDiskTests("trefoil doc", trefoil,
                "strand=1,3 depart=1 cross=6:u land=3", xg, yg);
            RunDiskTests("big k=3", big,
                "strand=1,3,5,7 depart=0 cross=51:o,122:o,239:o land=6", xg, yg);
            RunDiskTests("L6n1 3cpt", l6n1,
                "strand=1,3,5 depart=0 cross=12:o,17:o land=4", xg, yg);
            RunDiskTests("L10n104 4cpt", l10a,
                "strand=15,17,19,9 depart=14 cross=33:o,0:o,22:o land=8", xg, yg);
        }
    }

    std::printf("=== moves found by Knoodle's own search (in contract) ===\n");
    RunFoundPassTests("big",     big,   Int(40), Int(48));
    RunFoundPassTests("trefoil", trefoil, Int(0), Int(1));
    RunFoundPassTests("L6n1",    l6n1,  Int(0),  Int(2));
    RunFoundPassTests("L10n104", l10a,  Int(7),  Int(4));

    std::printf("=== unrooted isomorphism (what --verify's trace check uses) ===\n");
    RunIsomorphismTests();

    std::printf("=== the oracle must be able to fail ===\n");
    RunNegativeTests(trefoil, "strand=1,3 depart=1 cross=6:u land=3");

    std::printf(ok ? "PASS VIEW CHECK OK\n" : "PASS VIEW CHECK FAILED\n");
    return ok ? 0 : 1;
}
