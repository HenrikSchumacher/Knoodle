// animate_scene_check -- the animator's scene model, FrameAt, ToJSON, the
// turn of the sphere and the rasterizer (src/Animate/, docs/animate-design.md).
//
// Standard library only, and fast: it builds small animations by hand and
// checks the display list and the pixels against numbers worked out here.
//
//   animate_scene_check            run the checks
//   animate_scene_check --dump=DIR also write a few frames as DIR/*.ppm

#include "../src/Animate/Scene.hpp"
#include "../src/Animate/Frame.hpp"
#include "../src/Animate/Json.hpp"
#include "../src/Animate/Raster.hpp"
#include "../src/Animate/Turn.hpp"
#include "../src/Animate/Tray.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace Knoodle::Animate;

static int g_checks = 0, g_failed = 0;

static void Check( bool ok, const std::string & what )
{
    ++g_checks;
    if( !ok ) { ++g_failed; std::printf("FAILED: %s\n", what.c_str()); }
}
static void Near( Real got, Real want, Real tol, const std::string & what )
{
    ++g_checks;
    if( !(std::abs(got - want) <= tol) ) { ++g_failed; std::printf("FAILED: %s: got %.9g, want %.9g (tol %.3g)\n", what.c_str(), got, want, tol); }
}

static Real Length( const std::vector<P2> & p, bool closed )
{
    Real L = 0;
    for( std::size_t i = 1; i < p.size() + (closed ? 1 : 0); ++i ) { L += std::hypot(p[i % p.size()].x - p[i-1].x, p[i % p.size()].y - p[i-1].y); }
    return L;
}
static Real DistTo( const std::vector<P2> & p, const P2 & q )
{
    Real best = 1e300;
    for( std::size_t i = 1; i < p.size(); ++i )
    {
        const Real dx = p[i].x - p[i-1].x, dy = p[i].y - p[i-1].y, d2 = dx * dx + dy * dy;
        const Real t = (d2 > 0) ? std::min(1.0, std::max(0.0, ((q.x - p[i-1].x) * dx + (q.y - p[i-1].y) * dy) / d2)) : 0;
        best = std::min(best, std::hypot(q.x - p[i-1].x - t * dx, q.y - p[i-1].y - t * dy));
    }
    return best;
}
static Real Sum( const Image & img )            // of the red channel
{
    Real s = 0;
    for( std::size_t i = 0; i < img.rgb.size(); i += 3 ) { s += img.rgb[i]; }
    return s;
}
static void WritePPM( const std::string & path, const Image & img )
{
    std::ofstream o (path, std::ios::binary);
    o << "P6\n" << img.W << " " << img.H << "\n255\n";
    const auto b = img.Bytes();
    o.write(reinterpret_cast<const char *>(b.data()), std::streamsize(b.size()));
}

