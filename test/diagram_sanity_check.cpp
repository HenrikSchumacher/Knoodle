// diagram_sanity_check -- self-test for test/diagram_sanity.hpp.
//
// The sanity header exists because the library's `CheckAll` is deliberately
// LOCAL and lightweight (it runs inside performance-critical code, and its
// own comment says a `true` result does not certify a valid planar diagram).
// This test pins down three things:
//
//   1. The Euler convention of the CURRENT face computation. `FaceCount()`
//      counts faces by boundary components, so V - E + F == 2 per diagram
//      component -- measured here on a connected trefoil (chi = 2) and on a
//      split two-trefoil diagram (chi = 4). If Henrik ever changes the face
//      computation's counting convention, this is the test that goes red, and
//      diagram_sanity.hpp is the one place to update.
//
//   2. The checks CATCH what they claim to catch. A slot-swapped trefoil --
//      the two `In` entries of one crossing exchanged -- passes every local
//      check while its stored embedding is a torus (V-E+F = 0). CheckAll
//      passes it; DiagramSanityQ must reject it, at every crossing, for both
//      the In and the Out swap. Likewise a diagram with a corrupted A_cross
//      entry (which the LOCAL checks do catch) must be rejected.
//
//   3. Valid inputs PASS, including the edge cases: a 0-crossing unknot
//      diagram, a diagram with a warm cache, and complexes both empty and
//      populated.
//
// Build: `make diagram_sanity_check` in test/.

#include "../Knoodle.hpp"
#include "diagram_sanity.hpp"

#include <cstdio>
#include <string>
#include <vector>

using Int   = std::int64_t;
using PD_T  = Knoodle::PlanarDiagram<Int>;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;
using CS_T  = Knoodle::CrossingState_T;
using AS_T  = Knoodle::ArcState_T;

static int checks = 0;
static int fails  = 0;

static void check( bool okQ, const std::string & what )
{
    std::printf("  %-64s %s\n", what.c_str(), okQ ? "OK" : "FAILED");
    ++checks;
    if( !okQ ) { ++fails; }
}

// A copy of `pd` built through the internal-data constructor (compressQ =
// false, so labels survive exactly), with `mutate` applied to the C_arcs
// buffer before construction.
template<typename F>
static PD_T Rebuild( const PD_T & pd, F && mutate )
{
    const Int n = pd.MaxCrossingCount();
    const Int m = pd.MaxArcCount();

    std::vector<Int>  C  ( (std::size_t)(4*n) );
    std::vector<Int>  A  ( (std::size_t)(2*m) );
    std::vector<Int>  col( (std::size_t)m     );
    std::vector<CS_T> Cs ( (std::size_t)n     );
    std::vector<AS_T> As ( (std::size_t)m     );

    for( Int i = 0; i < n; ++i )
    {
        for( int t = 0; t < 2; ++t )
        {
            for( int lr = 0; lr < 2; ++lr )
            {
                C[(std::size_t)(4*i + 2*t + lr)] =
                    pd.Crossings()(i, (bool)t, (bool)lr);
            }
        }
        Cs[(std::size_t)i] = pd.CrossingStates()[i];
    }
    for( Int a = 0; a < m; ++a )
    {
        A[(std::size_t)(2*a + 0)] = pd.Arcs()(a,false);
        A[(std::size_t)(2*a + 1)] = pd.Arcs()(a,true );
        As [(std::size_t)a] = pd.ArcStates()[a];
        col[(std::size_t)a] = pd.ArcColors()[a];
    }

    mutate(C, A);

    return PD_T(
        n,
        C.data(), Cs.data(),
        A.data(), As.data(),
        col.data(), Int(-1),
        false,   // proven_minimalQ
        false ); // compressQ: keep the labels
}

