#pragma once

// Mesh -- the animator's triangle-mesh kit (docs/animate-design.md).
//
//   EarClip    triangulate a polygon by clipping ears that are ears in EVERY
//              drawing given (one drawing: plain ear clipping; two: the part
//              of a compatible triangulation that needs no extra points).
//   CheckMesh  the validity check: every triangle positive, every edge in at
//              most two triangles, a disk, the boundary the one expected.
//   Refine     Delaunay refinement of a polygon's triangulation: Lawson's
//              flips, then longest-edge bisection down to a target length.
//   Solve      the sparse solve: UMFPACK (Tensors' wrapper) when
//              KNOODLE_USE_UMFPACK is set, else a small BiCGSTAB.
//
// Tolerances are relative: a triangle's flatness is its doubled area over the
// sum of its squared edges, a number that does not change with scale.
//
// No file IO, no printing, no globals. Include after Knoodle.hpp.

#include "PlaneDrawing.hpp"

#include <cstdint>
#include <limits>
#include <unordered_map>

namespace Knoodle::Animate
{
    using Tri      = std::array<Idx,3>;
    using Drawings = std::vector<const std::vector<Point> *>;

    // A triangle is positive when its flatness is above FlatTol. A polygon's
    // corner is an ear's tip only above EarTol: a point put on a segment by
    // interpolation is off it by rounding, about 1e-16 of the drawing's size
    // over the segment's length, and must not pass for a corner.
    inline constexpr Real FlatTol = 1e-12;
    inline constexpr Real EarTol  = 1e-10;

    /*!@brief Doubled area over the sum of squared edges: positive for a
     * counterclockwise triangle, at most 1/(2 sqrt 3), 0 for a flat one. */
    inline Real Fat( const Point & a, const Point & b, const Point & c )
    {
        const Real s = (b[0]-a[0])*(b[0]-a[0]) + (b[1]-a[1])*(b[1]-a[1]) + (c[0]-b[0])*(c[0]-b[0]) + (c[1]-b[1])*(c[1]-b[1])
                     + (a[0]-c[0])*(a[0]-c[0]) + (a[1]-c[1])*(a[1]-c[1]);
        return (s > 0) ? Cross(a, b, c) / s : 0.0;
    }

    //==========================================================================
    // Ear clipping.
    //==========================================================================

    /*!@brief How good an ear corner k of the polygon V is: the least flatness
     * of its triangle over the drawings, or -1 if it is not an ear in one of
     * them. An ear is positive and holds no other corner of V in its closed
     * triangle; a corner at the very place of one of the ear's own (the other
     * visit to a pinch point, the far side of a slit) does not count.
     */
    inline Real EarQuality( const std::vector<Idx> & V, Idx k, const Drawings & D, Real tol )
    {
        const Idx n = V.size(), i0 = V[(k + n - 1) % n], i1 = V[k], i2 = V[(k + 1) % n];
        Real q = std::numeric_limits<Real>::infinity();
        for( const std::vector<Point> * P : D )
        {
            const Point & a = (*P)[i0]; const Point & b = (*P)[i1]; const Point & c = (*P)[i2];
            const Real f = Fat(a, b, c);
            if( !(f > tol) ) { return -1; }
            for( Idx j : V )
            {
                if( j == i0 || j == i1 || j == i2 ) { continue; }
                const Point & p = (*P)[j];
                if( p == a || p == b || p == c ) { continue; }
                if( Fat(a, b, p) >= -tol && Fat(b, c, p) >= -tol && Fat(c, a, p) >= -tol ) { return -1; }
            }
            q = std::min(q, f);
        }
        return q;
    }

