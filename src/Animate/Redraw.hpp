#pragma once

// The view of a space curve as a drawing (docs/animate-design.md, "Redraw
// records"). A redraw record carries a closed polygonal curve E in space and
// a rotation R; its claim is that E, seen from above, is the record's diagram
// and R E is the next one. Here a curve is seen along (a, b, 1): a point
// (x, y, z) is drawn at (x - a z, y - b z), and of two strands that meet in
// the picture the one with the larger z there is over. That is the map
// Prosector's simulation of simplicity uses, with a tilt large enough to see.
//
// Standard library, PlaneDrawing.hpp and Pass.hpp (for the weave) only.

#include "Pass.hpp"

#include <string>
#include <vector>

namespace Knoodle::Animate
{
    /*!@brief The drawing that the closed curves `curves` make when seen along
     * (a, b, 1), in labels of its own. A curve that meets nothing comes back in
     * `loops`. The exterior is not set. False, with a reason, if the view is
     * degenerate: two pieces overlap, or one passes through another's corner. */
    inline bool ViewDrawing( const std::vector<std::vector<P3>> & curves, const std::vector<long long> & colors, Real a, Real b,
                             PlaneDrawing & V, std::vector<Polyline> & loops, std::vector<long long> & loop_colors, std::string & why )
    {
        Weave Wv;
        std::vector<std::vector<Real>> depth (curves.size());
        Real size = 0;
        for( Idx k = 0; k < curves.size(); ++k )
        {
            Weave::Strand S; S.color = (k < colors.size()) ? colors[k] : static_cast<long long>(k);
            for( const P3 & p : curves[k] )
            {
                S.p.push_back(Point{ p.x - a * p.z, p.y - b * p.z }); S.x.push_back(-1); S.over.push_back(0); S.arc.push_back(None);
                depth[k].push_back(p.z);
                size = std::max(size, std::max(std::abs(S.p.back()[0]), std::abs(S.p.back()[1])));
            }
            if( S.p.size() < 3 ) { why = "a curve has fewer than three corners"; return false; }
            Wv.strands.push_back(std::move(S));
        }
        const Real tol = 1e-9 * std::max(size, 1e-300);
        struct Ins { Idx s; Real t; long x; char over; Point q; };
        std::vector<std::vector<Ins>> ins (curves.size());
        for( Idx j = 0; j < Wv.strands.size(); ++j )
        {
            const Polyline & P = Wv.strands[j].p; const Idx n = P.size();
            for( Idx l = j; l < Wv.strands.size(); ++l )
            {
                const Polyline & Q = Wv.strands[l].p; const Idx m = Q.size();
                for( Idx s = 0; s < n; ++s )
                {
                    for( Idx u = (l == j) ? s + 1 : 0; u < m; ++u )
                    {
                        const bool neighboursQ = (l == j) && (u == s + 1 || (s == 0 && u == m - 1));
                        const Point & p0 = P[s], & p1 = P[(s + 1) % n], & q0 = Q[u], & q1 = Q[(u + 1) % m];
                        const Real rx = p1[0] - p0[0], ry = p1[1] - p0[1], sx = q1[0] - q0[0], sy = q1[1] - q0[1], den = rx * sy - ry * sx;
                        const Real lp = std::hypot(rx, ry), lq = std::hypot(sx, sy);
                        if( lp <= tol || lq <= tol ) { why = "a piece of the curve is seen end on"; return false; }
                        if( std::abs(den) <= 1e-12 * lp * lq )
                        {
                            // parallel in the picture: they must not lie along one line and overlap
                            if( neighboursQ ) { continue; }
                            const Real off = std::abs((q0[0] - p0[0]) * ry - (q0[1] - p0[1]) * rx) / lp;
                            const Real t0 = ((q0[0] - p0[0]) * rx + (q0[1] - p0[1]) * ry) / (lp * lp), t1 = ((q1[0] - p0[0]) * rx + (q1[1] - p0[1]) * ry) / (lp * lp);
                            if( off <= tol && std::max(t0, t1) > 0 && std::min(t0, t1) < 1 ) { why = "two pieces of the curve overlap in the picture"; return false; }
                            continue;
                        }
                        if( neighboursQ ) { continue; }
                        const Real tp = ((q0[0] - p0[0]) * sy - (q0[1] - p0[1]) * sx) / den, tq = ((q0[0] - p0[0]) * ry - (q0[1] - p0[1]) * rx) / den;
                        const Real ep = tol / lp, eq = tol / lq;
                        if( tp < -ep || tp > 1 + ep || tq < -eq || tq > 1 + eq ) { continue; }
                        if( tp <= ep || tp >= 1 - ep || tq <= eq || tq >= 1 - eq ) { why = "a piece of the curve passes through a corner of another in the picture"; return false; }
                        const Real zp = depth[j][s] + (depth[j][(s + 1) % n] - depth[j][s]) * tp, zq = depth[l][u] + (depth[l][(u + 1) % m] - depth[l][u]) * tq;
                        if( zp == zq ) { why = "two pieces of the curve meet in space"; return false; }
                        const long x = Wv.next_crossing++;
                        // one point for both passages, worked out on the pieces as they are now
                        const Point at { p0[0] + rx * tp, p0[1] + ry * tp };
                        ins[j].push_back(Ins{ s, tp, x, static_cast<char>(zp > zq ? 1 : 0), at });
                        ins[l].push_back(Ins{ u, tq, x, static_cast<char>(zq > zp ? 1 : 0), at });
                    }
                }
            }
        }
        for( Idx j = 0; j < Wv.strands.size(); ++j )
        {
            Weave::Strand & T = Wv.strands[j];
            std::sort(ins[j].begin(), ins[j].end(), []( const Ins & p, const Ins & q ) { return (p.s != q.s) ? (p.s > q.s) : (p.t > q.t); });
            for( const Ins & e : ins[j] )
            {
                const auto at = static_cast<std::ptrdiff_t>(e.s) + 1;
                T.p.insert(T.p.begin() + at, e.q); T.x.insert(T.x.begin() + at, e.x); T.over.insert(T.over.begin() + at, e.over); T.arc.insert(T.arc.begin() + at, None);
            }
        }
        return DrawingOf(Wv, V, loops, loop_colors, why);
    }

} // namespace Knoodle::Animate
