#pragma once

// Sweep -- one strand's move from its old position W to its new route C
// (docs/animate-design.md, "One sweep").
//
// W and C are polylines with the same two end points. Where they run apart
// they bound a chain of lobes. Each lobe is triangulated (EarClip, Refine)
// and carries the harmonic function that is 0 on W and 1 on C; the strand at
// time s is, in every lobe at once, the level curve that has swept the
// fraction s of that lobe's area, so the total swept area is the fraction s
// of the total. Shared stretches and meeting points do not move.
//
// What makes this hold in a pocket behind a narrow neck:
//   * the function is kept two-sided (a Level is a small number a, standing
//     for a or for 1 - a), and each side is solved in stages, every stage
//     re-solving the part still below 1e-4 of its own scale, so a value like
//     1e-40 is as accurate as one like 0.3;
//   * the weights are the mean value weights, positive on every mesh, so the
//     function has no interior extremum and each level set is one curve;
//   * an interior edge between two boundary points is bisected, so there are
//     free points across every neck;
//   * a shared end has no value: in its triangles the function is constant
//     along the rays from it, so every level curve runs into the end itself.
// Level curves of one function never cross. Thinning keeps that: a chord
// replaces a run of points only if it meets neither the frame before (as
// thinned) nor the frame after (as traced) nor the rest of its own curve.
//
// Every tolerance is relative: the drawing is first put in a unit box, on a
// grid of 2^-40, so a drawing a million times larger gives the same frames.
//
// No file IO, no printing, no globals. Include after Knoodle.hpp.

#include "Mesh.hpp"

namespace Knoodle::Animate
{
    struct SweepResult
    {
        std::vector<Polyline> frames;   // frames[i] is the strand when the fraction times[i] of the area is swept
        Real area = 0;                  // total area between W and C
        Idx  lobes = 0;                 // number of regions of positive area between W and C
    };

    namespace SweepKit
    {
        inline constexpr Real Snap      = 1e-9;    // points this close (of the size) are one point
        inline constexpr Real Stage     = 1e-4;    // a solve is trusted down to this fraction of its scale
        inline constexpr Real Cells     = 500;     // the mesh's edge target is sqrt(lobe area / Cells)
        inline constexpr Real Reach     = 2.5;     // level curves are straight this far (in mesh edges) from a shared end
        inline constexpr Real Margin    = 1e-4;    // a level curve keeps this far (of a mesh edge) from the edge's ends
        inline constexpr Real ThinTol   = 1.0 / 2000;   // a thinned curve's deviation, of the size
        inline constexpr Real ThinArea  = 0.004;   // and the most of its lobe's area that may cost
        inline constexpr Real MinStep   = 1.0 / 1000;   // points closer than this (of the size) are merged
        inline constexpr Idx  MaxPoints = 400;     // per lobe and frame

