//#define TOOLS_NO_RESTRICT

#define TOOLS_AGGRESSIVE_INLINING
//#define TOOLS_NO_INT128

#define WIDE_INTEGER_HAS_LIMB_TYPE_UINT64
#include "../experimental/wide-integer/math/wide_integer/uintwide_t.h"

#include "../Knoodle.hpp"
//#include "../src/Prosector3.hpp"
#include "../src/WideInt.hpp"
#include "../src/WideInt/Boost_cpp_int.hpp"
#include "../src/WideInt/Tests.hpp"

using namespace Knoodle;
using namespace Tools;

struct A
{
    int member = 0;
    
    constexpr A() {}
    
    constexpr A( int a )
    :   member { a }
    {}
    
    constexpr friend A operator+( const A & a, const A & b )
    {
        if( !std::is_constant_evaluated() )
        {
            eprint("!!!");
        }
        return A(a.member + b.member);
    }
};


int main()
{
    {
//        Size_T n    = 1024;
//        Size_T reps = 1024 * 1024;
        
        Size_T n    = 1;
        Size_T reps = 1;
        
        const Size_T limb_count = 2;
        using Limb_T = UInt64;
        using Comp_T = UInt128;
        constexpr bool signQ = false;
        
        bool verbosed = true;
        
//        Test_NegativeQ    <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_Sign         <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_Negate       <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_less_small   <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_greater_small<limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_less         <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_greater      <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_Add          <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_Subtract     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_long_mul     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
        Test_long_fma     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
        Test_long_det     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );

    }
    
//    [[maybe_unused]] PRNG_T engine = InitializedRandomEngine<PRNG_T>();
//    
//    std::uniform_int_distribution<WInt64::Limb_T> dist (
//        std::numeric_limits<WInt64::Limb_T>::lowest(),
//        std::numeric_limits<WInt64::Limb_T>::max()
//    );
//
//    ToSigned<WInt64::Limb_T> a_raw = static_cast<ToSigned<WInt64::Limb_T>>(dist(engine));
//    ToSigned<WInt64::Limb_T> b_raw = static_cast<ToSigned<WInt64::Limb_T>>(dist(engine));
//    
////    ToSigned<WInt64::Limb_T> a_raw = 600;
////    ToSigned<WInt64::Limb_T> b_raw = -120;
//    
//    WInt64 a = WInt64(a_raw);
//    WInt64 b = WInt64(b_raw);
//    TOOLS_DUMP(a_raw);
//    TOOLS_DUMP(a[0]);
//    TOOLS_DUMP(a);
////    TOOLS_DUMP(-a);
//    
//    TOOLS_DUMP(b_raw);
//    TOOLS_DUMP(b[0]);
//    TOOLS_DUMP(b);
////    TOOLS_DUMP(-b);
//    
//    wint128 a_wint (a_raw);
//    wint128 b_wint (b_raw);
//    
//    WInt128 ab = long_mul(a,b);
//    wint128 ab_wint = a_wint * b_wint;
//    wint128 r_wint;
//    wide_convert(ab, r_wint);
//    
////    double a = ToDouble(long_mul(WInt64(u[1]),WInt64(v[2])));
////    double b = double(u[1]) * double(v[2]);
//    TOOLS_DUMP(ab[0]);
//    TOOLS_DUMP(ab[1]);
//    TOOLS_DUMP(ToDouble(ab));
//    TOOLS_DUMP(ToString(ab_wint));
//    TOOLS_DUMP(ToString(r_wint));
//    TOOLS_DUMP(static_cast<double>(ab_wint));
//    TOOLS_DUMP(static_cast<double>(r_wint));
//    
//    TOOLS_DUMP(ab_wint == r_wint);
    
    
    
    
//    TOOLS_DUMP( newTypeName<Int64> );
//    TOOLS_DUMP( newTypeName<Int128> );
//    
//    auto & s0 = newTypeName<WideInt<2,UInt32,UInt128,true>>;
//    auto & s1 = newTypeName<WideInt<2,UInt64,UInt128,true>>;
//    auto & s2 = newTypeName<Knoodle::WUInt256>;
//
//    TOOLS_DUMP(s0);
//    TOOLS_DUMP(s1);
//    TOOLS_DUMP(s2);
//    
//    constexpr std::string s  = ToString(124);
//    
//    TOOLS_DUMP(s);

    boost::multiprecision::int128_t z = 1;
}
