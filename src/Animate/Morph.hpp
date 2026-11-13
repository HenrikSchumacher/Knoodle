#pragma once

// Morph -- the animator's one morph (docs/animate-design.md): Floater-Gotsman
// between two PlaneDrawings of one connected diagram with the same exterior
// face. Given a triangulation valid in both drawings inside a fixed convex
// frame, every vertex is written as a convex combination of its neighbours
// (mean value weights) in each drawing; interpolating the weights and solving
// gives an embedding at every time. That theorem cannot fail. What is built
// here is the triangulation, by a construction with no searches that can give
// up and no absolute tolerances:
//
//   Faces.   A bounded face of a connected diagram is one closed walk, a
//            polygon that may touch itself at a crossing but not cross.
//   Frame.   Concentric regular polygons ("rings") round both drawings. The
//            outermost is the fixed frame. The innermost is turned, in the
//            second drawing, by the angle the drawing itself turns plus what
//            the cut asks for, and the rings between share that turn out, so
//            the strip between two neighbouring rings has one triangulation
//            valid in both drawings whatever the angle.
//   Cut.     The region between the drawing and the innermost ring is an
//            annulus. Each drawing's annulus is triangulated on its own (a
//            radial bridge from its farthest point makes it a polygon), and the
//            shortest way from one outer vertex to the ring through triangle
//            centroids and edge midpoints (Dijkstra) is that drawing's cut;
//            the vertex is the one whose two ways out are shortest together.
//            The cut annulus is a polygon whose two sides of the cut touch.
//   Overlay. A polygon given in two drawings: ears that are ears in both are
//            clipped first. What is left is triangulated in each drawing
//            separately and the two triangulations are overlaid as chords of
//            one convex polygon (Aronov-Seidel-Souvaine). Which chords cross,
//            and in what order, is read off the vertex numbers; each crossing
//            becomes a point placed evenly along its chord in the drawing
//            where that chord is straight. Every cell is then strictly convex
//            in both drawings.
//
// Tolerances are relative (Mesh.hpp). A failure returns false with a reason;
// there is no other morph to fall back to.
//
// No file IO, no printing, no globals. Include after Knoodle.hpp.

#include "Mesh.hpp"

#include <functional>
#include <queue>

namespace Knoodle::Animate
{
    inline constexpr Real Pi = 3.14159265358979323846;

    struct MorphProblem
    {
        std::vector<Point> P0, P1;                  // every point, in the first and in the second drawing
        std::vector<Point> Q0, Q1;                  // the same with each drawing at centre 0 and size 1: P = centre + size Q
        std::array<Point,2> centre { Point{ 0, 0 }, Point{ 0, 0 } };
        std::array<Real,2>  size   { 1, 1 };
        std::vector<Tri>   tris;                    // valid in both
        std::vector<Idx>   boundary;                // the frame, counterclockwise; the same points in Q0 and Q1
        std::vector<std::vector<Idx>> chains;       // the drawing's arcs, by label (points 0 .. n_drawing-1)
        Idx  n_drawing = 0;
        Idx  steiner   = 0;                         // points the overlays added
        Idx  cut       = 0;                         // points on the cut
        Idx  rings     = 0;
        Real turn      = 0;                         // the innermost ring's turn in the second drawing
    };

    struct MorphReport
    {
        Real worst    = std::numeric_limits<Real>::infinity();   // the least flatness of a triangle in any frame
        Real residual = 0;                                       // the largest relative residual of a solve
    };

    namespace detail
    {
        inline Real Wrap( Real x ) { return std::remainder(x, 2 * Pi); }
        inline Real Angle( const Point & from, const Point & to ) { return std::atan2(to[1] - from[1], to[0] - from[0]); }

