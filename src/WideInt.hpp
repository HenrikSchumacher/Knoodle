#pragma once

#include <bitset>

namespace Knoodle
{
    
#ifdef TOOLS_INT128_AVAILABLE
    /*!@brief Biggest limb type on this system.*/
    using DefaultLimb_T = UInt64;
    /*!@brief Biggest comp type on this system.*/
    using DefaultComp_T = UInt128;
#else
    /*!@brief Biggest limb type on this system.*/
    using DefaultLimb_T = UInt32;
    /*!@brief Biggest comp type on this system.*/
    using DefaultComp_T = UInt64;
#endif
    
    /*!@brief Biggest limb type on this system.*/
    using DefaultSignedLimb_T = ToSigned<DefaultLimb_T>;
    /*!@brief Biggest comp type on this system.*/
    using DefaultSignedComp_T = ToSigned<DefaultComp_T>;
    
    /*!@brief A class for wide integers.
     
     * An integer is represented by `limb_count` limbs of unsigned integer type `Limb_T`.
     * Computations are performed in the unsigned integer type _double limb_ `Comp_T`.
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
        
        using SignedLimb_T = ToSigned<Limb_T>;
        using SignedComp_T = ToSigned<Comp_T>;
        
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
        
        Limb_T limbs [limb_count] {{}}; // Initialize by 0.
        
    public:
        
        constexpr WideInt() = default;
        
        constexpr explicit WideInt( Limb_T a )
        {
            limbs[0] = a;
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
        constexpr explicit WideInt( SignedLimb_T a )
        :   WideInt { static_cast<Limb_T>(a)}
        {
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
        template<typename dummy = std::conditional_t<limb_count == Size_T(2),void,void>>
        constexpr explicit WideInt( Comp_T a )
        {
            limbs[0] = Lo_Limb(a);
            limbs[1] = Hi_Limb(a);
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
        template<typename dummy = std::conditional_t<limb_count == Size_T(2),void,void>>
        constexpr explicit WideInt( SignedComp_T a )
        : WideInt { static_cast<Comp_T>(a)}
        {
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
        constexpr explicit WideInt( cref<BitSet_T> a  )
        {
            std::copy_n(
                reinterpret_cast<const std::byte *>(&a),
                byte_count,
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
//                // TODO: This is dangerous because it may break in big-endian systems.
//                *this = WideInt( reinterpret_cast<const Limb_T *>(&a), sizeof(a)/sizeof(Limb_T) );
                
                UInt b = a;
                
                constexpr int n = static_cast<int>(sizeof(UInt)/sizeof(Limb_T)) - 1;
                
                for( int i = 0; i < n; ++i )
                {
                    limbs[i] = static_cast<Limb_T>(b);
                    b >> limb_bit_count;
                }
                limbs[n-1] = static_cast<Limb_T>(b);
            }
            else
            {
                eprint(ClassName()+"(" + TypeName<UInt> + "): To many bytes to fit into type.");
            }
        }
        
        
        template<SignedIntQ Int>
        constexpr WideInt( Int a )
        :   WideInt{ static_cast<ToUnsigned<Int>>( NegativeQ(a) ? -a : a) }
        {
            // TODO: Make this more efficient in the important case sizeof(Int) == sizeof(WideInt).
            
//            print("WideInt{ static_cast<ToUnsigned<Int>>(Abs(a)) }");
            if( NegativeQ(a) ) { this->Negate(); }
        }
        
        template<Size_T m>
        constexpr explicit WideInt( const std::array<Limb_T,m> a )
        :   WideInt( &*a.begin(), a.size() )
        {}
        
        template<int m, typename ExtComp_T, typename Void = std::enable_if_t<m <= limb_count, void>>
        constexpr explicit WideInt( cref<WideInt<m,Limb_T,ExtComp_T,signQ>> a )
        {
            std::copy_n( &a.limbs[0], m, &limbs[0] );
            // No need to fill up  with zeroes as WideInt is initialized by 0.
        }
        
    public:
        
        /*!@brief Return `i`-th limb, read only. The least significant limb is at position `0`.*/
        template<IntQ Int>
        TOOLS_FORCE_INLINE constexpr Limb_T operator[]( const Int i ) const
        {
            return limbs[i];
        }
        
        /*!@brief Return `i`-th limb. The least significant limb is at position `0`.*/
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

        Limb_T & MostSignificantLimb()
        {
            return limbs[limb_count-Idx(1)];
        }
        
