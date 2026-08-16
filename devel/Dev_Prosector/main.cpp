#define TOOLS_NO_RESTRICT

#include "../../Knoodle.hpp"
#include "../../src/LinkEmbedding3.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using IReal = Int64;

//using Real  = Int32;
//using IReal = Int32;



using Int  = Int64;

//using Link_T         = LinkEmbedding3<Real,Int,IReal>;
//using Prosector_T    = Link_T::Prosector_T;
//using Vector3_T      = Prosector_T::Vector3_T;
//using Intersection_T = Prosector_T::Intersection;
//using Flag_T         = Prosector_T::Flag_T;




template<typename A_T, typename B_T>
void fun( A_T a, B_T b )
{
    auto c = long_mul(a,b);
    print(std::string(TypeName<decltype(a)>) + " * " + TypeName<decltype(b)> + " -> " + TypeName<decltype(c)>);
    print("a = " + ToString(a) + ", b = " + ToString(b) + ", c = " + ToString(c));
}

int main()
{
    Profiler::Clear();
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

    TOOLS_DUMP(TypeName<DefaultLimb_T>);
    TOOLS_DUMP(TypeName<DefaultComp_T>);
    
    fun(  Int16{3},  Int16{4} );
    
    fun(  Int32{3},  Int32{4} );
    fun(  Int32{3},  Int64{4} );
    fun(  Int32{3}, Int128{4} );
    
    fun(  Int64{3},  Int64{4} );
    fun( Int128{3},  Int64{4} );
    fun(  Int64{3}, Int128{4} );
    fun( Int128{3}, Int128{4} );
    
    fun(  WInt64{3},  WInt64{4} );
    fun( WInt128{3},  WInt64{4} );
    fun(  WInt64{3}, WInt128{4} );
    fun( WInt128{3}, WInt128{4} );
    
    auto a = WideInt<2,UInt64,UInt128, false>( Int64(1) );
    TOOLS_DUMP(a);
    auto b = WideInt<2,UInt64,UInt128, false>( Int128(1) );
    TOOLS_DUMP(b);
    
//    {
//        Int64  b = 1;
//        Int128 a = 2;
//        auto c = long_mul(a,b);
//        print(TypeName<decltype(a)>);
//        TOOLS_DUMP(a);
//        print(TypeName<decltype(b)>);
//        TOOLS_DUMP(b);
//        print(TypeName<decltype(c)>);
//        TOOLS_DUMP(c);
//    }
    
//    {
//        Int128 b = 1;
//        Int128 a = 2;
//        auto c = long_mul(a,b);
//        print(TypeName<decltype(a)>);
//        TOOLS_DUMP(a);
//        print(TypeName<decltype(b)>);
//        TOOLS_DUMP(b);
//        print(TypeName<decltype(c)>);
//        TOOLS_DUMP(c);
//        
//    }
//    
//    TOOLS_DUMP(sizeof(Int64));
//    TOOLS_DUMP(sizeof(Int128));
//    
//    TOOLS_DUMP(sizeof(WInt64));
////    TOOLS_DUMP(sizeof(WInt96));
//    TOOLS_DUMP(sizeof(WInt128));
//    TOOLS_DUMP(sizeof(WInt192));
//    TOOLS_DUMP(sizeof(WInt256));
//    
//    Profiler::Clear();
//    Link_T L = Link_T::FromFile("/Users/Henrik/a.txt");
//    TOOLS_DUMP(L.EdgeCount());
//    auto err = L.RequireIntersections();
//    TOOLS_DUMP(err);
}
