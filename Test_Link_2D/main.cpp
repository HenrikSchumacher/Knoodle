//
//  main.cpp
//  Test_Link_2D
//
//  Created by Henrik on 12.02.24.
//

#include <iostream>

#define LAPACK_DISABLE_NAN_CHECK
#define ACCELERATE_NEW_LAPACK
#include <Accelerate/Accelerate.h>

#define PD_ASSERTS

#include "KnotTools.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

using Real = double;
using Int  = long long;

int main(void)
{
    
    const Real coords [4][3] = { {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0} };
    
    const Int  edges [4][2] = { {0,1}, {1,2}, {2,3}, {3,0} };
    
    Link_2D<Real,Int> L ( &edges[0][0], 4 );
    
    L.ReadVertexCoordinates( &coords[0][0] );
    
    dump( L.AmbientDimension() );
    

    return 0;
}
