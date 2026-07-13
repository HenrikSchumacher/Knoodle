// Round-trip + accumulator checks for LatticeLink (step 1 of the BFACF preprocessing work).
//
//   LinkEmbedding --(LatticeLink ctor: validate + subdivide)--> LatticeLink
//   LatticeLink   --(ToLinkEmbedding: re-weld collinear runs)--> LinkEmbedding
//
// We verify: (a) the round-trip reproduces the collinear-free polygon, up to cyclic
// rotation, for a range of shapes (unit + subdivided + non-convex + 3D + multi-component);
// (b) collinear pass-through vertices are dropped by re-welding; (c) non-self-avoiding
// input is rejected; (d) the O(1) R_g accumulators agree with a from-scratch recompute
// (and a hand-computed value on the unit square).
//
// No HOMFLY oracle / libhomfly here -- knot-type invariance under BFACF moves is step 2.

#include "../Knoodle.hpp"
#include "../src/LatticeLink.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    using Int             = std::int64_t;
    using Real            = double;
    using LinkEmbedding_T = Knoodle::LinkEmbedding<Real,Int,float>;
    using LatticeLink_T   = Knoodle::LatticeLink<Real,Int,float>;

    using Pt   = std::array<int,3>;
    using Cyc  = std::vector<Pt>;          // one component's vertex cycle
    using Poly = std::vector<Cyc>;         // a link = several component cycles

    int g_failures = 0;

    void check( bool ok, const std::string & name )
    {
        std::cout << (ok ? "  ok   " : "  FAIL ") << name << "\n";
        if( !ok ) { ++g_failures; }
    }

    // Build a LinkEmbedding from explicit integer component cycles.
    LinkEmbedding_T BuildEmbedding( const Poly & comps )
    {
        const Int ncomp = static_cast<Int>(comps.size());

        Tensors::Tensor1<Int,Int> cp( ncomp + 1 );
        Tensors::Tensor1<Int,Int> col( ncomp );

        Int total = 0;
        cp[0] = 0;
        for( Int c = 0; c < ncomp; ++c )
        {
            total += static_cast<Int>(comps[c].size());
            cp[c+1] = total;
            col[c]  = c;
        }

        std::vector<Real> coord( static_cast<std::size_t>(3*total) );
        Int row = 0;
        for( const Cyc & cyc : comps )
        {
            for( const Pt & p : cyc )
            {
                coord[3*row + 0] = static_cast<Real>(p[0]);
                coord[3*row + 1] = static_cast<Real>(p[1]);
                coord[3*row + 2] = static_cast<Real>(p[2]);
                ++row;
            }
        }

        LinkEmbedding_T emb( std::move(cp), std::move(col) );
        emb.template ReadVertexCoordinates<false,false>( coord.data() );
        return emb;
    }

    // Pull the per-component integer vertex cycles back out of a LinkEmbedding.
    Poly EmbeddingToPoly( const LinkEmbedding_T & emb )
    {
        const Int V = emb.VertexCount();
        std::vector<Real> coord( static_cast<std::size_t>(3*V) );
        emb.WriteVertexCoordinates( coord.data() );

        const auto & cp   = emb.ComponentPointers();
        const Int    ncmp = emb.ComponentCount();

        Poly out;
        for( Int c = 0; c < ncmp; ++c )
        {
            Cyc cyc;
            for( Int i = cp[c]; i < cp[c+1]; ++i )
            {
                cyc.push_back( Pt{ static_cast<int>(std::llround(coord[3*i+0])),
                                   static_cast<int>(std::llround(coord[3*i+1])),
                                   static_cast<int>(std::llround(coord[3*i+2])) } );
            }
            out.push_back( std::move(cyc) );
        }
        return out;
    }

    // Canonical form of a directed cycle: rotate so it starts at its lexicographically
    // smallest vertex (direction preserved). Renders to a comparable string.
    std::string CanonCycle( const Cyc & cyc )
    {
        const std::size_t n = cyc.size();
        std::size_t best = 0;
        for( std::size_t i = 1; i < n; ++i )
        {
            if( cyc[i] < cyc[best] ) { best = i; }
        }
        std::string s;
        for( std::size_t k = 0; k < n; ++k )
        {
            const Pt & p = cyc[(best + k) % n];
            s += "(" + std::to_string(p[0]) + "," + std::to_string(p[1]) + ","
                     + std::to_string(p[2]) + ")";
        }
        return s;
    }

    // Multiset of canonical component cycles (component order ignored).
    std::vector<std::string> CanonPoly( const Poly & poly )
    {
        std::vector<std::string> v;
        for( const Cyc & c : poly ) { v.push_back( CanonCycle(c) ); }
        std::sort( v.begin(), v.end() );
        return v;
    }

    // Round-trip a polygon and assert it matches `expected` (both canonicalized).
    void RoundTripCase( const std::string & name, const Poly & input, const Poly & expected )
    {
        LinkEmbedding_T emb_in = BuildEmbedding( input );
        LatticeLink_T   ll( emb_in );

        if( !ll.ValidQ() )
        {
            check( false, name + " [ctor ValidQ]" );
            return;
        }

        const bool acc_ok = ll.AccumulatorsConsistentQ();
        check( acc_ok, name + " [accumulators consistent]" );

        LinkEmbedding_T emb_out = ll.ToLinkEmbedding();
        Poly out = EmbeddingToPoly( emb_out );

        check( CanonPoly(out) == CanonPoly(expected), name + " [round-trip shape]" );
    }
}

