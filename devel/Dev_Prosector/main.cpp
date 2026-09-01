//#define TOOLS_NO_RESTRICT

//#define TOOLS_ENABLE_PROFILER
//#define TOOLS_NO_INT128
#define TOOLS_AGGRESSIVE_INLINING


#include "../../Knoodle.hpp"
//#include "../../experimental/LinkEmbedding_Boost.hpp"
#include "../../experimental/LinkEmbedding3.hpp"
#include "../../experimental/LinkEmbedding4.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using Int   = Int64;
using IReal = Int64;

using Link1_T = LinkEmbedding<Real,Int>;
//using Link2_T = LinkEmbedding_Boost<Real,Int>;
using Link3_T = LinkEmbedding3<Real,Int>;
using Link4_T = LinkEmbedding4<Real,Int,IReal>;
using Link5_T = LinkEmbedding5<Real,Int,IReal>;

using PDC_T   = PlanarDiagramComplex<Int>;
int main()
{
    Profiler::Clear();
    int err;
    
    auto file = HomeDirectory() / "a.txt";
    
    print("");

    auto L1 = Link1_T::FromFile(file);
    tic(L1.ClassName());
    err = L1.RequireIntersections();
    toc(L1.ClassName());
    TOOLS_DUMP(err);
    print("");
    
//    auto L2 = Link2_T::FromFile(file);
//    tic(L2.ClassName());
//    err = L2.RequireIntersections();
//    toc(L2.ClassName());
//    TOOLS_DUMP(err);
//    print("");
    
    Link3_T L3 = Link3_T::FromFile(file);
    tic(L3.ClassName());
    err = L3.RequireIntersections();
    toc(L3.ClassName());
    TOOLS_DUMP(err);
    print("");
    
    Link4_T L4 = Link4_T::FromFile(file);
    tic(L4.ClassName());
    err = L4.RequireIntersections();
    toc(L4.ClassName());
    TOOLS_DUMP(err);
    print("");
    
    Link5_T L5 = Link5_T::FromFile(file);
    tic(L5.ClassName());
    err = L5.RequireIntersections();
    toc(L5.ClassName());
    TOOLS_DUMP(err);
    print("");
    
    TOOLS_DUMP(L1.AllocatedByteCount());
//    TOOLS_DUMP(L2.AllocatedByteCount());
    TOOLS_DUMP(L3.AllocatedByteCount());
    TOOLS_DUMP(L4.AllocatedByteCount());
    TOOLS_DUMP(L5.AllocatedByteCount());
    
    print("");
    
    
//    TOOLS_DUMP(L2.EdgeCrossings() == L1.EdgeCrossings());
    TOOLS_DUMP(L3.EdgeCrossings() == L1.EdgeCrossings());
    TOOLS_DUMP(L4.EdgeCrossings() == L1.EdgeCrossings());
    TOOLS_DUMP(L5.EdgeCrossings() == L1.EdgeCrossings());
    
//    PlanarDiagramComplex<Int> pdc1 (L1);
//    PlanarDiagramComplex<Int> pdc5 (L5);
//
//    pdc1.Simplify();
//    pdc5.Simplify();
//    
//    TOOLS_DUMP(pdc1.CrossingCount());
//    TOOLS_DUMP(pdc5.CrossingCount());
//    

}
