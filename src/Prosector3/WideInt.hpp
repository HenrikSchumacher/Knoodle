#pragma once

namespace Knoodle
{
//    using Limb_T = UInt32;
//    static constexpr UInt64 limb_bit_count = 32;
//    using Dimb_T = UInt64;
    
    template<Size_T limb_count_, UnsignedIntQ Limb_T_, UnsignedIntQ Dimb_T_, bool signQ = false>
    class WideInt
    {
    public:
        
        using Limb_T = Limb_T_;
        using Dimb_T = Dimb_T_;
        
        static constexpr Size_T limb_count = limb_count_;
        
        using This_T = WideInt<    limb_count,Limb_T,Dimb_T,signQ>;
        using Prod_T = WideInt<2 * limb_count,Limb_T,Dimb_T,signQ>;
        
        static constexpr Size_T limb_bit_count = sizeof(Limb_T) * CHAR_BIT;
        static constexpr Size_T dimb_bit_count = sizeof(Dimb_T) * CHAR_BIT;
        static_assert(dimb_bit_count == Size_T(2) * limb_bit_count,"");
        
        static constexpr Limb_T zero_limb = Limb_T{0};
        static constexpr Limb_T max_limb  = static_cast<Limb_T>(-1);
        static constexpr Limb_T sign_mask = static_cast<Limb_T>(Limb_T(1) << (limb_bit_count-1));
        
        static constexpr Dimb_T lo_mask   = Dimb_T{max_limb};
        static constexpr Dimb_T hi_mask   = Dimb_T{lo_mask  << limb_bit_count};
        
        
        TOOLS_FORCE_INLINE static constexpr Dimb_T Hi_Dimb( cref<Dimb_T> a )
        {
            return a >> limb_bit_count;
        }
        
        TOOLS_FORCE_INLINE static constexpr Dimb_T Lo_Dimb( cref<Dimb_T> a )
        {
            return a & lo_mask;
        }
        

        TOOLS_FORCE_INLINE static constexpr Limb_T Hi_Limb( cref<Dimb_T> a )
        {
            return static_cast<Limb_T>(Hi_Dimb(a));
        }
        
        TOOLS_FORCE_INLINE static constexpr Limb_T Lo_Limb( cref<Dimb_T> a )
        {
            return static_cast<Limb_T>(Lo_Dimb(a));
        }
        
        struct Pair_T
        {
            Limb_T value {0};
            Limb_T carry {0};
            
            constexpr Pair_T() = default;
            
            constexpr Pair_T( cref<Limb_T> v, cref<Limb_T> c )
            :   value {v}
            ,   carry {c}
            {}
            
            constexpr explicit Pair_T( cref<Dimb_T> x )
            :   value {Lo_Limb(x)}
            ,   carry {Hi_Limb(x)}
            {}
            
            friend std::string ToString( cref<Pair_T> p )
            {
                return std::string("{ .value = ") +ToString(p.value) + ", .carry = " + ToString(p.carry) +" }";
            }
        };

        
        TOOLS_FORCE_INLINE static constexpr
        Dimb_T add_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b )
        {
            return static_cast<Dimb_T>(Dimb_T{a} + Dimb_T{b});
        }
        
        TOOLS_FORCE_INLINE static constexpr
        Pair_T add_as_Pair ( cref<Limb_T> a, cref<Limb_T> b )
        {
            return Pair_T{add_as_Dimb(a,b)};
        }
        
        
        TOOLS_FORCE_INLINE static constexpr
        Dimb_T add_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
        {
            return static_cast<Dimb_T>(Dimb_T{a} + Dimb_T{b} + Dimb_T{c});
        }
        
        TOOLS_FORCE_INLINE static constexpr
        Pair_T add_as_Pair ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
        {
            return Pair_T{add_as_Dimb(a,b,c)};
        }
        
        
        TOOLS_FORCE_INLINE static constexpr
        Dimb_T mul_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b )
        {
            return Dimb_T{a} * Dimb_T{b};
        }
        
        TOOLS_FORCE_INLINE static constexpr
        Pair_T mul_as_Pair ( cref<Limb_T> a, cref<Limb_T> b )
        {
            return Pair_T{mul_as_Dimb(a,b)};
        }
        
        
        TOOLS_FORCE_INLINE static constexpr
        Dimb_T fma_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
        {
            return (Dimb_T{a} * Dimb_T{b}) + Dimb_T{c};
        }
        
        TOOLS_FORCE_INLINE static constexpr
        Pair_T fma_as_Pair ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
        {
            return Pair_T{fma_as_Dimb(a,b,c)};
        }
        
