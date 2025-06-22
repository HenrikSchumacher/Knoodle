#define TOOLS_ENABLE_PROFILER
#define TENSORS_BOUND_CHECKS
#define TENSORS_ALLOCATION_LOGS

#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
//#include "../submodules/Tensors/Clp.hpp"
//#include "../src/OrthogonalRepresentation.hpp"
#include "../src/OrthogonalRepresentation2.hpp"
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
    
//    constexpr Int c_count = 3;
//    Int pd_code [c_count][5] = {
//        {5, 3, 0, 2,  1},
//        {1, 1, 2, 0,  1},
//        {4, 3, 5, 4, -1}
//    };
//
//    Int bends [6] = { 2, -3, 0, 0, 3, -2 };
    
    
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

    constexpr Int c_count = 9;
    Int pd_code [c_count][5] = {
        {17, 16, 0, 17, -1},
        {0, 16, 1, 15, 1},
        {1, 7, 2, 6, 1},
        {2, 7, 3,8, -1},
        {3, 9, 4, 8, 1},
        {4, 9, 5, 10, -1},
        {5, 11, 6, 10, 1},
        {14, 12, 15, 11, 1},
        {13, 12, 14, 13, -1}
    };
    
    PlanarDiagram<Int> pd = PlanarDiagram<Int>::FromSignedPDCode(
        &pd_code[0][0], c_count, 0, false, false
    );
    
    Profiler::Clear();
    
    OrthogonalRepresentation2<Int> H (pd);
    H.TurnRegularize();
    
    TOOLS_DUMP(H.CrossingCount());
    TOOLS_DUMP(H.ArcCount());
    TOOLS_DUMP(H.VertexCount());
    TOOLS_DUMP(H.EdgeCount());
    TOOLS_DUMP(H.VirtualEdgeCount());
    
    TOOLS_DUMP(H.VertexDedges());
    TOOLS_DUMP(H.Edges());
    TOOLS_DUMP(H.EdgeFlags());
    
    TOOLS_DUMP(H.Bends());
    
    TOOLS_DUMP(H.EdgeTurns());
    

    print("\nArcs (vertices)");
    {
        auto & A_V = H.ArcVertices();
        for( Int a = 0; a < A_V.SublistCount(); ++a )
        {
            valprint( "arc " + ToString(a), ToString(A_V.Sublist(a)) );
        }
    }
    
    print("\nArcs (edges)");
    {
        auto & A_E = H.ArcEdges();
        for( Int a = 0; a < A_E.SublistCount(); ++a )
        {
            valprint( "arc " + ToString(a), ToString(A_E.Sublist(a)) );
        }
    }
    
    print("\nFaces (dedges)");
    {
        auto & F_dE = H.FaceDedges();
        
        for( Int f = 0; f < F_dE.SublistCount(); ++f )
        {
            valprint( "face " + ToString(f), ToString(F_dE.Sublist(f)) );
        }
    }
  
    print("");
    print("Checks");
    print("");
    
    TOOLS_DUMP(H.CheckEdgeDirections());
    TOOLS_DUMP(H.CheckFaceTurns());
    
//    print("");
//    print("Diagram");
//    print("");
//    H.SetHorizontalGridSize(8);
//    H.SetVerticalGridSize(4);
//    H.SetHorizontalGapSize(1);
//    H.SetVerticalGapSize(1);
    
    
    
//    H.ComputeVertexCoordinates_ByTopologicalTightening();
//    
//    TOOLS_DUMP(H.VertexCoordinates());
//        
//    print(H.DiagramString());
//    
//    

//    
//    TOOLS_DUMP(H.FindAllIntersections(H.VertexCoordinates()));
//    
//    TOOLS_DUMP(H.Test_TRE_DhE());
//    TOOLS_DUMP(H.Test_TRE_DvE());
    
    return EXIT_SUCCESS;
}
