#pragma once

namespace Tools
{
    using wint64   = ::math::wide_integer::uintwide_t< 64,UInt64,void,1>;
    using wint128  = ::math::wide_integer::uintwide_t<128,UInt64,void,1>;
    using wint256  = ::math::wide_integer::uintwide_t<256,UInt64,void,1>;
    using wint512  = ::math::wide_integer::uintwide_t<512,UInt64,void,1>;
    
    using wuint64   = ::math::wide_integer::uintwide_t< 64,UInt64,void,0>;
    using wuint128  = ::math::wide_integer::uintwide_t<128,UInt64,void,0>;
    using wuint256  = ::math::wide_integer::uintwide_t<256,UInt64,void,0>;
    using wuint512  = ::math::wide_integer::uintwide_t<512,UInt64,void,0>;
    
    template<> constexpr const char * TypeName<wint64>   = "wint64";
    template<> constexpr const char * TypeName<wint128>  = "wint128";
    template<> constexpr const char * TypeName<wint256>  = "wint256";
    template<> constexpr const char * TypeName<wint512>  = "wint512";
    
    template<> constexpr const char * TypeName<wuint64>   = "wuint64";
    template<> constexpr const char * TypeName<wuint128>  = "wuint128";
    template<> constexpr const char * TypeName<wuint256>  = "wuint256";
    template<> constexpr const char * TypeName<wuint512>  = "wuint512";
    
} // namespace Tools


namespace Knoodle
{
    
    template<unsigned int bit_count, UnsignedIntQ Limb_T, bool signQ>
    using  cint = ::math::wide_integer::uintwide_t<bit_count,Limb_T,void,signQ>;
    
    template<int limb_count, UnsignedIntQ Limb_T, bool signQ>
    using CheckInt = cint<limb_count * sizeof(Limb_T) * CHAR_BIT,Limb_T,signQ>;
    
    
    template<int limb_count, unsigned int bit_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    void wide_convert(
        cref<WideInt<limb_count,Limb_T,Comp_T,signQ>> x,
        mref<cint<bit_count,Limb_T,signQ>> y
    )
    {
        constexpr int m = limb_count;
        constexpr int n = bit_count / (sizeof(Limb_T) * CHAR_BIT);
        
        static_assert(m <= n, "");
        using WInt = WideInt<m,Limb_T,Comp_T,signQ>;
        
        typename WInt::Limb_T * y_ptr = &y.representation()[0];
        for( UInt32 i = 0; i < m; ++i )
        {
            y_ptr[i] = x[i];
        }
        for( UInt32 i = m; i < n; ++i )
        {
            y_ptr[i] = Limb_T(0);
        }
    }
    
    template<unsigned int bit_count, int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    void wide_convert(
        cref<cint<bit_count,Limb_T,signQ>> x,
        mref<WideInt<limb_count,Limb_T,Comp_T,signQ>> y
    )
    {
        constexpr int m = bit_count / (sizeof(Limb_T) * CHAR_BIT);
        constexpr int n = limb_count;
        
        static_assert(m <= n, "");
        
//        using CInt = cint<n * (sizeof(Limb_T) * CHAR_BIT),Limb_T,signQ>;
        using WInt = WideInt<m,Limb_T,Comp_T,signQ>;
        
        typename WInt::Limb_T * y_ptr = &y[0];
        for( UInt32 i = 0; i < m; ++i )
        {
            y_ptr[i] = x[i];
        }
        for( UInt32 i = m; i < n; ++i )
        {
            y_ptr[i] = Limb_T(0);
        }
    }
    
    template<unsigned int bit_count_x, unsigned int bit_count_y, UnsignedIntQ Limb_T, bool signQ>
    void wide_convert(
        cref<cint<bit_count_x,Limb_T,signQ>> x,
        mref<cint<bit_count_y,Limb_T,signQ>> y
    )
    {
        static_assert(bit_count_x <= bit_count_y, "");
        y = x;
    }
    
    template<int m, int n, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    void wide_convert(
        cref<WideInt<m,Limb_T,Comp_T,signQ>> x,
        mref<WideInt<n,Limb_T,Comp_T,signQ>> y
    )
    {
        static_assert(m <= n, "");
        y = WideInt<n,Limb_T,Comp_T,signQ>(x);
    }
    
    template<unsigned int bit_count, UnsignedIntQ Limb_T, bool signQ>
    std::string ToString( cref<::math::wide_integer::uintwide_t<bit_count,Limb_T,void,signQ>> x )
    {
        auto x_rep = x.representation();
        return OutString::FromVector(&x_rep[0],x_rep.size());
    }
    
    template<unsigned int bit_count, UnsignedIntQ Limb_T, bool signQ, typename RandomFunction_T>
    void Randomize(
        cref<::math::wide_integer::uintwide_t<bit_count,Limb_T,void,signQ>> x,
        mref<RandomFunction_T> rand
    )
    {
        auto x_rep = x.representation();
        for( unsigned int i = 0; i < x_rep.size(); ++i )
        {
            x_rep[i] = dist(rand);
        }
    }
    
    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ, typename RandomFunction_T>
    void Randomize( mref<WideInt<limb_count,Limb_T,Comp_T,signQ>> x, mref<RandomFunction_T> rand )
    {
        x.Randomize(rand);
    }
    
} // namespace Knoodle
