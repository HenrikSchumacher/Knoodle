//#define TOOLS_ENABLE_PROFILER
//#define TOOLS_DEBUG

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
using BReal = Real64;   // scalar type used for bounding boxes
using Int   = Int64;    // integer type used, e.g., for indices
using LInt  = Int64;    // integer type for counting objects

using PolyFold_T = PolyFold<Real,Int,LInt,BReal>;

int main( int argc, char** argv )
{
    try
    {
        PolyFold_T polyfold ( argc, argv );
    }
    catch(...)
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