        /*!@brief Triangles over the polygon `poly` (point ids, counterclockwise
         * and weakly simple in both drawings) valid in both. New points are
         * appended to P0 and P1 and counted in `steiner`. See "Overlay" above.
         */
        inline bool Compatible( const std::vector<Idx> & poly, std::vector<Point> & P0, std::vector<Point> & P1,
                                std::vector<Tri> & tris, Idx & steiner, std::string & why )
        {
            std::vector<Idx> V = poly;
            EarClip(V, { &P0, &P1 }, tris);
            if( V.empty() ) { return true; }
            const Idx n = V.size();
            std::vector<Point> L0 (n), L1 (n);
            for( Idx k = 0; k < n; ++k ) { L0[k] = P0[V[k]]; L1[k] = P1[V[k]]; }
            std::vector<Idx> U0 (n), U1 (n);
            for( Idx k = 0; k < n; ++k ) { U0[k] = U1[k] = k; }
            std::vector<Tri> T0, T1;
            EarClip(U0, { &L0 }, T0);
            if( !U0.empty() ) { why = "a polygon of the first drawing has no ear (it is not a valid polygon)"; return false; }
            EarClip(U1, { &L1 }, T1);
            if( !U1.empty() ) { why = "a polygon of the second drawing has no ear (it is not a valid polygon)"; return false; }
            auto validQ = []( const std::vector<Tri> & T, const std::vector<Point> & L )
            {
                for( const Tri & t : T ) { if( !(Fat(L[t[0]], L[t[1]], L[t[2]]) > EarTol) ) { return false; } }
                return true;
            };
            if( validQ(T1, L0) || validQ(T0, L1) )
            {
                for( const Tri & t : (validQ(T1, L0) ? T1 : T0) ) { tris.push_back(Tri{ V[t[0]], V[t[1]], V[t[2]] }); }
                return true;
            }

            // The chords of each triangulation (a < b), with the triangles on their two sides.
            struct Chord { Idx a, b; std::array<Idx,2> tri { None, None }; std::vector<Idx> x, node; };
            std::vector<Chord> C0, C1;
            std::unordered_map<std::uint64_t,Idx> id0, id1;
            auto chords = [n]( const std::vector<Tri> & T, std::vector<Chord> & C, std::unordered_map<std::uint64_t,Idx> & id )
            {
                for( Idx t = 0; t < T.size(); ++t )
                {
                    for( Idx k = 0; k < 3; ++k )
                    {
                        const Idx lo = std::min(T[t][k], T[t][(k + 1) % 3]), hi = std::max(T[t][k], T[t][(k + 1) % 3]);
                        if( hi - lo == 1 || (lo == 0 && hi == n - 1) ) { continue; }
                        const auto [it, newQ] = id.try_emplace(EdgeKey(lo, hi), C.size());
                        if( newQ ) { C.push_back(Chord{ lo, hi, { None, None }, {}, {} }); }
                        Chord & c = C[it->second];
                        (c.tri[0] == None ? c.tri[0] : c.tri[1]) = t;
                    }
                }
            };
            chords(T0, C0, id0);
            chords(T1, C1, id1);
            // Chords e and f cross iff their ends alternate round the polygon. Along e
            // from a to b, the chords crossing it (which do not cross one another) come
            // in the order of their end inside (a, b), then of their other end going
            // backward from a.
            auto crossings = [n]( std::vector<Chord> & E, const std::vector<Chord> & F )
            {
                for( Chord & e : E )
                {
                    std::vector<std::array<Idx,3>> hit;
                    for( Idx fi = 0; fi < F.size(); ++fi )
                    {
                        const Chord & f = F[fi];
                        const bool ainQ = (e.a < f.a && f.a < e.b), binQ = (e.a < f.b && f.b < e.b);
                        const Idx in = ainQ ? f.a : f.b, out = ainQ ? f.b : f.a;
                        if( ainQ == binQ || out == e.a || out == e.b ) { continue; }
                        hit.push_back({ in - e.a, (e.a + n - out) % n, fi });
                    }
                    std::sort(hit.begin(), hit.end());
                    for( const auto & h : hit ) { e.x.push_back(h[2]); }
                }
            };
            crossings(C0, C1);
            crossings(C1, C0);
            // One new point per crossing: node n + s here, point S[s] in P0 and P1.
            std::vector<Idx> S;
            std::unordered_map<std::uint64_t,Idx> node_of;
            for( Idx ei = 0; ei < C0.size(); ++ei )
            {
                Chord & e = C0[ei];
                for( Idx r = 0; r < e.x.size(); ++r )
                {
                    node_of[ei * C1.size() + e.x[r]] = n + S.size();
                    e.node.push_back(n + S.size());
                    S.push_back(P0.size());
                    P0.push_back(Lerp(L0[e.a], L0[e.b], static_cast<Real>(r + 1) / static_cast<Real>(e.x.size() + 1)));
                    P1.push_back(Point{ 0, 0 });
                }
            }
            for( Idx fi = 0; fi < C1.size(); ++fi )
            {
                Chord & f = C1[fi];
                for( Idx r = 0; r < f.x.size(); ++r )
                {
                    const Idx nd = node_of.at(f.x[r] * C1.size() + fi);
                    f.node.push_back(nd);
                    P1[S[nd - n]] = Lerp(L1[f.a], L1[f.b], static_cast<Real>(r + 1) / static_cast<Real>(f.x.size() + 1));
                }
            }
            steiner += S.size();
            // Each triangle of T0 starts as one cell: its corners and the new points on its sides.
            std::vector<std::vector<std::vector<Idx>>> cells (T0.size());
            for( Idx t = 0; t < T0.size(); ++t )
            {
                std::vector<Idx> cyc;
                for( Idx k = 0; k < 3; ++k )
                {
                    const Idx i = T0[t][k], j = T0[t][(k + 1) % 3];
                    cyc.push_back(i);
                    const auto it = id0.find(EdgeKey(i, j));
                    if( it == id0.end() ) { continue; }
                    const std::vector<Idx> & nd = C0[it->second].node;
                    if( i < j ) { cyc.insert(cyc.end(), nd.begin(), nd.end()); } else { cyc.insert(cyc.end(), nd.rbegin(), nd.rend()); }
                }
                cells[t].push_back(cyc);
            }
            // Each chord of T1 crosses T0's triangles one piece at a time; a piece splits a cell in two.
            auto has = [&T0]( Idx t, Idx v ) { return t != None && (T0[t][0] == v || T0[t][1] == v || T0[t][2] == v); };
            for( const Chord & f : C1 )
            {
                if( f.node.empty() ) { continue; }            // a chord of both triangulations
                std::vector<Idx> seq { f.a };
                seq.insert(seq.end(), f.node.begin(), f.node.end());
                seq.push_back(f.b);
                for( Idx k = 0; k + 1 < seq.size(); ++k )
                {
                    Idx t = None;
                    if( k == 0 ) { const Chord & e = C0[f.x.front()]; t = has(e.tri[0], f.a) ? e.tri[0] : e.tri[1]; }
                    else if( k + 2 == seq.size() ) { const Chord & e = C0[f.x.back()]; t = has(e.tri[0], f.b) ? e.tri[0] : e.tri[1]; }
                    else
                    {
                        const Chord & e = C0[f.x[k-1]]; const Chord & g = C0[f.x[k]];
                        t = (e.tri[0] == g.tri[0] || e.tri[0] == g.tri[1]) ? e.tri[0] : e.tri[1];
                    }
                    bool splitQ = false;
                    for( Idx c = 0; t != None && c < cells[t].size() && !splitQ; ++c )
                    {
                        std::vector<Idx> & cyc = cells[t][c];
                        const auto iu = std::find(cyc.begin(), cyc.end(), seq[k]), iw = std::find(cyc.begin(), cyc.end(), seq[k+1]);
                        if( iu == cyc.end() || iw == cyc.end() ) { continue; }
                        const auto lo = std::min(iu, iw), hi = std::max(iu, iw);
                        std::vector<Idx> one (lo, hi + 1), two (hi, cyc.end());
                        two.insert(two.end(), cyc.begin(), lo + 1);
                        cyc = one;
                        cells[t].push_back(two);
                        splitQ = true;
                    }
                    if( !splitQ ) { why = "the overlay lost a piece of a chord (this is a bug)"; return false; }
                }
            }
            // Fan each cell from the corner that gives the fattest thinnest triangle.
            auto g = [&]( Idx k ) { return k < n ? V[k] : S[k - n]; };
            for( const auto & list : cells )
            {
                for( const auto & cyc : list )
                {
                    const Idx m = cyc.size();
                    Idx best = 0;
                    Real best_q = -1;
                    for( Idx s = 0; s < m && m > 3; ++s )
                    {
                        Real q = 1;
                        for( Idx i = 1; i + 1 < m; ++i )
                        {
                            const Idx a = g(cyc[s]), b = g(cyc[(s + i) % m]), c = g(cyc[(s + i + 1) % m]);
                            q = std::min({ q, Fat(P0[a], P0[b], P0[c]), Fat(P1[a], P1[b], P1[c]) });
                        }
                        if( q > best_q ) { best_q = q; best = s; }
                    }
                    for( Idx i = 1; i + 1 < m; ++i ) { tris.push_back(Tri{ g(cyc[best]), g(cyc[(best + i) % m]), g(cyc[(best + i + 1) % m]) }); }
                }
            }
            return true;
        }

