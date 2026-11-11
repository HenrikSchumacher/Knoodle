// animate_morph_check -- the contract of the animator's first three pieces
// (docs/animate-design.md): src/Animate/PlaneDrawing.hpp, Mesh.hpp, Morph.hpp.
//
// Part 1 checks the kits on small cases with known answers.
//
// Part 2 is the morph's contract. For every diagram -- each `#state` and
// `#result` of the committed traces test/*.trace, a few built in, and any PD
// code file named on the command line -- and for several exterior faces, two
// or more different OrthoDraw layouts on that exterior are made (other bend
// settings, another compaction, a quarter and a half turn; one turned 37
// degrees, sheared and made 1000 times larger; one a million times smaller)
// and morphed into one another. The contract, checked here independently of the library's own
// checks:
//
//   * every triangle of every frame has positive area;
//   * the first and the last frame are the two drawings exactly;
//   * no two strand pieces without a common end meet in any frame.
//
// There is no fallback morph, so a morph that cannot be made is a failure of
// this test. The summary line gives the failure rate; each failure is printed
// with its reason.
//
// Usage: animate_morph_check [--faces=N] [--frames=K] [--max-crossings=N] [--verbose] [--only-files] [pd files...]
//   --faces=N   exterior faces tried per diagram (default 3; 0 = all)
//   A PD file holds rows `a b c d sign` (more columns are ignored); a line that
//   is not such a row ends a diagram.
//
// Build: `make animate_morph_check` in test/ (a row in test/manifest.tsv).

#include "../Knoodle.hpp"
#include "../src/MoveTrace.hpp"
#include "../src/Animate/Morph.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using Int     = std::int64_t;
using PD_T    = Knoodle::PlanarDiagram<Int>;
using OD_T    = Knoodle::OrthoDraw<PD_T>;
using Trace_T = Knoodle::MoveTrace<PD_T>;

namespace A = Knoodle::Animate;
using A::Idx; using A::Real; using A::Tri;
using Pt = A::Point;   // not named Point: macOS headers (MacTypes.h, through Accelerate) declare a global ::Point

static int checks_passed = 0, checks_failed = 0;

static void check( bool passedQ, const std::string & what )
{
    if( passedQ ) { ++checks_passed; return; }
    ++checks_failed;
    std::printf("  FAILED: %s\n", what.c_str());
}

//==============================================================================
// Part 1: the kits.
//==============================================================================

static Real MeshArea( const std::vector<Pt> & P, const std::vector<Tri> & T )
{
    Real s = 0;
    for( const Tri & t : T ) { s += A::Cross(P[t[0]], P[t[1]], P[t[2]]); }
    return s;
}

