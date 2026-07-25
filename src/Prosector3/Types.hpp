#pragma once

#include "WideInt.hpp"

namespace Knoodle
{
    using WInt32  = WideInt<1,UInt32,UInt64,true>;
    using WInt64  = WideInt<2,UInt32,UInt64,true>;
    using WInt128 = Knoodle::WideInt<4,UInt32,UInt64,true>;
    using WInt256 = Knoodle::WideInt<8,UInt32,UInt64,true>;
    
    using WUInt32  = WideInt<1,UInt32,UInt64,false>;
    using WUInt64  = WideInt<2,UInt32,UInt64,false>;
    using WUInt128 = Knoodle::WideInt<4,UInt32,UInt64,false>;
    using WUInt256 = Knoodle::WideInt<8,UInt32,UInt64,false>;

    using Int128   = WInt128;
    using Int256   = WInt256;
    using UInt128  = WUInt128;
    using UInt256  = WUInt256;
}

namespace Tools
{
    template<> constexpr const char * TypeName<Knoodle::WInt64>  = "WInt64";
    template<> constexpr const char * FullTypeName<Knoodle::WInt64>  = "WideInt64";
    
    template<> constexpr const char * TypeName<Knoodle::Int128>  = "WideInt128";
    template<> constexpr const char * FullTypeName<Knoodle::Int128>  = "WideInt128";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<Knoodle::Int128> = true;
        template<> constexpr bool ComplexQ<Knoodle::Int128> = false;
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
    
    
    template<> constexpr const char * TypeName<Knoodle::Int256>  = "WideInt256";
    template<> constexpr const char * FullTypeName<Knoodle::Int256>  = "WideInt256";
    
    namespace Scalar
    {
        template<> constexpr bool RealQ<Knoodle::Int256> = true;
        template<> constexpr bool ComplexQ<Knoodle::Int256> = false;
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
