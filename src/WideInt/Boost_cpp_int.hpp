#pragma once

// Boost: Rather slow, but maybe I missed something.
#include <boost/multiprecision/cpp_int.hpp>
namespace Tools
{
    template<> constexpr const char * TypeName<boost::multiprecision::int128_t>  = "Boost_I128";
    template<> constexpr const char * FullTypeName<boost::multiprecision::int128_t>  = "boost::multiprecision::int128_t";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<boost::multiprecision::int128_t> = true;
        template<> constexpr bool ComplexQ<boost::multiprecision::int128_t> = false;
    }
    
    double ToDouble( cref<boost::multiprecision::int128_t> a )
    {
        return static_cast<double>(a);
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
    
    double ToDouble( cref<boost::multiprecision::int256_t> a )
    {
        return static_cast<double>(a);
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
    
} // namespace Tools

namespace Knoodle
{
    

    
} // namespace Knoodle
