#pragma once

// Boost: Rather slow, but maybe I missed something.
#include <boost/multiprecision/cpp_int.hpp>

namespace Tools
{
    using BoostInt128 = boost::multiprecision::int128_t;
    using BoostInt256 = boost::multiprecision::int256_t;
    
    template<> constexpr const char * TypeName<BoostInt128>  = "BoostInt128";
    template<> constexpr const char * FullTypeName<BoostInt128>  = "boost::multiprecision::int128_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<BoostInt128> = true;
        template<> constexpr bool ComplexQ<BoostInt128> = false;
    }
    
    TOOLS_FORCE_INLINE double ToDouble( cref<BoostInt128> number )
    {
        return static_cast<double>(number);
    }
    
    // String generator.
    std::string ToString( cref<BoostInt128> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }
    
    template<SignedIntQ R = FastInt8>
    TOOLS_FORCE_INLINE constexpr R Sign( cref<BoostInt128> number )
    {
        if( number < 0 ) { return R(-1); }
        if( number > 0 ) { return R( 1); }
        return R(0);
    }
    
    TOOLS_FORCE_INLINE constexpr bool NegativeQ( cref<BoostInt128> a )
    {
        return (a < 0);
    }
    
    TOOLS_FORCE_INLINE constexpr bool PositiveQ( cref<BoostInt128> a )
    {
        return (a > 0);
    }
    
    TOOLS_FORCE_INLINE constexpr bool ZeroQ( cref<BoostInt128> a )
    {
        return (a == 0);
    }

    // String generator to make it work with OutString.
    template<> struct ToChars<BoostInt128>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 40;
        
        ToCharResult operator()( char * begin, char * end, const BoostInt128 & x ) const
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
    
    
    template<> constexpr const char * TypeName<BoostInt256>  = "BoostInt256";
    template<> constexpr const char * FullTypeName<BoostInt256>  = "boost::multiprecision::int256_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<BoostInt256> = true;
        template<> constexpr bool ComplexQ<BoostInt256> = false;
    }
    
    TOOLS_FORCE_INLINE double ToDouble( cref<BoostInt256> number )
    {
        return static_cast<double>(number);
    }
    
    // String generator.
    std::string ToString( cref<BoostInt256> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }
    
    template<SignedIntQ R = FastInt8>
    TOOLS_FORCE_INLINE constexpr R Sign( cref<BoostInt256> number )
    {
        if( number < 0 ) { return R(-1); }
        if( number > 0 ) { return R( 1); }
        return R(0);
    }
    
    TOOLS_FORCE_INLINE constexpr bool NegativeQ( cref<BoostInt256> a )
    {
        return (a < 0);
    }
    
    TOOLS_FORCE_INLINE constexpr bool PositiveQ( cref<BoostInt256> a )
    {
        return (a > 0);
    }
    
    TOOLS_FORCE_INLINE constexpr bool ZeroQ( cref<BoostInt256> a )
    {
        return (a == 0);
    }
    
    // String generator to make it work with OutString.
    template<> struct ToChars<BoostInt256>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 79;
        
        ToCharResult operator()( char * begin, char * end, const BoostInt256 & x ) const
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