        const Limb_T & MostSignificantLimb() const
        {
            return limbs[limb_count-Idx(1)];
        }
        
        SignedLimb_T & MostSignificantSignedLimb()
        {
            return reinterpret_cast<SignedLimb_T &>(
                this->MostSignificantLimb()
            );
        }
        
        const SignedLimb_T & MostSignificantSignedLimb() const
        {
            return reinterpret_cast<const SignedLimb_T &>(
                this->MostSignificantLimb()
            );
        }
        
//        /*!@brief Return the value of the sign bit.*/
//        TOOLS_FORCE_INLINE friend constexpr
//        bool SignBit( cref<WideInt> a )
//        {
//            // Not portable.
//            return get_bit(a.MostSignificantLimb(),limb_bit_count-Idx(1));
//        }
        
        /*!@brief Check whether this wide integer is negative.*/
        TOOLS_FORCE_INLINE friend constexpr
        bool NegativeQ( cref<WideInt> a )
        {
            if( !signQ ) return false;

            return NegativeQ(a.MostSignificantSignedLimb());
        }
        
        /*!@brief Check whether this wide integer is negative.*/
        TOOLS_FORCE_INLINE friend constexpr
        bool PositiveQ( cref<WideInt> a )
        {
            if constexpr ( signQ )
            {
                if( NegativeQ(a) ) return false;
            }
            
            if( ZeroQ(a) ) return false;
            
            return true;
        }
        
        /*!@brief Return the sign of  */
        template<SignedIntQ R = FastInt8>
        TOOLS_FORCE_INLINE friend constexpr
        R Sign( cref<This_T> a )
        {
            if constexpr ( signQ )
            {
                if( NegativeQ(a) ) return R(-1);
            }
            
            if( ZeroQ(a) ) return R(0);
            
            return R(1);
        }
        
