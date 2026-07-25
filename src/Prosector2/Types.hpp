#pragma once

// Boost: Rather slow, but maybe I missed something.
#include <boost/multiprecision/cpp_int.hpp>
namespace Knoodle
{
    using Int128  = boost::multiprecision::int128_t;
    using Int256  = boost::multiprecision::int256_t;
}


//// Wide integer class from https://github.com/ckormanyos/wide-integer
//// Only fast if WIDE_INTEGER_HAS_LIMB_TYPE_UINT64 is defined.
//#define WIDE_INTEGER_DISABLE_TRIVIAL_COPY_AND_STD_LAYOUT_CHECKS
//#define WIDE_INTEGER_HAS_LIMB_TYPE_UINT64
//#include "../../experimental/wide-integer/math/wide_integer/uintwide_t.h"
//namespace Knoodle
//{
//    // TODO: This has to_chars and from_chars.
//    using Int128  = ::math::wide_integer::int128_t;
//    using Int256  = ::math::wide_integer::int256_t;
//}


//// Faster than boost.
////#include "Integer.hpp"
//namespace Knoodle
//{
//    using Int128  = JIO::Integer<16, true>;
//    using Int256  = JIO::Integer<32, true>;
//}

namespace Tools
{
    template<> constexpr const char * TypeName<Knoodle::Int128>  = "I128";
    template<> constexpr const char * FullTypeName<Knoodle::Int128>  = "boost::multiprecision::int128_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<Knoodle::Int128> = true;
        template<> constexpr bool ComplexQ<Knoodle::Int128> = false;
    }
    
    // String generator.
    std::string ToString( cref<Knoodle::Int128> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }

    // String generator to make it work with OutString.
    template<> struct ToChars<Knoodle::Int128>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 40;
        
        ToCharResult operator()( char * begin, char * end, const Knoodle::Int128 & x ) const
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
    
    
    template<> constexpr const char * TypeName<Knoodle::Int256>  = "I256";
    template<> constexpr const char * FullTypeName<Knoodle::Int256>  = "boost::multiprecision::int256_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<Knoodle::Int256> = true;
        template<> constexpr bool ComplexQ<Knoodle::Int256> = false;
    }
    
    // String generator.
    std::string ToString( cref<Knoodle::Int256> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }
    
    // String generator to make it work with OutString.
    template<> struct ToChars<Knoodle::Int256>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 79;
        
        ToCharResult operator()( char * begin, char * end, const Knoodle::Int256 & x ) const
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
}
