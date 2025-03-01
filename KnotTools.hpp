#pragma once

#include "submodules/Tensors/Tensors.hpp"
#include "submodules/Tensors/submodules/Tools/Oriented2D.hpp"

namespace KnotTools
{
    using namespace Tensors;
    using namespace Tools;
    
    using Tools::ToString;
    
    enum class CrossingState : Int8
    {
        // Important! Active values are the only odd ones.
        RightHanded          =  1,
        RightHandedUnchanged =  2,
        LeftHanded           = -1,
        LeftHandedUnchanged  = -2,
        Inactive             =  0
    };
    
    inline std::string ToString( cref<CrossingState> s )
    {
        switch( s )
        {
            case CrossingState::Inactive             : return "Inactive";
                
            case CrossingState::RightHanded          : return "RightHanded";
                
            case CrossingState::RightHandedUnchanged : return "RightHandedUnchanged";
                
            case CrossingState::LeftHanded           : return "LeftHanded";
                
            case CrossingState::LeftHandedUnchanged  : return "LeftHandedUnchanged";
                
            default:
            {
                eprint( "ToOpString: Argument s = " + ToString(s) + " is invalid." );
                return "Inactive";
            }
        }
    }

    inline std::ostream & operator<<( std::ostream & s, cref<CrossingState> state )
    {
        s << static_cast<int>(ToUnderlying(state));
        return s;
    }
    
    inline constexpr bool ActiveQ( const CrossingState & s )
    {
        return ToUnderlying(s);
    }
    
    inline constexpr bool ChangedQ( const CrossingState & s )
    {
        return ToUnderlying(s) % 2;
    }
    
    inline constexpr bool UnchangedQ( const CrossingState & s )
    {
        return !ChangedQ(s);
    }
    
    inline constexpr bool RightHandedQ( const CrossingState & s )
    {
        return ToUnderlying(s) > 0;
    }
    
    inline constexpr bool LeftHandedQ( const CrossingState & s )
    {
        return ToUnderlying(s) < 0;
    }
    
    inline constexpr bool OppositeHandednessQ(
        const CrossingState & s_0, const CrossingState & s_1
    )
    {
        // TODO: Careful, this evaluates to true if both are `Inactive`.
        
        return ( Sign(ToUnderlying(s_0)) == -Sign(ToUnderlying(s_1)) );
    }
    
    inline constexpr bool SameHandednessQ(
        const CrossingState & s_0, const CrossingState & s_1
    )
    {
        // TODO: Careful, this evaluates to true if both are `Inactive`.
        
        return ( Sign(ToUnderlying(s_0)) == Sign(ToUnderlying(s_1)) );
    }
    
    
    
    enum class ArcState : Int8
    {
        Unchanged =  2,
        Active    =  1,
        Inactive  =  0
    };
    
    inline constexpr bool ChangedQ( const ArcState & s )
    {
        return ToUnderlying(s) % 2;
    }
    
    inline constexpr bool UnchangedQ( const ArcState & s )
    {
        return !ChangedQ(s);
    }
    
    inline constexpr bool ActiveQ( const ArcState & s )
    {
        return ToUnderlying(s);
    }
    
    inline std::string ToString( cref<ArcState> s )
    {
        switch( s )
        {
            case ArcState::Active    : return "Active";
                
            case ArcState::Inactive  : return "Inactive";
                
            case ArcState::Unchanged : return "Unchanged";
        }
    }
    
    inline std::ostream & operator<<( std::ostream & s, cref<ArcState> state )
    {
        s << static_cast<int>(ToUnderlying(state));
        return s;
    }
    
    
    /*!
     * Deduces the handedness of a crossing from the entries `X[0]`, `X[1]`, `X[2]`, `X[3]` alone. This only works if every link component has more than two arcs and if all arcs in each component are numbered consecutively.
     */
    
