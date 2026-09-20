/**
 * @file redraw_move.hpp
 * @brief The `redraw` descriptor: the rotation a re-projection step used.
 *
 * `#move kind=redraw rot=<r00,...,r22>`, row-major, and per
 * docs/move-descriptor.md it must be ONE OF EXACTLY TWO matrices -- the cyclic
 * axis permutations. That restriction is the point of the kind, so the parser
 * enforces it rather than leaving it to a caller:
 *
 *   0,0,1,1,0,0,0,1,0   x -> y -> z -> x   +2pi/3 about (1,1,1)
 *   0,1,0,0,0,1,1,0,0   x -> z -> y -> x   -2pi/3 about (1,1,1)
 *
 * WHY ONLY THESE. Any `R` in SO(3) checked to a tolerance puts a
 * floating-point tie-break inside the trust base, exactly where the geometry
 * is hardest: a rotation can bring two strands into near-alignment, and
 * whether a crossing exists there becomes a question about epsilon. These two
 * are integer matrices, so `R` is checked by EQUALITY; they carry a lattice
 * curve to a lattice curve, so `LinkEmbedding_Int` + `Prosector` (simulation
 * of simplicity, which makes a fully degenerate projection parse repeatably)
 * apply to both views; and with the identity they generate the three axis
 * projections, which is what a lattice simplifier wants.
 *
 * The identity is NOT a redraw: the move would change nothing.
 *
 * No drawing here, deliberately -- a verifier checks a redraw without laying
 * anything out.
 */

#pragma once

#include <array>
#include <string>
#include <vector>
#include <string_view>

namespace KnoodleRedraw
{

template<class Int_>
struct RedrawDescriptor
{
    using Int = Int_;

    std::array<Int,9> rot {};          // row-major
    int which = 0;                     // 1 = x->y->z->x, 2 = x->z->y->x

    static constexpr std::array<Int,9> Cycle1 = {0,0,1,1,0,0,0,1,0};
    static constexpr std::array<Int,9> Cycle2 = {0,1,0,0,0,1,1,0,0};

    std::string ToString() const
    {
        std::string s = "kind=redraw rot=";
        for( int k = 0; k < 9; ++k )
        {
            if( k ) { s += ","; }
            s += std::to_string(rot[static_cast<std::size_t>(k)]);
        }
        return s;
    }

    /// How this rotation permutes the axes, for a report line.
    std::string CycleName() const
    {
        if( which == 1 ) { return "x->y->z->x (+2pi/3 about (1,1,1))"; }
        if( which == 2 ) { return "x->z->y->x (-2pi/3 about (1,1,1))"; }
        return "not one of the two permitted rotations";
    }

    /*!@brief Parse the `redraw` payload. `kind=redraw` may be present or not;
     * `rot=` is required, must hold nine integers, and must be one of the two.
     */
    static bool Parse( std::string_view spec, RedrawDescriptor & out,
                       std::string & err )
    {
        out = RedrawDescriptor();

        const auto pos = spec.find("rot=");
        if( pos == std::string_view::npos )
        {
            err = "a redraw descriptor needs a 'rot=' field";
            return false;
        }

        std::string_view tail = spec.substr(pos + 4);
        const auto end = tail.find_first_of(" \t");
        if( end != std::string_view::npos ) { tail = tail.substr(0,end); }

        int k = 0;
        std::size_t at = 0;
        while( at <= tail.size() )
        {
            auto comma = tail.find(',',at);
            if( comma == std::string_view::npos ) { comma = tail.size(); }

            const std::string_view tok = tail.substr(at, comma - at);
            if( tok.empty() )
            {
                err = "empty entry in 'rot=" + std::string(tail) + "'";
                return false;
            }
            if( k >= 9 )
            {
                err = "'rot=' has more than nine entries";
                return false;
            }

            Int v = Int(0);
            bool negQ = false;
            std::size_t i = 0;
            if( (tok[0] == '-') || (tok[0] == '+') )
            {
                negQ = (tok[0] == '-');
                i = 1;
            }
            if( i >= tok.size() )
            {
                err = "bad entry '" + std::string(tok) + "' in 'rot='";
                return false;
            }
            for( ; i < tok.size(); ++i )
            {
                if( (tok[i] < '0') || (tok[i] > '9') )
                {
                    err = "entry '" + std::string(tok) + "' in 'rot=' is not an"
                          " integer -- the permitted rotations are integer"
                          " matrices";
                    return false;
                }
                v = Int(10) * v + Int(tok[i] - '0');
            }
            out.rot[static_cast<std::size_t>(k++)] = negQ ? -v : v;

            at = comma + 1;
        }

        if( k != 9 )
        {
            err = "'rot=' has " + std::to_string(k) + " entries, want nine";
            return false;
        }

        if     ( out.rot == Cycle1 ) { out.which = 1; }
        else if( out.rot == Cycle2 ) { out.which = 2; }
        else
        {
            out.which = 0;
            err = "rot=" + std::string(tail) + " is not one of the two"
                  " permitted rotations (0,0,1,1,0,0,0,1,0 = x->y->z->x, or"
                  " 0,1,0,0,0,1,1,0,0 = x->z->y->x); see"
                  " docs/move-descriptor.md, 'Why only two rotations'";
            return false;
        }

        return true;
    }

    /*!@brief Apply the rotation to a flat 3n coordinate array.*/
    template<class Real>
    std::vector<Real> Apply( const std::vector<Real> & xs ) const
    {
        std::vector<Real> out (xs.size());

        for( std::size_t v = 0; v + 2 < xs.size(); v += 3 )
        {
            for( int i = 0; i < 3; ++i )
            {
                Real acc = Real(0);
                for( int j = 0; j < 3; ++j )
                {
                    acc += static_cast<Real>(rot[static_cast<std::size_t>(3*i+j)])
                         * xs[v + static_cast<std::size_t>(j)];
                }
                out[v + static_cast<std::size_t>(i)] = acc;
            }
        }
        return out;
    }
};

} // namespace KnoodleRedraw