    /*!@brief Clip ears off the polygon V (point ids, counterclockwise in every
     * drawing; weakly simple: it may touch itself, under different ids, but
     * not cross), best ear first, for as long as there is a corner that is an
     * ear in every drawing. The triangles are appended to `tris`; V is left
     * holding what could not be clipped, and is empty on success. With one
     * drawing of a valid polygon it always succeeds.
     */
    inline void EarClip( std::vector<Idx> & V, const Drawings & D, std::vector<Tri> & tris, Real tol = EarTol )
    {
        std::vector<Real> q (V.size());
        auto all = [&]() { for( Idx k = 0; k < V.size(); ++k ) { q[k] = EarQuality(V, k, D, tol); } };
        all();
        bool freshQ = true;
        while( V.size() > 3 )
        {
            const Idx n = V.size();
            const Idx best = static_cast<Idx>(std::max_element(q.begin(), q.end()) - q.begin());
            if( !(q[best] > 0) )
            {
                if( freshQ ) { return; }
                all(); freshQ = true;          // a clipped corner may have been all that stood in an ear's way
                continue;
            }
            tris.push_back(Tri{ V[(best + n - 1) % n], V[best], V[(best + 1) % n] });
            V.erase(V.begin() + static_cast<std::ptrdiff_t>(best));
            q.erase(q.begin() + static_cast<std::ptrdiff_t>(best));
            const Idx m = n - 1, l = (best + m - 1) % m, r = best % m;
            q[l] = EarQuality(V, l, D, tol);
            q[r] = EarQuality(V, r, D, tol);
            freshQ = false;
        }
        if( V.size() == 3 )
        {
            for( const std::vector<Point> * P : D ) { if( !(Fat((*P)[V[0]], (*P)[V[1]], (*P)[V[2]]) > tol) ) { return; } }
            tris.push_back(Tri{ V[0], V[1], V[2] });
            V.clear();
        }
    }

    //==========================================================================
    // The validity check.
    //==========================================================================

    inline std::uint64_t EdgeKey( Idx a, Idx b )
    {
        return (static_cast<std::uint64_t>(std::min(a, b)) << 32) | static_cast<std::uint64_t>(std::max(a, b));
    }

    struct MeshCheck
    {
        Idx  bad       = 0;       // triangles whose flatness is not above the tolerance
        Idx  overfull  = 0;       // edges in more than two triangles
        long euler     = 0;       // V - E + F
        bool boundaryQ = false;   // the edges in one triangle are exactly the boundary cycle's
        Real worst     = std::numeric_limits<Real>::infinity();   // the least flatness

        bool OKQ() const { return bad == 0 && overfull == 0 && euler == 1 && boundaryQ; }
        std::string Why() const
        {
            return std::to_string(bad) + " triangles not positive, " + std::to_string(overfull) + " edges in more than two triangles, Euler characteristic "
                 + std::to_string(euler) + (boundaryQ ? "" : ", wrong boundary");
        }
    };

    /*!@brief Is `tris` a triangulation of a disk with boundary cycle `boundary`,
     * every triangle positive (flatness above `tol`) in the drawing P? */
    inline MeshCheck CheckMesh( const std::vector<Point> & P, const std::vector<Tri> & tris, const std::vector<Idx> & boundary, Real tol = FlatTol )
    {
        MeshCheck r;
        std::vector<std::uint64_t> edges, bnd, once;
        std::vector<Idx> verts;
        for( const Tri & t : tris )
        {
            const Real f = Fat(P[t[0]], P[t[1]], P[t[2]]);
            r.worst = std::min(r.worst, f);
            if( !(f > tol) ) { ++r.bad; }
            for( Idx k = 0; k < 3; ++k ) { edges.push_back(EdgeKey(t[k], t[(k + 1) % 3])); verts.push_back(t[k]); }
        }
        std::sort(edges.begin(), edges.end());
        std::sort(verts.begin(), verts.end());
        Idx n_edges = 0;
        for( Idx i = 0; i < edges.size(); )
        {
            Idx j = i;
            while( j < edges.size() && edges[j] == edges[i] ) { ++j; }
            ++n_edges;
            if( j - i == 1 ) { once.push_back(edges[i]); }
            if( j - i > 2 ) { ++r.overfull; }
            i = j;
        }
        for( Idx i = 0; i < boundary.size(); ++i ) { bnd.push_back(EdgeKey(boundary[i], boundary[(i + 1) % boundary.size()])); }
        std::sort(bnd.begin(), bnd.end());
        const long n_verts = static_cast<long>(std::unique(verts.begin(), verts.end()) - verts.begin());
        r.euler = n_verts - static_cast<long>(n_edges) + static_cast<long>(tris.size());
        r.boundaryQ = (once == bnd);
        return r;
    }

    //==========================================================================
    // Delaunay refinement.
    //==========================================================================

