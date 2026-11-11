#pragma once

// PlaneDrawing -- the animator's one drawing type (docs/animate-design.md).
//
// A diagram drawn with floating-point polylines: one polyline per arc, from
// its tail crossing's point to its head crossing's point; the over strand at
// each crossing; the link components in order; and the exterior face, named
// by a directed arc that has it on its left. Arrays are indexed by the
// diagram's own crossing and arc labels; unused labels are empty slots.
//
// Also here: the polyline kit (arc length, sub-curve, intersection, inside,
// box), the export from an OrthoDraw layout, and Match, which puts two
// drawings of one diagram point for point by normalised arc length.
//
// No file IO, no printing, no globals. Include after Knoodle.hpp.

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace Knoodle::Animate
{
    using Real     = double;
    using Idx      = std::size_t;
    using Point    = std::array<Real,2>;
    using Polyline = std::vector<Point>;

    inline constexpr Idx None = static_cast<Idx>(-1);

    //==========================================================================
    // The polyline kit.
    //==========================================================================

    /*!@brief Twice the signed area of the triangle abc (positive: counterclockwise). */
    inline Real Cross( const Point & a, const Point & b, const Point & c )
    {
        return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    }

    /*!@brief The sign of Cross(a,b,c), with the two products' rounding errors
     * put back (std::fma). This is Tools' DetSign2D_Corrected (Oriented2D.hpp),
     * written out because that template does not compile from Knoodle.hpp: the
     * TwoMulFMA it calls is declared in Interval.hpp, which is not included. */
    inline int Side( const Point & a, const Point & b, const Point & c )
    {
        const Real u = b[0] - a[0], v = c[1] - a[1], w = b[1] - a[1], x = c[0] - a[0];
        const Real p = u * v, q = w * x;
        const Real s = (p - q) + (std::fma(u, v, -p) - std::fma(w, x, -q));
        return (s > 0) - (s < 0);
    }

    inline Real Dist( const Point & a, const Point & b ) { return std::hypot(b[0] - a[0], b[1] - a[1]); }

    inline Point Lerp( const Point & a, const Point & b, Real t )
    {
        return { a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]) };
    }

    inline Real Length( const Polyline & L )
    {
        Real s = 0;
        for( Idx i = 1; i < L.size(); ++i ) { s += Dist(L[i-1], L[i]); }
        return s;
    }

    /*!@brief The normalised arc-length parameter of each point of L (0 to 1). */
    inline std::vector<Real> Params( const Polyline & L )
    {
        std::vector<Real> s (L.size(), 0.0);
        for( Idx i = 1; i < L.size(); ++i ) { s[i] = s[i-1] + Dist(L[i-1], L[i]); }
        const Real total = s.empty() ? 0.0 : s.back();
        if( total > 0 ) { for( Real & x : s ) { x /= total; } }
        return s;
    }

    /*!@brief The point of L at parameter t, given s = Params(L). */
    inline Point At( const Polyline & L, const std::vector<Real> & s, Real t )
    {
        if( t <= 0 ) { return L.front(); }
        if( t >= 1 ) { return L.back(); }
        const Idx k = static_cast<Idx>(std::upper_bound(s.begin(), s.end(), t) - s.begin());
        const Idx i = std::min(std::max<Idx>(k, 1), L.size() - 1);
        const Real d = s[i] - s[i-1];
        return Lerp(L[i-1], L[i], (d > 0) ? (t - s[i-1]) / d : 0.0);
    }

    /*!@brief The piece of L from parameter t0 to t1 (t0 <= t1), with L's corners between. */
    inline Polyline SubCurve( const Polyline & L, Real t0, Real t1 )
    {
        const std::vector<Real> s = Params(L);
        Polyline out { At(L, s, t0) };
        for( Idx i = 0; i < L.size(); ++i ) { if( s[i] > t0 && s[i] < t1 ) { out.push_back(L[i]); } }
        out.push_back(At(L, s, t1));
        return out;
    }

    /*!@brief L without repeated points and without points on the line of their neighbours. */
    inline Polyline Corners( const Polyline & L )
    {
        Polyline out;
        for( const Point & p : L )
        {
            if( !out.empty() && out.back() == p ) { continue; }
            while( out.size() >= 2 && Side(out[out.size()-2], out.back(), p) == 0
                   && (out.back()[0] - out[out.size()-2][0]) * (p[0] - out.back()[0])
                    + (out.back()[1] - out[out.size()-2][1]) * (p[1] - out.back()[1]) > 0 ) { out.pop_back(); }
            out.push_back(p);
        }
        return out;
    }

    /*!@brief Do the closed segments pq and rs meet (cross or touch)? */
    inline bool Meet( const Point & p, const Point & q, const Point & r, const Point & s )
    {
        const int d1 = Side(p, q, r), d2 = Side(p, q, s), d3 = Side(r, s, p), d4 = Side(r, s, q);
        if( d1 * d2 < 0 && d3 * d4 < 0 ) { return true; }
        auto on = []( const Point & a, const Point & b, const Point & c )
        {
            return std::min(a[0], b[0]) <= c[0] && c[0] <= std::max(a[0], b[0])
                && std::min(a[1], b[1]) <= c[1] && c[1] <= std::max(a[1], b[1]);
        };
        return (d1 == 0 && on(p, q, r)) || (d2 == 0 && on(p, q, s)) || (d3 == 0 && on(r, s, p)) || (d4 == 0 && on(r, s, q));
    }

    /*!@brief Where the lines pq and rs meet; false if they are parallel. */
    inline bool Intersection( const Point & p, const Point & q, const Point & r, const Point & s, Point & x )
    {
        const Real d1 = Cross(r, s, p), d2 = Cross(r, s, q);
        if( d1 == d2 ) { return false; }
        x = Lerp(p, q, d1 / (d1 - d2));
        return true;
    }

    /*!@brief Twice the signed area of the closed polygon. */
    inline Real Area2( const Polyline & poly )
    {
        Real A = 0;
        for( Idx i = 0; i < poly.size(); ++i )
        {
            const Point & p = poly[i]; const Point & q = poly[(i + 1) % poly.size()];
            A += p[0] * q[1] - q[0] * p[1];
        }
        return A;
    }

    /*!@brief Is p strictly inside the closed polygon? (Even-odd; exact sides.) */
    inline bool Inside( const Polyline & poly, const Point & p )
    {
        bool inQ = false;
        for( Idx i = 0; i < poly.size(); ++i )
        {
            const Point & a = poly[i]; const Point & b = poly[(i + 1) % poly.size()];
            if( (a[1] > p[1]) == (b[1] > p[1]) ) { continue; }
            const int sd = Side(a, b, p);
            if( sd == 0 ) { return false; }
            if( (b[1] > a[1]) == (sd > 0) ) { inQ = !inQ; }
        }
        return inQ;
    }

    struct Box
    {
        Point lo { 1e300, 1e300 }, hi { -1e300, -1e300 };

        void Add( const Point & p )
        {
            for( int k = 0; k < 2; ++k ) { lo[k] = std::min(lo[k], p[k]); hi[k] = std::max(hi[k], p[k]); }
        }
        Point Centre() const { return { (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2 }; }
        Real  Size()   const { return std::hypot(hi[0] - lo[0], hi[1] - lo[1]); }
    };

    /*!@brief Is the drawing made of the chains (point ids into P) embedded? Two
     * segments with no common end may not meet; two with a common end may meet
     * only there.
     */
    inline bool EmbeddedQ( const std::vector<Point> & P, const std::vector<std::vector<Idx>> & chains, std::string & why )
    {
        struct Seg { Idx a, b; Real x0, x1; };
        std::vector<Seg> S;
        for( const auto & ch : chains )
        {
            for( Idx i = 0; i + 1 < ch.size(); ++i )
            {
                S.push_back({ ch[i], ch[i+1], std::min(P[ch[i]][0], P[ch[i+1]][0]), std::max(P[ch[i]][0], P[ch[i+1]][0]) });
            }
        }
        std::sort(S.begin(), S.end(), []( const Seg & s, const Seg & t ) { return s.x0 < t.x0; });
        auto on = [&P]( const Seg & s, Idx v )   // is point v on the closed segment s?
        {
            return Meet(P[s.a], P[s.b], P[v], P[v]);
        };
        for( Idx i = 0; i < S.size(); ++i )
        {
            for( Idx j = i + 1; j < S.size() && S[j].x0 <= S[i].x1; ++j )
            {
                const Seg & s = S[i]; const Seg & t = S[j];
                const bool aQ = (t.a == s.a || t.a == s.b), bQ = (t.b == s.a || t.b == s.b);
                const bool badQ = (aQ && bQ) ? true      // two straight segments between the same two points
                                : aQ ? (on(s, t.b) || on(t, s.a == t.a ? s.b : s.a))
                                : bQ ? (on(s, t.a) || on(t, s.a == t.b ? s.b : s.a))
                                : Meet(P[s.a], P[s.b], P[t.a], P[t.b]);
                if( badQ )
                {
                    why = "segments " + std::to_string(s.a) + "-" + std::to_string(s.b) + " and "
                        + std::to_string(t.a) + "-" + std::to_string(t.b) + " meet";
                    return false;
                }
            }
        }
        return true;
    }

    //==========================================================================
    // The drawing.
    //==========================================================================

    struct PlaneDrawing
    {
        struct Crossing
        {
            Point p { 0, 0 };
            std::array<Idx,2> over  { None, None };   // the over strand: arc in, arc out
            std::array<Idx,2> under { None, None };   // the under strand: arc in, arc out
        };
        struct Arc
        {
            Idx      tail = None, head = None;        // crossings; None for an unused label
            Polyline pts;                             // tail's point .. head's point
            Idx      component = None;
        };

        std::vector<Crossing>         crossings;
        std::vector<Arc>              arcs;
        std::vector<std::vector<Idx>> components;     // each component's arcs in order
        std::vector<long long>        colors;         // each component's colour in the trace
        Idx  exterior_arc      = None;                // the exterior face is on the left of this arc,
        bool exterior_forwardQ = true;                // run forward (tail to head) or backward

        bool ActiveQ( Idx a ) const { return arcs[a].tail != None; }

        // A directed arc ("darc") is 2 a + 1 for arc a forward and 2 a backward.
        Idx ExteriorDarc() const { return 2 * exterior_arc + (exterior_forwardQ ? 1 : 0); }

        Box Bounds() const
        {
            Box B;
            for( const Arc & A : arcs ) { for( const Point & p : A.pts ) { B.Add(p); } }
            return B;
        }

        /*!@brief Read the components off by walking each strand through its crossings. */
        void WalkComponents( const std::vector<long long> & arc_color = {} )
        {
            components.clear(); colors.clear();
            for( Arc & A : arcs ) { A.component = None; }
            for( Idx a0 = 0; a0 < arcs.size(); ++a0 )
            {
                if( !ActiveQ(a0) || arcs[a0].component != None ) { continue; }
                components.emplace_back();
                colors.push_back(a0 < arc_color.size() ? arc_color[a0] : static_cast<long long>(components.size() - 1));
                for( Idx a = a0; a != None && arcs[a].component == None; )
                {
                    arcs[a].component = components.size() - 1;
                    components.back().push_back(a);
                    const Crossing & C = crossings[arcs[a].head];
                    a = (C.over[0] == a) ? C.over[1] : C.under[1];
                }
            }
        }

        /*!@brief For each darc, the next darc round the face on its left, read
         * from the geometry: at the head, the first arc clockwise from the way
         * back. Unused darcs get None. False if an arc does not run between its
         * crossings' points or leaves one with no direction.
         */
        bool NextDarcs( std::vector<Idx> & next, std::string & why ) const
        {
            next.assign(2 * arcs.size(), None);
            std::vector<std::vector<std::pair<Real,Idx>>> out (crossings.size());   // (angle, darc leaving)
            for( Idx a = 0; a < arcs.size(); ++a )
            {
                if( !ActiveQ(a) ) { continue; }
                const Arc & A = arcs[a];
                const Idx n = A.pts.size();
                if( n < 2 || A.tail >= crossings.size() || A.head >= crossings.size()
                 || A.pts.front() != crossings[A.tail].p || A.pts.back() != crossings[A.head].p || A.pts[0] == A.pts[1] || A.pts[n-1] == A.pts[n-2] )
                {
                    why = "arc " + std::to_string(a) + " does not run between its crossings' points";
                    return false;
                }
                out[A.tail].push_back({ std::atan2(A.pts[1][1] - A.pts[0][1], A.pts[1][0] - A.pts[0][0]), 2 * a + 1 });
                out[A.head].push_back({ std::atan2(A.pts[n-2][1] - A.pts[n-1][1], A.pts[n-2][0] - A.pts[n-1][0]), 2 * a });
            }
            for( auto & o : out )
            {
                std::sort(o.begin(), o.end());
                for( Idx k = 0; k < o.size(); ++k )
                {
                    // o[k] leaves; its reverse arrives, and goes on along the arc before o[k] counterclockwise
                    next[o[k].second ^ 1] = o[(k + o.size() - 1) % o.size()].second;
                }
            }
            return true;
        }

        /*!@brief The faces as cycles of darcs, each face on the left of its darcs. */
        bool Faces( std::vector<std::vector<Idx>> & faces, std::string & why ) const
        {
            std::vector<Idx> next;
            if( !NextDarcs(next, why) ) { return false; }
            std::vector<char> seen (next.size(), 0);
            faces.clear();
            for( Idx d0 = 0; d0 < next.size(); ++d0 )
            {
                if( next[d0] == None || seen[d0] ) { continue; }
                faces.emplace_back();
                for( Idx d = d0; !seen[d]; d = next[d] ) { seen[d] = 1; faces.back().push_back(d); }
            }
            return true;
        }
    };

    /*!@brief The drawing of `pd` that the OrthoDraw layout `H` makes. Uses the
     * layout's vertex coordinates, not ArcLines(), which stops an arc short of
     * a crossing it goes under.
     */
    template<class OD_T, class PD_T>
    bool FromOrthoDraw( const OD_T & H, const PD_T & pd, PlaneDrawing & D, std::string & why )
    {
        using Int = typename PD_T::Int;
        D = PlaneDrawing{};
        const auto & V   = H.VertexCoordinates();
        const auto & A_V = H.ArcVertices();
        if( A_V.SublistCount() < pd.MaxArcCount() || H.FaceCount() <= Int(0) ) { why = "the layout is empty or is not this diagram's"; return false; }
        auto pt = [&V]( Int v ) { return Point{ static_cast<Real>(V(v,0)), static_cast<Real>(V(v,1)) }; };
        D.crossings.resize(static_cast<Idx>(pd.MaxCrossingCount()));
        D.arcs.resize(static_cast<Idx>(pd.MaxArcCount()));
        for( Int c = 0; c < pd.MaxCrossingCount(); ++c ) { if( pd.CrossingActiveQ(c) ) { D.crossings[static_cast<Idx>(c)].p = pt(c); } }
        std::vector<long long> color (D.arcs.size(), 0);
        for( Int a = 0; a < pd.MaxArcCount(); ++a )
        {
            if( !pd.ArcActiveQ(a) ) { continue; }
            PlaneDrawing::Arc & A = D.arcs[static_cast<Idx>(a)];
            A.tail = static_cast<Idx>(pd.Arcs()(a, PD_T::Tail));
            A.head = static_cast<Idx>(pd.Arcs()(a, PD_T::Head));
            Polyline L;
            for( Int v : A_V.Sublist(a) ) { L.push_back(pt(v)); }
            A.pts = Corners(L);
            (pd.ArcOverQ(a, PD_T::Head) ? D.crossings[A.head].over : D.crossings[A.head].under)[0] = static_cast<Idx>(a);
            (pd.ArcOverQ(a, PD_T::Tail) ? D.crossings[A.tail].over : D.crossings[A.tail].under)[1] = static_cast<Idx>(a);
            color[static_cast<Idx>(a)] = static_cast<long long>(pd.ArcColors()[a]);
        }
        D.WalkComponents(color);
        const Int da = *H.FaceDarcs().Sublist(H.ExteriorFace()).begin();
        D.exterior_arc      = static_cast<Idx>(da / Int(2));
        D.exterior_forwardQ = (da % Int(2)) != Int(0);
        return true;
    }

    //==========================================================================
    // Two drawings of one diagram, point for point.
    //==========================================================================

    struct Matched
    {
        std::vector<Point>            P0, P1;     // every point, in the first and in the second drawing
        std::vector<Idx>              crossing;   // by crossing label: its point (None if unused)
        std::vector<std::vector<Idx>> chain;      // by arc label: tail's point .. head's point (empty if unused)
        std::vector<std::vector<Idx>> faces;      // cycles of points, each face on its left
        Idx                           exterior = None;   // which of `faces` is the exterior
    };

    /*!@brief Match the drawings A and B of one diagram by normalised arc length
     * along each arc. Every corner of either drawing becomes a point of both,
     * at its own parameter, and each drawing's own corners are kept exactly;
     * every arc gets at least one point between its crossings. False, with a
     * reason, if A and B are not drawings of the same plane graph with the
     * same exterior face.
     */
    inline bool Match( const PlaneDrawing & A, const PlaneDrawing & B, Matched & M, std::string & why, Real merge = 1e-6 )
    {
        M = Matched{};
        if( A.arcs.size() != B.arcs.size() || A.crossings.size() != B.crossings.size() ) { why = "the drawings have different label ranges"; return false; }
        for( Idx a = 0; a < A.arcs.size(); ++a )
        {
            if( A.arcs[a].tail != B.arcs[a].tail || A.arcs[a].head != B.arcs[a].head ) { why = "arc " + std::to_string(a) + " joins different crossings in the two drawings"; return false; }
        }
        for( Idx c = 0; c < A.crossings.size(); ++c )
        {
            if( A.crossings[c].over != B.crossings[c].over || A.crossings[c].under != B.crossings[c].under ) { why = "crossing " + std::to_string(c) + " differs in the two drawings"; return false; }
        }
        std::vector<Idx> nextA, nextB;
        if( !A.NextDarcs(nextA, why) || !B.NextDarcs(nextB, why) ) { return false; }
        if( nextA != nextB ) { why = "the drawings are not of the same plane graph (their faces differ)"; return false; }
        if( A.exterior_arc == None || A.exterior_arc >= A.arcs.size() || !A.ActiveQ(A.exterior_arc) ) { why = "the first drawing names no exterior face"; return false; }

        M.crossing.assign(A.crossings.size(), None);
        M.chain.assign(A.arcs.size(), {});
        for( Idx a = 0; a < A.arcs.size(); ++a )
        {
            if( !A.ActiveQ(a) ) { continue; }
            for( Idx c : { A.arcs[a].tail, A.arcs[a].head } )
            {
                if( M.crossing[c] != None ) { continue; }
                M.crossing[c] = M.P0.size();
                M.P0.push_back(A.crossings[c].p); M.P1.push_back(B.crossings[c].p);
            }
        }
        for( Idx a = 0; a < A.arcs.size(); ++a )
        {
            if( !A.ActiveQ(a) ) { continue; }
            const Polyline & L0 = A.arcs[a].pts; const Polyline & L1 = B.arcs[a].pts;
            const std::vector<Real> s0 = Params(L0), s1 = Params(L1);
            struct Stop { Real t; Idx i, j; };            // i, j: the corner of L0, L1 here, or None
            std::vector<Stop> stops, merged;
            for( Idx i = 1; i + 1 < L0.size(); ++i ) { stops.push_back({ s0[i], i, None }); }
            for( Idx j = 1; j + 1 < L1.size(); ++j ) { stops.push_back({ s1[j], None, j }); }
            if( stops.empty() ) { stops.push_back({ 0.5, None, None }); }
            std::sort(stops.begin(), stops.end(), []( const Stop & x, const Stop & y ) { return x.t < y.t; });
            for( const Stop & st : stops )
            {
                if( !merged.empty() && st.t - merged.back().t < merge && (merged.back().i == None) != (st.i == None) )
                {
                    if( st.i != None ) { merged.back().i = st.i; } else { merged.back().j = st.j; }
                    continue;
                }
                merged.push_back(st);
            }
            std::vector<Idx> & ch = M.chain[a];
            ch.push_back(M.crossing[A.arcs[a].tail]);
            for( const Stop & st : merged )
            {
                M.P0.push_back(st.i != None ? L0[st.i] : At(L0, s0, st.t));
                M.P1.push_back(st.j != None ? L1[st.j] : At(L1, s1, st.t));
                ch.push_back(M.P0.size() - 1);
            }
            ch.push_back(M.crossing[A.arcs[a].head]);
            for( Idx k = 0; k + 1 < ch.size(); ++k )
            {
                if( M.P0[ch[k]] == M.P0[ch[k+1]] || M.P1[ch[k]] == M.P1[ch[k+1]] ) { why = "matching arc " + std::to_string(a) + " made a segment of no length"; return false; }
            }
        }
        // the faces as cycles of points; the exterior must be the same face in both
        std::vector<char> seen (nextA.size(), 0);
        Idx extB = None;
        for( Idx d0 = 0; d0 < nextA.size(); ++d0 )
        {
            if( nextA[d0] == None || seen[d0] ) { continue; }
            M.faces.emplace_back();
            for( Idx d = d0; !seen[d]; d = nextA[d] )
            {
                seen[d] = 1;
                if( d == A.ExteriorDarc() ) { M.exterior = M.faces.size() - 1; }
                if( d == B.ExteriorDarc() ) { extB = M.faces.size() - 1; }
                const std::vector<Idx> & ch = M.chain[d / 2];
                for( Idx t = 0; t + 1 < ch.size(); ++t ) { M.faces.back().push_back((d & 1) ? ch[t] : ch[ch.size() - 1 - t]); }
            }
        }
        if( M.exterior != extB ) { why = "the drawings have different exterior faces"; return false; }
        return true;
    }

} // namespace Knoodle::Animate
