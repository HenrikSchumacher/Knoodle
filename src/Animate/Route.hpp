#pragma once

// Routes for pass moves, through the faces of the drawing itself
// (docs/animate-design.md). A pass move names the faces its strand's new
// route runs through by the arcs it crosses on the way. Here each face is a
// polygon; a leg of the route enters at a point of the polygon's boundary and
// leaves at another. Which way it goes is read off a triangulation of the
// polygon (the chain of triangles from the way in to the way out); how it
// goes is a band pulled taut, held a little off the polygon's sides. A leg,
// once laid, cuts its polygon in two, so later legs cannot cross it.
//
// Nothing here needs a lattice: any drawing will do, a layout or not.
//
// Standard library, PlaneDrawing.hpp and Mesh.hpp only.

#include "PlaneDrawing.hpp"
#include "Mesh.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
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
         * the two vertices, and between them the midpoints of the diagonals
         * the chain of triangles from one to the other crosses. Empty if the
         * polygon cannot be triangulated. */
        inline Polyline Leg( const RoutePoly & Q, Idx from, Idx to )
        {
            std::vector<Idx> V (Q.p.size());
            for( Idx i = 0; i < V.size(); ++i ) { V[i] = i; }
            std::vector<Point> P = Q.p;
            std::vector<Tri> tris;
            const Drawings one { &P };
            EarClip(V, one, tris);
            if( !V.empty() || tris.empty() ) { return {}; }
            std::string why;
            if( !Refine(P, tris, std::numeric_limits<Real>::infinity(), why) ) { return {}; }      // flips only: no edge is too long
            // triangles across each interior edge
            std::unordered_map<std::uint64_t,std::array<Idx,2>> across;
            for( Idx t = 0; t < tris.size(); ++t )
            {
                for( int k = 0; k < 3; ++k )
                {
                    auto it = across.emplace(EdgeKey(tris[t][k], tris[t][(k + 1) % 3]), std::array<Idx,2>{ None, None }).first;
                    (it->second[0] == None ? it->second[0] : it->second[1]) = t;
                }
            }
            auto has = [&]( Idx t, Idx v ) { return tris[t][0] == v || tris[t][1] == v || tris[t][2] == v; };
            std::vector<Idx> prev (tris.size(), None), via_a (tris.size(), None), via_b (tris.size(), None);
            std::vector<char> seen (tris.size(), 0);
            std::deque<Idx> queue;
            for( Idx t = 0; t < tris.size(); ++t ) { if( has(t, from) ) { seen[t] = 1; queue.push_back(t); } }
            Idx goal = None;
            while( !queue.empty() && goal == None )
            {
                const Idx t = queue.front(); queue.pop_front();
                if( has(t, to) ) { goal = t; break; }
                for( int k = 0; k < 3; ++k )
                {
                    const Idx a = tris[t][k], b = tris[t][(k + 1) % 3];
                    const auto & pr = across[EdgeKey(a, b)];
                    const Idx u = (pr[0] == t) ? pr[1] : pr[0];
                    if( u == None || seen[u] ) { continue; }
                    seen[u] = 1; prev[u] = t; via_a[u] = a; via_b[u] = b;
                    queue.push_back(u);
                }
            }
            if( goal == None ) { return {}; }
            Polyline back { Q.p[to] };
            for( Idx t = goal; prev[t] != None; t = prev[t] ) { back.push_back(Point{ (P[via_a[t]][0] + P[via_b[t]][0]) / 2, (P[via_a[t]][1] + P[via_b[t]][1]) / 2 }); }
            back.push_back(Q.p[from]);
            std::reverse(back.begin(), back.end());
            return back;
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
         * chains with no break inside. */
        inline std::vector<Polyline> Runs( const RoutePoly & Q, Idx tag )
        {
            std::vector<Polyline> out;
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
                out.push_back(std::move(L));
            }
            return out;
        }

        /*!@brief Places to cross the run L: the middle of the edge that holds
         * its half-way point first, then points a step apart along it, half a
         * step in from its ends. Never a corner of the run. */
        inline std::vector<Point> Places( const Polyline & L, Real step )
        {
            std::vector<Real> upto { 0 };
            for( Idx i = 0; i + 1 < L.size(); ++i ) { upto.push_back(upto.back() + Animate::Dist(L[i], L[i+1])); }
            const Real total = upto.back();
            std::vector<Point> out;
            auto at = [&]( Real s )
            {
                for( Idx i = 0; i + 1 < L.size(); ++i )
                {
                    const Real d = upto[i+1] - upto[i];
                    if( d <= 0 || s > upto[i+1] ) { continue; }
                    Real t = (s - upto[i]) / d;
                    if( t * d < 1e-6 * total || (1 - t) * d < 1e-6 * total ) { t = 0.5; }       // off the corner
                    out.push_back(Point{ L[i][0] + (L[i+1][0] - L[i][0]) * t, L[i][1] + (L[i+1][1] - L[i][1]) * t });
                    return;
                }
            };
            if( total <= 0 ) { return out; }
            at(total / 2);
            const Real m = std::min(step / 2, total / 4);
            for( Real s = m; s <= total - m + 1e-9 * total; s += step ) { if( std::abs(s - total / 2) > step / 4 ) { at(s); } }
            return out;
        }

        /*!@brief The leg laid as a hand would draw it. `leg` says only which
         * way to go (any path inside the polygon does: the polygon is a disk).
         * Each end gets a stem straight in from its corner, of length `keep`
         * or less; between the stems the leg is first pushed off the polygon's
         * sides, to `keep` where there is room, and then pulled taut like a
         * band that will not come nearer a side than `keep`, or than it
         * already is where the way is narrower. */
        inline Polyline Tighten( const RoutePoly & Q, Idx from, Idx to, const Polyline & leg, Real keep, Real grain )
        {
            const Idx m = Q.p.size();
            if( leg.size() < 2 || m < 3 ) { return leg; }
            auto ps = []( const Point & p, const Point & a, const Point & b, Point * at = nullptr )
            {
                const Real dx = b[0] - a[0], dy = b[1] - a[1], d2 = dx * dx + dy * dy;
                const Real t = (d2 > 0) ? std::clamp(((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / d2, 0.0, 1.0) : 0.0;
                if( at ) { *at = Point{ a[0] + t * dx, a[1] + t * dy }; }
                return std::hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy);
            };
            // how far the piece ab is from the polygon's sides, as far as `keep` matters; 0 if it crosses one
            auto clear = [&]( const Point & a, const Point & b )
            {
                Real best = keep;
                const Real lox = std::min(a[0], b[0]) - keep, hix = std::max(a[0], b[0]) + keep, loy = std::min(a[1], b[1]) - keep, hiy = std::max(a[1], b[1]) + keep;
                for( Idx e = 0; e < m && best > 0; ++e )
                {
                    const Point & c = Q.p[e], & d = Q.p[(e + 1) % m];
                    if( std::max(c[0], d[0]) < lox || std::min(c[0], d[0]) > hix || std::max(c[1], d[1]) < loy || std::min(c[1], d[1]) > hiy ) { continue; }
                    if( Side(a, b, c) * Side(a, b, d) < 0 && Side(c, d, a) * Side(c, d, b) < 0 ) { return Real(0); }
                    best = std::min({ best, ps(a, c, d), ps(b, c, d), ps(c, a, b), ps(d, a, b) });
                }
                return best;
            };
            // the top of the stem at corner v: straight in along the corner's bisector, as far as there is room
            auto stem = [&]( Idx v, Point & top )
            {
                Idx a = (v + m - 1) % m, b = (v + 1) % m;
                while( a != v && Animate::Dist(Q.p[a], Q.p[v]) <= 0 ) { a = (a + m - 1) % m; }
                while( b != v && Animate::Dist(Q.p[b], Q.p[v]) <= 0 ) { b = (b + 1) % m; }
                if( a == v || b == v ) { return false; }
                const Real ax = Q.p[v][0] - Q.p[a][0], ay = Q.p[v][1] - Q.p[a][1], bx = Q.p[b][0] - Q.p[v][0], by = Q.p[b][1] - Q.p[v][1];
                const Real al = std::hypot(ax, ay), bl = std::hypot(bx, by);
                Real dx = -(ay / al + by / bl), dy = ax / al + bx / bl;
                const Real dl = std::hypot(dx, dy);
                if( dl < 1e-9 ) { return false; }
                for( Real h = keep; h > keep / 20; h /= 2 )
                {
                    top = Point{ Q.p[v][0] + h * dx / dl, Q.p[v][1] + h * dy / dl };
                    Real room = h;
                    for( Idx e = 0; e < m && room >= h / 2; ++e ) { room = std::min(room, ps(top, Q.p[e], Q.p[(e + 1) % m])); }
                    if( room >= h / 2 && Inside(Q.p, top) ) { return true; }
                }
                return false;
            };
            // the stems, joined to the first point of the leg each can see
            Polyline L = leg;
            Point top { 0, 0 };
            bool s0 = false, s1 = false;
            if( stem(from, top) )
            {
                for( Idx k = 1; k + 1 < L.size() && !s0; ++k )
                {
                    if( clear(top, L[k]) <= 0 ) { continue; }
                    L.erase(L.begin() + 1, L.begin() + static_cast<std::ptrdiff_t>(k)); L.insert(L.begin() + 1, top); s0 = true;
                }
            }
            if( stem(to, top) )
            {
                for( Idx k = L.size() - 1; k-- > 1 && !s1; )
                {
                    if( clear(top, L[k]) <= 0 ) { continue; }
                    L.erase(L.begin() + static_cast<std::ptrdiff_t>(k) + 1, L.end() - 1); L.insert(L.end() - 1, top); s1 = true;
                }
            }
            if( L.size() < 4 ) { return L; }
            // the leg's own corners, and points a grain apart between them
            Polyline P { L.front() };
            std::vector<char> fixed { 1 };
            for( Idx i = 0; i + 1 < L.size(); ++i )
            {
                const bool stemQ = (i == 0 && s0) || (i + 2 == L.size() && s1);
                const Idx k = stemQ ? 1 : std::max<Idx>(static_cast<Idx>(std::ceil(Animate::Dist(L[i], L[i+1]) / grain)), 1);
                for( Idx j = 1; j <= k; ++j ) { const Real t = static_cast<Real>(j) / static_cast<Real>(k); P.push_back(Point{ L[i][0] + (L[i+1][0] - L[i][0]) * t, L[i][1] + (L[i+1][1] - L[i][1]) * t }); fixed.push_back(0); }
            }
            const Idx n = P.size();
            fixed[n-1] = 1;
            if( s0 ) { fixed[1] = 1; }
            if( s1 ) { fixed[n-2] = 1; }
            std::vector<Real> room (n, 0);                      // room[i]: of the piece from P[i] to P[i+1]
            for( Idx i = 0; i + 1 < n; ++i ) { room[i] = clear(P[i], P[i+1]); }
            Real total = 0;
            for( Idx i = 0; i + 1 < n; ++i ) { total += Animate::Dist(P[i], P[i+1]); }
            auto move = [&]( Idx i, const Point & c, bool mustGainQ )
            {
                const Real had = std::min(room[i-1], room[i]);
                const Real r0 = clear(P[i-1], c), r1 = (r0 > 0) ? clear(c, P[i+1]) : Real(0), got = std::min(r0, r1);
                if( got <= 0 || got < had - 1e-12 * total || (mustGainQ && got <= had) ) { return false; }
                P[i] = c; room[i-1] = r0; room[i] = r1;
                return true;
            };
            // off the sides
            for( int sweep = 0; sweep < 60; ++sweep )
            {
                bool anyQ = false;
                for( Idx i = 1; i + 1 < n; ++i )
                {
                    if( fixed[i] || std::min(room[i-1], room[i]) >= keep ) { continue; }
                    Real d = keep; Point at { 0, 0 }, q { 0, 0 };
                    for( Idx e = 0; e < m; ++e ) { const Real de = ps(P[i], Q.p[e], Q.p[(e + 1) % m], &q); if( de < d ) { d = de; at = q; } }
                    if( d >= keep || d <= 0 ) { continue; }
                    const Real go = std::min(keep - d, grain / 2);
                    anyQ = move(i, Point{ P[i][0] + go * (P[i][0] - at[0]) / d, P[i][1] + go * (P[i][1] - at[1]) / d }, false) || anyQ;
                }
                if( !anyQ ) { break; }
            }
            // taut
            for( int sweep = 0; sweep < 200; ++sweep )
            {
                Real moved = 0;
                for( Idx i = 1; i + 1 < n; ++i )
                {
                    if( fixed[i] ) { continue; }
                    for( Real f : { 1.0, 0.5, 0.25 } )
                    {
                        const Point was = P[i], c { was[0] + f * ((P[i-1][0] + P[i+1][0]) / 2 - was[0]), was[1] + f * ((P[i-1][1] + P[i+1][1]) / 2 - was[1]) };
                        if( move(i, c, false) ) { moved = std::max(moved, Animate::Dist(was, c)); break; }
                    }
                }
                if( moved < 1e-3 * grain ) { break; }
            }
            Polyline out { P[0] };                              // drop what lies on a straight piece
            for( Idx i = 1; i + 1 < n; ++i ) { if( fixed[i] || ps(P[i], out.back(), P[i+1]) > 1e-3 * grain ) { out.push_back(P[i]); } }
            out.push_back(P[n-1]);
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
                    for( Idx i = 1; i <= F.size(); ++i ) { const Point & q = F[(hit + i) % F.size()]; if( Animate::Dist(q, on) > tol ) { add(q, None); } }     // the cut may land on a corner of the frame
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
                        std::vector<Point> targets { b };
                        if( runsQ ) { targets.clear(); for( const Polyline & L : detail::Runs(Q, tag_b) ) { const auto pl = detail::Places(L, step); if( !pl.empty() ) { targets.push_back(pl[0]); } } }
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
                const RouteRequest & rq; Accept_T & accept; int & budget; bool & anyQ; Real tol; Point head; Real step; std::vector<Point> aim;
                // the leg from `at` to q, pulled taut; W is the polygon with both ends in it
                Polyline Lay( const RoutePoly & Q, Idx start_tag, const Point & at, Idx want, const Point & q, bool tautQ, RoutePoly & W, Idx & from, Idx & to )
                {
                    W = Q;
                    to = detail::Vertex(W, want, q, tol); from = None;
                    if( to == None ) { return {}; }
                    for( Idx e = 0; e < W.p.size(); ++e ) { if( Animate::Dist(W.p[e], at) <= tol && (W.tag[e] == start_tag || W.tag[(e + W.p.size() - 1) % W.p.size()] == start_tag) ) { from = e; break; } }
                    if( from == None || from == to ) { return {}; }
                    Polyline leg = detail::Leg(W, from, to);
                    return (tautQ && leg.size() >= 2) ? detail::Tighten(W, from, to, leg, 0.45 * step, step / 4) : leg;
                }
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
                        // Where to cross the next arc: on each stretch of it this polygon has, the place that
                        // makes this leg and the straight line on to the arc after shortest.
                        std::vector<std::pair<Real,Point>> targets;
                        if( lastQ ) { targets.push_back({ 0, head }); }
                        else
                        {
                            for( const Polyline & L : detail::Runs(Q, want) )
                            {
                                std::vector<Point> pl = detail::Places(L, step);
                                const Idx stride = (pl.size() + 8) / 9;
                                Real best = std::numeric_limits<Real>::infinity(); Point q { 0, 0 };
                                for( Idx k = 0; k < pl.size(); k += std::max<Idx>(stride, 1) )
                                {
                                    RoutePoly W; Idx from = None, to = None;
                                    const Polyline leg = Lay(Q, start_tag, at, want, pl[k], false, W, from, to);
                                    if( leg.size() < 2 ) { continue; }
                                    const Real score = Length(leg) + Animate::Dist(pl[k], aim[i]);
                                    if( score < best - 1e-9 * step ) { best = score; q = pl[k]; }
                                }
                                if( std::isfinite(best) ) { targets.push_back({ best, q }); }
                            }
                        }
                        std::sort(targets.begin(), targets.end(), []( const auto & a, const auto & b ) { return a.first < b.first; });
                        for( const auto & [score, q] : targets )
                        {
                            RoutePoly W; Idx from = None, to = None;
                            const Polyline leg = Lay(Q, start_tag, at, want, q, true, W, from, to);
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
            std::vector<Point> aim;                             // after crossing i: the middle of the next arc to cross, or the head
            for( Idx i = 0; i < rq.cross.size(); ++i ) { aim.push_back(head); if( i + 1 < rq.cross.size() ) { const Polyline & L = D.arcs[rq.cross[i+1] / 2].pts; aim.back() = At(L, Params(L), 0.5); } }
            Go go { rq, accept, budget, anyQ, tol, head, step, aim };
            if( go.Leg(polys, Polyline{ tail }, rq.depart, 0) ) { return true; }
        }
        why = anyQ ? "no route through the faces makes the move" : "no route could be laid through the faces the move names";
        return false;
    }

} // namespace Knoodle::Animate