    /*!@brief Refine the triangulation `tris` of a polygon in place. First
     * Lawson's flips make every interior edge Delaunay (a flip only happens in
     * a strictly convex quadrilateral, so triangles stay positive). Then every
     * triangle whose longest edge is longer than `max_edge` is bisected there,
     * together with its neighbour across that edge (Rivara: the neighbour is
     * bisected first until the edge is its longest too), so the mesh stays
     * conforming and no angle falls below half the smallest there was. Edges
     * in one triangle are the boundary: never flipped, and split in two
     * boundary edges. New points are appended to P; `parent`, if given,
     * receives for each the edge it bisected. False if the mesh would pass
     * `max_tris` triangles.
     */
    inline bool Refine( std::vector<Point> & P, std::vector<Tri> & tris, Real max_edge, std::string & why,
                        std::vector<std::array<Idx,2>> * parent = nullptr, Idx max_tris = 400000 )
    {
        std::vector<Tri> T = tris;
        std::vector<char> alive (T.size(), 1);
        std::unordered_map<std::uint64_t,std::array<Idx,2>> E;     // edge -> its one or two triangles
        Idx live = 0;
        auto key = [&]( Idx t, Idx k ) { return EdgeKey(T[t][k], T[t][(k + 1) % 3]); };
        auto link = [&]( Idx t )
        {
            for( Idx k = 0; k < 3; ++k )
            {
                auto & e = E.try_emplace(key(t, k), std::array<Idx,2>{ None, None }).first->second;
                (e[0] == None ? e[0] : e[1]) = t;
            }
            ++live;
        };
        auto add = [&]( const Tri & t ) { T.push_back(t); alive.push_back(1); link(T.size() - 1); };
        auto kill = [&]( Idx t )
        {
            for( Idx k = 0; k < 3; ++k )
            {
                auto it = E.find(key(t, k));
                auto & e = it->second;
                if( e[0] == t ) { e[0] = e[1]; }
                e[1] = None;
                if( e[0] == None ) { E.erase(it); }
            }
            alive[t] = 0; --live;
        };
        auto other = [&]( Idx t, Idx a, Idx b ) { const auto & e = E.at(EdgeKey(a, b)); return (e[0] == t) ? e[1] : e[0]; };
        auto corner = [&]( Idx t, Idx a, Idx b ) { Idx j = 0; while( !(T[t][j] == a && T[t][(j + 1) % 3] == b) ) { ++j; } return T[t][(j + 2) % 3]; };
        auto incircle = [&]( Idx a, Idx b, Idx c, Idx d )   // d strictly inside the circle through the positive triangle abc
        {
            const Real ax = P[a][0] - P[d][0], ay = P[a][1] - P[d][1], bx = P[b][0] - P[d][0], by = P[b][1] - P[d][1], cx = P[c][0] - P[d][0], cy = P[c][1] - P[d][1];
            const Real a2 = ax * ax + ay * ay, b2 = bx * bx + by * by, c2 = cx * cx + cy * cy;
            const Real det = ax * (by * c2 - b2 * cy) - ay * (bx * c2 - b2 * cx) + a2 * (bx * cy - by * cx);
            return det > 1e-9 * (a2 + b2 + c2) * (std::abs(bx * cy - by * cx) + std::abs(ax * cy - ay * cx) + std::abs(ax * by - ay * bx));
        };
        for( Idx t = 0; t < T.size(); ++t ) { link(t); }

        std::vector<std::uint64_t> stack;
        for( const auto & [k, e] : E ) { if( e[1] != None ) { stack.push_back(k); } }
        std::sort(stack.begin(), stack.end());
        for( Idx flips = 0, cap = 50 * (T.size() + 16); !stack.empty() && flips < cap; )
        {
            const auto it = E.find(stack.back());
            stack.pop_back();
            if( it == E.end() || it->second[1] == None ) { continue; }
            const Idx t = it->second[0], n = it->second[1];
            Idx i = 0;
            while( key(t, i) != it->first ) { ++i; }
            const Idx a = T[t][i], b = T[t][(i + 1) % 3], c = T[t][(i + 2) % 3], d = corner(n, b, a);
            if( !incircle(a, b, c, d) || !(Fat(P[c], P[a], P[d]) > FlatTol) || !(Fat(P[d], P[b], P[c]) > FlatTol) ) { continue; }
            kill(t); kill(n);
            add(Tri{ c, a, d }); add(Tri{ d, b, c });
            ++flips;
            for( const auto k : { EdgeKey(a, d), EdgeKey(d, b), EdgeKey(b, c), EdgeKey(c, a) } ) { stack.push_back(k); }
        }

        auto len2 = [&]( Idx a, Idx b ) { const Real dx = P[b][0] - P[a][0], dy = P[b][1] - P[a][1]; return dx * dx + dy * dy; };
        auto longest = [&]( Idx t )      // ties to the larger key, so that two triangles agree on a shared edge
        {
            Idx best = 0;
            for( Idx k = 1; k < 3; ++k )
            {
                const Real l = len2(T[t][k], T[t][(k + 1) % 3]), lb = len2(T[t][best], T[t][(best + 1) % 3]);
                if( l > lb || (l == lb && key(t, k) > key(t, best)) ) { best = k; }
            }
            return best;
        };
        auto bisect = [&]( Idx t0 )
        {
            std::vector<Idx> todo { t0 };
            while( !todo.empty() )
            {
                const Idx t = todo.back();
                if( !alive[t] ) { todo.pop_back(); continue; }
                const Idx i = longest(t), a = T[t][i], b = T[t][(i + 1) % 3], c = T[t][(i + 2) % 3], n = other(t, a, b);
                if( n != None && key(n, longest(n)) != EdgeKey(a, b) ) { todo.push_back(n); continue; }
                P.push_back(Lerp(P[a], P[b], 0.5));
                if( parent ) { parent->push_back({ a, b }); }
                const Idx m = P.size() - 1;
                if( n != None ) { const Idx d = corner(n, b, a); kill(n); add(Tri{ b, m, d }); add(Tri{ m, a, d }); }
                kill(t); add(Tri{ a, m, c }); add(Tri{ m, b, c });
            }
        };
        if( max_edge > 0 )
        {
            for( Idx t = 0; t < T.size(); ++t )
            {
                if( !alive[t] ) { continue; }
                const Idx i = longest(t);
                if( len2(T[t][i], T[t][(i + 1) % 3]) > max_edge * max_edge ) { bisect(t); }
                if( live > max_tris ) { why = "refinement passed " + std::to_string(max_tris) + " triangles"; return false; }
            }
        }
        tris.clear();
        for( Idx t = 0; t < T.size(); ++t ) { if( alive[t] ) { tris.push_back(T[t]); } }
        return true;
    }

