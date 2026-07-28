#pragma once

namespace Knoodle
{
    /*!@brief A class for wide integers.
     
     * The integer is represented by `limb_count` limbs of type `Limb_T`.
     * Computations are performed in the _double limb_ `Comp_T`.
     *
     * @tparam limb_count_ The number of limbs to use.
     *
     * @tparam Limb_T_ The unsigned integral type used to store a limb.
     *
     * @tparam Comp_T_ The unsigned integral type used for computations. It needs to be at least twice as wide as `Limb_T`, and it should be the fastest type the processor has available.
     */
    
    template<int limb_count_, UnsignedIntQ Limb_T_, UnsignedIntQ Comp_T_, bool signQ>
    class WideInt
    {
    public:
        
        using Limb_T = Limb_T_;
        using Comp_T = Comp_T_;
        using Idx    = std::int_fast32_t;
        
        static constexpr Idx limb_count = static_cast<Idx>(limb_count_);
        
        using This_T     = WideInt<    limb_count_,Limb_T,Comp_T,signQ>;
        using Prod_T     = WideInt<2 * limb_count_,Limb_T,Comp_T,signQ>;
        
        using Signed_T   = WideInt<    limb_count_,Limb_T,Comp_T,true>;
        using Unsigned_T = WideInt<    limb_count_,Limb_T,Comp_T,false>;
        
        static constexpr Idx limb_byte_count = sizeof(Limb_T);
        static constexpr Idx comp_byte_count = sizeof(Comp_T);
        static constexpr Idx limb_bit_count  = limb_byte_count * CHAR_BIT;
        static constexpr Idx comp_bit_count  = comp_byte_count * CHAR_BIT;
        static constexpr Idx bit_count       = limb_count * limb_bit_count;
        static constexpr Idx byte_count      = limb_count * limb_byte_count;
        
        static_assert(comp_bit_count >= Idx(2) * limb_bit_count,"");
        
        /*!@ A std::bitset with the same width as the internal list of limbs. Used to accelerate bitwise operations.*/
        using BitSet_T = std::bitset<bit_count>;
        
        static constexpr Limb_T zero_limb = Limb_T{0};
        static constexpr Comp_T zero_comp = Comp_T{0};
        
        static constexpr Limb_T max_limb  = static_cast<Limb_T>(-1);
        static constexpr Limb_T sign_mask = static_cast<Limb_T>(Limb_T(1) << (limb_bit_count-1));
        
        static constexpr Comp_T lo_mask   = Comp_T{max_limb};
        static constexpr Comp_T hi_mask   = Comp_T{lo_mask  << limb_bit_count};
        
        
        
        TOOLS_FORCE_INLINE static constexpr Limb_T As_Limb( cref<Comp_T> a )
        {
            return static_cast<Limb_T>(a); // No need to mask here. Unsigned integers are simply truncated.
        }
        
        template<IntQ T>
        TOOLS_FORCE_INLINE static constexpr Comp_T As_Comp( cref<T> a )
        {
            return static_cast<Comp_T>(a);
        }
        
        TOOLS_FORCE_INLINE static constexpr Comp_T Hi_Comp( cref<Comp_T> a )
        {
            if( comp_bit_count > Idx(2) * limb_bit_count )
            {
                // If Comp_T is too large, we might have to mask here.
                return (a >> limb_bit_count) & lo_mask;
            }
            else
            {
                return a >> limb_bit_count;
            }
        }
        
        TOOLS_FORCE_INLINE static constexpr Comp_T Lo_Comp( cref<Comp_T> a )
        {
            return As_Comp(As_Limb(a));
        }
        
        TOOLS_FORCE_INLINE static constexpr Limb_T Hi_Limb( cref<Comp_T> a )
        {
            // This automatically discards extra digits of Comp_t.
            return As_Limb(a >> limb_bit_count);
        }
        
        TOOLS_FORCE_INLINE static constexpr Limb_T Lo_Limb( cref<Comp_T> a )
        {
            // This automatically discards extra digits of Comp_t.
            return As_Limb(a);
        }
        