int main()
{
    // Right-hand trefoil.
    Int tre[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1 };
    PD_T trefoil = PD_T::FromSignedPDCode(&tre[0], Int(3), false, false);

    // ---- valid diagrams pass -----------------------------------------------

    check( KnoodleTest::DiagramSanityQ(trefoil),
           "trefoil passes DiagramSanityQ" );

    check( trefoil.CrossingCount() - trefoil.ArcCount() + trefoil.FaceCount()
               == Int(2),
           "trefoil: V-E+F = 2 (pins the Euler convention, connected)" );

    // Two disjoint trefoils in ONE diagram: the outer region is counted once
    // per diagram component, so chi = 4, and the sanity check must accept it.
    Int two[] = { 0,4,1,3,1,  2,0,3,5,1,  4,2,5,1,1,
                  6,10,7,9,1, 8,6,9,11,1, 10,8,11,7,1 };
    PD_T split = PD_T::FromSignedPDCode(&two[0], Int(6), false, false);

    check( split.DiagramComponentCount() == Int(2),
           "split two-trefoil diagram has 2 diagram components" );

    check( split.CrossingCount() - split.ArcCount() + split.FaceCount()
               == Int(4),
           "split: V-E+F = 4 (pins the Euler convention, split)" );

    check( KnoodleTest::DiagramSanityQ(split),
           "split two-trefoil diagram passes DiagramSanityQ" );

    // Edge case: the 0-crossing unknot diagram has no faces to recompute.
    PD_T unknot = PD_T::Unknot(Int(0));
    check( KnoodleTest::DiagramSanityQ(unknot),
           "0-crossing unknot passes DiagramSanityQ" );

    // A warm cache on a healthy diagram must agree with the recomputation.
    (void)trefoil.FaceCount();
    (void)trefoil.LinkComponentCount();
    check( trefoil.InCacheQ("FaceDarcs")
               && KnoodleTest::DiagramSanityQ(trefoil),
           "trefoil with warm cache still passes (stale-cache arm agrees)" );

    // `FaceString` is a debugging accessor nobody calls, so a typo in it never
    // fails to compile (it called `RaggedList::Indices()`, which does not
    // exist, for a long time). Instantiate it on every face and check that it
    // lists exactly the darcs FaceDarcs() does, so it cannot rot again.
    {
        const auto & F_dA = trefoil.FaceDarcs();
        bool okQ = true;
        for( Int f = 0; f < trefoil.FaceCount(); ++f )
        {
            const Int i_begin = F_dA.Pointers()[f  ];
            const Int i_end   = F_dA.Pointers()[f+1];
            std::string want = "face " + std::to_string((long long)f) + " = "
                             + std::string(Tools::OutString::FromVector(
                                   &F_dA.Elements()[i_begin], i_end - i_begin ));
            okQ = okQ && ( trefoil.FaceString(f) == want );
        }
        check( okQ, "FaceString(f) compiles and lists FaceDarcs() for every face" );
    }

    // An invalid diagram is corrupt by definition here.
    {
        PD_T invalid = PD_T::InvalidDiagram();
        std::string why;
        check( !KnoodleTest::DiagramSanityQ(invalid, &why),
               "InvalidDiagram is rejected" );
    }

    // ---- the swap that CheckAll cannot see ---------------------------------
    //
    // Exchanging the two In (or Out) entries of a single crossing leaves every
    // local check satisfied -- A_cross is untouched -- but changes the cyclic
    // order at that vertex: the embedding is no longer planar.

    for( Int c = 0; c < Int(3); ++c )
    {
        for( bool io : { false, true } )
        {
            PD_T q = Rebuild( trefoil,
                [c,io]( std::vector<Int> & C, std::vector<Int> & )
                {
                    const std::size_t base =
                        (std::size_t)(4*c + 2*(io ? 1 : 0));
                    std::swap( C[base], C[base + 1] );
                } );

            const std::string where =
                "c=" + std::to_string((long long)c) + (io ? " In" : " Out");

            check( q.CheckAll(),
                   where + ": swapped trefoil still passes local CheckAll" );

            std::string why;
            check( !KnoodleTest::DiagramSanityQ(q, &why),
                   where + ": DiagramSanityQ rejects it (Euler)" );
        }
    }

    // ---- corruption the local checks DO see must also be rejected ----------
    {
        PD_T q = Rebuild( trefoil,
            []( std::vector<Int> &, std::vector<Int> & A )
            {
                // Point arc 0's head at the wrong crossing.
                A[1] = (A[1] + 1) % 3;
            } );

        std::string why;
        check( !KnoodleTest::DiagramSanityQ(q, &why),
               "corrupted A_cross entry is rejected (via CheckAll)" );
    }

    // ---- the complex-level wrapper -----------------------------------------

    {
        PDC_T empty;
        check( KnoodleTest::ComplexSanityQ(empty),
               "an empty complex passes ComplexSanityQ" );

        PDC_T pdc { trefoil };
        check( pdc.DiagramCount() == Int(1)
                   && KnoodleTest::ComplexSanityQ(pdc),
               "a complex holding a trefoil passes ComplexSanityQ" );

        PDC_T multi { split };
        {
            Knoodle::ScopedUnlock unlock ( multi );
            multi.Push( PD_T::Unknot(Int(7)) );
        }
        check( multi.DiagramCount() >= Int(1)
                   && KnoodleTest::ComplexSanityQ(multi),
               "a multi-diagram complex passes ComplexSanityQ" );
    }

    // The count is what manifest.tsv's work pattern matches: a run that
    // asserted nothing must not read as a pass.
    std::printf("\n%s (%d checks, %d failed)\n",
                fails == 0 ? "DIAGRAM SANITY CHECK OK"
                           : "DIAGRAM SANITY CHECK FAILED",
                checks, fails);
    return fails == 0 ? 0 : 1;
}
