#define TOOLS_ENABLE_PROFILER

#define TOOLS_AGGRESSIVE_INLINING

//#define TOOLS_ENABLE_PROFILER

//#define PD_DEBUG
//#define PD_VERBOSE
//#define PD_TIMINGQ
//#define PD_COUNTERS

//#define TOOLS_DEBUG
//#define TENSORS_BOUND_CHECKS
//#define TENSORS_ALLOCATION_LOGS

//#define PD_ALLOCATE_SCRATCH

#ifdef __APPLE__
    #include "../../submodules/Tensors/Accelerate.hpp"
#else
    #include "../../submodules/Tensors/OpenBLAS.hpp"
#endif


#include "../../Knoodle.hpp"
#include "../../src/KnotInvariants/Alexander_UMFPACK.hpp"

using namespace Knoodle;
using namespace Tensors;
using namespace Tools;

using Real  = Real64; // scalar type used for positions of polygon
using Int   = Int64;  // integer type used, e.g., for indices
using LInt  = Int64;  // integer type for counting objects

using Complex = Complex64;

using PDC_T           = PlanarDiagramComplex<Int>;
using LinkEmbedding_T = PDC_T::LinkEmbedding_T;
using PD_T            = PDC_T::PD_T;
using OrthoDraw_T     = OrthoDraw<PD_T>;
using Alex_T          = Alexander_UMFPACK<Complex,Int>;

int main()
{
    std::filesystem::path path      = std::filesystem::path(__FILE__);
    Tools::valprint("working directory", path );
    
    std::filesystem::path data_path = path.parent_path().parent_path().parent_path() / "examples" / "Example_Knoodle";
    Tools::valprint("data directory", data_path );
    
    // Load a knot from file.
    std::vector<Int> pd_code;
    
    {
        std::string filename ( data_path / "ExampleKnot.tsv" );
        std::ifstream s ( filename );
        Int number;
        
        if(!s)
        {
            Tools::eprint( "File ", filename, " not found." );
            return EXIT_FAILURE;
        }
        
        while( s )
        {
            if( s >> number )
            {
                pd_code.push_back(number);
            }
        }
    }
    
    Int c_count = static_cast<Int>( pd_code.size()/5 );
    
    // Create an instance of PlanarDiagram.
    PDC_T pdc ( PD_T::FromSignedPDCode(
        &pd_code[0],     // pointer to array of pd code.
        c_count          // number of crossingss
    ));
    
    pdc.Simplify();
    
    PD_T pd = pdc.Diagram(0);
    
    Alex_T alex;
    
    
    Complex mantissa;
    Int     exponent;
    
    alex.Alexander(pd, Complex(0.4), mantissa, exponent, false );
    
    TOOLS_DUMP(mantissa);
    TOOLS_DUMP(exponent);
    
    
    return EXIT_SUCCESS;
}
