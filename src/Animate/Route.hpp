#pragma once

// Routes for pass moves, through the faces of the drawing itself
// (docs/animate-design.md). A pass move names the faces its strand's new
// route runs through by the arcs it crosses on the way. Here each face is a
// polygon; a leg of the route enters at a point of the polygon's boundary and
// leaves at another, and in between it follows the skeleton of the polygon:
// its medial axis, the points as far from one edge as from another, which
// keeps the route as clear of the strands as the face allows. A leg, once laid, cuts
// its polygon in two, so later legs cannot cross it.
//
// Nothing here needs a lattice: any drawing will do, a layout or not.
//
// Standard library, PlaneDrawing.hpp and Boost.Polygon (its Voronoi diagram
// of segments, exact on integer input; Boost is already required by Knoodle).

#include "PlaneDrawing.hpp"

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-float-conversion"      // Boost.Polygon, under C++20
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace Knoodle::Animate
{
    /*!@brief A pass move as the router needs it. Darcs are 2 a + 1 for arc a
     * run forwards and 2 a for it run backwards; a darc's face is on its left. */
    struct RouteRequest
    {
        Idx  first_arc = None, last_arc = None;     // the strand's end arcs
        bool forwardQ  = true;                      // does the strand run along its arcs' orientation?
        Idx  depart    = None;                      // a darc of the first arc: the route starts into the face on its left
        std::vector<Idx> cross;                     // darcs crossed in order, each from its left face into its right
        Idx  land      = None;                      // a darc of the last arc: the route ends in the face on its left
    };

    namespace detail
    {
        struct RoutePoly
        {
            std::vector<Point> p;       // counterclockwise; may touch itself, never crosses
            std::vector<Idx>   tag;     // the darc that edge p[i] -> p[i+1] is part of, or None
            std::vector<char>  brk;     // a point where a run of one darc's edges ends: a dot, or a crossing of the route
        };

        /*!@brief Is q on edge e of the polygon? If so, how far along. */
        inline bool OnEdge( const RoutePoly & Q, Idx e, const Point & q, Real tol, Real & t )
        {
            const Point & a = Q.p[e], & b = Q.p[(e + 1) % Q.p.size()];
            const Real dx = b[0] - a[0], dy = b[1] - a[1], d2 = dx * dx + dy * dy;
            if( d2 <= 0 ) { return false; }
            t = ((q[0] - a[0]) * dx + (q[1] - a[1]) * dy) / d2;
            if( t < -1e-12 || t > 1 + 1e-12 ) { return false; }
            return std::hypot(q[0] - a[0] - t * dx, q[1] - a[1] - t * dy) <= tol;
        }

        /*!@brief Make q, a point of an edge tagged `tag`, a vertex of the
         * polygon, and a break. Returns its index, or None if it is on no such edge. */
        inline Idx Vertex( RoutePoly & Q, Idx tag, const Point & q, Real tol )
        {
            for( Idx e = 0; e < Q.p.size(); ++e )
            {
                Real t = 0;
                if( Q.tag[e] != tag || !OnEdge(Q, e, q, tol, t) ) { continue; }
                const Idx n = Q.p.size(), f = (e + 1) % n;
                if( Animate::Dist(Q.p[e], q) <= tol ) { Q.brk[e] = 1; return e; }
                if( Animate::Dist(Q.p[f], q) <= tol ) { Q.brk[f] = 1; return f; }
                Q.p.insert(Q.p.begin() + static_cast<std::ptrdiff_t>(e) + 1, q);
                Q.tag.insert(Q.tag.begin() + static_cast<std::ptrdiff_t>(e) + 1, tag);
                Q.brk.insert(Q.brk.begin() + static_cast<std::ptrdiff_t>(e) + 1, 1);
                return e + 1;
            }
            return None;
        }

        /*!@brief The leg from vertex `from` to vertex `to` of the polygon:
         * each end goes straight in, along the bisector of its corner, to the
         * polygon's medial axis, and the leg follows the axis between them.
         * The axis is the inside part of the Voronoi diagram of the polygon's
         * edges (Boost.Polygon, exact on the snapped corners); the parabolic
         * pieces round reflex corners are sampled. Empty if there is no way. */
        inline Polyline Leg( const RoutePoly & Q, Idx from, Idx to )
        {
            namespace bp = boost::polygon;
            using IP = bp::point_data<int>; using IS = bp::segment_data<int>;
            const Idx n = Q.p.size();
            if( n < 3 ) { return {}; }
            Box B; for( const Point & p : Q.p ) { B.Add(p); }
            const Real size = std::max(B.Size(), 1e-300), scale = static_cast<Real>(1 << 29) / size;
            auto world = [&]( const Point & q ) { return Point{ B.lo[0] + q[0] / scale, B.lo[1] + q[1] / scale }; };
            std::vector<IP> ip (n);
            std::set<std::pair<int,int>> corners;
            for( Idx i = 0; i < n; ++i )
            {
                ip[i] = IP{ static_cast<int>(std::llround((Q.p[i][0] - B.lo[0]) * scale)), static_cast<int>(std::llround((Q.p[i][1] - B.lo[1]) * scale)) };
                corners.insert({ ip[i].x(), ip[i].y() });
            }
            std::vector<IS> segs;                               // each edge once: a cut is walked both ways
            std::set<std::array<int,4>> have;
            for( Idx i = 0; i < n; ++i )
            {
                IP a = ip[i], b = ip[(i + 1) % n];
                if( a == b ) { continue; }
                if( std::make_pair(b.x(), b.y()) < std::make_pair(a.x(), a.y()) ) { std::swap(a, b); }
                if( have.insert({ a.x(), a.y(), b.x(), b.y() }).second ) { segs.push_back(IS{ a, b }); }
            }
            bp::voronoi_diagram<double> vd;
            bp::construct_voronoi(segs.begin(), segs.end(), &vd);

            // the axis as a graph, in snapped coordinates
            std::vector<Point> node; std::vector<std::array<Idx,2>> edge;
            std::vector<Idx> at (vd.num_vertices(), None);
            auto vert = [&]( const bp::voronoi_diagram<double>::vertex_type * v )
            {
                Idx & i = at[static_cast<Idx>(v - &vd.vertices()[0])];
                if( i == None ) { i = node.size(); node.push_back(Point{ v->x(), v->y() }); }
                return i;
            };
            for( const auto & e : vd.edges() )
            {
                if( !e.is_primary() || !e.is_finite() || &e > e.twin() ) { continue; }
                const Point v0 { e.vertex0()->x(), e.vertex0()->y() }, v1 { e.vertex1()->x(), e.vertex1()->y() };
                Polyline c { v0 };
                if( e.is_curved() )
                {
                    // a parabola: as far from a corner (the focus) as from the line of an edge
                    const auto * cp = e.cell(), * cs = e.twin()->cell();
                    if( !cp->contains_point() ) { std::swap(cp, cs); }
                    const IS & sp = segs[cp->source_index()], & ss = segs[cs->source_index()];
                    const IP f = (cp->source_category() == bp::SOURCE_CATEGORY_SEGMENT_START_POINT) ? bp::low(sp) : bp::high(sp);
                    const Point s0 { static_cast<Real>(bp::low(ss).x()), static_cast<Real>(bp::low(ss).y()) };
                    Real ux = bp::high(ss).x() - s0[0], uy = bp::high(ss).y() - s0[1];
                    const Real ul = std::hypot(ux, uy); ux /= ul; uy /= ul;
                    const Real fx = (f.x() - s0[0]) * ux + (f.y() - s0[1]) * uy, fy = -(f.x() - s0[0]) * uy + (f.y() - s0[1]) * ux;
                    const Real x0 = (v0[0] - s0[0]) * ux + (v0[1] - s0[1]) * uy, x1 = (v1[0] - s0[0]) * ux + (v1[1] - s0[1]) * uy;
                    for( int k = 1; k < 8 && fy != 0; ++k )
                    {
                        const Real x = x0 + (x1 - x0) * k / 8, y = ((x - fx) * (x - fx) + fy * fy) / (2 * fy);
                        c.push_back(Point{ s0[0] + x * ux - y * uy, s0[1] + x * uy + y * ux });
                    }
                }
                c.push_back(v1);
                const Point mid = (c.size() > 2) ? c[c.size() / 2] : Point{ (v0[0] + v1[0]) / 2, (v0[1] + v1[1]) / 2 };
                if( !Inside(Q.p, world(mid)) ) { continue; }
                Idx a = vert(e.vertex0());
                for( Idx i = 1; i + 1 < c.size(); ++i ) { node.push_back(c[i]); edge.push_back({ a, node.size() - 1 }); a = node.size() - 1; }
                edge.push_back({ a, vert(e.vertex1()) });
            }
            if( edge.empty() ) { return {}; }

            // an end goes in along its corner's bisector to the first thing of the axis it meets
            auto join = [&]( Idx v ) -> Idx
            {
                const Point o { static_cast<Real>(ip[v].x()), static_cast<Real>(ip[v].y()) };
                Idx a = (v + n - 1) % n, b = (v + 1) % n;
                while( a != v && ip[a] == ip[v] ) { a = (a + n - 1) % n; }
                while( b != v && ip[b] == ip[v] ) { b = (b + 1) % n; }
                if( a == v || b == v ) { return None; }
                Real ax = o[0] - ip[a].x(), ay = o[1] - ip[a].y(), bx = ip[b].x() - o[0], by = ip[b].y() - o[1];
                const Real al = std::hypot(ax, ay), bl = std::hypot(bx, by);
                Real dx = -(ay / al + by / bl), dy = ax / al + bx / bl;
                const Real dl = std::hypot(dx, dy);
                if( dl < 1e-9 ) { return None; }
                dx /= dl; dy /= dl;
                const Real near = 1e-4;                         // of a snapped unit
                Real best = std::numeric_limits<Real>::infinity(); Idx best_node = None, best_edge = None;
                for( Idx i = 0; i < node.size(); ++i )
                {
                    const Real px = node[i][0] - o[0], py = node[i][1] - o[1], t = px * dx + py * dy;
                    if( t >= -near && std::abs(px * dy - py * dx) <= near && t < best - near ) { best = std::max(t, Real(0)); best_node = i; best_edge = None; }
                }
                for( Idx k = 0; k < edge.size(); ++k )
                {
                    const Point & p = node[edge[k][0]], & q = node[edge[k][1]];
                    const Real ex = q[0] - p[0], ey = q[1] - p[1], den = dx * ey - dy * ex;
                    if( std::abs(den) <= 1e-12 * std::hypot(ex, ey) ) { continue; }
                    const Real t = ((p[0] - o[0]) * ey - (p[1] - o[1]) * ex) / den, u = ((p[0] - o[0]) * dy - (p[1] - o[1]) * dx) / den;
                    if( u < 0 || u > 1 || t < 0 || t >= best - near ) { continue; }
                    best = t; best_edge = k; best_node = None;
                }
                if( best_node != None ) { return best_node; }
                if( best_edge == None ) { return None; }
                const std::array<Idx,2> cut = edge[best_edge];
                node.push_back(Point{ o[0] + best * dx, o[1] + best * dy });
                edge[best_edge] = { cut[0], node.size() - 1 }; edge.push_back({ node.size() - 1, cut[1] });
                return node.size() - 1;
            };
            const Idx hf = join(from), ht = join(to);
            if( hf == None || ht == None ) { return {}; }

            // the shortest way along the axis, never through a corner of the polygon
            std::vector<std::vector<Idx>> nb (node.size());
            for( const auto & e : edge ) { nb[e[0]].push_back(e[1]); nb[e[1]].push_back(e[0]); }
            auto cornerQ = [&]( Idx i )
            {
                const std::pair<int,int> r { static_cast<int>(std::llround(node[i][0])), static_cast<int>(std::llround(node[i][1])) };
                return corners.count(r) && std::hypot(node[i][0] - r.first, node[i][1] - r.second) < 1e-3;
            };
            std::vector<Real> dist (node.size(), std::numeric_limits<Real>::infinity());
            std::vector<Idx> prev (node.size(), None);
            std::priority_queue<std::pair<Real,Idx>, std::vector<std::pair<Real,Idx>>, std::greater<>> heap;
            dist[hf] = 0; heap.push({ 0, hf });
            while( !heap.empty() )
            {
                const auto [d, i] = heap.top(); heap.pop();
                if( d > dist[i] ) { continue; }
                if( i == ht ) { break; }
                if( i != hf && cornerQ(i) ) { continue; }
                for( Idx j : nb[i] ) { const Real w = d + Animate::Dist(node[i], node[j]); if( w < dist[j] ) { dist[j] = w; prev[j] = i; heap.push({ w, j }); } }
            }
            if( !std::isfinite(dist[ht]) ) { return {}; }
            Polyline back { Q.p[to] };
            for( Idx i = ht; i != None; i = prev[i] ) { back.push_back(world(node[i])); }
            back.push_back(Q.p[from]);
            std::reverse(back.begin(), back.end());
            Polyline leg { back.front() };                      // no piece too short to see
            for( Idx i = 1; i + 1 < back.size(); ++i ) { if( Animate::Dist(back[i], leg.back()) > 1e-7 * size && Animate::Dist(back[i], back.back()) > 1e-7 * size ) { leg.push_back(back[i]); } }
            leg.push_back(back.back());
            return leg;
        }

        /*!@brief Cut the polygon in two along a leg from its vertex `from` to
         * its vertex `to`. */
        inline void Split( const RoutePoly & Q, Idx from, Idx to, const Polyline & leg, RoutePoly & A, RoutePoly & B )
        {
            const Idx n = Q.p.size();
            A = RoutePoly{}; B = RoutePoly{};
            // A: the boundary from `from` round to `to`, then the leg back
            for( Idx i = from; i != to; i = (i + 1) % n ) { A.p.push_back(Q.p[i]); A.tag.push_back(Q.tag[i]); A.brk.push_back(Q.brk[i]); }
            A.p.push_back(Q.p[to]); A.tag.push_back(None); A.brk.push_back(1);
            for( Idx i = leg.size() - 1; i-- > 1; ) { A.p.push_back(leg[i]); A.tag.push_back(None); A.brk.push_back(1); }
            A.brk[0] = 1;
            // B: the boundary from `to` round to `from`, then the leg forward
            for( Idx i = to; i != from; i = (i + 1) % n ) { B.p.push_back(Q.p[i]); B.tag.push_back(Q.tag[i]); B.brk.push_back(Q.brk[i]); }
            B.p.push_back(Q.p[from]); B.tag.push_back(None); B.brk.push_back(1);
            for( Idx i = 1; i + 1 < leg.size(); ++i ) { B.p.push_back(leg[i]); B.tag.push_back(None); B.brk.push_back(1); }
            B.brk[0] = 1;
        }

        /*!@brief The runs of the polygon's edges on darc `tag`: maximal
         * chains with no break inside. Each is given by a point near half way
         * along it. */
        inline std::vector<Point> Runs( const RoutePoly & Q, Idx tag )
        {
            std::vector<Point> out;
            const Idx n = Q.p.size();
            std::vector<char> used (n, 0);
            for( Idx e0 = 0; e0 < n; ++e0 )
            {
                if( Q.tag[e0] != tag || used[e0] ) { continue; }
                Idx s = e0;                                     // back to where the run starts
                for( Idx guard = 0; guard < n; ++guard ) { const Idx b = (s + n - 1) % n; if( Q.tag[b] != tag || Q.brk[s] || b == e0 ) { break; } s = b; }
                Polyline L { Q.p[s] };
                for( Idx e = s; ; e = (e + 1) % n )
                {
                    used[e] = 1; L.push_back(Q.p[(e + 1) % n]);
                    const Idx f = (e + 1) % n;
                    if( Q.tag[f] != tag || Q.brk[f] || used[f] ) { break; }
                }
                // the middle of the edge that holds the run's half-way point: never a corner of the arc
                Real total = 0, upto = 0;
                for( Idx i = 0; i + 1 < L.size(); ++i ) { total += Animate::Dist(L[i], L[i+1]); }
                Idx mid = 0;
                for( Idx i = 0; i + 1 < L.size(); ++i ) { const Real d = Animate::Dist(L[i], L[i+1]); if( upto + d >= total / 2 && d > 0 ) { mid = i; break; } upto += d; }
                out.push_back(Point{ (L[mid][0] + L[mid+1][0]) / 2, (L[mid][1] + L[mid+1][1]) / 2 });
            }
            return out;
        }
    } // namespace detail

    /*!@brief Offer routes for the move `rq` in the drawing D to `accept`,
     * one after another, until it takes one. A route runs from the tail dot,
     * on the first arc, to the head dot, on the last, and passes through a
     * point of each arc it crosses. Where the move leaves a choice (which way
     * round the outside; which part of an arc already met), the choices are
     * tried in turn; `accept` should check that the route makes the move it
     * is meant to. At most `budget` routes are offered. */
    template<class Accept_T>
    bool SkeletonRoutes( const PlaneDrawing & D, const RouteRequest & rq, Accept_T && accept, std::string & why, int budget = 48 )
    {
        using detail::RoutePoly;
        std::vector<std::vector<Idx>> faces;
        if( !D.Faces(faces, why) ) { return false; }
        const auto B = D.Bounds();
        const Real size = std::max(B.Size(), 1e-300), tol = 1e-9 * size;

        // the dots, to begin with: half way along their arcs, or a third and two thirds when the strand is one arc
        const bool oneQ = (rq.first_arc == rq.last_arc);
        auto dot = [&]( Idx a, Real f ) { const Polyline & L = D.arcs[a].pts; return At(L, Params(L), rq.forwardQ ? f : 1 - f); };
        const Point tail0 = dot(rq.first_arc, oneQ ? 1.0 / 3 : 0.5), head0 = dot(rq.last_arc, oneQ ? 2.0 / 3 : 0.5);

        // a step: the middle length of the drawing's pieces (the grid step, for a layout)
        std::vector<Real> lens;
        for( const auto & A : D.arcs ) { for( Idx i = 0; i + 1 < A.pts.size(); ++i ) { lens.push_back(Animate::Dist(A.pts[i], A.pts[i+1])); } }
        std::nth_element(lens.begin(), lens.begin() + static_cast<std::ptrdiff_t>(lens.size() / 2), lens.end());
        const Real step = lens.empty() ? 0.05 * size : std::max(lens[lens.size() / 2], 0.01 * size);

        bool anyQ = false;
        for( int way = 0; way < 4 && budget > 0; ++way )            // which way the outside is cut open
        {
            std::vector<RoutePoly> polys;
            for( const auto & f : faces )
            {
                RoutePoly Q;
                for( Idx d : f )
                {
                    const Polyline & L = D.arcs[d / 2].pts;
                    for( Idx i = 0; i + 1 < L.size(); ++i ) { Q.p.push_back((d % 2) ? L[i] : L[L.size() - 1 - i]); Q.tag.push_back(d); Q.brk.push_back(0); }
                }
                if( std::find(f.begin(), f.end(), D.ExteriorDarc()) != f.end() )
                {
                    // The outside: a frame round the drawing, joined to the drawing's outline by a cut
                    // from the outline's farthest point one way. Round the frame, in, round the outline, out.
                    // The frame is the outline's convex hull pushed out by a step, so a route round the
                    // outside runs like a band round the drawing, half a step off it.
                    const Point dir[4] { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
                    Idx v = 0;
                    for( Idx i = 1; i < Q.p.size(); ++i ) { if( Q.p[i][0] * dir[way][0] + Q.p[i][1] * dir[way][1] > Q.p[v][0] * dir[way][0] + Q.p[v][1] * dir[way][1] ) { v = i; } }
                    Polyline S = Q.p, H;
                    std::sort(S.begin(), S.end());
                    for( int pass = 0; pass < 2; ++pass )
                    {
                        const Idx base = H.size();
                        for( const Point & q : S )
                        {
                            while( H.size() >= base + 2 && Side(H[H.size()-2], H.back(), q) <= 0 ) { H.pop_back(); }
                            H.push_back(q);
                        }
                        H.pop_back(); std::reverse(S.begin(), S.end());
                    }
                    Polyline F;                                 // each corner of the hull cut off square
                    for( Idx i = 0; i < H.size(); ++i )
                    {
                        const Point & a = H[(i + H.size() - 1) % H.size()], & b = H[i], & c = H[(i + 1) % H.size()];
                        const Real l0 = Animate::Dist(a, b), l1 = Animate::Dist(b, c);
                        F.push_back(Point{ b[0] + step * (b[1] - a[1]) / l0, b[1] - step * (b[0] - a[0]) / l0 });
                        F.push_back(Point{ b[0] + step * (c[1] - b[1]) / l1, b[1] - step * (c[0] - b[0]) / l1 });
                    }
                    Real best = std::numeric_limits<Real>::infinity(); Idx hit = None;
                    for( Idx i = 0; i < F.size(); ++i )
                    {
                        const Point & p = F[i], & q = F[(i + 1) % F.size()];
                        const Real ex = q[0] - p[0], ey = q[1] - p[1], den = dir[way][0] * ey - dir[way][1] * ex;
                        if( std::abs(den) <= 1e-12 * std::hypot(ex, ey) ) { continue; }
                        const Real t = ((p[0] - Q.p[v][0]) * ey - (p[1] - Q.p[v][1]) * ex) / den, u = ((p[0] - Q.p[v][0]) * dir[way][1] - (p[1] - Q.p[v][1]) * dir[way][0]) / den;
                        if( u >= 0 && u <= 1 && t > 0 && t < best ) { best = t; hit = i; }
                    }
                    if( hit == None ) { continue; }
                    const Point on { Q.p[v][0] + best * dir[way][0], Q.p[v][1] + best * dir[way][1] };
                    RoutePoly K;
                    auto add = [&]( const Point & q, Idx tag ) { K.p.push_back(q); K.tag.push_back(tag); K.brk.push_back(1); };
                    add(on, None);
                    for( Idx i = 1; i <= F.size(); ++i ) { add(F[(hit + i) % F.size()], None); }
                    add(on, None);
                    for( Idx i = 0; i < Q.p.size(); ++i ) { const Idx j = (v + i) % Q.p.size(); add(Q.p[j], Q.tag[j]); K.brk.back() = 0; }
                    add(Q.p[v], None);
                    Q = std::move(K);
                }
                polys.push_back(std::move(Q));
            }
            // The dots slide along their arcs to where the route is shortest: the tail to suit the
            // route's first leg, then the head to suit its last. Each keeps half a step from its arc's
            // ends (a quarter of the arc, if that is less), and a strand of one arc keeps them apart.
            Point tail = tail0, head = head0;
            {
                auto shortest = [&]( const std::vector<RoutePoly> & PP, Idx tag_a, const Point & a, bool runsQ, Idx tag_b, const Point & b )
                {
                    Real best = std::numeric_limits<Real>::infinity();
                    for( const RoutePoly & Q0 : PP )
                    {
                        RoutePoly Q = Q0;
                        if( detail::Vertex(Q, tag_a, a, tol) == None ) { continue; }
                        const std::vector<Point> targets = runsQ ? detail::Runs(Q, tag_b) : std::vector<Point>{ b };
                        for( const Point & q : targets )
                        {
                            RoutePoly W = Q;
                            const Idx to = detail::Vertex(W, tag_b, q, tol), from = detail::Vertex(W, tag_a, a, tol);
                            if( to == None || from == None || from == to ) { continue; }
                            const Polyline leg = detail::Leg(W, from, to);
                            if( leg.size() >= 2 ) { best = std::min(best, Length(leg)); }
                        }
                    }
                    return best;
                };
                // places on arc a between the fractions lo and hi of it, the nearest to the middle first
                auto places = [&]( Idx a, Real lo, Real hi )
                {
                    const Real len = Length(D.arcs[a].pts), m = std::min(step / 2, (hi - lo) * len / 4);
                    std::vector<Real> f;
                    for( Real s = lo * len + m; s <= hi * len - m + tol && len > 0; s += step ) { f.push_back(s / len); }
                    if( len > 0 ) { f.push_back((hi * len - m) / len); }
                    const Real mid = (lo + hi) / 2;
                    std::sort(f.begin(), f.end(), [&]( Real x, Real y ) { return std::abs(x - mid) < std::abs(y - mid); });
                    return f;
                };
                const bool legQ = !rq.cross.empty();
                auto with = [&]( const Point & t, const Point & h )
                {
                    std::vector<RoutePoly> PP = polys;
                    for( RoutePoly & Q : PP ) { for( Idx side = 0; side < 2; ++side ) { detail::Vertex(Q, 2 * rq.first_arc + side, t, tol); detail::Vertex(Q, 2 * rq.last_arc + side, h, tol); } }
                    return PP;
                };
                Real best = std::numeric_limits<Real>::infinity();
                {
                    const std::vector<RoutePoly> PP = with(head0, head0);
                    for( Real f : places(rq.first_arc, 0, oneQ ? 0.45 : 1) )
                    {
                        const Point t = dot(rq.first_arc, f);
                        const Real l = shortest(PP, rq.depart, t, legQ, legQ ? rq.cross.front() : rq.land, head0);
                        if( l < best - 1e-6 * size ) { best = l; tail = t; }
                    }
                }
                best = std::numeric_limits<Real>::infinity();
                {
                    const std::vector<RoutePoly> PP = with(tail, tail);
                    for( Real f : places(rq.last_arc, oneQ ? 0.55 : 0, 1) )
                    {
                        const Point h = dot(rq.last_arc, f);
                        const Real l = shortest(PP, rq.land, h, legQ, legQ ? (rq.cross.back() ^ Idx(1)) : rq.depart, tail);
                        if( l < best - 1e-6 * size ) { best = l; head = h; }
                    }
                }
            }
            // the dots are vertices, and breaks, wherever their arcs are
            for( RoutePoly & Q : polys )
            {
                for( int side = 0; side < 2; ++side )
                {
                    detail::Vertex(Q, 2 * rq.first_arc + static_cast<Idx>(side), tail, tol);
                    detail::Vertex(Q, 2 * rq.last_arc + static_cast<Idx>(side), head, tol);
                }
            }

            // lay the legs one after another, trying each run the next arc offers
            struct Go
            {
                const RouteRequest & rq; Accept_T & accept; int & budget; bool & anyQ; Real tol; Point head;
                bool Leg( std::vector<RoutePoly> polys, Polyline route, Idx start_tag, Idx i )
                {
                    if( budget <= 0 ) { return false; }
                    const Point at = route.back();
                    const bool lastQ = (i == rq.cross.size());
                    const Idx want = lastQ ? rq.land : rq.cross[i];
                    for( Idx pc = 0; pc < polys.size(); ++pc )
                    {
                        RoutePoly Q = polys[pc];
                        const Idx from0 = detail::Vertex(Q, start_tag, at, tol);
                        if( from0 == None ) { continue; }
                        std::vector<Point> targets = lastQ ? std::vector<Point>{ head } : detail::Runs(Q, want);
                        std::sort(targets.begin(), targets.end(), [&]( const Point & a, const Point & b ) { return Animate::Dist(a, at) < Animate::Dist(b, at); });
                        for( const Point & q : targets )
                        {
                            RoutePoly W = Q;
                            const Idx to = detail::Vertex(W, want, q, tol);
                            if( to == None ) { continue; }
                            Idx from = None;
                            for( Idx e = 0; e < W.p.size(); ++e ) { if( Animate::Dist(W.p[e], at) <= tol && (W.tag[e] == start_tag || W.tag[(e + W.p.size() - 1) % W.p.size()] == start_tag) ) { from = e; break; } }
                            if( from == None || from == to ) { continue; }
                            const Polyline leg = detail::Leg(W, from, to);
                            if( leg.size() < 2 ) { continue; }
                            Polyline r = route;
                            r.insert(r.end(), leg.begin() + 1, leg.end());
                            if( lastQ )
                            {
                                --budget; anyQ = true;
                                if( accept(r) ) { return true; }
                                continue;
                            }
                            std::vector<RoutePoly> next = polys;
                            RoutePoly A, B;
                            detail::Split(W, from, to, leg, A, B);
                            next[pc] = std::move(A); next.push_back(std::move(B));
                            if( Leg(std::move(next), std::move(r), want ^ Idx(1), i + 1) ) { return true; }
                        }
                    }
                    return false;
                }
            };
            Go go { rq, accept, budget, anyQ, tol, head };
            if( go.Leg(polys, Polyline{ tail }, rq.depart, 0) ) { return true; }
        }
        why = anyQ ? "no route through the faces makes the move" : "no route could be laid through the faces the move names";
        return false;
    }

} // namespace Knoodle::Animate
