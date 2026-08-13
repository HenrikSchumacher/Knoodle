// oracle_vs_reroute -- two independent answers to one pass move, compared.
//
//   OrthoDecorate::AfterDiagram  built from the descriptor alone; never calls
//                                the applier
//   PassSimplifier::Reroute      the applier itself
//
// If they disagree, one of them is wrong. This is the check a pass drawing is
// supposed to carry: the picture claims "delete W and you get this diagram",
// and the applier is the ground truth for that claim.
//
// Comparison is by `DiagramsAgreeQ` (same oriented diagram up to relabelling,
// seeded at an anchor the move must not touch, so no isomorphism search) and,
// independently, by the Fox-colouring determinant. CheckAll is deliberately
// NOT the criterion: it says "legal diagram", not "the right one", which is
// exactly how a wrong splice presents.
//
// CONTRACT NOTE. Henrik states one precondition for `Reroute` (2026-08-12):
// the corridor came from `FindShortestPath` and is a SHORTEST path. A
// descriptor that violates it is outside the API, and disagreement there is
// not a bug report -- it is the boundary being probed. `--expect-shortest`
// checks the corridor against the true minimum before running, so a run can
// state which side of the contract it is on.
//
// Friend access to `LoadDiagram`/`Reroute` goes through `test/pass_oracle.hpp`
// (Henrik approved it 2026-08-12); nothing here reimplements the applier.
//
// usage: oracle_vs_reroute DIAGRAM.tsv "descriptor" [--expect-shortest]
//        oracle_vs_reroute DIAGRAM.tsv --find A B
//
// `--find A B` asks `PlanarDiagramComplex::FindShortestRerouting` for the
// shortest rerouting of the strand from arc A to arc B and compares that. It
// is the knoodleprove pipeline end to end -- Henrik's own public finder
// supplies the corridor, so the move is IN CONTRACT by construction -- and it
// is the positive control for this driver: on such a move the two sides must
// AGREE, which is what makes a disagreement elsewhere meaningful.

#include "../Knoodle.hpp"
#include "pass_oracle.hpp"
#include "../tools/diagram_agreement.hpp"
#include "knot_determinant.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using Int    = std::int64_t;
using PD_T   = Knoodle::PlanarDiagram<Int>;
using PDC_T  = Knoodle::PlanarDiagramComplex<Int>;
using PS_T   = Knoodle::PassSimplifier<Int>;
using OD_T   = Knoodle::OrthoDraw<PD_T>;
using Deco   = Knoodle::OrthoDecorate<PD_T>;
using Oracle = Knoodle::PassOracle<Int>;
using Desc_T = Knoodle::PassDescriptor<Int>;

static std::vector<Int> ReadCode( const char * f )
{
    std::vector<Int> code;
    std::ifstream in(f);
    std::string line;
    while( std::getline(in,line) )
    {
        std::istringstream ss(line);
        std::string t;
        std::vector<Int> r;
        bool okQ = true;
        while( ss >> t )
        {
            try { r.push_back(std::stoll(t)); } catch(...) { okQ = false; break; }
        }
        if( okQ && (r.size() == 5) ) { code.insert(code.end(), r.begin(), r.end()); }
    }
    return code;
}

/*!@brief Round-trip a diagram through its own PD code.
 *
 * WHY THIS EXISTS. After a `Reroute` that corrupts the diagram, reading
 * `C_arcs` directly and reading the diagram's own `PDCode()` output give
 * DIFFERENT KNOTS -- measured on the PR #30 reproducer: 115 from the live
 * slots, 247 from the traversal. `CheckAll` passes on both. So the corrupted
 * object is not merely the wrong knot, it is not self-consistent between the
 * two ways of reading it.
 *
 * The determinant is therefore measured on the traversal view, which is what
 * `PDCode` emits, what every downstream consumer sees, and what the handoff's
 * independent `det.py` was run on. With this normalisation our determinant
 * agrees with `det.py` exactly (247 and 5, up to sign).
 *
 * `ClearCache()` before `PDCode()` is REQUIRED -- without it the emitted code
 * is stale traversal garbage.
 *
 * NOTE: `DiagramsAgreeQ` is deliberately NOT given normalised diagrams. It
 * pins crossings by index and seeds at an untouched anchor, which only works
 * on the live labels; a round trip renumbers and destroys that.
 */
static PD_T ThroughPDCode( Knoodle::mref<PD_T> pd )
{
    pd.ClearCache();
    auto t = pd.template PDCode<Int,{.signQ=true,.colorQ=false}>();
    return PD_T::FromSignedPDCode(t.data(), pd.CrossingCount(), false, false);
}