        TOOLS_FORCE_INLINE constexpr friend const BitSet_T & As_BitSet( cref<This_T> a )
        {
            const BitSet_T * a_ptr = reinterpret_cast<const BitSet_T *>(&a.limbs[0]);
            return *a_ptr;
        }
        
        TOOLS_FORCE_INLINE constexpr friend const BitSet_T & As_BitSet( mref<This_T> a )
        {
            const BitSet_T * a_ptr = reinterpret_cast<BitSet_T *>(&a.limbs[0]);
            return *a_ptr;
        }
        
    private:
        
        Limb_T limbs[limb_count] = {}; // Initialize by 0.
        
    public:
        
        constexpr WideInt() = default;
        
        constexpr explicit WideInt( Limb_T a )
        {
            limbs[0] = a;
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
        constexpr explicit WideInt( cref<BitSet_T> a  )
        {
            copy_buffer<byte_count>(
                reinterpret_cast<const std::byte *>(&a),
                reinterpret_cast<      std::byte *>(&limbs[0])
            );
        }
        
        constexpr explicit WideInt( cptr<Limb_T> a  )
        {
            // CAUTION: We do _not_ use memcopy because that might not be portable.
            for( int i = 0; i < limb_count; ++i )
            {
                limbs[i] = a[i];
            }
        }
        
        constexpr explicit WideInt( cptr<Limb_T> a, Idx a_size )
        {
            if( a_size > limb_count )
            {
                eprint(ClassName()+"(const Limb_T *): More limbs than the type can store.");
                return;
            }
            else
            {
                for( int i = 0; i < a_size; ++i )
                {
                    limbs[i] = a[i];
                }
                // No need to fill up  with zeroes as WideInt is initialized by 0.
            }
        }

        template<UnsignedIntQ UInt>
        constexpr WideInt( UInt a )
        {
            if( sizeof(a) <= sizeof(Limb_T) )
            {
                limbs[0] = static_cast<Limb_T>(a);
                // No need to fill up  with zeroes as WideInt is initialized by 0.
            }
            else if( sizeof(a) <= byte_count )
            {
                // TODO: This is dangerous because it may break in big-endian systems.
                *this = WideInt( reinterpret_cast<const Limb_T *>(&a), sizeof(a)/sizeof(Limb_T) );
            }
            else
            {
                eprint(ClassName()+"(const " + TypeName<UInt> + " &): To many bytes to fit into type.");
            }
        }
        
        template<SignedIntQ Int>
        constexpr WideInt( Int a )
        :   WideInt{ static_cast<ToUnsigned<Int>>(Abs(a)) }
        {
//            print("WideInt{ static_cast<ToUnsigned<Int>>(Abs(a)) }");
            if( a < Int{0} ) { this->Negate(); }
        }
        
        template<Size_T m>
        constexpr explicit WideInt( const std::array<Limb_T,m> a )
        :   WideInt( &*a.begin(), a.size() )
        {}
        
        template<int m, typename ExtComp_T, typename Void = std::enable_if_t<m <= limb_count, void>>
        constexpr explicit WideInt( cref<WideInt<m,Limb_T,ExtComp_T,signQ>> a )
        {
            copy_buffer<m>( &a.limbs[0], &limbs[0] );
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
    public:
        
        /*!@brief Return `i`-th limb, read only.*/
        template<IntQ Int>
        TOOLS_FORCE_INLINE constexpr Limb_T operator[]( const Int i ) const
        {
            return limbs[i];
        }
        
        /*!@brief Return `i`-th limb.*/
        template<IntQ Int>
        TOOLS_FORCE_INLINE constexpr Limb_T & operator[]( const Int i )
        {
            return limbs[i];
        }
        
        /*!@brief Preincrement.*/
        TOOLS_FORCE_INLINE constexpr This_T & operator++()
        {
            // Exploiting that Limb_T wraps around.
            static_assert(static_cast<Limb_T>(max_limb + Limb_T(1)) == zero_limb,"");
            
            for( Idx k = 0; k < limb_count; ++k )
            {
                ++limbs[k];
                if( limbs[k] != zero_limb ) { break; }
            }
            
            return *this;
        }
        
        /*!@brief Predecrement.*/
        TOOLS_FORCE_INLINE constexpr This_T & operator--()
        {
            // Exploiting that Limb_T wraps around.
            static_assert(static_cast<Limb_T>(zero_limb - Limb_T(1)) == max_limb,"");
            
            for( int k = 0; k < limb_count; ++k )
            {
                --limbs[k];
                if( limbs[k] != max_limb ) { break; }
            }
            
            return *this;
        }
        
        /*!@brief Negate this wide integer.*/
        TOOLS_FORCE_INLINE constexpr This_T & Negate()
        {
//            for( Idx k = 0; k < limb_count; ++k )
//            {
////                limbs[k] ^= max_limb;
//                limbs[k] = ~limbs[k];
//            }

            (*this) = ~(*this);
            ++(*this);
            
            return *this;
        }
        
        /*!@brief Return negative of this wide integer.*/
        TOOLS_FORCE_INLINE constexpr This_T operator-() const
        {
            This_T c { *this };
            (void)c.Negate();
            return c;
        }

        
        /*!@brief Return the value of the sign but.*/
        TOOLS_FORCE_INLINE constexpr bool SignBit() const
        {
            return get_bit(limbs[limb_count-Idx(1)],limb_bit_count-Idx(1));
        }
        
        /*!@brief Check whether this wide integer is negative.*/
        TOOLS_FORCE_INLINE constexpr bool NegativeQ() const
        {
            if( !signQ ) return false;
            
            return SignBit();
        }
        
        /*!@brief Check whether this wide integer is positive. (This costs as much as `NegativeQ` and `ZeroQ` together, so it is relatively expensive.) */
        TOOLS_FORCE_INLINE constexpr bool PositiveQ() const
        {
            if constexpr ( signQ )
            {
                if( NegativeQ() ) return false;
            }
            
            if( ZeroQ() ) return false;
            
            return true;
        }
        
        /*!@brief Return the sign of  */
        template<SignedIntQ Sign_T = Int8>
        TOOLS_FORCE_INLINE constexpr friend Sign_T Sign( cref<This_T> a )
        {
            if constexpr ( signQ )
            {
                if( a.NegativeQ() ) return Sign_T(-1);
            }
            
            if( a.ZeroQ() ) return Sign_T(0);
            
            return Sign_T(1);
        }
        
        /*!@brief Comparison operator.*/
        TOOLS_FORCE_INLINE constexpr friend bool operator<( cref<This_T> a, cref<This_T> b )
        {
            if( a.SignBit() )
            {
                if( b.SignBit() )
                {
                    // If a and b have the same sign, then a - b cannot overflow.
                    return (a - b).SignBit();
                }
                else
                {
                    return signQ;
                }
            }
            else
            {
                if( b.SignBit() )
                {
                    return !signQ;
                }
                else
                {
                    // If a and b have the same sign, then a - b cannot overflow.
                    return (a - b).SignBit();
                }
            }
        }
        
        /*!@brief Comparison operator.*/
        TOOLS_FORCE_INLINE constexpr friend bool operator>=( cref<This_T> a, cref<This_T> b )
        {
            return !(a < b);
        }
        
        /*!@brief Comparison operator.*/
        TOOLS_FORCE_INLINE constexpr friend bool operator>( cref<This_T> a, cref<This_T> b )
        {
            return (b < a);
        }
        
        /*!@brief Comparison operator.*/
        TOOLS_FORCE_INLINE constexpr friend bool operator<=( cref<This_T> a, cref<This_T> b )
        {
            return !(b < a);
        }
            
        /*!@brief Three-way comparison operator.*/
        TOOLS_FORCE_INLINE constexpr friend std::strong_ordering operator<=>( cref<This_T> a, cref<This_T> b )
        {
            if( a.SignBit() )
            {
                if( !b.SignBit() )
                {
                    return signQ ? std::strong_ordering::less : std::strong_ordering::greater;
                }
            }
            else
            {
                if( b.SignBit() )
                {
                    return !signQ ? std::strong_ordering::less : std::strong_ordering::greater;
                }
            }
            
            // If a and b have the same sign, then a - b cannot overflow.
            
            This_T c = a - b;
            
            if( c.SignBit() ) { return std::strong_ordering::less; }
            if( c.ZeroQ()   ) { return std::strong_ordering::equal; }
            return std::strong_ordering::greater;
        }
        
        /*!@brief (Short) addition operator. The result is of the same type.*/
        TOOLS_FORCE_INLINE constexpr friend This_T operator+( cref<This_T> a, cref<This_T> b )
        {
            This_T c {};
            Comp_T X = As_Comp(a[0]) + As_Comp(b[0]);
            c[0] = Lo_Limb(X);
            
            for( int k = 1; k < limb_count; ++k )
            {
                X = As_Comp(a[k]) + As_Comp(b[k]) +  Hi_Comp(X);
                c[k] = Lo_Limb(X);
            }
            // Carry Hi_Comp(X) is silently lost.
            return c;
        }

        /*!@brief (Short) subtraction operator. The result is of the same type.*/
        TOOLS_FORCE_INLINE constexpr friend This_T operator-( cref<This_T> a, cref<This_T> b )
        {
            This_T c {};
            // Using 2's complement representation.
            Comp_T X = As_Comp(a[0]) + As_Comp(b[0] ^ max_limb) + Comp_T(1);
            c[0] = Lo_Limb(X);
            
            for( Idx k = 1; k < limb_count; ++k )
            {
                // Using 2's complement representation.
                X = As_Comp(a[k]) + As_Comp(b[k] ^ max_limb) +  Hi_Comp(X);
                c[k] = Lo_Limb(X);
            }
            // Carry Hi_Comp(X) is silently lost.
            return c;
        }

        
        // TODO: constexpr friend This_T operator*( cref<This_T> a, cref<This_T> b )

#include "WideInt/bitwise.hpp"
#include "WideInt/long_mul.hpp"
#include "WideInt/long_fma.hpp"
        
    public:
        
        TOOLS_FORCE_INLINE constexpr friend Prod_T long_det(
            cref<WideInt> a, cref<WideInt> b, cref<WideInt> c, cref<WideInt> d
        )
        {
            return long_mul(a,d) - long_mul(b,c);
        }
        
        
        template<IntQ T = signed __int128>
        T ToNumber()
        {
            static_assert(sizeof(T) >= byte_count, "");
            WideInt b = *this;
            
            if( NegativeQ() ) { b.Negate(); }
            
            T   x = 0;
            Idx s = 0;
            
            for( Idx i = 0; i < limb_count; ++i )
            {
                x |= static_cast<T>(b[i]) << s; // This shift should work with signed types.
                s += 8 * sizeof(Limb_T);
            }
            
            if( NegativeQ() ) { x = -x; }
            
            return x;
        }
        
        friend double ToDouble( cref<WideInt> a )
        {
            WideInt b = a;
            
            bool negativeQ = b.NegativeQ();
            
            if( negativeQ ) { b.Negate(); }
            
            double x = 0;
            Idx    s = 0;
            
            for( Idx i = 0; i < limb_count; ++i )
            {
                x += static_cast<double>(b[i]) * std::pow(double(2),s);
                s += Idx(8) * sizeof(Limb_T);
            }
            
            if( negativeQ ) { x = -x; }
            
            return x;
        }
        
        /*!@ Fill the limb buffer with (pseudo)random numbers. With every call the function `fun` is required to generate number that is convertible to `Limb_T`*/
        template<typename RandomFunction_T>
        TOOLS_FORCE_INLINE void Randomize( mref<RandomFunction_T> fun )
        {
            for( Idx i = 0; i < limb_count; ++i )
            {
                limbs[i] = static_cast<Limb_T>(fun());
            }
        }
        
//        friend std::string ToString( cref<This_T> c )
//        {
//            return
//            ClassName() + std::string(OutString::FromVector(&c.limbs[0],limb_count));
//        }
        
        friend std::string ToString( cref<This_T> c )
        {
            return ToString(ToDouble(c));
        }
        
        template<typename CharT,typename Traits>
        std::stringstream & operator<<( mref<std::basic_ostream<CharT,Traits>&> s ) const
        {
            s << ToString(*this);
        }
        
    public:
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("WideInt")
            + "<" + to_ct_string(limb_count)
            + "," + TypeName<Limb_T>
            + "," + TypeName<Comp_T>
            + "," + to_ct_string(signQ)
            + ">";
        }
    };
    
