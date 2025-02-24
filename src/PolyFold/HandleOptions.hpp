private:

void HandleOptions( int argc, char** argv )
{
    namespace po = boost::program_options;
    
    // `try` executes some code; `catch` catches exceptions and handles them.
    try
    {
        // Declare all possible options with type information and a documentation string that will be printed when calling this routine with the -h [--help] option.
        po::options_description desc("Allowed options");
        desc.add_options()
        ("help,h", "produce help message")
        ("edge-count,n", po::value<Int>(), "set number of edges")
        ("burn-in,b", po::value<LInt>(), "set number of burn-in steps")
        ("skip,s", po::value<LInt>(), "set number of steps skipped between samples")
        ("sample-count,N", po::value<Int>(), "set number of samples")
        ("output,o", po::value<std::string>(), "set output directory")
        ("tag,T", po::value<std::string>(), "set a tag to append to output directory")
        ("extend,e", "extend name of output directory by information about the experiment, then append value of --tag option")
        ("verbosity,v", po::value<int>(), "how much information should be printed to Log.txt file.")
//        ("seed,S", po::value<std::string>(), "a seed string consisting of exactly 32 hexadecimal digits.")
        ("pcg-multiplier", po::value<std::string>(), "specify 128 bit unsigned integer used by pcg64 for the \"multiplier\" (implementation dependent -- better not touch it unless you really know what you do)")
        ("pcg-increment", po::value<std::string>(), "specify 128 bit unsigned integer used by pcg64 for the \"increment\" (every processor should have its own)")
        ("pcg-state", po::value<std::string>(), "specify 128 bit unsigned integer used by pcg64 for the state (use this for seeding)")
        ("low-mem,m", "force deallocation of large data structures; this will be a bit slower but peak memory will be less")
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
            std::stringstream s;
            s << desc;
            print(s.str());
            exit(0);
        }
        
        if( vm.count("edge-count") )
        {
            n = vm["edge-count"].as<Int>();
            
            print("Number of edges set to " + ToString(n) + ".");
        }
        else
        {
            throw std::invalid_argument( "Number of edges unspecified. Use the option -n to set it." );
        }
        
        if( vm.count("sample-count") )
        {
            N = vm["sample-count"].as<Int>();
            
            print("Number of samples set to " + ToString(N) + ".");
        }
        else
        {
            throw std::invalid_argument( "Number of samples unspecified. Use the option -N to set it." );
        }
        
        if( vm.count("burn-in") )
        {
            burn_in_accept_count = vm["burn-in"].as<LInt>();
            
            print("Number of burn-in steps set to " + ToString(burn_in_accept_count) + ".");
        }
        else
        {
            throw std::invalid_argument( "Number of burn-in steps unspecified. Use the option -b to set it." );
        }
        
        if( vm.count("skip") )
        {
            skip = vm["skip"].as<LInt>();
            
            print("Number of skip steps set to " + ToString(skip) + ".");
        }
        else
        {
            throw std::invalid_argument( "Number of skip unspecified. Use the option -s to set it." );
        }
        
        if( vm.count("verbosity") )
        {
            verbosity = vm["verbosity"].as<int>();
        }
        
        if( vm.count("low-mem") )
        {
            force_deallocQ = true;
        }
        else
        {
            force_deallocQ = false;
        }
        
        print( std::string("Forced deallocation set to ") + ToString(force_deallocQ) + "." );

        if( vm.count("pcg-multiplier") )
        {
            random_engine_multiplierQ = true;
            
            random_engine_state.multiplier = vm["pcg-multiplier"].as<std::string>();
        }
        
        if( vm.count("pcg-increment") )
        {
            random_engine_incrementQ = true;
            
            random_engine_state.increment = vm["pcg-increment"].as<std::string>();
        }
        
        if( vm.count("pcg-state") )
        {
            random_engine_stateQ = true;
            
            random_engine_state.state = vm["pcg-state"].as<std::string>();
        }
        
        print(std::string("Report mode set to ") + ToString(verbosity) + ".");
        
        if( vm.count("output") )
        {
            if( vm.count("extend") )
            {
                path = std::filesystem::path( vm["output"].as<std::string>()
                    + "__n_" + ToString(n)
                    + "__b_" + ToString(burn_in_accept_count)
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
            
            print( std::string("Output path set to ") + path.string() + "." );
        }
        else
        {
            throw std::invalid_argument( "Output path unspecified. Use the option -o to set it." );
        }
        
    }
    catch( std::exception & e )
    {
        eprint(e.what());
        exit(1);
    }
    catch(...)
    {
        eprint("Exception of unknown type!");
        exit(1);
    }
    
    
    // Make sure that the working directory exists.
    try{
        std::filesystem::create_directories(path);
    }
    catch( std::exception & e )
    {
        eprint(e.what());
        exit(1);
    }
    catch(...)
    {
        eprint("Exception of unknown type!");
        exit(1);
    }
}
