#pragma once

// Boost: Rather slow, but maybe I missed something.
#include <boost/multiprecision/cpp_int.hpp>
//namespace Knoodle
//{
//    using Int128  = boost::multiprecision::int128_t;
//    using Int256  = boost::multiprecision::int256_t;
//}


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
    template<> constexpr const char * TypeName<boost::multiprecision::int128_t>  = "Boost_I128";
    template<> constexpr const char * FullTypeName<boost::multiprecision::int128_t>  = "boost::multiprecision::int128_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<boost::multiprecision::int128_t> = true;
        template<> constexpr bool ComplexQ<boost::multiprecision::int128_t> = false;
    }
    
    double ToDouble( cref<boost::multiprecision::int128_t> number )
    {
        static_cast<double>(number);
    }
    
    // String generator.
    std::string ToString( cref<boost::multiprecision::int128_t> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }

    // String generator to make it work with OutString.
    template<> struct ToChars<boost::multiprecision::int128_t>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 40;
        
        ToCharResult operator()( char * begin, char * end, const boost::multiprecision::int128_t & x ) const
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
    
    
    template<> constexpr const char * TypeName<boost::multiprecision::int256_t>  = "Boost_I256";
    template<> constexpr const char * FullTypeName<boost::multiprecision::int256_t>  = "boost::multiprecision::int256_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<boost::multiprecision::int256_t> = true;
        template<> constexpr bool ComplexQ<boost::multiprecision::int256_t> = false;
    }
    
    double ToDouble( cref<boost::multiprecision::int256_t> number )
    {
        static_cast<double>(number);
    }
    
    // String generator.
    std::string ToString( cref<boost::multiprecision::int256_t> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }
    
    // String generator to make it work with OutString.
    template<> struct ToChars<boost::multiprecision::int256_t>
    {
        static constexpr bool implementedQ = true;
        
        static constexpr Size_T char_count = 79;
        
        ToCharResult operator()( char * begin, char * end, const boost::multiprecision::int256_t & x ) const
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

namespace Knoodle
{
    double ToDouble( cref<boost::multiprecision::int128_t> a )
    {
        return static_cast<double>(a);
    }
    
    double ToDouble( cref<boost::multiprecision::int256_t> a )
    {
        return static_cast<double>(a);
    }
}
