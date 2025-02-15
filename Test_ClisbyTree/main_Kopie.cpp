#include <iostream>
#include <iterator>
#include <boost/program_options.hpp>
#include <exception>

//#define TOOLS_DEACTIVATE_VECTOR_EXTENSIONS
//#define TOOLS_DEACTIVATE_MATRIX_EXTENSIONS

//#define TOOLS_AGGRESSIVE_INLINING
//#define TOOLS_HOT_CODE

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

using Link_T   = Link_2D<Real,Int,Int8>;
using PD_T     = PlanarDiagram<Int>;
using Clisby_T = ClisbyTree<AmbDim,Real,Int,LInt>;

namespace po = boost::program_options;

std::string pd_code_string( mref<PD_T> PD )
{
    if( PD.CrossingCount() <= 0 )
    {
        return std::string();
    }
    
    auto pdcode = PD.PDCode();
    
    cptr<Int> a = pdcode.data();
    
    std::string s;
    
    s += "\ns ";
    s += PD.ProvablyMinimalQ();
    
    for( Int i = 0; i < pdcode.Dimension(0); ++i )
    {
        s += VectorString<5>(&a[5 * i + 0], "\n", "\t", "" );
    }
    
    return s;
}

int main( int argc, char** argv )
{
    print("Welcome to PolyFold.");
    print("");
    
    if( vec_enabledQ )
    {
        print("Vector extensions enabled.");
    }
    else
    {
        print("Vector extensions disabled.");
    }
    
    if( mat_enabledQ )
    {
        print("Matrix extensions enabled.");
    }
    else
    {
        print("Matrix extensions disabled.");
    }
    
    print("");
    
    
    Int thread_count = 1;
    Int job_count    = thread_count;
    
    Int  n = 100000;
    Int  N = 100;

    LInt  burn_in = 1000000;
    LInt  skip    = 100000;
    
    std::filesystem::path path;
    
    // `try` executes some code; `catch` catches exceptions and handles them.
    try
    {
        // Declare all possible options with type information and a documentation string that will be printed when calling this routine with the -h [--help] option.
        po::options_description desc("Allowed options");
        desc.add_options()
        ("help,h", "produce help message")
        ("threads,t", po::value<Int>(), "set number of threads")
        ("jobs,j", po::value<Int>(), "set number of jobs (time series)")
        ("edge-count,n", po::value<Int>(), "set number of edges")
        ("burn-in,b", po::value<LInt>(), "set number of burn-in steps")
        ("skip,s", po::value<LInt>(), "set number of steps skipped between samples")
        ("sample-count,N", po::value<Int>(), "set number of samples")
        ("output,o", po::value<std::string>(), "set output directory")
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
        
        if( vm.count("threads") )
        {
            thread_count = vm["threads"].as<Int>();
            
            std::cout << "Number of threads set to "
            << thread_count << ".\n";
        }
        else
        {
            std::cout << "Using default number of threads (" << ToString(thread_count) + ").\n";
        }
        
        if( vm.count("jobs") )
        {
            job_count = vm["jobs"].as<Int>();
            
            std::cout << "Number of jobs set to "
            << job_count << ".\n";
        }
        else
        {
            job_count = thread_count;
            std::cout << "Using default number of jobs (" << ToString(thread_count) + ").\n";
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
    }
    catch( std::exception & e )
    {
        std::cerr << "error: " << e.what() << "\n";
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
    catch(...)
    {
        std::cerr << "Failed to create directory " << path << std::endl;
        return 1;
    }
    
    // Use this path for profiles and general log files.
    Profiler::Clear(path);
    
    print("Start sampling.");
    
    ParallelDo(
        [&]( const Int job )
        {
            const Time job_start_time = Clock::now();
            
            std::filesystem::path local_path = path / ("Job_" + StringWithLeadingZeroes(job,6));
            
            // Make sure that the working directory for current job exists.
            std::filesystem::create_directory( local_path );
            
            std::filesystem::path log_file = local_path / "Log.txt";
            
            // Open file as stream.
            std::ofstream log ( log_file );
            
            // Stream data to the file.
            log << "n = " << n << "\n";
            log << "N = " << N << "\n";
            log << "burn-in = " << burn_in << "\n";
            log << "skip = " << skip << "\n";
            
            // std::endl adds a "\n" and flushes the string buffer so that the string is actually written to file.
            log << "\n " << std::endl;
            
            
            Clisby_T T ( n, Real(1) );
        
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
                log << "Sample " + StringWithLeadingZeroes(i,6) << std::endl;
                
                const Time start_time = Clock::now();
        
                
                // Do polygon folds until we have at least `skip` successes.
                auto counts = T.FoldRandom(skip);

                const LInt attempt_count = counts.Total();
                
                steps_taken += attempt_count;
                
                log << "counts = " + ToString(counts) << std::endl;
        
                T.WriteVertexCoordinates( x.data() );
                
//                // Open file as stream.
//                std::ofstream p_file (
//                    local_path / (std::string("Polygon_") + StringWithLeadingZeroes(i,6) + ".tsv")
//                );
//
//                // Write the polygon coordinates to file.
//                p_file << MatrixStringTSV(
//                    x.Dimension(0), x.Dimension(1), x.data(), x.Dimension(1)
//                );
//
//                // The file stream will automatically be closed by the desctructor of the class `std::ofstream` once the symbol `p_file` goes out of scope -- one of the pleasantries of C++.
        
                // Read coordinates into `Link_T` object `L`...
                L.ReadVertexCoordinates ( x.data() );
        
                L.FindIntersections();
        
                // ... and use it to initialize the planar diagram
                PD_T PD ( L );
        
                // And empty list of planar diagram to where Simplify5 can push the connected summands.
                std::vector<PD_T> PD_list;
        
                PD.Simplify5( PD_list );
        
                // Writing the PD codes to file.
                std::ofstream pds (
                    local_path / (std::string("PDCodes_") + StringWithLeadingZeroes(i,6) + ".m")
                );
                
                // Writing the PD codes to file.
                
                pds << "k";
                
                pds << pd_code_string(PD);
                
                for( auto & P : PD_list )
                {
                    pds << pd_code_string(P);
                }
                
                pds << "\n";
        
                const Time stop_time = Clock::now();
                
                const double timing = Tools::Duration( start_time, stop_time );
                
                // Writing a few statistics to log file.
                log << "Time elapsed = " << timing << " s.\n";
                
                log << "Achieved " << Frac<Real>(attempt_count,timing) << " attempts per second.\n";
                log << "Achieved " << Frac<Real>(counts[0],timing) << " successes per second. \n";
                log << "Success rate = " << Frac<Real>(100 * counts[0],attempt_count) << " %.\n";
                
                log << std::endl;
            }
            
            const Time job_stop_time = Clock::now();
            
            // Writing some more statistics to log file.
            log << "Job done.\n";
            log << "Steps taken = " << ToString(steps_taken) << ".\n";
            log << "Time elapsed = " << Tools::Duration( job_start_time, job_stop_time ) << "." << std::endl;
            
            print( std::string("Done with job ") + ToString(job) + ". Time elapsed = " + ToString(Tools::Duration( job_start_time, job_stop_time )) + "." );
        },
        job_count,
        thread_count
    );
    
    return 0;
}