    // Some convenience type cast.


    using WInt8     = WideInt<1,UInt8,UInt16,true>;
    using WUInt8    = WideInt<1,UInt8,UInt16,false>;
    
    using WInt16    = WideInt<1,UInt16,UInt32,true>;
    using WUInt16   = WideInt<1,UInt16,UInt32,false>;
    
    using WInt32    = WideInt<1,UInt32,UInt64,true>;
    using WUInt32   = WideInt<1,UInt32,UInt64,false>;
    
#ifdef TOOLS_INT128_AVAILABLE

    using WInt64    = WideInt< 1,UInt64,UInt128,true>;
    using WInt128   = WideInt< 2,UInt64,UInt128,true>;
    using WInt256   = WideInt< 4,UInt64,UInt128,true>;
    using WInt512   = WideInt< 8,UInt64,UInt128,true>;
    using WInt1024  = WideInt<16,UInt64,UInt128,true>;
    
    using WUInt64   = WideInt< 1,UInt64,UInt128,false>;
    using WUInt128  = WideInt< 2,UInt64,UInt128,false>;
    using WUInt256  = WideInt< 4,UInt64,UInt128,false>;
    using WUInt512  = WideInt< 8,UInt64,UInt128,false>;
    using WUInt1024 = WideInt<16,UInt64,UInt128,false>;
#else