        /*!@brief The distance from p to the closed segment ab; t is the nearest point's parameter. */
        inline Real SegDist( const Point & p, const Point & a, const Point & b, Real & t )
        {
            const Real dx = b[0] - a[0], dy = b[1] - a[1], d2 = dx * dx + dy * dy;
            t = (d2 > 0) ? std::clamp(((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / d2, 0.0, 1.0) : 0.0;
            return Dist(p, Lerp(a, b, t));
        }

        /*!@brief Is the chord pq clear of the segments R[from..to]? A segment
         * that starts or ends at p or q may touch the chord there, no more. */
        inline bool ClearQ( const Point & p, const Point & q, const Polyline & R, Idx from, Idx to )
        {
            const Real x0 = std::min(p[0], q[0]), x1 = std::max(p[0], q[0]), y0 = std::min(p[1], q[1]), y1 = std::max(p[1], q[1]);
            for( Idx i = from; i < to; ++i )
            {
                const Point & r = R[i]; const Point & s = R[i+1];
                if( std::max(r[0], s[0]) < x0 || std::min(r[0], s[0]) > x1 || std::max(r[1], s[1]) < y0 || std::min(r[1], s[1]) > y1 ) { continue; }
                const bool rQ = (r == p || r == q), sQ = (s == p || s == q);
                if( rQ && sQ ) { return false; }
                if( rQ || sQ )
                {
                    const Point & c = rQ ? r : s; const Point & y = rQ ? s : r; const Point & x = (c == p) ? q : p;
                    if( Side(c, x, y) == 0 && (x[0] - c[0]) * (y[0] - c[0]) + (x[1] - c[1]) * (y[1] - c[1]) > 0 ) { return false; }
                }
                else if( Meet(p, q, r, s) ) { return false; }
            }
            return true;
        }

        /*!@brief Thin the curve A0, which lies strictly between `prev` and
         * `next` (the three meet only at their ends). First a run of points
         * within `step` of its first gives way to one point; then a run of
         * points within `tol` of a chord gives way to the chord. Either only
         * if the new segments are clear of prev, of next and of the rest of
         * the curve: so the result is simple and still between the two. It
         * keeps at least one point between its ends.
         */
        inline Polyline Thin( const Polyline & A0, const Polyline & prev, const Polyline & next, Real tol, Real step )
        {
            auto clearQ = [&]( const Point & p, const Point & q, const Polyline & done, const Polyline & rest, Idx from )
            {
                return ClearQ(p, q, prev, 0, prev.size() - 1) && ClearQ(p, q, next, 0, next.size() - 1)
                    && ClearQ(p, q, done, 0, done.size() - 1) && ClearQ(p, q, rest, from, rest.size() - 1);
            };
            Polyline A { A0[0] }, out { A0[0] };
            for( Idx i = 1, j = 1; i + 1 < A0.size(); i = ++j )
            {
                while( j + 2 < A0.size() && Dist(A0[j+1], A0[i]) <= step ) { ++j; }
                Point x = A0[i];                       // where the segments into and out of the run would meet
                if( !Intersection(A.back(), A0[i], A0[j], A0[j+1], x) || Dist(x, A0[i]) > 2 * step ) { x = A0[i]; }      // (the outermost choice: it leaves room for the curves that follow)
                bool oneQ = false;
                for( const Point & c : { x, A0[(i + j) / 2], A0[j] } )
                {
                    if( j > i && !oneQ && !(A.size() == 1 && j + 2 == A0.size()) && clearQ(A.back(), c, A, A0, j + 1) && clearQ(c, A0[j+1], A, A0, j + 1) ) { A.push_back(c); oneQ = true; }
                }
                if( !oneQ ) { A.insert(A.end(), A0.begin() + static_cast<std::ptrdiff_t>(i), A0.begin() + static_cast<std::ptrdiff_t>(j + 1)); }
            }
            A.push_back(A0.back());
            const Idx n = A.size();
            Real t;
            for( Idx a = 0, k = 1; a + 1 < n; a = k, k = a + 1 )
            {
                for( bool okQ = true; okQ && k + 1 < n && !(a == 0 && k + 2 == n); k += okQ )
                {
                    for( Idx m = a + 1; m <= k && okQ; ++m ) { okQ = SegDist(A[m], A[a], A[k+1], t) <= tol; }
                }
                while( k > a + 1 && !clearQ(A[a], A[k], out, A, k) ) { k = a + (k - a) / 2; }
                out.push_back(A[k]);
            }
            return out;
        }

        /*!@brief A value in [0,1] kept from the nearer end: a, or 1 - a (a <= 1/2). */
        struct Level { Real a = 0; bool hi = false; };

        inline Real Diff( const Level & x, const Level & y )      // x - y
        {
            if( x.hi == y.hi ) { return x.hi ? y.a - x.a : x.a - y.a; }
            return x.hi ? (1 - x.a) - y.a : x.a - (1 - y.a);
        }
        inline Level Mid( const Level & x, const Level & y )      // x below y
        {
            if( x.hi == y.hi ) { return { (x.a + y.a) / 2, x.hi }; }
            const Real m = (x.a + (1 - y.a)) / 2;
            return (m <= 0.5) ? Level{ m, false } : Level{ 1 - m, true };
        }
        inline Real Ratio( Real p, Real q ) { return (q > 0) ? std::min(p / q, 1.0) : 1.0; }

        /*!@brief One region between a piece of W and a piece of C. */
        struct Lobe
        {
            using Rows = std::vector<std::vector<std::pair<Idx,Real>>>;
            struct Cell { std::array<Idx,3> v { None, None, None }; Idx n = 0; Real area = 0; };   // a triangle's valued corners, lowest first

            Polyline           Wp, Cp;         // its two sides, each from the first shared end to the second
            std::vector<Point> P;              // mesh points: 0 and e1 are the shared ends
            std::vector<Tri>   T;
            std::vector<char>  cls;            // 0 free, 1 on W, 2 on C, 3 a shared end
            Idx                e1 = 0;
            Real               area = 0;
            std::array<Real,2> reach { 0, 0 }; // near each end the level curves are made straight this far
            std::unordered_map<std::uint64_t,std::array<Idx,2>> E;   // edge -> its one or two triangles
            std::vector<Level> U;              // the harmonic function
            std::vector<Idx>   order, rank;    // the valued points by value
            std::vector<Cell>  cells;          // by triangle
            std::vector<Real>  G, Done;        // by rank k: the area below order[k]'s value; the area of the triangles wholly below
            std::vector<Idx>   fan;            // the triangles at point 0

            void Link()
            {
                E.clear();
                for( Idx t = 0; t < T.size(); ++t )
                {
                    for( Idx k = 0; k < 3; ++k )
                    {
                        auto & e = E.try_emplace(EdgeKey(T[t][k], T[t][(k + 1) % 3]), std::array<Idx,2>{ None, None }).first->second;
                        (e[0] == None ? e[0] : e[1]) = t;
                    }
                }
            }

            bool Mesh( std::string & why )
            {
                if( Wp.size() == 2 ) { Wp.insert(Wp.begin() + 1, Lerp(Wp[0], Wp[1], 0.5)); }   // each side gets a point of its own
                if( Cp.size() == 2 ) { Cp.insert(Cp.begin() + 1, Lerp(Cp[0], Cp[1], 0.5)); }
                P = Wp;
                e1 = P.size() - 1;
                P.insert(P.end(), Cp.rbegin() + 1, Cp.rend() - 1);
                cls.assign(P.size(), 2);
                std::fill(cls.begin(), cls.begin() + static_cast<std::ptrdiff_t>(e1), 1);
                cls[0] = cls[e1] = 3;
                std::vector<Idx> V (P.size());
                for( Idx i = 0; i < V.size(); ++i ) { V[i] = i; }
                const Real a2 = Area2(P);
                area = std::abs(a2) / 2;
                if( a2 < 0 ) { std::reverse(V.begin(), V.end()); }
                const std::vector<Idx> boundary = V;
                const Real edge = std::sqrt(area / Cells);
                for( int end = 0; end < 2; ++end )           // the disk about an end that only its own two edges enter
                {
                    const Idx v = end ? e1 : 0;
                    Real t;
                    reach[end] = std::min(Reach * edge, 0.45 * Dist(P[0], P[e1]));
                    for( Idx i = 0; i < P.size(); ++i )
                    {
                        const Idx j = (i + 1) % P.size();
                        if( i != v && j != v ) { reach[end] = std::min(reach[end], 0.9 * SegDist(P[v], P[i], P[j], t)); }
                    }
                }
                EarClip(V, Drawings{ &P }, T);
                if( !V.empty() ) { why = "a region between W and C could not be triangulated"; return false; }
                const MeshCheck mc = CheckMesh(P, T, boundary);
                if( !mc.OKQ() ) { why = "a region between W and C has a bad triangulation: " + mc.Why(); return false; }
                const Idx n0 = P.size();
                std::vector<std::array<Idx,2>> parent;
                if( !Refine(P, T, edge, why, &parent) ) { return false; }
                Link();
                std::vector<char> onB (P.size(), 0);
                for( const auto & [key, e] : E ) { if( e[1] == None ) { onB[key >> 32] = onB[key & 0xffffffffu] = 1; } }
                auto wQ = [this]( Idx v ) { return cls[v] == 1 || cls[v] == 3; };
                for( Idx v = n0; v < P.size(); ++v ) { cls.push_back(!onB[v] ? 0 : (wQ(parent[v-n0][0]) && wQ(parent[v-n0][1])) ? 1 : 2); }
                for( bool anyQ = true; anyQ; )           // bisect every interior edge between two boundary points
                {
                    anyQ = false;
                    const Idx nT = T.size();
                    std::vector<char> touched (nT, 0);
                    for( Idx t = 0; t < nT; ++t )
                    {
                        for( Idx k = 0; k < 3 && !touched[t]; ++k )
                        {
                            const Idx a = T[t][k], b = T[t][(k + 1) % 3], c = T[t][(k + 2) % 3];
                            if( cls[a] == 0 || cls[b] == 0 ) { continue; }
                            const auto & e = E.at(EdgeKey(a, b));
                            if( e[1] == None ) { continue; }
                            const Idx n = (e[0] == t) ? e[1] : e[0];
                            if( touched[n] ) { continue; }             // next round
                            Idx j = 0;
                            while( T[n][j] != b ) { ++j; }
                            const Idx d = T[n][(j + 2) % 3], m = P.size();
                            P.push_back(Lerp(P[a], P[b], 0.5)); cls.push_back(0);
                            T[t] = Tri{ a, m, c }; T.push_back(Tri{ m, b, c });
                            T[n] = Tri{ b, m, d }; T.push_back(Tri{ m, a, d });
                            touched[t] = touched[n] = 1; anyQ = true;
                        }
                    }
                    if( anyQ ) { Link(); }
                }
                return true;
            }

            /*!@brief The function that is 0 on the side `zero`, 1 on the other
             * and harmonic between, accurate relative to its own size: solved
             * on the free points, then again and again on those whose value is
             * below Stage of the scale of the solve that gave it. */
            bool Harmonic( const Rows & R, char zero, std::vector<Real> & f, std::string & why ) const
            {
                f.assign(P.size(), 0.0);
                std::vector<Idx> S, pos (P.size(), None);
                for( Idx v = 0; v < P.size(); ++v ) { if( cls[v] == 0 ) { S.push_back(v); } else if( cls[v] != zero && cls[v] != 3 ) { f[v] = 1; } }
                for( int stage = 0; stage < 80 && !S.empty(); ++stage )
                {
                    SparseRows A;
                    std::vector<std::vector<Real>> B (1, std::vector<Real>(S.size(), 0.0)), X;
                    Real scale = 0, residual = 0;
                    for( Idx k = 0; k < S.size(); ++k ) { pos[S[k]] = k; }
                    for( Idx k = 0; k < S.size(); ++k )
                    {
                        A.Add(k, 1.0);
                        for( const auto & [j, w] : R[S[k]] ) { if( pos[j] != None ) { A.Add(pos[j], -w); } else { B[0][k] += w * f[j]; } }
                        A.EndRow();
                        scale = std::max(scale, B[0][k]);
                    }
                    for( Idx k = 0; k < S.size(); ++k ) { pos[S[k]] = None; }
                    if( !(scale > 0) ) { break; }
                    for( Real & b : B[0] ) { b /= scale; }
                    if( !Solve(A, B, X, residual, why) ) { return false; }
                    std::vector<Idx> S2;
                    for( Idx k = 0; k < S.size(); ++k )
                    {
                        f[S[k]] = scale * std::max(X[0][k], 0.0);
                        if( X[0][k] < Stage ) { S2.push_back(S[k]); }
                    }
                    if( S2.size() == S.size() ) { break; }
                    S.swap(S2);
                }
                return true;
            }

            /*!@brief The part of a cell's area below the level l. */
            Real Below( const Cell & c, const Level & l ) const
            {
                const Level & x = U[c.v[0]]; const Level & z = U[c.v[c.n-1]];
                const Real p = Diff(l, x), q = Diff(z, l), d = Diff(z, x);
                if( !(p > 0) ) { return 0; }
                if( !(q > 0) || !(d > 0) ) { return 1; }
                if( c.n == 2 ) { return Ratio(p, d); }
                const Level & y = U[c.v[1]];
                return (Diff(l, y) < 0) ? Ratio(p, Diff(y, x)) * Ratio(p, d) : 1 - Ratio(q, Diff(z, y)) * Ratio(q, d);
            }

            bool Levels( std::string & why )
            {
                const Idx n = P.size();
                Rows R (n);                                 // mean value weights, each free point's summing to 1
                for( const Tri & t : T )
                {
                    for( Idx c = 0; c < 3; ++c )
                    {
                        const Idx i = t[c], j = t[(c + 1) % 3], k = t[(c + 2) % 3];
                        if( cls[i] != 0 ) { continue; }
                        const Real ax = P[j][0] - P[i][0], ay = P[j][1] - P[i][1], bx = P[k][0] - P[i][0], by = P[k][1] - P[i][1];
                        const Real la = std::hypot(ax, ay), lb = std::hypot(bx, by), tn = (la * lb - (ax * bx + ay * by)) / (ax * by - ay * bx);
                        if( cls[j] != 3 ) { R[i].push_back({ j, tn / la }); }
                        if( cls[k] != 3 ) { R[i].push_back({ k, tn / lb }); }
                    }
                }
                for( auto & r : R )
                {
                    std::sort(r.begin(), r.end());
                    Idx m = 0; Real sum = 0;
                    for( Idx k = 0; k < r.size(); ++k ) { sum += r[k].second; if( m > 0 && r[m-1].first == r[k].first ) { r[m-1].second += r[k].second; } else { r[m++] = r[k]; } }
                    r.resize(m);
                    for( auto & e : r ) { e.second /= sum; }
                }
                std::vector<Real> u, v;
                if( !Harmonic(R, 1, u, why) || !Harmonic(R, 2, v, why) ) { return false; }
                U.assign(n, Level{});
                for( Idx x = 0; x < n; ++x )
                {
                    U[x] = (cls[x] == 0) ? ((u[x] <= 0.5) ? Level{ u[x], false } : Level{ std::min(v[x], 0.5), true }) : Level{ 0, cls[x] == 2 };
                    if( cls[x] != 3 ) { order.push_back(x); }
                }
                std::sort(order.begin(), order.end(), [this]( Idx x, Idx y )
                {
                    if( U[x].hi != U[y].hi ) { return U[y].hi; }
                    if( U[x].a != U[y].a ) { return U[x].hi ? U[x].a > U[y].a : U[x].a < U[y].a; }
                    return x < y;
                });
                rank.assign(n, None);
                for( Idx k = 0; k < order.size(); ++k ) { rank[order[k]] = k; }
                std::vector<std::vector<Idx>> start (order.size());
                for( Idx t = 0; t < T.size(); ++t )
                {
                    Cell c;
                    for( Idx x : T[t] ) { if( cls[x] != 3 ) { c.v[c.n++] = x; } else if( x == 0 ) { fan.push_back(t); } }
                    if( c.n < 2 ) { why = "a triangle holds both ends of a region"; return false; }
                    // Two or three corners: Tools' sorting networks. (std::sort on a run whose length gcc
                    // cannot bound drew -Warray-bounds; the run was never longer than the array.)
                    auto lower = [this]( Idx x, Idx y ) { return rank[x] < rank[y]; };
                    if( c.n == 3 ) { Tools::SortNet<3>()(c.v.data(), lower); } else { Tools::SortNet<2>()(c.v.data(), lower); }
                    c.area = Cross(P[T[t][0]], P[T[t][1]], P[T[t][2]]) / 2;
                    cells.push_back(c);
                    start[rank[c.v[0]]].push_back(t);
                }
                G.assign(order.size(), 0.0); Done = G;
                std::vector<Idx> active;
                Real done = 0;
                for( Idx k = 0; k < order.size(); ++k )
                {
                    active.insert(active.end(), start[k].begin(), start[k].end());
                    Real sum = 0;
                    for( Idx i = 0; i < active.size(); )
                    {
                        const Cell & c = cells[active[i]];
                        if( rank[c.v[c.n-1]] <= k ) { done += c.area; active[i] = active.back(); active.pop_back(); }
                        else { sum += c.area * Below(c, U[order[k]]); ++i; }
                    }
                    Done[k] = done;
                    G[k] = std::max(done + sum, k > 0 ? G[k-1] : 0.0);
                }
                return true;
            }

            /*!@brief Make the curve straight from where it last leaves the disk
             * of radius reach[end] about its first (0) or last (1) point. In
             * that disk the region is a sector with its tip at the end, and a
             * true level curve is all but a ray; the traced one wobbles there,
             * and may even set off the wrong way (a hook). The curves of all
             * levels stay apart: rays inside the disk, old pieces outside. */
            void Straighten( Polyline & L, int end ) const
            {
                if( end ) { std::reverse(L.begin(), L.end()); }
                Idx i = L.size() - 1;
                while( i > 0 && Dist(L[i], L[0]) >= reach[end] ) { --i; }       // L[i] is the last point inside
                if( i > 0 && i + 1 < L.size() )
                {
                    const Real dx = L[i+1][0] - L[i][0], dy = L[i+1][1] - L[i][1], ex = L[i][0] - L[0][0], ey = L[i][1] - L[0][1];
                    const Real A = dx * dx + dy * dy, B = ex * dx + ey * dy, D = ex * ex + ey * ey - reach[end] * reach[end];
                    L[i] = Lerp(L[i], L[i+1], std::clamp((std::sqrt(std::max(B * B - A * D, 0.0)) - B) / A, 0.0, 1.0));
                    L.erase(L.begin() + 1, L.begin() + static_cast<std::ptrdiff_t>(i));
                }
                if( end ) { std::reverse(L.begin(), L.end()); }
            }

            /*!@brief The level curve below which lies the fraction s of the
             * area, from the first shared end to the second. It crosses each
             * mesh edge no nearer to its ends than Margin s and Margin (1 - s)
             * of the edge: where the level is within rounding of a side's (the
             * strand waits at a wall while a pocket is swept), the curves of
             * different times are still apart, in the right order, and off
             * the wall. */
            bool Curve( Real s, Polyline & out, std::string & why ) const
            {
                const Real target = s * area;
                const Idx k = std::min<Idx>(static_cast<Idx>(std::upper_bound(G.begin(), G.end(), target) - G.begin()), order.size() - 1) - 1;
                Level lo = U[order[k]], hi = U[order[k+1]];        // the level is between these two; the points up to rank k are below it
                std::vector<Idx> active;
                for( Idx t = 0; t < cells.size(); ++t ) { if( rank[cells[t].v[0]] <= k && rank[cells[t].v[cells[t].n-1]] > k ) { active.push_back(t); } }
                for( int it = 0; it < 48 && Diff(hi, lo) > 0; ++it )
                {
                    const Level mid = Mid(lo, hi);
                    if( !(Diff(mid, lo) > 0) || !(Diff(hi, mid) > 0) ) { break; }
                    Real below = Done[k];
                    for( Idx t : active ) { below += cells[t].area * Below(cells[t], mid); }
                    (below < target ? lo : hi) = mid;
                }
                auto belowQ = [&]( Idx v ) { return rank[v] <= k; };
                auto cut = [&]( Idx a, Idx b )
                {
                    if( !belowQ(a) ) { std::swap(a, b); }
                    const Real d = Diff(U[b], U[a]);
                    return Lerp(P[a], P[b], std::clamp((d > 0) ? Diff(hi, U[a]) / d : 0.5, Margin * s, 1 - Margin * (1 - s)));
                };
                Idx t = None, a = None, b = None, count = 0;
                for( Idx f : fan )
                {
                    Idx i = 0;
                    while( T[f][i] != 0 ) { ++i; }
                    const Idx x = T[f][(i + 1) % 3], y = T[f][(i + 2) % 3];
                    if( belowQ(x) != belowQ(y) ) { t = f; a = x; b = y; ++count; }
                }
                if( count != 1 ) { why = "a level curve does not leave its end once"; return false; }
                out = { P[0], cut(a, b) };
                for( Idx steps = 0; steps <= T.size(); ++steps )
                {
                    const auto & e = E.at(EdgeKey(a, b));
                    const Idx n = (e[0] == t) ? e[1] : e[0];
                    if( n == None ) { break; }
                    Idx d = T[n][0];
                    if( d == a || d == b ) { d = T[n][1]; }
                    if( d == a || d == b ) { d = T[n][2]; }
                    if( d == e1 ) { out.push_back(P[e1]); Straighten(out, 0); Straighten(out, 1); return true; }
                    if( d == 0 ) { break; }
                    (belowQ(d) == belowQ(a) ? a : b) = d;
                    const Point p = cut(a, b);
                    if( p != out.back() ) { out.push_back(p); }
                    t = n;
                }
                why = "a level curve does not reach the far end";
                return false;
            }
        };
    } // namespace SweepKit

    /*!@brief Sweep the strand from W to C. `times` are nondecreasing values in
     * [0,1]; frames[i] is the strand when the fraction times[i] of the area
     * between W and C is swept, so frames.size() == times.size(), time 0 gives
     * W exactly and time 1 gives C exactly. W and C must be simple, with the
     * same two (distinct) ends; they may touch, cross and share stretches, if
     * they meet at the same points in the same order. Every frame runs from
     * end to end, is simple, stays in the closed region between W and C, and
     * lies on C's side of every earlier frame. False, with a reason, if that
     * cannot be had.
     */
    inline bool Sweep( const Polyline & W, const Polyline & C, const std::vector<Real> & times, SweepResult & out, std::string & why )
    {
        using namespace SweepKit;
        out = SweepResult{};
        if( W.size() < 2 || C.size() < 2 || W.front() != C.front() || W.back() != C.back() || W.front() == W.back() )
        {
            why = "W and C must run between the same two distinct end points"; return false;
        }
        for( Idx i = 0; i < times.size(); ++i )
        {
            if( !(times[i] >= 0 && times[i] <= 1) || (i > 0 && times[i] < times[i-1]) ) { why = "the times must be nondecreasing values in [0,1]"; return false; }
        }
        // The unit box, on a grid: from here on nothing depends on the drawing's scale.
        Box box;
        for( const Point & p : W ) { box.Add(p); }
        for( const Point & p : C ) { box.Add(p); }
        const Real scale = std::max(box.hi[0] - box.lo[0], box.hi[1] - box.lo[1]), grid = 1099511627776.0;
        auto unit = [&]( const Polyline & L )
        {
            Polyline Q;
            for( const Point & p : L )
            {
                const Point q { std::rint((p[0] - box.lo[0]) / scale * grid) / grid, std::rint((p[1] - box.lo[1]) / scale * grid) / grid };
                if( Q.empty() || Q.back() != q ) { Q.push_back(q); }
            }
            return Q;
        };
        const Polyline Wu = unit(W), Cu = unit(C);
        Box ubox;
        for( const Point & p : Wu ) { ubox.Add(p); }
        for( const Point & p : Cu ) { ubox.Add(p); }
        const Real size = ubox.Size(), snap = Snap * size;
        if( Wu.size() < 2 || Cu.size() < 2 ) { why = "W or C has no length"; return false; }

        // Where W and C meet: (place along W, place along C, point), in W's order.
        struct Node { Real tw, tc; Point p; };
        std::vector<Node> found, nodes;
        for( Idx i = 0; i + 1 < Wu.size(); ++i )
        {
            const Point & a = Wu[i]; const Point & b = Wu[i+1];
            for( Idx j = 0; j + 1 < Cu.size(); ++j )
            {
                const Point & c = Cu[j]; const Point & d = Cu[j+1];
                if( std::max(a[0], b[0]) + snap < std::min(c[0], d[0]) || std::max(c[0], d[0]) + snap < std::min(a[0], b[0])
                 || std::max(a[1], b[1]) + snap < std::min(c[1], d[1]) || std::max(c[1], d[1]) + snap < std::min(a[1], b[1]) ) { continue; }
                Real t;
                if( SegDist(a, c, d, t) <= snap ) { found.push_back({ Real(i), j + t, a }); }
                if( SegDist(b, c, d, t) <= snap ) { found.push_back({ Real(i + 1), j + t, b }); }
                if( SegDist(c, a, b, t) <= snap ) { found.push_back({ i + t, Real(j), c }); }
                if( SegDist(d, a, b, t) <= snap ) { found.push_back({ i + t, Real(j + 1), d }); }
                if( Side(a, b, c) * Side(a, b, d) < 0 && Side(c, d, a) * Side(c, d, b) < 0 )
                {
                    const Real d1 = Cross(c, d, a), d2 = Cross(c, d, b), d3 = Cross(a, b, c), d4 = Cross(a, b, d);
                    found.push_back({ i + d1 / (d1 - d2), j + d3 / (d3 - d4), Lerp(a, b, d1 / (d1 - d2)) });
                }
            }
        }
        std::sort(found.begin(), found.end(), []( const Node & x, const Node & y ) { return x.tw < y.tw || (x.tw == y.tw && x.tc < y.tc); });
        for( const Node & f : found )
        {
            if( !nodes.empty() && Dist(f.p, nodes.back().p) <= 2 * snap ) { continue; }
            if( !nodes.empty() && !(f.tc > nodes.back().tc) )
            {
                why = "W and C do not meet in the same order along both: the meeting point " + std::to_string(nodes.size()) + " along W comes before the one before it along C";
                return false;
            }
            nodes.push_back(f);
        }
        if( nodes.size() < 2 || nodes.front().p != Wu.front() || nodes.back().p != Wu.back() ) { why = "W or C comes back to an end point"; return false; }

        // The pieces between meeting points: a shared segment, or a lobe.
        std::vector<Lobe> lobes;
        std::vector<Idx> piece (nodes.size() - 1, None);       // the lobe, or None for a shared segment
        std::vector<Point> all;
        std::vector<std::vector<Idx>> chains;
        for( const Node & nd : nodes ) { all.push_back(nd.p); }
        for( Idx k = 0; k + 1 < nodes.size(); ++k )
        {
            auto between = [&]( const Polyline & L, Real t0, Real t1 )
            {
                Polyline Q { nodes[k].p };
                for( Idx i = static_cast<Idx>(t0) + 1; i < L.size() && Real(i) < t1; ++i )
                {
                    if( Dist(L[i], nodes[k].p) > 2 * snap && Dist(L[i], nodes[k+1].p) > 2 * snap ) { Q.push_back(L[i]); }
                }
                Q.push_back(nodes[k+1].p);
                return Q;
            };
            auto chain = [&]( const Polyline & L )
            {
                chains.push_back({ k });
                for( Idx m = 1; m + 1 < L.size(); ++m ) { all.push_back(L[m]); chains.back().push_back(all.size() - 1); }
                chains.back().push_back(k + 1);
            };
            Lobe lobe;
            lobe.Wp = between(Wu, nodes[k].tw, nodes[k+1].tw);
            lobe.Cp = between(Cu, nodes[k].tc, nodes[k+1].tc);
            chain(lobe.Wp);
            if( lobe.Wp.size() == 2 && lobe.Cp.size() == 2 ) { continue; }
            chain(lobe.Cp);
            piece[k] = lobes.size();
            lobes.push_back(std::move(lobe));
        }
        if( !EmbeddedQ(all, chains, why) ) { why = "W or C crosses itself, or they run too close to tell apart (" + why + ")"; return false; }
        for( Idx l = 0; l < lobes.size(); ++l )                 // no lobe inside another: their sides do not cross, so one point tells
        {
            Polyline ring = lobes[l].Wp;
            ring.insert(ring.end(), lobes[l].Cp.rbegin() + 1, lobes[l].Cp.rend() - 1);
            for( Idx m = 0; m < lobes.size(); ++m )
            {
                if( m != l && Inside(ring, lobes[m].Wp.size() > 2 ? lobes[m].Wp[1] : lobes[m].Cp[1]) ) { why = "the regions between W and C overlap: one lies inside another"; return false; }
            }
        }

        // Each lobe: mesh, harmonic function, the level curves, thinned in order.
        std::vector<Real> ts;                                   // the distinct times strictly between 0 and 1
        for( Real s : times ) { if( s > 0 && s < 1 && (ts.empty() || s > ts.back()) ) { ts.push_back(s); } }
        std::vector<std::vector<Polyline>> curves (lobes.size());
        for( Idx l = 0; l < lobes.size(); ++l )
        {
            Lobe & lobe = lobes[l];
            if( !lobe.Mesh(why) || !lobe.Levels(why) ) { return false; }
            out.area += lobe.area;
            std::vector<Polyline> raw (ts.size());
            for( Idx i = 0; i < ts.size(); ++i ) { if( !lobe.Curve(ts[i], raw[i], why) ) { return false; } }
            for( Idx i = 0; i < ts.size(); ++i )
            {
                if( i > 0 && raw[i] == raw[i-1] ) { curves[l].push_back(curves[l].back()); continue; }
                Idx j = i + 1;
                while( j < ts.size() && raw[j] == raw[i] ) { ++j; }
                const Polyline & prev = (i > 0) ? curves[l].back() : lobe.Wp;
                const Polyline & next = (j < ts.size()) ? raw[j] : lobe.Cp;
                Real tol = std::min(ThinTol * size, ThinArea * lobe.area / Length(raw[i])), step = MinStep * size;
                Polyline Q = Thin(raw[i], prev, next, tol, step);
                for( int more = 0; more < 4 && Q.size() > MaxPoints; ++more ) { tol *= 2; step *= 2; Q = Thin(Q, prev, next, tol, step); }
                curves[l].push_back(std::move(Q));
            }
        }
        out.lobes = lobes.size();
        out.area *= scale * scale;

        for( Real s : times )
        {
            if( s <= 0 ) { out.frames.push_back(W); continue; }
            if( s >= 1 ) { out.frames.push_back(C); continue; }
            const Idx i = static_cast<Idx>(std::lower_bound(ts.begin(), ts.end(), s) - ts.begin());
            Polyline F { W.front() };
            for( Idx k = 0; k + 1 < nodes.size(); ++k )
            {
                const Polyline one { nodes[k].p, nodes[k+1].p };
                const Polyline & Q = (piece[k] == None) ? one : curves[piece[k]][i];
                for( Idx m = 1; m < Q.size(); ++m ) { F.push_back({ box.lo[0] + scale * Q[m][0], box.lo[1] + scale * Q[m][1] }); }
            }
            F.back() = W.back();
            out.frames.push_back(std::move(F));
        }
        return true;
    }

} // namespace Knoodle::Animate
