#include "../../Knoodle.hpp"
#include "../../experimental/Tetrahedralization.hpp"

#include <iostream>

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

using Int = Int64;

using Tetrahedralization_T = Tetrahedralization<Int>;
using Vertex_T = Tetrahedralization_T::Vertex_T;
using Dedge_T  = Tetrahedralization_T::Dedge_T;
using Dface_T  = Tetrahedralization_T::Dface_T;
using Gluing_T = Tetrahedralization_T::Gluing_T;
using Z3    = Tetrahedralization_T::Z3;

int main()
{
    // insert code here...
    print("Hello, World!\n");
    
    Tetrahedralization_T T;
    
    {
        ScopedUnlock unlocker (T);
        
        Int t_0 = T.CreateTet();
        Int t_1 = T.CreateTet();
//        Int t_2 = T.CreateTet();
        
        T.Glue( {t_0, 0}, {t_1, 0}, Z3{2} );
//        T.Glue( {t_1, 1}, {t_2, 2}, Z3{1} );
    }
    
    
    TOOLS_DUMP(T.TetCount());
//    TOOLS_DUMP(T.TetGluings());
//    TOOLS_DUMP(T.VertexLabels());
    TOOLS_DUMP(T.TetGluingsOkayQ());
    
    Dedge_T de_0 (1,2);
    TOOLS_DUMP(de_0);
    TOOLS_DUMP(T.VertexLabel(de_0.Tail()));
    TOOLS_DUMP(T.VertexLabel(de_0.Head()));

    Dedge_T de_1 = T.CycleForward(de_0);
    TOOLS_DUMP(de_1);
    TOOLS_DUMP(T.VertexLabel(de_1.Tail()));
    TOOLS_DUMP(T.VertexLabel(de_1.Head()));
    
    TOOLS_DUMP(Abs(-4) % 3);
    TOOLS_DUMP(Abs(-3) % 3);
    TOOLS_DUMP(Abs(-2) % 3);
    TOOLS_DUMP(Abs(-1) % 3);
    TOOLS_DUMP( 0 % 3);
    TOOLS_DUMP( 1 % 3);
    TOOLS_DUMP( 2 % 3);
    TOOLS_DUMP( 3 % 3);
    
    return EXIT_SUCCESS;
}
