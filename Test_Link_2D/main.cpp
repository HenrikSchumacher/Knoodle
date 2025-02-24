#include <iostream>

#define PD_DEBUG
//#define PD_VERBOSE

#ifdef __APPLE__
    #include "../submodules/Tensors/Accelerate.hpp"
#else
    #include "../submodules/Tensors/OpenBLAS.hpp"
#endif

#include "../KnotTools.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

using Real = double;
using Int  = Int32;

int main(void)
{
    
    const Real coords [4][3] = { {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0} };
    
    const Int  edges [4][2] = { {0,1}, {1,2}, {2,3}, {3,0} };
    
    Link_2D<Real,Int> L ( &edges[0][0], 4 );
    
    L.ReadVertexCoordinates( &coords[0][0] );
    
    TOOLS_DUMP( L.AmbientDimension() );

    PlanarDiagram<Int> PD(L);
    return 0;
}
