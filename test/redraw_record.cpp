// redraw_record -- write a redraw record for one diagram of a trace, with a
// Reapr-style lattice witness (docs/animation-redraw.md, R1).
//
// Nobody emits redraw records yet (not Knoodle, not middlestrands), and the
// animation needs realistic ones to be built and tested against. This takes
// record K of a trace -- its snapshot, and its threaded exterior if it has one
// -- lays the diagram out with OrthoDraw on that exterior, and lifts the layout
// to an INTEGER lattice curve the way Reapr lifts it (src/Reapr/Embedding.hpp,
// Embedding_VertexCoordinates):
//
//   * x and y are the layout's grid coordinates, doubled, so that the middle
//     of an arc with an even number of vertices is still a lattice point;
//   * z is zscale times the arc's level (Reapr's TV energy, solved by MCF: the
//     levels are whole numbers);
//   * each arc runs at its own level up to its middle, jumps there, and runs
//     at the next arc's level from there on. Unlike Reapr, the jump is always
//     in the middle of a straight segment, never at a corner (see below).
//
// It then applies the rotation, projects both curves with the one projector
// knoodleprove trusts (KnoodleTraceVerify::ProjectLattice), checks that
// project(E) is the snapshot (colour-kept isomorphism), and writes a
// two-record v1 trace to stdout:
//
//   #step n=0   #move kind=redraw rot=..., #embedding, #state of the input
//   #step n=1   #state of project(R*E)
//
// `knoodleprove` must VERIFY the result; that is the generator's own check.
//
// Usage:
//   redraw_record --trace=FILE [--record=K] [--rot=1|2] [--zscale=reapr|N]
//                 [--exterior=view|max] [--result-state=FILE]
//
//   --record=K        which record's snapshot to lift (default 0)
//   --rot=1           x->y->z->x (+2pi/3 about (1,1,1)); --rot=2 the inverse
//   --zscale=reapr    Reapr's own normalisation (z-extent about the smaller of
//                     the layout's width and height); --zscale=N multiplies
//                     the levels by N (default: reapr)
//   --exterior=view   the record's threaded `#view exterior=` face if it has
//                     one, else OrthoDraw's default (the largest face);
//                     --exterior=max always the largest face (default: view)
//   --result-state=F  also write project(R*E) as a bare `#state` block to F
//   --dump=F          write the curve as built, before any check (debugging)
//   --result-pd=F     also write project(R*E) as a signed PD code (5 columns),
//                     e.g. for middlestrands' `middlepass` to continue from

#include "../Knoodle.hpp"
#include "../src/MoveTrace.hpp"
#include "../src/PassDescriptor.hpp"
#include "../tools/exterior_thread.hpp"
#include "../tools/trace_verify.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace Knoodle;

using Int         = std::int64_t;
using PD_T        = PlanarDiagram<Int>;
using OrthoDraw_T = OrthoDraw<PD_T>;
using Trace_T     = MoveTrace<PD_T>;
using Move_T      = PassDescriptor<Int>;
using Reapr_T     = Reapr<Real64,Int>;

static int Fail( const std::string & why )
{
    std::fprintf(stderr, "redraw_record: %s\n", why.c_str());
    return 1;
}

static bool Arg( const std::string & a, const char * key, std::string & val )
{
    const std::string k = key;
    if( a.rfind(k, 0) != 0 ) { return false; }
    val = a.substr(k.size());
    return true;
}

