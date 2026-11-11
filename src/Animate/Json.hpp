#pragma once

// ToJSON: an animation as one JSON document, "knoodle-animation" version 2.
// The keys follow Scene.hpp field for field. Coordinates carry four decimals,
// angles and axes six. Standard library only; there is no reader in C++.

#include "Scene.hpp"

#include <cstdio>
#include <string>

namespace Knoodle::Animate
{
    namespace detail
    {
        struct JsonOut
        {
            std::string s;

            void Num( Real v, int decimals = 4 )
            {
                char buf[64];
                std::snprintf(buf, sizeof buf, "%.*f", decimals, std::isfinite(v) ? v : 0.0);
                std::string t (buf);
                if( t.find('.') != std::string::npos )
                {
                    while( t.back() == '0' ) { t.pop_back(); }
                    if( t.back() == '.' ) { t.pop_back(); }
                }
                s += (t == "-0") ? "0" : t;
            }
            void Str( const std::string & v )
            {
                s += '"';
                for( char ch : v )
                {
                    if( ch == '"' || ch == '\\' ) { s += '\\'; s += ch; }
                    else if( ch == '\n' ) { s += "\\n"; }
                    else if( static_cast<unsigned char>(ch) < 0x20 ) { s += ' '; }
                    else { s += ch; }
                }
                s += '"';
            }
            void Key( const char * k, bool first = false ) { if( !first ) { s += ','; } s += '"'; s += k; s += "\":"; }
            void Pair( Real a, Real b, int d = 4 ) { s += '['; Num(a, d); s += ','; Num(b, d); s += ']'; }
            void Triple( const P3 & p, int d = 4 ) { s += '['; Num(p.x, d); s += ','; Num(p.y, d); s += ','; Num(p.z, d); s += ']'; }
            void Box4( const ViewBox & b ) { s += '['; Num(b.x0); s += ','; Num(b.y0); s += ','; Num(b.x1); s += ','; Num(b.y1); s += ']'; }
        };

        inline const char * Name( Role r )
        {
            switch( r )
            {
                case Role::Strand: return "Strand"; case Role::Moving: return "Moving"; case Role::Route: return "Route";
                case Role::Disk:   return "Disk";   case Role::Loop:   return "Loop";   default:          return "Token";
            }
        }
        inline const char * Name( Space v ) { return (v == Space::World) ? "World" : ((v == Space::Screen) ? "Screen" : "WorldToScreen"); }
    }

