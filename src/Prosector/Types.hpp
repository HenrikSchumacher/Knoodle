#pragma once

#include <boost/multiprecision/cpp_int.hpp>


namespace Knoodle
{
    using Int128  = boost::multiprecision::int128_t;
    using Int256  = boost::multiprecision::int256_t;
    
//    using mpz_int =  boost::multiprecision::mpz_int;
}

namespace Tools
{
    
    template<> constexpr const char * TypeName<Knoodle::Int128>  = "I128";
    template<> constexpr const char * FullTypeName<Knoodle::Int128>  = "boost::multiprecision::int128_t";
    
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