int main( int argc, char ** argv )
{
    std::string trace_path, zscale_arg = "reapr", exterior_arg = "view", result_path, dump_path, pd_path;
    Int record = 0;
    int rot = 1;

    for( int i = 1; i < argc; ++i )
    {
        const std::string a = argv[i];
        std::string v;
        if(      Arg(a, "--trace=",        v) ) { trace_path   = v; }
        else if( Arg(a, "--record=",       v) ) { record       = std::stoll(v); }
        else if( Arg(a, "--rot=",          v) ) { rot          = std::stoi(v); }
        else if( Arg(a, "--zscale=",       v) ) { zscale_arg   = v; }
        else if( Arg(a, "--exterior=",     v) ) { exterior_arg = v; }
        else if( Arg(a, "--result-state=", v) ) { result_path  = v; }
        else if( Arg(a, "--dump=",         v) ) { dump_path    = v; }
        else if( Arg(a, "--result-pd=",    v) ) { pd_path      = v; }
        else { return Fail("unknown argument " + a); }
    }
    if( trace_path.empty() )                          { return Fail("--trace=FILE is required"); }
    if( rot != 1 && rot != 2 )                        { return Fail("--rot must be 1 or 2"); }
    if( exterior_arg != "view" && exterior_arg != "max" ) { return Fail("--exterior must be view or max"); }

    // ---- the record ---------------------------------------------------------
    std::vector<Trace_T::Record> recs;
    {
        std::ifstream in(trace_path);
        if( !in ) { return Fail("cannot read " + trace_path); }
        Trace_T::Reader reader (in);
        Trace_T::Record rec;
        std::string why;
        for(;;)
        {
            const auto st = reader.Next(rec, why);
            if( st == Trace_T::Status::Eof )   { break; }
            if( st == Trace_T::Status::Error ) { return Fail(trace_path + ": " + why); }
            if( !rec.candidateQ ) { recs.push_back(rec); }
        }
    }
    if( record < 0 || record >= Int(recs.size()) ) { return Fail("no record " + std::to_string(record)); }
    if( !recs[record].state )                      { return Fail("record " + std::to_string(record) + " has no snapshot"); }

    PD_T pd = *recs[record].state;
    if( pd.CrossingCount() <= 0 )             { return Fail("the snapshot has no crossings"); }
    if( pd.DiagramComponentCount() != 1 )     { return Fail("the snapshot is a split diagram"); }

    Int face = -1;
    if( exterior_arg == "view" )
    {
        if( auto line = KnoodleExterior::ViewLine(recs[record].headers) )
        {
            KnoodleExterior::View v;
            std::string why;
            if( !KnoodleExterior::ParseView(*line, v, why) ) { return Fail("#view: " + why); }
            face = Move_T::LeftFace(pd, v.exterior);
            if( face < 0 ) { return Fail("#view exterior= names no face"); }
        }
    }

    // ---- the layout and the levels --------------------------------------------
    OrthoDraw_T H (pd, face);
    const Int gx = H.HorizontalGridSize();
    const Int gy = H.VerticalGridSize();

    Reapr_T::Settings_T rs;
    rs.permute_randomQ = false;
    Reapr_T reapr (rs);
    const Tensor1<Real64,Int> L = reapr.Levels(pd);

    const Int m = pd.MaxArcCount();
    Int L_min = 0, L_max = 0;
    bool firstQ = true;
    std::vector<Int> level (static_cast<std::size_t>(m), 0);
    for( Int a = 0; a < m; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; }
        const Real64 r = std::round(L[a]);
        if( std::abs(L[a] - r) > 1e-9 ) { return Fail("level of arc " + std::to_string(a) + " is not a whole number"); }
        level[a] = static_cast<Int>(r);
        if( firstQ ) { L_min = L_max = level[a]; firstQ = false; }
        L_min = std::min(L_min, level[a]);
        L_max = std::max(L_max, level[a]);
    }

    // Lattice units: one grid step is 2.
    Int zscale = 1;
    if( zscale_arg == "reapr" )
    {
        // Reapr: scale = Min(w,h) / (L_max - L_min), w and h the layout's size.
        const Int w = 2 * H.Width(), h = 2 * H.Height();
        zscale = (L_max > L_min) ? std::max<Int>(1, std::llround(Real64(std::min(w,h)) / Real64(L_max - L_min))) : 1;
    }
    else
    {
        zscale = std::stoll(zscale_arg);
        if( zscale < 1 ) { return Fail("--zscale must be positive"); }
    }

    // ---- the lattice curve (Reapr's Embedding_VertexCoordinates, in integers) --
    const auto & lc_arcs  = pd.LinkComponentArcs();
    const Int    lc_count = lc_arcs.SublistCount();
    const auto & A_V      = H.ArcVertices();
    const auto & V_coords = H.VertexCoordinates();
    const auto & A_next   = H.ArcNextArc();
    const auto & A_color  = pd.ArcColors();

    std::vector<std::int64_t> xyz;
    std::vector<Int> ptr {0}, colors;

    auto X = [&]( Int v ) -> std::int64_t
    {
        const Int x = V_coords(v,0);
        return (x % gx == 0) ? 2 * (x / gx) : std::numeric_limits<std::int64_t>::min();
    };
    auto Y = [&]( Int v ) -> std::int64_t
    {
        const Int y = V_coords(v,1);
        return (y % gy == 0) ? 2 * (y / gy) : std::numeric_limits<std::int64_t>::min();
    };
    auto push = [&]( std::int64_t x, std::int64_t y, std::int64_t z )
    {
        xyz.push_back(x); xyz.push_back(y); xyz.push_back(z);
    };

    for( Int lc = 0; lc < lc_count; ++lc )
    {
        colors.push_back(A_color[*(lc_arcs.Sublist(lc).begin())]);

        for( Int a : lc_arcs.Sublist(lc) )
        {
            const Int b = A_next[a];
            const std::int64_t La = zscale * level[a];
            const std::int64_t Lb = zscale * level[b];

            const Int k_begin = A_V.Pointers()[a];
            const Int k_end   = A_V.Pointers()[a + 1];
            const Int n       = k_end - k_begin;
            const Int k_half  = k_begin + n / 2;
            const auto & V    = A_V.Elements();

            // The jump is ALWAYS in the middle of the segment V[k_half-1] V[k_half].
            // Reapr puts it there only when n is even; when n is odd it jumps AT
            // V[k_half], which is usually a corner. Seen down (eps, eps^3, 1), a
            // jump at a corner can fold back across the strand arriving there
            // and add a curl (an R1 kink): measured, project(E) had 13 crossings
            // for Goeritz's 11. Reapr never sees it, because it rotates at random
            // before it projects. In the middle of a straight segment the jump's
            // shadow runs along the strand or beside it, and cannot cross it.
            for( Int k = k_begin; k < k_half; ++k ) { push(X(V[k]), Y(V[k]), La); }

            {
                // Two grid points' midpoint: (2x0/gx + 2x1/gx)/2, a whole number.
                const std::int64_t mx = (X(V[k_half-1]) + X(V[k_half])) / 2;
                const std::int64_t my = (Y(V[k_half-1]) + Y(V[k_half])) / 2;
                push(mx, my, La);
                if( Lb != La ) { push(mx, my, Lb); }
            }

            for( Int k = k_half; k < k_end - 1; ++k ) { push(X(V[k]), Y(V[k]), Lb); }
        }
        ptr.push_back(static_cast<Int>(xyz.size() / 3));
    }

    for( std::size_t i = 0; i < xyz.size(); ++i )
    {
        if( xyz[i] == std::numeric_limits<std::int64_t>::min() ) { return Fail("a layout vertex is off the grid"); }
        if( std::abs(xyz[i]) >= (std::int64_t(1) << 31) )        { return Fail("a coordinate does not fit an int32"); }
    }

    if( !dump_path.empty() )
    {
        // Debugging aid: the curve as built, one "component vertex x y z" row each.
        std::ofstream d(dump_path);
        for( std::size_t c = 0; c + 1 < ptr.size(); ++c )
        {
            for( Int v = ptr[c]; v < ptr[c+1]; ++v )
            {
                d << c << "\t" << v << "\t" << xyz[3*v] << "\t" << xyz[3*v+1] << "\t" << xyz[3*v+2] << "\n";
            }
        }
    }

    // ---- check the witness, rotate, project -------------------------------------
    const std::string desc_text = (rot == 1) ? "kind=redraw rot=0,0,1,1,0,0,0,1,0"
                                             : "kind=redraw rot=0,1,0,0,0,1,1,0,0";
    KnoodleRedraw::RedrawDescriptor<Int> desc;
    {
        std::string why;
        if( !KnoodleRedraw::RedrawDescriptor<Int>::Parse(desc_text, desc, why) ) { return Fail("rotation: " + why); }
    }

    const auto before = KnoodleTraceVerify::ProjectLattice<PD_T>(xyz, ptr, colors);
    if( !before.okQ )          { return Fail("project(E): " + before.why); }
    if( !before.anelli.empty() ) { return Fail("project(E) has a crossingless component"); }
    {
        std::string why;
        if( !DiagramsIsomorphicQ(before.pd, pd, why, nullptr, true) )
        {
            return Fail("project(E) is not the snapshot: " + why);
        }
    }

    const std::vector<std::int64_t> rxyz = desc.Apply(xyz);
    const auto after = KnoodleTraceVerify::ProjectLattice<PD_T>(rxyz, ptr, colors);
    if( !after.okQ ) { return Fail("project(R*E): " + after.why); }
    if( after.crossings > 0 && after.pd.DiagramComponentCount() != 1 )
    {
        return Fail("project(R*E) is split (a redraw refuses that; check 5)");
    }
    if( after.crossings == 0 )
    {
        return Fail("project(R*E) has no crossings; a v1 trace cannot spell the empty record");
    }

    // ---- write ------------------------------------------------------------------
    std::string state0, state1;
    if( !Trace_T::WriteStateBlock(pd, "#state", state0) )       { return Fail("cannot write the snapshot"); }
    if( !Trace_T::WriteStateBlock(after.pd, "#state", state1) ) { return Fail("cannot write project(R*E)"); }

    std::ostringstream o;
    o << "#trace v=1\n";
    o << "#step n=0 summand=0\n";
    o << "#comment redraw_record: record " << record << " of " << trace_path << "\n";
    o << "#comment OrthoDraw exterior " << ((face < 0) ? std::string("max") : std::to_string(face))
      << ", TV levels " << L_min << ".." << L_max << ", zscale " << zscale
      << "; " << pd.CrossingCount() << " -> " << after.crossings << " crossings\n";
    o << "#move " << desc_text << "\n";
    if( !after.anelli.empty() )
    {
        o << "#spinoffs colors=";
        for( std::size_t i = 0; i < after.anelli.size(); ++i ) { o << (i ? "," : "") << after.anelli[i]; }
        o << "\n";
    }
    o << "#embedding components=" << colors.size() << "\n";
    for( std::size_t c = 0; c < colors.size(); ++c )
    {
        o << "#component color=" << colors[c] << " rows=" << (ptr[c+1] - ptr[c]) << "\n";
        for( Int v = ptr[c]; v < ptr[c+1]; ++v )
        {
            o << xyz[3*v] << "\t" << xyz[3*v+1] << "\t" << xyz[3*v+2] << "\n";
        }
    }
    o << state0 << "\n";
    o << "#step n=1 summand=0\n";
    o << "#comment project(R*E), colours carried through the rotation\n";
    o << state1;

    std::cout << o.str();

    if( !pd_path.empty() )
    {
        PD_T q = after.pd;
        const auto code = q.template PDCode<Int, {.signQ = true}>();
        std::ofstream r(pd_path);
        for( Int c = 0; c < q.CrossingCount(); ++c )
        {
            r << code(c, 0);
            for( Int j = 1; j < 5; ++j ) { r << '\t' << code(c, j); }
            r << '\n';
        }
        if( !r ) { return Fail("cannot write " + pd_path); }
    }
    if( !result_path.empty() )
    {
        std::ofstream r(result_path);
        r << state1;
        if( !r ) { return Fail("cannot write " + result_path); }
    }

    std::fprintf(stderr, "redraw_record: %lld -> %lld crossings, %lld vertices, z %lld..%lld (zscale %lld)\n",
        (long long)pd.CrossingCount(), (long long)after.crossings, (long long)(xyz.size()/3),
        (long long)(zscale*L_min), (long long)(zscale*L_max), (long long)zscale);
    return 0;
}