int main()
{
    std::cout << "LatticeLink round-trip checks\n";

    // 1. Unit square: no subdivision.
    {
        Poly sq = {{ {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0} }};
        RoundTripCase( "unit square", sq, sq );
    }

    // 2. Large rectangle: edges are multi-unit -> subdivided, then re-welded to 4 corners.
    {
        Poly rect = {{ {0,0,0}, {5,0,0}, {5,3,0}, {0,3,0} }};
        RoundTripCase( "5x3 rectangle (subdivide+reweld)", rect, rect );
    }

    // 3. Non-convex L-shaped loop: re-weld must NOT merge across the concave corners.
    {
        Poly ell = {{ {0,0,0}, {3,0,0}, {3,1,0}, {1,1,0}, {1,3,0}, {0,3,0} }};
        RoundTripCase( "L-shaped loop", ell, ell );
    }

    // 4. Skew hexagon on a cube: uses all three axes.
    {
        Poly hex = {{ {0,0,0}, {2,0,0}, {2,2,0}, {2,2,2}, {0,2,2}, {0,0,2} }};
        RoundTripCase( "3D skew hexagon", hex, hex );
    }

    // 5. Two disjoint squares in parallel planes: multi-component, shared OccupiedSet.
    {
        Poly two = {
            { {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0} },
            { {0,0,5}, {2,0,5}, {2,2,5}, {0,2,5} }
        };
        RoundTripCase( "two-component (disjoint squares)", two, two );
    }

    // 6. Collinear pass-through vertex: re-welding must drop (1,0,0).
    {
        Poly with_collinear = {{ {0,0,0}, {1,0,0}, {2,0,0}, {2,2,0}, {0,2,0} }};
        Poly welded         = {{ {0,0,0},          {2,0,0}, {2,2,0}, {0,2,0} }};
        RoundTripCase( "collinear vertex dropped", with_collinear, welded );
    }

    // 7. Self-avoidance rejection: a figure-eight loop revisiting the origin.
    {
        Poly fig8 = {{ {0,0,0}, {2,0,0}, {2,-2,0}, {0,-2,0},
                       {0,0,0}, {-2,0,0}, {-2,2,0}, {0,2,0} }};
        LinkEmbedding_T emb = BuildEmbedding( fig8 );
        LatticeLink_T   ll( emb );
        check( !ll.ValidQ(), "non-self-avoiding input rejected" );
    }

    // 8. Hand-computed R_g^2 for the unit square: centroid (0.5,0.5,0),
    //    each vertex at squared distance 0.5 -> R_g^2 = 0.5 exactly.
    {
        Poly sq = {{ {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0} }};
        LatticeLink_T ll( BuildEmbedding(sq) );
        const Real rg2 = ll.SquaredGyradius();
        check( std::fabs(rg2 - 0.5) < 1e-12, "unit-square R_g^2 == 0.5" );
    }

    std::cout << (g_failures == 0 ? "\nALL PASSED\n" : "\nFAILED\n");
    return (g_failures == 0) ? 0 : 1;
}