static void TestKits()
{
    std::printf("=== polyline kit ===\n");
    const A::Polyline L { {0,0}, {4,0}, {4,3} };
    check(A::Length(L) == 7.0, "length of an L-shaped polyline");
    const auto s = A::Params(L);
    check(A::At(L, s, 2.0 / 7.0) == Pt{2,0} && A::At(L, s, 1.0) == Pt{4,3}, "the point at an arc-length parameter");
    const A::Polyline sub = A::SubCurve(L, 2.0 / 7.0, 5.5 / 7.0);
    check(sub.size() == 3 && sub[0] == Pt{2,0} && sub[1] == Pt{4,0} && std::abs(sub[2][1] - 1.5) < 1e-12, "a sub-curve keeps the corner between its ends");
    check(A::Corners({ {0,0}, {1,0}, {1,0}, {2,0}, {2,5} }) == A::Polyline({ {0,0}, {2,0}, {2,5} }), "corners drops repeated and collinear points");
    check(A::Meet({0,0}, {2,2}, {0,2}, {2,0}) && A::Meet({0,0}, {2,0}, {1,0}, {1,5}) && !A::Meet({0,0}, {2,0}, {0,1}, {2,1}) && !A::Meet({0,0}, {1,0}, {2,0}, {3,0}),
          "segments that cross, touch, run parallel, lie apart on one line");
    Pt x;
    check(A::Intersection({0,0}, {2,2}, {0,2}, {2,0}, x) && x == Pt{1,1} && !A::Intersection({0,0}, {1,0}, {0,1}, {1,1}, x), "where two lines meet");
    const A::Polyline sq { {0,0}, {4,0}, {4,4}, {0,4} };
    check(A::Inside(sq, {1,1}) && !A::Inside(sq, {5,1}) && !A::Inside(sq, {4,2}) && A::Area2(sq) == 32.0, "inside a square, outside it, on it; its area");
    A::Box box;
    for( const Pt & p : L ) { box.Add(p); }
    check(box.Centre() == Pt{2,1.5} && box.Size() == 5.0, "bounding box");
    {
        std::string why;
        const std::vector<Pt> P { {0,0}, {2,0}, {2,2}, {0,2}, {1,-1}, {1,1} };
        check(A::EmbeddedQ(P, { {0,1,2,3} }, why) && !A::EmbeddedQ(P, { {0,1,2,3}, {4,5} }, why) && !A::EmbeddedQ(P, { {0,2}, {1,3} }, why),
              "embedded chains; a chain through another; two crossing diagonals");
    }

    std::printf("=== ear clipping and the mesh check ===\n");
    {
        // an L-shaped hexagon with a point in the middle of a side
        const std::vector<Pt> P { {0,0}, {2,0}, {4,0}, {4,2}, {2,2}, {2,4}, {0,4} };
        std::vector<Idx> V { 0,1,2,3,4,5,6 };
        std::vector<Tri> T;
        A::EarClip(V, { &P }, T);
        const A::MeshCheck c = A::CheckMesh(P, T, { 0,1,2,3,4,5,6 });
        check(V.empty() && T.size() == 5 && c.OKQ() && MeshArea(P, T) == 24.0, "an L-shaped polygon with a point on a side: 5 positive triangles, the right area");
        check(!A::CheckMesh(P, T, { 0,1,2,3,4,6,5 }).OKQ(), "the mesh check refuses the wrong boundary");
        std::vector<Tri> flipped = T;
        std::swap(flipped[0][0], flipped[0][1]);
        check(A::CheckMesh(P, flipped, { 0,1,2,3,4,5,6 }).bad == 1, "the mesh check counts a negative triangle");
    }
    {
        // a pinched polygon (a square with a triangular hole that touches its corner 0) and a slit one (a square cut from corner 0 to its centre)
        const std::vector<Pt> P { {0,0}, {6,0}, {6,6}, {0,6}, {2,4}, {4,2} };
        std::vector<Idx> V { 0,1,2,3,0,4,5 };
        std::vector<Tri> T;
        A::EarClip(V, { &P }, T);
        check(V.empty() && T.size() == 5 && MeshArea(P, T) == 60.0, "a polygon that touches itself at a corner");
        const std::vector<Pt> Q { {0,0}, {4,0}, {4,4}, {0,4}, {2,2} };
        std::vector<Idx> U { 0,1,2,3,0,4 };
        T.clear();
        A::EarClip(U, { &Q }, T);
        check(U.empty() && T.size() == 4 && MeshArea(Q, T) == 32.0, "a square slit from a corner to its centre");
    }
    {
        // ears common to two drawings: a square and the same square with corner 2 pushed in past the diagonal 1-3
        const std::vector<Pt> P { {0,0}, {4,0}, {4,4}, {0,4} }, Q { {0,0}, {4,0}, {1,1}, {0,4} };
        std::vector<Idx> V { 0,1,2,3 };
        std::vector<Tri> T;
        A::EarClip(V, { &P, &Q }, T);
        check(V.empty() && A::CheckMesh(P, T, { 0,1,2,3 }).OKQ() && A::CheckMesh(Q, T, { 0,1,2,3 }).OKQ(), "a triangulation valid in two drawings of a quadrilateral");
    }

    std::printf("=== Delaunay refinement ===\n");
    {
        std::vector<Pt> P { {0,0}, {8,0}, {8,1}, {4,1}, {4,6}, {0,6} };
        std::vector<Idx> V { 0,1,2,3,4,5 };
        std::vector<Tri> T;
        A::EarClip(V, { &P }, T);
        std::string why;
        std::vector<std::array<Idx,2>> parent;
        const bool okQ = A::Refine(P, T, 0.75, why, &parent);
        Real longest = 0, worst = 1;
        std::map<std::uint64_t,int> uses;
        for( const Tri & t : T )
        {
            worst = std::min(worst, A::Fat(P[t[0]], P[t[1]], P[t[2]]));
            for( Idx k = 0; k < 3; ++k ) { longest = std::max(longest, A::Dist(P[t[k]], P[t[(k + 1) % 3]])); ++uses[A::EdgeKey(t[k], t[(k + 1) % 3])]; }
        }
        bool conformingQ = true;
        Real boundary = 0;
        for( const auto & [k, n] : uses ) { conformingQ = conformingQ && n <= 2; if( n == 1 ) { boundary += A::Dist(P[k >> 32], P[k & 0xffffffffu]); } }
        check(okQ && T.size() > 50 && longest <= 0.75 && worst > 0.01, "every edge at most the target, no sliver (" + std::to_string(T.size()) + " triangles)");
        check(conformingQ && std::abs(MeshArea(P, T) - 2 * 28.0) < 1e-9 && std::abs(boundary - 28.0) < 1e-9 && parent.size() + 6 == P.size(),
              "conforming, the same area, the same boundary length, a parent edge for each new point");
        check(!A::Refine(P, T, 0.01, why, nullptr, 1000) && !why.empty(), "refinement stops at its triangle limit, with a reason");
    }

    std::printf("=== sparse solve ===\n");
    {
        // the 1D Laplacian on 200 points, and an unsymmetric perturbation of it
        const Idx n = 200;
        A::SparseMatrix M;
        for( Idx i = 0; i < n; ++i )
        {
            if( i > 0 ) { M.Add(i - 1, -1.0); }
            M.Add(i, 2.5);
            if( i + 1 < n ) { M.Add(i + 1, -1.3); }
            M.EndRow();
        }
        std::vector<std::vector<Real>> X0 (2, std::vector<Real>(n)), B (2), X;
        for( Idx i = 0; i < n; ++i ) { X0[0][i] = std::sin(0.1 * static_cast<Real>(i)); X0[1][i] = static_cast<Real>(i % 7); }
        M.Mul(X0[0], B[0]); M.Mul(X0[1], B[1]);
        Real res = 1, err = 0;
        std::string why;
        const bool okQ = A::Solve(M, B, X, res, why);
        for( Idx k = 0; k < 2; ++k ) { for( Idx i = 0; i < n; ++i ) { err = std::max(err, std::abs(X[k][i] - X0[k][i])); } }
        check(okQ && res < 1e-12 && err < 1e-10, "two right-hand sides, residual " + std::to_string(res));
    }
}