//    private:
    public:
        
//        Tiny::Vector<limb_count,Limb_T,int> limbs { Limb_T(0) };
        Limb_T limbs[limb_count] = {};
        
    public:
        
        constexpr WideInt() = default;
        
        constexpr explicit WideInt( cref<Limb_T> a )
        :   WideInt()
        {
            limbs[0] = a;
        }
        
        // TODO: Can probably be optimized a lot.
        constexpr explicit WideInt( cref<ToSigned<Limb_T>> a )
        :   WideInt(  Limb_T(Abs(a)) )
        {
            if( a < ToSigned<Limb_T>(0) ) { this->Negate(); }
        }
        
        // TODO: I don't know whether I like this constructor.
        template<typename = typename std::enable_if_t<limb_count >= 2>>
        constexpr explicit WideInt( cref<Limb_T> a_0, cref<Limb_T> a_1 )
        :   WideInt()
        {
            limbs[0] = a_0;
            limbs[1] = a_1;
        }
        
//        template<typename = typename std::enable_if_t<limb_count >= 2>>
        constexpr explicit WideInt( cref<Dimb_T> a )
        :   WideInt()
        {
            limbs[0] = Lo_Limb(a);
            limbs[1] = Hi_Limb(a);
        }
        
        // TODO: Can probably be optimized a lot.