        /*!@brief One drawing's way out: the annulus between the exterior walk W
         * and a ring of `sides` points at radius R round o (appended to Q, the
         * first on the ray through the drawing's farthest point), triangulated,
         * with the length of each triangle's shortest way to the ring through
         * centroids and edge midpoints (Dijkstra).
         */
        struct WayOut
        {
            std::vector<Point> Q;
            std::vector<Tri>   tris;
            std::vector<Real>  dist;                  // by triangle: from its centroid to the ring
            std::vector<Idx>   toward;                // by triangle: the next triangle that way (None: straight to the ring)
            std::vector<std::array<Idx,2>> gate;      // by triangle: the edge into `toward`
            std::vector<Idx>   start;                 // by point: the triangle its way out starts in
            std::vector<Real>  cost;                  // by point: the length of its way out
            Real phi = 0;                             // the ring's first angle

            Point Centroid( Idx t ) const
            {
                const Tri & T = tris[t];
                return { (Q[T[0]][0] + Q[T[1]][0] + Q[T[2]][0]) / 3, (Q[T[0]][1] + Q[T[1]][1] + Q[T[2]][1]) / 3 };
            }
            Idx RingCorner( Idx t ) const { return std::max({ tris[t][0], tris[t][1], tris[t][2] }); }   // ring points have the largest ids
        };

