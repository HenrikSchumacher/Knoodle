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
            default:
            {
                eprint( "ToString: Argument s = " + ToString(s) + " is invalid." );
                return "Inactive";
            }
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
    
    template<IntQ Int, IntQ Int_2>
    CrossingState_T PDCodeHandedness( mptr<Int_2> X )
    {
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
    
    template<IntQ Int, IntQ Int_2, IntQ Int_3>
    Tensor2<Int,Int> PDCode_AppendHandedness( mptr<Int_2> pd, const Int_3 crossing_count )
    {
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




namespace Tools
{
    template<> struct ToChars<Knoodle::CrossingState_T>
    {
        using U = std::underlying_type_t<Knoodle::CrossingState_T>;
        
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 11;
        
        ToCharResult operator()( char * & begin, char * end, const Knoodle::CrossingState_T & s ) const
        {
            switch( s )
            {
                case Knoodle::CrossingState_T::Inactive:
                {
                    return CharArray("Inactive").ToChars(begin,end);
                }
                case Knoodle::CrossingState_T::RightHanded:
                {
                    return CharArray("RightHanded").ToChars(begin,end);
                }
                case Knoodle::CrossingState_T::LeftHanded:
                {
                    return CharArray("LeftHanded").ToChars(begin,end);
                }
            }
        }
    };
    
    
    template<> struct ToChars<Knoodle::ArcState_T>
    {
        using U = std::underlying_type_t<Knoodle::ArcState_T>;
        
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 8;
        
        ToCharResult operator()( char * & begin, char * end, const Knoodle::ArcState_T & s ) const
        {
            switch( s )
            {
                case Knoodle::ArcState_T::Inactive:
                {
                    return CharArray("Inactive").ToChars(begin,end);
                }
                case Knoodle::ArcState_T::Active:
                {
                    return CharArray("Active").ToChars(begin,end);
                }
            }
        }
    };
    
    
    
    
    namespace Chiral
    {
        enum class Group : UInt8
        {
            e  = (1 << 0),
            m  = (1 << 1),
            r  = (1 << 2),
            mr = (1 << 3)
        };
        
        enum class Coset : UInt8
        {
            e        = ToUnderlying(Group::e),
            m        = ToUnderlying(Group::m),
            r        = ToUnderlying(Group::r),
            mr       = ToUnderlying(Group::mr),
            
            e_m      = ToUnderlying(Group::e) | ToUnderlying(Group::m),
            e_r      = ToUnderlying(Group::e) | ToUnderlying(Group::r),
            e_mr     = ToUnderlying(Group::e) | ToUnderlying(Group::mr),
            
            m_r      = ToUnderlying(Group::m) | ToUnderlying(Group::r),
            m_mr     = ToUnderlying(Group::m) | ToUnderlying(Group::mr),
            r_mr     = ToUnderlying(Group::r) | ToUnderlying(Group::mr),
            
            e_m_r_mr = ToUnderlying(Group::e) | ToUnderlying(Group::m) | ToUnderlying(Group::r) | ToUnderlying(Group::mr),
            
            Invalid  = 0
        };
        
    } // namespace Chiral
    
    
    std::string ToString( Chiral::Coset type )
    {
        using Chiral::Coset;
        
        switch(type)
        {
            case Coset::e        : return "e";
            case Coset::m        : return "m";
            case Coset::r        : return "r";
            case Coset::mr       : return "mr";
                
            case Coset::e_m      : return "e/m";
            case Coset::e_r      : return "e/r";
            case Coset::e_mr     : return "e/mr";
                
            case Coset::m_r      : return "m/r";
            case Coset::m_mr     : return "m/mr";
            case Coset::r_mr     : return "r/mr";
                
            case Coset::e_m_r_mr : return "e/m/r/mr";
                
            default: return "Invalid";
        }
    }
    
    Chiral::Coset StringToCoset( std::string_view s )
    {
        using Chiral::Coset;
        
        switch (s.size())
        {
            case 1:
            {
                if(s == "e")
                {
                    return Coset::e;
                }
                else if(s == "m")
                {
                    return Coset::m;
                }
                else if(s == "r")
                {
                    return Coset::r;
                }
                else
                {
                    return Coset::Invalid;
                }
            }
            case 2:
            {
                if(s == "mr")
                {
                    return Coset::mr;
                }
                else
                {
                    return Coset::Invalid;
                }
            }
            case 3:
            {
                if(s == "e/m")
                {
                    return Coset::e_m;
                }
                else if(s == "e/r")
                {
                    return Coset::e_r;
                }
                else if(s == "m/r")
                {
                    return Coset::m_r;
                }
                else
                {
                    return Coset::Invalid;
                }
            }
            case 4:
            {
                if(s == "e/mr")
                {
                    return Coset::e_mr;
                }
                else if(s == "m/mr")
                {
                    return Coset::m_mr;
                }
                else if(s == "r/mr")
                {
                    return Coset::r_mr;
                }
                else
                {
                    return Coset::Invalid;
                }
            }
            default:
            {
                if( s == "e/m/r/mr" )
                {
                    return Coset::e_m_r_mr;
                }
                else
                {
                    return Coset::Invalid;
                }
            }
        }
    }
    
    Chiral::Coset ChiralityTransform( Chiral::Coset c, Chiral::Group g  )
    {
        using Chiral::Group;
        using Chiral::Coset;
        switch( g )
        {
            case Group::e  : return c;
            case Group::m  :
            {
                UInt8 a = ToUnderlying(c);
                return Coset(
                    (get_bit(a,0)<<1) | (get_bit(a,1)<<0) | (get_bit(a,2)<<3) | (get_bit(a,3)<<2)
                );
            }
            case Group::r  :
            {
                UInt8 a = ToUnderlying(c);
                return Coset(
                    (get_bit(a,0)<<2) | (get_bit(a,1)<<3) | (get_bit(a,2)<<0) | (get_bit(a,3)<<1)
                );
            }
            case Group::mr :
            {
                UInt8 a = ToUnderlying(c);
                return Coset(
                    (get_bit(a,0)<<3) | (get_bit(a,1)<<2) | (get_bit(a,2)<<1) | (get_bit(a,3)<<0)
                );
            }
            default: return Coset::Invalid;
        }
    }
    
    Chiral::Coset ChiralityTransform( Chiral::Coset c, bool mirrorQ, bool reverseQ )
    {
        using Chiral::Group;
        
        if( mirrorQ )
        {
            if( reverseQ )
            {
                return ChiralityTransform(c, Group::mr );
            }
            else
            {
                return ChiralityTransform(c, Group::m );
            }
        }
        else
        {
            if( reverseQ )
            {
                return ChiralityTransform(c, Group::r );
            }
            else
            {
                return c;
            }
        }
    }
    
} // namespace Tools
