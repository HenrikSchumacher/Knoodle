#pragma once

// The surgery of a pass move, done on the drawing itself
// (docs/animate-design.md, "One drawing type").
//
// A pass move lifts a strand W off the diagram between two dots on its end
// arcs and lays it down again along a route between the same dots. Here the
// drawing is held as a WEAVE: each component one closed polyline, with a
// crossing's name and level at the vertices where it passes through one. In
// that form the move is three edits: forget the crossings W went through, put
// the route where W was, and mark the places the route meets other strands.
// The drawing the move leaves is read back off the weave; its labels are its
// own, and ToPlanarDiagram and an isomorphism carry it to the trace's.
//
// Standard library and PlaneDrawing.hpp only.

#include "PlaneDrawing.hpp"
#include "Scene.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace Knoodle::Animate
{
    struct Weave
    {
        struct Strand
        {
            Polyline          p;        // closed: the last point joins the first
            std::vector<long> x;        // per vertex: the crossing it passes through there, or -1
            std::vector<char> over;     // per vertex with a crossing: is this the over passage?
            std::vector<Idx>  arc;      // per vertex: the drawing's arc that the segment from here runs on (None on a route)
            long long         color = 0;
        };
        std::vector<Strand> strands;
        long next_crossing = 0;         // a name no crossing has yet
    };

    /*!@brief The weave of a drawing: strand k is component k. `start[a]` is
     * the vertex of its strand at which arc a begins. */
    inline Weave WeaveOf( const PlaneDrawing & D, std::vector<Idx> & start )
    {
        Weave Wv;
        start.assign(D.arcs.size(), None);
        Wv.next_crossing = static_cast<long>(D.crossings.size());
        for( Idx k = 0; k < D.components.size(); ++k )
        {
            Weave::Strand S; S.color = D.colors[k];
            for( Idx a : D.components[k] )
            {
                start[a] = S.p.size();
                const Polyline & L = D.arcs[a].pts;
                for( Idx i = 0; i + 1 < L.size(); ++i )
                {
                    S.p.push_back(L[i]);
                    S.x.push_back((i == 0) ? static_cast<long>(D.arcs[a].tail) : -1);
                    S.over.push_back((i == 0 && D.crossings[D.arcs[a].tail].over[1] == a) ? 1 : 0);
                    S.arc.push_back(a);
                }
            }
            Wv.strands.push_back(std::move(S));
        }
        return Wv;
    }

    /*!@brief The drawing a weave is, with labels of its own (crossings and
     * arcs numbered as they are met). A strand with no crossing is not part
     * of a drawing: it is handed back in `loops`, with its colour. The
     * exterior is not set. False if a crossing does not have exactly one over
     * and one under passage. */
    inline bool DrawingOf( const Weave & Wv, PlaneDrawing & E, std::vector<Polyline> & loops, std::vector<long long> & loop_colors, std::string & why )
    {
        E = PlaneDrawing{}; loops.clear(); loop_colors.clear();
        std::map<long,Idx> name;
        std::vector<int> overs, unders;
        for( const Weave::Strand & S : Wv.strands )
        {
            for( Idx i = 0; i < S.p.size(); ++i )
            {
                if( S.x[i] < 0 ) { continue; }
                auto it = name.find(S.x[i]);
                if( it == name.end() ) { it = name.emplace(S.x[i], name.size()).first; overs.push_back(0); unders.push_back(0); }
                ++(S.over[i] ? overs : unders)[it->second];
            }
        }
        for( Idx c = 0; c < overs.size(); ++c ) { if( overs[c] != 1 || unders[c] != 1 ) { why = "a crossing does not have one over and one under passage"; return false; } }
        E.crossings.resize(name.size());
        std::vector<long long> color;
        for( const Weave::Strand & S : Wv.strands )
        {
            const Idx n = S.p.size();
            std::vector<Idx> ev;
            for( Idx i = 0; i < n; ++i ) { if( S.x[i] >= 0 ) { ev.push_back(i); } }
            if( ev.empty() ) { loops.push_back(S.p); loop_colors.push_back(S.color); continue; }
            const Idx first = E.arcs.size();
            for( Idx j = 0; j < ev.size(); ++j )
            {
                const Idx i0 = ev[j], i1 = ev[(j + 1) % ev.size()], a = first + j;
                PlaneDrawing::Arc A;
                A.tail = name[S.x[i0]]; A.head = name[S.x[i1]];
                for( Idx i = i0; ; i = (i + 1) % n ) { A.pts.push_back(S.p[i]); if( i == i1 && A.pts.size() > 1 ) { break; } }
                E.arcs.push_back(std::move(A)); color.push_back(S.color);
                PlaneDrawing::Crossing & T = E.crossings[name[S.x[i0]]], & H = E.crossings[name[S.x[i1]]];
                T.p = S.p[i0];
                (S.over[i0] ? T.over : T.under)[1] = a;
                (S.over[i1] ? H.over : H.under)[0] = a;
            }
        }
        // the two passages of a crossing were placed along two different strands: make them one point exactly
        for( PlaneDrawing::Arc & A : E.arcs ) { A.pts.front() = E.crossings[A.tail].p; A.pts.back() = E.crossings[A.head].p; }
        E.WalkComponents(color);
        return true;
    }

    /*!@brief Where the point q lies on the closed polyline p, looking only at
     * the `count` segments from vertex `from` on: the segment, and how far
     * along it. False if it is on none of them (to within `tol`). */
    inline bool Locate( const Polyline & p, Idx from, Idx count, const Point & q, Real tol, Idx & seg, Real & t )
    {
        const Idx n = p.size();
        for( Idx s = 0; s < count; ++s )
        {
            const Idx i = (from + s) % n;
            const Point & a = p[i], & b = p[(i + 1) % n];
            const Real dx = b[0] - a[0], dy = b[1] - a[1], d2 = dx * dx + dy * dy;
            if( d2 <= 0 ) { continue; }
            const Real u = ((q[0] - a[0]) * dx + (q[1] - a[1]) * dy) / d2;
            if( u < -1e-12 || u > 1 + 1e-12 ) { continue; }
            if( std::hypot(q[0] - a[0] - u * dx, q[1] - a[1] - u * dy) <= tol ) { seg = i; t = std::min(1.0, std::max(0.0, u)); return true; }
        }
        return false;
    }

    /*!@brief What a pass move's animation needs. */
    struct PassPicture
    {
        Polyline W, route;                          // from the tail dot to the head dot
        std::vector<Polyline>  still;               // every strand with W taken out; W's own is open, from the head dot round to the tail dot
        std::vector<char>      still_closed;
        std::vector<long long> still_color;
        std::vector<std::vector<Idx>> still_arc;    // per still vertex: the drawing's arc that the segment from there runs on
        std::vector<Idx>       met_arc;             // the arcs the route meets, in its own order, and ...
        std::vector<Point>     met_at;              // ... where
        std::vector<Cut>       still_cuts;          // the crossings among them: (under path, vertex, over path, vertex)
        std::vector<Cut>       w_cuts;              // W's own crossings before the move: under/over are still paths, or -1 for W
        PlaneDrawing           after;               // the drawing the move leaves, in labels of its own
        std::vector<Polyline>  freed;               // components it leaves with no crossing
        std::vector<long long> freed_colors;
        Idx removed = 0, added = 0;                 // crossings
    };

    /*!@brief Do a pass move on the drawing D. `strand` is W as arcs in the
     * order W runs, all of them run forwards (`forwardQ`) or all backwards;
     * `route` runs from the tail dot, a point of the first arc, to the head
     * dot, a point of the last; `over_new[i]` says whether the strand passes
     * over the i-th strand the route meets. False, with a reason, if the
     * drawing and the route do not fit the move. */
    inline bool PassSurgery( const PlaneDrawing & D, const std::vector<Idx> & strand, bool forwardQ, const Polyline & route,
                             const std::vector<char> & over_new, PassPicture & out, std::string & why )
    {
        out = PassPicture{};
        if( strand.empty() || route.size() < 2 ) { why = "no strand or no route"; return false; }
        std::vector<Idx> start;
        Weave Wv = WeaveOf(D, start);
        const Idx k = D.arcs[strand[0]].component;
        Weave::Strand & S = Wv.strands[k];
        const Real tol = 1e-9 * std::max(D.Bounds().Size(), 1e-300);

        // the two dots, in the order the strand's own orientation meets them
        const Idx aT = strand.front(), aH = strand.back();
        const Point dotT = route.front(), dotH = route.back();
        const Idx a1 = forwardQ ? aT : aH, a2 = forwardQ ? aH : aT;
        const Point d1 = forwardQ ? dotT : dotH, d2 = forwardQ ? dotH : dotT;
        Idx s1 = 0, s2 = 0; Real t1 = 0, t2 = 0;
        if( !Locate(S.p, start[a1], D.arcs[a1].pts.size() - 1, d1, tol, s1, t1) || !Locate(S.p, start[a2], D.arcs[a2].pts.size() - 1, d2, tol, s2, t2) )
        {
            why = "a dot of the route is not on its arc"; return false;
        }
        if( a1 == a2 && (s2 < s1 || (s2 == s1 && t2 <= t1)) ) { why = "the dots are in the wrong order on the strand's one arc"; return false; }

        // rotate the strand so that it starts at the first dot; the stretch to the second dot is W
        {
            const Idx n = S.p.size();
            Weave::Strand R; R.color = S.color;
            auto same = [&]( const Point & u, const Point & v ) { return std::hypot(u[0] - v[0], u[1] - v[1]) <= tol; };
            // add a point; one that repeats the last and names no crossing is not added. Returns its place.
            auto put = [&]( const Point & q, long x, char o, Idx arc ) -> Idx
            {
                if( !R.p.empty() && same(q, R.p.back()) && x < 0 ) { R.arc.back() = arc; return R.p.size() - 1; }
                if( !R.p.empty() && same(q, R.p.back()) && R.x.back() < 0 ) { R.x.back() = x; R.over.back() = o; R.arc.back() = arc; return R.p.size() - 1; }
                R.p.push_back(q); R.x.push_back(x); R.over.push_back(o); R.arc.push_back(arc);
                return R.p.size() - 1;
            };
            put(d1, -1, 0, a1);
            Idx mark = None;
            if( s2 == s1 && t2 > t1 ) { mark = put(d2, -1, 0, a2); }       // both dots on one segment, in order
            for( Idx step = 0; step < n; ++step )
            {
                const Idx g = (s1 + step) % n;                              // the segment being walked
                if( step > 0 && g == s2 && mark == None ) { mark = put(d2, -1, 0, a2); }
                const Idx v = (g + 1) % n;
                put(S.p[v], S.x[v], S.over[v], S.arc[v]);
            }
            if( mark == None && s2 == s1 ) { mark = put(d2, -1, 0, a2); }  // the second dot comes last, just before the first
            if( mark == None ) { why = "the second dot was not met going round the strand"; return false; }
            if( R.p.size() > 1 && same(R.p.back(), R.p.front()) && R.x.back() < 0 ) { R.p.pop_back(); R.x.pop_back(); R.over.pop_back(); R.arc.pop_back(); }
            if( mark >= R.p.size() ) { mark = 0; }
            S = std::move(R);
            s2 = mark;
        }
        // now S.p[0] is the first dot and S.p[s2] the second; vertices 1 .. s2-1 are W's inside
        Polyline Wfwd (S.p.begin(), S.p.begin() + static_cast<std::ptrdiff_t>(s2) + 1);
        std::vector<long> gone;
        for( Idx i = 1; i < s2; ++i ) { if( S.x[i] >= 0 ) { gone.push_back(S.x[i]); } }
        out.removed = gone.size();
        if( out.removed + 1 != strand.size() ) { why = "the strand passes through " + std::to_string(out.removed) + " crossings between its dots, not " + std::to_string(strand.size() - 1); return false; }
        out.W = Wfwd; if( !forwardQ ) { std::reverse(out.W.begin(), out.W.end()); }
        out.route = route;

        // the stills: every strand, W's stretch taken out of its own; and W's old crossings with them
        const Idx nS = S.p.size();
        std::map<long,std::pair<int,int>> passage;                  // gone crossing -> (still path, vertex) of the other passage
        std::map<long,int> wvertex;                                 // gone crossing -> vertex of out.W
        for( Idx i = 1; i < s2; ++i ) { if( S.x[i] >= 0 ) { wvertex[S.x[i]] = static_cast<int>(forwardQ ? i : s2 - i); } }
        std::map<long,std::array<int,4>> live;                      // crossing -> under path, vertex, over path, vertex
        for( Idx j = 0; j < Wv.strands.size(); ++j )
        {
            const Weave::Strand & T = Wv.strands[j];
            Polyline L; std::vector<Idx> from; std::vector<Idx> arcOf;
            if( j == k ) { for( Idx i = s2; i < nS; ++i ) { L.push_back(T.p[i]); from.push_back(i); } L.push_back(T.p[0]); from.push_back(0); }
            else         { for( Idx i = 0; i < T.p.size(); ++i ) { L.push_back(T.p[i]); from.push_back(i); } }
            for( Idx v = 0; v < L.size(); ++v )
            {
                const Idx i = from[v];
                if( T.x[i] < 0 || (j == k && (i == 0 || i == s2)) ) { continue; }
                const bool goneQ = std::find(gone.begin(), gone.end(), T.x[i]) != gone.end();
                if( goneQ ) { passage[T.x[i]] = { static_cast<int>(j), static_cast<int>(v) }; continue; }
                auto & e = live.emplace(T.x[i], std::array<int,4>{ -1, 0, -1, 0 }).first->second;
                e[T.over[i] ? 2 : 0] = static_cast<int>(j); e[T.over[i] ? 3 : 1] = static_cast<int>(v);
            }
            for( Idx i : from ) { arcOf.push_back(T.arc[i]); }
            out.still.push_back(std::move(L)); out.still_closed.push_back(j != k); out.still_color.push_back(T.color); out.still_arc.push_back(std::move(arcOf));
        }
        for( const auto & [x, e] : live ) { if( e[0] >= 0 && e[2] >= 0 ) { out.still_cuts.push_back(Cut{ e[0], e[1], e[2], e[3] }); } }
        for( Idx i = 1; i < s2; ++i )
        {
            if( S.x[i] < 0 ) { continue; }
            const auto it = passage.find(S.x[i]);
            if( it == passage.end() ) { continue; }                 // W crosses itself there
            const int wv = wvertex[S.x[i]];
            if( S.over[i] ) { out.w_cuts.push_back(Cut{ it->second.first, it->second.second, -1, wv }); }
            else            { out.w_cuts.push_back(Cut{ -1, wv, it->second.first, it->second.second }); }
        }

        // forget W's crossings everywhere, and put the route where W was
        for( Weave::Strand & T : Wv.strands ) { for( Idx i = 0; i < T.p.size(); ++i ) { if( T.x[i] >= 0 && std::find(gone.begin(), gone.end(), T.x[i]) != gone.end() ) { T.x[i] = -1; T.over[i] = 0; } } }
        {
            Polyline r = route; if( !forwardQ ) { std::reverse(r.begin(), r.end()); }
            Weave::Strand R; R.color = S.color;
            for( Idx i = 0; i + 1 < r.size(); ++i ) { R.p.push_back(r[i]); R.x.push_back(-1); R.over.push_back(0); R.arc.push_back(None); }
            const Idx m = R.p.size();                               // the second dot's place now
            for( Idx i = s2; i < nS; ++i ) { R.p.push_back(S.p[i]); R.x.push_back(S.x[i]); R.over.push_back(S.over[i]); R.arc.push_back(S.arc[i]); }
            S = std::move(R);
            s2 = m;
        }

        // where the route meets other strands: a new crossing each, in the route's own order
        struct Meet { Real along; Idx rs; Real rt; Idx j, s; Real t; };
        std::vector<Meet> meets;
        for( Idx rs = 0; rs < s2; ++rs )
        {
            const Point & a = S.p[rs], & b = S.p[rs + 1 == S.p.size() ? 0 : rs + 1];
            for( Idx j = 0; j < Wv.strands.size(); ++j )
            {
                const Weave::Strand & T = Wv.strands[j];
                const Idx n = T.p.size();
                for( Idx s = (j == k ? s2 : 0); s < n; ++s )
                {
                    const Point & c = T.p[s], & d = T.p[(s + 1) % n];
                    const Real rx = b[0] - a[0], ry = b[1] - a[1], sx = d[0] - c[0], sy = d[1] - c[1], den = rx * sy - ry * sx;
                    if( den == 0 ) { continue; }
                    const Real ta = ((c[0] - a[0]) * sy - (c[1] - a[1]) * sx) / den, tc = ((c[0] - a[0]) * ry - (c[1] - a[1]) * rx) / den;
                    const Real ea = tol / std::max(std::hypot(rx, ry), 1e-300), ec = tol / std::max(std::hypot(sx, sy), 1e-300);
                    if( ta <= ea || ta >= 1 - ea || tc < -ec || tc > 1 + ec ) { continue; }
                    if( tc <= ec || tc >= 1 - ec ) { why = "the route passes through a corner of another strand"; return false; }
                    meets.push_back(Meet{ forwardQ ? Real(rs) + ta : Real(s2) - Real(rs) - ta, rs, ta, j, s, tc });
                }
            }
        }
        std::sort(meets.begin(), meets.end(), []( const Meet & p, const Meet & q ) { return p.along < q.along; });
        if( meets.size() != over_new.size() ) { why = "the route meets " + std::to_string(meets.size()) + " strands, and the move says " + std::to_string(over_new.size()); return false; }
        out.added = meets.size();
        for( const Meet & m : meets )
        {
            const Weave::Strand & T = Wv.strands[m.j];
            const Point & c = T.p[m.s], & d = T.p[(m.s + 1) % T.p.size()];
            out.met_arc.push_back(T.arc[m.s]);
            out.met_at.push_back(Point{ c[0] + (d[0] - c[0]) * m.t, c[1] + (d[1] - c[1]) * m.t });
        }
        struct Ins { Idx s; Real t; long x; char over; Point q; };
        std::vector<std::vector<Ins>> ins (Wv.strands.size());
        for( Idx i = 0; i < meets.size(); ++i )
        {
            const long x = Wv.next_crossing++;
            // one point for both passages, worked out on the pieces as they are now (before any is cut)
            const Point & ra = S.p[meets[i].rs], & rb = S.p[(meets[i].rs + 1) % S.p.size()];
            const Point at { ra[0] + (rb[0] - ra[0]) * meets[i].rt, ra[1] + (rb[1] - ra[1]) * meets[i].rt };
            ins[k].push_back(Ins{ meets[i].rs, meets[i].rt, x, over_new[i], at });
            ins[meets[i].j].push_back(Ins{ meets[i].s, meets[i].t, x, static_cast<char>(over_new[i] ? 0 : 1), at });
        }
        for( Idx j = 0; j < Wv.strands.size(); ++j )
        {
            Weave::Strand & T = Wv.strands[j];
            std::sort(ins[j].begin(), ins[j].end(), []( const Ins & p, const Ins & q ) { return (p.s != q.s) ? (p.s > q.s) : (p.t > q.t); });
            for( const Ins & e : ins[j] )
            {
                const auto at = static_cast<std::ptrdiff_t>(e.s) + 1;
                T.p.insert(T.p.begin() + at, e.q); T.x.insert(T.x.begin() + at, e.x); T.over.insert(T.over.begin() + at, e.over);
                T.arc.insert(T.arc.begin() + at, T.arc[e.s]);
            }
        }
        return DrawingOf(Wv, out.after, out.freed, out.freed_colors, why);
    }

    /*!@brief Weld a moving polyline to the still ones for one frame: wherever
     * `moving` properly crosses a still polyline, both get a vertex there and
     * a cut is made: `over_at(path, segment, t)` says whether the moving strand
     * goes over the still one where it meets that segment (before any vertex
     * is added) at t along it. `cuts` come in as the cuts among the stills and are kept, their
     * vertices renumbered. In the cuts, path `moving_path` is the moving one. */
    template<class Role_T>
    void Weld( std::vector<std::vector<P2>> & still, const std::vector<char> & closed, std::vector<P2> & moving, int moving_path,
               Role_T && over_at, std::vector<Cut> & cuts )
    {
        struct Hit { Idx seg; Real t; int id; };
        std::vector<char> overs;
        std::vector<std::vector<Hit>> on (still.size());
        std::vector<Hit> onM;
        int ids = 0;
        for( Idx ms = 0; ms + 1 < moving.size(); ++ms )
        {
            const P2 & a = moving[ms], & b = moving[ms + 1];
            for( Idx j = 0; j < still.size(); ++j )
            {
                const Idx n = still[j].size(), segs = closed[j] ? n : n - 1;
                for( Idx s = 0; s < segs; ++s )
                {
                    const P2 & c = still[j][s], & d = still[j][(s + 1) % n];
                    const Real rx = b.x - a.x, ry = b.y - a.y, sx = d.x - c.x, sy = d.y - c.y, den = rx * sy - ry * sx;
                    if( den == 0 ) { continue; }
                    const Real ta = ((c.x - a.x) * sy - (c.y - a.y) * sx) / den, tc = ((c.x - a.x) * ry - (c.y - a.y) * rx) / den;
                    if( ta < 0 || ta >= 1 || tc < 0 || tc >= 1 ) { continue; }
                    if( (ms == 0 && ta <= 1e-9) ) { continue; }             // the dot itself, where the stub ends
                    on[j].push_back(Hit{ s, tc, ids }); onM.push_back(Hit{ ms, ta, ids }); ++ids;
                    overs.push_back(over_at(j, s, tc) ? 1 : 0);
                }
            }
        }
        std::vector<std::pair<int,int>> where (static_cast<Idx>(ids), { 0, 0 }), whereM (static_cast<Idx>(ids), { 0, 0 });
        auto insert = [&]( std::vector<P2> & p, bool closedQ, std::vector<Hit> & hits, std::vector<std::pair<int,int>> & place, int path, std::vector<Idx> * remap )
        {
            std::sort(hits.begin(), hits.end(), []( const Hit & x, const Hit & y ) { return (x.seg != y.seg) ? (x.seg < y.seg) : (x.t < y.t); });
            std::vector<P2> q; std::vector<Idx> map (p.size(), 0);
            Idx h = 0;
            for( Idx i = 0; i < p.size(); ++i )
            {
                map[i] = q.size(); q.push_back(p[i]);
                const P2 & a = p[i], & b = p[(i + 1) % p.size()];
                for( ; h < hits.size() && hits[h].seg == i; ++h )
                {
                    if( !closedQ && i + 1 == p.size() ) { break; }
                    place[static_cast<Idx>(hits[h].id)] = { path, static_cast<int>(q.size()) };
                    q.push_back(P2{ a.x + (b.x - a.x) * hits[h].t, a.y + (b.y - a.y) * hits[h].t });
                }
            }
            p = std::move(q);
            if( remap ) { *remap = std::move(map); }
        };
        std::vector<std::vector<Idx>> remap (still.size());
        for( Idx j = 0; j < still.size(); ++j ) { insert(still[j], closed[j] != 0, on[j], where, static_cast<int>(j), &remap[j]); }
        insert(moving, false, onM, whereM, moving_path, nullptr);
        for( Cut & c : cuts )
        {
            if( c.under != moving_path ) { c.iu = static_cast<int>(remap[static_cast<Idx>(c.under)][static_cast<Idx>(c.iu)]); }
            if( c.over  != moving_path ) { c.io = static_cast<int>(remap[static_cast<Idx>(c.over)][static_cast<Idx>(c.io)]); }
        }
        for( Idx i = 0; i < static_cast<Idx>(ids); ++i )
        {
            if( overs[i] ) { cuts.push_back(Cut{ where[i].first, where[i].second, whereM[i].first, whereM[i].second }); }
            else        { cuts.push_back(Cut{ whereM[i].first, whereM[i].second, where[i].first, where[i].second }); }
        }
    }

} // namespace Knoodle::Animate