        inline bool FindWayOut( const std::vector<Point> & P, const std::vector<Idx> & W, const Point & o, Real R, Idx sides,
                                WayOut & w, std::string & why )
        {
            const Idx N = P.size(), n = W.size();
            const Real inf = std::numeric_limits<Real>::infinity();
            // The farthest point is on the hull; if the walk visits it twice, the visit that turns right has the outside.
            Idx kf = 0;
            for( Idx k = 1; k < n; ++k )
            {
                const Real d = Dist(o, P[W[k]]) - Dist(o, P[W[kf]]);
                auto turn = [&]( Idx i ) { return Fat(P[W[(i + n - 1) % n]], P[W[i]], P[W[(i + 1) % n]]); };
                if( d > 0 || (W[k] == W[kf] && turn(k) < turn(kf)) ) { kf = k; }
            }
            w.phi = Angle(o, P[W[kf]]);
            w.Q = P;
            std::vector<Idx> V;
            for( Idx i = 0; i < sides; ++i )
            {
                const Real a = w.phi + 2 * Pi * static_cast<Real>(i) / static_cast<Real>(sides);
                w.Q.push_back(Point{ o[0] + R * std::cos(a), o[1] + R * std::sin(a) });
                V.push_back(N + i);
            }
            V.push_back(N);                                                  // round the ring, in along the bridge,
            for( Idx k = 0; k <= n; ++k ) { V.push_back(W[(kf + k) % n]); }  // round the drawing, and out along it
            w.tris.clear();
            EarClip(V, { &w.Q }, w.tris);
            if( !V.empty() ) { why = "the region outside the drawing has no ear (the exterior walk is not a valid polygon)"; return false; }
            const Idx nt = w.tris.size();
            std::unordered_map<std::uint64_t,std::array<Idx,2>> E;
            for( Idx t = 0; t < nt; ++t )
            {
                for( Idx k = 0; k < 3; ++k )
                {
                    auto & e = E.try_emplace(EdgeKey(w.tris[t][k], w.tris[t][(k + 1) % 3]), std::array<Idx,2>{ None, None }).first->second;
                    (e[0] == None ? e[0] : e[1]) = t;
                }
            }
            w.dist.assign(nt, inf); w.toward.assign(nt, None); w.gate.assign(nt, { None, None });
            using Item = std::pair<Real,Idx>;
            std::priority_queue<Item,std::vector<Item>,std::greater<Item>> heap;
            for( Idx t = 0; t < nt; ++t )
            {
                if( w.RingCorner(t) < N ) { continue; }
                w.dist[t] = Dist(w.Centroid(t), w.Q[w.RingCorner(t)]);
                heap.push({ w.dist[t], t });
            }
            while( !heap.empty() )
            {
                const auto [d, t] = heap.top();
                heap.pop();
                if( d > w.dist[t] ) { continue; }
                for( Idx k = 0; k < 3; ++k )
                {
                    const Idx a = w.tris[t][k], b = w.tris[t][(k + 1) % 3];
                    const auto & e = E.at(EdgeKey(a, b));
                    const Idx s = (e[0] == t) ? e[1] : e[0];
                    if( s == None ) { continue; }
                    const Point mid = Lerp(w.Q[a], w.Q[b], 0.5);
                    const Real ds = d + Dist(w.Centroid(t), mid) + Dist(mid, w.Centroid(s));
                    if( !(ds < w.dist[s]) ) { continue; }
                    w.dist[s] = ds; w.toward[s] = t; w.gate[s] = { a, b };
                    heap.push({ ds, s });
                }
            }
            w.start.assign(N, None); w.cost.assign(N, inf);
            for( Idx t = 0; t < nt; ++t )
            {
                for( Idx v : w.tris[t] )
                {
                    if( v >= N ) { continue; }
                    const Real c = Dist(w.Q[v], w.Centroid(t)) + w.dist[t];
                    if( c < w.cost[v] ) { w.cost[v] = c; w.start[v] = t; }
                }
            }
            return true;
        }

        /*!@brief The cut from point v to the ring: v, then centroids and edge
         * midpoints of the triangles on the way, then the ring point reached. */
        inline Polyline CutFrom( const WayOut & w, Idx v, Idx N, Idx & ring_index )
        {
            Polyline path { w.Q[v] };
            Idx t = w.start[v];
            for( ; ; t = w.toward[t] )
            {
                path.push_back(w.Centroid(t));
                if( w.toward[t] == None ) { break; }
                path.push_back(Lerp(w.Q[w.gate[t][0]], w.Q[w.gate[t][1]], 0.5));
            }
            path.push_back(w.Q[w.RingCorner(t)]);
            ring_index = w.RingCorner(t) - N;
            return path;
        }

        /*!@brief The angle a path sweeps as seen from c (c is not on it). */
        inline Real Swept( const Polyline & path, const Point & c )
        {
            Real a = 0;
            for( Idx i = 0; i + 1 < path.size(); ++i ) { a += Wrap(Angle(c, path[i+1]) - Angle(c, path[i])); }
            return a;
        }
    } // namespace detail

