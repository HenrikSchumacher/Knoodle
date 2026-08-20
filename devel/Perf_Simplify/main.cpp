//#define TOOLS_NO_RESTRICT

//#define TOOLS_NO_INT128
//#define KNOODLE_USE_BOOST_MULTIPRECISION
#define TOOLS_AGGRESSIVE_INLINING
#define TOOLS_USE_BOOST_UNORDERED
#define KNOODLE_USE_BOOST_UNORDERED
//#define PD_ALLOCATE_SCRATCH

#ifdef __APPLE__
    #include "../../submodules/Tensors/Accelerate.hpp"
#else
    #include "../../submodules/Tensors/OpenBLAS.hpp"
#endif

#include "../../Knoodle.hpp"

using namespace Knoodle;
using namespace Tools;

using Real = Real64;
using Int  = Int64;

using PDC_T = PlanarDiagramComplex<Int>;
using PD_T  = PDC_T::PD_T;

int main()
{
    Profiler::Clear();
    
    std::filesystem::path in_file = HomeDirectory() / "Perf_Simplify_Diagram.txt";

    std::filesystem::path out_file = HomeDirectory() / "Perf_Simplify_PDCodes.txt";
    
    tic("PD_T::FromFile");
    PDC_T pdc { PD_T::FromFile(in_file) };
    toc("PD_T::FromFile");
    
//    tic("PD_T::WriteToFile");
//    pdc.Diagram(0).WriteToFile(out_file);
//    toc("PD_T::WriteToFile");
    
    TOOLS_DUMP(pdc.TotalCrossingCount());
  
//    TOOLS_DUMP(pdc.Diagram(0).FaceCount());
    
    tic("pdc.Simplify");
    pdc.Simplify( {.embedding_trials = 2, .canonicalizeQ = true});
    toc("pdc.Simplify");
    
    TOOLS_DUMP(pdc.DiagramCount());
    TOOLS_DUMP(pdc.TotalCrossingCount());
    
//    tic("pdc.WriteToFile");
//    pdc.WriteToFile(out_file);
//    toc("pdc.WriteToFile");
}