        /*!@brief Comparison operator.*/
        TOOLS_FORCE_INLINE constexpr friend bool operator<( cref<This_T> a, cref<This_T> b )
        {
            if( NegativeQ(a) )
            {
                if( NegativeQ(b) )
                {
                    // If a and b have the same sign, then a - b cannot overflow.
                    return NegativeQ(a - b);
                }
                else
                {
                    return signQ;
                }
            }
            else
            {
                if( NegativeQ(b) )
                {
                    return !signQ;
                }
                else
                {
                    // If a and b have the same sign, then a - b cannot overflow.
                    return NegativeQ(a - b);
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
            if( NegativeQ(a) )
            {
                if( !NegativeQ(b) )
                {
                    return signQ ? std::strong_ordering::less : std::strong_ordering::greater;
                }
            }
            else
            {
                if( NegativeQ(b) )
                {
                    return !signQ ? std::strong_ordering::less : std::strong_ordering::greater;
                }
            }
            
            // If a and b have the same sign, then a - b cannot overflow.
            
            This_T c = a - b;
            
            if( NegativeQ(c) ) { return std::strong_ordering::less; }
            if( ZeroQ(c)     ) { return std::strong_ordering::equal; }
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
            
            // Carry Hi_Comp(X) is silently discarded.
            
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

#include "WideInt/bitwise.hpp"
#include "WideInt/long_mul.hpp"
#include "WideInt/long_fma.hpp"
#include "WideInt/long_det.hpp"
        
    public:

        template<IntQ T = signed __int128>
        T ToNumber()
        {
            static_assert(sizeof(T) >= byte_count, "");
            WideInt b = *this;
            
            if( NegativeQ(*this) ) { Negate(b); }
            
            T   x = 0;
            Idx s = 0;
            
            for( Idx i = 0; i < limb_count; ++i )
            {
                x |= static_cast<T>(b[i]) << s; // This shift should work with signed types.
                s += 8 * sizeof(Limb_T);
            }
            
            if( NegativeQ(*this) ) { x = -x; }
            
            return x;
        }
        
        /*!@brief Conversion to `double`.*/
        friend double ToDouble( cref<WideInt> a )
        {
            WideInt b = a;
            
            bool negativeQ = NegativeQ(b);
            
            if( negativeQ ) { b.Negate(); }
            
            double x = 0;
            Idx    s = 0;
            
            for( Idx i = 0; i < limb_count; ++i )
            {
                x += static_cast<double>(b[i]) * std::pow(double(2),s);
                s += Idx(CHAR_BIT) * sizeof(Limb_T);
            }
            
            if( negativeQ ) { x = -x; }
            
            return x;
        }
        
        /*!@brief Explicit cast operator to `double`.*/
        explicit operator double() const
        {
            return ToDouble(*this);
        }
        
        /*!@ Fill the limb buffer with (pseudo)random numbers. With every call the function `fun` is required to generate number that is convertible to `Limb_T`*/
        template<typename RandomFunction_T>
        TOOLS_FORCE_INLINE This_T & Randomize( mref<RandomFunction_T> fun )
        {
            for( Idx i = 0; i < limb_count; ++i )
            {
                limbs[i] = static_cast<Limb_T>(fun());
            }
            
            return *this;
        }
        
        friend std::string ToString( cref<This_T> c )
        {
            return ClassName() + std::string(OutString::FromVector(&c.limbs[0],limb_count));
        }
        
        friend std::ostream & operator << (std::ostream &s, const This_T & c )
        {
            return s << ClassName() << OutString::FromVector(&c.limbs[0],limb_count);
        }
        
    public:
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("WideInt")
            + "<" + ToString(limb_count)
            + "," + TypeName<Limb_T>
            + "," + TypeName<Comp_T>
            + "," + ToString(signQ)
            + ">";
        }
    };
    
    // Some convenience type casts.

    template<IntQ Int, UnsignedIntQ Limb_T, UnsignedIntQ Comb_T>
    using ToWideInt = WideInt<
        CeilDivide(sizeof(Int),sizeof(Limb_T)), Limb_T, Comb_T, SignedIntQ<Int>
    >;
    
    template<IntQ Int>
    using DefaultWideInt = ToWideInt<Int,DefaultLimb_T,DefaultComp_T>;
    
    /*!@brief The smallest signed `WideInt` using the standard settings for limb type and comp type that has at least `bit_count` bits.*/
    template<Size_T bit_count>
    using WInt = WideInt<
        CeilDivide(bit_count, CHAR_BIT * sizeof(DefaultLimb_T)),
        DefaultLimb_T,
        DefaultComp_T,
        true
    >;
    
    /*!@brief The smallest unsigned `WideInt` using the standard settings for limb type and comp type that has at least `bit_count` bits.*/
    template<Size_T bit_count>
    using WUInt = WideInt<
        CeilDivide(bit_count, CHAR_BIT * sizeof(DefaultLimb_T)),
        DefaultLimb_T,
        DefaultComp_T,
        false
    >;
    
    using WInt64    = WInt< 64>;
    using WInt128   = WInt<128>;
    using WInt192   = WInt<192>;
    using WInt256   = WInt<256>;
    using WInt512   = WInt<512>;
    using WInt1024  = WInt<1024>;
    
    using WUInt64    = WUInt< 64>;
    using WUInt128   = WUInt<128>;
    using WUInt192   = WUInt<192>;
    using WUInt256   = WUInt<256>;
    using WUInt512   = WUInt<512>;
    using WUInt1024  = WUInt<1024>;
    
} // namespace Knoodle


namespace Tools
{
    template<> constexpr const char * TypeName<Knoodle::WInt64>    = "WInt64";
    template<> constexpr const char * TypeName<Knoodle::WInt128>   = "WInt128";
    template<> constexpr const char * TypeName<Knoodle::WInt192>   = "WInt192";
    template<> constexpr const char * TypeName<Knoodle::WInt256>   = "WInt256";
    template<> constexpr const char * TypeName<Knoodle::WInt512>   = "WInt512";
    template<> constexpr const char * TypeName<Knoodle::WInt1024>  = "WInt1024";
    
    template<> constexpr const char * TypeName<Knoodle::WUInt64>   = "WUInt64";
    template<> constexpr const char * TypeName<Knoodle::WUInt128>  = "WUInt128";
    template<> constexpr const char * TypeName<Knoodle::WUInt192>  = "WUInt192";
    template<> constexpr const char * TypeName<Knoodle::WUInt256>  = "WUInt256";
    template<> constexpr const char * TypeName<Knoodle::WUInt512>  = "WUInt512";
    template<> constexpr const char * TypeName<Knoodle::WUInt1024> = "WUInt1024";
    
    constexpr int const_evaluatedQ = -1;
    
    namespace Scalar
    {
        template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
        constexpr bool RealQ<Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>> = true;
        
        template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
        constexpr bool ComplexQ<Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>> = false;
    }
    
    // TODO: We need to make this work also for general WideInts.
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
    
    // TODO: We need to make this work also for general WideInts.
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

#include "WideInt/long_mul_extern.hpp"