    template<typename Int, typename Int_2>
    CrossingState PDCodeHandedness( mptr<Int_2> X )
    {
        static_assert( SignedIntQ<Int>, "" );
        static_assert( IntQ<Int_2>, "" );
        
        const Int i = static_cast<Int>(X[0]);
        const Int j = static_cast<Int>(X[1]);
        const Int k = static_cast<Int>(X[2]);
        const Int l = static_cast<Int>(X[3]);
        
        /* This is what we know so far:
         *
         *      l \     ^ k
         *         \   /
         *          \ /
         *           \
         *          / \
         *         /   \
         *      i /     \ j
         */

        // Generically, we should have l = j + 1 or j = l + 1.
        // But of course, we also have to treat the edge case where
        // j and l differ by more than one due to the wrap-around at the end
        // of a connected component.
        
        // I "stole" this pretty neat code snippet from the KnotTheory Mathematica package by Dor Bar-Natan.
        
        if( (i == j) || (k == l) || (j == l + 1) || (l > j + 1) )
        {
            /* These are right-handed:
             *
             *       O       O            O-------O
             *      l \     ^ k          l \     ^ k
             *         \   /                \   /
             *          \ /                  \ /
             *           \                    \
             *          / \                  / \
             *         /   \                /   \
             *      i /     v j          i /     v j
             *       O---<---O            O       O
             *
             *       O       O            O       O
             *      l \     ^ k     j + x  \     ^ k
             *         \   /                \   /
             *          \ /                  \ /
             *           \                    \
             *          / \                  / \
             *         /   \                /   \
             *      i /     v l + 1     i  /     v j
             *       O       O            O       O
             */
            
            return CrossingState::RightHanded;
        }
        else if( (i == l) || (j == k) || (l == j + 1) || (j > l + 1) )
        {
            /* These are left-handed:
             *
             *       O       O            O       O
             *      l|^     ^ k          l ^     ^|k
             *       | \   /                \   / |
             *       |  \ /                  \ /  |
             *       |   \                    \   |
             *       |  / \                  / \  |
             *       | /   \                /   \ |
             *      i|/     \ j          i /     \|j
             *       O       O            O       O
             *
             *       O       O            O       O
             *    j+1 ^     ^ k         l  ^     ^ k
             *         \   /                \   /
             *          \ /                  \ /
             *           \                    \
             *          / \                  / \
             *         /   \                /   \
             *      i /     \ j         i  /     \ l + x
             *       O       O            O       O
             */
            return CrossingState::LeftHanded;
        }
        else
        {
            eprint( std::string("PDHandedness: Handedness of {") + ToString(i) + "," + ToString(i) + "," + ToString(j) + "," + ToString(k) + "," + ToString(l) + "} could not be determined. Make sure that consecutive arcs on each component have consecutive labels (except the wrap-around, of course).");
            
            return CrossingState::Inactive;
        }
    }
    
    
    /*!
     * Reads in an unsigned PD code and tries to infer the handedness of crossings.
     *
     * The return value is a `Tensor2<Int,Int>` of size `crossing_count` x `5`; the last position is the signedness, encoded by `+1` for right-handed crossings and `-1` for left-handed crossongs.
     *
     * @param pd An array of size `crossing_count` x `4` holding the pd code.
     *
     * @param crossing_count The number of crossings in the pd code.
     */
    
    template<typename Int, typename Int_2, typename Int_3>
    Tensor2<Int,Int> PDCode_AppendHandedness( mptr<Int_2> pd, const Int_3 crossing_count )
    {
        static_assert( IntQ<Int>, "" );
        
        const Int n = int_cast<Int>(crossing_count);
        
        Tensor2<Int,Int> pd_new( n, 5 );
        
        for( Int i = 0; i < n; ++i )
        {
            copy_buffer<4>( &pd[4 * i], pd_new.data(i) );
            
            pd_new(i,4) = PDCode_Handedness<Int>( pd_new.data(i) );
        }
            
        return pd_new;
    }

    
} // namespace KnotTools

#include "src/Link.hpp"

#include "src/CompleteBinaryTree.hpp"
#include "src/CompleteBinaryTree_Precomp.hpp"
#include "src/AABBTree.hpp"

#include "src/Intersection.hpp"

#include "src/PlanarLineSegmentIntersector.hpp"
#include "src/Link_2D.hpp"
//#include "src/Knot_2D.hpp"
#include "src/Multigraph.hpp"

#include "src/Debugging.hpp"
#include "src/ArcContainer.hpp"
#include "src/CrossingContainer.hpp"
#include "src/PlanarDiagram.hpp"
#include "src/CrossingSimplifier.hpp"
#include "src/ArcSimplifier.hpp"
#include "src/StrandSimplifier.hpp"

//#include "src/ReAPr.hpp"

//#include "src/Alexander.hpp"
//
//#include "src/Seifert.hpp"

#include "src/Link_3D.hpp"

#include "src/Collision.hpp"

#include "src/LinearHomotopy_3D.hpp"

#include "src/AlexanderStrandMatrix.hpp"
#include "src/AlexanderFaceMatrix.hpp"


#include "src/PolygonFolder.hpp"

#include "src/AffineTransform.hpp"
#include "src/ClangMatrix.hpp"
#include "src/ClangAffineTransform.hpp"
#include "src/ClangQuaternionTransform.hpp"
#include "src/ClisbyTree.hpp"

