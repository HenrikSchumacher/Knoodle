#pragma once

namespace Knoodle
{
    using PRNG_T = pcg64;
    
    enum class CrossingState_T : Int8
    {
        // Important! Active values are the only odd ones.
        RightHanded          =  1,
        LeftHanded           = -1,
        Inactive             =  0
    };
    
    
    static constexpr CrossingState_T BooleanToCrossingState( bool right_handedQ )
    {
        return right_handedQ ? CrossingState_T::RightHanded : CrossingState_T::LeftHanded;
    }
    
    CrossingState_T Switch( CrossingState_T s )
    {
        return CrossingState_T(- ToUnderlying(s));
    }
    
    inline std::string ToString( cref<CrossingState_T> s )
    {
        switch( s )
        {
            case CrossingState_T::Inactive             : return "Inactive";
                
            case CrossingState_T::RightHanded          : return "RightHanded";
                
            case CrossingState_T::LeftHanded           : return "LeftHanded";
                
            default:
            {
                eprint( "ToString: Argument s = " + ToString(s) + " is invalid." );
                return "Inactive";
            }
        }
    }

    inline std::ostream & operator<<( std::ostream & s, cref<CrossingState_T> state )
    {
        s << static_cast<int>(ToUnderlying(state));
        return s;
    }
    
    inline constexpr bool ActiveQ( const CrossingState_T & s )
    {
        return ToUnderlying(s);
    }
    
    inline constexpr bool RightHandedQ( const CrossingState_T & s )
    {
        return ToUnderlying(s) > Underlying_T<CrossingState_T>(0);
    }
    
    inline constexpr bool LeftHandedQ( const CrossingState_T & s )
    {
        return ToUnderlying(s) < Underlying_T<CrossingState_T>(0);
    }
    
    inline constexpr bool OppositeHandednessQ(
        const CrossingState_T & s_0, const CrossingState_T & s_1
    )
    {
        // TODO: Careful, this evaluates to true if both are `Inactive`.
        
        return ( Sign(ToUnderlying(s_0)) == -Sign(ToUnderlying(s_1)) );
    }
    
    inline constexpr bool SameHandednessQ(
        const CrossingState_T & s_0, const CrossingState_T & s_1
    )
    {
        // TODO: Careful, this evaluates to true if both are `Inactive`.
        
        return ( Sign(ToUnderlying(s_0)) == Sign(ToUnderlying(s_1)) );
    }

        
    enum class ArcState_T : UInt8
    {
        Active           =  1,
        Inactive         =  0
    };
    
    inline constexpr bool ActiveQ( const ArcState_T & s )
    {
        return ToUnderlying(s);
    }
    
    inline std::string ToString( cref<ArcState_T> s )
    {
        switch( s )
        {
            case ArcState_T::Active    : return "Active";
            case ArcState_T::Inactive  : return "Inactive";
        }
    }
    
    inline std::ostream & operator<<( std::ostream & s, cref<ArcState_T> state )
    {
        s << static_cast<int>(ToUnderlying(state));
        return s;
    }
    
    
    /*!
     * Deduces the handedness of a crossing from the entries `X[0]`, `X[1]`, `X[2]`, `X[3]` alone. This only works if every link component has more than two arcs and if all arcs in each component are numbered consecutively.
     */
    
    template<typename Int, typename Int_2>
    CrossingState_T PDCodeHandedness( mptr<Int_2> X )
    {
        static_assert( IntQ<Int>, "" );
        static_assert( IntQ<Int_2>, "" );
        
        const Int i = int_cast<Int>(X[0]);
        const Int j = int_cast<Int>(X[1]);
        const Int k = int_cast<Int>(X[2]);
        const Int l = int_cast<Int>(X[3]);
        
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
        
        // I "stole" this pretty neat code snippet from the KnotTheory Mathematica package by Dror Bar-Natan.
        
        if ( (i == j) || (k == l) || (j == l + 1) || (l > j + 1) )
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
            
            return CrossingState_T::RightHanded;
        }
        else if ( (i == l) || (j == k) || (l == j + 1) || (j > l + 1) )
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
            return CrossingState_T::LeftHanded;
        }
        else
        {
            eprint( std::string("PDHandedness: Handedness of {") + ToString(i) + "," + ToString(i) + "," + ToString(j) + "," + ToString(k) + "," + ToString(l) + "} could not be determined. Make sure that consecutive arcs on each component have consecutive labels (except the wrap-around, of course).");
            
            return CrossingState_T::Inactive;
        }
    }
    
    
    /*!Reads in an unsigned PD code and tries to infer the handedness of crossings.
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

    
} // namespace Knoodle
