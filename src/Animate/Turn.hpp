#pragma once

// A turn of the sphere: the animated change of exterior face
// (docs/animate-design.md). The plane rolls up into a sphere; the drawing on
// it is balanced, by a Moebius map of the sphere to itself, so that its faces
// are as nearly of a size as such a map can make them; and the sphere turns
// until a chosen point of the new exterior face is at its top. This file
// builds those stages and the drawing the turned sphere gives when it is
// seen from the top again (its stereographic projection): a Moebius image of
// the drawing that came in, with the new face outside.
//
// The Moebius maps are the shifts (Frame.hpp, `Shift`): one for each point w
// inside the ball, moving the sphere's points toward w. With the rotations
// they are all the Moebius maps of the sphere.
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
        Real t_roll = 2.4, t_balance = 1.6, t_turn = 2.6;   // seconds
        Real t_show = 0.7;                                  // with marks: before, to point the face out
    };

    struct TurnResult
    {
        std::vector<Stage>     stages;      // (point out,) roll up, balance, turn
        std::vector<Curve3>    left;        // the drawing the turned sphere projects to: (x, y, h), cut as the stages' curves are
        std::vector<Crossing3> crossings;   // the crossings, in the cut curves' indices
        std::vector<std::vector<int>> at;   // per curve: where each vertex that came in is in the cut curve
        Stage sphere;                       // a stage that only lifts a drawing to the sphere, as it stands: roll 1, nothing else
        Real radius = 0, angle = 0;
        P3   shift;                         // the balancing map's point of the ball
        Real spread0 = 0, spread1 = 0;      // spread of the logarithms of the face areas, before and after balancing
    };

    /*!@brief Build a turn that puts `pole` (a point off every curve, inside the
     * face that is to become the exterior) at the top of the sphere. `faces`,
     * if given, are the boundaries of all the drawing's faces, each with its
     * face on its left: the sphere is balanced to even their areas. `marks`
     * are carried along and pointed out. */
    inline TurnResult Turn( const std::vector<Curve3> & curves, const std::vector<Crossing3> & crossings, const P2 & pole,
                            const TurnOptions & opt = TurnOptions(), const std::vector<Curve3> & marks = {}, const std::vector<Curve3> & faces = {} )
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
        std::vector<Curve3> dense (curves.size()), dense_marks (marks.size()), dense_faces (faces.size());
        std::vector<std::vector<int>> at (curves.size());      // old vertex -> new vertex
        auto cut = [&]( const Curve3 & in, Curve3 & out_c, std::vector<int> * where )
        {
            const auto & p = in.pts;
            out_c.color = in.color;
            for( std::size_t i = 0; i < p.size(); ++i )
            {
                const P3 & a = p[i], & b = p[(i + 1) % p.size()];
                if( where ) { where->push_back(int(out_c.pts.size())); }
                const Real len = std::hypot(b.x - a.x, b.y - a.y);
                Real t = 0;
                while( true )
                {
                    out_c.pts.push_back(P3{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t });
                    const P3 & q = out_c.pts.back();
                    const Real step = std::min(opt.max_step, std::max(0.02, opt.near * std::hypot(q.x - pole.x, q.y - pole.y)));
                    // the last piece of an edge is never a sliver: stop while there is still a step and a half to go
                    if( len <= 0 || t + 1.5 * step / len >= 1 ) { break; }
                    t += step / len;
                }
            }
        };
        for( std::size_t c = 0; c < curves.size(); ++c ) { cut(curves[c], dense[c], &at[c]); }
        for( std::size_t c = 0; c < marks.size(); ++c ) { cut(marks[c], dense_marks[c], nullptr); }
        for( std::size_t c = 0; c < faces.size(); ++c ) { cut(faces[c], dense_faces[c], nullptr); }
        out.at = at;
        for( const Crossing3 & x : crossings ) { out.crossings.push_back(Crossing3{ x.a, at[std::size_t(x.a)][std::size_t(x.ia)], x.b, at[std::size_t(x.b)][std::size_t(x.ib)] }); }

        Stage roll;
        roll.map = Map3::Roll; roll.centre = centre; roll.radius = R;
        roll.curves = dense; roll.crossings = out.crossings; roll.marks = dense_marks;
        out.sphere = roll; out.sphere.roll0 = out.sphere.roll1 = 1; out.sphere.fog0 = out.sphere.fog1 = 1;

        // a point of the plane, as a unit vector from the centre of the rolled-up sphere
        auto unit = [&]( Real x, Real y )
        {
            const P3 X = StagePoint(roll, P3{ x, y, 0 }, 0, 1, 1, 0);
            return P3{ (X.x - centre.x) / R, (X.y - centre.y) / R, (X.z - R) / R };
        };

        // The balance: the shift that makes the faces' areas most nearly equal as ratios, which is to
        // say the one that makes the spread (standard deviation) of the logarithms of their areas
        // least. (Largest less smallest would only shrink the largest face: among many faces the
        // smallest is next to nothing whatever is done. Largest over smallest is not smooth, and a
        // search from the identity stops where it starts.) Found from the identity by trying the six
        // axis directions and halving the stride when none gains.
        P3 w { 0, 0, 0 };
        if( faces.size() >= 2 )
        {
            const Real four_pi = 12.566370614359172;
            std::vector<std::vector<P3>> U (dense_faces.size());
            for( std::size_t f = 0; f < dense_faces.size(); ++f ) { for( const P3 & p : dense_faces[f].pts ) { U[f].push_back(unit(p.x, p.y)); } }
            // the solid angle on a loop's left, up to sign and a multiple of 4 pi: a fan of triangles from its first point
            auto fan = [&]( const std::vector<P3> & loop, const P3 & v )
            {
                if( loop.size() < 3 ) { return Real(0); }
                const P3 a = Shift(v, loop[0]); P3 b = Shift(v, loop[1]); Real om = 0;
                for( std::size_t i = 2; i < loop.size(); ++i )
                {
                    const P3 c = Shift(v, loop[i]);
                    const Real det = a.x * (b.y * c.z - b.z * c.y) - a.y * (b.x * c.z - b.z * c.x) + a.z * (b.x * c.y - b.y * c.x);
                    om += 2 * std::atan2(det, 1 + (a.x * b.x + a.y * b.y + a.z * b.z) + (b.x * c.x + b.y * c.y + b.z * c.z) + (c.x * a.x + c.y * a.y + c.z * a.z));
                    b = c;
                }
                return om;
            };
            auto areas = [&]( const P3 & v, Real sign, Real & total )
            {
                Real sum = 0, sum2 = 0; total = 0;
                for( const auto & loop : U )
                {
                    Real a = std::fmod(sign * fan(loop, v), four_pi);
                    if( a <= 0 ) { a += four_pi; }
                    const Real l = std::log(a);
                    sum += l; sum2 += l * l; total += a;
                }
                const Real n = static_cast<Real>(U.size());
                return std::sqrt(std::max(sum2 / n - (sum / n) * (sum / n), Real(0)));
            };
            // the sign for which the faces cover the sphere once
            Real tp = 0, tm = 0;
            const Real sp = areas(w, 1, tp), sm = areas(w, -1, tm);
            const Real sign = (std::abs(tp - four_pi) <= std::abs(tm - four_pi)) ? 1 : -1;
            Real total = 0, best = (sign > 0) ? sp : sm;
            out.spread0 = best;
            const P3 dirs[6] { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
            for( Real stride = 0.25; stride > 1e-3; )
            {
                bool gainQ = false;
                for( const P3 & d : dirs )
                {
                    const P3 q { w.x + stride * d.x, w.y + stride * d.y, w.z + stride * d.z };
                    if( q.x * q.x + q.y * q.y + q.z * q.z >= 0.97 * 0.97 ) { continue; }
                    const Real s = areas(q, sign, total);
                    // (the areas must still add up to the sphere: if not, one of them has been read a turn out)
                    if( std::abs(total - four_pi) < 1e-3 * four_pi && s < best - 1e-12 ) { best = s; w = q; gainQ = true; }
                }
                if( !gainQ ) { stride /= 2; }
            }
            out.spread1 = best;
        }
        out.shift = w; roll.mob = w;

        // the turn that takes the pole's point of the balanced sphere to the top
        const P3 a = Shift(w, unit(pole.x, pole.y)), b { 0, 0, 1 };
        P3 axis { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
        if( std::hypot(axis.x, std::hypot(axis.y, axis.z)) < 1e-12 ) { axis = P3{ 1, 0, 0 }; }
        const Real angle = std::acos(detail::Clamp(a.z, -1, 1));
        roll.axis = axis; out.angle = angle;

        // seen from the top again: the plane point under each point of the turned sphere
        out.left = dense;
        for( Curve3 & c : out.left )
        {
            for( P3 & p : c.pts )
            {
                const P3 X = StagePoint(roll, P3{ p.x, p.y, 0 }, angle, 1, 1, 1);
                const Real k = 2 * R / (2 * R - X.z);
                p.x = centre.x + (X.x - centre.x) * k; p.y = centre.y + (X.y - centre.y) * k;
            }
        }

        Stage up = roll;      up.duration = opt.t_roll;         up.roll0 = 0; up.roll1 = 1; up.fog0 = 0; up.fog1 = 1;
        Stage balance = roll; balance.duration = opt.t_balance; balance.roll0 = balance.roll1 = 1; balance.mob0 = 0; balance.mob1 = 1; balance.fog0 = balance.fog1 = 1;
        Stage turn = roll;    turn.duration = opt.t_turn;       turn.roll0 = turn.roll1 = 1; turn.mob0 = turn.mob1 = 1; turn.angle0 = 0; turn.angle1 = angle; turn.fog0 = turn.fog1 = 1;
        if( marks.empty() ) { out.stages = { std::move(up), std::move(balance), std::move(turn) }; return out; }

        // With a mark (the face that is to be the outside): it is pointed out on the flat drawing, stays
        // filled while the drawing rolls up and is balanced, and fades as the sphere turns it to the top.
        // Its outline is there throughout.
        Stage show = up; show.duration = opt.t_show; show.roll0 = show.roll1 = 0; show.fog0 = show.fog1 = 0;
        show.mark_in1 = 1; show.mark_line1 = 1;
        up.mark_in0 = up.mark_in1 = 1; up.mark_line0 = up.mark_line1 = 1;
        balance.mark_in0 = balance.mark_in1 = 1; balance.mark_line0 = balance.mark_line1 = 1;
        turn.mark_in0 = 1; turn.mark_in1 = 0; turn.mark_line0 = turn.mark_line1 = 1;
        out.stages = { std::move(show), std::move(up), std::move(balance), std::move(turn) };
        return out;
    }

} // namespace Knoodle::Animate
