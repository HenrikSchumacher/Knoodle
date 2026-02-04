#define KNOODLE_USE_BOOST_UNORDERED

#define TENSORS_BOUND_CHECKS

#define TOOLS_ENABLE_PROFILER
#define PD_DEBUG
#define PD_VERBOSE
//#define PD_COUNTERS



#include "../Knoodle.hpp"
#include "../src/OrthoDraw.hpp"
//#include "../Reapr.hpp"


// integer type used, e.g., for indices
using Int         = std::int64_t;
//using Int         = std::int32_t;
//using Int         = std::int16_t;

using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
//using Reapr_T     = Knoodle::Reapr<double,Int>;


template<typename T>
void valprint( const std::string & s, const T & val )
{
    Tools::valprint(s,val);
}

void print( const std::string & s )
{
    Tools::print(s);
}

void PrintInfo( const PDC_T & pdc )
{
    valprint( "CrossingCount()", pdc.CrossingCount() );
    valprint( "MaxMaxCrossingCount()", pdc.MaxMaxCrossingCount() );
    valprint( "TotalMaxCrossingCount()", pdc.TotalMaxCrossingCount() );
    valprint( "DiagramCount()", pdc.DiagramCount() );
    for( Int i = 0; i < pdc.DiagramCount(); ++i )
    {
        const PD_T & pd = pdc.Diagram(i);
        
        if( pd.ProvenUnknotQ() ) { continue; }
        
        print("Diagram(" + Tools::ToString(i) + ")");
        valprint("\tValidQ()", pd.ValidQ());
        valprint("\tCheckAll()", pd.CheckAll());
        valprint("\tCrossingCount()", pd.CrossingCount());
        valprint("\tMaxCrossingCount()", pd.MaxCrossingCount());
        
//        pd.PrintInfo();
    }
}

int main()
{
    
    std::filesystem::path in_path  = std::filesystem::path(__FILE__).parent_path();
    valprint("Input  directory", in_path );
//    std::filesystem::path out_path = in_path;
    std::filesystem::path out_path = "/Volumes/RamDisk";
    valprint("Output directory", out_path );
    Knoodle::Profiler::Clear(out_path,false,false);
    
    print( "-=| An example program for Knoodle. |=-" );
    print( "" );
 
    // Load a knot from file.
    std::vector<Int> pd_code;
    
    {
        std::string filename ( in_path / "../Example_Knoodle/ExampleKnot.tsv" );
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
    
    print("Compress()");
    pd_0.Compress();
    
//    print("");
//    pd_0.PrintInfo();
//    print("");
    
    PDC_T pdc ( std::move(pd_0), Int(0) );
    
    print("");
    PrintInfo(pdc);
    print("");

    {
        std::ofstream file ( out_path / "PDCode.tsv");
        file << ToString( pdc.Diagram(0).PDCode() );
    }
    
//    print("");
//    print("SimplifyLocal(4,false)");
//    try
//    {
//        pdc.SimplifyLocal(4,false);
//    }
//    catch( const std::exception & e )
//    {
//        Knoodle::eprint(e.what());
//        exit(-1);
//    }
//    
//    print("");
//    PrintInfo(pdc);
//    print("");
    
//    print("Simplify()");
//    try
//    {
//        pdc.Simplify({
//            .local_opt_level = 0,
//            .strategy        = Knoodle::DijkstraStrategy_T::Bidirectional,
//            .disconnectQ     = true,
//            .splitQ          = true,
//            .compressQ       = true
//        });
//    }
//    catch( const std::exception & e )
//    {
//        Knoodle::eprint(e.what());
//        exit(-1);
//    }
    
    print("Simplify2()");
    try
    {
        pdc.Simplify2({
            .strategy        = Knoodle::DijkstraStrategy_T::Bidirectional,
            .disconnectQ     = true,
            .splitQ          = true,
            .compressQ       = true
        });
    }
    catch( const std::exception & e )
    {
        Knoodle::eprint(e.what());
        exit(-1);
    }
    
    print("");
    PrintInfo(pdc);
    print("");
    
//    print("Unite()");
//    pdc.Unite();
//    
//    print("");
//    PrintInfo(pdc);
//    print("");
//    
//    print("Split()");
//    pdc.Split();
//    
//    print("");
//    PrintInfo(pdc);
//    print("");

    
//    pdc.DisconnectDiagrams();
    
    // Graphics settings for ASCII art. (Move on, nothing to see here.)
    OrthoDraw_T::Settings_T plot_settings {
        .x_grid_size              = 8,
        .y_grid_size              = 4,
        .x_gap_size               = 1,
        .y_gap_size               = 1
    };

    std::string filename ( out_path / "Diagram_Simplified.txt" );
    Tools::print( "Writing diagram(s) to file " + filename + "." );
    std::ofstream out_file ( filename );
    
    for( Int i = 0; i < pdc.DiagramCount(); ++ i )
    {
        const PD_T & pd = pdc.Diagram(i);
        
        if( pd.ProvenUnknotQ() ) { continue; }
        
        Tools::print( "Connected component no. " + Tools::ToString(i) + ":" );
        valprint("\tCrossingCount()", pd.CrossingCount());
        // Create an orthogonal layout for the current knot diagram.
        OrthoDraw_T H ( pd, Int(-1), plot_settings );
//        Tools::print(H.DiagramString());
        out_file << H.DiagramString() << "\n";
        Tools::print("");
    }
    
#ifdef PD_COUNTERS
    TOOLS_DUMP(pdc.StrandSimplifier().RecordedDualArcs().size());
    TOOLS_DUMP(pdc.StrandSimplifier().RecordedFaceSizes().size());
#endif
    
    return EXIT_SUCCESS;
}
