#include <iostream>
#include <iterator>
#include <boost/program_options.hpp>
#include <exception>

//#define TOOLS_DEACTIVATE_VECTOR_EXTENSIONS
//#define TOOLS_DEACTIVATE_MATRIX_EXTENSIONS

//#ifdef __APPLE__
//    #include "../submodules/Tensors/Accelerate.hpp"
//#else
//    #include "../submodules/Tensors/OpenBLAS.hpp"
//#endif

#include "../KnotTools.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

using Real = double;
using Int  = Int64;
using LInt = Int64;

constexpr Int AmbDim = 3;

using Link_T = Link_2D<Real,Int,Int8>;
using PD_T   = PlanarDiagram<Int>;

namespace po = boost::program_options;

void write_pd_code( mref<std::ofstream> pds, mref<PD_T> PD )
{
    if( PD.CrossingCount() <= 0 )
    {
        return;
    }
    
    auto pdcode = PD.PDCode();
    
    cptr<Int> a = pdcode.data();
    
    pds << "s " << PD.ProvablyMinimalQ();
    
    for( Int i = 0; i < pdcode.Dimension(0); ++i )
    {
        pds << "\n" << a[5 * i + 0]
            << "\t" << a[5 * i + 1]
            << "\t" << a[5 * i + 2]
            << "\t" << a[5 * i + 3]
            << "\t" << a[5 * i + 4];
    }
}