    /*!@brief Build the Floater-Gotsman problem between the matched drawings.
     * `sides`: the rings' number of sides; `gap`: each ring's radius over the
     * one inside it. */
    inline bool BuildMorph( const Matched & M, MorphProblem & pr, std::string & why, Idx sides = 12, Real gap = 1.25 )
    {
        using namespace detail;
        pr = MorphProblem{};
        pr.P0 = M.P0; pr.P1 = M.P1; pr.chains = M.chain;
        const Idx N = M.P0.size();
        pr.n_drawing = N;
        if( N == 0 || M.exterior == None ) { why = "nothing to morph"; return false; }

        // The faces: bounded ones counterclockwise, the exterior alone clockwise, in both.
        for( int d = 0; d < 2; ++d )
        {
            const std::vector<Point> & P = d ? pr.P1 : pr.P0;
            for( Idx f = 0; f < M.faces.size(); ++f )
            {
                Polyline cyc;
                for( Idx v : M.faces[f] ) { cyc.push_back(P[v]); }
                if( (Area2(cyc) < 0) == (f == M.exterior) ) { continue; }
                why = std::string(f == M.exterior ? "the exterior face runs counterclockwise" : "a second face runs clockwise (the diagram is not connected, or the exterior is another face)")
                    + " in the " + (d ? "second" : "first") + " drawing";
                return false;
            }
        }

        // From here on each drawing is at its own centre and size (its box's centre at
        // 0, its farthest point at distance 1): the triangulation should not depend
        // on where the two drawings are or how large. Undone at the end.
        for( int d = 0; d < 2; ++d )
        {
            std::vector<Point> & P = d ? pr.P1 : pr.P0;
            Box box;
            for( const Point & p : P ) { box.Add(p); }
            pr.centre[d] = box.Centre();
            pr.size[d] = 0;
            for( const Point & p : P ) { pr.size[d] = std::max(pr.size[d], Dist(pr.centre[d], p)); }
            if( !(pr.size[d] > 0) ) { why = "a drawing has no size"; return false; }
            for( Point & p : P ) { p = Point{ (p[0] - pr.centre[d][0]) / pr.size[d], (p[1] - pr.centre[d][1]) / pr.size[d] }; }
        }
        const Point o { 0, 0 };
        const Real R1 = 1.5;

        // Each drawing's way out, and the outer vertex with the shortest way out of both.
        const std::vector<Idx> & W = M.faces[M.exterior];
        WayOut w0, w1;
        if( !FindWayOut(pr.P0, W, o, R1, sides, w0, why) ) { why += " (first drawing)"; return false; }
        if( !FindWayOut(pr.P1, W, o, R1, sides, w1, why) ) { why += " (second drawing)"; return false; }
        std::vector<Idx> visits (N, 0);
        for( Idx v : W ) { ++visits[v]; }
        Idx kv = None;
        for( Idx k = 0; k < W.size(); ++k )
        {
            const Idx v = W[k];
            if( visits[v] != 1 || w0.start[v] == None || w1.start[v] == None ) { continue; }
            if( kv == None || w0.cost[v] + w1.cost[v] < w0.cost[W[kv]] + w1.cost[W[kv]] ) { kv = k; }
        }
        if( kv == None ) { why = "the exterior walk has no vertex to cut from"; return false; }
        const Idx v = W[kv];
        Idx q0 = 0, q1 = 0;
        Polyline cut0 = CutFrom(w0, v, N, q0), cut1 = CutFrom(w1, v, N, q1);

        // How far the innermost ring turns. The drawing turns by rho (the best
        // rotation of the first drawing's points onto the second's); v's own
        // direction from a far point of the drawing turns by that, give or take;
        // the cuts add what they sweep. Whole turns are settled by rho.
        Idx far = (v == 0) ? 1 : 0;
        for( Idx u = 0; u < N; ++u )
        {
            auto reach = [&]( Idx x ) { return std::min(Dist(pr.P0[x], pr.P0[v]), Dist(pr.P1[x], pr.P1[v])); };
            if( u != v && reach(u) > reach(far) ) { far = u; }
        }
        Point c0 { 0, 0 }, c1 { 0, 0 };
        for( Idx u = 0; u < N; ++u ) { for( int k = 0; k < 2; ++k ) { c0[k] += pr.P0[u][k] / static_cast<Real>(N); c1[k] += pr.P1[u][k] / static_cast<Real>(N); } }
        Real dot = 0, crs = 0;
        for( Idx u = 0; u < N; ++u )
        {
            const Real ax = pr.P0[u][0] - c0[0], ay = pr.P0[u][1] - c0[1], bx = pr.P1[u][0] - c1[0], by = pr.P1[u][1] - c1[1];
            dot += ax * bx + ay * by; crs += ax * by - ay * bx;
        }
        const Real rho  = std::atan2(crs, dot);
        const Real rhov = rho + Wrap(Angle(pr.P1[far], pr.P1[v]) - Angle(pr.P0[far], pr.P0[v]) - rho);
        const Real want = rhov + Swept(cut1, pr.P1[far]) - Swept(cut0, pr.P0[far]);
        const Real step = 2 * Pi / static_cast<Real>(sides);
        pr.turn = want + Wrap((w1.phi + step * static_cast<Real>(q1)) - (w0.phi + step * static_cast<Real>(q0)) - want);

        // The rings: ring j at radius R1 gap^j; in the second drawing ring j is
        // turned by turn (1 - j/rings). A strip takes up to one whole step of turn.
        pr.rings = std::max<Idx>(1, static_cast<Idx>(std::ceil(std::abs(pr.turn) / (0.9 * step))));
        std::vector<Idx> ring_base;
        for( Idx j = 0; j <= pr.rings; ++j )
        {
            ring_base.push_back(pr.P0.size());
            const Real R = R1 * std::pow(gap, static_cast<Real>(j));
            const Real t = pr.turn * (1.0 - static_cast<Real>(j) / static_cast<Real>(pr.rings));
            for( Idx i = 0; i < sides; ++i )
            {
                const Real a = w0.phi + step * static_cast<Real>(i);
                pr.P0.push_back(Point{ o[0] + R * std::cos(a), o[1] + R * std::sin(a) });
                pr.P1.push_back(j == pr.rings ? pr.P0.back() : Point{ o[0] + R * std::cos(a + t), o[1] + R * std::sin(a + t) });
            }
        }
        for( Idx j = 0; j < pr.rings; ++j )
        {
            for( Idx i = 0; i < sides; ++i )
            {
                const Idx r = ring_base[j] + i, r2 = ring_base[j] + (i + 1) % sides, f = ring_base[j+1] + i, f2 = ring_base[j+1] + (i + 1) % sides;
                // the outer ring lags the inner by turn/rings in the second drawing: lean the diagonal that way
                if( pr.turn >= 0 ) { pr.tris.push_back(Tri{ r, f, f2 }); pr.tris.push_back(Tri{ r, f2, r2 }); }
                else               { pr.tris.push_back(Tri{ r, f, r2 }); pr.tris.push_back(Tri{ r2, f, f2 }); }
            }
        }
        for( Idx i = 0; i < sides; ++i ) { pr.boundary.push_back(ring_base[pr.rings] + i); }
        // The innermost ring is, in each drawing, the ring its cut was found in (renumbered in the second).
        for( Idx i = 0; i < sides; ++i ) { pr.P0[ring_base[0] + i] = w0.Q[N + i]; pr.P1[ring_base[0] + i] = w1.Q[N + (i + sides - q0 + q1) % sides]; }

        // The cuts, point for point: bisect the longest piece of the one with fewer points.
        while( cut0.size() != cut1.size() )
        {
            Polyline & c = (cut0.size() < cut1.size()) ? cut0 : cut1;
            Idx best = 0;
            for( Idx i = 1; i + 1 < c.size(); ++i ) { if( Dist(c[i], c[i+1]) > Dist(c[best], c[best+1]) ) { best = i; } }
            c.insert(c.begin() + static_cast<std::ptrdiff_t>(best) + 1, Lerp(c[best], c[best+1], 0.5));
        }
        std::vector<Idx> cut;                                   // from v's side to the ring's, both ends left out
        for( Idx i = 1; i + 1 < cut0.size(); ++i ) { cut.push_back(pr.P0.size()); pr.P0.push_back(cut0[i]); pr.P1.push_back(cut1[i]); }
        pr.cut = cut.size();

        // The cut annulus: round the ring, in along the cut, round the drawing, out along the cut.
        std::vector<std::vector<Idx>> polys (1);
        for( Idx i = 0; i <= sides; ++i ) { polys[0].push_back(ring_base[0] + (q0 + i) % sides); }
        polys[0].insert(polys[0].end(), cut.rbegin(), cut.rend());
        for( Idx k = 0; k <= W.size(); ++k ) { polys[0].push_back(W[(kv + k) % W.size()]); }
        polys[0].insert(polys[0].end(), cut.begin(), cut.end());
        for( Idx f = 0; f < M.faces.size(); ++f ) { if( f != M.exterior ) { polys.push_back(M.faces[f]); } }
        for( const auto & poly : polys )
        {
            if( !Compatible(poly, pr.P0, pr.P1, pr.tris, pr.steiner, why) ) { return false; }
        }
        for( int d = 0; d < 2; ++d )
        {
            const MeshCheck c = CheckMesh(d ? pr.P1 : pr.P0, pr.tris, pr.boundary);
            if( !c.OKQ() ) { why = std::string("the triangulation is not valid in the ") + (d ? "second" : "first") + " drawing: " + c.Why(); return false; }
        }
        // Back where the drawings are; their own points exactly as given.
        pr.Q0 = pr.P0; pr.Q1 = pr.P1;
        for( int d = 0; d < 2; ++d )
        {
            std::vector<Point> & P = d ? pr.P1 : pr.P0;
            for( Point & p : P ) { p = Point{ pr.centre[d][0] + pr.size[d] * p[0], pr.centre[d][1] + pr.size[d] * p[1] }; }
            std::copy((d ? M.P1 : M.P0).begin(), (d ? M.P1 : M.P0).end(), P.begin());
        }
        return true;
    }