/*!@brief Shortest corridor length between the merged endpoint faces -- the
 * graph `FindShortestPath` actually searches. Strand arcs are hidden, so the
 * faces along them are one node and crossing one is free; every other arc
 * costs one. Cross-checked against `FindShortestRerouting`, which returns a
 * path of exactly this many crossings.
 */
static Int MergedDualDistance(
    Knoodle::cref<PD_T> pd, Knoodle::cref<std::set<Int>> w_arcs,
    const Int src, const Int dst )
{
    const Int nf = pd.FaceCount();
    if( (src < Int(0)) || (dst < Int(0)) || (src >= nf) || (dst >= nf) )
    {
        return Int(-1);
    }

    std::vector<Int> dist( static_cast<std::size_t>(nf), Int(-1) );
    std::deque<Int>  dq;
    dist[static_cast<std::size_t>(src)] = Int(0);
    dq.push_back(src);

    while( !dq.empty() )
    {
        const Int f  = dq.front(); dq.pop_front();
        const Int df = dist[static_cast<std::size_t>(f)];

        for( Int da : pd.FaceDarcs()[f] )
        {
            const Int g = Desc_T::LeftFace(pd, da ^ Int(1));
            if( g < Int(0) ) { continue; }

            const Int w  = (w_arcs.count(Desc_T::ArcOf(da)) > 0) ? Int(0) : Int(1);
            const Int dg = dist[static_cast<std::size_t>(g)];

            if( (dg < Int(0)) || (df + w < dg) )
            {
                dist[static_cast<std::size_t>(g)] = df + w;
                if( w == Int(0) ) { dq.push_front(g); } else { dq.push_back(g); }
            }
        }
    }
    return dist[static_cast<std::size_t>(dst)];
}

