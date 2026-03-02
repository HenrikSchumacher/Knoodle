#define TOOLS_AGGRESSIVE_INLINING

#include "../Knoodle.hpp"

using namespace Knoodle;
using namespace Tools;

using Int   = Size_T;
using UInt  = UInt64;
using Real  = Real64;
//using BReal = Real64;
using BReal = Real32;

using T = Real;

template<Int point_count, Int AmbDim>
void PrimitiveToBox2(
    cptr<Real> P, mptr<BReal> B
)
{
    Tiny::Vector<3,Real,Int> lo { Scalar::Max<Real> };
    Tiny::Vector<3,Real,Int> hi { Scalar::Min<Real> };
    
    for( Int i = 0; i < point_count; ++i )
    {
        lo.ElementwiseMin(&P[AmbDim * i]);
        hi.ElementwiseMax(&P[AmbDim * i]);
    }
    
//     If we have to round, make sure that the boxes become a little bit bigger, so that it still contains all primitives.
    if constexpr ( Scalar::Prec<BReal> != Scalar::Prec<Real> )
    {
        #pragma STDC FENV_ACCESS ON
        std::fesetround(FE_UPWARD);
        
        for( Int k = 0; k < AmbDim; ++k )
        {
            B[         k] = -static_cast<BReal>(-lo[k]);
            B[AmbDim + k] =  static_cast<BReal>( hi[k]);
        }
    }
    else
    {
        for( Int k = 0; k < AmbDim; ++k )
        {
            B[         k] = static_cast<BReal>(lo[k]);
            B[AmbDim + k] = static_cast<BReal>(hi[k]);
        }
    }
//    
//    for( Int k = 0; k < AmbDim; ++k )
//    {
//        if( static_cast<Real>(B[k]) > lo[k] ) { eprint("B[k] > lo[k]"); }
//        if( static_cast<Real>(B[AmbDim + k]) < hi[k] ) { eprint("B[AmbDim + k] < hi[k]"); }
//    }
}

using Rand_T = PRNG_T;
using Tree_T = AABBTree<3,Real,Int,BReal>;

int main()
{
    Int n = 12'000'000;
 
    {
        double dnumber = 1.23456786012345679;
        
        {
            float  fnumber = static_cast<float>(dnumber);
            double dnumber2 = static_cast<double>(fnumber);
            
            std::cout << ToString(dnumber) << std::endl;
            std::cout << ToString(fnumber) << std::endl;
            std::cout << ToString(dnumber2) << std::endl;
            //    TOOLS_DUMP(float(0.123456789012345679));
        }
        
        {
            #pragma STDC FENV_ACCESS ON
            
//            RoundingModeBarrier barrier ( RoundingMode_T::Downward );
            std::fesetround(FE_DOWNWARD);
            
            float  fnumber = static_cast<float>(dnumber);
            double dnumber2 = static_cast<double>(fnumber);
            
//            float  fnumber = static_cast<float>(dnumber);
            
            std::cout << ToString(dnumber) << std::endl;
            std::cout << ToString(fnumber) << std::endl;
            std::cout << ToString(dnumber2) << std::endl;
        }
        
    }
    
//    TOOLS_DUMP(float(0.123456789012345679));
    
    PRNG_T r = InitializedRandomEngine<PRNG_T>();
    
    std::uniform_real_distribution<Real> dist (1,2);
    
    constexpr Int point_count = 4;
    constexpr Int AmbDim = 3;
    
    Tensor3<Real,Int> P ( n, point_count, AmbDim );
    
    P.FillByFunction(
        [&r,&dist]( const Int i, const Int j, const Int k )
        {
            (void)i; (void)j; (void)k;
            return dist(r);
        }
    );

    valprint("P",ArrayToString(P.data(),{Int(6),point_count,AmbDim},[](Real x){ return ToStringFPGeneral(x); }));
    
    Tensor3<BReal,Int> B ( n, 2, AmbDim );
    
    tic("PrimitiveToBox");
    for( Int i = 0; i < n; ++i )
    {
        Tree_T::PrimitiveToBox<point_count,AmbDim>( P.data(i), B.data(i) );
    }
    toc("PrimitiveToBox");
    
//    valprint("B",ArrayToString(B.data(),{Int(2),Int(2),AmbDim},[](Real x){ return ToStringFPGeneral(x); }));
    
    valprint("B",ArrayToString(B.data(),{Int(2),Int(2),AmbDim}));
    
    
    Tensor3<BReal,Int> B2 ( n, 2, AmbDim );
    
    tic("PrimitiveToBox2");
    for( Int i = 0; i < n; ++i )
    {
        PrimitiveToBox2<point_count,AmbDim>( P.data(i), B2.data(i) );
    }
    toc("PrimitiveToBox2");
    
    valprint("B2",ArrayToString(B2.data(),{Int(2),Int(2),AmbDim}));
//s
    
    
//    Tree_T
    
//    columnwise_minmax<point_count,AmbDim>(
//        P, dimP, &B[0], Int(1), &B[AmbDim], Int(1)
//    );
    
    print(ToString(double(0.12345678901234567890)));
    print(ToString(float (0.12345678901234567890)));
    
 }

// elementwise_minmax
// columnwise_minmax