    //==========================================================================
    // Frames.
    //==========================================================================

    namespace detail
    {
        /*!@brief Mean value coordinates of P[v] in its ring (Floater 2003):
         * positive when the triangles round v are, and unchanged by rotations,
         * translations and scalings. */
        inline std::vector<Real> MeanValue( const std::vector<Point> & P, Idx v, const std::vector<Idx> & ring )
        {
            const Idx k = ring.size();
            std::vector<Real> r (k), a (k), w (k);
            for( Idx i = 0; i < k; ++i )
            {
                const Point & p = P[ring[i]]; const Point & q = P[ring[(i + 1) % k]];
                const Real ax = p[0] - P[v][0], ay = p[1] - P[v][1], bx = q[0] - P[v][0], by = q[1] - P[v][1];
                r[i] = std::hypot(ax, ay);
                a[i] = std::atan2(ax * by - ay * bx, ax * bx + ay * by);
            }
            Real s = 0;
            for( Idx i = 0; i < k; ++i ) { w[i] = (std::tan(a[(i + k - 1) % k] / 2) + std::tan(a[i] / 2)) / r[i]; s += w[i]; }
            for( Real & x : w ) { x /= s; }
            return w;
        }

        // A symmetric 2x2 matrix {a, b, c} = [[a b][b c]]; a full one {m00, m01, m10, m11}.
        using Sym  = std::array<Real,3>;
        using Mat2 = std::array<Real,4>;

