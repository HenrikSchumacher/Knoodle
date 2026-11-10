#pragma once

// A turn of the sphere: the animated change of exterior face
// (docs/animate-design.md). The plane rolls up into a sphere, the sphere turns
// until a chosen point of the new exterior face is at its top, and it unrolls
// from there. Start to finish that is the Moebius map z -> 1 / (z - pole), up
// to a similarity; this file builds the three stages and the drawing left.
//
// The diagram comes in as closed curves of (x, y, h): h is a small height,
// positive where the strand is the over strand of a crossing and negative
// where it is the under strand, so that the curves are embedded in space.
//
// Standard library only.

#include "Frame.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Knoodle::Animate
{
    struct TurnOptions
    {
        Real radius   = 0;      // the sphere's; 0: 0.65 of the drawing's reach from its centre
        Real max_step = 1.0;    // longest piece an edge is cut into, far from the pole ...
        Real near     = 0.12;   // ... and, near it, this share of the distance to the pole
        Real t_roll = 2.4, t_turn = 2.6, t_unroll = 2.4;    // seconds
    };

    struct TurnResult
    {
        std::vector<Stage>     stages;      // roll up, turn, unroll
        std::vector<Curve3>    left;        // the drawing the turn leaves: (x, y, h), cut as the stages' curves are
        std::vector<Crossing3> crossings;   // the crossings, in the cut curves' indices
        std::vector<std::vector<int>> at;   // per curve: where each vertex that came in is in the cut curve
        Real radius = 0, angle = 0;
    };

    /*!@brief Build a turn that puts `pole` (a point off every curve, inside the
     * face that is to become the exterior) at infinity. The drawing left has
     * the size and the centre of the one that came in. */
    inline TurnResult Turn( const std::vector<Curve3> & curves, const std::vector<Crossing3> & crossings,
                            const P2 & pole, const TurnOptions & opt = TurnOptions() )
    {
        TurnResult out;

        // the drawing's box
        Real x0 = 1e300, y0 = 1e300, x1 = -1e300, y1 = -1e300;
        for( const Curve3 & c : curves ) { for( const P3 & p : c.pts ) { x0 = std::min(x0, p.x); y0 = std::min(y0, p.y); x1 = std::max(x1, p.x); y1 = std::max(y1, p.y); } }
        const P3 centre { (x0 + x1) / 2, (y0 + y1) / 2, 0 };
        Real reach = 0;
        for( const Curve3 & c : curves ) { for( const P3 & p : c.pts ) { reach = std::max(reach, std::hypot(p.x - centre.x, p.y - centre.y)); } }
        const Real R = (opt.radius > 0) ? opt.radius : 0.65 * reach;
        out.radius = R;

        // cut every edge into pieces short enough for its image to follow an arc
        std::vector<Curve3> dense (curves.size());
        std::vector<std::vector<int>> at (curves.size());      // old vertex -> new vertex
        for( std::size_t c = 0; c < curves.size(); ++c )
        {
            const auto & p = curves[c].pts;
            dense[c].color = curves[c].color;
            for( std::size_t i = 0; i < p.size(); ++i )
            {
                const P3 & a = p[i], & b = p[(i + 1) % p.size()];
                at[c].push_back(int(dense[c].pts.size()));
                const Real len = std::hypot(b.x - a.x, b.y - a.y);
                Real t = 0;
                while( true )
                {
                    dense[c].pts.push_back(P3{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t });
                    const P3 & q = dense[c].pts.back();
                    const Real step = std::min(opt.max_step, std::max(0.02, opt.near * std::hypot(q.x - pole.x, q.y - pole.y)));
                    // the last piece of an edge is never a sliver: stop while there is still a step and a half to go
                    if( len <= 0 || t + 1.5 * step / len >= 1 ) { break; }
                    t += step / len;
                }
            }
        }
        out.at = at;
        for( const Crossing3 & x : crossings ) { out.crossings.push_back(Crossing3{ x.a, at[std::size_t(x.a)][std::size_t(x.ia)], x.b, at[std::size_t(x.b)][std::size_t(x.ib)] }); }

        // the turn that takes the pole's point of the sphere to the top
        Stage roll;
        roll.map = Map3::Roll; roll.centre = centre; roll.radius = R;
        roll.curves = dense; roll.crossings = out.crossings;
        const P3 sp = StagePoint(roll, P3{ pole.x, pole.y, 0 }, 0, 1, 1, 0);
        const P3 a { sp.x - centre.x, sp.y - centre.y, sp.z - R }, b { 0, 0, R };
        P3 axis { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
        if( std::hypot(axis.x, std::hypot(axis.y, axis.z)) < 1e-12 ) { axis = P3{ 1, 0, 0 }; }
        const Real angle = std::acos(detail::Clamp((a.x * b.x + a.y * b.y + a.z * b.z) / (R * R), -1, 1));
        roll.axis = axis; out.angle = angle;

        // roll up, turn, unroll: the plane point under a point of the turned sphere
        auto moebius = [&]( const P3 & p )
        {
            const P3 X = StagePoint(roll, P3{ p.x, p.y, 0 }, angle, 1, 1, 0);
            const Real k = 2 * R / (2 * R - X.z);
            return P2{ centre.x + (X.x - centre.x) * k, centre.y + (X.y - centre.y) * k };
        };
        Stage unroll = roll;
        for( Curve3 & c : unroll.curves ) { for( P3 & p : c.pts ) { const P2 q = moebius(p); p.x = q.x; p.y = q.y; } }

        // the finish: a logarithmic radius about the image of infinity, then back to size
        {
            const P3 top = detail::Rotate(P3{ 0, 0, R }, axis, angle);       // the old top, turned (relative to the sphere's centre)
            const Real k = 2 * R / (R - top.z + 1e-300);
            unroll.fin_o = P2{ centre.x + top.x * k, centre.y + top.y * k };
            Real rmin = 1e300;
            for( const Curve3 & c : unroll.curves ) { for( const P3 & p : c.pts ) { rmin = std::min(rmin, std::hypot(p.x - unroll.fin_o.x, p.y - unroll.fin_o.y)); } }
            unroll.fin_rmin = 0.5 * rmin; unroll.fin_scale = 1; unroll.fin_shift = P2{ 0, 0 };
            Real u0 = 1e300, v0 = 1e300, u1 = -1e300, v1 = -1e300;
            for( const Curve3 & c : unroll.curves ) { for( const P3 & p : c.pts ) { const P2 z = Finish(unroll, P2{ p.x, p.y }, 1); u0 = std::min(u0, z.x); v0 = std::min(v0, z.y); u1 = std::max(u1, z.x); v1 = std::max(v1, z.y); } }
            const Real s = std::max(x1 - x0, y1 - y0) / std::max(std::max(u1 - u0, v1 - v0), 1e-300);
            unroll.fin_scale = s;
            unroll.fin_shift = P2{ (u0 + u1) / 2 - centre.x / s, (v0 + v1) / 2 - centre.y / s };
        }

        Stage up = roll;   up.duration = opt.t_roll;   up.roll0 = 0; up.roll1 = 1; up.fog0 = 0; up.fog1 = 1;
        Stage turn = roll; turn.duration = opt.t_turn; turn.roll0 = turn.roll1 = 1; turn.angle0 = 0; turn.angle1 = angle; turn.fog0 = turn.fog1 = 1;
        unroll.duration = opt.t_unroll; unroll.roll0 = 1; unroll.roll1 = 0; unroll.angle0 = unroll.angle1 = 0;
        unroll.fin0 = 0; unroll.fin1 = 1; unroll.fog0 = 1; unroll.fog1 = 0;

        out.left = unroll.curves;
        for( Curve3 & c : out.left ) { for( P3 & p : c.pts ) { const P2 z = Finish(unroll, P2{ p.x, p.y }, 1); p.x = z.x; p.y = z.y; } }
        out.stages = { std::move(up), std::move(turn), std::move(unroll) };
        return out;
    }

} // namespace Knoodle::Animate
