// Objective-driven BFACF (step 3): the swappable Objective_T seam does real work.
//
//   MinLength   -- collapse a bloated (subdivided) ReAPR trefoil toward its MINIMAL lattice
//                  length (24 edges) while the knot type (HOMFLY) is preserved. This is the
//                  decongestion the whole experiment is about: a huge ReAPR embedding is
//                  reduced to the tightest lattice trefoil, still a trefoil.
//   MaxRg2Banded-- starting from that compact trefoil, climb R_g^2 (expand the conformation)
//                  while a soft length band holds the size; HOMFLY preserved.
//
// Both are objective *policies* over the same knot-type-preserving move engine, checked with
// the shared libhomfly oracle (homfly_invariance.hpp).

#include "homfly_invariance.hpp"
#include "../src/LatticeLink.hpp"

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

    Polynomial HomflyOf( const Lattice_T & ll, bool & ok )
    {
        LinkEmb_T emb = ll.ToLinkEmbedding();
        const Int V = emb.VertexCount();
        std::vector<double> c( static_cast<std::size_t>(3*V) );
        emb.WriteVertexCoordinates( c.data() );
        for( Int i = 0; i < V; ++i )   // generic shear: kills axis-projection degeneracy
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

    // Deterministic lattice trefoil from ReAPR (permutation off -> fixed start each run).
    Lattice_T ReaprTrefoil()
    {
        std::vector<Int> pd = {0,4,1,3,1, 2,0,3,5,1, 4,2,5,1,1};
        PD_T trefoil = PD_T::FromSignedPDCode( pd.data(), Int(3), false, true );
        Reapr_T reapr;
        reapr.Settings().permute_randomQ = false;
        return Lattice_T( reapr.Embedding( trefoil ) );
    }
}

int main()
{
    std::cout << "LatticeLink objective-driven BFACF\n";

    // -----------------------------------------------------------------------------------
    // 1. MinLength: decongest a bloated ReAPR trefoil down to the minimal lattice trefoil.
    // -----------------------------------------------------------------------------------
    Lattice_T minimal;   // captured for reuse by test 2
    {
        Lattice_T ll = ReaprTrefoil();
        const Int start_edges = ll.EdgeCount();

        bool ok0 = false;
        const Polynomial before = HomflyOf( ll, ok0 );

        ll.Settings().objective    = Lattice_T::Objective_T::MinLength;
        ll.Settings().beta_init    = 5.0;      // strong, constant death bias
        ll.Settings().beta_final   = 5.0;
        ll.Settings().anneal_steps = 1;
        ll.Settings().max_edges    = 100000;

        std::mt19937_64 rng( 0xDECAF01u );
        bool audit_ok = true;
        for( std::uint64_t k = 0; k < 25'000'000; ++k )
        {
            ll.Step( rng );
            if( (k % 2'000'000) == 0 && !ll.SelfConsistentQ() ) { audit_ok = false; break; }
        }
        check( audit_ok && ll.SelfConsistentQ(), "MinLength [structural audit]" );

        const Int end_edges = ll.EdgeCount();
        bool ok1 = false;
        const Polynomial after = HomflyOf( ll, ok1 );

        std::cout << "        edges: " << start_edges << " -> " << end_edges
                  << "   (minimal lattice trefoil = 24)\n";
        std::cout << "        HOMFLY: " << PolyToString(after) << "\n";

        check( ok0 && ok1 && (before == after), "MinLength [HOMFLY invariant = still trefoil]" );
        check( end_edges >= 24 && end_edges <= 40, "MinLength [reached ~minimal length]" );
        check( end_edges * 5 < start_edges,        "MinLength [length collapsed]" );

        minimal = ll;
    }

    // -----------------------------------------------------------------------------------
    // 2. MaxRg2Banded: expand the compact trefoil (grow R_g^2) with a soft length band.
    // -----------------------------------------------------------------------------------
    {
        Lattice_T ll = minimal;
        if( !ll.ValidQ() ) { ll = ReaprTrefoil(); }

        const Int    start_edges = ll.EdgeCount();
        const double rg2_before  = ll.SquaredGyradius();
        bool ok0 = false;
        const Polynomial before = HomflyOf( ll, ok0 );

        ll.ResetSchedule();
        ll.Settings().objective      = Lattice_T::Objective_T::MaxRg2Banded;
        ll.Settings().rg_beta        = 0.05;
        ll.Settings().band_lo        = 60;
        ll.Settings().band_hi        = 160;
        ll.Settings().band_stiffness = 0.1;
        ll.Settings().max_edges      = 600;

        std::mt19937_64 rng( 0xF00Du );
        bool audit_ok = true;
        for( std::uint64_t k = 0; k < 10'000'000; ++k )
        {
            ll.Step( rng );
            if( (k % 2'000'000) == 0 && !ll.SelfConsistentQ() ) { audit_ok = false; break; }
        }
        check( audit_ok && ll.SelfConsistentQ(), "MaxRg2Banded [structural audit]" );

        const double rg2_after = ll.SquaredGyradius();
        const Int    end_edges = ll.EdgeCount();
        bool ok1 = false;
        const Polynomial after = HomflyOf( ll, ok1 );

        std::cout << "        Rg^2: " << rg2_before << " -> " << rg2_after
                  << "   edges " << start_edges << " -> " << end_edges
                  << " (band [" << ll.Settings().band_lo << "," << ll.Settings().band_hi << "])\n";
        std::cout << "        HOMFLY: " << PolyToString(after) << "\n";

        check( ok0 && ok1 && (before == after), "MaxRg2Banded [HOMFLY invariant]" );
        check( rg2_after > rg2_before * 2.0,    "MaxRg2Banded [R_g^2 grew (expanded)]" );
        // Soft band: length settles near the band, allowed to overshoot band_hi modestly but
        // stays well under the hard cap.
        const Int soft_hi = ll.Settings().band_hi + ll.Settings().band_hi / 5;
        check( end_edges >= ll.Settings().band_lo - 20 && end_edges <= soft_hi,
               "MaxRg2Banded [length held ~in band]" );
    }

    std::cout << (g_failures == 0 ? "\nALL PASSED\n" : "\nFAILED\n");
    return (g_failures == 0) ? 0 : 1;
}