//==============================================================================
// Part 2: the morph.
//==============================================================================

struct Tally
{
    int diagrams = 0, skipped = 0, morphs = 0, failed = 0, refused_ok = 0;
    Idx most_points = 0, most_steiner = 0, most_rings = 0;
    Real worst = 1, residual = 0;
    std::map<std::string,int> reasons;
};

static bool verboseQ = false, only_filesQ = false;
static int  n_faces = 3, n_frames = 7;
static Int  max_crossings = 0;

/*!@brief `D` turned k quarter turns counterclockwise. */
static A::PlaneDrawing Turned( A::PlaneDrawing D, int k )
{
    auto turn = [k]( Pt p ) { for( int t = 0; t < (k & 3); ++t ) { p = Pt{ -p[1], p[0] }; } return p; };
    for( auto & c : D.crossings ) { c.p = turn(c.p); }
    for( auto & a : D.arcs ) { for( Pt & p : a.pts ) { p = turn(p); } }
    return D;
}

/*!@brief `D` sheared by `shear`, turned by `angle`, scaled and moved (an affine map of positive determinant). */
static A::PlaneDrawing Mapped( A::PlaneDrawing D, Real angle, Real shear, Real scale, Pt shift )
{
    const Real c = scale * std::cos(angle), s = scale * std::sin(angle);
    auto map = [=]( Pt p ) { const Real x = p[0] + shear * p[1]; return Pt{ c * x - s * p[1] + shift[0], s * x + c * p[1] + shift[1] }; };
    for( auto & cr : D.crossings ) { cr.p = map(cr.p); }
    for( auto & a : D.arcs ) { for( Pt & p : a.pts ) { p = map(p); } }
    D.Seat();       // gcc fuses the multiply-adds of `map` in one of these loops and not the other
    return D;
}

/*!@brief Is the chain `ch` of the frame `F` the polyline `L` exactly: L's
 * points, in order, with any further points on L's segments? */
