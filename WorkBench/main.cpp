#define TOOLS_NO_RESTRICT

#include "../Knoodle.hpp"
//#include "../src/Prosector3.hpp"
#include "../src/Prosector3/WideInteger.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Int64;
using IReal = Int64;
//using Real = Int64;
//using Int  = Size_T;

//using Prosector_T    = Prosector<IReal,Int>;
//using Vector3_T      = Prosector_T::Vector3_T;
//using Intersection_T = Prosector_T::Intersection;
//using Flag_T         = Prosector_T::Flag_T;


//WInt128 long_mul( Int64 a, Int64 b )
//{
//    return long_mul( WInt64{a}, WInt64{b} );
//}

int main()
{
    WInt32 z { Int32{1134134} };
    WInt64 a { Int64{1212424234134134} };
    WInt64 b { Int64{9236845324122124} };
    
    valprint("a",a);
    valprint("b",b);
    valprint("a + b",a + b);
    
    auto c = Knoodle::long_mul(a,b);
    valprint("c",c);
    
    TOOLS_DUMP(TypeName<WInt64>);
    TOOLS_DUMP(WInt64::ClassName());
    
    TOOLS_DUMP(TypeName<WInt128>);
    TOOLS_DUMP(WInt128::ClassName());
    
    
//    WInt128 c = long_mul(a,b);
    
    print("");
    
////    using Int  = Knoodle::UInteger<2,UInt32,UInt64>;
////    using LInt = Knoodle::UInteger<4,UInt32,UInt64>;
//    
//    using Int  = Knoodle::WideInteger<2,UInt8,UInt16,false>;
//    using LInt = Int::Prod_T;
//    
//    using EmulatedInt  = UInt16;
//    using LEmulatedInt = UInt32;
//    
//    Size_T s [4] {0,8,16,24};
//    
////    using Int  = Knoodle::UInteger<2,UInt16,UInt32>;
////    using LInt = Knoodle::UInteger<4,UInt16,UInt32>;
////    Size_T s [4] {0,16,32,48};
//    
////        using Int  = Knoodle::UInteger<2,UInt32,UInt64>;
////        using LInt = Knoodle::UInteger<4,UInt32,UInt64>;
////        Size_T s [4] {0,32,64,96};
//    
//    TOOLS_DUMP(sizeof(Int::Limb_T));
//    TOOLS_DUMP(Int::limb_bit_count);
//    TOOLS_DUMP(Int::lo_mask);
//    TOOLS_DUMP(Int::hi_mask);
//    
//    Int  a{Int::Limb_T(255),Int::Limb_T(2)};
//    valprint("a",a);
//    EmulatedInt A = EmulatedInt(EmulatedInt(a[1]) << s[1])
//                  + EmulatedInt(EmulatedInt(a[0]) << s[0]);
//    TOOLS_DUMP(A);
//    
//    Int  b{Int::Limb_T(255),Int::Limb_T(256)};
//    valprint("b",b);
//    EmulatedInt B = EmulatedInt(EmulatedInt(b[1]) << s[1])
//                  + EmulatedInt(EmulatedInt(b[0]) << s[0]);
//    TOOLS_DUMP(B);
//    
//    LInt c = long_mul(a,b);
//    valprint("c",c);
//    LEmulatedInt C = LEmulatedInt(LEmulatedInt(c[3]) << s[3])
//                   + LEmulatedInt(LEmulatedInt(c[2]) << s[2])
//                   + LEmulatedInt(LEmulatedInt(c[1]) << s[1])
//                   + LEmulatedInt(LEmulatedInt(c[0]) << s[0]);
//    TOOLS_DUMP(C);
//    TOOLS_DUMP(static_cast<LEmulatedInt>(A * B));
//    
//    print("\n");
//    Int d = a + b;
//    TOOLS_DUMP(d);
//    EmulatedInt D = (EmulatedInt(EmulatedInt(d[1]) << s[1]) + EmulatedInt(EmulatedInt(d[0]) << s[0]));
//    TOOLS_DUMP(D);
//    TOOLS_DUMP(static_cast<EmulatedInt>(A + B));
//    
//    
//    Int e = a - b;
//    TOOLS_DUMP(e);
//    EmulatedInt E = EmulatedInt(EmulatedInt(e[1]) << s[1])
//                  + EmulatedInt(EmulatedInt(e[0]) << s[0]);
//    TOOLS_DUMP(E);
//    TOOLS_DUMP(static_cast<EmulatedInt>(A - B));
    
    
    
    
//    Prosector_T S;
//
//    print(S.ClassName());
//    TOOLS_DUMP(TypeName<Prosector_T::Int>);
//    TOOLS_DUMP(TypeName<Prosector_T::LInt>);
//    TOOLS_DUMP(TypeName<Prosector_T::LLInt>);
//
//    TOOLS_DUMP(Scalar::RealQ<Prosector_T::Int>);
//    TOOLS_DUMP(Scalar::RealQ<Prosector_T::LInt>);
//    TOOLS_DUMP(Scalar::RealQ<Prosector_T::LLInt>);
//    TOOLS_DUMP(sizeof(Prosector_T::Int));
//    TOOLS_DUMP(sizeof(Prosector_T::LInt));
//    TOOLS_DUMP(sizeof(Prosector_T::LLInt));
//    TOOLS_DUMP(sizeof(Prosector_T::IntersectionTime));
//    TOOLS_DUMP(sizeof(Prosector_T::IntersectionTime)/sizeof(double));
//    TOOLS_DUMP(sizeof(Prosector_T::Intersection)/sizeof(Knoodle::Intersection<>));
//    TOOLS_DUMP(sizeof(_BitInt(128)));
//    Int i = 0;
//    Int j = 1;
//    Int k = 2;
//    
//    Vector3_T x_0 {0, 1, 1};
//    Vector3_T x_1 {12, 10, 4};
//    Vector3_T y_0 {0, -4, -1};
//    Vector3_T y_1 {8, 12, 1};
//    Vector3_T z_0 {0, 12, 3};
//    Vector3_T z_1 {8, -4, 5};
//    
//    TOOLS_DUMP(x_0);
//    TOOLS_DUMP(x_1);
//    TOOLS_DUMP(y_0);
//    TOOLS_DUMP(y_1);
//    TOOLS_DUMP(z_0);
//    TOOLS_DUMP(z_1);
//    
//    print("");
//    
//    Vector3_T u = x_1 - x_0;
//    Vector3_T v = y_1 - y_0;
//    Vector3_T p = y_1 - x_0;
//    Vector3_T q = x_1 - y_0;
//    
//    TOOLS_DUMP(u);
//    TOOLS_DUMP(v);
//    TOOLS_DUMP(p);
//    TOOLS_DUMP(q);
//    
//    print("\nIntersecting lines x and y.");
//    S.LoadLineSements(i, x_0, x_1, j, y_0, y_1);
//    
//    TOOLS_DUMP(S.IntersectionType());
//    TOOLS_DUMP(S.Flag());
//    Intersection_T xy_inter = S.ComputeIntersection();
//    
//    TOOLS_DUMP(xy_inter.flag);
//    TOOLS_DUMP(xy_inter.edges[0]);
//    TOOLS_DUMP(xy_inter.edges[1]);
//    TOOLS_DUMP(xy_inter.times[0]);
//    TOOLS_DUMP(xy_inter.times[1]);
//    TOOLS_DUMP(xy_inter.times[0].ToDouble());
//    TOOLS_DUMP(xy_inter.times[1].ToDouble());
//    TOOLS_DUMP(xy_inter.handedness);
//    
//    print("\nIntersecting lines x and z.");
//    S.LoadLineSements(i, x_0, x_1, k, z_0, z_1);
//    TOOLS_DUMP(S.IntersectionType());
//    TOOLS_DUMP(S.Flag());
//    Intersection_T xz_inter = S.ComputeIntersection();
//    
//    TOOLS_DUMP(xz_inter.flag);
//    TOOLS_DUMP(xz_inter.edges[0]);
//    TOOLS_DUMP(xz_inter.edges[1]);
//    TOOLS_DUMP(xz_inter.times[0]);
//    TOOLS_DUMP(xz_inter.times[1]);
//    TOOLS_DUMP(xz_inter.times[0].ToDouble());
//    TOOLS_DUMP(xz_inter.times[1].ToDouble());
//    TOOLS_DUMP(xz_inter.handedness);
//    
//    print("");
//    
//    TOOLS_DUMP(xy_inter.times[0] < xz_inter.times[0]);
//    TOOLS_DUMP(xy_inter.times[0] > xz_inter.times[0]);
}