        /*!@brief The length-weighted centre and covariance of the chains. */
        inline void Moments( const std::vector<Point> & P, const std::vector<std::vector<Idx>> & chains, Point & mu, Sym & C )
        {
            Real L = 0, mx = 0, my = 0, xx = 0, xy = 0, yy = 0;
            for( const auto & ch : chains )
            {
                for( Idx i = 0; i + 1 < ch.size(); ++i )
                {
                    const Point & p = P[ch[i]]; const Point & q = P[ch[i+1]];
                    const Real l = Dist(p, q);
                    L += l;
                    mx += l * (p[0] + q[0]) / 2; my += l * (p[1] + q[1]) / 2;
                    xx += l * (p[0] * p[0] + q[0] * q[0] + p[0] * q[0]) / 3;
                    yy += l * (p[1] * p[1] + q[1] * q[1] + p[1] * q[1]) / 3;
                    xy += l * (p[0] * p[1] + q[0] * q[1] + (p[0] * q[1] + q[0] * p[1]) / 2) / 3;
                }
            }
            mu = { mx / L, my / L };
            C  = { xx / L - mu[0] * mu[0], xy / L - mu[0] * mu[1], yy / L - mu[1] * mu[1] };
        }

        inline Mat2 SqrtSPD( const Sym & M )
        {
            const Real s = std::sqrt(std::max(M[0] * M[2] - M[1] * M[1], 0.0)), t = std::sqrt(M[0] + M[2] + 2 * s);
            return { (M[0] + s) / t, M[1] / t, M[1] / t, (M[2] + s) / t };
        }
        inline Mat2 Mul( const Mat2 & A, const Mat2 & B )
        {
            return { A[0] * B[0] + A[1] * B[2], A[0] * B[1] + A[1] * B[3], A[2] * B[0] + A[3] * B[2], A[2] * B[1] + A[3] * B[3] };
        }
        /*!@brief The symmetric positive-definite A with A C A = T. */
        inline Mat2 Stretch( const Sym & C, const Sym & T )
        {
            const Mat2 Ch = SqrtSPD(C);
            const Real d  = Ch[0] * Ch[3] - Ch[1] * Ch[2];
            const Mat2 Chi { Ch[3] / d, -Ch[1] / d, -Ch[2] / d, Ch[0] / d };
            const Mat2 Mm = Mul(Mul(Ch, Mat2{ T[0], T[1], T[1], T[2] }), Ch);
            return Mul(Mul(Chi, SqrtSPD(Sym{ Mm[0], (Mm[1] + Mm[2]) / 2, Mm[3] })), Chi);
        }
    } // namespace detail

    inline Real SmootherStep( Real u ) { return u * u * u * (u * (6.0 * u - 15.0) + 10.0); }

