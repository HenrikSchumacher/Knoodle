// Knot-type invariance of LatticeLink BFACF moves (step 2).
//
// Every admissible BFACF move preserves self-avoidance, hence the knot/link type. We check
// that directly: build a lattice polygon, run many accept-all-admissible moves (auditing
// full structural + accumulator consistency along the way), then confirm the HOMFLY
// polynomial is unchanged. Starting configurations:
//   - a genuine lattice TREFOIL, obtained by running ReAPR on the trefoil PD (its embedding
//     is integer + axis-aligned, so LatticeLink ingests it directly);
//   - unknot polygons (rectangle, 3D skew hexagon) as controls.
//
// HOMFLY is computed via the shared oracle (homfly_invariance.hpp -> vendored libhomfly):
// export -> generic shear (kills axis-aligned projection degeneracy, preserves knot type)
// -> PD -> Simplify -> HOMFLY.

#include "homfly_invariance.hpp"
#include "../src/LatticeLink.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
    using LinkEmb_T = Knoodle::LinkEmbedding<double,Int,float>;
    using Lattice_T = Knoodle::LatticeLink<double,Int,float>;
    using Reapr_T   = Knoodle::Reapr<double,Int,float>;

    int g_failures = 0;

    void check( bool ok, const std::string & name )
    {
        std::cout << (ok ? "  ok   " : "  FAIL ") << name << "\n";
        if( !ok ) { ++g_failures; }
    }

    // Single-component knot embedding from integer corner vertices.
    LinkEmb_T BuildKnot( const std::vector<std::array<int,3>> & corners )
    {
        const Int n = static_cast<Int>(corners.size());
        Tensors::Tensor1<Int,Int> cp( 2 );
        Tensors::Tensor1<Int,Int> col( 1 );
        cp[0] = 0; cp[1] = n; col[0] = 0;

        std::vector<double> coord( static_cast<std::size_t>(3*n) );
        for( Int i = 0; i < n; ++i )
        {
            coord[3*i+0] = corners[i][0];
            coord[3*i+1] = corners[i][1];
            coord[3*i+2] = corners[i][2];
        }
        LinkEmb_T emb( std::move(cp), std::move(col) );
        emb.template ReadVertexCoordinates<false,false>( coord.data() );
        return emb;
    }

    // Export -> generic shear -> PD -> Simplify -> HOMFLY. Shear preserves knot type but makes
    // the axis-projection non-degenerate.
    Polynomial HomflyOf( const Lattice_T & ll, bool & ok )
    {
        LinkEmb_T emb = ll.ToLinkEmbedding();
        const Int V = emb.VertexCount();
        std::vector<double> c( static_cast<std::size_t>(3*V) );
        emb.WriteVertexCoordinates( c.data() );

        for( Int i = 0; i < V; ++i )
        {
            const double x = c[3*i+0], y = c[3*i+1], z = c[3*i+2];
            c[3*i+0] = x + 0.017300*z + 0.004270*y;
            c[3*i+1] = y + 0.009100*z;
            c[3*i+2] = z;
        }

        auto [pd, unlinks] = PD_T::FromKnotEmbedding( c.data(), V );
        (void)unlinks;
        return SimplifiedHomfly( pd, ok );
    }

    void InvarianceCase( const std::string & name, Lattice_T & ll,
                         const int moves, const std::uint64_t seed,
                         const std::size_t max_edges )
    {
        if( !ll.ValidQ() || !ll.SelfConsistentQ() )
        {
            check( false, name + " [initial ValidQ+consistent]" );
            return;
        }

        bool ok0 = false;
        const Polynomial before = HomflyOf( ll, ok0 );

        std::mt19937_64 rng( seed );
        int births = 0, deaths = 0, flips = 0, rejects = 0;
        bool audit_ok = true;

        for( int k = 0; k < moves; ++k )
        {
            switch( ll.AttemptMove( rng, max_edges ) )
            {
                case Lattice_T::MoveType::Birth: ++births; break;
                case Lattice_T::MoveType::Death: ++deaths; break;
                case Lattice_T::MoveType::Flip:  ++flips;  break;
                default:                         ++rejects; break;
            }
            if( (k % 500) == 0 && !ll.SelfConsistentQ() )
            {
                audit_ok = false;
                check( false, name + " [structural audit @ step " + std::to_string(k) + "]" );
                break;
            }
        }
        if( !audit_ok ) { return; }

        check( ll.SelfConsistentQ(), name + " [final structural audit]" );

        bool ok1 = false;
        const Polynomial after = HomflyOf( ll, ok1 );

        std::cout << "        moves: birth=" << births << " death=" << deaths
                  << " flip=" << flips << " reject=" << rejects
                  << " | edges " << ll.EdgeCount()
                  << " | Rg^2 " << ll.SquaredGyradius() << "\n";
        std::cout << "        HOMFLY before: " << PolyToString(before) << "\n";
        std::cout << "        HOMFLY after : " << PolyToString(after)  << "\n";

        check( ok0 && ok1, name + " [HOMFLY computable]" );
        check( ok0 && ok1 && (before == after), name + " [HOMFLY invariant under BFACF]" );
    }
}

int main()
{
    std::cout << "LatticeLink BFACF knot-type invariance\n";

    // 1. Genuine lattice trefoil from ReAPR.
    {
        std::vector<Int> pd = {0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1};
        PD_T trefoil = PD_T::FromSignedPDCode( pd.data(), Int(3), false, true );
        Reapr_T reapr;
        LinkEmb_T emb = reapr.Embedding( trefoil );
        Lattice_T ll( emb );
        std::cout << "[trefoil] start edges = " << ll.EdgeCount() << "\n";
        InvarianceCase( "trefoil (ReAPR start)", ll, 60000, 0x9E3779B9u, 2600 );
    }

    // 2. Unknot rectangle (subdivided).
    {
        LinkEmb_T emb = BuildKnot( { {0,0,0}, {8,0,0}, {8,5,0}, {0,5,0} } );
        Lattice_T ll( emb );
        InvarianceCase( "rectangle (unknot)", ll, 20000, 0xB5297A4Du, 400 );
    }

    // 3. Unknot 3D skew hexagon.
    {
        LinkEmb_T emb = BuildKnot( { {0,0,0}, {4,0,0}, {4,4,0}, {4,4,4}, {0,4,4}, {0,0,4} } );
        Lattice_T ll( emb );
        InvarianceCase( "3D hexagon (unknot)", ll, 20000, 0x68E31DA4u, 400 );
    }

    std::cout << (g_failures == 0 ? "\nALL PASSED\n" : "\nFAILED\n");
    return (g_failures == 0) ? 0 : 1;
}