    using WInt64    = WideInt< 2,UInt32,UInt64,true>;
    using WInt128   = WideInt< 4,UInt32,UInt64,true>;
    using WInt256   = WideInt< 8,UInt32,UInt64,true>;
    using WInt512   = WideInt<16,UInt32,UInt64,true>;
    using WInt1024  = WideInt<32,UInt32,UInt64,true>;
    
    using WUInt64   = WideInt< 2,UInt32,UInt64,false>;
    using WUInt128  = WideInt< 4,UInt32,UInt64,false>;
    using WUInt256  = WideInt< 8,UInt32,UInt64,false>;
    using WUInt512  = WideInt<16,UInt32,UInt64,false>;
    using WUInt1024 = WideInt<32,UInt32,UInt64,false>;
#endif

    
} // namespace Knoodle


namespace Tools
{
    //    template<> constexpr const char * TypeName<Knoodle::WInt64>    = "WInt64";
    //    template<> constexpr const char * TypeName<Knoodle::WInt128>   = "WInt128";
    //    template<> constexpr const char * TypeName<Knoodle::WInt256>   = "WInt256";
    //    template<> constexpr const char * TypeName<Knoodle::WInt512>   = "WInt512";
    //    template<> constexpr const char * TypeName<Knoodle::WInt1024>  = "WInt1024";
    //
    //    template<> constexpr const char * TypeName<Knoodle::WUInt64>   = "WUInt64";
    //    template<> constexpr const char * TypeName<Knoodle::WUInt128>  = "WUInt128";
    //    template<> constexpr const char * TypeName<Knoodle::WUInt256>  = "WUInt256";
    //    template<> constexpr const char * TypeName<Knoodle::WUInt512>  = "WUInt512";
    //    template<> constexpr const char * TypeName<Knoodle::WUInt1024> = "WUInt1024";
    
    
    constexpr int const_evaluatedQ = -1;
    
