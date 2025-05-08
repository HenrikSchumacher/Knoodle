private:

void HandleOptions( int argc, char** argv )
{
    namespace po = boost::program_options;
    
    // Declare all possible options with type information and a documentation string that will be printed when calling this routine with the -h [--help] option.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "produce help message")
    ("diam,d", po::value<Real>()->default_value(1), "set hard sphere diameter")
    ("edge-count,n", po::value<Int>(), "set number of edges")
    ("burn-in,b", po::value<LInt>(), "set number of burn-in steps")
    ("skip,s", po::value<LInt>(), "set number of steps skipped between samples")
    ("sample-count,N", po::value<Int>(), "set number of samples")
    ("output,o", po::value<std::string>(), "set output directory")
    ("input,i", po::value<std::string>(), "set input file")
    ("tag,T", po::value<std::string>(), "set a tag to append to output directory")
    ("extend,e", "extend name of output directory by information about the experiment, then append value of --tag option")
    ("verbosity,v", po::value<int>()->default_value(1), "how much information should be printed to Log.txt file.")
    ("pcg-multiplier", po::value<std::string>(), "specify 128 bit unsigned integer used by pcg64 for the \"multiplier\" (implementation dependent -- better not touch it unless you really know what you do)")
    ("pcg-increment", po::value<std::string>(), "specify 128 bit unsigned integer used by pcg64 for the \"increment\" (every processor should have its own)")
    ("pcg-state", po::value<std::string>(), "specify 128 bit unsigned integer used by pcg64 for the state (use this for seeding)")
    ("low-mem,m", "force deallocation of large data structures; this will be a bit slower but peak memory will be less")
    ("angles,a", "compute statistics on curvature and torsion angles and report them in file \"Info.m\"")
    ("squared-gyradius,g", "compute squared radius of gyration and report in file \"Info.m\"")
    ("pd-code,c", "compute pd codes and print them to file \"PDCodes.tsv\"")
    ("gauss-code,G", "compute extended Gauss codes and them print to file \"GaussCodes.txt\"")
    ("bounding-boxes,B", "compute statistics of bounding boxes and report them in file \"Info.m\"")
    ("polygons,P", po::value<LInt>()->default_value(-1), "print every [arg] sample to file; if [arg] is negative, no samples are written to file ; if [arg] is 0, then only the polygon directly after burn-in and the final sample are written to file")
    ("histograms", po::value<Int>()->default_value(0), "create histograms for curvature and torsion angles with [arg] bins")
    ("checks,C", po::value<bool>()->default_value(true), "whether to perform hard sphere collision checks")
    ("reflections,R", po::value<Real>()->default_value(0.5), "probability that a pivot move is orientation reversing")
    ("hierarchical,H", po::value<bool>()->default_value(false), "whether to use hierarchical moves for burn-in and sampling")
    ("shift,S", po::value<bool>()->default_value(true), "shift vertex indices randomly in each sample")
    ("recenter,Z", po::value<bool>()->default_value(true), "translate each sample so that its barycenter is the origin")
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
        throw; // TODO: Need to find an elegant way to exit here.
    }
    
    
    valprint<a>("Use Quaternions", BoolString(Clisby_T::quaternionsQ) );
    print("");
    
    hard_sphere_diam = vm["diam"].as<Real>();
    valprint<a>("Hard Sphere Diameter d", hard_sphere_diam);
    
    if( vm.count("edge-count") )
    {
        n = vm["edge-count"].as<Int>();
        
        edge_length_tolerance = Real(0.00000000001) * Real(n);
        
        valprint<a>("Edge Count n", n);
    }
    else
    {
        throw std::invalid_argument( "Number of edges unspecified. Use the option -n to set it." );
    }
    
    if( vm.count("burn-in") )
    {
        burn_in_accept_count = vm["burn-in"].as<LInt>();
        
        valprint<a>("Burn-in Count b", burn_in_accept_count);
    }
    else
    {
        throw std::invalid_argument(
            "Number of burn-in steps unspecified. Use the option -b to set it."
        );
    }
    
    if( vm.count("skip") )
    {
        skip = vm["skip"].as<LInt>();
        
        valprint<a>("Skip Count s", skip);
    }
    else
    {
        throw std::invalid_argument(
            "Number of skip unspecified. Use the option -s to set it."
        );
    }
    
    if( vm.count("sample-count") )
    {
        N = vm["sample-count"].as<Int>();
        
        valprint<a>("Sample Count N", N);
    }
    else
    {
        throw std::invalid_argument(
            "Number of samples unspecified. Use the option -N to set it."
        );
    }
    
    print("");
    
    anglesQ = (vm.count("angles") != 0);
    valprint<a>("Compute Angle Statistics", BoolString(anglesQ) );

    squared_gyradiusQ = (vm.count("squared-gyradius") != 0);
    valprint<a>("Compute Squared Gyradius", BoolString(squared_gyradiusQ) );
    
    bounding_boxesQ = (vm.count("bounding-boxes") != 0);
    valprint<a>("Compute Bounding Boxes", BoolString(bounding_boxesQ) );
    
    pdQ = (vm.count("pd-code") != 0);
    valprint<a>("Compute PD Codes", BoolString(pdQ) );
    
    gaussQ = (vm.count("gauss-code") != 0);
    valprint<a>("Compute Gauss Codes", BoolString(gaussQ) );
    
    steps_between_print = vm["polygons"].as<LInt>();
    printQ = (steps_between_print > LInt(0));
    valprint<a>("Polygon Snapshot Skip", ToString(steps_between_print) );
    
    bin_count = Ramp( vm["histograms"].as<Int>() );
    curvature_hist = Tensor1<LInt,Int> ( bin_count, 0 );
    torsion_hist   = Tensor1<LInt,Int> ( Int(2) * bin_count, 0 );
    valprint<a>("Histogram Bin Count", ToString(bin_count) );

    print("");
    
    force_deallocQ = (vm.count("low-mem") != 0);
    valprint<a>("Forced Deallocation", BoolString(force_deallocQ) );
    
    reflection_probability = Clamp(vm["reflections"].as<Real>(),Real(0),Real(1));
    if( Clisby_T::quaternionsQ && (reflection_probability > Real(0)) )
    {
        wprint("Reflections cannot be activated while quaternions are used. Deactivating reflections");
        
        reflection_probability = 0;
    }
    valprint<a>("Reflection Probability",ToStringFPGeneral(reflection_probability));
    
    hierarchicalQ = vm["hierarchical"].as<bool>();
    valprint<a>("Hierarchical Moves", BoolString(hierarchicalQ) );

    shiftQ = vm["shift"].as<bool>();
    valprint<a>("Shift Indices", BoolString(shiftQ) );
    
    recenterQ = vm["recenter"].as<bool>();
    valprint<a>("Recenter", BoolString(recenterQ) );

    checksQ = vm["checks"].as<bool>();
    valprint<a>("Hard Sphere Checks", BoolString(checksQ) );
    
    if( vm.count("pcg-multiplier") )
    {
        prng_multiplierQ = true;
        
        prng_init.multiplier = vm["pcg-multiplier"].as<std::string>();
        
        valprint("PCG Multiplier", prng_init.multiplier);
    }
    
    if( vm.count("pcg-increment") )
    {
        prng_incrementQ = true;
        
        prng_init.increment = vm["pcg-increment"].as<std::string>();
        
        valprint<a>("PCG Increment", prng_init.increment);
    }
    
    if( vm.count("pcg-state") )
    {
        prng_stateQ = true;
        
        prng_init.state = vm["pcg-state"].as<std::string>();
        
        valprint<a>("PCG State", prng_init.state);
    }
    
    print("");
    
    verbosity = vm["verbosity"].as<int>();
    valprint<a>("Verbosity", verbosity);
    
    if( vm.count("output") )
    {
        if( vm.count("extend") )
        {
                path = std::filesystem::path( vm["output"].as<std::string>()
                    + "-H" + ToString(hierarchicalQ)
                    + "-d" + ToStringFPGeneral(hard_sphere_diam)
                    + "-n" + ToString(n)
                    + "-b" + ToString(burn_in_accept_count)
                    + "-s" + ToString(skip)
                    + "-N" + ToString(N)
                    + (vm.count("tag") ? ("_" + vm["tag"].as<std::string>()) : "")
                );
        }
        else
        {
            path = std::filesystem::path( vm["output"].as<std::string>()
                + (vm.count("tag") ? ("_" + vm["tag"].as<std::string>()) : "")
            );
        }
    }
    else
    {
        throw std::invalid_argument( "Output path unspecified. Use the option -o to set it." );
    }
    
    
    // Make sure that the working directory exists.
    std::filesystem::create_directories(path);
    
    valprint<a>("Output Path", path.string());
    
    // Use this path for profiles and general log files.
    Profiler::Clear(path,true);
    
    if( !Profiler::log )
    {
        throw std::runtime_error(
             ClassName() + "::Initialize: Failed to create file \"" + Profiler::log_file.string() + "\"."
        );
    }
    
    if( vm.count("input") )
    {
        input_file = std::filesystem::path ( vm["input"].as<std::string>() );
        
        x = PolygonContainer_T( n, AmbDim );
        
        Int flag = x.ReadFromFile( input_file );
        
        if( flag != 0 )
        {
            throw std::invalid_argument(
                ClassName() + "::HandleOptions: Failed to read input file."
            );
        }
        
        inputQ = true;
        
        valprint<a>("Input file", input_file.string());
    }
    else
    {
        x = Tensor2<Real,Int>( n, AmbDim );
    }
    
    print("");
    
    
    if( !(squared_gyradiusQ || pdQ || gaussQ || anglesQ || bounding_boxesQ || (bin_count > Int(1)) || (steps_between_print > LInt(0)) ) )
    {
        throw std::runtime_error("Not computing anything. Use the command line flags -c, -g, -P, -a, -B, or -histograms to define outputs.");
    }
    
}