    /*!@brief The morph's frames at the given times (0: the first drawing, 1:
     * the second; those two frames are the drawings exactly). Each frame holds
     * every point of the problem; the drawing's are the first n_drawing.
     * The solve is done with each drawing at its own centre and size (Q0, Q1),
     * so it does not matter where the two drawings are or how large.
     *
     * With `round` >= 0 each frame strictly between is drawn at a steady centre
     * and shape: the drawing's length-weighted centre is put on the line
     * between the two drawings' centres, and its covariance is stretched onto
     * the two drawings' interpolated one, pulled toward round, at the same
     * area, by round * 4t(1-t). That is an affine map of positive determinant,
     * so the frame is still an embedding (but the frame polygon moves with
     * it). With `round` < 0 a frame is only put at the interpolated centre and
     * size. Every frame is checked: every triangle has positive area.
     */
    inline bool MorphFrames( const MorphProblem & pr, const std::vector<Real> & times, Real round,
                             std::vector<std::vector<Point>> & frames, MorphReport & rep, std::string & why )
    {
        using namespace detail;
        const Idx N = pr.P0.size();
        rep = MorphReport{};
        frames.clear();
        std::vector<char> fixedQ (N, 0);
        for( Idx v : pr.boundary ) { fixedQ[v] = 1; }
        // each interior vertex's neighbours, counterclockwise
        std::vector<std::vector<std::array<Idx,2>>> nxt (N);
        for( const Tri & t : pr.tris ) { for( Idx k = 0; k < 3; ++k ) { nxt[t[k]].push_back({ t[(k + 1) % 3], t[(k + 2) % 3] }); } }
        std::vector<std::vector<Idx>> ring (N);
        std::vector<Idx> interior, idx (N, None);
        for( Idx v = 0; v < N; ++v )
        {
            if( fixedQ[v] || nxt[v].empty() ) { continue; }
            Idx u = nxt[v][0][0];
            for( Idx steps = 0; steps < nxt[v].size(); ++steps )
            {
                ring[v].push_back(u);
                const auto it = std::find_if(nxt[v].begin(), nxt[v].end(), [u]( const std::array<Idx,2> & e ) { return e[0] == u; });
                if( it == nxt[v].end() ) { break; }
                u = (*it)[1];
            }
            if( u != nxt[v][0][0] || ring[v].size() != nxt[v].size() ) { why = "the triangles round point " + std::to_string(v) + " do not close up"; return false; }
            idx[v] = interior.size();
            interior.push_back(v);
        }
        std::vector<std::vector<Real>> L0 (N), L1 (N);
        for( Idx v : interior )
        {
            L0[v] = MeanValue(pr.Q0, v, ring[v]);
            L1[v] = MeanValue(pr.Q1, v, ring[v]);
            for( Idx i = 0; i < L0[v].size(); ++i )
            {
                if( !(L0[v][i] > 0.0) || !(L1[v][i] > 0.0) ) { why = "a mean value weight is not positive"; return false; }
            }
        }
        Point mu0, mu1;
        Sym C0, C1;
        Moments(pr.P0, pr.chains, mu0, C0);
        Moments(pr.P1, pr.chains, mu1, C1);
        const Idx n = interior.size();
        std::vector<std::vector<Real>> X (2, std::vector<Real>(n));
        for( Idx i = 0; i < n; ++i ) { X[0][i] = pr.Q0[interior[i]][0]; X[1][i] = pr.Q0[interior[i]][1]; }
        for( Idx k = 0; k < times.size(); ++k )
        {
            const Real t = times[k];
            std::vector<Point> P = (t >= 1) ? pr.P1 : (t > 0) ? pr.Q0 : pr.P0;
            if( t > 0 && t < 1 )
            {
                SparseRows A;
                std::vector<std::vector<Real>> B (2, std::vector<Real>(n, 0.0));
                for( Idx i = 0; i < n; ++i )
                {
                    const Idx v = interior[i];
                    A.Add(i, 1.0);
                    for( Idx j = 0; j < ring[v].size(); ++j )
                    {
                        const Idx u = ring[v][j];
                        const Real lam = (1 - t) * L0[v][j] + t * L1[v][j];
                        if( idx[u] != None ) { A.Add(idx[u], -lam); }
                        else { B[0][i] += lam * pr.Q0[u][0]; B[1][i] += lam * pr.Q0[u][1]; }
                    }
                    A.EndRow();
                }
                Real res = 0;
                if( !Solve(A, B, X, res, why) ) { why = "frame " + std::to_string(k) + ": " + why; return false; }
                rep.residual = std::max(rep.residual, res);
                for( Idx i = 0; i < n; ++i ) { P[interior[i]] = Point{ X[0][i], X[1][i] }; }
                if( round >= 0 )
                {
                    Point mu;
                    Sym C;
                    Moments(P, pr.chains, mu, C);
                    const Sym Lm { (1 - t) * C0[0] + t * C1[0], (1 - t) * C0[1] + t * C1[1], (1 - t) * C0[2] + t * C1[2] };
                    const Real area = std::sqrt(std::max(Lm[0] * Lm[2] - Lm[1] * Lm[1], 0.0)), b = round * 4 * t * (1 - t);
                    const Mat2 S = Stretch(C, Sym{ (1 - b) * Lm[0] + b * area, (1 - b) * Lm[1], (1 - b) * Lm[2] + b * area });
                    if( !(S[0] * S[3] - S[1] * S[2] > 0.0) ) { why = "frame " + std::to_string(k) + ": the shape map does not keep orientation"; return false; }
                    const Point c = Lerp(mu0, mu1, t);
                    for( Point & p : P )
                    {
                        const Real dx = p[0] - mu[0], dy = p[1] - mu[1];
                        p = Point{ c[0] + S[0] * dx + S[1] * dy, c[1] + S[2] * dx + S[3] * dy };
                    }
                }
                else       // where and how large the two drawings are, interpolated
                {
                    const Point c = Lerp(pr.centre[0], pr.centre[1], t);
                    const Real sz = std::pow(pr.size[0], 1 - t) * std::pow(pr.size[1], t);
                    for( Point & p : P ) { p = Point{ c[0] + sz * p[0], c[1] + sz * p[1] }; }
                }
            }
            for( const Tri & tr : pr.tris )
            {
                const Real f = Fat(P[tr[0]], P[tr[1]], P[tr[2]]);
                rep.worst = std::min(rep.worst, f);
                if( !(f > 0.0) ) { why = "frame " + std::to_string(k) + ": a triangle does not have positive area"; return false; }
            }
            frames.push_back(std::move(P));
        }
        return true;
    }

    /*!@brief `count` times from 0 to 1, eased by SmootherStep. */
    inline std::vector<Real> EasedTimes( Idx count )
    {
        std::vector<Real> t (count, 0.0);
        for( Idx k = 0; k < count; ++k ) { t[k] = (k + 1 == count) ? 1.0 : SmootherStep(static_cast<Real>(k) / static_cast<Real>(count - 1)); }
        return t;
    }

    /*!@brief The drawing A with its points moved to a frame of the morph it was matched in. */
    inline PlaneDrawing DrawingAt( const PlaneDrawing & A, const Matched & M, const std::vector<Point> & frame )
    {
        PlaneDrawing D = A;
        for( Idx c = 0; c < D.crossings.size(); ++c ) { if( M.crossing[c] != None ) { D.crossings[c].p = frame[M.crossing[c]]; } }
        for( Idx a = 0; a < D.arcs.size(); ++a )
        {
            D.arcs[a].pts.clear();
            for( Idx v : M.chain[a] ) { D.arcs[a].pts.push_back(frame[v]); }
        }
        return D;
    }

    /*!@brief Everything a morph between two drawings made. */
    struct MorphResult
    {
        Matched      match;
        MorphProblem problem;
        MorphReport  report;
        std::vector<std::vector<Point>> frames;
    };

    /*!@brief The morph from drawing A to drawing B of one connected diagram
     * with the same exterior face, at the given times. False, with a reason,
     * if it cannot be made; there is no other morph. */
    inline bool Morph( const PlaneDrawing & A, const PlaneDrawing & B, const std::vector<Real> & times, Real round,
                       MorphResult & R, std::string & why )
    {
        return Match(A, B, R.match, why) && BuildMorph(R.match, R.problem, why)
            && MorphFrames(R.problem, times, round, R.frames, R.report, why);
    }

} // namespace Knoodle::Animate