// A syntax check of a JSON text: values, strings, numbers, nesting. Returns
// the number of objects, or -1.
static long JsonObjects( const std::string & s )
{
    std::size_t i = 0; long objects = 0; bool ok = true;
    auto ws = [&] { while( i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\t' || s[i] == '\r') ) { ++i; } };
    auto value = [&]( auto && self ) -> void
    {
        ws();
        if( i >= s.size() ) { ok = false; return; }
        if( s[i] == '{' || s[i] == '[' )
        {
            const bool objQ = (s[i] == '{'); const char close = objQ ? '}' : ']';
            if( objQ ) { ++objects; }
            ++i; ws();
            if( i < s.size() && s[i] == close ) { ++i; return; }
            while( ok )
            {
                if( objQ )
                {
                    ws(); if( i >= s.size() || s[i] != '"' ) { ok = false; return; }
                    ++i; while( i < s.size() && s[i] != '"' ) { i += (s[i] == '\\') ? 2 : 1; } ++i;
                    ws(); if( i >= s.size() || s[i] != ':' ) { ok = false; return; } ++i;
                }
                self(self); ws();
                if( i < s.size() && s[i] == ',' ) { ++i; continue; }
                if( i < s.size() && s[i] == close ) { ++i; return; }
                ok = false;
            }
        }
        else if( s[i] == '"' ) { ++i; while( i < s.size() && s[i] != '"' ) { i += (s[i] == '\\') ? 2 : 1; } if( i >= s.size() ) { ok = false; } ++i; }
        else if( s.compare(i, 4, "true") == 0 ) { i += 4; }
        else if( s.compare(i, 5, "false") == 0 ) { i += 5; }
        else if( s.compare(i, 4, "null") == 0 ) { i += 4; }
        else
        {
            const std::size_t j = i;
            if( s[i] == '-' ) { ++i; }
            while( i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.') ) { ++i; }
            if( i == j || s[i-1] == '-' ) { ok = false; }
        }
    };
    value(value); ws();
    return (ok && i == s.size()) ? objects : -1;
}

// Two squares that cross twice, as a Hopf link: path 0 goes under path 1 at
// (4,2), and path 1 goes under path 0 at (2,4).
static Scene HopfScene()
{
    Scene sc;
    sc.label = "Reshape diagram"; sc.crossings_before = sc.crossings_after = 2;
    sc.duration = 2; sc.easing = Easing::Linear;
    sc.view0 = sc.view1 = ViewBox{ -1, -1, 7, 7 };
    sc.paths.resize(2);
    sc.paths[0].closed = sc.paths[1].closed = true;
    sc.paths[0].color = 0; sc.paths[1].color = 1;
    Key k;
    k.pts = { { {0,0}, {4,0}, {4,2}, {4,4}, {2,4}, {0,4} }, { {2,2}, {4,2}, {6,2}, {6,6}, {2,6}, {2,4} } };
    k.cuts = { Cut{ 0, 2, 1, 1 }, Cut{ 1, 5, 0, 4 } };
    sc.keys.push_back(k);
    for( auto & path : k.pts ) { for( P2 & p : path ) { p.x += 1; } }      // the second key: one unit to the right
    sc.keys.push_back(k);
    return sc;
}

// The same link as space curves of (x, y, h).
static void HopfCurves( std::vector<Curve3> & curves, std::vector<Crossing3> & crossings )
{
    const Real d = 0.35;
    curves.resize(2);
    curves[0].color = 0; curves[1].color = 1;
    curves[0].pts = { {0,0,0}, {4,0,0}, {4,2,-d}, {4,4,0}, {2,4,d}, {0,4,0} };
    curves[1].pts = { {2,2,0}, {4,2,d}, {6,2,0}, {6,6,0}, {2,6,0}, {2,4,-d} };
    crossings = { Crossing3{ 0, 2, 1, 1 }, Crossing3{ 0, 4, 1, 5 } };
}

int main( int argc, char ** argv )
{
    std::string dump;
    for( int i = 1; i < argc; ++i ) { const std::string a = argv[i]; if( a.rfind("--dump=", 0) == 0 ) { dump = a.substr(7); } }

    const Style st = Style::Talk();
    const Real W = 800, H = 800;

    // ---- the timeline ---------------------------------------------------
    {
        Animation A; std::size_t s = 9; Real local = -1;
        Check(!A.Locate(0, s, local), "an empty animation has no scene");
        A.scenes.push_back(HopfScene()); A.scenes.push_back(HopfScene()); A.scenes[1].duration = 3;
        Near(A.Duration(), 5, 0, "duration is the sum of the scenes'");
        Check(A.Locate(1.5, s, local) && s == 0, "1.5 s is in scene 0"); Near(local, 1.5, 1e-12, "local time in scene 0");
        Check(A.Locate(2.0, s, local) && s == 1, "2.0 s starts scene 1"); Near(local, 0, 1e-12, "local time at a boundary");
        Check(A.Locate(99, s, local) && s == 1, "past the end is the last scene"); Near(local, 3, 1e-12, "... at its end");
        Near(Smoother(0.5), 0.5, 1e-15, "Smoother(1/2)"); Near(Smoother(-1), 0, 0, "Smoother clamps below"); Near(Smoother(2), 1, 0, "Smoother clamps above");
    }

    // ---- a flat scene: gaps, interpolation, the camera ----------------------
    Animation flat; flat.source = "hand-made"; flat.scenes.push_back(HopfScene());
    {
        const Frame f = FrameAt(flat, 0, st, W, H);
        const Real s = W * (1 - 2 * st.margin) / 8, wpx = std::max(st.stroke * s, st.min_stroke * H), gap = st.gap * wpx;
        Check(f.items.size() == 2, "two crossings, one on each closed path: two open strokes");
        if( f.items.size() == 2 )
        {
            Check(!f.items[0].stroke.closed && !f.items[1].stroke.closed, "a cut path is drawn open");
            Near(Length(f.items[0].stroke.pts, false), 16 * s - 2 * gap, 1e-6, "path 0: its length less one gap");
            Near(Length(f.items[1].stroke.pts, false), 16 * s - 2 * gap, 1e-6, "path 1: its length less one gap");
            const P2 x0 { W / 2 + (4 - 3) * s, H / 2 - (2 - 3) * s }, x1 { W / 2 + (2 - 3) * s, H / 2 - (4 - 3) * s };
            Near(DistTo(f.items[0].stroke.pts, x0), gap, 1e-6, "path 0 stops a gap short of the crossing it goes under");
            Near(DistTo(f.items[1].stroke.pts, x0), 0, 1e-6, "path 1 runs through that crossing");
            Near(DistTo(f.items[1].stroke.pts, x1), gap, 1e-6, "path 1 stops a gap short of the other crossing");
            Near(DistTo(f.items[0].stroke.pts, x1), 0, 1e-6, "path 0 runs through the other crossing");
            Near(f.items[0].stroke.width, wpx, 1e-12, "stroke width");
            Check(f.items[0].stroke.color.r == st.ink.r && f.items[0].stroke.color.a == 1, "no component colours in the talk style: ink");
        }
        const Frame m = FrameAt(flat, 1, st, W, H);     // half way, Linear: half a unit to the right
        if( f.items.size() == 2 && m.items.size() == 2 )
        {
            Near(DistTo(m.items[1].stroke.pts, P2{ W / 2 + (4.5 - 3) * s, H / 2 - (2 - 3) * s }), 0, 1e-6, "half way, the drawing has moved half a unit");
        }
        Style colours = Style::Light();
        const Frame c = FrameAt(flat, 0, colours, W, H);
        Check(c.items.size() == 2 && c.items[1].stroke.color.b == colours.components[1].b, "component colours come from the style");
        Check(f.label == "Reshape diagram" && f.crossings_before == 2, "the frame carries the scene's label and counts");
    }
    {
        // unequal point counts: the nearer key, never a blend
        Animation A; A.scenes.push_back(HopfScene());
        A.scenes[0].keys[1].pts[0].push_back(P2{ 0, 2 });
        const Frame a = FrameAt(A, 0.9, st, W, H), b = FrameAt(A, 1.1, st, W, H), k0 = FrameAt(A, 0, st, W, H), k1 = FrameAt(A, 2, st, W, H);
        Check(a.items.size() == 2 && k0.items.size() == 2 && a.items[1].stroke.pts.size() == k0.items[1].stroke.pts.size()
              && a.items[1].stroke.pts[1].x == k0.items[1].stroke.pts[1].x, "before half way: the first key as it is");
        Check(b.items.size() == 2 && k1.items.size() == 2 && b.items[1].stroke.pts[1].x == k1.items[1].stroke.pts[1].x, "after half way: the second key as it is");
    }
    {
        // a pass disk, opacity, layers, a slanted crossing, a token on the screen
        Scene sc;
        sc.duration = 1; sc.easing = Easing::Linear; sc.view0 = sc.view1 = ViewBox{ 0, 0, 10, 10 };
        sc.paths.resize(4);
        sc.paths[0].role = Role::Moving; sc.paths[0].layer = 2; sc.paths[0].alpha0 = 1; sc.paths[0].alpha1 = 0.3;
        sc.paths[1].role = Role::Route;  sc.paths[1].layer = 1;
        sc.paths[2].role = Role::Strand; sc.paths[2].layer = 0;
        sc.paths[3].role = Role::Token;  sc.paths[3].closed = true; sc.paths[3].space = Space::Screen; sc.paths[3].color = 2; sc.paths[3].layer = 3;
        sc.fills.push_back(Fill{ Role::Disk, 1, { { 0, false }, { 1, true } } });
        Key k;
        k.pts = { { {2,2}, {5,6}, {8,2} }, { {2,2}, {8,2} }, { {1,1}, {5,5}, {9,9} }, { {0.02,0.02}, {0.08,0.02}, {0.08,0.08}, {0.02,0.08} } };
        k.cuts = { Cut{ 2, 1, 1, 0 } };     // the still strand goes under the route's line, at 45 degrees
        sc.keys.push_back(k);
        Animation A; A.scenes.push_back(sc);
        const Frame f = FrameAt(A, 0, st, W, H);
        // order: strand (two pieces, layer 0), disk then route (layer 1), moving (2), the token's outline (3)
        Check(f.items.size() == 6, "six items in layer order");
        if( f.items.size() == 6 )
        {
            Check(!f.items[0].fillQ && !f.items[1].fillQ && f.items[2].fillQ && !f.items[3].fillQ && !f.items[4].fillQ && !f.items[5].fillQ, "a fill comes before a path of its layer; a token is an outline, never filled");
            Check(f.items[2].shape.rings.size() == 1 && f.items[2].shape.rings[0].size() == 5, "the disk is the strand followed by the route reversed");
            Near(f.items[2].shape.color.a, st.disk.a, 0, "the disk takes the style's colour");
            const Real s = W * (1 - 2 * st.margin) / 10, wpx = std::max(st.stroke * s, st.min_stroke * H);
            const Real total = Length(f.items[0].stroke.pts, false) + Length(f.items[1].stroke.pts, false);
            Near(total, 8 * std::sqrt(2.0) * s - 2 * st.gap * wpx * std::sqrt(2.0), 1e-6, "a 45 degree crossing has a gap sqrt(2) wider");
            Near(f.items[3].stroke.width, wpx * st.route_width, 1e-12, "the route is thinner");
            Near(f.items[5].stroke.pts[0].x, 0.02 * H, 1e-9, "a screen path is placed by the picture's height ...");
            Near(f.items[5].stroke.pts[0].y, H - 0.02 * H, 1e-9, "... from its bottom left");
        }
        const Frame g = FrameAt(A, 1, st, W, H);
        if( g.items.size() == 6 ) { Near(g.items[4].stroke.color.a, 0.3 * st.moving.a, 1e-12, "opacity runs from alpha0 to alpha1"); }
        A.scenes[0].view1 = ViewBox{ 0, 0, 20, 20 };
        const Frame z = FrameAt(A, 1, st, W, H);
        if( z.items.size() == 6 )
        {
            Near(z.items[5].stroke.pts[0].x, 0.02 * H, 1e-9, "the camera does not move a screen path");
            Near(z.items[3].stroke.pts[1].x - z.items[3].stroke.pts[0].x, 6 * W * (1 - 2 * st.margin) / 20, 1e-6, "the camera's box at the end of the scene");
        }
    }

    // ---- a space scene, Affine: crossings from the geometry -----------------
    {
        Scene sc; sc.kind = Scene::Kind::Space; sc.duration = 2; sc.tilt_b = 0.5; sc.depth = 2;
        sc.view0 = sc.view1 = ViewBox{ -3, -3, 5, 5 };
        Stage S; S.map = Map3::Affine; S.duration = 2; S.angle0 = 0; S.angle1 = 0;
        S.curves.resize(2);
        S.curves[0].pts = { {-2,-2,0}, {2,-2,0}, {2,2,0}, {-2,2,0} };          // flat, in z = 0
        S.curves[1].pts = { {0,0,-2}, {4,0,-2}, {4,0,2}, {0,0,2} };            // upright, in y = 0
        sc.stages.push_back(S);
        Animation A; A.scenes.push_back(sc);
        const Frame f = FrameAt(A, 0, st, W, H);
        const Real s = W * (1 - 2 * st.margin) / 8, wpx = std::max(st.stroke * s, st.min_stroke * H), gap = st.gap * wpx;
        Check(f.items.size() == 2, "two linked squares seen at a tilt: two crossings, one cut on each");
        if( f.items.size() == 2 )
        {
            const bool firstIsFlat = std::abs(Length(f.items[0].stroke.pts, false) - (16 * s - 2 * gap)) < 1e-6;
            const Stroke & flatSq = f.items[firstIsFlat ? 0 : 1].stroke, & upSq = f.items[firstIsFlat ? 1 : 0].stroke;
            Near(Length(flatSq.pts, false), 16 * s - 2 * gap, 1e-6, "the flat square: its length less one gap");
            Near(Length(upSq.pts, false), 12 * s - 2 * gap, 1e-6, "the upright square (drawn 4 by 2): its length less one gap");
            // screen y = y - z/2: the far edge (z = -2) of the upright square is drawn at y = 1 and goes under the flat one
            const P2 far { W / 2 + (2 - 1) * s, H / 2 - (1 - 1) * s }, nearX { W / 2 + (2 - 1) * s, H / 2 - (-1 - 1) * s };
            Near(DistTo(upSq.pts, far), gap, 1e-6, "the far edge is cut where the flat square passes over it");
            Near(DistTo(flatSq.pts, nearX), gap, 1e-6, "the flat square is cut where the near edge passes over it");
        }
        A.scenes[0].stages[0].fog0 = A.scenes[0].stages[0].fog1 = 1;
        const Frame g = FrameAt(A, 0, st, W, H);
        Check(g.items.size() > 2, "with depth colouring the curves are drawn in runs of one depth level");
        bool ordered = true; Real lastw = 0;
        for( const Item & it : g.items ) { if( it.stroke.width + 1e-12 < lastw ) { ordered = false; } lastw = it.stroke.width; }
        Check(ordered, "far to near: the runs get no thinner as they go");
        // a quarter turn about z keeps the picture's structure
        A.scenes[0].stages[0].fog0 = A.scenes[0].stages[0].fog1 = 0;
        A.scenes[0].stages[0].angle0 = A.scenes[0].stages[0].angle1 = 1.5707963267948966;
        Check(FrameAt(A, 0, st, W, H).items.size() == 2, "turned a quarter about z it is still two strokes");
        const P3 p = StagePoint(A.scenes[0].stages[0], P3{ 1, 0, 3 }, 1.5707963267948966, 0.5, 0, 0);
        Near(p.x, 0, 1e-12, "Affine: (1,0) turns to (0,1) ..."); Near(p.y, 1, 1e-12, "... about z"); Near(p.z, 1.5, 1e-12, "... and z is scaled by the lift");
    }

    // ---- the turn of the sphere ----------------------------------------------
    std::vector<Curve3> curves; std::vector<Crossing3> crossings;
    HopfCurves(curves, crossings);
    const P2 pole { 3, 3 };                         // inside the lens the two squares share
    const TurnResult T = Turn(curves, crossings, pole);
    {
        Check(T.stages.size() == 4, "a turn has four stages: roll up, turn, even out, unroll");
        const Stage & up = T.stages[0], & turn = T.stages[1], & un = T.stages[2];
        const Real R = T.radius;
        Check(R > 0 && T.angle > 0.5 && T.angle < 3.2, "a sphere and an angle");
        // rolled flat is the plane; rolled up, every point is on its sphere, h inside it
        Real worstFlat = 0, worstSphere = 0, worstSeam = 0;
        for( std::size_t c = 0; c < up.curves.size(); ++c )
        {
            for( std::size_t i = 0; i < up.curves[c].pts.size(); ++i )
            {
                const P3 & p = up.curves[c].pts[i];
                const P3 a = StagePoint(up, p, 0, 1, 0, 0);
                worstFlat = std::max(worstFlat, std::abs(a.x - p.x) + std::abs(a.y - p.y) + std::abs(a.z));
                const P3 b = StagePoint(up, p, 0, 1, 1, 0);
                worstSphere = std::max(worstSphere, std::abs(std::sqrt((b.x - up.centre.x) * (b.x - up.centre.x) + (b.y - up.centre.y) * (b.y - up.centre.y) + (b.z - R) * (b.z - R)) - (R - p.z)));
                // the end of the turn is the start of the unroll, point for point
                const P3 e = StagePoint(turn, p, turn.angle1, 1, 1, 0), s0 = StagePoint(un, un.curves[c].pts[i], 0, 1, 1, 0);
                worstSeam = std::max(worstSeam, std::abs(e.x - s0.x) + std::abs(e.y - s0.y) + std::abs(e.z - s0.z));
            }
        }
        Near(worstFlat, 0, 1e-12, "rolled flat, the curve is the drawing, in the plane");
        Near(worstSphere, 0, 1e-9, "rolled up, a point with height h is h inside the sphere");
        Near(worstSeam, 0, 1e-8, "the turn ends where the unroll starts");
        const P3 top = StagePoint(turn, P3{ pole.x, pole.y, 0 }, turn.angle1, 1, 1, 0);
        Near(top.z, 2 * R, 1e-9, "the turn takes the pole to the top");
        // the drawing left: the size and centre of the one that came in
        Real x0 = 1e300, y0 = 1e300, x1 = -1e300, y1 = -1e300;
        for( const Curve3 & c : T.left ) { for( const P3 & p : c.pts ) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); } }
        Near(std::max(x1 - x0, y1 - y0), 6, 1e-9, "the drawing left has the size of the one that came in");
        Near((x0 + x1) / 2, 3, 1e-9, "... and its centre, in x"); Near((y0 + y1) / 2, 3, 1e-9, "... and in y");
        // it is a drawing: no two of its pieces cross, except at the two crossings
        long proper = 0, pieces = 0;
        std::vector<std::pair<P3,P3>> segs;
        for( const Curve3 & c : T.left ) { for( std::size_t i = 0; i < c.pts.size(); ++i ) { segs.emplace_back(c.pts[i], c.pts[(i + 1) % c.pts.size()]); ++pieces; } }
        auto orient = []( const P3 & a, const P3 & b, const P3 & c ) { return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); };
        auto same = []( const P3 & a, const P3 & b ) { return std::abs(a.x - b.x) < 1e-9 && std::abs(a.y - b.y) < 1e-9; };
        for( std::size_t i = 0; i < segs.size(); ++i ) { for( std::size_t j = i + 1; j < segs.size(); ++j )
        {
            const auto & [a, b] = segs[i]; const auto & [c, d] = segs[j];
            if( same(a, c) || same(a, d) || same(b, c) || same(b, d) ) { continue; }
            if( orient(a, b, c) * orient(a, b, d) < 0 && orient(c, d, a) * orient(c, d, b) < 0 ) { ++proper; }
        } }
        Check(proper == 0, "the drawing left has no crossings but the diagram's own (" + std::to_string(pieces) + " pieces)");
        // the pole is outside it now: the unbounded face. The old far corner (0,0) is inside the outermost curve.
        Check(T.crossings.size() == 2, "the crossings come through, re-indexed");
        for( const Crossing3 & x : T.crossings )
        {
            const P3 & a = T.left[std::size_t(x.a)].pts[std::size_t(x.ia)], & b = T.left[std::size_t(x.b)].pts[std::size_t(x.ib)];
            Near(std::hypot(a.x - b.x, a.y - b.y), 0, 1e-9, "a crossing's two passages are still one point of the plane");
            Check(a.z * b.z < 0, "... one above, one below");
        }
        // the finish is one to one along every ray from its origin
        bool monotone = true; Real last = -1;
        for( int i = 0; i <= 400; ++i )
        {
            const Real r = un.fin_rmin * 0.25 * std::pow(1.03, i);
            const P2 z = Finish(un, P2{ un.fin_o.x + r, un.fin_o.y }, 0.6), o = Finish(un, un.fin_o, 0.6);
            const Real rr = std::hypot(z.x - o.x, z.y - o.y);
            if( rr <= last ) { monotone = false; } last = rr;
        }
        Check(monotone, "half finished, the finish still increases along a ray");
    }
    Animation turned;
    {
        Scene sc; sc.kind = Scene::Kind::Space; sc.label = "Turn the sphere"; sc.stages = T.stages;
        sc.duration = 0; for( const Stage & s : sc.stages ) { sc.duration += s.duration; }
        sc.tilt_b = 0.45; sc.depth = T.radius;
        sc.view0 = sc.view1 = ViewBox{ -3, -2, 9, 10 };
        turned.scenes.push_back(sc);
        const Frame a = FrameAt(turned, 0, st, W, H);
        Check(a.items.size() == 2, "the turn's first frame is the flat drawing: two strokes, one gap each");
        if( a.items.size() == 2 )
        {
            const Real s = std::min(W * (1 - 2 * st.margin) / 12, H * (1 - 2 * st.margin) / 12);
                        Near(DistTo(a.items[1].stroke.pts, P2{ W / 2 + (4 - 3) * s, H / 2 - (2 - 4) * s }), 0, 1e-6, "flat, the heights are not in the picture: the over strand runs through the crossing itself");
        }
        const Frame last = FrameAt(turned, sc.duration, st, W, H);
        Check(last.items.size() == 2, "its last frame is flat again: two strokes");
        const Frame mid = FrameAt(turned, T.stages[0].duration + T.stages[1].duration / 2, st, W, H);
        Check(mid.items.size() > 4, "mid-turn it is drawn in depth levels");
    }

    // ---- spin-offs and the tray -----------------------------------------------
    Animation spun;
    {
        TrayOptions o; o.aspect = 1; o.size = 0.1; o.gap = 0.05; o.margin = 0.05;      // six to a row
        Near(TrayCentre(0, o).x, 0.1, 1e-12, "the first square is at the bottom left ..."); Near(TrayCentre(0, o).y, 0.1, 1e-12, "... in y too");
        Near(TrayCentre(5, o).x, 0.1 + 5 * 0.15, 1e-12, "the row fills to the right"); Near(TrayCentre(5, o).y, 0.1, 1e-12, "... on the bottom");
        Near(TrayCentre(6, o).x, 0.1, 1e-12, "the seventh starts the next row ..."); Near(TrayCentre(6, o).y, 0.25, 1e-12, "... one up");
        const std::vector<P2> sq = RoundedSquare(P2{ 0, 0 }, 2, 0.25, 4000);
        Real area = 0, len = 0, worst = 0;
        for( std::size_t i = 0; i < sq.size(); ++i )
        {
            const P2 & a = sq[i], & b = sq[(i + 1) % sq.size()];
            area += (a.x * b.y - a.y * b.x) / 2; len += std::hypot(b.x - a.x, b.y - a.y);
            worst = std::max(worst, std::max(std::abs(a.x), std::abs(a.y)));
        }
        Near(area, 4 - (4 - 3.14159265358979) * 0.25, 1e-4, "a rounded square's area (counterclockwise)");
        Near(len, 4 * 1 + 3.14159265358979, 1e-4, "... and its length");
        Near(worst, 1, 1e-9, "... inside its box");
        Near(RoundedSquare(P2{ 0, 0 }, 2, 0.25, 8, false, 1.5707963267948966)[0].y, 1, 1e-9, "it starts in the direction asked for");

        Scene sc = HopfScene();                                 // two keys
        sc.view0 = sc.view1 = ViewBox{ -1, -1, 7, 7 };
        const std::vector<P2> loop { {0,0}, {4,0}, {4,4}, {0,4} };
        Check(AddSpinOff(sc, loop, 1, 2), "a spin-off is added to a scene of two keys");
        AddTray(sc, { 0, 3 });
        Check(sc.paths.size() == 5 && sc.keys[0].pts.size() == 5 && sc.keys[1].pts.size() == 5, "one path for the spin-off, one for each square in the tray");
        Check(sc.paths[2].space == Space::WorldToScreen && sc.paths[3].space == Space::Screen && sc.paths[2].color == 1 && sc.paths[4].color == 3, "spaces and colours");
        Scene one = HopfScene(); one.keys.pop_back();
        Check(!AddSpinOff(one, loop, 1, 0) && one.paths.size() == 2, "a scene of one key is refused and left alone");
        // at the start it is the loop, in the world; at the end the square in its place, on the screen
        spun.scenes.push_back(sc);
        const Style light = Style::Light();
        const Frame a = FrameAt(spun, 0, light, 1600, 900), b = FrameAt(spun, sc.duration, light, 1600, 900);
        const Stroke * sa = nullptr, * sb = nullptr;
        for( const Item & it : a.items ) { if( !it.fillQ && it.stroke.closed && it.stroke.pts.size() == 48 && !sa ) { sa = &it.stroke; } }
        for( const Item & it : b.items ) { if( !it.fillQ && it.stroke.closed && it.stroke.pts.size() == 48 && !sb ) { sb = &it.stroke; } }
        Check(sa && sb, "the spin-off is a closed stroke of 48 points");
        if( sa && sb )
        {
            const Real s = 900 * (1 - 2 * light.margin) / 8;
            Near(Length(sa->pts, true), 16 * s, 1e-6, "it starts as the loop, in the world");
            Near(Length(sb->pts, true), 900 * 0.045 * (4 - 8 * 0.28 + 2 * 3.14159265358979 * 0.28), 0.5, "it ends as a rounded square on the screen (48 chords)");
            const P2 c = TrayCentre(2);
            Real cx = 0, cy = 0; for( const P2 & p : sb->pts ) { cx += p.x / 48; cy += p.y / 48; }
            Near(cx, c.x * 900, 0.05, "... at its place in the tray"); Near(cy, 900 - c.y * 900, 0.05, "... from the bottom");
            Check(sb->color.b == light.components[1].b, "... in its component's colour");
        }
        if( !dump.empty() ) { for( int i = 0; i < 3; ++i ) { WritePPM(dump + "/spin" + std::to_string(i) + ".ppm", Draw(FrameAt(spun, sc.duration * i / 2.0, light, 800, 450))); } }
    }

    // ---- ToJSON -----------------------------------------------------------
    {
        Animation A = flat; A.scenes.push_back(turned.scenes[0]); A.source = "a \"quoted\" name";
        const std::string j = ToJSON(A);
        const long objects = JsonObjects(j);
        Check(objects > 0, "the JSON parses");
        Check(j.find("\"Format\":\"knoodle-animation\",\"Version\":2") != std::string::npos, "format and version");
        Check(j.find("a \\\"quoted\\\" name") != std::string::npos, "strings are escaped");
        Check(j.find("\"Kind\":\"Flat\"") != std::string::npos && j.find("\"Kind\":\"Space\"") != std::string::npos, "both kinds of scene");
        Check(j.find("\"Cuts\":[[0,2,1,1],[1,5,0,4]]") != std::string::npos, "cuts as written");
        Check(j.find("[[0,0],[4,0],[4,2],[4,4],[2,4],[0,4]]") != std::string::npos, "whole numbers are written as integers");
        Check(j.find("\"Map\":\"Roll\"") != std::string::npos && j.find("\"Finish\":{") != std::string::npos, "a roll stage with its finish");
        if( !dump.empty() )
        {
            // everything in one file, and what FrameAt makes of it, for checking another renderer against this one
            A.scenes.push_back(spun.scenes[0]);
            std::ofstream(dump + "/all.json") << ToJSON(A);
            std::ofstream fr (dump + "/frames.txt");
            const Style light = Style::Light();
            for( int i = 0; i <= 60; ++i )
            {
                const Real t = A.Duration() * i / 60.0;
                const Frame f = FrameAt(A, t, light, 1600, 900);
                Real len = 0, wid = 0; long fills = 0;
                for( const Item & it : f.items ) { if( it.fillQ ) { fills += long(it.shape.rings[0].size()); } else { len += Length(it.stroke.pts, it.stroke.closed); wid += it.stroke.width; } }
                fr << t << " " << f.items.size() << " " << len << " " << wid << " " << fills << "\n";
            }
        }
    }

    // ---- the rasterizer -----------------------------------------------------
    {
        const RGBA black { 0, 0, 0, 1 }, white { 1, 1, 1, 1 };
        Frame f; f.width = 100; f.height = 80; f.background = black;
        Item sq; sq.fillQ = true; sq.shape.color = white; sq.shape.rings = { { {10.25,10.5}, {20.25,10.5}, {20.25,20.5}, {10.25,20.5} } };
        f.items = { sq };
        Near(Sum(Draw(f)), 100, 0.01, "a 10 by 10 square off the pixel grid covers 100 pixels' worth");
        Item ln; ln.stroke.color = white; ln.stroke.width = 4; ln.stroke.pts = { {10.3, 40.2}, {60.3, 40.2} };
        f.items = { ln };
        Near(Sum(Draw(f)), 200, 0.01, "a butt stroke 50 long and 4 wide covers 200");
        ln.stroke.pts = { {20,20}, {60,20}, {60,60}, {20,60} }; ln.stroke.closed = true;
        f.items = { ln };
        Near(Sum(Draw(f)), 44 * 44 - 36 * 36, 0.01, "a closed square with miter joins is a square frame");
        ln.stroke.closed = false; ln.stroke.round = true; ln.stroke.pts = { {30,40}, {70,40} }; ln.stroke.width = 10;
        f.items = { ln };
        Near(Sum(Draw(f)), 400 + 3.14159265 * 25, 1.5, "round caps add a disc");
        ln.stroke.color = RGBA{ 1, 1, 1, 0.5 }; ln.stroke.pts = { {20,20}, {60,40}, {20,44} }; ln.stroke.width = 8;
        f.items = { ln };
        const Image over = Draw(f); float mx = 0;
        for( std::size_t i = 0; i < over.rgb.size(); i += 3 ) { mx = std::max(mx, over.rgb[i]); }
        Near(mx, 0.5, 1e-6, "a translucent stroke is no darker where its pieces overlap");
        const auto bytes = Draw(f).Bytes();
        Check(bytes.size() == 100 * 80 * 3, "rgb24: three bytes a pixel");
        f.items = { sq }; f.items[0].shape.rings[0] = { {-50,-50}, {500,-50}, {500,500}, {-50,500} };
        Near(Sum(Draw(f)), 8000, 0.01, "a shape larger than the picture fills it and no more");
    }

    // ---- pictures, for a person to look at ----------------------------------
    if( !dump.empty() )
    {
        const Style light = Style::Light();
        WritePPM(dump + "/flat.ppm", Draw(FrameAt(flat, 0.7, light, 640, 640)));
        const Real d = turned.scenes[0].duration;
        int n = 0;
        for( Real t : { 0.0, 0.2 * d, 0.33 * d, 0.5 * d, 0.66 * d, 0.85 * d, d } )
        {
            WritePPM(dump + "/turn" + std::to_string(n++) + ".ppm", Draw(FrameAt(turned, t, light, 640, 640)));
        }
    }

    std::printf("animate_scene_check: (%d checks, %d failed)\n", g_checks, g_failed);
    return g_failed ? 1 : 0;
}
