#pragma once

// Routes for pass moves, through the faces of the drawing itself
// (docs/animate-design.md). A pass move names the faces its strand's new
// route runs through by the arcs it crosses on the way. Here each face is a
// polygon; a leg of the route enters at a point of the polygon's boundary and
// leaves at another, and in between it follows the skeleton of the polygon:
// the midpoints of the diagonals of a Delaunay triangulation, along the one
// chain of triangles from the way in to the way out. A leg, once laid, cuts
// its polygon in two, so later legs cannot cross it.
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

        // the dots: half way along their arcs, or a third and two thirds when the strand is one arc
        const bool oneQ = (rq.first_arc == rq.last_arc);
        auto dot = [&]( Idx a, Real f ) { const Polyline & L = D.arcs[a].pts; return At(L, Params(L), rq.forwardQ ? f : 1 - f); };
        const Point tail = dot(rq.first_arc, oneQ ? 1.0 / 3 : 0.5), head = dot(rq.last_arc, oneQ ? 2.0 / 3 : 0.5);

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
                    const Real m = 0.2 * size + 4 * tol;
                    const Point c[4] { { B.lo[0] - m, B.lo[1] - m }, { B.hi[0] + m, B.lo[1] - m }, { B.hi[0] + m, B.hi[1] + m }, { B.lo[0] - m, B.hi[1] + m } };
                    const Point dir[4] { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };      // side i runs from corner i to corner i + 1
                    Idx v = 0;
                    for( Idx i = 1; i < Q.p.size(); ++i ) { if( Q.p[i][0] * dir[way][0] + Q.p[i][1] * dir[way][1] > Q.p[v][0] * dir[way][0] + Q.p[v][1] * dir[way][1] ) { v = i; } }
                    const Point on = (way % 2 == 0) ? Point{ Q.p[v][0], c[way][1] } : Point{ c[way][0], Q.p[v][1] };
                    RoutePoly K;
                    auto add = [&]( const Point & q, Idx tag ) { K.p.push_back(q); K.tag.push_back(tag); K.brk.push_back(1); };
                    add(on, None);
                    for( int i = 1; i <= 4; ++i ) { add(c[(way + i) % 4], None); }
                    add(on, None);
                    for( Idx i = 0; i < Q.p.size(); ++i ) { const Idx j = (v + i) % Q.p.size(); add(Q.p[j], Q.tag[j]); K.brk.back() = 0; }
                    add(Q.p[v], None);
                    Q = std::move(K);
                }
                polys.push_back(std::move(Q));
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
