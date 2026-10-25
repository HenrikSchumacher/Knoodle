//#define TOOLS_NO_RESTRICT

//#define TOOLS_ENABLE_PROFILER
#define TOOLS_USE_BOOST_UNORDERED
#define TOOLS_AGGRESSIVE_INLINING


#include "../../Knoodle.hpp"
#include "../../src/KnotInvariants/LaurentPolynomial.hpp"
#include "../../experimental/KauffmanBracket.hpp"
#include "../../experimental/JonesPolynomial.hpp"

using namespace Knoodle;
using namespace Tools;

using Int         = Int64; // integer type used, e.g., for indices
using PDC_T       = PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;
using LP_T        = LaurentPolynomial<Int,Int>;
//using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;


using Int = Int64;
using LP  = LaurentPolynomial<Int,Int>;

int main()
{
//    LP p;
//    LP q;
//    
//    p.Set(-1, 1).Set( 1, 1);
//    
//    q.Set( 0, 2).Set( 2, 3);
//    
//    TOOLS_DUMP(p);
//    TOOLS_DUMP(q);
//    
//    LP r = p * q;
//    
//    TOOLS_DUMP(r);

    
//    Int pd_code [5][5] = {
//        {9, 5, 0, 4, 1},
//        {5, 1, 6, 0, 1},
//        {1, 7, 2, 6, 1},
//        {7, 3, 8, 2, 1},
//        {3, 9, 4, 8, 1}
//    };
    
//    Int pd_code [4][5] = {
//        {7, 5, 0, 4,  1},
//        {3, 1, 4, 0,  1},
//        {1, 6, 2, 7, -1},
//        {5, 2, 6, 3, -1}
//    };
//    PD_T pd = PD_T::FromSignedPDCode( &pd_code[0][0], 4 );
    
    
//    Int pd_code [11][5] = {
//        {21, 17, 0, 16, 1},
//        {5, 1, 6, 0, 1},
//        {1, 7, 2, 6, 1},
//        {7, 3, 8, 2, 1},
//        {3, 12, 4, 13, -1},
//        {11, 4, 12, 5, -1},
//        {15, 9, 16, 8, 1},
//        {9, 20, 10, 21, -1},
//        {17, 10, 18, 11, -1},
//        {13, 18, 14, 19, -1},
//        {19, 14, 20, 15, -1}
//    };
//    PD_T pd = PD_T::FromSignedPDCode( &pd_code[0][0], 11 );
    
    Int pd_code [13][5] = {{25, 19, 0, 18, 1}, {23, 1, 24, 0, 1}, {1, 14, 2, 15, -1}, {7, 3, 8, 2, 1}, {3, 9, 4, 8, 1}, {9, 5, 10, 4, 1}, {5, 21, 6, 20, 1}, {21, 7, 22, 6, 1}, {15, 10, 16, 11, -1}, {11, 16, 12, 17, -1}, {19, 12, 20, 13, -1}, {13, 22, 14, 23, -1}, {17, 25, 18, 24, 1}};
    PD_T pd = PD_T::FromSignedPDCode( &pd_code[0][0], 13 );
    
//    Int pd_code [3][5] = {{2, 0, 3, 5, 1}, {0, 4, 1, 3, 1}, {4, 2, 5, 1, 1}};
//    PD_T pd = PD_T::FromSignedPDCode( &pd_code[0][0], 3 );

    {
        auto B = KauffmanBracket(false);
        tic("KauffmanBracket");
        auto bracket = B(pd);
        toc("KauffmanBracket");
        TOOLS_DUMP(bracket);
    }
    
    
    {
        auto B = KauffmanBracket(true);
        tic("KauffmanBracket compressed");
        auto bracket = B(pd);
        toc("KauffmanBracket compressed");
        TOOLS_DUMP(bracket);
        
        TOOLS_DUMP(B.Jones4(pd));
        TOOLS_DUMP(B.Jones2(pd));
    }
    

//    {
//        auto J = JonesPolynomial();
//        tic("JonesPolynomial");
//        auto jones = J(pd);
//        toc("JonesPolynomial");
//        TOOLS_DUMP(jones);
//    }
    
    return 0;
}
