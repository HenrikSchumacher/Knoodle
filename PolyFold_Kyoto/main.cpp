#include <iostream>
#include <iterator>

#include "../KnotTools.hpp"
#include "../src/PolyFold.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

//using Real  = Real32; // Never use this unless for producing edge cases.
using Real  = Real64;
//using BReal = Real;
using BReal = Real32;
//using Int   = Int64;
using Int   = Int32;
using LInt  = Int64;

int main( int argc, char** argv )
{
    PolyFold<Real,Int,LInt,BReal> polyfold ( argc, argv );

    return 0;
}
