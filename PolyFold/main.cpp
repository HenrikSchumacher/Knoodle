#include <iostream>
#include <iterator>

//#define PD_DEBUG

//#define POLYFOLD_SIGNPOSTS
//#define POLYFOLD_NO_QUATERNIONS
//#define POLYFOLD_WITNESSES

#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
#include "../src/PolyFold.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;


using Real  = Real64;   // scalar type used for positions of polygon
//using Real  = Real32;   // Never use this unless for producing edge cases.
//using BReal = Real32;   // scalar type used for bounding boxes
using BReal = Real64;   // scalar type used for bounding boxes
using Int   = Int64;    // integer type used, e.g., for indices in PlanarDiagram etc.
//using Int   = Int32;    // integer type used, e.g., for indices in PlanarDiagram etc.
using LInt  = Int64;    // integer type for counting objects

using PolyFold_T = PolyFold<Real,Int,LInt,BReal>;

int main( int argc, char** argv )
{
    PolyFold_T polyfold ( argc, argv );
    
    return 0;
}