//        template<typename = typename std::enable_if_t<limb_count >= 2>>
        constexpr explicit WideInt( cref<ToSigned<Dimb_T>> a )
        :   WideInt( Dimb_T(Abs(a)) )
        {
            if( a < ToSigned<Dimb_T>(0) ) { this->Negate(); }
        }

        template<Size_T m, typename Void = std::enable_if_t<m <= limb_count, void>>
        constexpr explicit WideInt( cref<WideInt<m,Limb_T,Dimb_T>> a )
        :   WideInt()
        {
            copy_buffer( &a.limbs[0], &limbs[0], m );
        }
        
    public:
        
        template<IntQ Int>
        TOOLS_FORCE_INLINE constexpr Limb_T operator[]( const Int i ) const
        {
            return limbs[i];
        }
        
        template<IntQ Int>
        TOOLS_FORCE_INLINE constexpr Limb_T & operator[]( const Int i )
        {
            return limbs[i];
        }
        
        constexpr This_T & operator++()
        {
            // Exploiting that Limb_T wraps around.
            static_assert(static_cast<Limb_T>(max_limb + Limb_T(1)) == zero_limb,"");
            
            for( Size_T k = 0; k < limb_count; ++k )
            {
                ++limbs[k];
                if( limbs[k] != zero_limb ) { break; }
            }
            
            return *this;
        }
        
        constexpr This_T & operator--()
        {
            // Exploiting that Limb_T wraps around.
            static_assert(static_cast<Limb_T>(zero_limb - Limb_T(1)) == max_limb,"");
            
            for( Size_T k = 0; k < limb_count; ++k )
            {
                --limbs[k];
                if( limbs[k] != max_limb ) { break; }
            }
            
            return *this;
        }
        
        constexpr This_T & Negate()
        {
            for( Size_T k = 0; k < limb_count; ++k )
            {
                limbs[k] ^= max_limb;
            }
            
            ++(*this);
            
            return *this;
        }
        
        constexpr This_T operator-()
        {
            This_T c { *this };
            (void)c.Negate();
            return c;
        }
        
        constexpr bool NegativeQ() const
        {
            if( !signQ ) return false;
            
            return ( limbs[limb_count-1] & sign_mask );
        }
        
        constexpr bool ZeroQ() const
        {
            for( Size_T k = 0; k < limb_count; ++k )
            {
                if( limbs[k] != zero_limb ) { return false; }
            }
            return true;
        }
        
        constexpr bool PositiveQ() const
        {
            if( !signQ ) return true;
            
            if( NegativeQ() ) return false;
            
            if( ZeroQ() ) return false;
            
            return true;
        }
        
        constexpr friend Int8 Sign( cref<This_T> a )
        {
            
            if( a.NegativeQ() ) return Int8(-1);
            
            if( a.ZeroQ() ) return Int8(0);
            
            return Int8(1);
        }
        
        // TODO: We need something more efficient here.
        constexpr friend std::strong_ordering operator<=>( cref<This_T> a, cref<This_T> b )
        {
            This_T c = a - b;
            
            if( c.NegativeQ() ) { return std::strong_ordering::less; }
            
            if( c.ZeroQ() ) { return std::strong_ordering::equal; }
            
            return std::strong_ordering::greater;
        }
        
        constexpr friend This_T operator+( cref<This_T> a, cref<This_T> b )
        {
            // TODO: What if This_T is signed?
            
            This_T c {};
            Pair_T p = add_as_Pair( a[0], b[0] );
            c[0] = p.value;
            
            for( Size_T k = 1; k < limb_count; ++k )
            {
                p = add_as_Pair( a[k], b[k], p.carry );
                c[k] = p.value;
            }
            // p.carry is silently lost.
            return c;
        }
        
        constexpr friend This_T operator-( cref<This_T> a, cref<This_T> b )
        {
            This_T c {};
            Pair_T p = add_as_Pair( a[0], b[0] ^ max_limb, Limb_T(1) );
            c[0] = p.value;
            
            for( Size_T k = 1; k < limb_count; ++k )
            {
                p = add_as_Pair( a[k], b[k] ^ max_limb, p.carry );
                c[k] = p.value;
            }
            // p.carry is silently lost.
            return c;
        }
        
        // TODO: constexpr friend This_T operator*( cref<This_T> a, cref<This_T> b )

        
        
        friend std::string ToString( cref<This_T> c )
        {
            return OutString::FromVector(&c.limbs[0],limb_count);
        }
        
        
        std::stringstream & operator<<( mref<std::stringstream> s )
        {
            s << ToString(*this);
        }
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("WideInt")
                + "<" + ToString(limb_count)
                + "," + TypeName<Limb_T>
                + "," + TypeName<Dimb_T>
                + "," + ToString(signQ)
                + ">";
        }
    };
    
    
    
    
    template<typename Limb_T, typename Dimb_T,bool signQ>
    constexpr WideInt<4,Limb_T,Dimb_T,signQ> long_mul(
        cref<WideInt<2,Limb_T,Dimb_T,signQ>> a,
        cref<WideInt<2,Limb_T,Dimb_T,signQ>> b
    )
    {
        using Int    = WideInt<2,Limb_T,Dimb_T,signQ>;
        using LInt   = WideInt<4,Limb_T,Dimb_T,signQ>;
        using Pair_T = Int::Pair_T;
        
        LInt c {};
        
        Pair_T p {};
        Pair_T q {};
        
        p     = Int::fma_as_Pair( a[0], b[0], c[0] );
        c[0]  = p.value;
        c[1]  = p.carry; // We know that c[1] was 0, so this could not overflow.
//        if( p.carry != Limb_T{0} )
//        {
//            q     = add_as_Pair( c[1], p.carry );
//            c[1]  = q.value;
//            c[2] += q.carry;
//        }
        
        p     = Int::fma_as_Pair( a[1], b[0], c[1] );
        c[1]  = p.value;
        c[2]  = p.carry; // We know that c[2] was 0, so this could not overflow.
//        if( p.carry != Limb_T{0} )
//        {
//            q     = add_as_Pair( c[2], p.carry );
//            c[2]  = q.value;
//            c[3] += q.carry;
//        }
        
        p     = Int::fma_as_Pair( a[0], b[1], c[1] );
        c[1]  = p.value;
        // Caution: We want to do c[2] += p.carry, but this may induce a further carry!
        if( p.carry != Limb_T{0} )
        {
            q     = Int::add_as_Pair( c[2], p.carry );
            c[2]  = q.value;
            c[3] += q.carry;
        }
        
        p     = Int::fma_as_Pair( a[1], b[1], c[2] );
        c[2]  = p.value;
        c[3] += p.carry; // No carry can happen here.
//        if( p.carry != Limb_T{0} )
//        {
//            q     = add_as_Pair( c[3], p.carry );
//            c[3]  = q.value;
//            // q.carry must be 0.
//        }
        
        return c;
    }

}
