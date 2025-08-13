private:

void HandleOptions( int argc, char** argv )
{
    namespace po = boost::program_options;
    
    // Declare all possible options with type information and a documentation string that will be printed when calling this routine with the -h [--help] option.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "Produce help message.")
    ("diam,d", po::value<Real>()->default_value(1), "Set hard sphere diameter.")
    ("edge-count,n", po::value<Int>(), "Set number of edges.")
    ("burn-in,b", po::value<LInt>(), "Set number attempts made for burn-in.")
    ("skip,s", po::value<LInt>(), "Set number of attempts made between samples.")
    ("sample-count,N", po::value<Int>(), "Set number of samples.")
    ("output,o", po::value<std::string>(), "Set output directory.")
    ("input,i", po::value<std::string>(), "Set input file.")
    ("tag,T", po::value<std::string>(), "Set a tag to append to output directory")
    ("extend,e", "extend name of output directory by information about the experiment, then append value of --tag option.")
    ("verbosity,v", po::value<int>()->default_value(1), "Set how much information should be printed to Log.txt file.")
    ("pcg-multiplier", po::value<std::string>(), "Specify 128 bit unsigned integer used by pcg64 for the \"multiplier\" (implementation dependent -- better not touch it unless you really know what you do).")
    ("pcg-increment", po::value<std::string>(), "Specify 128 bit unsigned integer used by pcg64 for the \"increment\" (every processor should have its own).")
    ("pcg-state", po::value<std::string>(), "Specify 128 bit unsigned integer used by pcg64 for the state (use this for seeding).")
    ("low-mem,m", "Force deallocation of large data structures; this will be a bit slower but peak memory will be less.")
    ("angles,a", "Compute statistics on curvature and torsion angles and report them in file \"Info.m\".")
    ("squared-gyradius,g", "Compute squared radius of gyration and report in file \"Info.m\".")
    ("pd-code,c", "Compute pd codes and print them to file \"PDCodes.tsv\".")
    ("gauss-code,G", "Compute extended Gauss codes and print them to file \"GaussCodes.txt\".")
    ("macleod-code,M", "Compute MacLeod codes and print them to file \"MacLeodCodes.txt\".")
    ("tally-unknots,u", po::value<bool>()->default_value(true), "Abbreviate runs of unknots when reporting PD/extended Gauss/MacLeod codes. If X is the number of unknots, write \"u X\" unstead of X consecutive \"k\"s.")
    ("tally-trefoils", po::value<bool>()->default_value(true), "Instead of writing out PD/extended Gauss/MacLeod codes for all trefoils in a connected sum just write \"T+ X\" and \"T- Y\", where X is the number of right-handed trefoils and Y is the number of left-handed trefoils.")
    ("tally-f8", po::value<bool>()->default_value(true), "Instead of writing out PD/extended Gauss/MacLeod codes for all figure-eight knots in a connected sum just write \"F8 X\", where X is the number of figure-eight knots.")
    ("bounding-boxes,B", "Compute statistics of bounding boxes and report them in file \"Info.m\".")
    ("polygons,P", po::value<LInt>()->default_value(-1), "Print every [arg] sample to file; if [arg] is negative, no samples are written to file ; if [arg] is 0, then only the polygon directly after burn-in and the final sample are written to file.")
    ("histograms", po::value<Int>()->default_value(0), "Create histograms for curvature and torsion angles with [arg] bins.")
    ("checks,C", po::value<bool>()->default_value(true), "Set whether to perform hard sphere collision checks.")
    ("check-joints,j", po::value<bool>()->default_value(true), "Set whether to check the joints before the pivot move (this can increase performance).")
    ("reflections,R", po::value<Real>()->default_value(0.5), "Set probability that a pivot move is orientation reversing.")
    ("gaussian-angles", po::value<Real>(), "Use the wrapped Gaussian distribution on [0,2 * pi) with standard deviation [arg] and center 0 - pi to generate pivot angles.")
    ("test-angles", po::value<LInt>()->default_value(0), "Create [arg] samples of the angle distribution and write them to TestAngles.tsv")
    ("gaussian-pivots", po::value<double>(), "Sample pivots by picking the first pivot p uniformly in {0,1,2,..,n-1}; then pick q = p + delta mod n, where delta is sampled according to the discretized wrapped Gaussian distribution on {0,1,2,..,n-1} with standard deviation [arg] and center 0. This is repeated until the distance of p to q mod n is greater than 1.")
    ("laplace-pivots", po::value<double>(), "Sample pivots by picking the first pivot p uniformly in {0,1,2,..,n-1}; then pick q = p + delta mod n, where delta is sampled according to the discretized wrapped Laplace distribution on {0,1,2,..,n-1} with scale parameter beta = [arg] and center 0. This is repeated until the distance of p to q mod n is greater than 1.")
    ("clisby-pivots", "Sample u uniformly on [1,n/2], then take pivot distance d = floor(2^u.")
    ("test-pivots", po::value<LInt>()->default_value(0), "Create [arg] samples of the pivot distribution and write them to TestPivots.tsv")
    ("hierarchical,H", po::value<bool>()->default_value(false), "Set whether to use hierarchical moves for burn-in and sampling (caution: this is a very experimental feature.")
    ("shift,S", po::value<bool>()->default_value(true), "Shift vertex indices randomly in each sample.")
    ("recenter,Z", po::value<bool>()->default_value(true), "Translate each sample so that its barycenter is the origin.")
    ("edge-length-tol", po::value<Real>()->default_value(0.00000000001), "Set relative tolerance for the edge lengths.")
    ;
    
    
    
    // Declare that arguments without option prefix are reinterpreted as if they were assigned to -o [--output].
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
        burn_in = vm["burn-in"].as<LInt>();
        
        valprint<a>("Burn-in Count b", burn_in);
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
    
    macleodQ = (vm.count("macleod-code") != 0);
    valprint<a>("Compute MacLeod Codes", BoolString(macleodQ) );
    
    tally_unknotsQ = vm["tally-unknots"].as<bool>();
    valprint<a>("Tally Unknots", BoolString(tally_unknotsQ) );
    
    tally_trefoilsQ = vm["tally-trefoils"].as<bool>();
    valprint<a>("Tally Trefoils", BoolString(tally_trefoilsQ) );
    
    tally_F8Q = vm["tally-f8"].as<bool>();
    valprint<a>("Tally Figure Eights", BoolString(tally_F8Q) );
    
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
    
    if( vm.count("gaussian-angles") && vm.count("reflections") && !vm["reflections"].defaulted()
    )
    {
        throw std::invalid_argument("The options \"--gaussian-angles\" and \"--reflections\" are mutually exclusive.");
    }
    
    reflection_probability = Clamp(vm["reflections"].as<Real>(),Real(0),Real(1));
    if( Clisby_T::quaternionsQ && (reflection_probability > Real(0)) )
    {
        wprint("Reflections cannot be activated while quaternions are used. Deactivating reflections");
        reflection_probability = 0;
    }
    valprint<a>("Reflection Probability",ToStringFPGeneral(reflection_probability));
    
    
    edge_length_tolerance = vm["edge-length-tol"].as<Real>() * Real(n);
    valprint<a>("Edge Length Tolerance", edge_length_tolerance);
    
    if( vm.count("gaussian-angles") && !vm.count("gaussian-angles") )
    {
        Real sigma = vm["gaussian-angles"].as<Real>();
        
        valprint<a>("Angle Random Method", "Wrapped Gaussian ( SD = " + ToStringFPGeneral(sigma) +")");
        angle_method = AngleRandomMethod_T::WrappedGaussian;
        angle_sigma = sigma;
    }
    else
    {
        valprint<a>("Angle Random Method", "Uniform");
        angle_method = AngleRandomMethod_T::Uniform;
    }
    test_angles = vm["test-angles"].as<LInt>();
    
    
    if( vm.count("gaussian-pivots") && vm.count("laplace-pivots") )
    {
        throw std::invalid_argument("The options \"--gaussian-pivots\" and \"--laplace-pivots\" are mutually exclusive.");
    }
    
    if( vm.count("gaussian-pivots") && vm.count("clisby-pivots") )
    {
        throw std::invalid_argument("The options \"--gaussian-pivots\" and \"--clisby-pivots\" are mutually exclusive.");
    }
    
    if( vm.count("laplace-pivots") && vm.count("clisby-pivots") )
    {
        throw std::invalid_argument("The options \"--laplace-pivots\" and \"--clisby-pivots\" are mutually exclusive.");
    }
    
    if( vm.count("gaussian-pivots") )
    {
        pivot_sigma = vm["gaussian-pivots"].as<double>();
        valprint<a>("Pivot Random Method", "Discrete Wrapped Gaussian ( SD = " + ToStringFPGeneral(pivot_sigma) + " )");
        pivot_method = PivotRandomMethod_T::DiscreteWrappedGaussian;
    }
    if( vm.count("laplace-pivots") )
    {
        pivot_beta = vm["laplace-pivots"].as<Real>();
        valprint<a>("Pivot Random Method", "Wrapped Laplace ( beta = " + ToStringFPGeneral(pivot_beta) + " )");
        pivot_method = PivotRandomMethod_T::DiscreteWrappedLaplace;
    }
    if( vm.count("clisby-pivots") )
    {
        valprint<a>("Pivot Random Method", "Clisby");
        pivot_method = PivotRandomMethod_T::Clisby;
    }
    else
    {
        valprint<a>("Pivot Random Method", "Uniform");
        pivot_method = PivotRandomMethod_T::Uniform;
    }
    test_pivots = vm["test-pivots"].as<LInt>();
    
    hierarchicalQ = vm["hierarchical"].as<bool>();
    valprint<a>("Hierarchical Moves", BoolString(hierarchicalQ) );
    
    shiftQ = vm["shift"].as<bool>();
    valprint<a>("Shift Indices", BoolString(shiftQ) );
    
    recenterQ = vm["recenter"].as<bool>();
    valprint<a>("Recenter", BoolString(recenterQ) );

    checksQ = vm["checks"].as<bool>();
    valprint<a>("Hard Sphere Checks", BoolString(checksQ) );
    
    check_jointsQ = vm["check-joints"].as<bool>();
    valprint<a>("Joint Checks", BoolString(check_jointsQ) );
    
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
                    + "-b" + ToString(burn_in)
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
             ClassName()+"::Initialize: Failed to create file \"" + Profiler::log_file.string() + "\"."
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
                ClassName()+"::HandleOptions: Failed to read input file."
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
    
    
    if( !(squared_gyradiusQ || pdQ || gaussQ || macleodQ || anglesQ || bounding_boxesQ || (bin_count > Int(1)) || (steps_between_print > LInt(0)) ) )
    {
        throw std::runtime_error("Not computing anything. Use the command line flags -c, -g, -P, -a, -B, or -histograms to define outputs.");
    }
    
}