int main( int argc, char** argv )
{
    std::cout << "Welcome to PolyFold.\n\n";
    
    
    std::cout << "Vector extensions " << ( vec_enabledQ ? "enabled" : "disabled" ) << ".\n";
    std::cout << "Matrix extensions " << ( mat_enabledQ ? "enabled" : "disabled" ) << ".\n";
    std::cout << "\n";
    
    Int job_count = 1;
    
    Int  n = 1;
    Int  N = 1;

    LInt  burn_in = 1;
    LInt  skip    = 1;
    
    bool verboseQ = false;
    bool appendQ  = false;
    
    std::filesystem::path path;
    
    // `try` executes some code; `catch` catches exceptions and handles them.
    try
    {
        // Declare all possible options with type information and a documentation string that will be printed when calling this routine with the -h [--help] option.
        po::options_description desc("Allowed options");
        desc.add_options()
        ("help,h", "produce help message")
        ("jobs,j", po::value<Int>()->default_value(1), "set number of jobs (time series)")
        ("edge-count,n", po::value<Int>(), "set number of edges")
        ("burn-in,b", po::value<LInt>(), "set number of burn-in steps")
        ("skip,s", po::value<LInt>(), "set number of steps skipped between samples")
        ("sample-count,N", po::value<Int>(), "set number of samples")
        ("output,o", po::value<std::string>(), "set output directory")
        ("verbose,v", po::value<std::string>(), "gather statistics for each sample")
        ("append,a", po::value<std::string>(), "append to existent file")
        ;
        
        // Declare that arguments without option prefixe are reinterpreted as if they were assigned to -o [--output].
        po::positional_options_description p;
        p.add("output", -1);
        
        // Parse the arguments.
        po::variables_map vm;
        po::store(
            po::command_line_parser(argc, argv).options(desc).positional(p).run(),
            vm
        );
        po::notify(vm);
        
        if( vm.count("help") )
        {
            std::cout << desc << "\n";
            return 0;
        }
        
        if( vm.count("output") )
        {
            path = std::filesystem::path( vm["output"].as<std::string>() );
            
            std::cout << "Output path set to "
            << path << ".\n";
        }
        else
        {
            throw std::invalid_argument( "Output path unspecified. Use the option -o to set it." );
        }
        
        if( vm.count("jobs") )
        {
            job_count = vm["jobs"].as<Int>();
            
            std::cout << "Number of jobs set to " << job_count << ".\n";
        }
        else
        {
            job_count = 1;
            std::cout << "Using default number of jobs (" << 1 << ").\n";
        }
        
        if( vm.count("edge-count") )
        {
            n = vm["edge-count"].as<Int>();
            
            std::cout << "Number of edges set to "
            << n << ".\n";
        }
        else
        {
            throw std::invalid_argument( "Number of edges unspecified. Use the option -n to set it." );
        }
        
        if( vm.count("sample-count") )
        {
            N = vm["sample-count"].as<Int>();
            
            std::cout << "Number of samples set to "
            << N << ".\n";
        }
        else
        {
            throw std::invalid_argument( "Number of samples unspecified. Use the option -N to set it." );
        }
        
        if( vm.count("burn-in") )
        {
            burn_in = vm["burn-in"].as<LInt>();
            
            std::cout << "Number of burn-in steps set to "
            << burn_in << ".\n";
        }
        else
        {
            throw std::invalid_argument( "Number of burn-in steps unspecified. Use the option -b to set it." );
        }
        
        if( vm.count("skip") )
        {
            skip = vm["skip"].as<LInt>();
            
            std::cout << "Number of skip steps set to "
            << skip << ".\n";
        }
        else
        {
            throw std::invalid_argument( "Number of skip unspecified. Use the option -s to set it." );
        }
        
        
        if( vm.count("verbose") )
        {
            verboseQ = true;
        }
        
        std::cout << "Report mode set to " << (verboseQ ? "verbose" : "muted" ) << ".\n";
        
        if( vm.count("append") )
        {
            appendQ = true;
        }
        
        std::cout << "Write mode set to " << (appendQ ? "append" : "overwrite" ) << ".\n";
    }
    catch( std::exception & e )
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Exception of unknown type!\n";
        return 1;
    }
    
    std::cout << "" << std::endl;
    
    // Make sure that the working directory exists.
    try{
        std::filesystem::create_directory( path );
    }
    catch( std::exception & e )
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Exception of unknown type!\n";
        return 1;
    }
    
    // Use this path for profiles and general log files.
    Profiler::Clear(path);
    
    print("Start sampling.");
    
    const Time job_start_time = Clock::now();
    
    std::filesystem::path log_file = path / "Log.txt";
    std::filesystem::path pds_file = path / "PDCodes.tsv";
    
    std::ofstream log;
    std::ofstream pds;
    
    // Open file as stream.
    try {
        if( appendQ )
        {
            log.open( log_file, std::ios_base::app );
        }
        else
        {
            log.open( log_file );
        }
    }
    catch( std::exception & e )
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Exception of unknown type!\n";
        return 1;
    }
    
    // Open file as stream.
    try {
        if( appendQ )
        {
            pds.open( pds_file, std::ios_base::app );
        }
        else
        {
            pds.open( pds_file );
        }
    }
    catch( std::exception & e )
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Exception of unknown type!\n";
        return 1;
    }
        
    
    
    
    // Stream data to the file.
    log << "n = " << n << "\n";
    log << "N = " << N << "\n";
    log << "burn-in = " << burn_in << "\n";
    log << "skip = " << skip << "\n";
    
    // std::endl adds a "\n" and flushes the string buffer so that the string is actually written to file.
    log << "\n " << std::endl;
    
    
    ClisbyTree<AmbDim,Real,Int,LInt> T ( n, Real(1) );

    LInt steps_taken = 0;

    // Burn-in.
    {
        log << "Burn-in for " << burn_in << " successful steps..." << std::endl;
        
        const Time b_start_time = Clock::now();
        
        auto counts = T.FoldRandom(burn_in);
        
        const LInt attempt_count = counts.Total();
        
        steps_taken += attempt_count;
        
        const Time b_stop_time = Clock::now();
        
        const double timing = Tools::Duration( b_start_time, b_stop_time );
        
        log << "Time elapsed = " << timing << " s.\n" ;
        log << "Achieved " << Frac<Real>(attempt_count,timing) << " attempts per second.\n";
        log << "Achieved " << Frac<Real>(counts[0],timing) << " successes per second. \n";
        log << "Success rate = " << Frac<Real>(100 * counts[0],attempt_count) << " %.\n";
        
        log << std::endl;
    }

    // Buffer for the polygon coordinates.
    Tensor2<Real,Int> x ( n, AmbDim, Real(0) );
    
    // The Link_T object does the actual projection into the plane and the calculation of intersections. We can resuse it and load new vertex coordinates into it.
    Link_T L ( n );
    
    for( Int i = 0; i < N; ++i )
    {
        Time start_time;
                
        if( verboseQ )
        {
            log << "Sample " + StringWithLeadingZeroes(i,6) << std::endl;
            
            start_time = Clock::now();
        }

        
        // Do polygon folds until we have at least `skip` successes.
        auto counts = T.FoldRandom(skip);

        const LInt attempt_count = counts.Total();
        
        steps_taken += attempt_count;
        
        if( verboseQ )
        {
            log << "counts = " + ToString(counts) << std::endl;
        }

        T.WriteVertexCoordinates( x.data() );
        
        // Read coordinates into `Link_T` object `L`...
        L.ReadVertexCoordinates ( x.data() );

        L.FindIntersections();

        // ... and use it to initialize the planar diagram
        PD_T PD ( L );

        // And empty list of planar diagram to where Simplify5 can push the connected summands.
        std::vector<PD_T> PD_list;

        PD.Simplify5( PD_list );

        // Writing the PD codes to file.
        
        pds << "k";
        
        write_pd_code(pds,PD);
        
        for( auto & P : PD_list )
        {
            write_pd_code(pds,P);
        }
        
        pds << "\n";

        if( verboseQ )
        {
            const Time stop_time = Clock::now();
            
            const double timing = Tools::Duration( start_time, stop_time );
            
            // Writing a few statistics to log file.
            log << "Time elapsed = " << timing << " s.\n";
            
            log << "Achieved " << Frac<Real>(attempt_count,timing) << " attempts per second.\n";
            log << "Achieved " << Frac<Real>(counts[0],timing) << " successes per second. \n";
            log << "Success rate = " << Frac<Real>(100 * counts[0],attempt_count) << " %.\n";
            
            log << std::endl;
        }
    }
    
    const Time job_stop_time = Clock::now();
    
    // Writing some more statistics to log file.
    log << "Job done.\n";
    log << "Steps taken = " << ToString(steps_taken) << ".\n";
    log << "Time elapsed = " << Tools::Duration( job_start_time, job_stop_time ) << "." << std::endl;
    
    std::cout << "Done. Time elapsed = " << Tools::Duration( job_start_time, job_stop_time ) << "." << std::endl;
    
    return 0;
}

