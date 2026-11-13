#pragma once

// A small antialiased rasterizer for the display list FrameAt returns: enough
// to write movie frames with no graphics library. Fills use the nonzero rule,
// four sub-scanlines per pixel row with exact horizontal coverage. A stroke is
// the union of its segments' quads and its joins, covered once, so a
// translucent stroke does not darken where its pieces overlap.
//
// Standard library only. No text: movies carry no captions.

#include "Frame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Knoodle::Animate
{
    struct Image
    {
        int W = 0, H = 0;
        std::vector<float> rgb;     // 3 per pixel, row by row from the top, each in [0, 1]

        Image( int w, int h, const RGBA & bg ) : W(w), H(h), rgb(std::size_t(w) * std::size_t(h) * 3)
        {
            for( std::size_t i = 0; i < rgb.size(); i += 3 ) { rgb[i] = float(bg.r); rgb[i+1] = float(bg.g); rgb[i+2] = float(bg.b); }
        }

        /*!@brief The picture as rgb24 bytes, the layout `ffmpeg -f rawvideo
         * -pix_fmt rgb24` reads. */
        std::vector<std::uint8_t> Bytes() const
        {
            std::vector<std::uint8_t> out (rgb.size());
            for( std::size_t i = 0; i < rgb.size(); ++i ) { out[i] = std::uint8_t(std::lround(std::min(1.0f, std::max(0.0f, rgb[i])) * 255.0f)); }
            return out;
        }
    };

    namespace detail
    {
        using Poly = std::vector<P2>;

        /*!@brief Blend `color` over the union (nonzero rule) of `polys`. */
        inline void FillPolys( Image & img, const std::vector<Poly> & polys, const RGBA & color )
        {
            constexpr int SUB = 4;
            struct Edge { Real x0, y0, x1, y1; int dir; };
            std::vector<Edge> edges;
            Real ymin = 1e300, ymax = -1e300, xmin = 1e300, xmax = -1e300;
            for( const Poly & p : polys )
            {
                for( std::size_t i = 0; i < p.size(); ++i )
                {
                    const P2 & a = p[i], & b = p[(i + 1) % p.size()];
                    xmin = std::min(xmin, a.x); xmax = std::max(xmax, a.x);
                    ymin = std::min(ymin, a.y); ymax = std::max(ymax, a.y);
                    if( a.y == b.y ) { continue; }
                    if( a.y < b.y ) { edges.push_back(Edge{ a.x, a.y, b.x, b.y, +1 }); }
                    else            { edges.push_back(Edge{ b.x, b.y, a.x, a.y, -1 }); }
                }
            }
            if( edges.empty() || color.a <= 0 ) { return; }
            const int r0 = std::max(0, int(std::floor(ymin))), r1 = std::min(img.H - 1, int(std::floor(ymax)));
            const int c0 = std::max(0, int(std::floor(xmin))), c1 = std::min(img.W - 1, int(std::floor(xmax)));
            if( r0 > r1 || c0 > c1 ) { return; }
            std::vector<float> cov (std::size_t(c1 - c0 + 1));
            std::vector<std::pair<Real,int>> xs;
            for( int row = r0; row <= r1; ++row )
            {
                std::fill(cov.begin(), cov.end(), 0.0f);
                bool any = false;
                for( int k = 0; k < SUB; ++k )
                {
                    const Real sy = row + (k + 0.5) / SUB;
                    xs.clear();
                    for( const Edge & e : edges ) { if( e.y0 <= sy && sy < e.y1 ) { xs.emplace_back(e.x0 + (sy - e.y0) * (e.x1 - e.x0) / (e.y1 - e.y0), e.dir); } }
                    if( xs.empty() ) { continue; }
                    std::sort(xs.begin(), xs.end());
                    int wind = 0;
                    for( std::size_t i = 0; i < xs.size(); ++i )
                    {
                        if( wind != 0 )
                        {
                            const Real a = std::max(xs[i-1].first, Real(c0)), b = std::min(xs[i].first, Real(c1 + 1));
                            if( b > a )
                            {
                                any = true;
                                const int ia = int(std::floor(a)), ib = std::min(c1, int(std::floor(b)));
                                if( ia == ib ) { cov[std::size_t(ia - c0)] += float((b - a) / SUB); }
                                else
                                {
                                    cov[std::size_t(ia - c0)] += float((ia + 1 - a) / SUB);
                                    for( int x = ia + 1; x < ib; ++x ) { cov[std::size_t(x - c0)] += 1.0f / SUB; }
                                    cov[std::size_t(ib - c0)] += float((b - ib) / SUB);
                                }
                            }
                        }
                        wind += xs[i].second;
                    }
                }
                if( !any ) { continue; }
                float * px = &img.rgb[(std::size_t(row) * std::size_t(img.W) + std::size_t(c0)) * 3];
                for( std::size_t x = 0; x < cov.size(); ++x, px += 3 )
                {
                    const float a = float(color.a) * std::min(1.0f, cov[x]);
                    if( a <= 0 ) { continue; }
                    px[0] += (float(color.r) - px[0]) * a; px[1] += (float(color.g) - px[1]) * a; px[2] += (float(color.b) - px[2]) * a;
                }
            }
        }

        inline void Oriented( std::vector<Poly> & out, Poly p )
        {
            Real area = 0;
            for( std::size_t i = 0; i < p.size(); ++i ) { const P2 & a = p[i], & b = p[(i + 1) % p.size()]; area += a.x * b.y - a.y * b.x; }
            if( area == 0 ) { return; }
            if( area < 0 ) { std::reverse(p.begin(), p.end()); }
            out.push_back(std::move(p));
        }

        /*!@brief The convex pieces whose union is a stroke. */
        inline std::vector<Poly> StrokePolys( const Stroke & s )
        {
            std::vector<Poly> out;
            std::vector<P2> p;
            for( const P2 & q : s.pts ) { if( p.empty() || std::hypot(q.x - p.back().x, q.y - p.back().y) > 1e-9 ) { p.push_back(q); } }
            if( s.closed && p.size() > 2 && std::hypot(p.front().x - p.back().x, p.front().y - p.back().y) <= 1e-9 ) { p.pop_back(); }
            const std::size_t n = p.size();
            if( n < 2 ) { return out; }
            const Real hw = s.width / 2;
            const std::size_t nseg = s.closed ? n : n - 1;
            auto dir = [&]( std::size_t i ) { const P2 & a = p[i], & b = p[(i + 1) % n]; const Real d = std::hypot(b.x - a.x, b.y - a.y); return P2{ (b.x - a.x) / d, (b.y - a.y) / d }; };
            auto disc = [&]( const P2 & c )
            {
                const int m = std::max(12, std::min(48, int(hw * 4) + 8));
                Poly q; for( int i = 0; i < m; ++i ) { const Real t = 6.283185307179586 * i / m; q.push_back(P2{ c.x + hw * std::cos(t), c.y + hw * std::sin(t) }); }
                Oriented(out, std::move(q));
            };
            for( std::size_t i = 0; i < nseg; ++i )
            {
                const P2 & a = p[i], & b = p[(i + 1) % n], d = dir(i), nrm { -d.y * hw, d.x * hw };
                Oriented(out, Poly{ P2{ a.x + nrm.x, a.y + nrm.y }, P2{ b.x + nrm.x, b.y + nrm.y }, P2{ b.x - nrm.x, b.y - nrm.y }, P2{ a.x - nrm.x, a.y - nrm.y } });
            }
            for( std::size_t i = (s.closed ? 0 : 1); i < (s.closed ? n : n - 1); ++i )      // joins
            {
                if( s.round ) { disc(p[i]); continue; }
                const P2 d0 = dir((i + nseg - 1) % nseg), d1 = dir(i);
                const Real cross = d0.x * d1.y - d0.y * d1.x;
                if( std::abs(cross) < 1e-9 ) { continue; }
                const Real sgn = (cross > 0) ? -1 : 1;                                       // the outer side
                const P2 a { p[i].x - d0.y * hw * sgn, p[i].y + d0.x * hw * sgn }, b { p[i].x - d1.y * hw * sgn, p[i].y + d1.x * hw * sgn };
                Oriented(out, Poly{ p[i], a, b });                                           // bevel
                const P2 m { -(d0.y + d1.y) * sgn, (d0.x + d1.x) * sgn };
                const Real ml = std::hypot(m.x, m.y);
                if( ml > 0.5 )                                                               // miter, limit 4
                {
                    const Real k = hw * 2 / (ml * ml);
                    Oriented(out, Poly{ a, P2{ p[i].x + m.x * k, p[i].y + m.y * k }, b });
                }
            }
            if( s.round && !s.closed ) { disc(p.front()); disc(p.back()); }
            return out;
        }
    } // namespace detail

    /*!@brief Draw a frame. */
    inline Image Draw( const Frame & f )
    {
        Image img (int(std::lround(f.width)), int(std::lround(f.height)), f.background);
        for( const Item & it : f.items )
        {
            if( it.fillQ ) { detail::FillPolys(img, it.shape.rings, it.shape.color); }
            else           { detail::FillPolys(img, detail::StrokePolys(it.stroke), it.stroke.color); }
        }
        return img;
    }

} // namespace Knoodle::Animate
