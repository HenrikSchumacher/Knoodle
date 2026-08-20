#define TOOLS_AGGRESSIVE_INLINING
#define TOOLS_USE_BOOST_UNORDERED
#define TOOLS_USE_MIMALLOC
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
    Profiler::Clear( HomeDirectory() );
    
#ifdef TOOLS_USE_MIMALLOC
    print("Compiled with mimalloc.");
#else
    print("Compiled without mimalloc.");
#endif
    
    std::filesystem::path in_file = HomeDirectory() / "Perf_Simplify_Diagram.txt";

    std::filesystem::path out_file = HomeDirectory() / "Perf_Simplify_PDCodes.txt";
    
    PD_T pd = PD_T::FromFile(in_file);
    PDC_T pdc { PD_T(pd) };
    TOOLS_DUMP(pdc.CrossingCount());
    
    Int counter = 0;
    
    tic("Main loop");
    for( int i = 0; i < 200; ++i )
    {
        pdc = PDC_T { PD_T(pd) };
        pdc.Simplify( {.embedding_trials = 10, .canonicalizeQ = true});
        counter += pdc.DiagramCount();
    }
    toc("Main loop");
    
    TOOLS_DUMP(pdc.DiagramCount());
    TOOLS_DUMP(pdc.TotalCrossingCount());
    TOOLS_DUMP(counter);
    
    tic("pdc.WriteToFile");
    pdc.WriteToFile(out_file);
    toc("pdc.WriteToFile");
}
