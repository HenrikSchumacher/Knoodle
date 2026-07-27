//#define TOOLS_NO_RESTRICT

#define TOOLS_AGGRESSIVE_INLINING

#define WIDE_INTEGER_HAS_LIMB_TYPE_UINT64
#include "../experimental/wide-integer/math/wide_integer/uintwide_t.h"

#include "../Knoodle.hpp"
//#include "../src/Prosector3.hpp"
#include "../src/WideInt.hpp"
#include "../src/WideInt/Tests.hpp"


namespace Tools
{
    using Int128  = signed   __int128;
    using UInt128 = unsigned __int128;
    template<> constexpr const char * TypeName<Int128>   = "Int128";
    template<> constexpr const char * TypeName<UInt128>  = "UInt128";
}

using namespace Knoodle;
using namespace Tools;


int main()
{
//    {
//        Size_T n    = 1024;
//        Size_T reps = 1024 * 1024;
//        
////        Size_T n    = 10;
////        Size_T reps = 1;
//        
//        const Size_T limb_count = 2;
//        using Limb_T = UInt64;
//        using Comp_T = UInt128;
//        constexpr bool signQ = true;
//        
//        bool verbosed = false;
//        
////        Test_NegativeQ    <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_Sign         <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_Negate       <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_less_small   <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_greater_small<limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_less         <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_greater      <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_Add          <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
////        Test_Subtract     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_long_mul     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_long_fma     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//        Test_long_det     <limb_count,Limb_T,Comp_T,signQ>( n, reps, verbosed );
//
//    }
 
//    ToSigned<WInt64::Limb_T> u [3] { 14331995546441088, 60079646119598720, 37112354874561664 };
//    ToSigned<WInt64::Limb_T> v [3] { -64009635545751168, 29543087488075264, -14908702073180672 };
    
//    ToSigned<WInt64::Limb_T> u [3] { 143, 600, 371 };
//    ToSigned<WInt64::Limb_T> v [3] { 640, 295, 149 };
//    
//    TOOLS_DUMP(WInt64(u[0]));
//    TOOLS_DUMP(WInt64(u[1]));
//    TOOLS_DUMP(WInt64(u[2]));
//    
//    TOOLS_DUMP(WInt64(v[0]));
//    TOOLS_DUMP(WInt64(v[1]));
//    TOOLS_DUMP(WInt64(v[2]));
//    
//    WInt128 uxv_0 = long_det(u[1],u[2],v[1],v[2]);
//    WInt128 uxv_1 = long_det(u[2],u[0],v[2],v[0]);
//    WInt128 uxv_2 = long_det(u[0],u[1],v[0],v[1]);
//    
//    TOOLS_DUMP(uxv_0);
//    TOOLS_DUMP(uxv_1);
//    TOOLS_DUMP(uxv_2);
//    
//    double d_uxv_0 = double(u[1]) * double(v[2]) - double(u[2]) * double(v[1]);
//    double d_uxv_1 = double(u[2]) * double(v[0]) - double(u[0]) * double(v[2]);
//    double d_uxv_2 = double(u[0]) * double(v[1]) - double(u[1]) * double(v[0]);
//    
//    TOOLS_DUMP(d_uxv_0);
//    TOOLS_DUMP(d_uxv_1);
//    TOOLS_DUMP(d_uxv_2);
    
    ToSigned<WInt64::Limb_T> a_raw = -600;
    ToSigned<WInt64::Limb_T> b_raw = 149;
    
    WInt64 a = WInt64(a_raw);
    WInt64 b = WInt64(b_raw);
    TOOLS_DUMP(a[0]);
    TOOLS_DUMP(a);
    TOOLS_DUMP(-a);
    
    TOOLS_DUMP(b[0]);
    TOOLS_DUMP(b);
    TOOLS_DUMP(-b);
    
    wint128 a_wint (a_raw);
    wint128 b_wint (b_raw);
    
    WInt128 ab = long_mul(a,b);
    wint128 ab_wint = a_wint * b_wint;
    
//    double a = ToDouble(long_mul(WInt64(u[1]),WInt64(v[2])));
//    double b = double(u[1]) * double(v[2]);
    TOOLS_DUMP(ab[0]);
    TOOLS_DUMP(ab[1]);
    TOOLS_DUMP(ToDouble(ab));
    TOOLS_DUMP(ToString(ab_wint));
    TOOLS_DUMP(static_cast<double>(ab_wint));
}