    //==========================================================================
    // The sparse solve.
    //==========================================================================

    /*!@brief A square sparse matrix by rows. */
    struct SparseMatrix
    {
        Idx n = 0;
        std::vector<Idx>  rp { 0 };  // row pointers, n + 1
        std::vector<Idx>  ci;        // column indices
        std::vector<Real> v;         // values

        void Add( Idx col, Real value ) { ci.push_back(col); v.push_back(value); }
        void EndRow()                // sorts the row by column, as a direct solver wants it
        {
            std::vector<std::pair<Idx,Real>> row;
            for( Idx k = rp.back(); k < ci.size(); ++k ) { row.push_back({ ci[k], v[k] }); }
            std::sort(row.begin(), row.end());
            for( Idx k = 0; k < row.size(); ++k ) { ci[rp.back() + k] = row[k].first; v[rp.back() + k] = row[k].second; }
            rp.push_back(ci.size()); ++n;
        }

        void Mul( const std::vector<Real> & x, std::vector<Real> & y ) const
        {
            y.assign(n, 0.0);
            for( Idx i = 0; i < n; ++i ) { for( Idx k = rp[i]; k < rp[i+1]; ++k ) { y[i] += v[k] * x[ci[k]]; } }
        }

        /*!@brief ||b - A x|| / ||b|| (0 if b is 0 and so is the residual). */
        Real Residual( const std::vector<Real> & b, const std::vector<Real> & x ) const
        {
            std::vector<Real> y;
            Mul(x, y);
            Real e = 0, bn = 0;
            for( Idx i = 0; i < n; ++i ) { e += (b[i] - y[i]) * (b[i] - y[i]); bn += b[i] * b[i]; }
            return (bn > 0) ? std::sqrt(e / bn) : std::sqrt(e);
        }
    };

