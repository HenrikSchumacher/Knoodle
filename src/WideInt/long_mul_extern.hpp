#pragma once

namespace Knoodle
{
    template<SignedIntQ A_T, SignedIntQ B_T>
    constexpr auto long_mul( cref<A_T> a, cref<B_T> b )
    {
        using SignedLimb_T = DefaultSignedLimb_T;
        using SignedComp_T = DefaultSignedComp_T;
        
        if constexpr ( sizeof(A_T) + sizeof(B_T) <= sizeof(SignedLimb_T) )
        {
            // use boost::int_max_value_t<V>
            return static_cast<SignedLimb_T>(a) * static_cast<SignedLimb_T>(b);
        }
        else if constexpr ( sizeof(A_T) + sizeof(B_T) <= sizeof(SignedComp_T) )
        {
            // use boost::int_max_value_t<V>
            return static_cast<DefaultSignedComp_T>(a) * static_cast<DefaultSignedComp_T>(b);
        }
        else
        {
            using WA_T = DefaultWideInt<A_T>;
            using WB_T = DefaultWideInt<B_T>;

            return long_mul( WA_T{a}, WB_T{b} );
        }
    }
    
    template<SignedIntQ A_T, SignedIntQ B_T, SignedIntQ C_T>
    constexpr auto long_fma( cref<A_T> a, cref<B_T> b, cref<C_T> c )
    {
        using SignedLimb_T = DefaultSignedLimb_T;
        using SignedComp_T = DefaultSignedComp_T;
        
        static_assert(sizeof(C_T) <= std::max(sizeof(A_T),sizeof(B_T)),"");
        
        if constexpr ( sizeof(A_T) + sizeof(B_T) <= sizeof(SignedLimb_T) )
        {
            // use boost::int_max_value_t<V>
            // TODO: Cast to smalles signed integer type that can hold sizeof(A_T) + sizeof(B_T) bits.
            return static_cast<SignedLimb_T>(a) * static_cast<SignedLimb_T>(b) + static_cast<SignedLimb_T>(c);
        }
        else if constexpr ( sizeof(A_T) + sizeof(B_T) <= sizeof(SignedComp_T) )
        {
            // use boost::int_max_value_t<V>
            // TODO: Cast to smalles signed integer type that can hold sizeof(A_T) + sizeof(B_T) bits.
            return static_cast<DefaultSignedComp_T>(a) * static_cast<DefaultSignedComp_T>(b) + static_cast<DefaultSignedComp_T>(c);
        }
        else
        {
            using WA_T = DefaultWideInt<A_T>;
            using WB_T = DefaultWideInt<B_T>;
            using WC_T = DefaultWideInt<C_T>;

            return long_fma( WA_T{a}, WB_T{b}, WC_T{c} );
        }
    }
    
    template<SignedIntQ T>
    constexpr auto long_det( cref<T> a, cref<T> b, cref<T> c, cref<T> d )
    {
        using SignedLimb_T = DefaultSignedLimb_T;
        using SignedComp_T = DefaultSignedComp_T;
        
        if constexpr ( sizeof(T) + sizeof(T) <= sizeof(SignedLimb_T) )
        {
            // use boost::int_max_value_t<V>
            return static_cast<SignedLimb_T>(a) * static_cast<SignedLimb_T>(d) - static_cast<SignedLimb_T>(b) * static_cast<SignedLimb_T>(c);
        }
        else if constexpr ( sizeof(T) + sizeof(T) <= sizeof(SignedComp_T) )
        {
            // use boost::int_max_value_t<V>
            return static_cast<DefaultSignedComp_T>(a) * static_cast<DefaultSignedComp_T>(d) - static_cast<DefaultSignedComp_T>(b) * static_cast<DefaultSignedComp_T>(c);
        }
        else
        {
            using T_T = DefaultWideInt<T>;

            return long_det( T_T{a}, T_T{b}, T_T{c}, T_T{d} );
        }
    }
    
} // namespace Knoodle
