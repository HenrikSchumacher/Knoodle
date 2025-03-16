#include <iostream>
#include <iterator>

#include "../Knoodle.hpp"
#include "../src/PolyFold.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

using Real  = Real64;   // scalar type used for positions of polygon
using BReal = Real32;   // scalar type used for bounding boxes
using Int   = Int64;    // integer type used, e.g., for indices in PlanarDiagram etc.
using LInt  = Int64;    // integer type for counting objects

int main( int argc, char** argv )
{
    PolyFold<Real,Int,LInt,BReal> polyfold ( argc, argv );

    return 0;
}