    inline std::string ToJSON( const Animation & A )
    {
        detail::JsonOut o;
        o.s += "{\"Format\":\"knoodle-animation\",\"Version\":2";
        o.Key("Source"); o.Str(A.source);
        o.Key("Scenes"); o.s += '[';
        for( std::size_t n = 0; n < A.scenes.size(); ++n )
        {
            const Scene & sc = A.scenes[n];
            const bool flatQ = (sc.kind == Scene::Kind::Flat);
            o.s += (n ? ",\n{" : "\n{");
            o.Key("Kind", true); o.Str(flatQ ? "Flat" : "Space");
            o.Key("Label"); o.Str(sc.label);
            if( !sc.note.empty() ) { o.Key("Note"); o.Str(sc.note); }
            o.Key("Crossings"); o.Pair(sc.crossings_before, sc.crossings_after, 0);
            o.Key("Duration"); o.Num(sc.duration);
            o.Key("Easing"); o.Str((sc.easing == Easing::Smoother) ? "Smoother" : (sc.easing == Easing::Step) ? "Step" : "Linear");
            o.Key("View"); o.s += '['; o.Box4(sc.view0); o.s += ','; o.Box4(sc.view1); o.s += ']';
            if( flatQ )
            {
                o.Key("Paths"); o.s += '[';
                for( std::size_t j = 0; j < sc.paths.size(); ++j )
                {
                    const Path & p = sc.paths[j];
                    o.s += (j ? ",{" : "{");
                    o.Key("Role", true); o.Str(detail::Name(p.role));
                    o.Key("Color"); o.Num(p.color, 0);
                    o.Key("Closed"); o.s += p.closed ? "true" : "false";
                    o.Key("Round");  o.s += p.round ? "true" : "false";
                    o.Key("Layer"); o.Num(p.layer, 0);
                    o.Key("Alpha"); o.Pair(p.alpha0, p.alpha1);
                    o.Key("Space"); o.Str(detail::Name(p.space));
                    o.s += '}';
                }
                o.s += ']';
                o.Key("Fills"); o.s += '[';
                for( std::size_t j = 0; j < sc.fills.size(); ++j )
                {
                    const Fill & f = sc.fills[j];
                    o.s += (j ? ",{" : "{");
                    o.Key("Role", true); o.Str(detail::Name(f.role));
                    o.Key("Layer"); o.Num(f.layer, 0);
                    o.Key("Parts"); o.s += '[';
                    for( std::size_t i = 0; i < f.parts.size(); ++i ) { if( i ) { o.s += ','; } o.s += '['; o.Num(f.parts[i].first, 0); o.s += f.parts[i].second ? ",true]" : ",false]"; }
                    o.s += "]}";
                }
                o.s += ']';
                o.Key("Keys"); o.s += '[';
                for( std::size_t k = 0; k < sc.keys.size(); ++k )
                {
                    const Key & key = sc.keys[k];
                    o.s += (k ? ",\n {" : "\n {");
                    o.Key("Points", true); o.s += '[';
                    for( std::size_t j = 0; j < key.pts.size(); ++j )
                    {
                        o.s += (j ? ",[" : "[");
                        for( std::size_t i = 0; i < key.pts[j].size(); ++i ) { if( i ) { o.s += ','; } o.Pair(key.pts[j][i].x, key.pts[j][i].y); }
                        o.s += ']';
                    }
                    o.s += ']';
                    o.Key("Cuts"); o.s += '[';
                    for( std::size_t i = 0; i < key.cuts.size(); ++i )
                    {
                        const Cut & c = key.cuts[i];
                        if( i ) { o.s += ','; }
                        o.s += '['; o.Num(c.under, 0); o.s += ','; o.Num(c.iu, 0); o.s += ','; o.Num(c.over, 0); o.s += ','; o.Num(c.io, 0); o.s += ']';
                    }
                    o.s += "]}";
                }
                o.s += ']';
            }
            else
            {
                o.Key("Tilt"); o.Pair(sc.tilt_a, sc.tilt_b, 6);
                o.Key("Depth"); o.Num(sc.depth);
                o.Key("Stages"); o.s += '[';
                for( std::size_t k = 0; k < sc.stages.size(); ++k )
                {
                    const Stage & st = sc.stages[k];
                    o.s += (k ? ",\n {" : "\n {");
                    o.Key("Map", true); o.Str((st.map == Map3::Affine) ? "Affine" : "Roll");
                    o.Key("Duration"); o.Num(st.duration);
                    o.Key("Curves"); o.s += '[';
                    for( std::size_t c = 0; c < st.curves.size(); ++c )
                    {
                        o.s += (c ? ",{" : "{");
                        o.Key("Color", true); o.Num(st.curves[c].color, 0);
                        o.Key("Points"); o.s += '[';
                        for( std::size_t i = 0; i < st.curves[c].pts.size(); ++i ) { if( i ) { o.s += ','; } o.Triple(st.curves[c].pts[i]); }
                        o.s += "]}";
                    }
                    o.s += ']';
                    o.Key("Crossings"); o.s += '[';
                    for( std::size_t i = 0; i < st.crossings.size(); ++i )
                    {
                        const Crossing3 & x = st.crossings[i];
                        if( i ) { o.s += ','; }
                        o.s += '['; o.Num(x.a, 0); o.s += ','; o.Num(x.ia, 0); o.s += ','; o.Num(x.b, 0); o.s += ','; o.Num(x.ib, 0); o.s += ']';
                    }
                    o.s += ']';
                    o.Key("Centre"); o.Triple(st.centre);
                    o.Key("Axis");   o.Triple(st.axis, 6);
                    o.Key("Angle");  o.Pair(st.angle0, st.angle1, 6);
                    o.Key("Lift");   o.Pair(st.lift0, st.lift1, 6);
                    o.Key("Radius"); o.Num(st.radius);
                    o.Key("Roll");   o.Pair(st.roll0, st.roll1, 6);
                    o.Key("Finish"); o.s += '{';
                    o.Key("Origin", true); o.Pair(st.fin_o.x, st.fin_o.y, 6);
                    o.Key("Rmin");   o.Num(st.fin_rmin, 6);
                    o.Key("Scale");  o.Num(st.fin_scale, 6);
                    o.Key("Shift");  o.Pair(st.fin_shift.x, st.fin_shift.y, 6);
                    o.Key("Amount"); o.Pair(st.fin0, st.fin1, 6);
                    o.Key("Undo");   o.Num(st.unfinish, 6);
                    o.Key("Blend");  o.s += (st.blend ? "true" : "false");
                    o.s += '}';
                    o.Key("Fog");    o.Pair(st.fog0, st.fog1);
                    if( !st.marks.empty() )
                    {
                        o.Key("Marks"); o.s += '[';
                        for( std::size_t c = 0; c < st.marks.size(); ++c )
                        {
                            o.s += (c ? ",[" : "[");
                            for( std::size_t i = 0; i < st.marks[c].pts.size(); ++i ) { if( i ) { o.s += ','; } o.Triple(st.marks[c].pts[i]); }
                            o.s += ']';
                        }
                        o.s += ']';
                        o.Key("MarkIn");   o.Pair(st.mark_in0, st.mark_in1);
                        o.Key("MarkOut");  o.Pair(st.mark_out0, st.mark_out1);
                        o.Key("MarkLine"); o.Pair(st.mark_line0, st.mark_line1);
                    }
                    o.s += '}';
                }
                o.s += ']';
            }
            o.s += '}';
        }
        o.s += "\n]}\n";
        return o.s;
    }

} // namespace Knoodle::Animate