    //    // default implementation
    template<typename T>
    struct TypeNameHelper
    {
        static constexpr std::string name() { return "UnknownType"; }
    };
    
    
    template<typename T>
    constexpr const std::string newTypeName = TypeNameHelper<T>::name();
    
    
    template<> constexpr std::string TypeNameHelper<Int64>::name()
    {
        return "Int64";
    }
    
    template<> constexpr std::string TypeNameHelper<Int128>::name()
    {
        return "Int128";
    }
    
    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T,bool signQ>
    struct TypeNameHelper<typename Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>>
    {
        //        static constexpr std::string name()
        //        {
        //            return std::string("WI<") + std::string(to_ct_string(limb_count)) + "," + TypeName<Int64> + "," + TypeName<Int128> + ">";
        //        }
        using Class_T = Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>;
        
        static constexpr std::string name()
        {
            return Class_T::ClassName();
        }
    };
    
//    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T>
//    struct TypeNameHelper<typename Knoodle::WideInt<limb_count,Limb_T,Comp_T,false>>
//    {
//        static constexpr std::string name()
//        {
//            return std::string("WUI<") + std::string(to_ct_string(limb_count)) + "," + TypeName<Int64> + "," + TypeName<Int128> + ">";
//        }
//    };
    
    
//    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T,
//        typename T = Knoodle::WideInt<limb_count,Limb_T,Comp_T,true>
//    >
//    template<> constexpr std::string TypeNameHelper<T>::name()
//    {
//        return std::string("WI<") + newTypeName<Int64> + std::string(",") + newTypeName<Int128> + ">";
//    }
    
//    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T,
//        typename T = Knoodle::WideInt<limb_count,Limb_T,Comp_T,true>
//    >
//    template<>
//    constexpr std::string TypeNameHelper<T>::name()
//    {
//        return "A";
//    }
    
//    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T>
//    struct TypeNameHelper<
//        typename Knoodle::WideInt<limb_count,Limb_T,Comp_T,true>>
//    >
//    {
//        static constexpr std::string name()
//        {
//            return = std::string("Int<")
//            + std::string(to_ct_string(limb_count))
//            + std::string(",")
//            + newTypeName<Limb_T>()
//            + std::string(",")
//            //        + newTypeName<Comp_T>
//            + std::string(to_ct_string(true))
//            + std::string(">");
//        }
//    };
//
//    template<>
//    constexpr std::string
//    newTypeName<Int128> = std::string("Int<") + std::string(to_ct_string(128)) + std::string(">");

    
    namespace Scalar
    {
        template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
        constexpr bool RealQ<Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>> = true;
    }
    
