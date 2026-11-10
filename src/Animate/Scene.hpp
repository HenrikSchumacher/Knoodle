#pragma once

// The animator's data model (docs/animate-design.md): an animation is a list
// of scenes, played in order. A scene holds geometry, timing and ROLES. It
// holds no colour, opacity or width: those come from a `Style`, keyed by role.
//
// There are two kinds of scene.
//
//   Flat   polylines in the plane, given at one or more keyframes. Reshapes,
//          pass sweeps, curl removals, spin-offs and stills are all flat.
//   Space  a closed space curve under a staged motion, seen from a fixed
//          oblique camera. A redraw (lift, rotate) and a turn of the sphere
//          (roll up, turn, unroll) are space scenes. They are stored as
//          parameters, not as sampled frames.
//
// Standard library only: no Knoodle types, no files, no printing, no globals.

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Knoodle::Animate
{
    using Real = double;

    struct P2 { Real x = 0, y = 0; };
    struct P3 { Real x = 0, y = 0, z = 0; };
    struct ViewBox { Real x0 = 0, y0 = 0, x1 = 1, y1 = 1; };

    /*!@brief What a path is; the style sheet turns it into a look. */
    enum class Role : std::uint8_t { Strand, Moving, Route, Disk, Loop, Token };

    /*!@brief Where a path's coordinates live. `Screen` is the view itself: the
     * origin at its bottom left, one unit its height (the tray lives there).
     * `WorldToScreen` paths start in the world at the first key and end on
     * the screen at the last (a spin-off sliding to the tray). */
    enum class Space : std::uint8_t { World, Screen, WorldToScreen };

    enum class Easing : std::uint8_t { Linear, Smoother };

    /*!@brief s(u) = u^3 (6u^2 - 15u + 10), clamped: zero speed and zero
     * acceleration at both ends. */
    inline Real Smoother( Real u )
    {
        u = (u < 0) ? 0 : ((u > 1) ? 1 : u);
        return u * u * u * (u * (6 * u - 15) + 10);
    }

    //==================================================================
    // Flat scenes
    //==================================================================

    struct Path
    {
        Role  role   = Role::Strand;
        int   color  = -1;      // component colour (the trace's), or -1 for none
        bool  closed = false;
        bool  round  = false;   // round joins (smooth sweeps), else miter
        int   layer  = 0;       // drawn in increasing order; a fill before a path of its layer
        Real  alpha0 = 1;       // opacity at the scene's start ...
        Real  alpha1 = 1;       // ... and at its end
        Space space  = Space::World;
    };

    /*!@brief At vertex `iu` of path `under`, that path passes under path
     * `over` at its vertex `io`. The renderer cuts a gap in the under path. */
    struct Cut { int under = 0, iu = 0, over = 0, io = 0; };

    /*!@brief A filled region: the polygon made by joining paths end to end,
     * each forwards or reversed (a pass disk is the moving strand followed by
     * its route reversed). */
    struct Fill
    {
        Role role  = Role::Disk;
        int  layer = 0;
        std::vector<std::pair<int,bool>> parts;   // (path, reversed)
    };

    /*!@brief One keyframe: a polyline per path, and that keyframe's cuts.
     * Two neighbouring keyframes are interpolated only if every path has the
     * same number of points in both; otherwise the nearer one is shown. */
    struct Key
    {
        std::vector<std::vector<P2>> pts;
        std::vector<Cut>             cuts;
    };

    //==================================================================
    // Space scenes
    //==================================================================

    /*!@brief A closed curve. Under `Map3::Affine` its points are (x, y, z).
     * Under `Map3::Roll` they are (x, y, h): a point of the plane and a small
     * height along the surface normal. */
    struct Curve3 { int color = -1; std::vector<P3> pts; };

    /*!@brief `Roll` only: vertex `ia` of curve `a` and vertex `ib` of curve
     * `b` are the two passages of one crossing of the diagram. Whichever is
     * farther from the camera in a frame is cut there. (Under `Affine` the
     * crossings are found from the geometry, frame by frame.) */
    struct Crossing3 { int a = 0, ia = 0, b = 0, ib = 0; };

    enum class Map3 : std::uint8_t { Affine, Roll };

    /*!@brief One stage of a space scene; every parameter runs from its `0`
     * value to its `1` value with `Smoother` over `duration`.
     *
     * Affine: X = centre + Rot(axis, angle) * (x - cx, y - cy, lift * (z - cz)).
     *
     * Roll:   q = Finish(x, y) (below), relative to centre; X = the point of
     *         the sphere of radius `radius / roll` tangent to the plane at the
     *         centre that q lifts to by inverse stereographic projection from
     *         the sphere's top, moved by h along the inward normal; then
     *         turned by `angle` about `axis` through the sphere's centre.
     *         roll = 0 is the flat plane.
     *
     * Finish: the identity at fin = 0. At fin = 1: a radial map about `fin_o`
     *         (r -> r for r <= fin_rmin, else fin_rmin (1 + ln(r / fin_rmin))),
     *         then q -> fin_scale * (q - fin_shift). In between, each of the
     *         two is blended linearly with the identity, which keeps the map
     *         one to one. It evens out the drawing a turn leaves.
     */
    struct Stage
    {
        Map3 map      = Map3::Affine;
        Real duration = 1;
        std::vector<Curve3>    curves;
        std::vector<Crossing3> crossings;

        P3   centre;
        P3   axis { 0, 0, 1 };
        Real angle0 = 0, angle1 = 0;        // radians, right-handed about `axis`
        Real lift0  = 1, lift1  = 1;        // Affine
        Real radius = 1;                    // Roll
        Real roll0  = 0, roll1  = 0;
        P2   fin_o, fin_shift;
        Real fin_rmin = 0, fin_scale = 1;
        Real fin0 = 0, fin1 = 0;
        Real fog0 = 0, fog1 = 0;            // strength of the depth colouring
    };

    //==================================================================
    // Scenes and animations
    //==================================================================

    struct Scene
    {
        enum class Kind : std::uint8_t { Flat, Space };

        Kind        kind = Kind::Flat;
        std::string label;                  // "Pass move", "Reshape diagram", ...
        std::string note;                   // why the picture departs from the plain one, if it does
        int         crossings_before = 0, crossings_after = 0;
        Real        duration = 1;           // Space: the sum of its stages' durations
        Easing      easing   = Easing::Smoother;
        ViewBox         view0, view1;           // the camera's box at the start and at the end

        // Flat
        std::vector<Path> paths;
        std::vector<Fill> fills;
        std::vector<Key>  keys;

        // Space: drawn at (x - tilt_a z, y - tilt_b z); larger z is nearer.
        std::vector<Stage> stages;
        Real tilt_a = 0, tilt_b = 0;
        Real depth  = 1;                    // the scale of the depth colouring
    };

    struct Animation
    {
        std::string        source;
        std::vector<Scene> scenes;

        Real Duration() const
        {
            Real t = 0;
            for( const Scene & s : scenes ) { t += s.duration; }
            return t;
        }

        /*!@brief The scene playing at time `t`, and the time into it. Past the
         * end, the last scene at its end. Returns false on an empty animation. */
        bool Locate( Real t, std::size_t & scene, Real & local ) const
        {
            if( scenes.empty() ) { return false; }
            if( t < 0 ) { t = 0; }
            for( std::size_t i = 0; i < scenes.size(); ++i )
            {
                if( t < scenes[i].duration || i + 1 == scenes.size() )
                {
                    scene = i;
                    local = (t < scenes[i].duration) ? t : scenes[i].duration;
                    return true;
                }
                t -= scenes[i].duration;
            }
            return false;
        }
    };

    //==================================================================
    // Style
    //==================================================================

    struct RGBA { Real r = 0, g = 0, b = 0, a = 1; };

    inline RGBA Mix( const RGBA & p, const RGBA & q, Real f )
    {
        return RGBA{ p.r + (q.r - p.r) * f, p.g + (q.g - p.g) * f, p.b + (q.b - p.b) * f, p.a + (q.a - p.a) * f };
    }

    /*!@brief How roles look. Everything a viewer might want to change is here
     * and nowhere else. */
    struct Style
    {
        RGBA background { 1, 1, 1, 1 };
        RGBA ink        { 0, 0, 0, 1 };                  // strands with no component colour
        RGBA moving     { 0.106, 0.122, 0.541, 1 };      // the moving strand; a contracting loop
        RGBA route      { 0.533, 0.545, 0.784, 0.9 };
        RGBA disk       { 0.820, 0.827, 0.949, 0.75 };
        RGBA fog        { 0.86, 0.87, 0.95, 1 };         // what far strands fade toward
        std::vector<RGBA> components;                    // by colour, cyclically; empty: all ink

        Real stroke      = 0.28;        // stroke width in world units ...
        Real min_stroke  = 1.0 / 128;   // ... but at least this share of the picture's height
        Real route_width = 0.8;         // relative to the stroke
        Real gap         = 1.7;         // half a crossing gap, in stroke widths
        Real fog_depth   = 0.55;        // how far toward `fog` the farthest piece goes
        Real fog_width   = 0.35;        // nearer thicker, farther thinner, by this share
        Real margin      = 0.06;        // of the picture, kept clear round the camera's box
        Real token_stroke = 1.0 / 300;  // stroke width on the screen (the tray), as a share of the picture's height

        RGBA Of( Role role, int color ) const
        {
            switch( role )
            {
                case Role::Moving: case Role::Loop: return moving;
                case Role::Route:                   return route;
                case Role::Disk:                    return disk;
                default: break;
            }
            if( color < 0 || components.empty() ) { return ink; }
            return components[std::size_t(color) % components.size()];
        }

        /*!@brief The talk house style: black line art on white, indigo accent,
         * lavender fills. Also the default. */
        static Style Talk() { return Style{}; }

        static Style Light()
        {
            Style s;
            s.ink    = RGBA{ 0.18, 0.18, 0.18, 1 };
            s.moving = RGBA{ 0.0, 0.55, 0.62, 1 };
            s.route  = RGBA{ 0.86, 0.58, 0.0, 0.8 };
            s.disk   = RGBA{ 0.80, 0.80, 0.80, 0.55 };
            s.fog    = RGBA{ 0.79, 0.84, 0.93, 1 };
            s.components = { s.ink, RGBA{ 0.11, 0.12, 0.54, 1 }, RGBA{ 0.80, 0.0, 0.17, 1 }, RGBA{ 0.0, 0.47, 0.24, 1 } };
            return s;
        }

        static Style Dark()
        {
            Style s;
            s.background = RGBA{ 0.055, 0.078, 0.102, 1 };
            s.ink    = RGBA{ 0.725, 0.776, 0.827, 1 };
            s.moving = RGBA{ 0.373, 0.843, 0.843, 1 };
            s.route  = RGBA{ 1.0, 0.843, 0.373, 0.8 };
            s.disk   = RGBA{ 0.22, 0.25, 0.29, 0.55 };
            s.fog    = RGBA{ 0.122, 0.208, 0.322, 1 };
            s.components = { s.ink, RGBA{ 0.60, 0.65, 1.0, 1 }, RGBA{ 1.0, 0.55, 0.55, 1 }, RGBA{ 0.55, 0.9, 0.6, 1 } };
            return s;
        }
    };

} // namespace Knoodle::Animate
