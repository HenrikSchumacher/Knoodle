#pragma once

// Spin-offs and their tray (docs/animate-design.md). A component that a move
// leaves with no crossings keeps its colour, becomes a rounded square, and
// slides to a tray: a stack of such squares that starts at the bottom left of
// the picture and fills to the right, then upward. The tray is on the screen,
// not in the world, so the camera does not move it. A square carries no mark:
// its colour says which component it was, its place says when it came free.
//
// Standard library only.

#include "Scene.hpp"

#include <cmath>
#include <vector>

namespace Knoodle::Animate
{
    struct TrayOptions
    {
        Real aspect = 16.0 / 9.0;   // the picture's width over its height
        Real size   = 0.045;        // a square's side, in picture heights
        Real gap    = 0.015;        // between squares, and ...
        Real margin = 0.025;        // ... from the picture's edges
        Real corner = 0.28;         // corner radius, as a share of the side
        int  points = 48;           // points on a square's outline (a multiple of 4)
        int  layer  = 100;          // above everything in the world
    };

    /*!@brief The centre of tray place `slot` (0 is the bottom left), in screen
     * coordinates. */
    inline P2 TrayCentre( int slot, const TrayOptions & o = TrayOptions() )
    {
        const Real pitch = o.size + o.gap;
        const int  per_row = std::max(1, int(std::floor((o.aspect - 2 * o.margin + o.gap) / pitch)));
        return P2{ o.margin + o.size / 2 + (slot % per_row) * pitch, o.margin + o.size / 2 + (slot / per_row) * pitch };
    }

    /*!@brief A rounded square of side `size` about `centre`: `points` points,
     * evenly spaced along it, counterclockwise if `ccw`, the first one in the
     * direction `angle` from the centre. */
    inline std::vector<P2> RoundedSquare( const P2 & centre, Real size, Real corner, int points, bool ccw = true, Real angle = 0 )
    {
        const Real h = size / 2, r = std::min(corner * size, h), s = h - r;       // s: half a straight side
        const Real quarter = 2 * s + 1.5707963267948966 * r, total = 4 * quarter;
        // by length t from the middle of the right side, counterclockwise
        auto at = [&]( Real t )
        {
            t = std::fmod(std::fmod(t, total) + total, total);
            const int  q = int(t / quarter);
            Real u = t - q * quarter, x, y;
            if( u <= s )            { x = h; y = u; }
            else if( u <= s + 1.5707963267948966 * r ) { const Real a = (u - s) / r; x = s + r * std::cos(a); y = s + r * std::sin(a); }
            else                    { x = s - (u - s - 1.5707963267948966 * r); y = h; }
            for( int i = 0; i < q; ++i ) { const Real nx = -y; y = x; x = nx; }       // a quarter turn per side
            return P2{ x, y };
        };
        // the length at which the outline is in the direction `angle`
        Real t0 = 0, best = 1e300;
        for( int i = 0; i < 720; ++i )
        {
            const P2 p = at(total * i / 720);
            Real d = std::abs(std::atan2(p.y, p.x) - angle);
            d = std::min(d, 6.283185307179586 - d);
            if( d < best ) { best = d; t0 = total * i / 720; }
        }
        std::vector<P2> out;
        for( int i = 0; i < points; ++i )
        {
            const P2 p = at(t0 + (ccw ? 1 : -1) * total * i / points);
            out.push_back(P2{ centre.x + p.x, centre.y + p.y });
        }
        return out;
    }

    /*!@brief Add the tray as it stands to a flat scene: one square per colour
     * in `colors`, in the order they came free. */
    inline void AddTray( Scene & sc, const std::vector<int> & colors, const TrayOptions & o = TrayOptions() )
    {
        for( std::size_t k = 0; k < colors.size(); ++k )
        {
            Path p; p.role = Role::Token; p.color = colors[k]; p.closed = true; p.round = true; p.layer = o.layer; p.space = Space::Screen;
            sc.paths.push_back(p);
            const std::vector<P2> sq = RoundedSquare(TrayCentre(int(k), o), o.size, o.corner, o.points);
            for( Key & key : sc.keys ) { key.pts.push_back(sq); }
        }
    }

    /*!@brief Add a spin-off to a flat scene of exactly two keyframes: the
     * closed curve `loop` (world coordinates) becomes the square at tray place
     * `slot` over the scene. Returns false, changing nothing, if the scene
     * does not have two keyframes or the loop has under three points. */
    inline bool AddSpinOff( Scene & sc, const std::vector<P2> & loop, int color, int slot, const TrayOptions & o = TrayOptions() )
    {
        if( sc.keys.size() != 2 || loop.size() < 3 ) { return false; }
        const std::size_t n = loop.size();
        std::vector<Real> cum (n + 1, 0.0);
        Real area = 0, cx = 0, cy = 0;
        for( std::size_t i = 0; i < n; ++i )
        {
            const P2 & a = loop[i], & b = loop[(i + 1) % n];
            cum[i+1] = cum[i] + std::hypot(b.x - a.x, b.y - a.y);
            area += a.x * b.y - a.y * b.x; cx += a.x; cy += a.y;
        }
        if( cum[n] <= 0 ) { return false; }
        cx /= Real(n); cy /= Real(n);
        // the loop at `points` places evenly spaced along it, from its first point
        std::vector<P2> start;
        std::size_t j = 0;
        for( int i = 0; i < o.points; ++i )
        {
            const Real s = cum[n] * i / o.points;
            while( j + 1 < n && cum[j+1] <= s ) { ++j; }
            const Real len = cum[j+1] - cum[j], t = (len > 0) ? (s - cum[j]) / len : 0;
            const P2 & a = loop[j], & b = loop[(j + 1) % n];
            start.push_back(P2{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t });
        }
        // the square turns the same way and starts in the same direction, so the morph does not twist
        const std::vector<P2> square = RoundedSquare(TrayCentre(slot, o), o.size, o.corner, o.points, area > 0, std::atan2(loop[0].y - cy, loop[0].x - cx));
        Path p; p.role = Role::Token; p.color = color; p.closed = true; p.round = true; p.layer = o.layer; p.space = Space::WorldToScreen;
        sc.paths.push_back(p);
        sc.keys[0].pts.push_back(start);
        sc.keys[1].pts.push_back(square);
        return true;
    }

} // namespace Knoodle::Animate