int main( int argc, char ** argv )
{
    if( argc < 3 )
    {
        std::fprintf(stderr,
            "usage: oracle_vs_reroute DIAGRAM.tsv \"descriptor\" "
            "[--expect-shortest]\n");
        return 2;
    }

    bool expect_shortestQ = false;
    for( int i = 3; i < argc; ++i )
    {
        if( std::strcmp(argv[i],"--expect-shortest") == 0 )
        {
            expect_shortestQ = true;
        }
    }

    const std::vector<Int> code = ReadCode(argv[1]);
    const Int n = static_cast<Int>(code.size()/5);
    if( n <= Int(0) ) { std::fprintf(stderr,"no PD rows in %s\n",argv[1]); return 2; }

    Deco::PassMove_T mv;
    std::string err, why;

    PD_T pd_a = PD_T::FromSignedPDCode(&code[0], n, false, false);

    const bool findQ = (std::strcmp(argv[2],"--find") == 0);

    if( findQ )
    {
        if( argc < 5 )
        {
            std::fprintf(stderr,"--find needs two arc indices\n");
            return 2;
        }
        const Int a = static_cast<Int>(std::atoll(argv[3]));
        const Int b = static_cast<Int>(std::atoll(argv[4]));

        PDC_T pdc_f { PD_T(pd_a) };
        pdc_f.Unlock();
        auto found = pdc_f.FindShortestRerouting(
            Int(0), a, b, pd_a.MaxArcCount(),
            PDC_T::Dijkstra_T::Bidirectional );

        if( found.Size() < Int(2) )
        {
            std::fprintf(stderr,
                "FindShortestRerouting found no rerouting for arcs %lld..%lld\n",
                (long long)a, (long long)b);
            return 1;
        }

        // Rebuild the pass the finder was asked about. `overQ` is W's own
        // uniform role; `WellFormedQ` (check 5) is what decides it, so try
        // both rather than re-derive the convention here.
        bool builtQ = false;
        for( bool overval : { false, true } )
        {
            typename PS_T::Pass_T pass;
            pass.first     = a;
            pass.last      = b;
            pass.overQ     = overval;
            pass.activeQ   = true;
            pass.arc_count = Int(0);
            {   // count the strand's arcs along the orientation
                Int x = a, guard = 0;
                const Int lim = Int(2) * pd_a.MaxArcCount() + Int(2);
                ++pass.arc_count;
                while( (x != b) && (guard++ < lim) )
                {
                    x = pd_a.NextArc(x, PD_T::Head);
                    ++pass.arc_count;
                }
            }

            Deco::PassMove_T cand;
            if( Deco::PassMove_T::FromPassAndPath(pd_a, pass, found, cand, why)
                && cand.WellFormedQ(pd_a, why) )
            {
                mv = cand; builtQ = true; break;
            }
        }
        if( !builtQ )
        {
            std::fprintf(stderr,
                "could not build a well-formed descriptor from the found "
                "path: %s\n", why.c_str());
            return 1;
        }
        std::printf("--find: %s\n", mv.ToString().c_str());
    }
    else if( !Deco::PassMove_T::Parse(argv[2], mv, err) )
    {
        std::fprintf(stderr,"bad descriptor: %s\n", err.c_str());
        return 2;
    }

    // ---- where this move sits relative to Reroute's contract ------------

    std::set<Int> w_arcs;
    for( Int da : mv.strand ) { w_arcs.insert(Desc_T::ArcOf(da)); }

    const Int kmin = MergedDualDistance(
        pd_a, w_arcs,
        Desc_T::LeftFace(pd_a, mv.depart), Desc_T::LeftFace(pd_a, mv.land) );
    const Int k = static_cast<Int>(mv.cross.size());

    const bool shortestQ = (kmin >= Int(0)) && (k == kmin);
    std::printf("corridor: k = %lld, shortest possible = %lld  -> %s\n",
        (long long)k, (long long)kmin,
        shortestQ ? "IN CONTRACT (shortest)"
                  : "OUT OF CONTRACT (not a shortest path)");

    if( expect_shortestQ && !shortestQ )
    {
        std::fprintf(stderr,
            "--expect-shortest: refusing to run, the corridor is %lld over "
            "the minimum\n", (long long)(k - kmin));
        return 2;
    }

    // ---- oracle: AfterDiagram, from the descriptor alone ----------------
    OD_T::Settings_T settings {};
    OD_T H(pd_a, Int(-1), settings);
    Deco deco(H, Int(2));

    PD_T oracle = deco.AfterDiagram(pd_a, mv, why);
    if( !why.empty() )
    {
        std::fprintf(stderr,"AfterDiagram: %s\n", why.c_str());
        return 1;
    }

    // ---- applier: Reroute, through the friend harness -------------------
    PD_T  pd_b = PD_T::FromSignedPDCode(&code[0], n, false, false);
    PDC_T pdc { PD_T(pd_b) };
    PS_T  ps(pdc, Knoodle::DijkstraStrategy_T::Bidirectional);

    Oracle::LoadDiagram(ps, pd_b);

    typename PS_T::Pass_T pass;
    typename PS_T::Path_T path( pd_b.MaxArcCount() + Int(4) );
    if( !mv.ToPassAndPath(pd_b, pass, path, why) )
    {
        std::fprintf(stderr,"ToPassAndPath: %s\n", why.c_str());
        return 1;
    }

    if( !Oracle::Reroute(ps, pass, path) )
    {
        std::fprintf(stderr,"Reroute refused the move\n");
        return 1;
    }

    std::printf("oracle : %lld crossings, CheckAll %s\n",
        (long long)oracle.CrossingCount(), oracle.CheckAll() ? "PASS" : "FAIL");
    std::printf("Reroute: %lld crossings, CheckAll %s\n",
        (long long)pd_b.CrossingCount(), pd_b.CheckAll() ? "PASS" : "FAIL");

    // ---- determinant: independent of the structural comparison ----------
    //
    // The knot determinant is only defined UP TO SIGN -- which sign the minor
    // yields depends on which row and column were dropped -- so the two runs
    // may legitimately differ by a factor of -1. These are residues mod p, so
    // that comparison has to be made in the ring: -x is p-x, NOT the negative
    // integer. Testing `det_o == -det_r` on residues can never fire and would
    // report a spurious DIFFERENT KNOT on any sign-flipped pair.
    constexpr Int det_p = Int(1000003);
    PD_T oracle_n = ThroughPDCode(oracle);
    PD_T pd_b_n   = ThroughPDCode(pd_b);
    const Int det_o = DeterminantModP(oracle_n, det_p);
    const Int det_r = DeterminantModP(pd_b_n,   det_p);

    const bool det_agreeQ =
        (det_o == det_r) || (((det_o + det_r) % det_p) == Int(0));

    std::printf("determinant (mod %lld): oracle %lld, Reroute %lld  -> %s\n",
        (long long)det_p, (long long)det_o, (long long)det_r,
        det_agreeQ ? "agree (up to sign)" : "DIFFERENT KNOT");

    // ---- structural comparison, seeded at an untouched anchor -----------
    const Int seed = Deco::PassMove_T::DarcTailCrossing(pd_a, mv.strand.front());
    std::printf("seed (tail anchor, untouched by the move): crossing %lld\n",
        (long long)seed);

    const bool agreeQ = DiagramsAgreeQ(oracle, pd_b, seed, why);
    std::printf("\n%s\n", agreeQ
        ? "AGREE: the applier produced the diagram the drawing describes."
        : ("DISAGREE: " + why).c_str());

    return (agreeQ && det_agreeQ) ? 0 : 1;
}
