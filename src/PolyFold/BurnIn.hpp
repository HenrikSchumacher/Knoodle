private:

template<Size_T t0, int my_verbosity, bool reflectionsQ, bool checksQ>
void BurnIn()
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    
//    constexpr bool V0Q = true;
//    constexpr bool V1Q = my_verbosity >= 1;
    constexpr bool V2Q = my_verbosity >= 2;
    
    log << ",\n" + ct_tabs<t0> + "\"Burn-in\" -> <|" << std::flush;

    TimeInterval T_clisby;
    TimeInterval T_fold;
    TimeInterval T_write;
    TimeInterval T_dealloc;
    
    Size_T bytes;
    
    LInt attempt_count;
    LInt accept_count;
    Int active_node_count;
    FoldFlagCounts_T counts;
    
    TimeInterval T_burn_in (0);
    
    typename Clisby_T::CallCounters_T call_counters;
    
    clisby_begin();
    {
        T_clisby.Tic<V2Q>();
        Clisby_T T ( x.data(), n, hard_sphere_diam, prng );
        T_clisby.Toc<V2Q>();
    
//        pre_state = FullState( T.RandomEngine() );
        
        T_fold.Tic<V2Q>();
        counts = T.template FoldRandom<reflectionsQ,checksQ>(burn_in_accept_count);
        T_fold.Toc<V2Q>();
        
        attempt_count = counts.Total();
        accept_count = counts[0];
        
        total_attempt_count += attempt_count;
        total_accept_count += accept_count;
        
        burn_in_attempt_count = attempt_count;
        burn_in_accept_count = accept_count;
        
        if constexpr ( V2Q )
        {
            active_node_count = T.ActiveNodeCount();
        }
        
        T_write.Tic<V2Q>();
        T.WriteVertexCoordinates( x.data() );
        T_write.Toc<V2Q>();
        
        prng = T.RandomEngine();
        
        e_dev = T.MinMaxEdgeLengthDeviation( x.data() );
        
        bytes = T.AllocatedByteCount();
        
        if constexpr ( V2Q && Clisby_T::countersQ )
        {
            call_counters = T.CallCounters();
        }
        
        if( force_deallocQ )
        {
            T_dealloc.Tic<V2Q>();
            T = Clisby_T();
            T_dealloc.Toc<V2Q>();
        }
    }
    clisby_end();
    
    T_burn_in.Toc();

    const double timing = T_burn_in.Duration();
    
    burn_in_time = timing ;
    
    total_timing = Duration( T_run[0], T_burn_in[1] );
    
    kv<t1,0>("Burn-In Seconds Elapsed", burn_in_time );
    kv<t1>("Attempted Steps", attempt_count );
    kv<t1>("Attempted Steps/Second", Frac<Real>(attempt_count,timing) );
    kv<t1>("Accepted Steps", accept_count );
    kv<t1>("Accepted Steps/Second", Frac<Real>(accept_count,timing) );
    kv<t1>("Acceptance Probability", Frac<Real>(accept_count,attempt_count) );
    
    log << ",\n" + ct_tabs<t1> + "\"Clisby Tree\" -> <|";
        kv<t2,0>("Byte Count", bytes );
    if constexpr ( V2Q )
    {
        if constexpr ( Clisby_T::countersQ )
        {
            PrintCallCounts<t2>( call_counters );
        }
        
        PrintClisbyFlagCounts<t2>( counts );
        
        kv<t2>("Active Node Count", active_node_count );
        kv<t2>("Active Node Percentage", Percentage<Real>(active_node_count,n-1) );
    }
    log << "\n" + ct_tabs<t1> + "|>";
    
    if constexpr ( V2Q )
    {
        log << ",\n" + ct_tabs<t1> + "\"Time Details\" -> <|";
            kv<t2,0>("Create Clisby Tree", T_clisby.Duration());
            kv<t2>("Fold", T_fold.Duration());
            kv<t2>("Write Vertex Coordinates", T_write.Duration());
            if( force_deallocQ )
            {
                kv<t2>("Deallocate Clisby Tree", T_dealloc.Duration());
            }
        log << "\n" + ct_tabs<t1> + "|>";
    }
    kv<t1>("Total Seconds Elapsed", total_timing );
    kv<t1>("Total Attempted Steps (w/ Burn-in)", total_attempt_count );
    
    kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
    kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second );

    if( steps_between_print >= 0 )
    {
        PolygonSnapshot<t1>(LInt(0));
    }
    log << "\n" + ct_tabs<t0> + "|>";

    
} // BurnIn
