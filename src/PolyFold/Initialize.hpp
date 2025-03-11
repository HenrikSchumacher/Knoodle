private:

template<Size_T t0>
void Initialize()
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    constexpr Size_T t3 = t0 + 3;
    constexpr Size_T t4 = t0 + 4;
    
    TimeInterval T_init(0);
    
//    log_file = path / "Info.m";
//    pds_file = path / "PDCodes.tsv";
    
    log.open( path / "Info.m", std::ios_base::out );
    pds.open( path / "PDCodes.tsv", std::ios_base::out );
    
    if constexpr ( Clisby_T::witnessesQ )
    {
        witness_stream.open( path / "Witnesses.tsv", std::ios_base::out );
        
        witness_stream << "Pivot 0"   << "\t" << "Pivot 1"
        << "\t" << "Witness 0" << "\t" << "Witness 1\n";
        
        pivot_stream.open( path / "AcceptedPivotMoves.tsv", std::ios_base::out );
        
        pivot_stream   << "Pivot 0"   << "\t" << "Pivot 1" << "\t" << "Angle\n";
    }
    
    
    // Use this path for profiles and general log files.
    Profiler::Clear(path,true);
    
    log << ct_tabs<t0> + "<|";
    
    kv<t1,0>("Prescribed Edge Length",prescribed_edge_length);
    kv<t1>  ("Hard Sphere Diameter",hard_sphere_diam);
    kv<t1>  ("Edge Count",n);
    kv<t1>  ("Sample Count",N);
    kv<t1>  ("Burn-in Count",burn_in_accept_count);
    kv<t1>  ("Skip Count",skip);
    kv<t1>  ("Verbosity",verbosity);
    
    log << ",\n" + ct_tabs<t1> + "\"Initialization\" -> <|";
    
    log << "\n" + ct_tabs<t2> + "\"PolyFold\" -> <|";
        kv<t3,0>("Class",ClassName());
        kv<t3>("Vector Extensions Enabled",vec_enabledQ);
        kv<t3>("Matrix Extensions Enabled",mat_enabledQ);
        kv<t3>("Forced Deallocation Enabled",force_deallocQ);
    log << "\n" + ct_tabs<t2> + "|>";
    
    log << ",\n" + ct_tabs<t2> + "\"Coordinate Buffer\" -> <|";
    
    x = Tensor2<Real,Int>( n, AmbDim );
    
        kv<t3,0>("Byte Count", x.ByteCount() );
    log << "\n" + ct_tabs<t2> + "|>";
    
    log << std::flush;

    log << ",\n" + ct_tabs<t2> + "\"Clisby Tree\" -> <|";
    
    kv<t3,0>("Class",Clisby_T::ClassName());
    log << std::flush;
    
    clisby_begin();
    {
        Clisby_T T ( n, hard_sphere_diam );
        
        prng = T.RandomEngine();
        
        PRNG_State_T state = State( prng );

        if( !prng_multiplierQ )
        {
            prng_init.multiplier = state.multiplier;
        }
        
        if( !prng_incrementQ )
        {
            prng_init.increment = state.increment;
        }
        
        if( !prng_stateQ )
        {
            prng_init.state = state.state;
        }
        
        if( prng_multiplierQ || prng_incrementQ || prng_stateQ)
        {
           if( SetState( prng, prng_init ) )
           {
               eprint( ClassName() + "::Initialize: Failed to initialize random engine with <|"
                      + "Multiplier -> " + prng_init.multiplier + ", "
                      + "Increment -> "  + prng_init.increment + ", "
                      + "State -> "      + prng_init.state + "|>. Aborting."
                );
               exit(1);
           }
        }
        
        kv<t3>("Byte Count", T.ByteCount() );
        log << ",\n" + ct_tabs<t3> + "\"Byte Count Details\" -> ";
        log << T.template AllocatedByteCountDetails<t3>();
        
        kv<t3>("Euclidean Transformation Class",Clisby_T::Transform_T::ClassName());
        log << ",\n" + ct_tabs<t3> + "\"PCG64\" -> <|";
            kv<t4,0>("Multiplier", prng_init.multiplier);
            kv<t4>("Increment"   , prng_init.increment );
            kv<t4>("State"       , prng_init.state     );
        log << "\n" + ct_tabs<t3> + "|>";
        log << "\n" + ct_tabs<t2> + "|>";
        
        log << std::flush;
        
        T.WriteVertexCoordinates( x.data() );
        
        // Remember random engine for later use.
        prng = T.RandomEngine();
        
        if( force_deallocQ )
        {
            T = Clisby_T();
        }
    }
    clisby_end();

    link_begin();
    {
        Link_T L ( n );

        L.ReadVertexCoordinates ( x.data() );

        log << ",\n" + ct_tabs<t2> + "\"Link\" -> <|";
            kv<t3,0>("Class", L.ClassName());
        log << std::flush;
        
        (void)L.FindIntersections();
        
            kv<t3>("Byte Count", L.ByteCount() );
            log << ",\n" + ct_tabs<t3> + "\"Byte Count Details\" -> ";
            log << L.template AllocatedByteCountDetails<t3>();
        
        log << "\n" + ct_tabs<t2> + "|>";
        log << std::flush;

        // Deallocate tree-related data in L to make room for the PlanarDiagram.
        if( force_deallocQ )
        {
            L.DeleteTree();
        }

        log << ",\n" + ct_tabs<t2> + "\"Planar Diagram\" -> <|";
            kv<t3,0>("Class", PD_T::ClassName());
        
        // We delay the allocation until substantial parts of L have been deallocated.
        pd_begin();
        PD_T PD ( L );

        if( force_deallocQ )
        {
            L = Link_T();
            link_end();
        }
        
            kv<t3>("Byte Count (Before Simplification)", PD.ByteCount() );
        log << "\n" + ct_tabs<t2> + "|>";
        log << std::flush;

        if( force_deallocQ )
        {
            PD = PD_T();
        }
        pd_end();
    }
    
    T_init.Toc();
            
    kv<t2>("Initialization Seconds Elapsed", T_init.Duration() );

    log << "\n" + ct_tabs<t1> + "|>"  << std::flush;
    
    
    acc_intersec_counts.SetZero();
}