    /*!@brief BiCGSTAB with Jacobi preconditioning for A x = b, from the guess
     * in x. Stops at relative residual `tol` or after `max_it` iterations. */
    inline void BiCGSTAB( const SparseMatrix & A, const std::vector<Real> & b, std::vector<Real> & x, Real tol, Idx max_it )
    {
        const Idx n = A.n;
        std::vector<Real> d (n, 1.0), Ax, res (n), rh, p (n, 0.0), vv (n, 0.0), s (n), t (n), y (n), z (n);
        for( Idx i = 0; i < n; ++i ) { for( Idx k = A.rp[i]; k < A.rp[i+1]; ++k ) { if( A.ci[k] == i && A.v[k] != 0.0 ) { d[i] = 1.0 / A.v[k]; } } }
        auto dot = [n]( const std::vector<Real> & a, const std::vector<Real> & c ) { Real r = 0; for( Idx i = 0; i < n; ++i ) { r += a[i] * c[i]; } return r; };
        const Real bn = std::sqrt(dot(b, b));
        if( bn == 0.0 ) { x.assign(n, 0.0); return; }
        A.Mul(x, Ax);
        for( Idx i = 0; i < n; ++i ) { res[i] = b[i] - Ax[i]; }
        rh = res;
        Real rho = 1.0, alpha = 1.0, omega = 1.0;
        for( Idx it = 0; it < max_it && std::sqrt(dot(res, res)) > tol * bn; ++it )
        {
            const Real rho1 = dot(rh, res);
            if( rho1 == 0.0 ) { break; }
            const Real beta = (rho1 / rho) * (alpha / omega);
            for( Idx i = 0; i < n; ++i ) { p[i] = res[i] + beta * (p[i] - omega * vv[i]); y[i] = d[i] * p[i]; }
            A.Mul(y, vv);
            const Real rv = dot(rh, vv);
            if( rv == 0.0 ) { break; }
            alpha = rho1 / rv;
            for( Idx i = 0; i < n; ++i ) { s[i] = res[i] - alpha * vv[i]; z[i] = d[i] * s[i]; }
            A.Mul(z, t);
            const Real tt = dot(t, t);
            omega = (tt > 0.0) ? dot(t, s) / tt : 0.0;
            for( Idx i = 0; i < n; ++i ) { x[i] += alpha * y[i] + omega * z[i]; res[i] = s[i] - omega * t[i]; }
            rho = rho1;
            if( omega == 0.0 ) { break; }
        }
    }

    /*!@brief Solve A X = B for each right-hand side in B. With
     * KNOODLE_USE_UMFPACK: one sparse LU factorization. Without: BiCGSTAB from
     * the guesses in X (the previous frame's solution, say), restarted from
     * where it stops while that still gains. `residual` returns the largest
     * true relative residual; false, with a reason, if it is above `accept`.
     */
    inline bool Solve( const SparseMatrix & A, const std::vector<std::vector<Real>> & B, std::vector<std::vector<Real>> & X,
                       Real & residual, std::string & why, Real accept = 1e-9 )
    {
        X.resize(B.size());
        for( auto & x : X ) { if( x.size() != A.n ) { x.assign(A.n, 0.0); } }
        residual = 0;
        if( A.n == 0 ) { return true; }
#ifdef KNOODLE_USE_UMFPACK
        {
            const std::vector<Int64> rp (A.rp.begin(), A.rp.end()), ci (A.ci.begin(), A.ci.end());
            Tensors::UMFPACK<Real,Int64> lu ( static_cast<Int64>(A.n), static_cast<Int64>(A.n), rp.data(), ci.data(), A.v.data() );
            if( lu.NumericStatus() != 0 ) { why = "UMFPACK could not factor the matrix (status " + std::to_string(lu.NumericStatus()) + ")"; return false; }
            for( Idx k = 0; k < B.size(); ++k ) { lu.Solve(B[k].data(), X[k].data()); }
        }
#else
        for( Idx k = 0; k < B.size(); ++k )
        {
            Real last = std::numeric_limits<Real>::infinity();
            for( int attempt = 0; attempt < 12; ++attempt )
            {
                BiCGSTAB(A, B[k], X[k], 1e-14, std::max<Idx>(200, 10 * A.n));
                const Real r = A.Residual(B[k], X[k]);
                if( r <= 1e-13 || !(r < 0.5 * last) ) { break; }
                last = r;
            }
        }
#endif
        for( Idx k = 0; k < B.size(); ++k ) { residual = std::max(residual, A.Residual(B[k], X[k])); }
        if( !(residual <= accept) ) { why = "the sparse solve stopped at relative residual " + std::to_string(residual); return false; }
        return true;
    }

} // namespace Knoodle::Animate