static bool SamePolylineQ( const std::vector<Pt> & F, const std::vector<Idx> & ch, const A::Polyline & L, Real size )
{
    Idx j = 0;
    for( Idx v : ch )
    {
        if( j < L.size() && F[v] == L[j] ) { ++j; continue; }
        if( j == 0 || j >= L.size() ) { return false; }
        // between L[j-1] and L[j]: on that segment, to rounding
        if( std::abs(A::Cross(L[j-1], L[j], F[v])) > 1e-9 * size * A::Dist(L[j-1], L[j]) ) { return false; }
    }
    return j == L.size();
}

/*!@brief One morph and its contract. */
static void MorphAndCheck( const A::PlaneDrawing & D0, const A::PlaneDrawing & D1, const std::string & name, Tally & tally, Real round = 0.3 )
{
    ++tally.morphs;
    A::MorphResult R;
    std::string why;
    if( !A::Morph(D0, D1, A::EasedTimes(static_cast<Idx>(n_frames)), round, R, why) )
    {
        ++tally.failed;
        // the reason without its numbers, so that like failures are counted together
        std::string kind = why;
        for( char & ch : kind ) { if( ch >= '0' && ch <= '9' ) { ch = '#'; } }
        ++tally.reasons[kind];
        std::printf("  MORPH FAILED: %s: %s\n", name.c_str(), why.c_str());
        return;
    }
    const A::MorphProblem & pr = R.problem;
    tally.most_points  = std::max(tally.most_points, pr.P0.size());
    tally.most_steiner = std::max(tally.most_steiner, pr.steiner);
    tally.most_rings   = std::max(tally.most_rings, pr.rings);
    tally.worst        = std::min(tally.worst, R.report.worst);
    tally.residual     = std::max(tally.residual, R.report.residual);
    if( verboseQ )
    {
        std::printf("  %s: %zu points (%zu drawing, %zu overlay, %zu cut), %zu rings, turn %.1f deg, flatness %.2g\n", name.c_str(),
                    pr.P0.size(), pr.n_drawing, pr.steiner, pr.cut, pr.rings, pr.turn * 180 / A::Pi, R.report.worst);
    }

    bool positiveQ = (R.frames.size() == static_cast<std::size_t>(n_frames)), embeddedQ = true;
    for( const auto & F : R.frames )
    {
        for( const Tri & t : pr.tris ) { positiveQ = positiveQ && (A::Cross(F[t[0]], F[t[1]], F[t[2]]) > 0.0); }
        embeddedQ = embeddedQ && A::EmbeddedQ(F, pr.chains, why);
    }
    check(positiveQ, name + ": every triangle of every frame has positive area");
    check(embeddedQ, name + ": no two strand pieces meet in any frame (" + why + ")");
    bool endsQ = !R.frames.empty();
    const Real size = D0.Bounds().Size();
    for( Idx a = 0; endsQ && a < D0.arcs.size(); ++a )
    {
        if( !D0.ActiveQ(a) ) { continue; }
        endsQ = SamePolylineQ(R.frames.front(), pr.chains[a], D0.arcs[a].pts, size) && SamePolylineQ(R.frames.back(), pr.chains[a], D1.arcs[a].pts, size);
    }
    check(endsQ, name + ": the first and last frames are the two drawings exactly");
    const A::PlaneDrawing mid = A::DrawingAt(D0, R.match, R.frames[R.frames.size() / 2]);
    std::vector<Idx> n0, nm;
    check(D0.NextDarcs(n0, why) && mid.NextDarcs(nm, why) && n0 == nm, name + ": a middle frame is a drawing of the same plane graph");
}

