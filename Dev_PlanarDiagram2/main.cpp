#define TOOLS_ENABLE_PROFILER

#define PD_DEBUG
//#define PD_VERBOSE


#include "../Knoodle.hpp"
//#include "../src/OrthoDraw.hpp"
//#include "../Reapr.hpp"


using Int         = std::int64_t;                 // integer type used, e.g., for indices
using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;

static constexpr Int infty = Knoodle::Scalar::Infty<Int>;

//using OrthoDraw_T = Knoodle::OrthoDraw<Int>;
//using Reapr_T     = Knoodle::Reapr<double,Int>;

int main()
{
    Knoodle::Profiler::Clear();
    
    Tools::print( "-=| An example program for Knoodle. |=-" );
    Tools::print( "" );
 
    std::filesystem::path path = std::filesystem::path(__FILE__).parent_path();
  
    Tools::valprint("working directory", path );
    
    // Load a knot from file.
    std::vector<Int> pd_code;
    
    {
        std::string filename ( path / "../Example_Knoodle/ExampleKnot.tsv" );
        std::ifstream s ( filename );
        Int number;
        
        if(!s)
        {
            Tools::eprint( "File " + filename + " not found." );
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
    PD_T pd_0 = PD_T::FromSignedPDCode(
        &pd_code[0],     // pointer to array of pd code.
        c_count,         // number of crossingss
        false             // whether to compress
    );
    
//    TOOLS_LOGDUMP(pd_0.CopyCrossing(0));
//    TOOLS_LOGDUMP(pd_0.CopyArc(0));
//    
//    TOOLS_LOGDUMP(pd_0.Crossings());
//    TOOLS_LOGDUMP(pd_0.CrossingStates());
//    TOOLS_LOGDUMP(pd_0.Arcs());
//    TOOLS_LOGDUMP(pd_0.ArcStates());
//    TOOLS_LOGDUMP(pd_0.ArcColors());
    
    pd_0.Compress();
    
    TOOLS_DUMP(pd_0.CheckAll());
    TOOLS_DUMP(pd_0.CheckLeftDarc());
    TOOLS_DUMP(pd_0.CheckRightDarc());
//    TOOLS_DUMP(pd_0.CheckNextDarc());

    
    TOOLS_DUMP(pd_0.PDCode());
    
    PDC_T pdc ( std::move(pd_0), Int(0) );
    
    Knoodle::print("");
    Knoodle::print("RecomputeArcStates");
    for( Int i = 0; i < pdc.DiagramCount(); ++i )
    {
        TOOLS_DUMP( pdc.Diagram(i).CheckAll() );
    }

    
    TOOLS_DUMP(pdc.DiagramCount());
    TOOLS_DUMP(pdc.Diagram(0).ColorCount());
    TOOLS_DUMP(pdc.ColorCount());
    TOOLS_DUMP(pdc.CrossingCount());

    
    {
        std::ofstream file ("/Users/Henrik/a.txt");
        file << ToString( pdc.Diagram(0).PDCode() );
    }
    
    
    
    Knoodle::print("");
    Knoodle::print("SimplifyLocal(4,false)");
    try
    {
        pdc.SimplifyLocal(4,false);
    }
    catch( const std::exception & e )
    {
        Knoodle::eprint(e.what());
        exit(-1);
    }
    
    for( Int i = 0; i < pdc.DiagramCount(); ++i )
    {
        TOOLS_DUMP( pdc.Diagram(i).CheckAll() );
    }
    
    TOOLS_DUMP(pdc.CrossingCount());
    TOOLS_DUMP(pdc.Diagram(0).CrossingCount());
    TOOLS_DUMP(pdc.Diagram(0).MaxCrossingCount());
    TOOLS_DUMP(pdc.Diagram(0).PDCode().Dim(0));

    
    
    Knoodle::print("");
    Knoodle::print("SimplifyGlobal(6,infty,4,infty,true)");
    try
    {
        pdc.SimplifyGlobal(6,infty,4,infty,true);
    }
    catch( const std::exception & e )
    {
        Knoodle::eprint(e.what());
        exit(-1);
    }
    
    for( Int i = 0; i < pdc.DiagramCount(); ++i )
    {
        TOOLS_DUMP( pdc.Diagram(i).CheckAll() );
    }
    
    TOOLS_DUMP(pdc.CrossingCount());
    TOOLS_DUMP(pdc.Diagram(0).CrossingCount());
    TOOLS_DUMP(pdc.Diagram(0).MaxCrossingCount());
    TOOLS_DUMP(pdc.Diagram(0).PDCode().Dim(0));
    
    return EXIT_SUCCESS;
}
