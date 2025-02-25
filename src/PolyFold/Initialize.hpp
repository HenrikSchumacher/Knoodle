private:

template<Size_T t0>
void Initialize()
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    constexpr Size_T t3 = t0 + 3;
    constexpr Size_T t4 = t0 + 4;
    
    TimeInterval T_init(0);
    
    log_file = path / "Info.m";
    pds_file = path / "PDCodes.tsv";
    
    log.open( log_file, std::ios_base::out );
    pds.open( pds_file, std::ios_base::out );
    
    // Use this path for profiles and general log files.
    Profiler::Clear(path,true);
    
    log << ct_tabs<t0> + "<|";
    
    kv<t1,0>("Edge Count",n);
    kv<t1>  ("Sample Count",N);
    kv<t1>  ("Burn-in Count",burn_in_accept_count);
    kv<t1>  ("Skip Count",skip);
    kv<t1>  ("Radius",radius);
    kv<t1>  ("Prescribed Edge Length",Real(1));
    kv<t1>  ("Verbosity",verbosity);
    
    log << ",\n" + ct_tabs<t1> + "\"Initialization\" -> <|";
    
    log << "\n" << ct_tabs<t2> << "\"PolyFold\" -> <|";
        kv<t3,0>("Class",ClassName());
        kv<t3>("Vector Extensions Enabled",vec_enabledQ);
        kv<t3>("Matrix Extensions Enabled",mat_enabledQ);
        kv<t3>("Forced Deallocation Enabled",force_deallocQ);
    log << "\n" << ct_tabs<t2> << "|>";
    
    log << ",\n" << ct_tabs<t2> << "\"Coordinate Buffer\" -> <|";
    
    x = Tensor2<Real,Int>( n, AmbDim );
    
        kv<t3,0>("Byte Count",x.ByteCount());
    log << "\n" << ct_tabs<t2> << "|>";
    
    log << std::flush;

    {
        log << ",\n" << ct_tabs<t2> << "\"Clisby Tree\" -> <|";
        
        kv<t3,0>("Class",Clisby_T::ClassName());
        log << std::flush;
        
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
        
        kv<t3>("Byte Count",T.ByteCount());
        kv<t3>("Euclidean Transformation Class",Clisby_T::Transform_T::ClassName());
        log << ",\n" << ct_tabs<t3> << "\"PCG64\" -> <|";
            kv<t4,0>("Multiplier", full_state.multiplier);
            kv<t4>("Increment" , full_state.increment );
            kv<t4>("State"     , full_state.state     );
        log << "\n" << ct_tabs<t3> << "|>";
        log << "\n" << ct_tabs<t2> << "|>";
        
        log << std::flush;
        
        T.WriteVertexCoordinates( x.data() );
        
        // Remember random engine for later use.
        random_engine = T.GetRandomEngine();
        
        if( force_deallocQ )
        {
            T = Clisby_T();
        }
    }
    
    {
        Link_T L ( n );
        
        // Read coordinates into `Link_T` object `L`...
        L.ReadVertexCoordinates ( x.data() );
        
        log << ",\n" << ct_tabs<t2> << "\"Link\" -> <|";
            kv<t3,0>("Class", L.ClassName());
        
//        kv<t2>("Link Class",L.ClassName());
        log << std::flush;
        
        (void)L.FindIntersections();
        
            kv<t3>("Byte Count",L.ByteCount());
        log << "\n" << ct_tabs<t2> << "|>";
        log << std::flush;
        
        // Deallocate tree-related data in L to make room for the PlanarDiagram.
        if( force_deallocQ )
        {
            L.DeleteTree();
        }
        
        log << ",\n" << ct_tabs<t2> << "\"Planar Diagram\" -> <|";
            kv<t3,0>("Class", PD_T::ClassName());
        
        // We delay the allocation until substantial parts of L have been deallocated.
        PD_T PD( L );
        
        if( force_deallocQ )
        {
            L = Link_T();
        }
            kv<t3>("Byte Count (Before Simplification)", PD.ByteCount() );
        log << "\n" << ct_tabs<t2> << "|>";
        log << std::flush;
        
        if( force_deallocQ )
        {
            PD = PD_T();
        }
    }
    
    log << "\n" + ct_tabs<t1> + "|>"  << std::flush;
}
