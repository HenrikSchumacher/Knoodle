private:

template<Size_T t0>
void Initialize()
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t1 + 1;
    
    log_file = path / "Info.m";
    pds_file = path / "PDCodes.tsv";
    
    log.open( log_file, std::ios_base::out );
    pds.open( pds_file, std::ios_base::out );
    
    // Use this path for profiles and general log files.
    Profiler::Clear(path,true);
    
    log << Tabs<t0> + "<|";
    
    log << kv<t1,0>("n",n);
    log << kv<t1>  ("N",N);
    log << kv<t1>  ("Burn-In",burn_in_accept_count);
    log << kv<t1>  ("Skip",skip);
    log << kv<t1>  ("Radius",radius);
    log << kv<t1>  ("Prescribed Edge Length",Real(1));
    
    log << ",\n" + Tabs<t1> + "\"Initialization\" -> <|";
    
    log << kv<t1,0>("PolyFold Class",ClassName());
    
    log << kv<t2>  ("Vector Extensions Enabled",vec_enabledQ);
    log << kv<t2>  ("Matrix Extensions Enabled",mat_enabledQ);
    log << kv<t2>  ("Forced Deallocation Enabled",force_deallocQ);
    
    x = Tensor2<Real,Int>( n, AmbDim );
    
    log << kv<t2>("Creation of Coordinate Buffer Succeeded",true) << std::flush;

    {
        log << kv<t2>("Clisby Tree Class",Clisby_T::ClassName()) << std::flush;
        
        Clisby_T T ( n, radius );
        
        PRNG_FullState_T full_state = T.RandomEngineFullState();

        if( random_engine_multiplierQ )
        {
            full_state.multiplier = random_engine_state.multiplier;
        }
        
        if( random_engine_incrementQ )
        {
            full_state.increment = random_engine_state.increment;
        }
        
        if( random_engine_stateQ )
        {
            full_state.state = random_engine_state.state;
        }
        
        random_engine_state = full_state;
        
        if( random_engine_multiplierQ || random_engine_incrementQ || random_engine_stateQ)
        {
           if( T.SetRandomEngine(random_engine_state) )
           {
               eprint( ClassName() + "::Initialize: Failed to initialize random engine with "
                      + "multiplier = " + full_state.multiplier + ", "
                      + "increment = " + full_state.increment + ", "
                      + "state = " + full_state.state + ". Aborting."
                );
               exit(1);
           }
        }
                
        log << kv<t2>("Clisby Tree Byte Count",T.ByteCount());
        log << kv<t2>("PCG64 Multiplier",random_engine_state.multiplier);
        log << kv<t2>("PCG64 Increment",random_engine_state.increment);
        log << kv<t2>("PCG64 State",random_engine_state.state) << std::flush;
        
        T.WriteVertexCoordinates( x.data() );
        
        // Remember random engine for later use.
        random_engine = T.GetRandomEngine();
        
        if( force_deallocQ )
        {
            T = Clisby_T();
        }
    }
    log << kv<t2>("Creation of Clisby Tree Succeeded",true) << std::flush;
    
    {
        Link_T L ( n );
        
        // Read coordinates into `Link_T` object `L`...
        L.ReadVertexCoordinates ( x.data() );
        
        log << kv<t2>("Link Class",L.ClassName()) << std::flush;
        
        (void)L.FindIntersections();
        
        log << kv<t2>("Link Byte Count",L.ByteCount()) << std::flush;
        
        // Deallocate tree-related data in L to make room for the PlanarDiagram.
        if( force_deallocQ )
        {
            L.DeleteTree();
        }
        
        // We delay the allocation until substantial parts of L have been deallocated.
        PlanarDiagram PD( L );
        
        if( force_deallocQ )
        {
            L = Link_T();
        }
        
        log << kv<t2>("PlanarDiagram Class",PD.ClassName());
        
        log << kv<t2>(
            "PlanarDiagram Byte Count (Before Simplification)",
            PD.ByteCount()
        );
        
        if( force_deallocQ )
        {
            PD = PD_T();
        }
    }
    
    log << kv<t2>("Creation of PlanarDiagram Succeeded",true)  << std::flush;
    
    log << "\n" + Tabs<t1> + "|>"  << std::flush;
}