static void TestDiagram( const PD_T & pd, const std::string & name, Tally & tally )
{
    if( !pd.ValidQ() || pd.CrossingCount() <= Int(0) ) { return; }
    if( max_crossings > 0 && pd.CrossingCount() > max_crossings ) { return; }
    if( pd.DiagramComponentCount() > Int(1) ) { ++tally.skipped; return; }
    ++tally.diagrams;
    const Int F = pd.FaceCount();
    // The exterior faces to try: OrthoDraw's own choice first, then spread over the rest.
    std::vector<Int> exteriors { Int(-1) };
    const Int want = (n_faces <= 0) ? F : std::min<Int>(F, n_faces);
    for( Int k = 1; k < want; ++k ) { exteriors.push_back((k * F) / want); }
    for( Int ext : exteriors )
    {
        std::string why;
        OD_T::Settings_T base {};
        OD_T H0 (pd, ext, base);
        A::PlaneDrawing D0;
        const std::string tag = name + " ext=" + std::to_string(ext);
        if( !A::FromOrthoDraw(H0, pd, D0, why) ) { check(false, tag + ": export: " + why); continue; }
        const Int f = H0.ExteriorFace();
        // the export is a drawing of pd: an embedding, with pd's faces, the exterior alone clockwise
        {
            A::Matched M;
            const bool okQ = A::Match(D0, D0, M, why);
            int clockwise = 0;
            bool extQ = false;
            for( Idx k = 0; okQ && k < M.faces.size(); ++k )
            {
                A::Polyline cyc;
                for( Idx v : M.faces[k] ) { cyc.push_back(M.P0[v]); }
                if( A::Area2(cyc) < 0 ) { ++clockwise; extQ = (k == M.exterior); }
            }
            check(okQ && M.faces.size() == static_cast<Idx>(F) && clockwise == 1 && extQ && A::EmbeddedQ(M.P0, M.chain, why),
                  tag + ": the exported drawing is embedded, has the diagram's faces, and its exterior is the one clockwise face (" + why + ")");
            Idx arcs = 0;
            for( const auto & comp : D0.components ) { arcs += comp.size(); }
            check(D0.components.size() == static_cast<Idx>(pd.LinkComponentCount()) && arcs == static_cast<Idx>(pd.ArcCount()), tag + ": the components hold every arc once");
        }
        std::vector<std::pair<std::string,A::PlaneDrawing>> others;
        auto layout = [&]( const std::string & label, const OD_T::Settings_T & s )
        {
            OD_T H (pd, f, s);
            A::PlaneDrawing D;
            if( H.ExteriorFace() == f && A::FromOrthoDraw(H, pd, D, why) ) { others.push_back({ label, D }); }
            else { check(false, tag + " " + label + ": no layout on the same exterior"); }
        };
        { OD_T::Settings_T s = base; s.randomize_bends = 4; layout("bends", s); }
        { OD_T::Settings_T s = base; s.compaction_method = OD_T::CompactionMethod_T::TopologicalNumbering; s.redistribute_bendsQ = false; layout("numbering", s); }
        others.push_back({ "quarter", Turned(D0, 1) });
        if( !others.empty() ) { others.push_back({ "bends+half", Turned(others[0].second, 2) }); }
        // not orthogonal, and at another scale and place: turned 37 degrees, sheared, 1000 times the size, far from the origin;
        // and a very small copy of the bends layout turned 200 degrees
        if( others.size() >= 2 ) { others.push_back({ "oblique", Mapped(others[1].second, 0.6435, 0.4, 1000.0, { 5e4, -3e4 }) }); }
        others.push_back({ "small", Mapped(others[0].second, 3.5, -0.25, 1e-6, { 0.25, 0.5 }) });
        for( const auto & [label, D1] : others ) { MorphAndCheck(D0, D1, tag + " -> " + label, tally); }
        if( others.size() >= 2 ) { MorphAndCheck(others[0].second, others[1].second, tag + " bends -> numbering", tally); }
        // without the steady centre and shape: the frames are only put at the interpolated place and size
        if( ext == Int(-1) ) { MorphAndCheck(D0, others.back().second, tag + " -> small, not steadied", tally, -1.0); }

        // what must be refused, with a reason: a mirror image, and another exterior face
        if( ext == Int(-1) )
        {
            A::PlaneDrawing mirror = D0;
            for( auto & c : mirror.crossings ) { c.p[0] = -c.p[0]; }
            for( auto & a : mirror.arcs ) { for( Pt & p : a.pts ) { p[0] = -p[0]; } }
            A::MorphResult R;
            why.clear();
            const bool mQ = !A::Morph(D0, mirror, { 0.0, 0.5, 1.0 }, 0.3, R, why) && !why.empty();
            check(mQ, tag + ": the mirror image is refused");
            OD_T H2 (pd, (f + 1) % F, base);
            A::PlaneDrawing D2;
            why.clear();
            const bool eQ = A::FromOrthoDraw(H2, pd, D2, why) && !A::Morph(D0, D2, { 0.0, 0.5, 1.0 }, 0.3, R, why) && !why.empty();
            check(eQ, tag + ": a drawing on another exterior face is refused");
            tally.refused_ok += (mQ ? 1 : 0) + (eQ ? 1 : 0);
        }
    }
}

