#include <iostream>
#include <iterator>
#include <boost/program_options.hpp>
#include <exception>

#include "../KnotTools.hpp"

using namespace KnotTools;
using namespace Tensors;
using namespace Tools;

using Real = Real64;
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
    std::cout << "Welcome to PolyFold.\n\n";
    
    std::cout << "Vector extensions " << ( vec_enabledQ ? "enabled" : "disabled" ) << ".\n";
    std::cout << "Matrix extensions " << ( mat_enabledQ ? "enabled" : "disabled" ) << ".\n";
    std::cout << "\n";
    
    Int thread_count = 1;
    Int job_count    = thread_count;
    
    Int  n = 1;
    
    LInt N = 1;
    LInt burn_in_success_count = 1;
    LInt skip    = 1;
    
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
        ("threads,t", po::value<Int>()->default_value(1), "set number of threads")
        ("jobs,j", po::value<Int>(), "set number of jobs (time series)")
        ("edge-count,n", po::value<Int>(), "set number of edges")
        ("burn-in,b", po::value<LInt>(), "set number of burn-in steps")
        ("skip,s", po::value<LInt>(), "set number of steps skipped between samples")
        ("sample-count,N", po::value<Int>(), "set number of samples")
        ("output,o", po::value<std::string>(), "set output directory")
        ("tag,T", po::value<std::string>(), "set a tag to append to output directory")
        ("extend,e", "extend name of output directory by information about the experiment, then append value of --tag option")
        ("verbose,v", "gather statistics for each sample")
        ("append,a", "append to existent file(s)")
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
            
            std::cout << "Number of jobs set to " << job_count << ".\n";
        }
        else
        {
            job_count = thread_count;
            std::cout << "Using default number of jobs (" << job_count << ").\n";
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
            burn_in_success_count = vm["burn-in"].as<LInt>();
            
            std::cout << "Number of burn-in steps set to "
            << burn_in_success_count << ".\n";
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
        
        if( vm.count("output") )
        {
            if( vm.count("extend") )
            {
                path = std::filesystem::path( vm["output"].as<std::string>()
                     + "__n_" + ToString(n)
                     + "__b_" + ToString(burn_in_success_count)
                     + "__s_" + ToString(skip)
                     + "__N_" + ToString(N)
                     + (vm.count("tag") ? ("__" + vm["tag"].as<std::string>()) : "")
                );
            }
            else
            {
                path = std::filesystem::path( vm["output"].as<std::string>()
                     + (vm.count("tag") ? ("__" + vm["tag"].as<std::string>()) : "")
                );
            }
            
            std::cout << "Output path set to "
            << path << ".\n";
        }
        else
        {
            throw std::invalid_argument( "Output path unspecified. Use the option -o to set it." );
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
    
    
    // Make sure that the working directory exists.
    try{
        std::filesystem::create_directories(path);
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
        
    std::cout << "\n";
    std::cout << "Using sampler of class " << Clisby_T::ClassName() << "\n";
    std::cout << "\n";
    std::cout << "Start sampling." << std::endl;
    
    const Time program_start_time = Clock::now();
    
    ParallelDo(
        [&]( const Int job )
        {
            std::string job_name = (job_count == 1) ? "" : std::string("Job_") + StringWithLeadingZeroes(job,6) + "_";
            std::filesystem::path log_file = path / (job_name + "Log.txt");
            std::filesystem::path pds_file = path / (job_name + "PDCodes.tsv");
            
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
                return;
            }
            catch(...)
            {
                std::cerr << "Exception of unknown type!\n";
                return;
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
                return;
            }
            catch(...)
            {
                std::cerr << "Exception of unknown type!\n";
                return;
            }
            
            Clisby_T T ( n, Real(1) );
            
            log << "Using class " << T.ClassName() << std::endl;
            log << "Vector extensions " << ( vec_enabledQ ? "enabled" : "disabled" ) << ".\n";
            log << "Matrix extensions " << ( mat_enabledQ ? "enabled" : "disabled" ) << ".\n";
            
            log << "\n";
            
            // Stream data to the file.
            log << "n = " << n << "\n";
            log << "N = " << N << "\n";
            log << "burn-in = " << burn_in_success_count << "\n";
            log << "skip = " << skip << "\n";
            
            // std::endl adds a "\n" and flushes the string buffer so that the string is actually written to file.
            log << "\n " << std::endl;

            const Time job_start_time = Clock::now();
            
            LInt total_attempt_count = 0;
            LInt burn_in_attempt_count = 0;
            
            // Buffer for the polygon coordinates.
            Tensor2<Real,Int> x ( n, AmbDim, Real(0) );
            
            // Burn-in.
            {
                log << "Burn-in for " << burn_in_success_count << " successful steps..." << std::endl;
                
                const Time b_start_time = Clock::now();
                
                auto counts = T.FoldRandom(burn_in_success_count);
                
                const LInt attempt_count = counts.Total();
                
                total_attempt_count += attempt_count;
                
                burn_in_attempt_count = attempt_count;
                
                const Time b_stop_time = Clock::now();
                
                const double timing = Tools::Duration( b_start_time, b_stop_time );
                
                log << "Burn-in done.\n";
                log << "\n";
                log << "Time elapsed   = " << timing << " s.\n" ;
                log << "Attempts made  = " << burn_in_attempt_count << ".\n";
                log << "Attempt speed  = " << Frac<Real>(attempt_count,timing) << "/s.\n";
                log << "Successes made = " << burn_in_success_count << ".\n";
                log << "Success speed  = " << Frac<Real>(counts[0],timing) << "/s. \n";
                log << "Success rate   = " << Percentage<Real>(counts[0],attempt_count) << " %.\n";
                
                log << "\n";
                
                {
                    T.WriteVertexCoordinates( x.data() );
                    
                    auto [min_dev,max_dev] = T.MinMaxEdgeLengthDeviation( x.data() );
                    
                    log << "Lower relative edge length deviation = " << ToString(min_dev) << ".\n";
                    log << "Upper relative edge length deviation = " << ToString(max_dev) << ".\n";
                }
                
                log << "\n";
                log << std::endl;
            }
            
            // The Link_T object does the actual projection into the plane and the calculation of intersections. We can resuse it and load new vertex coordinates into it.
            Link_T L ( n );
            
            log << "Sampling for " << (N * skip) << " successful steps...\n" << std::endl;
            
            const Time sample_start_time = Clock::now();
            
            for( LInt i = 0; i < N; ++i )
            {
                Time start_time;
                
                if( verboseQ )
                {
                    log << "Sample " << i << "\n";
                    
                    start_time = Clock::now();
                }
                
                // Do polygon folds until we have at least `skip` successes.
                auto counts = T.FoldRandom(skip);
                
                const LInt attempt_count = counts.Total();
                
                total_attempt_count += attempt_count;
                
                if( verboseQ )
                {
                    log << "counts = " + ToString(counts) << "\n";
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
                
                pds << pd_code_string(PD);
                
                for( auto & P : PD_list )
                {
                    pds << pd_code_string(P);
                }
                
                pds << "\n";
                
                if( verboseQ )
                {
                    const Time stop_time = Clock::now();
                    
                    const double timing = Tools::Duration( start_time, stop_time );
                    
                    auto [min_dev,max_dev] = T.MinMaxEdgeLengthDeviation( x.data() );
                    
                    // Writing a few statistics to log file.
                    log << "Time elapsed   = " << timing << " s.\n";
                    log << "Attempts made  = " << attempt_count << ".\n";
                    log << "Attempt speed  = " << Frac<Real>(attempt_count,timing) << "/s.\n";
                    log << "Successes made = " << skip << ".\n";
                    log << "Success speed  = " << Frac<Real>(counts[0],timing) << "/s. \n";
                    log << "Success rate   = " << Percentage<Real>(counts[0],attempt_count) << " %.\n";
                    
                    log << "\n";
                    
                    log << "Overall number of attempts = " << total_attempt_count << ".\n";
                    log << "Overall success rate (without burnin) = "
                        << Percentage<Real>( skip * (i+1), total_attempt_count - burn_in_attempt_count)
                        << " %.\n";
                    
                    log << "\n";
                    
                    log << "Lower relative edge length deviation = " << ToString(min_dev) << ".\n";
                    log << "Upper relative edge length deviation = " << ToString(max_dev) << ".\n";
                    
                    log << "\n";
                    log << "\n";
                }
            }
            
            const Time sample_stop_time = Clock::now();
            
            const Time job_stop_time = Clock::now();
            
            const double sample_timing  = Tools::Duration( sample_start_time, sample_stop_time );
            const double job_timing = Tools::Duration( job_start_time,job_stop_time    );
            
            const LInt sample_attempt_count = total_attempt_count - burn_in_attempt_count;
            const LInt sample_success_count = skip * N;
            
            const LInt total_success_count = burn_in_success_count + sample_success_count;
            
            log << "Sampling done.\n";
            
            log << "\n";
            
            log << "Sampling statistics\n";
            
            log << "Time elapsed   = "
                << sample_timing
                << " s.\n";
            
            log << "Attempts made  = "
                << sample_attempt_count
                << ".\n";
            
            log << "Attempt speed  = "
                << Frac<Real>(sample_attempt_count,sample_timing)
                << "/s.\n";
            
            log << "Successes made = "
                << sample_success_count
                << ".\n";
            
            log << "Success speed  = "
                << Frac<Real>(sample_success_count,sample_timing)
                << "/s. \n";
            
            log << "Success rate   = "
                << Percentage<Real>(sample_success_count,sample_attempt_count)
                << " %.\n";
            
            log << "\n";
            
            log << "Overall statistics\n";
            
            log << "Time elapsed   = "
                << job_timing
                << " s.\n";

            log << "Attempts made  = "
                << total_attempt_count
                << ".\n";
            
            log << "Attempt speed  = "
                << Frac<Real>(total_attempt_count,job_timing)
                << "s.\n";
            
            log << "Successes made = "
                << total_success_count
                << ".\n";
            
            log << "Success speed  = "
                << Frac<Real>(total_success_count,job_timing)
                << "s.\n";
            
            log << "Success rate   = "
                << Percentage<Real>(total_success_count,total_attempt_count)
                << " %.\n";
            
            log << "\n";
            
            T.WriteVertexCoordinates( x.data() );
            
            {
                auto [min_dev,max_dev] = T.MinMaxEdgeLengthDeviation( x.data() );
                
                log << "Lower relative edge length deviation = " << ToString(min_dev) << ".\n";
                log << "Upper relative edge length deviation = " << ToString(max_dev) << ".\n";
            }
        },
        thread_count,
        job_count
   );
    
    
    const Time program_stop_time = Clock::now();
    
    std::cout << "Done. Time elapsed = " << Tools::Duration(program_start_time,program_stop_time) << " s." << std::endl;
    
    return 0;
}
