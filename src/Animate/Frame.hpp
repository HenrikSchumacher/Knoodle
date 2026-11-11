#pragma once

// FrameAt: what an animation looks like at a time, as a display list of
// stroked polylines and filled polygons in pixel coordinates (y down), in
// drawing order. This is the reference for the picture. Anything that can
// stroke and fill can show it; the crossing gaps are already cut.
//
// Standard library only.

#include "Scene.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace Knoodle::Animate
{
    struct Stroke
    {
        std::vector<P2> pts;
        RGBA color;
        Real width  = 1;
        bool round  = false;    // round joins and caps; else miter joins, butt caps
        bool closed = false;
    };

    struct Shape
    {
        std::vector<std::vector<P2>> rings;     // filled by the nonzero rule
        RGBA color;
    };

    struct Item
    {
        bool   fillQ = false;
        Stroke stroke;
        Shape  shape;
    };

    struct Frame
    {
        Real width = 0, height = 0;
        RGBA background;
        std::vector<Item> items;
        std::size_t scene = 0;
        std::string label, note;
        int crossings_before = 0, crossings_after = 0;
    };

    namespace detail
    {
        inline Real Lerp( Real a, Real b, Real u ) { return a + (b - a) * u; }
        inline Real Clamp( Real v, Real lo, Real hi ) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

        // Polylines here are P3: x and y in pixels, z a depth that rides along.
        // Lengths are measured in x and y only.
        inline Real Dist( const P3 & a, const P3 & b ) { return std::hypot(b.x - a.x, b.y - a.y); }

        /*!@brief Cumulative length at every vertex; for a closed polyline one
         * more entry, the whole length. */
        inline std::vector<Real> ArcLengths( const std::vector<P3> & p, bool closed )
        {
            std::vector<Real> cum (p.size() + (closed ? 1 : 0), 0.0);
            for( std::size_t i = 1; i < cum.size(); ++i ) { cum[i] = cum[i-1] + Dist(p[i-1], p[i % p.size()]); }
            return cum;
        }

        inline P3 PointAt( const std::vector<P3> & p, const std::vector<Real> & cum, Real s )
        {
            const std::size_t n = p.size();
            std::size_t j = std::size_t(std::upper_bound(cum.begin(), cum.end(), s) - cum.begin());
            if( j == 0 ) { return p[0]; }
            if( j >= cum.size() ) { return p[(cum.size() - 1) % n]; }
            const Real len = cum[j] - cum[j-1], t = (len > 0) ? (s - cum[j-1]) / len : 0;
            const P3 & a = p[j-1], & b = p[j % n];
            return P3{ Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t) };
        }

        /*!@brief The piece of a polyline from length s0 to s1. On a closed
         * polyline s1 may run past the whole length (once). */
        inline std::vector<P3> SubCurve( const std::vector<P3> & p, const std::vector<Real> & cum, bool closed, Real s0, Real s1 )
        {
            const Real L = cum.back();
            std::vector<P3> out;
            auto at = [&]( Real s ) { return PointAt(p, cum, (closed && s >= L) ? s - L : s); };
            out.push_back(at(s0));
            for( int lap = 0; lap < (closed ? 2 : 1); ++lap )
            {
                for( std::size_t j = 0; j < p.size(); ++j )
                {
                    const Real s = cum[j] + lap * L;
                    if( s > s0 && s < s1 ) { out.push_back(p[j]); }
                }
            }
            out.push_back(at(s1));
            return out;
        }

        /*!@brief What is left of [0, L] after the intervals `cuts` (centre,
         * half-length) are removed. On a closed polyline the answer may run
         * past L, and is empty-with-`whole` when nothing was cut. */
        inline std::vector<std::pair<Real,Real>> Kept( std::vector<std::pair<Real,Real>> cuts, Real L, bool closed, bool & whole )
        {
            std::vector<std::pair<Real,Real>> out;
            whole = cuts.empty();
            if( whole || L <= 0 ) { return out; }
            std::vector<std::pair<Real,Real>> iv;
            for( auto [c, h] : cuts )
            {
                Real a = c - h, b = c + h;
                if( closed ) { if( b - a >= L ) { return out; } a = std::fmod(a, L); if( a < 0 ) { a += L; } b = a + 2 * h; }
                else { a = std::max(a, 0.0); b = std::min(b, L); if( b <= a ) { continue; } }
                iv.emplace_back(a, b);
            }
            if( iv.empty() ) { whole = true; return out; }
            std::sort(iv.begin(), iv.end());
            std::vector<std::pair<Real,Real>> m;
            for( auto & v : iv ) { if( !m.empty() && v.first <= m.back().second ) { m.back().second = std::max(m.back().second, v.second); } else { m.push_back(v); } }
            if( closed )
            {
                while( m.size() > 1 && m.back().second >= m.front().first + L ) { m.front().first = m.back().first - L; m.pop_back(); }
                if( m.size() == 1 && m[0].second - m[0].first >= L ) { return out; }
                for( std::size_t i = 0; i < m.size(); ++i )
                {
                    const Real a = m[i].second, b = (i + 1 < m.size()) ? m[i+1].first : m[0].first + L;
                    if( b > a ) { out.emplace_back(a, b); }
                }
            }
            else
            {
                Real a = 0;
                for( auto & v : m ) { if( v.first > a ) { out.emplace_back(a, v.first); } a = v.second; }
                if( L > a ) { out.emplace_back(a, L); }
            }
            return out;
        }

        /*!@brief The unit direction of a polyline at vertex i. */
        inline P2 Tangent( const std::vector<P3> & p, bool closed, std::size_t i )
        {
            const std::size_t n = p.size();
            if( n < 2 ) { return P2{ 1, 0 }; }
            const P3 & a = p[(i > 0) ? i - 1 : (closed ? n - 1 : 0)];
            const P3 & b = p[(i + 1 < n) ? i + 1 : (closed ? 0 : n - 1)];
            const Real d = Dist(a, b);
            return (d > 0) ? P2{ (b.x - a.x) / d, (b.y - a.y) / d } : P2{ 1, 0 };
        }

        /*!@brief A gap is wider where the strands meet at a slant: divided by
         * |sin| of the angle between them, at most by 1 / 0.35. */
        inline Real Slant( const P2 & t, const P2 & u )
        {
            return 1.0 / std::max(0.35, std::abs(t.x * u.y - t.y * u.x));
        }

        struct View
        {
            Real cx, cy, s, ox, oy, H;
            P2 World ( Real x, Real y ) const { return P2{ ox + (x - cx) * s, oy - (y - cy) * s }; }
            P2 Screen( Real x, Real y ) const { return P2{ x * H, H - y * H }; }
        };

        inline View MakeView( const ViewBox & b0, const ViewBox & b1, Real u, Real W, Real H, Real margin )
        {
            const ViewBox b { Lerp(b0.x0, b1.x0, u), Lerp(b0.y0, b1.y0, u), Lerp(b0.x1, b1.x1, u), Lerp(b0.y1, b1.y1, u) };
            const Real bw = std::max(b.x1 - b.x0, 1e-9), bh = std::max(b.y1 - b.y0, 1e-9);
            const Real s = std::min(W * (1 - 2 * margin) / bw, H * (1 - 2 * margin) / bh);
            return View{ (b.x0 + b.x1) / 2, (b.y0 + b.y1) / 2, s, W / 2, H / 2, H };
        }

        inline void PushStroke( Frame & f, const std::vector<P3> & p, const RGBA & c, Real w, bool round, bool closed )
        {
            if( p.size() < 2 || c.a <= 0 ) { return; }
            Item it;
            it.stroke.color = c; it.stroke.width = w; it.stroke.round = round; it.stroke.closed = closed;
            it.stroke.pts.reserve(p.size());
            for( const P3 & q : p ) { it.stroke.pts.push_back(P2{ q.x, q.y }); }
            f.items.push_back(std::move(it));
        }

        //--------------------------------------------------------------
        // Flat scenes
        //--------------------------------------------------------------

        inline void FlatFrame( const Scene & sc, Real local, const Style & st, Frame & f )
        {
            if( sc.keys.empty() ) { return; }
            const Real ulin = (sc.duration > 0) ? Clamp(local / sc.duration, 0, 1) : 1;
            const Real u    = (sc.easing == Easing::Smoother) ? Smoother(ulin) : ulin;
            const std::size_t K = sc.keys.size(), np = sc.paths.size();
            const Real pos = u * Real(K - 1);
            const std::size_t i0 = std::min(std::size_t(pos), K - 1), i1 = std::min(i0 + 1, K - 1);
            Real w = pos - Real(i0);
            bool lerpQ = (i1 != i0);
            for( std::size_t j = 0; lerpQ && j < np; ++j ) { lerpQ = (sc.keys[i0].pts[j].size() == sc.keys[i1].pts[j].size()); }
            if( !lerpQ ) { w = (w < 0.5 || i1 == i0) ? 0 : 1; }
            const Key & ka = sc.keys[i0], & kb = sc.keys[i1], & kc = (w < 0.5) ? ka : kb;

            const View V = MakeView(sc.view0, sc.view1, Smoother(ulin), f.width, f.height, st.margin);
            const Real wpx = std::max(st.stroke * V.s, st.min_stroke * f.height);

            // every path in pixels
            std::vector<std::vector<P3>> px (np);
            for( std::size_t j = 0; j < np; ++j )
            {
                const Path & P = sc.paths[j];
                auto map = [&]( const P2 & q, std::size_t key ) -> P2
                {
                    if( P.space == Space::World )  { return V.World(q.x, q.y); }
                    if( P.space == Space::Screen ) { return V.Screen(q.x, q.y); }
                    const Real g = (K > 1) ? Real(key) / Real(K - 1) : 1;
                    const P2 a = V.World(q.x, q.y), b = V.Screen(q.x, q.y);
                    return P2{ Lerp(a.x, b.x, g), Lerp(a.y, b.y, g) };
                };
                const auto & A = ka.pts[j], & B = kb.pts[j];
                if( w <= 0 || w >= 1 )
                {
                    const auto & S = (w <= 0) ? A : B;
                    for( const P2 & q : S ) { const P2 z = map(q, (w <= 0) ? i0 : i1); px[j].push_back(P3{ z.x, z.y, 0 }); }
                }
                else
                {
                    for( std::size_t i = 0; i < A.size(); ++i )
                    {
                        const P2 a = map(A[i], i0), b = map(B[i], i1);
                        px[j].push_back(P3{ Lerp(a.x, b.x, w), Lerp(a.y, b.y, w), 0 });
                    }
                }
            }

            // in layer order; a fill before a path of its layer
            struct Draw { int layer; int kind; std::size_t index; };
            std::vector<Draw> order;
            for( std::size_t i = 0; i < sc.fills.size(); ++i ) { order.push_back(Draw{ sc.fills[i].layer, 0, i }); }
            for( std::size_t j = 0; j < np; ++j )              { order.push_back(Draw{ sc.paths[j].layer, 1, j }); }
            std::stable_sort(order.begin(), order.end(), []( const Draw & a, const Draw & b )
                             { return (a.layer != b.layer) ? (a.layer < b.layer) : (a.kind < b.kind); });

            for( const Draw & d : order )
            {
                if( d.kind == 0 )
                {
                    const Fill & F = sc.fills[d.index];
                    Item it; it.fillQ = true; it.shape.color = st.Of(F.role, -1);
                    std::vector<P2> ring;
                    for( auto [j, rev] : F.parts )
                    {
                        const auto & p = px[std::size_t(j)];
                        for( std::size_t i = 0; i < p.size(); ++i ) { const P3 & q = p[rev ? p.size() - 1 - i : i]; ring.push_back(P2{ q.x, q.y }); }
                    }
                    if( ring.size() >= 3 ) { it.shape.rings.push_back(std::move(ring)); f.items.push_back(std::move(it)); }
                    continue;
                }
                const std::size_t j = d.index;
                const Path & P = sc.paths[j];
                const auto & p = px[j];
                if( p.size() < 2 ) { continue; }
                RGBA col = st.Of(P.role, P.color);
                col.a *= Lerp(P.alpha0, P.alpha1, u);
                // on the screen (the tray) a stroke is thin whatever the camera does; a spin-off thins as it goes there
                const Real onscreen = (P.space == Space::World) ? 0 : ((P.space == Space::Screen || K < 2) ? 1 : (Real(i0) + w) / Real(K - 1));
                const Real width = Lerp(wpx * ((P.role == Role::Route) ? st.route_width : 1.0), st.token_stroke * f.height, onscreen);
                if( P.role == Role::Token && p.size() >= 3 )
                {
                    Item it; it.fillQ = true; it.shape.color = col; it.shape.color.a *= Lerp(0.35, 0.85, onscreen);
                    std::vector<P2> ring; for( const P3 & q : p ) { ring.push_back(P2{ q.x, q.y }); }
                    it.shape.rings.push_back(std::move(ring)); f.items.push_back(std::move(it));
                }
                const std::vector<Real> cum = ArcLengths(p, P.closed);
                std::vector<std::pair<Real,Real>> cuts;
                for( const Cut & c : kc.cuts )
                {
                    if( std::size_t(c.under) != j || std::size_t(c.iu) >= p.size() ) { continue; }
                    const auto & o = px[std::size_t(c.over)];
                    if( std::size_t(c.io) >= o.size() ) { continue; }
                    const Real k = Slant(Tangent(p, P.closed, std::size_t(c.iu)), Tangent(o, sc.paths[std::size_t(c.over)].closed, std::size_t(c.io)));
                    cuts.emplace_back(cum[std::size_t(c.iu)], st.gap * wpx * k);
                }
                bool whole = false;
                const auto kept = Kept(cuts, cum.back(), P.closed, whole);
                if( whole ) { PushStroke(f, p, col, width, P.round, P.closed); continue; }
                for( auto [a, b] : kept ) { PushStroke(f, SubCurve(p, cum, P.closed, a, b), col, width, P.round, false); }
            }
        }

        //--------------------------------------------------------------
        // Space scenes
        //--------------------------------------------------------------

        inline P3 Rotate( const P3 & v, const P3 & axis, Real angle )
        {
            const Real n = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
            if( n <= 0 || angle == 0 ) { return v; }
            const P3 k { axis.x / n, axis.y / n, axis.z / n };
            const Real c = std::cos(angle), s = std::sin(angle), kv = k.x * v.x + k.y * v.y + k.z * v.z;
            const P3 cr { k.y * v.z - k.z * v.y, k.z * v.x - k.x * v.z, k.x * v.y - k.y * v.x };
            return P3{ v.x * c + cr.x * s + k.x * kv * (1 - c), v.y * c + cr.y * s + k.y * kv * (1 - c), v.z * c + cr.z * s + k.z * kv * (1 - c) };
        }
    } // namespace detail

    /*!@brief `Stage::Finish` (see Scene.hpp): the map that evens out the
     * drawing a turn leaves, blended in by `fin` in [0, 1]. */
    inline P2 Finish( const Stage & s, const P2 & q, Real fin )
    {
        if( fin <= 0 ) { return q; }
        const Real dx = q.x - s.fin_o.x, dy = q.y - s.fin_o.y, r = std::hypot(dx, dy);
        const Real fr = (s.fin_rmin <= 0 || r <= s.fin_rmin) ? r : s.fin_rmin * (1 + std::log(r / s.fin_rmin));
        const Real g = (r > 0) ? (1 - fin) + fin * fr / r : 1;
        const P2 z { s.fin_o.x + dx * g, s.fin_o.y + dy * g };
        return P2{ (1 - fin) * z.x + fin * s.fin_scale * (z.x - s.fin_shift.x), (1 - fin) * z.y + fin * s.fin_scale * (z.y - s.fin_shift.y) };
    }

    /*!@brief Where a stage puts a point of one of its curves, for given values
     * of its parameters. The reference for both maps.
     *
     * Under `Roll` the height h comes in with the roll: none of it while the
     * plane is flat, all of it from a quarter rolled. So a flat frame is the
     * flat drawing exactly, and hands over to a flat scene with no jump.
     * `full_height` gives the point with all of h whatever the roll; the
     * renderer uses it to decide which passage of a crossing is the farther. */
    inline P3 StagePoint( const Stage & s, const P3 & p, Real angle, Real lift, Real roll, Real fin, bool full_height = false )
    {
        if( s.map == Map3::Affine )
        {
            const P3 w = detail::Rotate(P3{ p.x - s.centre.x, p.y - s.centre.y, lift * (p.z - s.centre.z) }, s.axis, angle);
            return P3{ s.centre.x + w.x, s.centre.y + w.y, s.centre.z + w.z };
        }
        const P2 q = Finish(s, P2{ p.x, p.y }, fin);
        const Real x = q.x - s.centre.x, y = q.y - s.centre.y;
        const Real h = p.z * (full_height ? 1.0 : std::min(1.0, 4 * roll));
        if( roll < 1e-9 ) { return P3{ q.x, q.y, h }; }
        const Real R = s.radius / roll, t = 4 * R * R / (x * x + y * y + 4 * R * R);
        const P3 S { t * x, t * y, 2 * R * (1 - t) };
        // h along the inward normal, then the turn about the sphere's centre (0, 0, R)
        const P3 X { S.x - h * S.x / R, S.y - h * S.y / R, S.z + h * (R - S.z) / R };
        const P3 w = detail::Rotate(P3{ X.x, X.y, X.z - R }, s.axis, angle);
        return P3{ s.centre.x + w.x, s.centre.y + w.y, w.z + R };
    }

    namespace detail
    {
        inline void SpaceFrame( const Scene & sc, Real local, const Style & st, Frame & f )
        {
            if( sc.stages.empty() ) { return; }
            std::size_t k = 0; Real tl = local;
            while( k + 1 < sc.stages.size() && tl >= sc.stages[k].duration ) { tl -= sc.stages[k].duration; ++k; }
            const Stage & S = sc.stages[k];
            const Real u     = Smoother((S.duration > 0) ? tl / S.duration : 1);
            const Real angle = Lerp(S.angle0, S.angle1, u), lift = Lerp(S.lift0, S.lift1, u);
            const Real roll  = Lerp(S.roll0, S.roll1, u),   fin  = Lerp(S.fin0, S.fin1, u), fog = Lerp(S.fog0, S.fog1, u);
            const Real ulin  = (sc.duration > 0) ? Clamp(local / sc.duration, 0, 1) : 1;
            const View V = MakeView(sc.view0, sc.view1, Smoother(ulin), f.width, f.height, st.margin);
            const Real wpx = std::max(st.stroke * V.s, st.min_stroke * f.height);

            // every curve in pixels, its depth riding along
            const std::size_t nc = S.curves.size();
            std::vector<std::vector<P3>> px (nc);
            Real dmin = 1e300, dmax = -1e300;
            for( std::size_t c = 0; c < nc; ++c )
            {
                for( const P3 & p : S.curves[c].pts )
                {
                    const P3 X = StagePoint(S, p, angle, lift, roll, fin);
                    const P2 z = V.World(X.x - sc.tilt_a * X.z, X.y - sc.tilt_b * X.z);
                    px[c].push_back(P3{ z.x, z.y, X.z });
                    dmin = std::min(dmin, X.z); dmax = std::max(dmax, X.z);
                }
            }
            std::vector<std::vector<Real>> cum (nc);
            for( std::size_t c = 0; c < nc; ++c ) { cum[c] = ArcLengths(px[c], true); }

            // the gaps: the farther strand is cut where two meet
            std::vector<std::vector<std::pair<Real,Real>>> cuts (nc);
            if( S.map == Map3::Roll )
            {
                for( const Crossing3 & x : S.crossings )
                {
                    const std::size_t a = std::size_t(x.a), ia = std::size_t(x.ia), b = std::size_t(x.b), ib = std::size_t(x.ib);
                    if( a >= nc || b >= nc || ia >= px[a].size() || ib >= px[b].size() ) { continue; }
                    const Real kk = Slant(Tangent(px[a], true, ia), Tangent(px[b], true, ib));
                    const Real za = StagePoint(S, S.curves[a].pts[ia], angle, lift, roll, fin, true).z;
                    const Real zb = StagePoint(S, S.curves[b].pts[ib], angle, lift, roll, fin, true).z;
                    if( za < zb ) { cuts[a].emplace_back(cum[a][ia], st.gap * wpx * kk); }
                    else                            { cuts[b].emplace_back(cum[b][ib], st.gap * wpx * kk); }
                }
            }
            else
            {
                struct Seg { Real x0, x1, y0, y1; std::size_t c, i; };
                std::vector<Seg> segs;
                for( std::size_t c = 0; c < nc; ++c )
                {
                    const std::size_t n = px[c].size();
                    for( std::size_t i = 0; i < n; ++i )
                    {
                        const P3 & a = px[c][i], & b = px[c][(i + 1) % n];
                        segs.push_back(Seg{ std::min(a.x, b.x), std::max(a.x, b.x), std::min(a.y, b.y), std::max(a.y, b.y), c, i });
                    }
                }
                std::sort(segs.begin(), segs.end(), []( const Seg & a, const Seg & b ) { return a.x0 < b.x0; });
                for( std::size_t m = 0; m < segs.size(); ++m )
                {
                    for( std::size_t l = m + 1; l < segs.size() && segs[l].x0 <= segs[m].x1; ++l )
                    {
                        const Seg & s = segs[m], & t = segs[l];
                        if( t.y0 > s.y1 || t.y1 < s.y0 ) { continue; }
                        const std::size_t ns = px[s.c].size(), nt = px[t.c].size();
                        if( s.c == t.c && (s.i == t.i || (s.i + 1) % ns == t.i || (t.i + 1) % nt == s.i) ) { continue; }
                        const P3 & a = px[s.c][s.i], & b = px[s.c][(s.i + 1) % ns], & c = px[t.c][t.i], & d = px[t.c][(t.i + 1) % nt];
                        const Real rx = b.x - a.x, ry = b.y - a.y, sx = d.x - c.x, sy = d.y - c.y, den = rx * sy - ry * sx;
                        if( den == 0 ) { continue; }
                        const Real ta = ((c.x - a.x) * sy - (c.y - a.y) * sx) / den, tc = ((c.x - a.x) * ry - (c.y - a.y) * rx) / den;
                        if( ta <= 0 || ta >= 1 || tc <= 0 || tc >= 1 ) { continue; }
                        const Real la = std::hypot(rx, ry), lc = std::hypot(sx, sy);
                        const Real kk = 1.0 / std::max(0.35, std::abs(den) / std::max(la * lc, 1e-300));
                        if( Lerp(a.z, b.z, ta) < Lerp(c.z, d.z, tc) ) { cuts[s.c].emplace_back(cum[s.c][s.i] + ta * la, st.gap * wpx * kk); }
                        else                                          { cuts[t.c].emplace_back(cum[t.c][t.i] + tc * lc, st.gap * wpx * kk); }
                    }
                }
            }

            // under the curves: the ball, faintly, and the marks
            auto shape = [&]( std::vector<std::vector<P2>> rings, RGBA c, Real alpha )
            {
                c.a *= alpha;
                if( c.a <= 0 ) { return; }
                Item it; it.fillQ = true; it.shape.rings = std::move(rings); it.shape.color = c;
                f.items.push_back(std::move(it));
            };
            auto outline = [&]( const std::vector<P2> & ring, RGBA c, Real alpha, Real width )
            {
                c.a *= alpha;
                if( c.a <= 0 || ring.size() < 2 ) { return; }
                Item it; it.stroke.pts = ring; it.stroke.color = c; it.stroke.width = width; it.stroke.round = true; it.stroke.closed = true;
                f.items.push_back(std::move(it));
            };
            auto seen = [&]( const P3 & X ) { return V.World(X.x - sc.tilt_a * X.z, X.y - sc.tilt_b * X.z); };
            if( S.map == Map3::Roll && roll > 0.3 && fog > 0 )
            {
                // the sphere's outline as the camera sees it: the great circle square to the line of sight
                const Real R = S.radius / roll, vl = std::sqrt(sc.tilt_a * sc.tilt_a + sc.tilt_b * sc.tilt_b + 1);
                const P3 v { sc.tilt_a / vl, sc.tilt_b / vl, 1 / vl };
                const Real el = std::hypot(v.z, v.y);
                const P3 e1 { 0, v.z / el, -v.y / el }, e2 { v.y * e1.z - v.z * e1.y, v.z * e1.x - v.x * e1.z, v.x * e1.y - v.y * e1.x };
                std::vector<P2> ring;
                for( int i = 0; i < 96; ++i )
                {
                    const Real a = 6.283185307179586 * i / 96, c = std::cos(a), s = std::sin(a);
                    ring.push_back(seen(P3{ S.centre.x + R * (c * e1.x + s * e2.x), S.centre.y + R * (c * e1.y + s * e2.y), R + R * (c * e1.z + s * e2.z) }));
                }
                const Real show = fog * Clamp((roll - 0.3) / 0.4, 0, 1);
                shape({ ring }, st.fog, 0.30 * show);
                outline(ring, st.fog, show, 0.6 * wpx);
            }
            {
                const Real m_in = Lerp(S.mark_in0, S.mark_in1, u), m_out = Lerp(S.mark_out0, S.mark_out1, u), m_line = Lerp(S.mark_line0, S.mark_line1, u);
                for( const Curve3 & m : S.marks )
                {
                    std::vector<P2> ring;
                    for( const P3 & p : m.pts ) { ring.push_back(seen(StagePoint(S, p, angle, lift, roll, fin))); }
                    if( ring.size() < 3 ) { continue; }
                    shape({ ring }, st.disk, m_in);
                    if( m_out > 0 )
                    {
                        // the whole picture but the ring: the nonzero rule wants the ring to run against the frame
                        Real area = 0;
                        for( std::size_t i = 0; i < ring.size(); ++i ) { const P2 & a = ring[i], & b = ring[(i + 1) % ring.size()]; area += a.x * b.y - b.x * a.y; }
                        std::vector<P2> hole = ring;
                        if( area > 0 ) { std::reverse(hole.begin(), hole.end()); }
                        shape({ { P2{ 0, 0 }, P2{ f.width, 0 }, P2{ f.width, f.height }, P2{ 0, f.height } }, hole }, st.disk, m_out);
                    }
                    outline(ring, st.route, m_line, 2.4 * wpx);
                }
            }

            // runs of one depth level, far to near
            constexpr int LEVELS = 24;
            const Real span = std::max(dmax - dmin, 1e-12), dmid = (dmin + dmax) / 2;
            struct Run { int band; std::vector<P3> pts; RGBA color; Real width; };
            std::vector<Run> runs;
            for( std::size_t c = 0; c < nc; ++c )
            {
                if( px[c].size() < 2 ) { continue; }
                bool whole = false;
                auto kept = Kept(cuts[c], cum[c].back(), true, whole);
                if( whole ) { kept.assign(1, { 0.0, cum[c].back() }); }
                const RGBA base = st.Of(Role::Strand, S.curves[c].color);
                for( auto [a, b] : kept )
                {
                    const std::vector<P3> piece = SubCurve(px[c], cum[c], true, a, b);
                    int band = -1;
                    for( std::size_t i = 0; i + 1 < piece.size(); ++i )
                    {
                        const Real d = (piece[i].z + piece[i+1].z) / 2;
                        const int  nb = (fog > 0) ? std::min(LEVELS - 1, int(Real(LEVELS) * (d - dmin) / span)) : 0;
                        if( nb != band )
                        {
                            const Real far = Clamp((dmax - d) / (2 * std::max(sc.depth, 1e-12)), 0, 1);
                            Run r; r.band = nb; r.pts.push_back(piece[i]);
                            r.color = Mix(base, RGBA{ st.fog.r, st.fog.g, st.fog.b, base.a }, st.fog_depth * fog * far);
                            r.width = wpx * (1 + st.fog_width * fog * Clamp((d - dmid) / std::max(sc.depth, 1e-12), -1, 1));
                            runs.push_back(std::move(r)); band = nb;
                        }
                        runs.back().pts.push_back(piece[i+1]);
                    }
                }
            }
            std::stable_sort(runs.begin(), runs.end(), []( const Run & a, const Run & b ) { return a.band < b.band; });
            for( const Run & r : runs ) { PushStroke(f, r.pts, r.color, r.width, true, false); }
        }
    } // namespace detail

    /*!@brief The picture at `seconds`, for a canvas `width` by `height`
     * pixels. Past the end it is the last picture. */
    inline Frame FrameAt( const Animation & A, Real seconds, const Style & style, Real width, Real height )
    {
        Frame f;
        f.width = width; f.height = height; f.background = style.background;
        Real local = 0;
        if( !A.Locate(seconds, f.scene, local) ) { return f; }
        const Scene & sc = A.scenes[f.scene];
        f.label = sc.label; f.note = sc.note;
        f.crossings_before = sc.crossings_before; f.crossings_after = sc.crossings_after;
        if( sc.kind == Scene::Kind::Flat ) { detail::FlatFrame(sc, local, style, f); }
        else                               { detail::SpaceFrame(sc, local, style, f); }
        return f;
    }

} // namespace Knoodle::Animate
