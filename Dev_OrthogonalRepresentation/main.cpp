#define TOOLS_ENABLE_PROFILER

#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
//#include "../submodules/Tensors/Clp.hpp"
#include "../src/OrthogonalRepresentation.hpp"
//#include "ClpSimplex.hpp"
//#include "ClpSimplexDual.hpp"
//#include "CoinHelperFunctions.hpp"

//#include "../Reapr.hpp"

//#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

using Real  = double;          // scalar type used for positions of polygon
using BReal = double;          // scalar type used for bounding boxes
using Int   = Int64;           // integer type used, e.g., for indices
using LInt  = Int64;           // integer type used, e.g., for indices

int main( int argc, char** argv )
{
    (void)argc;
    (void)argv;
    
    Int pd_code [3][5] = {
        {5, 3, 0, 2,  1},
        {1, 1, 2, 0,  1},
        {4, 3, 5, 4, -1}
    };

    Int bends [6] = { 2, -3, 0, 0, 3, -2 };
    
    
//              v_6     e_8     v_5             e_14    v_11
//                 +<----------+           +<----------+
//                 |    a_0    ^           |    a_4    ^
//                 |           |           |           |
//              e_9|           |e_0    e_15|           |e_13
//                 |           |           |           |
//  v_9            v    a_2    |    a_3    v           |
//     +---------->|---------->----------->----------->+
//     ^    e_12   |c_1 e_2    ^c_0 e_3    |c_2 e_4     v_10
//     |           |           |           |
//     |e_11    e_1|           |e_7     e_5|
//     |           |           |           |
//     |    e_10   v           |    e_6    v
//     +<----------+           +<----------+
//  v_8     a_1     v_7     v_4     a_5     v_3

    
    PlanarDiagram<Int> pd = PlanarDiagram<Int>::FromSignedPDCode(
        &pd_code[0][0], 3, 0, false, false
    );
    
    Profiler::Clear();
    

    OrthogonalRepresentation<Int> H (pd,-1);
    
    TOOLS_DUMP(H.Vertices());
    TOOLS_DUMP(H.Edges());
    
    TOOLS_DUMP(H.Bends());
    valprint("bends",ArrayToString(&bends[0],{6}));
    
    TOOLS_DUMP(H.DirectedEdgeTurns());
    
    print("");
    cptr<Int> A_V_ptr = H.ArcVertexPointers().data();
    cptr<Int> A_V_idx = H.ArcVertexIndices().data();
//    
//    TOOLS_DUMP(H.ArcVertexPointers());
//    TOOLS_DUMP(H.ArcVertexIndices());
    
    for( Int a = 0; a < pd.ArcCount(); ++a )
    {
        valprint(
            "arc " + ToString(a),
            ArrayToString( &A_V_idx[A_V_ptr[a]], {A_V_ptr[a+1]-A_V_ptr[a]} )
        );
    }
    
    return EXIT_SUCCESS;
}
