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
#include "Turn.hpp"
#include "Redraw.hpp"
#include "Route.hpp"
#if __has_include("Sweep.hpp")
    #include "Sweep.hpp"
    #define KNOODLE_ANIMATE_SWEEP 1
#endif

#include "../PassDescriptor.hpp"
#include "../../tools/r1_move.hpp"
#include "../../tools/diagram_agreement.hpp"

#include <map>
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
        Real t_after   = 0.4;       // after a move, before the reshape
        Real t_reshape = 2.2;
        Real t_spin    = 1.2;       // a spin-off's slide to the tray
        Real t_cut     = 1.0;       // a drawing shown still before a cut
        Real t_end     = 1.5;
        Real height    = 0.35;      // in a turn of the sphere: how far a strand sits above or below the surface at a crossing
        Real turn_tilt = 0.45;
        Real t_fog     = 0.6;       // a redraw: the depth colouring coming in, and going out
        Real t_rotate  = 2.6;       // ... and the turn between      // the camera's tilt during a turn
        Idx  reshape_frames = 48;
        Real t_unroll = 2.4, t_let_go = 0.9;    // after a turn: the layout unrolling; the outside's tint fading
        Idx  turn_frames    = 34;   // of the reshape shown on the sphere, after a turn; the renderer blends between them
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
            // the trace may say which face to put outside (`#view exterior=`)
            const Int face0 = recs[0]->exterior_da ? PassDescriptor<Int>::LeftFace(*recs[0]->state, *recs[0]->exterior_da) : Int(-1);
            if( HasCrossings(*recs[0]->state) && !Layout(*recs[0]->state, face0, D, why) ) { return false; }
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
                else if( kind == "pass" || kind == "middlepass" ) { doneQ = Pass(rec, next, D, note); }
                else if( kind == "redraw" ) { doneQ = Redraw(rec, next, D, note); }
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

        bool Layout( const PD_T & pd, Int face, PlaneDrawing & D, std::string & why )
        {
            typename OD_T::Settings_T s {};
            s.x_grid_size = s.y_grid_size = Int(4);      // world units: the lattice is 4 apart, a stroke 0.28 wide
            OD_T H (pd, face, s);
            return FromOrthoDraw(H, pd, D, why);
        }

        void Push( Scene && sc, bool still, const ViewBox * box = nullptr )
        {
            ViewBox b;
            if( box ) { b = *box; }
            else
            {
                if( !detail::WorldBox(sc, opt.pad, b) ) { b = own.empty() ? ViewBox{ -10, -10, 10, 10 } : own.back(); }
                if( !still && !own.empty() && !stillQ.back() ) { b = own.back(); }   // two moving scenes in a row: the camera stays put
            }
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
            Point about { 0, 0 };
            detail::TurnToward(F, D, about);
            detail::MoveTo(F, D.Bounds().Centre());
            return MorphTo(D, std::move(F), pd.CrossingCount(), note);
        }

        /*!@brief The reshape from D into F, another drawing of the same diagram
         * with the same labels and exterior. On success D is F. */
        bool MorphTo( PlaneDrawing & D, PlaneDrawing F, Int crossings, std::string & note, const ViewBox * box = nullptr )
        {
            std::string why;
            MorphResult R;
            if( !Morph(D, F, EasedTimes(opt.reshape_frames), opt.round, R, why) ) { note = "The reshape could not be made: " + why; return false; }
            Scene sc;
            sc.label = "Reshape diagram"; sc.duration = opt.t_reshape; sc.easing = Easing::Linear;      // the frames are eased already
            sc.crossings_before = sc.crossings_after = static_cast<int>(crossings);
            for( std::size_t f = 0; f < R.frames.size(); ++f )
            {
                detail::Drawn dr = detail::DrawnOf(DrawingAt(D, R.match, R.frames[f]));
                if( f == 0 ) { sc.paths = dr.paths; }
                Key k; k.pts = std::move(dr.pts); k.cuts = std::move(dr.cuts);
                sc.keys.push_back(std::move(k));
            }
            AddTray(sc, tray, opt.tray);
            Push(std::move(sc), false, box);
            D = std::move(F);
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
        bool R1( const PD_T & pd, const std::string & move, const PD_T * next, PlaneDrawing & D, std::string & note, bool turnedQ = false )
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
                if( !lastQ )
                {
                    // Put another face outside first, and start again.
                    if( turnedQ ) { note = "The curl's loop is still drawn round the rest of the diagram after a turn."; return false; }
                    const auto B = D.Bounds();
                    const Real m = B.Size();
                    const Polyline everywhere { { B.lo[0] - m, B.lo[1] - m }, { B.hi[0] + m, B.lo[1] - m }, { B.hi[0] + m, B.hi[1] + m }, { B.lo[0] - m, B.hi[1] + m } };
                    Idx darc = 0; Point pole { 0, 0 };
                    if( !FaceInside(pd, D, everywhere, { r.monogon }, darc, pole) ) { note = "The curl's loop is drawn round the rest of the diagram, and no other face can be put outside."; return false; }
                    Still(D, "Diagram", opt.t_hold, pd, "");
                    if( !TurnTo(pd, D, darc, pole, "The next curl's loop is the outside of the drawing, so another face is put outside first.", note) ) { return false; }
                    return R1(pd, move, next, D, note, true);
                }
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
                D = std::move(E);
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
                D = std::move(E);
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
            if( !Layout(*next, Int(-1), F, why) ) { F = PlaneDrawing{}; }
            D = std::move(F);
            Still(D, "Diagram", opt.t_after, *next, "");
        }

        /*!@brief A pass move: the strand, its route and the disk between them
         * shown still; the strand sweeping across the disk; the drawing left
         * reshaping into a fresh layout. False, with a note, if it has to be a
         * cut; D is then unchanged. */
        /*!@brief A face of D to put outside before a move whose loop is
         * `loop`: one inside the loop, not one of `avoid` (faces of pd), and
         * the roomiest such. Gives a darc with that face on its left and a
         * point well inside it. */
        bool FaceInside( const PD_T & pd, const PlaneDrawing & D, const Polyline & loop, const std::vector<Int> & avoid, Idx & darc, Point & pole ) const
        {
            std::vector<std::vector<Idx>> faces; std::string why;
            if( !D.Faces(faces, why) ) { return false; }
            Real best = 0;
            for( const auto & f : faces )
            {
                const Int face = PassDescriptor<Int>::LeftFace(pd, static_cast<Int>(f[0]));
                if( std::find(avoid.begin(), avoid.end(), face) != avoid.end() ) { continue; }
                if( std::find(f.begin(), f.end(), D.ExteriorDarc()) != f.end() ) { continue; }
                Polyline poly;
                for( Idx d : f ) { const Polyline & L = D.arcs[d / 2].pts; for( Idx i = 0; i + 1 < L.size(); ++i ) { poly.push_back((d % 2) ? L[i] : L[L.size() - 1 - i]); } }
                // a point well inside: of the midpoints of pairs of its corners, the one farthest from its edge
                for( Idx i = 0; i < poly.size(); ++i ) { for( Idx j = i + 1; j < poly.size(); ++j )
                {
                    const Point m { (poly[i][0] + poly[j][0]) / 2, (poly[i][1] + poly[j][1]) / 2 };
                    if( !Inside(poly, m) || !Inside(loop, m) ) { continue; }
                    Real clear = 1e300;
                    for( Idx e = 0; e < poly.size() && clear > best; ++e )
                    {
                        const Point & a = poly[e], & b = poly[(e + 1) % poly.size()];
                        const Real dx = b[0] - a[0], dy = b[1] - a[1], d2 = dx * dx + dy * dy;
                        const Real t = (d2 > 0) ? std::min(1.0, std::max(0.0, ((m[0] - a[0]) * dx + (m[1] - a[1]) * dy) / d2)) : 0;
                        clear = std::min(clear, std::hypot(m[0] - a[0] - t * dx, m[1] - a[1] - t * dy));
                    }
                    if( clear > best ) { best = clear; darc = f[0]; pole = m; }
                } }
            }
            return best > 0;
        }

        /*!@brief Turn the sphere: put the face on the left of `darc` outside,
         * with `pole`, a point of it, sent to infinity. On success D is the
         * drawing the turn leaves (edges cut into arcs of circles), with that
         * exterior; it is not a layout. */
        /*!@brief A drawing as curves for a rolled-up stage: (x, y, h), h up
         * at an over passage and down at an under one; and its crossings. */
        void Lift( const PlaneDrawing & E, std::vector<Curve3> & curves, std::vector<Crossing3> & xs, std::vector<Idx> & start ) const
        {
            curves.assign(E.components.size(), Curve3{}); start.assign(E.arcs.size(), 0); xs.clear();
            for( Idx k = 0; k < E.components.size(); ++k )
            {
                curves[k].color = static_cast<int>(E.colors[k]);
                for( Idx a : E.components[k] )
                {
                    const PlaneDrawing::Arc & A = E.arcs[a];
                    const Real hT = (E.crossings[A.tail].over[1] == a) ? opt.height : -opt.height, hH = (E.crossings[A.head].over[0] == a) ? opt.height : -opt.height;
                    const std::vector<Real> prm = Params(A.pts);
                    start[a] = curves[k].pts.size();
                    for( Idx i = 0; i + 1 < A.pts.size(); ++i ) { curves[k].pts.push_back(P3{ A.pts[i][0], A.pts[i][1], hT + (hH - hT) * prm[i] }); }
                }
            }
            for( const PlaneDrawing::Crossing & C : E.crossings )
            {
                if( C.over[1] == None ) { continue; }
                xs.push_back(Crossing3{ static_cast<int>(E.arcs[C.under[1]].component), static_cast<int>(start[C.under[1]]),
                                        static_cast<int>(E.arcs[C.over[1]].component),  static_cast<int>(start[C.over[1]]) });
            }
        }

        /*!@brief The face on the left of `darc`, as a mark; or, for None, every face. */
        static std::vector<Curve3> FaceMark( const PlaneDrawing & E, Idx darc )
        {
            std::vector<Curve3> marks;
            std::vector<std::vector<Idx>> faces; std::string w;
            if( !E.Faces(faces, w) ) { return marks; }
            for( const auto & f : faces )
            {
                if( darc != None && std::find(f.begin(), f.end(), darc) == f.end() ) { continue; }
                Curve3 m;
                for( Idx d : f ) { const Polyline & L = E.arcs[d / 2].pts; for( Idx i = 0; i + 1 < L.size(); ++i ) { const Point & q = (d % 2) ? L[i] : L[L.size() - 1 - i]; m.pts.push_back(P3{ q[0], q[1], 0 }); } }
                marks.push_back(std::move(m));
            }
            return marks;
        }

        /*!@brief Put the face on the left of `darc` outside, by a turn of the
         * sphere, and leave D a fresh layout with that exterior. The drawing
         * rolls up; it is balanced on the sphere, its faces made as nearly of
         * a size as a Moebius map can; the sphere turns; the drawing is
         * reshaped into the layout while still on the sphere; and the layout
         * unrolls. */
        bool TurnTo( const PD_T & pd, PlaneDrawing & D, Idx darc, const Point & pole, const std::string & reason, std::string & note )
        {
            std::vector<Curve3> curves; std::vector<Crossing3> xs; std::vector<Idx> start;
            Lift(D, curves, xs, start);
            // the face that is to be the outside is pointed out
            const TurnResult T = Turn(curves, xs, P2{ pole[0], pole[1] }, TurnOptions(), FaceMark(D, darc), FaceMark(D, None));
            // the drawing it leaves, in D's labels
            PlaneDrawing G = D;
            for( Idx k = 0; k < D.components.size(); ++k )
            {
                const auto & comp = D.components[k];
                const auto & L = T.left[k].pts;
                for( Idx s = 0; s < comp.size(); ++s )
                {
                    const Idx a = comp[s], b = comp[(s + 1) % comp.size()];
                    const Idx i0 = static_cast<Idx>(T.at[k][start[a]]), i1 = static_cast<Idx>(T.at[k][start[b]]);
                    Polyline q;
                    for( Idx i = i0; ; i = (i + 1) % L.size() ) { q.push_back(Point{ L[i].x, L[i].y }); if( i == i1 && q.size() > 1 ) { break; } }
                    G.arcs[a].pts = std::move(q);
                    G.crossings[G.arcs[a].tail].p = G.arcs[a].pts.front();
                }
            }
            G.exterior_arc = darc / 2; G.exterior_forwardQ = (darc % 2) != 0;
            {
                const std::vector<Idx> cw = detail::ClockwiseFaces(G);
                std::vector<std::vector<Idx>> faces; std::string why;
                bool okQ = (cw.size() == 1) && G.Faces(faces, why);
                if( okQ ) { okQ = false; for( const auto & f : faces ) { if( std::find(f.begin(), f.end(), cw[0]) != f.end() ) { okQ = std::find(f.begin(), f.end(), darc) != f.end(); } } }
                if( !okQ ) { note = "The turn of the sphere did not put the chosen face outside."; return false; }
            }
            Scene sc;
            sc.kind = Scene::Kind::Space; sc.label = "Turn the sphere"; sc.note = reason;
            sc.crossings_before = sc.crossings_after = static_cast<int>(pd.CrossingCount());
            // The layout to end in, and the reshape into it. G is what the turned sphere looks like from
            // its top; the reshape is worked out in the plane from G, and every frame of it is lifted
            // back to the sphere. So what is seen is: the turned sphere, the drawing settling on it into
            // the layout, and then the layout unrolling.
            PlaneDrawing F; MorphResult M; std::string why;
            bool sphereQ = false;
            {
                const Int face = PassDescriptor<Int>::LeftFace(pd, static_cast<Int>(darc));
                if( face >= Int(0) && Layout(pd, face, F, why) )
                {
                    Point about { 0, 0 };
                    detail::TurnToward(F, G, about);
                    detail::MoveTo(F, D.Bounds().Centre());
                    sphereQ = Morph(G, F, EasedTimes(opt.turn_frames), opt.round, M, why);
                }
            }
            Box B = D.Bounds();
            sc.stages = T.stages;                                                   // point out, roll up, balance, turn
            if( sphereQ )
            {
                const Stage & even = T.sphere;                                      // lifts a drawing to the sphere as it stands
                const Idx K = M.frames.size();
                for( Idx f = 0; f < K; ++f )
                {
                    const PlaneDrawing E = DrawingAt(G, M.match, M.frames[f]);
                    Stage s = even;
                    std::vector<Idx> st;
                    Lift(E, s.curves, s.crossings, st);
                    s.marks = FaceMark(E, darc);
                    s.duration = (f + 1 < K) ? opt.t_reshape / static_cast<Real>(K - 1) : 0;
                    s.blend = (f + 1 < K);
                    s.mark_in0 = s.mark_in1 = s.mark_out0 = s.mark_out1 = 0; s.mark_line0 = s.mark_line1 = 1;
                    sc.stages.push_back(std::move(s));
                }
                Stage unroll = sc.stages.back();
                unroll.duration = opt.t_unroll;
                unroll.roll0 = 1; unroll.roll1 = 0; unroll.fog0 = 1; unroll.fog1 = 0; unroll.mark_out0 = 0; unroll.mark_out1 = 1;
                Stage settle = unroll;
                settle.duration = opt.t_let_go; settle.roll0 = settle.roll1 = 0; settle.fog0 = settle.fog1 = 0;
                settle.mark_out0 = 1; settle.mark_out1 = 0; settle.mark_line0 = 1; settle.mark_line1 = 0;
                sc.stages.push_back(std::move(unroll)); sc.stages.push_back(std::move(settle));
                const Box BF = F.Bounds(); B.Add(BF.lo); B.Add(BF.hi);
            }
            else
            {
                // the reshape cannot be shown on the sphere: the turned drawing unrolls as it is
                Stage unroll = T.sphere;
                std::vector<Idx> st;
                Lift(G, unroll.curves, unroll.crossings, st);
                unroll.marks = FaceMark(G, darc);
                unroll.duration = opt.t_unroll; unroll.roll0 = 1; unroll.roll1 = 0; unroll.fog0 = 1; unroll.fog1 = 0; unroll.mark_line0 = 1;
                sc.stages.push_back(std::move(unroll));
                const Box BG = G.Bounds(); B.Add(BG.lo); B.Add(BG.hi);
            }
            sc.duration = 0;
            for( const Stage & s : sc.stages ) { sc.duration += s.duration; }
            sc.tilt_a = 0; sc.tilt_b = opt.turn_tilt; sc.depth = T.radius;
            const Point c = D.Bounds().Centre(); const Real R = T.radius;
            ViewBox box { std::min(B.lo[0], c[0] - 1.15 * R) - opt.pad, std::min(B.lo[1], c[1] - R - 2 * R * opt.turn_tilt) - opt.pad,
                          std::max(B.hi[0], c[0] + 1.15 * R) + opt.pad, std::max(B.hi[1], c[1] + 1.15 * R) + opt.pad };
            Push(std::move(sc), false, &box);
            if( sphereQ ) { D = std::move(F); return true; }
            // ... and is reshaped flat
            D = std::move(G);
            return Reshape(D, pd, note);
        }

        bool Pass( const Record_T & rec, const PD_T * next, PlaneDrawing & D, std::string & note, bool turnedQ = false )
        {
            const PD_T & pd = *rec.state;
            PassDescriptor<Int> mv; std::string why;
            if( !PassDescriptor<Int>::Parse(*rec.move, mv, why) ) { note = "The pass move does not parse: " + why; return false; }
            if( mv.middlepassQ && !rec.feas ) { note = "The middle pass carries no witness, so its levels are not known."; return false; }
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

            // A middle pass: the level of every piece of its witness side.
            std::map<std::pair<Idx,int>,char> level;        // (arc, half) -> a, b or f
            if( mv.middlepassQ ) { for( const auto & cls : rec.feas->classes ) { for( const auto & pc : cls.pieces ) { level[{ static_cast<Idx>(pc.arc), pc.half }] = cls.label; } } }
            const Real tol = 1e-9 * std::max(D.Bounds().Size(), 1e-300);
            auto along = [&]( Idx a, const Point & q, Real & f )
            {
                const Polyline & L = D.arcs[a].pts;
                Idx seg = 0; Real t = 0;
                if( !Locate(L, 0, L.size() - 1, q, tol, seg, t) ) { return false; }
                Real before = 0, total = 0;
                for( Idx i = 0; i + 1 < L.size(); ++i ) { const Real d = Dist(L[i], L[i+1]); if( i < seg ) { before += d; } else if( i == seg ) { before += t * d; } total += d; }
                f = (total > 0) ? before / total : 0;
                return true;
            };

            // Does a route make the move? 0: no (why says); 1: yes, but the witness side holds infinity; 2: yes.
            struct Fit { PassPicture P; PD_T mine; std::map<Idx,Real> split; Polyline loop; std::vector<Polyline> frames; std::string no_sweep; };
            // the frames of the sweep along a route that makes the move, or why there are none
            auto sweep = [&]( Fit & T )
            {
#ifdef KNOODLE_ANIMATE_SWEEP
                std::vector<Real> times;
                for( Idx f = 0; f < opt.pass_frames; ++f ) { times.push_back(Smoother(static_cast<Real>(f) / static_cast<Real>(opt.pass_frames - 1))); }
                SweepResult SR; std::string w;
                if( Sweep(T.P.W, T.P.route, times, SR, w) ) { T.frames = std::move(SR.frames); }
                else { T.no_sweep = "The strand jumps to its route: no sweep could be made (" + w + ")."; }
#else
                T.no_sweep = "The strand jumps to its route: the sweep is not built yet.";
#endif
            };
            auto fit = [&]( const Polyline & route, Fit & F ) -> int
            {
                F = Fit{};
                if( !PassSurgery(D, arcs, forwardQ, route, over_new, F.P, why) ) { return 0; }
                if( !detail::EmptyQ(F.P.after) )
                {
                    const std::vector<Idx> cw = detail::ClockwiseFaces(F.P.after);
                    if( cw.size() != 1 ) { why = "the drawing the move leaves is not connected"; return 0; }
                    F.P.after.exterior_arc = cw[0] / 2; F.P.after.exterior_forwardQ = (cw[0] % 2) != 0;
                    F.mine = ToPlanarDiagram<PD_T>(F.P.after, why);
                    if( !F.mine.ValidQ() ) { return 0; }
                    PlaneDrawing tmp;
                    if( next && HasCrossings(*next) && !Relabel(F.P.after, F.mine, *next, tmp, why) ) { why = "the drawing it leaves is not the next diagram: " + why; return 0; }
                }
                else if( next && HasCrossings(*next) ) { why = "it leaves no crossing, and the next diagram has some"; return 0; }
                F.loop = F.P.W;
                for( Idx i = F.P.route.size(); i-- > 0; ) { F.loop.push_back(F.P.route[i]); }
                if( !mv.middlepassQ ) { return 2; }
                for( Idx i = 0; i < F.P.met_arc.size(); ++i ) { Real f = 0.5; if( F.P.met_arc[i] != None && along(F.P.met_arc[i], F.P.met_at[i], f) ) { F.split[F.P.met_arc[i]] = f; } }
                Idx inside = 0, outside = 0;
                for( const auto & [key, label] : level )
                {
                    const Idx a = key.first;
                    if( a >= D.arcs.size() || !D.ActiveQ(a) ) { continue; }
                    const Real s = F.split.count(a) ? F.split[a] : 0.5;
                    const Real f = (key.second < 0) ? 0.5 : ((key.second == 0) ? s / 2 : (1 + s) / 2);
                    const Polyline & L = D.arcs[a].pts;
                    ++(Inside(F.loop, At(L, Params(L), f)) ? inside : outside);
                }
                if( inside > 0 && outside > 0 ) { why = "the witness's pieces lie on both sides of the strand's loop"; return 0; }
                return (outside > 0) ? 1 : 2;
            };

            // The route: through the faces of the drawing in hand, the first one that makes the move
            // with its witness side bounded; failing that, one that makes the move at all.
            Fit F, round_back; int got = 0, good = 0; bool backQ = false; std::string how; Real least = 0;
            {
                RouteRequest rq;
                rq.first_arc = arcs.front(); rq.last_arc = arcs.back(); rq.forwardQ = forwardQ;
                rq.depart = static_cast<Idx>(mv.depart); rq.land = static_cast<Idx>(mv.land);
                for( Int da : mv.cross ) { rq.cross.push_back(static_cast<Idx>(da)); }
                std::string w2;
                SkeletonRoutes(D, rq, [&]( const Polyline & route )
                {
                    // of the routes that make the move, one the strand can sweep to before one it cannot,
                    // and then the one whose strand sweeps the least; four that can be swept are enough to choose from
                    Fit T; const int g = fit(route, T);
                    if( g == 2 )
                    {
                        sweep(T);
                        const Real swept = std::abs(Area2(T.loop));
                        const bool canQ = !T.frames.empty(), hadQ = (got == 2) && !F.frames.empty();
                        if( got != 2 || (canQ && !hadQ) || (canQ == hadQ && swept < least) ) { F = std::move(T); least = swept; }
                        got = 2;
                        return canQ && ++good >= 4;
                    }
                    if( g == 1 && !backQ ) { round_back = std::move(T); backQ = true; }
                    return false;
                }, w2);
                if( got == 0 && backQ ) { F = std::move(round_back); got = 1; }
                if( got == 0 ) { how = w2.empty() ? why : w2 + " (" + why + ")"; }
            }
            if( got == 0 ) { note = "No route makes the move in this drawing: " + how; return false; }
            PassPicture & P = F.P; PD_T & mine = F.mine; std::map<Idx,Real> & split = F.split;
            if( got == 1 )
            {
                // The witness side holds infinity. Put a face of the other side outside first, and start again.
                if( turnedQ ) { note = "The middle pass still goes round the back of the sphere after a turn."; return false; }
                std::vector<Int> corridor { PassDescriptor<Int>::LeftFace(pd, mv.depart) };
                for( Int da : mv.cross ) { corridor.push_back(PassDescriptor<Int>::RightFace(pd, da)); }
                Idx darc = 0; Point pole { 0, 0 };
                if( !FaceInside(pd, D, F.loop, corridor, darc, pole) ) { note = "The middle pass goes round the back of the sphere, and no face clear of its corridor can be put outside."; return false; }
                Still(D, "Diagram", opt.t_hold, pd, "");
                if( !TurnTo(pd, D, darc, pole, "The next move sweeps the side that holds infinity, so another face is put outside first.", note) ) { return false; }
                return Pass(rec, next, D, note, true);
            }
            if( F.frames.empty() && !turnedQ )
            {
                // No route can be swept to: the strand would cross its own other end on the way, because the
                // side it sweeps holds that end. Put a face of that side outside, and sweep the other side.
                std::vector<Int> corridor { PassDescriptor<Int>::LeftFace(pd, mv.depart) };
                for( Int da : mv.cross ) { corridor.push_back(PassDescriptor<Int>::RightFace(pd, da)); }
                Idx darc = 0; Point pole { 0, 0 };
                if( FaceInside(pd, D, F.loop, corridor, darc, pole) )
                {
                    Still(D, "Diagram", opt.t_hold, pd, "");
                    if( !TurnTo(pd, D, darc, pole, "The next move's strand would sweep across its own other end, so another face is put outside first.", note) ) { return false; }
                    return Pass(rec, next, D, note, true);
                }
            }
            auto over_at = [&]( Idx j, Idx s, Real t ) -> bool
            {
                if( !mv.middlepassQ ) { return overQ; }
                const Idx a = P.still_arc[j][s];
                if( a == None ) { return overQ; }
                int half = -1;
                if( split.count(a) )
                {
                    const Point & c = P.still[j][s], & d = P.still[j][(s + 1) % P.still[j].size()];
                    Real f = 0;
                    half = (along(a, Point{ c[0] + (d[0] - c[0]) * t, c[1] + (d[1] - c[1]) * t }, f) && f > split[a]) ? 1 : 0;
                }
                const auto it = level.find({ a, half });
                return (it == level.end()) ? overQ : (it->second != 'a');        // above the disk: the strand goes under it
            };

            // the frames of the sweep
            std::vector<Polyline> frames = std::move(F.frames);
            std::string method = F.no_sweep;
            if( frames.empty() ) { frames = { P.W, P.route }; }

            // the paths: the stills, the route, the strand
            const int ns = static_cast<int>(P.still.size()), iroute = ns, istrand = ns + 1;
            Scene base;
            base.label = mv.middlepassQ ? "Middle pass move" : "Pass move"; base.note = method; base.easing = Easing::Step;       // a sweep's keyframes are frames: never blended
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
                if( firstQ )            // on W itself the crossings are the diagram's own, at shared points
                {
                    for( Cut c : P.w_cuts ) { if( c.under < 0 ) { c.under = istrand; } if( c.over < 0 ) { c.over = istrand; } k.cuts.push_back(c); }
                }
                else { Weld(k.pts, P.still_closed, moving, istrand, over_at, k.cuts); }
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

            SpinOffs(P.after, P.freed, P.freed_colors, base.crossings_after);
            Settle(P.after, mine, next, D);
            return true;
        }

        /*!@brief Components a move left with no crossing slide to the tray, one after another. */
        void SpinOffs( const PlaneDrawing & after, const std::vector<Polyline> & freed, const std::vector<long long> & colors, int crossings )
        {
            for( Idx f = 0; f < freed.size(); ++f )
            {
                Scene slide = SceneOf(detail::DrawnOf(after), "Spin-off", opt.t_spin, crossings, crossings, "");
                slide.easing = Easing::Smoother;
                slide.keys.push_back(slide.keys[0]);
                for( Idx g = f + 1; g < freed.size(); ++g )         // those still waiting their turn
                {
                    Path p; p.closed = true; p.color = static_cast<int>(colors[g]); slide.paths.push_back(p);
                    std::vector<P2> q; for( const Point & z : freed[g] ) { q.push_back(detail::ToP2(z)); }
                    slide.keys[0].pts.push_back(q); slide.keys[1].pts.push_back(q);
                }
                AddTray(slide, tray, opt.tray);
                std::vector<P2> loop; for( const Point & z : freed[f] ) { loop.push_back(detail::ToP2(z)); }
                AddSpinOff(slide, loop, static_cast<int>(colors[f]), static_cast<int>(tray.size()), opt.tray);
                Push(std::move(slide), false);
                tray.push_back(static_cast<int>(colors[f]));
            }
        }

        /*!@brief A redraw: the drawing reshapes into the view of the record's
         * space curve E; the curve turns by the record's rotation, with depth
         * colouring; the view of the turned curve reshapes into a fresh layout
         * of the next diagram. The view is along (a, b, 1), for the largest
         * tilt of a ladder at which E is seen as this record's diagram and
         * R E as the next one's. False, with a note, if it has to be a cut. */
        bool Redraw( const Record_T & rec, const PD_T * next, PlaneDrawing & D, std::string & note )
        {
            const PD_T & pd = *rec.state;
            std::string why;
            // the rotation: one of the two turns by a third about (1, 1, 1)
            int R[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            {
                const auto at = rec.move->find("rot=");
                if( at == std::string::npos ) { note = "The redraw names no rotation."; return false; }
                std::size_t i = at + 4; int got = 0;
                while( got < 9 && i < rec.move->size() )
                {
                    std::size_t used = 0;
                    try { R[got] = std::stoi(rec.move->substr(i), &used); } catch( const std::exception & ) { break; }
                    ++got; i += used; if( i < rec.move->size() && (*rec.move)[i] == ',' ) { ++i; }
                }
                if( got != 9 ) { note = "The redraw's rotation does not parse."; return false; }
            }
            const bool plusQ  = (R[0] == 0 && R[3] == 1 && R[6] == 0 && R[1] == 0 && R[4] == 0 && R[7] == 1 && R[2] == 1 && R[5] == 0 && R[8] == 0);
            const bool minusQ = (R[0] == 0 && R[3] == 0 && R[6] == 1 && R[1] == 1 && R[4] == 0 && R[7] == 0 && R[2] == 0 && R[5] == 1 && R[8] == 0);
            if( !plusQ && !minusQ ) { note = "The redraw's rotation is not a turn by a third about (1, 1, 1)."; return false; }
            if( rec.embedding.empty() || rec.embedding_ptr.size() < 2 ) { note = "The redraw carries no curve."; return false; }
            if( detail::EmptyQ(D) ) { note = "There is no drawing to start the redraw from."; return false; }

            // the curve, placed over the drawing in hand and at its size
            const Idx nc = rec.embedding_ptr.size() - 1;
            std::vector<std::vector<P3>> E (nc), RE (nc);
            std::vector<long long> colors;
            Real lo[3] = { 1e300, 1e300, 1e300 }, hi[3] = { -1e300, -1e300, -1e300 };
            for( Idx k = 0; k < nc; ++k )
            {
                colors.push_back(k < rec.embedding_colors.size() ? static_cast<long long>(rec.embedding_colors[k]) : static_cast<long long>(k));
                for( Int v = rec.embedding_ptr[k]; v < rec.embedding_ptr[k+1]; ++v )
                {
                    const P3 p { static_cast<Real>(rec.embedding[static_cast<Idx>(3 * v)]), static_cast<Real>(rec.embedding[static_cast<Idx>(3 * v + 1)]), static_cast<Real>(rec.embedding[static_cast<Idx>(3 * v + 2)]) };
                    E[k].push_back(p);
                    lo[0] = std::min(lo[0], p.x); lo[1] = std::min(lo[1], p.y); lo[2] = std::min(lo[2], p.z);
                    hi[0] = std::max(hi[0], p.x); hi[1] = std::max(hi[1], p.y); hi[2] = std::max(hi[2], p.z);
                }
            }
            const auto B = D.Bounds(); const Point cD = B.Centre();
            const Real scale = std::max(B.hi[0] - B.lo[0], B.hi[1] - B.lo[1]) / std::max(std::max(hi[0] - lo[0], hi[1] - lo[1]), 1e-300);
            const P3 c { cD[0], cD[1], 0 };
            Real radius = 0;
            for( Idx k = 0; k < nc; ++k )
            {
                for( P3 & p : E[k] )
                {
                    const Real v[3] = { scale * (p.x - (lo[0] + hi[0]) / 2), scale * (p.y - (lo[1] + hi[1]) / 2), scale * (p.z - (lo[2] + hi[2]) / 2) };
                    p = P3{ c.x + v[0], c.y + v[1], c.z + v[2] };
                    RE[k].push_back(P3{ c.x + R[0] * v[0] + R[1] * v[1] + R[2] * v[2], c.y + R[3] * v[0] + R[4] * v[1] + R[5] * v[2], c.z + R[6] * v[0] + R[7] * v[1] + R[8] * v[2] });
                    radius = std::max(radius, std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
                }
            }

            // the tilt: the largest of a ladder that shows both diagrams
            PlaneDrawing G0, V1; PD_T mine1; std::vector<Polyline> loops1; std::vector<long long> loop_colors1;
            Real a = 0, b = 0; bool foundQ = false; std::string last = "no tilt was tried";
            for( Real N : { 3.0, 4.0, 6.0, 8.0, 12.0, 16.0, 24.0, 32.0, 48.0, 64.0, 96.0, 128.0 } )
            {
                for( Real M : { 3.0, 5.0, 7.0 } )
                {
                    a = 1 / N; b = a / M;
                    PlaneDrawing V0; std::vector<Polyline> loops0; std::vector<long long> lc0;
                    if( !ViewDrawing(E, colors, a, b, V0, loops0, lc0, why) ) { last = why; continue; }
                    if( !loops0.empty() || detail::EmptyQ(V0) ) { last = "a component of the curve crosses nothing in the view"; continue; }
                    std::vector<Idx> cw = detail::ClockwiseFaces(V0);
                    if( cw.size() != 1 ) { last = "the view of the curve is not connected"; continue; }
                    V0.exterior_arc = cw[0] / 2; V0.exterior_forwardQ = (cw[0] % 2) != 0;
                    const PD_T mine0 = ToPlanarDiagram<PD_T>(V0, why);
                    if( !mine0.ValidQ() || !Relabel(V0, mine0, pd, G0, why) ) { last = "the view of the curve is not the record's diagram: " + why; continue; }
                    if( !ViewDrawing(RE, colors, a, b, V1, loops1, loop_colors1, why) ) { last = why; continue; }
                    mine1 = PD_T();
                    if( !detail::EmptyQ(V1) )
                    {
                        cw = detail::ClockwiseFaces(V1);
                        if( cw.size() != 1 ) { last = "the view of the turned curve is not connected"; continue; }
                        V1.exterior_arc = cw[0] / 2; V1.exterior_forwardQ = (cw[0] % 2) != 0;
                        mine1 = ToPlanarDiagram<PD_T>(V1, why);
                        PlaneDrawing tmp;
                        if( !mine1.ValidQ() || !next || !HasCrossings(*next) || !Relabel(V1, mine1, *next, tmp, why) ) { last = "the view of the turned curve is not the next diagram: " + why; continue; }
                    }
                    else if( next && HasCrossings(*next) ) { last = "the turned curve shows no crossing, and the next diagram has some"; continue; }
                    foundQ = true; break;
                }
                if( foundQ ) { break; }
            }
            if( !foundQ ) { note = "No view shows the redraw's two diagrams (" + last + ")."; return false; }

            // the drawing in hand must have the view's exterior: if not, turn the sphere first
            const Int want = PassDescriptor<Int>::LeftFace(pd, static_cast<Int>(G0.ExteriorDarc()));
            if( want != PassDescriptor<Int>::LeftFace(pd, static_cast<Int>(D.ExteriorDarc())) )
            {
                std::vector<Int> avoid;
                for( Int f = 0; f < pd.FaceCount(); ++f ) { if( f != want ) { avoid.push_back(f); } }
                const Real m = B.Size();
                const Polyline everywhere { { B.lo[0] - m, B.lo[1] - m }, { B.hi[0] + m, B.lo[1] - m }, { B.hi[0] + m, B.hi[1] + m }, { B.lo[0] - m, B.hi[1] + m } };
                Idx darc = 0; Point pole { 0, 0 };
                if( !FaceInside(pd, D, everywhere, avoid, darc, pole) ) { note = "The redraw's curve was recorded on another exterior face, and that face cannot be put outside."; return false; }
                Still(D, "Diagram", opt.t_hold, pd, "");
                if( !TurnTo(pd, D, darc, pole, "The redraw's curve was recorded with another face outside, so that face is put outside first.", note) ) { return false; }
            }

            // one box for the reshape in, the turn, and what it leaves
            ViewBox box { c.x - 1.15 * radius, c.y - 1.15 * radius, c.x + 1.15 * radius, c.y + 1.15 * radius };
            auto grow = [&]( const PlaneDrawing & X ) { if( detail::EmptyQ(X) ) { return; } const auto Bx = X.Bounds(); box.x0 = std::min(box.x0, Bx.lo[0]); box.y0 = std::min(box.y0, Bx.lo[1]); box.x1 = std::max(box.x1, Bx.hi[0]); box.y1 = std::max(box.y1, Bx.hi[1]); };
            grow(D); grow(G0); grow(V1);
            box.x0 -= opt.pad; box.y0 -= opt.pad; box.x1 += opt.pad; box.y1 += opt.pad;
            Still(D, "Diagram", opt.t_hold, pd, "");
            if( !MorphTo(D, G0, pd.CrossingCount(), note, &box) ) { return false; }

            Scene sc;
            sc.kind = Scene::Kind::Space; sc.label = "Redraw move";
            sc.crossings_before = static_cast<int>(pd.CrossingCount());
            sc.crossings_after  = (next && next->ValidQ()) ? static_cast<int>(next->CrossingCount()) : 0;
            sc.tilt_a = a; sc.tilt_b = b; sc.depth = radius;
            Stage st; st.map = Map3::Affine; st.centre = c; st.axis = P3{ 1, 1, 1 };
            for( Idx k = 0; k < nc; ++k ) { Curve3 cv; cv.color = static_cast<int>(colors[k]); cv.pts = E[k]; st.curves.push_back(std::move(cv)); }
            const Real third = (plusQ ? 1 : -1) * 2.0943951023931953;
            Stage in = st;   in.duration = opt.t_fog;      in.fog0 = 0; in.fog1 = 1;
            Stage turn = st; turn.duration = opt.t_rotate; turn.angle1 = third; turn.fog0 = turn.fog1 = 1;
            Stage out = st;  out.duration = opt.t_fog;     out.angle0 = out.angle1 = third; out.fog0 = 1; out.fog1 = 0;
            sc.stages = { in, turn, out };
            sc.duration = 2 * opt.t_fog + opt.t_rotate;
            Push(std::move(sc), false, &box);

            SpinOffs(V1, loops1, loop_colors1, (next && next->ValidQ()) ? static_cast<int>(next->CrossingCount()) : 0);
            Settle(V1, mine1, next, D);
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
