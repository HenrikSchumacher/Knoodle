#define TOOLS_ENABLE_PROFILER
#define TENSORS_BOUND_CHECKS
#define TENSORS_ALLOCATION_LOGS

#define dump(x) TOOLS_DUMP(x);
#define mem_dump(x) TOOLS_MEM_DUMP(x);

#include "../Knoodle.hpp"
//#include "../submodules/Tensors/Clp.hpp"
//#include "../src/OrthogonalRepresentation.hpp"
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

using OR_T = OrthogonalRepresentation<Int>;

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

//    constexpr Int c_count = 9;
//    Int pd_code [c_count][5] = {
//        {17, 16, 0, 17, -1},
//        {0, 16, 1, 15, 1},
//        {1, 7, 2, 6, 1},
//        {2, 7, 3,8, -1},
//        {3, 9, 4, 8, 1},
//        {4, 9, 5, 10, -1},
//        {5, 11, 6, 10, 1},
//        {14, 12, 15, 11, 1},
//        {13, 12, 14, 13, -1}
//    };
    
    constexpr Int c_count = 14;
    Int pd_code [c_count][5] = {
        {27, 11, 0, 10, 1},
        {11, 1, 12, 0, 1},
        {1, 9, 2, 8, 1},
        {7, 3, 8, 2, 1},
        {3, 7, 4, 6, 1},
        {13, 4, 14, 5, -1},
        {5, 12, 6, 13, -1},
        {9, 27, 10, 26, 1},
        {14, 17, 15, 18, -1},
        {18, 15, 19, 16, -1},
        {16, 19, 17, 20, -1},
        {23, 21, 24, 20, 1},
        {21, 25, 22, 24, 1},
        {25, 23, 26, 22, 1}
    };
    
    PlanarDiagram<Int> pd = PlanarDiagram<Int>::FromSignedPDCode(
        &pd_code[0][0], c_count, 0, false, false
    );
    
    Profiler::Clear();
    
    OR_T H (pd, Int(-1),
        {
            .redistribute_bendsQ     = true,
            .use_dual_simplexQ       = false,
            .turn_regularizeQ        = true,
            .randomizeQ              = false,
            .saturate_facesQ         = true,
            .saturate_exterior_faceQ = true,
            .compaction_method       = 0,
        
            .x_grid_size             = 8,
            .y_grid_size             = 4,
            .x_gap_size              = 1,
            .y_gap_size              = 1
        }
    );
    
    TOOLS_DUMP(H.CrossingCount());
    TOOLS_DUMP(H.ArcCount());
    TOOLS_DUMP(H.VertexCount());
    TOOLS_DUMP(H.EdgeCount());
    TOOLS_DUMP(H.VirtualEdgeCount());
    
//    TOOLS_DUMP(H.VertexFlags());
//    TOOLS_DUMP(H.VertexDedges());
//    TOOLS_DUMP(H.Edges());
//    TOOLS_DUMP(H.EdgeFlags());
//    TOOLS_DUMP(H.Bends());
//    TOOLS_DUMP(H.EdgeTurns());
    

//    print("\nArcs (vertices)");
//    {
//        auto & A_V = H.ArcVertices();
//        for( Int a = 0; a < A_V.SublistCount(); ++a )
//        {
//            valprint( "arc " + ToString(a), ToString(A_V.Sublist(a)) );
//        }
//    }
//    
//    print("\nArcs (edges)");
//    {
//        auto & A_E = H.ArcEdges();
//        for( Int a = 0; a < A_E.SublistCount(); ++a )
//        {
//            valprint( "arc " + ToString(a), ToString(A_E.Sublist(a)) );
//        }
//    }
//    
//    print("\nFaces (dedges)");
//    {
//        auto & F_dE = H.FaceDedges();
//        
//        for( Int f = 0; f < F_dE.SublistCount(); ++f )
//        {
//            valprint( "face " + ToString(f), ToString(F_dE.Sublist(f)) );
//        }
//    }
  
    print("");
    print("Checks");
    print("");
    
    TOOLS_DUMP(H.CheckEdgeDirections());
    TOOLS_DUMP(H.CheckAllFaceTurns());
    
    print("");
    print("Diagram -- Default");
    print("");

    TOOLS_DUMP(H.Width());
    TOOLS_DUMP(H.Height());
    TOOLS_DUMP(H.Area());
    TOOLS_DUMP(H.Length());
    print(H.DiagramString());
    
//    H.ArcLines();
//    H.ArcSplines();

//    TOOLS_DUMP(H.FindAllIntersections(H.VertexCoordinates()));

    
    print("");
    print("Diagram -- ByLengthVariant1");
    print("");


    H.ComputeVertexCoordinates_ByLengths_Variant2(true);
    TOOLS_DUMP(H.Width());
    TOOLS_DUMP(H.Height());
    TOOLS_DUMP(H.Area());
    TOOLS_DUMP(H.Length());
    print(H.DiagramString());
    
    auto Gl_saturating_edges = H.template SaturatingEdges<0>();
    
    TOOLS_DUMP(Gl_saturating_edges);
    
    auto Gr_saturating_edges = H.template SaturatingEdges<1>();
    
    TOOLS_DUMP(Gr_saturating_edges);

    
    TOOLS_DUMP(H.VerticalSegmentVertices());
    TOOLS_DUMP(H.HorizontalSegmentVertices());
    
    H.SeparationConstraints();
    
    H.Dv().DirectedAdjacencyMatrix();
    
    {
        auto A = H.Dv().CreateAdjacencyMatrix<true,int>( (int*)nullptr, 8 );
     
        TOOLS_DUMP(A.ToTensor2());
    }
    
    {
        auto A = H.Dv().Laplacian();
     
        TOOLS_DUMP(A.ToTensor2());
        
        TOOLS_DUMP(A.NonzeroPositions_AoS());
        
        TOOLS_DUMP(H.Dv().Edges());
    }
    
    H.PrintSettings();
    
    H.SegmentsInfluencedByVirtualEdges();
    
    return EXIT_SUCCESS;
}
