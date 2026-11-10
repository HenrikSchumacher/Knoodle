#pragma once

// Build: a move trace as an Animation (docs/animate-design.md).
//
// The includer has included Knoodle.hpp and src/MoveTrace.hpp. This header
// reads no files, prints nothing and keeps no state.
//
// What is animated so far: the first layout; curl removals (r1), each followed
// by a reshape into a fresh layout of the next diagram; spin-offs to the tray.
// A move of any other kind is shown as a cut, with a note saying so.

#include "Morph.hpp"
#include "Scene.hpp"
#include "Tray.hpp"
#include "Pass.hpp"
#include "Diagram.hpp"
#if __has_include("Sweep.hpp")
    #include "Sweep.hpp"
    #define KNOODLE_ANIMATE_SWEEP 1
#endif

#include "../PassDescriptor.hpp"
#include "../OrthoDecorate.hpp"
#include "../../tools/r1_move.hpp"
#include "../../tools/diagram_agreement.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Knoodle::Animate
{
    struct BuildOptions
    {
        Real t_layout  = 1.0;       // the first drawing, still
        Real t_hold    = 0.7;       // before a move
        Real t_r1      = 1.2;       // a curl's contraction
        Real t_pass_hold = 1.0;     // a pass: its strand, route and disk shown still
        Real t_pass    = 3.2;       // the sweep
        Idx  pass_frames = 96;
        Int64 margin   = 4;         // cells round the layout that a corridor may use
        Real t_after   = 0.4;       // after a move, before the reshape
        Real t_reshape = 2.2;
        Real t_spin    = 1.2;       // a spin-off's slide to the tray
        Real t_cut     = 1.0;       // a drawing shown still before a cut
        Real t_end     = 1.5;
        Idx  reshape_frames = 48;
        Real round = 0.3;           // Morph's steady-shape parameter
        Real pad   = 3;             // world units kept clear round a drawing
        TrayOptions tray;
    };

    namespace detail
    {
        inline P2 ToP2( const Point & p ) { return P2{ p[0], p[1] }; }

        /*!@brief A drawing as paths: each component one closed path, in the
         * order of `D.components`, with a cut where a strand goes under. */
        struct Drawn
        {
            std::vector<Path> paths;
            std::vector<Cut>  cuts;
            std::vector<std::vector<P2>> pts;
        };

        inline Drawn DrawnOf( const PlaneDrawing & D )
        {
            Drawn R;
            std::vector<std::pair<int,int>> start (D.arcs.size(), { -1, 0 });      // by arc: its path, and its first point there
            for( Idx k = 0; k < D.components.size(); ++k )
            {
                Path p; p.closed = true; p.color = static_cast<int>(D.colors[k]);
                R.paths.push_back(p);
                R.pts.emplace_back();
                for( Idx a : D.components[k] )
                {
                    start[a] = { static_cast<int>(k), static_cast<int>(R.pts.back().size()) };
                    const Polyline & L = D.arcs[a].pts;
                    for( Idx i = 0; i + 1 < L.size(); ++i ) { R.pts.back().push_back(ToP2(L[i])); }
                }
            }
            for( const PlaneDrawing::Crossing & C : D.crossings )
            {
                if( C.under[1] == None || C.over[1] == None ) { continue; }
                R.cuts.push_back(Cut{ start[C.under[1]].first, start[C.under[1]].second, start[C.over[1]].first, start[C.over[1]].second });
            }
            return R;
        }

        inline bool EmptyQ( const PlaneDrawing & D )
        {
            for( Idx a = 0; a < D.arcs.size(); ++a ) { if( D.ActiveQ(a) ) { return false; } }
            return true;
        }

        inline Point MoveTo( PlaneDrawing & D, const Point & centre )
        {
            const Point c = D.Bounds().Centre(), d { centre[0] - c[0], centre[1] - c[1] };
            for( auto & C : D.crossings ) { C.p[0] += d[0]; C.p[1] += d[1]; }
            for( auto & A : D.arcs ) { for( Point & p : A.pts ) { p[0] += d[0]; p[1] += d[1]; } }
            return d;
        }

        /*!@brief Turn F by the quarter turn about its centre that brings its
         * crossings nearest to D's (both drawings of one diagram, same labels),
         * so that a reshape does not spin the picture. */
        inline int TurnToward( PlaneDrawing & F, const PlaneDrawing & D, Point & about )
        {
            const Point cf = F.Bounds().Centre(), cd = D.Bounds().Centre();
            about = cf;
            int best = 0; Real least = 1e300;
            for( int k = 0; k < 4; ++k )
            {
                Real sum = 0;
                for( Idx c = 0; c < F.crossings.size() && c < D.crossings.size(); ++c )
                {
                    if( F.crossings[c].over[1] == None || D.crossings[c].over[1] == None ) { continue; }
                    Real x = F.crossings[c].p[0] - cf[0], y = F.crossings[c].p[1] - cf[1];
                    for( int q = 0; q < k; ++q ) { const Real t = x; x = -y; y = t; }
                    sum += (x - D.crossings[c].p[0] + cd[0]) * (x - D.crossings[c].p[0] + cd[0]) + (y - D.crossings[c].p[1] + cd[1]) * (y - D.crossings[c].p[1] + cd[1]);
                }
                if( sum < least ) { least = sum; best = k; }
            }
            auto turn = [&]( Point & p ) { Real x = p[0] - cf[0], y = p[1] - cf[1]; for( int q = 0; q < best; ++q ) { const Real t = x; x = -y; y = t; } p = Point{ cf[0] + x, cf[1] + y }; };
            for( auto & C : F.crossings ) { turn(C.p); }
            for( auto & A : F.arcs ) { for( Point & p : A.pts ) { turn(p); } }
            return best;
        }

        /*!@brief A box round everything a flat scene draws in the world. */
        inline bool WorldBox( const Scene & sc, Real pad, ViewBox & b )
        {
            b = ViewBox{ 1e300, 1e300, -1e300, -1e300 };
            for( const Key & k : sc.keys )
            {
                for( Idx j = 0; j < k.pts.size(); ++j )
                {
                    if( sc.paths[j].space == Space::Screen ) { continue; }
                    const auto & pts = (sc.paths[j].space == Space::World || &k == &sc.keys.front()) ? k.pts[j] : sc.keys.front().pts[j];
                    for( const P2 & p : pts ) { b.x0 = std::min(b.x0, p.x); b.y0 = std::min(b.y0, p.y); b.x1 = std::max(b.x1, p.x); b.y1 = std::max(b.y1, p.y); }
                }
            }
            if( b.x0 > b.x1 ) { return false; }
            b.x0 -= pad; b.y0 -= pad; b.x1 += pad; b.y1 += pad;
            return true;
        }

        /*!@brief The darc cycles of the faces that run clockwise. A drawing of a
         * connected diagram has exactly one: its exterior. */
        inline std::vector<Idx> ClockwiseFaces( const PlaneDrawing & D )
        {
            std::vector<Idx> out;
            std::vector<std::vector<Idx>> faces; std::string why;
            if( !D.Faces(faces, why) ) { return out; }
            for( const auto & f : faces )
            {
                Polyline poly;
                for( Idx d : f )
                {
                    const Polyline & L = D.arcs[d / 2].pts;
                    for( Idx i = 0; i + 1 < L.size(); ++i ) { poly.push_back((d % 2) ? L[i] : L[L.size() - 1 - i]); }
                }
                if( Area2(poly) < 0 ) { out.push_back(f[0]); }
            }
            return out;
        }

        inline std::string KindOf( const std::string & move )
        {
            const auto i = move.find("kind=");
            if( i == std::string::npos ) { return ""; }
            const auto j = move.find(' ', i);
            return move.substr(i + 5, (j == std::string::npos) ? std::string::npos : j - i - 5);
        }
    } // namespace detail

    /*!@brief Builds an animation from the records of one trace. */
    template<class PD_T>
    class Builder
    {
    public:
        using Int      = typename PD_T::Int;
        using OD_T     = OrthoDraw<PD_T>;
        using Record_T = typename MoveTrace<PD_T>::Record;

        Builder( const BuildOptions & options ) : opt(options) {}

        /*!@brief False, with a reason, only if there is nothing to draw. A
         * move that cannot be animated becomes a cut whose note says why. */
        bool Build( const std::vector<Record_T> & trace, const std::string & source, Animation & out, std::string & why )
        {
            A = Animation{}; A.source = source; tray.clear(); own.clear(); stillQ.clear();
            std::vector<const Record_T *> recs;
            for( const Record_T & r : trace ) { if( !r.candidateQ && r.state ) { recs.push_back(&r); } }
            if( recs.empty() ) { why = "the trace has no diagram"; return false; }

            PlaneDrawing D;
            if( HasCrossings(*recs[0]->state) && !Layout(*recs[0]->state, Int(-1), D, why) ) { return false; }
            Still(D, "Diagram", opt.t_layout, *recs[0]->state, "");
            for( std::size_t i = 0; i < recs.size(); ++i )
            {
                const Record_T & rec = *recs[i];
                if( !rec.move ) { continue; }
                const PD_T * next = (i + 1 < recs.size()) ? &*recs[i+1]->state : (rec.result ? &*rec.result : nullptr);
                const std::string kind = detail::KindOf(*rec.move);
                std::string note;
                bool doneQ = false;
                if( kind == "r1" ) { doneQ = R1(*rec.state, *rec.move, next, D, note); }
                else if( kind == "pass" ) { doneQ = Pass(*rec.state, *rec.move, next, D, note); }
                else               { note = "A " + kind + " move is not animated yet: the picture cuts to the next diagram."; }
                if( !doneQ )
                {
                    Still(D, "Cut", opt.t_cut, *rec.state, note);
                    D = PlaneDrawing{};
                    std::string w;
                    if( next && HasCrossings(*next) && !Layout(*next, Int(-1), D, w) ) { D = PlaneDrawing{}; }
                    if( next ) { Still(D, "Diagram", opt.t_after, *next, ""); }
                }
            }
            if( A.scenes.empty() ) { why = "nothing to draw"; return false; }
            A.scenes.back().duration += opt.t_end;
            Camera();
            out = std::move(A);
            return true;
        }

    private:

        BuildOptions opt;
        Animation A;
        std::vector<int>     tray;      // the colours in the tray, in order
        std::vector<ViewBox> own;       // each scene's own box
        std::vector<char>    stillQ;

        static bool HasCrossings( const PD_T & pd ) { return pd.ValidQ() && pd.CrossingCount() > Int(0); }

        // The layout the drawing in hand came from, and how it was placed: turned `xk` quarter
        // turns about `xc`, then moved by `xd`. A corridor is routed in the layout and carried over.
        std::unique_ptr<OD_T> H;
        int   xk = 0;
        Point xc { 0, 0 }, xd { 0, 0 };
        bool  layoutQ = false;          // is the drawing in hand that layout, placed so?

        Point Placed( Real x, Real y ) const
        {
            x -= xc[0]; y -= xc[1];
            for( int q = 0; q < xk; ++q ) { const Real t = x; x = -y; y = t; }
            return Point{ xc[0] + x + xd[0], xc[1] + y + xd[1] };
        }

        bool Layout( const PD_T & pd, Int face, PlaneDrawing & D, std::string & why )
        {
            typename OD_T::Settings_T s {};
            s.x_grid_size = s.y_grid_size = Int(4);      // world units: the lattice is 4 apart, a stroke 0.28 wide
            H = std::make_unique<OD_T>(pd, face, s);
            xk = 0; xc = xd = Point{ 0, 0 };
            layoutQ = FromOrthoDraw(*H, pd, D, why);
            return layoutQ;
        }

        void Push( Scene && sc, bool still )
        {
            ViewBox b;
            if( !detail::WorldBox(sc, opt.pad, b) ) { b = own.empty() ? ViewBox{ -10, -10, 10, 10 } : own.back(); }
            if( !still && !own.empty() && !stillQ.back() ) { b = own.back(); }       // two moving scenes in a row: the camera stays put
            sc.view0 = sc.view1 = b;
            own.push_back(b); stillQ.push_back(still ? 1 : 0);
            A.scenes.push_back(std::move(sc));
        }

        Scene SceneOf( const detail::Drawn & R, const std::string & label, Real duration, Int before, Int after, const std::string & note )
        {
            Scene sc;
            sc.label = label; sc.note = note; sc.duration = duration; sc.easing = Easing::Linear;
            sc.crossings_before = static_cast<int>(before); sc.crossings_after = static_cast<int>(after);
            sc.paths = R.paths;
            Key k; k.pts = R.pts; k.cuts = R.cuts;
            sc.keys.push_back(std::move(k));
            return sc;
        }

        void Still( const PlaneDrawing & D, const std::string & label, Real duration, const PD_T & pd, const std::string & note )
        {
            const Int n = pd.ValidQ() ? pd.CrossingCount() : Int(0);
            Scene sc = SceneOf(detail::DrawnOf(D), label, duration, n, n, note);
            AddTray(sc, tray, opt.tray);
            Push(std::move(sc), true);
        }

        /*!@brief The reshape from D into a fresh layout of `pd` on D's exterior
         * face, centred where D is. On success D is that layout. */
        bool Reshape( PlaneDrawing & D, const PD_T & pd, std::string & note )
        {
            std::string why;
            PlaneDrawing F;
            const Int face = PassDescriptor<Int>::LeftFace(pd, static_cast<Int>(D.ExteriorDarc()));
            if( face < Int(0) ) { note = "The drawing's exterior names no face of the next diagram."; return false; }
            if( !Layout(pd, face, F, why) ) { note = "No layout of the next diagram: " + why; return false; }
            layoutQ = false;                        // until D is F
            xk = detail::TurnToward(F, D, xc);
            xd = detail::MoveTo(F, D.Bounds().Centre());
            MorphResult R;
            if( !Morph(D, F, EasedTimes(opt.reshape_frames), opt.round, R, why) ) { note = "The reshape could not be made: " + why; return false; }
            Scene sc;
            sc.label = "Reshape diagram"; sc.duration = opt.t_reshape; sc.easing = Easing::Linear;      // the frames are eased already
            sc.crossings_before = sc.crossings_after = static_cast<int>(pd.CrossingCount());
            for( std::size_t f = 0; f < R.frames.size(); ++f )
            {
                detail::Drawn dr = detail::DrawnOf(DrawingAt(D, R.match, R.frames[f]));
                if( f == 0 ) { sc.paths = dr.paths; }
                Key k; k.pts = std::move(dr.pts); k.cuts = std::move(dr.cuts);
                sc.keys.push_back(std::move(k));
            }
            AddTray(sc, tray, opt.tray);
            Push(std::move(sc), false);
            D = std::move(F);
            layoutQ = true;
            return true;
        }

        /*!@brief Carry a drawing of `from` over to the labels of `to`, an
         * isomorphic diagram, keeping colours. */
        bool Relabel( const PlaneDrawing & D, const PD_T & from, const PD_T & to, PlaneDrawing & out, std::string & why ) const
        {
            DiagramMatch_T<Int> m (from.MaxCrossingCount(), from.MaxArcCount(), to.MaxCrossingCount(), to.MaxArcCount());
            if( !DiagramsIsomorphicQ(from, to, why, &m, true) ) { return false; }
            out = PlaneDrawing{};
            out.crossings.resize(static_cast<Idx>(to.MaxCrossingCount()));
            out.arcs.resize(static_cast<Idx>(to.MaxArcCount()));
            auto am = [&]( Idx a ) { return (a == None) ? None : static_cast<Idx>(m.amap[a]); };
            std::vector<long long> color (out.arcs.size(), 0);
            for( Idx a = 0; a < D.arcs.size(); ++a )
            {
                if( !D.ActiveQ(a) ) { continue; }
                if( a >= m.amap.size() || m.amap[a] < Int(0) ) { why = "an arc of the drawing has no partner"; return false; }
                PlaneDrawing::Arc & B = out.arcs[am(a)];
                B.pts = D.arcs[a].pts;
                B.tail = static_cast<Idx>(m.cmap[D.arcs[a].tail]); B.head = static_cast<Idx>(m.cmap[D.arcs[a].head]);
                color[am(a)] = D.colors[D.arcs[a].component];
            }
            for( Idx c = 0; c < D.crossings.size(); ++c )
            {
                const PlaneDrawing::Crossing & C = D.crossings[c];
                if( C.over[1] == None ) { continue; }
                PlaneDrawing::Crossing & X = out.crossings[static_cast<Idx>(m.cmap[c])];
                X.p = C.p; X.over = { am(C.over[0]), am(C.over[1]) }; X.under = { am(C.under[0]), am(C.under[1]) };
            }
            out.exterior_arc = am(D.exterior_arc); out.exterior_forwardQ = D.exterior_forwardQ;
            out.WalkComponents(color);
            return true;
        }

        /*!@brief A curl removal: the loop contracts to its crossing, then the
         * drawing reshapes into a fresh layout of `next`. False, with a note,
         * if it has to be a cut; D is then unchanged. */
        bool R1( const PD_T & pd, const std::string & move, const PD_T * next, PlaneDrawing & D, std::string & note )
        {
            using namespace KnoodleR1View;
            R1Descriptor<Int> desc; std::string why;
            if( !R1Descriptor<Int>::Parse(move, desc, why) ) { note = "The r1 move does not parse: " + why; return false; }
            R1Resolved<PD_T> r = ResolveR1(pd, desc.loop);
            if( !r.validQ ) { note = "The r1 move does not fit its diagram: " + r.why; return false; }
            std::vector<Int> freed;
            const PD_T after = R1AfterDiagram(pd, r, why, freed);
            Idx a = static_cast<Idx>(r.a), c = static_cast<Idx>(r.c), ap = static_cast<Idx>(r.a_prev), an = static_cast<Idx>(r.a_next);
            const bool lastQ = (ap == an);          // the component is the curl and its twin
            if( PassDescriptor<Int>::LeftFace(pd, static_cast<Int>(D.ExteriorDarc())) == r.monogon )
            {
                // The curl's own face is the outside: pulling it in would sweep it across everything.
                if( !lastQ ) { note = "The curl's loop is drawn round the rest of the diagram; turning the sphere first is not built yet."; return false; }
                std::swap(a, ap); an = ap;          // at a last crossing either loop goes: show the twin
                note = "The trace removes the outer curl; the inner one is shown instead, with the same result.";
            }
            const PlaneDrawing::Crossing & X = D.crossings[c];
            const Idx k = D.arcs[a].component;
            // along the strand: `in` arrives at the crossing, the loop runs, `out` leaves. Which of the
            // two the trace calls a_prev (it dies) and which a_next (it lives on) is the applier's choice.
            const Idx in = (X.over[0] == a) ? X.under[0] : X.over[0], out = (X.over[1] == a) ? X.under[1] : X.over[1];
            if( !((in == ap && out == an) || (in == an && out == ap)) ) { note = "The drawing does not have the curl the move names."; return false; }
            // the curl's component: the loop, and the rest from the crossing round to the crossing
            detail::Drawn dr = detail::DrawnOf(D);
            std::vector<std::pair<int,int>> start (D.arcs.size(), { -1, 0 });
            for( Idx j = 0; j < D.components.size(); ++j )
            {
                int at = 0;
                for( Idx b : D.components[j] ) { start[b] = { static_cast<int>(j), at }; at += static_cast<int>(D.arcs[b].pts.size()) - 1; }
            }
            const int loop = static_cast<int>(dr.paths.size());
            {
                const auto & comp = D.components[k];
                Idx at = 0; while( comp[at] != out ) { ++at; }
                std::vector<P2> rest; int n = 0;
                for( Idx s = 0; s < comp.size(); ++s )
                {
                    const Idx b = comp[(at + s) % comp.size()];
                    if( b == a ) { continue; }
                    start[b] = { static_cast<int>(k), n };
                    for( Idx i = 0; i + 1 < D.arcs[b].pts.size(); ++i ) { rest.push_back(detail::ToP2(D.arcs[b].pts[i])); ++n; }
                }
                rest.push_back(detail::ToP2(X.p));
                dr.pts[k] = rest; dr.paths[k].closed = false;
                Path lp = dr.paths[k]; lp.role = Role::Loop; lp.alpha1 = 0.3; lp.layer = 1;
                dr.paths.push_back(lp);
                dr.pts.emplace_back();
                for( const Point & p : D.arcs[a].pts ) { dr.pts.back().push_back(detail::ToP2(p)); }
                start[a] = { loop, 0 };
            }
            const int restEnd = static_cast<int>(dr.pts[k].size()) - 1, loopEnd = static_cast<int>(dr.pts[loop].size()) - 1, kk = static_cast<int>(k);
            dr.cuts.clear();
            for( Idx x = 0; x < D.crossings.size(); ++x )
            {
                const PlaneDrawing::Crossing & C = D.crossings[x];
                if( C.under[1] == None || x == c ) { continue; }
                dr.cuts.push_back(Cut{ start[C.under[1]].first, start[C.under[1]].second, start[C.over[1]].first, start[C.over[1]].second });
            }
            std::vector<Cut> all = dr.cuts;         // with the curl's own crossing: the under passage is cut on both sides
            if( X.under[1] == a ) { all.push_back(Cut{ loop, 0, kk, 0 }); all.push_back(Cut{ kk, restEnd, kk, 0 }); }
            else                  { all.push_back(Cut{ kk, 0, loop, 0 }); all.push_back(Cut{ loop, loopEnd, loop, 0 }); }

            const Int n0 = pd.CrossingCount(), n1 = n0 - Int(1);
            detail::Drawn held = dr; held.cuts = all;
            Scene hold = SceneOf(held, "R1 move", opt.t_hold, n0, n1, note);
            AddTray(hold, tray, opt.tray); Push(std::move(hold), true);
            Scene go = SceneOf(held, "R1 move", opt.t_r1, n0, n1, note);
            go.easing = Easing::Smoother;
            Key shrunk; shrunk.pts = dr.pts; shrunk.cuts = dr.cuts;
            for( P2 & p : shrunk.pts[static_cast<Idx>(loop)] ) { p = detail::ToP2(X.p); }
            go.keys.push_back(std::move(shrunk));
            AddTray(go, tray, opt.tray); Push(std::move(go), false);

            // the drawing the move leaves, in the labels R1AfterDiagram keeps
            PlaneDrawing E = D;
            std::vector<long long> color (E.arcs.size(), 0);
            for( Idx b = 0; b < E.arcs.size(); ++b ) { if( E.ActiveQ(b) ) { color[b] = E.colors[E.arcs[b].component]; } }
            const long long curlColor = color[a];
            std::vector<P2> freeLoop;
            E.arcs[a] = PlaneDrawing::Arc{};
            E.crossings[c] = PlaneDrawing::Crossing{};
            if( lastQ )
            {
                for( Idx i = 0; i + 1 < E.arcs[an].pts.size(); ++i ) { freeLoop.push_back(detail::ToP2(E.arcs[an].pts[i])); }
                E.arcs[an] = PlaneDrawing::Arc{};
            }
            else
            {
                Polyline L = E.arcs[in].pts;
                L.insert(L.end(), E.arcs[out].pts.begin() + 1, E.arcs[out].pts.end());
                const Idx tail = E.arcs[in].tail, head = E.arcs[out].head, comp = E.arcs[an].component;
                E.arcs[ap] = PlaneDrawing::Arc{};
                E.arcs[an].pts = std::move(L); E.arcs[an].tail = tail; E.arcs[an].head = head; E.arcs[an].component = comp;
                for( Idx x : { tail, head } )
                {
                    PlaneDrawing::Crossing & T = E.crossings[x];
                    for( int s = 0; s < 2; ++s ) { if( T.over[s] == ap ) { T.over[s] = an; } if( T.under[s] == ap ) { T.under[s] = an; } }
                }
                // The face outside the loop is on the other side of the strand it joins: right of a
                // left-turning loop, and left of the arcs before and after it.
                if( E.exterior_arc == a ) { E.exterior_arc = an; E.exterior_forwardQ = !E.exterior_forwardQ; }
                else if( E.exterior_arc == ap ) { E.exterior_arc = an; }
            }
            E.WalkComponents(color);
            if( !lastQ )
            {
                // The exterior is the one face that runs clockwise. Read it off the drawing: the rule
                // above is right, but this is the claim the reshape will check.
                const std::vector<Idx> cw = detail::ClockwiseFaces(E);
                if( cw.size() == 1 ) { E.exterior_arc = cw[0] / 2; E.exterior_forwardQ = (cw[0] % 2) != 0; }
            }
            if( lastQ && detail::EmptyQ(E) ) { E = PlaneDrawing{}; }

            // a component that came free goes to the tray
            if( lastQ )
            {
                Scene slide = SceneOf(detail::DrawnOf(E), "Spin-off", opt.t_spin, n1, n1, "");
                slide.easing = Easing::Smoother;
                slide.keys.push_back(slide.keys[0]);
                AddTray(slide, tray, opt.tray);
                AddSpinOff(slide, freeLoop, static_cast<int>(curlColor), static_cast<int>(tray.size()), opt.tray);
                Push(std::move(slide), false);
                tray.push_back(static_cast<int>(curlColor));
            }
            if( !next || !HasCrossings(*next) || detail::EmptyQ(E) )
            {
                D = std::move(E); layoutQ = false;
                Still(D, "Diagram", opt.t_after, after, "");
                return true;
            }
            // into the next record's labels, then into its fresh layout
            PlaneDrawing G; std::string n2;
            if( !Relabel(E, after, *next, G, why) ) { n2 = "The drawing the curl left is not the next diagram: " + why; }
            else
            {
                Still(G, "Diagram", opt.t_after, *next, "");
                if( Reshape(G, *next, n2) ) { D = std::move(G); return true; }
            }
            // the move itself was shown; only the reshape is a cut
            Still(E, "Cut", opt.t_cut, after, n2);
            PlaneDrawing F;
            if( !Layout(*next, Int(-1), F, why) ) { F = PlaneDrawing{}; }
            D = std::move(F);
            Still(D, "Diagram", opt.t_after, *next, "");
            return true;
        }

        /*!@brief After a move: the drawing it left goes into the next record's
         * labels and reshapes into a fresh layout; or, failing that, a cut.
         * `mine` is the diagram E draws. Always leaves D ready for the next move. */
        void Settle( PlaneDrawing & E, const PD_T & mine, const PD_T * next, PlaneDrawing & D )
        {
            std::string why, n2;
            if( !next || !HasCrossings(*next) || detail::EmptyQ(E) )
            {
                D = std::move(E); layoutQ = false;
                Still(D, "Diagram", opt.t_after, mine, "");
                return;
            }
            PlaneDrawing G;
            if( !Relabel(E, mine, *next, G, why) ) { n2 = "The drawing the move left is not the next diagram: " + why; }
            else
            {
                Still(G, "Diagram", opt.t_after, *next, "");
                if( Reshape(G, *next, n2) ) { D = std::move(G); return; }
            }
            Still(E, "Cut", opt.t_cut, mine, n2);
            PlaneDrawing F;
            if( !Layout(*next, Int(-1), F, why) ) { F = PlaneDrawing{}; layoutQ = false; }
            D = std::move(F);
            Still(D, "Diagram", opt.t_after, *next, "");
        }

        /*!@brief A pass move: the strand, its route and the disk between them
         * shown still; the strand sweeping across the disk; the drawing left
         * reshaping into a fresh layout. False, with a note, if it has to be a
         * cut; D is then unchanged. */
        bool Pass( const PD_T & pd, const std::string & move, const PD_T * next, PlaneDrawing & D, std::string & note )
        {
            PassDescriptor<Int> mv; std::string why;
            if( !PassDescriptor<Int>::Parse(move, mv, why) ) { note = "The pass move does not parse: " + why; return false; }
            if( mv.middlepassQ ) { note = "A middlepass move is not animated yet: the picture cuts to the next diagram."; return false; }
            if( !layoutQ || !H ) { note = "The drawing in hand is not a layout, so no corridor can be routed in it."; return false; }
            const Int margin = static_cast<Int>(opt.margin);
            OrthoDecorate<PD_T> deco (*H, margin);
            const auto pr = deco.RoutePassMove(pd, mv);
            if( !pr.validQ ) { note = "The corridor does not route in this layout: " + pr.why; return false; }
            Polyline route;
            for( const auto & c : pr.route.path ) { route.push_back(Placed(static_cast<Real>(c[0] - margin), static_cast<Real>(c[1] - margin))); }
            route = Corners(route);

            std::vector<Idx> arcs; std::vector<char> inW (D.arcs.size(), 0);
            const bool forwardQ = PassDescriptor<Int>::DirOf(mv.strand.front());
            for( Int da : mv.strand )
            {
                if( PassDescriptor<Int>::DirOf(da) != forwardQ ) { note = "The strand does not run one way along its component."; return false; }
                arcs.push_back(static_cast<Idx>(da / Int(2))); inW[arcs.back()] = 1;
            }
            std::vector<char> over_new;
            for( std::size_t i = 0; i < mv.cross.size(); ++i ) { if( !inW[static_cast<Idx>(mv.cross[i] / Int(2))] ) { over_new.push_back(mv.over[i] ? 1 : 0); } }
            const bool overQ = !mv.over.empty() ? static_cast<bool>(mv.over[0])
                             : ((arcs.size() >= 2) ? pd.ArcOverQ(static_cast<Int>(arcs[0]), forwardQ ? PD_T::Head : PD_T::Tail) : true);

            PassPicture P;
            if( !PassSurgery(D, arcs, forwardQ, route, over_new, P, why) ) { note = "The move does not fit the drawing: " + why; return false; }
            PD_T mine;
            if( !detail::EmptyQ(P.after) )
            {
                const std::vector<Idx> cw = detail::ClockwiseFaces(P.after);
                if( cw.size() != 1 ) { note = "The drawing the move leaves is not connected: the reshape after it is not built yet."; return false; }
                P.after.exterior_arc = cw[0] / 2; P.after.exterior_forwardQ = (cw[0] % 2) != 0;
                mine = ToPlanarDiagram<PD_T>(P.after, why);
                if( !mine.ValidQ() ) { note = "The drawing the move leaves is not a diagram: " + why; return false; }
            }

            // the frames of the sweep
            std::vector<Polyline> frames;
            std::string method;
#ifdef KNOODLE_ANIMATE_SWEEP
            {
                std::vector<Real> times;
                for( Idx f = 0; f < opt.pass_frames; ++f ) { times.push_back(Smoother(static_cast<Real>(f) / static_cast<Real>(opt.pass_frames - 1))); }
                SweepResult SR;
                if( Sweep(P.W, P.route, times, SR, why) ) { frames = std::move(SR.frames); }
                else { method = "The strand jumps to its route: no sweep could be made (" + why + ")."; }
            }
#else
            method = "The strand jumps to its route: the sweep is not built yet.";
#endif
            if( frames.empty() ) { frames = { P.W, P.route }; }

            // the paths: the stills, the route, the strand
            const int ns = static_cast<int>(P.still.size()), iroute = ns, istrand = ns + 1;
            Scene base;
            base.label = "Pass move"; base.note = method; base.easing = Easing::Linear;
            base.crossings_before = static_cast<int>(pd.CrossingCount());
            base.crossings_after  = static_cast<int>(pd.CrossingCount()) - static_cast<int>(P.removed) + static_cast<int>(P.added);
            std::vector<std::vector<P2>> still (P.still.size());
            for( Idx j = 0; j < P.still.size(); ++j )
            {
                Path p; p.closed = (P.still_closed[j] != 0); p.color = static_cast<int>(P.still_color[j]);
                base.paths.push_back(p);
                for( const Point & q : P.still[j] ) { still[j].push_back(detail::ToP2(q)); }
            }
            { Path p; p.role = Role::Route; p.layer = 2; base.paths.push_back(p); }
            { Path p; p.role = Role::Moving; p.layer = 3; p.round = true; p.color = static_cast<int>(D.colors[D.arcs[arcs[0]].component]); base.paths.push_back(p); }
            base.fills.push_back(Fill{ Role::Disk, overQ ? 1 : -1, { { istrand, false }, { iroute, true } } });
            std::vector<P2> routeP; for( const Point & q : P.route ) { routeP.push_back(detail::ToP2(q)); }
            auto key = [&]( const Polyline & frame, bool firstQ )
            {
                Key k; k.pts = still; k.cuts = P.still_cuts;
                std::vector<P2> moving; for( const Point & q : frame ) { moving.push_back(detail::ToP2(q)); }
                if( firstQ )            // on W itself the crossings are the diagram\'s own, at shared points
                {
                    for( Cut c : P.w_cuts ) { if( c.under < 0 ) { c.under = istrand; } if( c.over < 0 ) { c.over = istrand; } k.cuts.push_back(c); }
                }
                else { Weld(k.pts, P.still_closed, moving, istrand, overQ, k.cuts); }
                k.pts.push_back(routeP); k.pts.push_back(std::move(moving));
                return k;
            };
            Scene hold = base; hold.duration = opt.t_pass_hold;
            hold.keys.push_back(key(frames.front(), true));
            AddTray(hold, tray, opt.tray); Push(std::move(hold), true);
            Scene go = base; go.duration = opt.t_pass;
            go.paths[static_cast<Idx>(iroute)].alpha0 = go.paths[static_cast<Idx>(iroute)].alpha1 = 0;
            for( Idx f = 0; f < frames.size(); ++f ) { go.keys.push_back(key(frames[f], f == 0)); }
            AddTray(go, tray, opt.tray); Push(std::move(go), false);

            // components the move left with no crossing go to the tray
            for( Idx f = 0; f < P.freed.size(); ++f )
            {
                Scene slide = SceneOf(detail::DrawnOf(P.after), "Spin-off", opt.t_spin, base.crossings_after, base.crossings_after, "");
                slide.easing = Easing::Smoother;
                slide.keys.push_back(slide.keys[0]);
                for( Idx g = f + 1; g < P.freed.size(); ++g )       // those still waiting their turn
                {
                    Path p; p.closed = true; p.color = static_cast<int>(P.freed_colors[g]); slide.paths.push_back(p);
                    std::vector<P2> q; for( const Point & z : P.freed[g] ) { q.push_back(detail::ToP2(z)); }
                    slide.keys[0].pts.push_back(q); slide.keys[1].pts.push_back(q);
                }
                AddTray(slide, tray, opt.tray);
                std::vector<P2> loop; for( const Point & z : P.freed[f] ) { loop.push_back(detail::ToP2(z)); }
                AddSpinOff(slide, loop, static_cast<int>(P.freed_colors[f]), static_cast<int>(tray.size()), opt.tray);
                Push(std::move(slide), false);
                tray.push_back(static_cast<int>(P.freed_colors[f]));
            }
            Settle(P.after, mine, next, D);
            return true;
        }

        /*!@brief The camera: a moving scene keeps its own box; a still one
         * eases from the box before it to the box after it. */
        void Camera()
        {
            const std::size_t n = A.scenes.size();
            for( std::size_t i = 0; i < n; ++i )
            {
                if( !stillQ[i] ) { continue; }
                // runs of stills share the move across them only at their ends: keep it simple, ease in each
                A.scenes[i].view0 = (i > 0) ? A.scenes[i-1].view1 : own[i];
                A.scenes[i].view1 = (i + 1 < n && !stillQ[i+1]) ? own[i+1] : own[i];
            }
        }
    };

} // namespace Knoodle::Animate
