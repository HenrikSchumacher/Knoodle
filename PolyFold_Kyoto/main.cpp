#include <iostream>
#include <iterator>

#include "../KnotTools.hpp"
#include "../src/PolyFold.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

using Real  = Real64;
using BReal = Real;
//using BReal = Real32;
//using Int   = Int64;
using Int   = Int32;
using LInt  = Int64;

int main( int argc, char** argv )
{
    const Time prog_start_time = Clock::now();

    PolyFold<Real,Int,LInt,BReal> polyfold ( argc, argv );
    
    const Time prog_stop_time = Clock::now();
    
    print("Done. Time elapsed = " + ToStringFPGeneral(
        Tools::Duration(prog_start_time,prog_stop_time)) + " s."
    );
    
    return 0;
}