    // String generator to make it work with OutString.
    template<> struct ToChars<Knoodle::WInt128>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 40;
        
        ToCharResult operator()( char * begin, char * end, const Knoodle::WInt128 & x ) const
        {
            std::string s = ToString(x);
            
            char * ptr = &begin[s.size()];
            
            if( ptr <= end )
            {
                std::copy(s.begin(),s.end(),begin);
                return ToCharResult{ .ptr = ptr, .failedQ = false };
            }
            else
            {
                return ToCharResult{ .ptr = begin, .failedQ = true };
            }
        }
    };
    
    // String generator to make it work with OutString.
    template<> struct ToChars<Knoodle::WInt256>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 79;
        
        ToCharResult operator()( char * begin, char * end, const Knoodle::WInt256 & x ) const
        {
            std::string s = ToString(x);
            
            char * ptr = &begin[s.size()];
            
            if( ptr <= end )
            {
                std::copy(s.begin(),s.end(),begin);
                return ToCharResult{ .ptr = ptr, .failedQ = false };
            }
            else
            {
                return ToCharResult{ .ptr = begin, .failedQ = true };
            }
        }
    };
    
} // namespace Tools


namespace Knoodle
{
    TOOLS_FORCE_INLINE constexpr
    WInt128 long_det( cref<Int64> a, cref<Int64> b, cref<Int64> c, cref<Int64> d )
    {
        return long_det( WInt64(a), WInt64(b), WInt64(c), WInt64(d) );
    }
    
    TOOLS_FORCE_INLINE constexpr
    WInt64 long_det( cref<Int32> a, cref<Int32> b, cref<Int32> c, cref<Int32> d )
    {
        return WInt64( Int64(a) * Int64(d) - Int64(b) * Int64(c));
    }
    
} // namespace Knoodle
