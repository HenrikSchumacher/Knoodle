#define KNOODLE_USE_BOOST_UNORDERED
//#define KNOODLE_USE_CLP

//#define TENSORS_BOUND_CHECKS

//#define TOOLS_ENABLE_PROFILER
//#define PD_DEBUG
//#define PD_VERBOSE
//#define PD_COUNTERS



#include "../Knoodle.hpp"
#include "../src/OrthoDraw.hpp"
//#include "../Reapr.hpp"

//#include "../src/ActionAngleSampler.hpp"
#include "../src/ConformalBarycenterSampler.hpp"

using Real        = double;
// integer type used, e.g., for indices
using Int         = std::int64_t;
//using Int         = std::int32_t;
//using Int         = std::int16_t;

using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;
//using Reapr_T     = Knoodle::Reapr<Real,Int>;


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

    TOOLS_DUMP( sizeof(Tools::CachedObject<0,0,0,0>) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<0,0,0,0>::CacheContainer_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<0,0,0,0>::CacheMutex_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<0,0,0,0>::P_CacheContainer_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<0,0,0,0>::P_CacheMutex_T) );

    TOOLS_DUMP( sizeof(Tools::CachedObject<1,0,0,0,8>) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,0,0,0,8>::CacheContainer_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,0,0,0,8>::CacheMutex_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,0,0,0,8>::P_CacheContainer_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,0,0,0,8>::P_CacheMutex_T) );
    
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,1,1,1>) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,1,1,1>::CacheContainer_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,1,1,1>::CacheMutex_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,1,1,1>::P_CacheContainer_T) );
    TOOLS_DUMP( sizeof(Tools::CachedObject<1,1,1,1>::P_CacheMutex_T) );

    print("\n\n");
    
    TOOLS_DUMP( PDC_T::ClassName() );
    TOOLS_DUMP( sizeof(PDC_T) );
    TOOLS_DUMP( alignof(PDC_T) );
    
    TOOLS_DUMP( PD_T::ClassName() );
    TOOLS_DUMP( sizeof(PD_T) );
    TOOLS_DUMP( alignof(PD_T) );
    
    TOOLS_DUMP( PD_T::CrossingContainer_T::ClassName() );
    TOOLS_DUMP( sizeof(PD_T::CrossingContainer_T) );
    TOOLS_DUMP( alignof(PD_T::CrossingContainer_T) );
    
    TOOLS_DUMP( PD_T::ArcContainer_T::ClassName() );
    TOOLS_DUMP( sizeof(PD_T::ArcContainer_T) );
    TOOLS_DUMP( alignof(PD_T::ArcContainer_T) );
    
    print("\n\n");
    
    TOOLS_DUMP( sizeof(Tensors::Tensor1<Int,Int>) );
    TOOLS_DUMP( sizeof(Tensors::Tensor1<Tools::Int32,Tools::Int32>) );
    
    TOOLS_DUMP( sizeof(Tensors::Tensor2<Int,Int>) );
    TOOLS_DUMP( sizeof(Tensors::Tensor2<Tools::Int32,Tools::Int32>) );
    
    TOOLS_DUMP( sizeof(Tensors::Tensor3<Int,Int>) );
    TOOLS_DUMP( sizeof(Tensors::Tensor3<Tools::Int32,Tools::Int32>) );
    
    
    print("\n\n");
    
    std::filesystem::path in_path  = std::filesystem::path(__FILE__).parent_path();
    valprint("Input  directory", in_path );
//    std::filesystem::path out_path = in_path;
    std::filesystem::path out_path = "/Volumes/RamDisk";
    valprint("Output directory", out_path );
    Knoodle::Profiler::Clear(out_path,false,false);
    
    print( "-=| An example program for Knoodle. |=-" );
    print( "" );
 
//    // Load a knot from file.
//    std::vector<Int> pd_code;
//    
//    {
//        std::string filename ( in_path / "../Example_Knoodle/ExampleKnot.tsv" );
//        std::ifstream s ( filename );
//        Int number;
//        
//        if(!s)
//        {
//            Tools::eprint( "File " + filename + " not found." );
//            return EXIT_FAILURE;
//        }
//        
//        while( s )
//        {
//            if( s >> number )
//            {
//                pd_code.push_back(number);
//            }
//        }
//    }
//    
//    Int c_count = static_cast<Int>( pd_code.size()/5 );
//    
//    // Create an instance of PlanarDiagram.
//    PDC_T pdc ( PD_T::FromSignedPDCode( &pd_code[0], c_count) );
    
    const Int n = 10'000;
    Knoodle::ConformalBarycenterSampler<3,Real,Int> S ( n );
    Tensors::Tensor2<Real,Int> vertex_coordinates( n + 1, 3 );
    Real K = 0;
    S.WriteRandomClosedPolygon(vertex_coordinates.data(),K);
    PDC_T pdc = PDC_T::FromKnotEmbedding ( vertex_coordinates.data(), n );
    

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
//            .local_opt_level        = 0,
//            .strategy               = Knoodle::DijkstraStrategy_T::Bidirectional,
//            .disconnectQ            = true,
//            .splitQ                 = true,
//            .compressQ              = true,
//            .reapr_embedding_trials = 9,
//            .reapr_rotation_trials  = 1
//        });
//    }
//    catch( const std::exception & e )
//    {
//        Knoodle::eprint(e.what());
//        exit(-1);
//    }
    
    print("");
    PrintInfo(pdc);
    print("");
    
    print("Simplify() + Reapr");
    try
    {
        pdc.Simplify({
            .local_opt_level       = 0,
            .strategy              = Knoodle::DijkstraStrategy_T::Bidirectional,
            .disconnectQ           = true,
            .splitQ                = true,
            .compressQ             = true,  // compress during rerouting
            .compression_threshold = 0,     // don't compress if crossing_count <= compression_threshold
            .embedding_trials      = 5,
            .rotation_trials       = 5
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