static void TestPDFile( const std::string & path, Tally & tally )
{
    std::ifstream in (path);
    if( !in ) { check(false, "cannot read " + path); return; }
    std::vector<Int> rows;
    int count = 0;
    auto flush = [&]()
    {
        if( rows.empty() ) { return; }
        TestDiagram(PD_T::FromSignedPDCode(rows.data(), static_cast<Int>(rows.size() / 5)), path + " #" + std::to_string(count++), tally);
        rows.clear();
    };
    std::string line;
    while( std::getline(in, line) )
    {
        std::istringstream ls (line);
        std::vector<Int> v;
        for( Int x; ls >> x; ) { v.push_back(x); }
        if( v.size() >= 5 && ls.eof() ) { rows.insert(rows.end(), v.begin(), v.begin() + 5); } else { flush(); }
    }
    flush();
}

static void TestTraces( Tally & tally )
{
    namespace fs = std::filesystem;
    fs::path dir = fs::exists("pass_lasso.trace") ? "." : "test";
    std::vector<fs::path> files;
    if( fs::exists(dir) ) { for( const auto & e : fs::directory_iterator(dir) ) { if( e.path().extension() == ".trace" ) { files.push_back(e.path()); } } }
    std::sort(files.begin(), files.end());
    check(files.size() >= 10, "the committed traces were found (" + std::to_string(files.size()) + ")");
    for( const auto & file : files )
    {
        std::ifstream in (file);
        Trace_T::Reader reader (in);
        Trace_T::Record rec;
        std::string why;
        int n = 0;
        while( reader.Next(rec, why) == Trace_T::Status::Record )
        {
            const std::string name = file.filename().string() + " record " + std::to_string(n++);
            if( rec.state )  { TestDiagram(*rec.state, name + " state", tally); }
            if( rec.result ) { TestDiagram(*rec.result, name + " result", tally); }
        }
    }
}

int main( int argc, char ** argv )
{
    std::vector<std::string> files;
    for( int i = 1; i < argc; ++i )
    {
        const std::string arg = argv[i];
        if( arg.rfind("--faces=", 0) == 0 ) { n_faces = std::stoi(arg.substr(8)); }
        else if( arg.rfind("--frames=", 0) == 0 ) { n_frames = std::max(2, std::stoi(arg.substr(9))); }
        else if( arg.rfind("--max-crossings=", 0) == 0 ) { max_crossings = std::stoll(arg.substr(16)); }
        else if( arg == "--verbose" ) { verboseQ = true; }
        else if( arg == "--only-files" ) { only_filesQ = true; }
        else { files.push_back(arg); }
    }
    TestKits();

    std::printf("=== the morph's contract ===\n");
    Tally tally;
    if( !only_filesQ )
    {
        Int trefoil[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
        TestDiagram(PD_T::FromSignedPDCode(&trefoil[0], Int(3), false, false), "trefoil", tally);
        TestDiagram(PD_T::FigureEightKnot(Int(0)), "figure eight", tally);
        TestTraces(tally);
    }
    for( const std::string & f : files ) { TestPDFile(f, tally); }

    std::printf("diagrams=%d (not connected, skipped: %d)  morphs=%d  failed=%d  refused as they must be: %d\n",
                tally.diagrams, tally.skipped, tally.morphs, tally.failed, tally.refused_ok);
    std::printf("largest problem: %zu points, %zu overlay points, %zu rings; least triangle flatness in a frame %.3g; largest solve residual %.3g\n",
                tally.most_points, tally.most_steiner, tally.most_rings, tally.worst, tally.residual);
    for( const auto & [why, n] : tally.reasons ) { std::printf("  %d x %s\n", n, why.c_str()); }
    const bool okQ = (checks_failed == 0) && (tally.failed == 0) && (tally.morphs > 0);
    std::printf("ANIMATE MORPH CHECK %s (%d checks passed, %d failed; %d morphs, %d could not be made)\n",
                okQ ? "OK" : "FAILED", checks_passed, checks_failed, tally.morphs, tally.failed);
    return okQ ? 0 : 1;
}
