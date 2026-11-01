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
using Zmod3    = Tetrahedralization_T::Zmod3;

int main()
{
    // insert code here...
    std::cout << "Hello, World!\n";
    
    Tetrahedralization_T T;
    
    {
        ScopedUnlock unlocker (T);
        
        Int t_0 = T.CreateTet();
        Int t_1 = T.CreateTet();
//        Int t_2 = T.CreateTet();
        
        T.Glue( {t_0, 0}, {t_1, 0}, Zmod3{2} );
//        T.Glue( {t_1, 1}, {t_2, 2}, Zmod3{1} );
    }
    
    
    TOOLS_DUMP(T.TetCount());
    TOOLS_DUMP(T.TetGluings());
    TOOLS_DUMP(T.VertexLabels());
    
    TOOLS_DUMP(T.TetGluingsOkayQ());
    
    {
        Zmod3 a ( 1 );
        Zmod3 b ( 2 );
        TOOLS_DUMP(a);
        TOOLS_DUMP(b);
        TOOLS_DUMP(a+b);
        TOOLS_DUMP(a-b);
        TOOLS_DUMP(-a);
    }
    
    Dface_T df_0 {Int{0}, Int{0}};
    Dface_T df_1 {Int{0}, Int{3}};
    
    TOOLS_DUMP(df_0);
    Gluing_T g = T.Gluing(df_0);
    TOOLS_DUMP(g);
    
    TOOLS_DUMP(df_0.Vertex(Zmod3{0}));
    TOOLS_DUMP(df_0.Vertex(Zmod3{1}));
    TOOLS_DUMP(df_0.Vertex(Zmod3{2}));
    
    TOOLS_DUMP(df_1.Vertex(Zmod3{0}));
    TOOLS_DUMP(df_1.Vertex(Zmod3{1}));
    TOOLS_DUMP(df_1.Vertex(Zmod3{2}));
    
    
    return EXIT_SUCCESS;
}
