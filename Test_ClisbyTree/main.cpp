#include <iostream>
#include <iterator>

//#include <boost/json.hpp>
//#include <boost/json/src.hpp>

// see https://www.jviotti.com/2022/02/21/emitting-signposts-to-instruments-on-macos-using-cpp.html
#include <os/signpost.h>

#include "../KnotTools.hpp"
#include "../src/PolyFold.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

//using Real  = Real32; // Never use this unless for producing edge cases.
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
    
    polyfold.Run();
    
    const Time prog_stop_time = Clock::now();
    
    print("Done. Time elapsed = " + ToStringFPGeneral(
        Tools::Duration(prog_start_time,prog_stop_time)) + " s."
    );
    
    return 0;
}
