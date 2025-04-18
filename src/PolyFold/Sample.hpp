private:

template<Size_T t0, int my_verbosity>
int Sample()
{
    int err;
    
    log << ",\n" + ct_tabs<t0> + "\"Samples\" -> {" << std::flush;;

    log << "\n" + ct_tabs<t0+1>;

    print_ctr = printQ ? LInt(0) : steps_between_print - LInt(1);
    
    err = Sample<t0+1,my_verbosity>(LInt(1));

    if( err != 0 )
    {
        goto exit;
    }
    
    for( LInt i = 2; i < N; ++ i )
    {
        log << ",\n" + ct_tabs<t0+1>;
        
        err = Sample<t0+1,my_verbosity>(i);

        if( err != 0 )
        {
            goto exit;
        }
    }
    
    {
        log << ",\n" + ct_tabs<t0+1>;
        
        // Run the last sample step with full verbosity, because that is affordable and we very often want this info for performance tuning.
        err = Sample<t0+1,2>(N);

        if( err != 0 )
        {
            goto exit;
        }
    }
    
exit:
    
    log << "\n" + ct_tabs<t0> + "}" << std::flush;
    
    return err;
}
    
template<Size_T t0, int my_verbosity>
int Sample( const LInt i )
{
    sample_begin();
    
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    
    constexpr bool V1Q = my_verbosity >= 1;
    constexpr bool V2Q = my_verbosity >= 2;
    
    print_ctr += printQ;
    
    log << "<|";
    kv<t1,0>("Sample", i );

    TimeInterval T_clisby;
    TimeInterval T_fold;
    TimeInterval T_write;
    TimeInterval T_dealloc;
    
    double sampling_time;
    Size_T bytes;
    
    LInt attempt_count;
    LInt accept_count;
    Int active_node_count;
    FoldFlagCounts_T counts;
    typename Clisby_T::CallCounters_T call_counters;
    
    TimeInterval T_sample (0);


    clisby_begin();
    {
        T_clisby.Tic<V2Q>();
        Clisby_T T( x.data(), n, hard_sphere_diam, prng );
        T_clisby.Toc<V2Q>();
        allocation_time += T_clisby.Duration();
        
        T_fold.Tic<V2Q>();
        // Do polygon folds until we have done at least `skip` accepted steps.
        counts = T.FoldRandom( skip, reflection_probability, checksQ );
        T_fold.Toc<V2Q>();
        
        attempt_count = counts.Total();
        accept_count = counts[0];
        
        total_attempt_count += attempt_count;
        total_accept_count += accept_count;
        
        if constexpr ( V2Q )
        {
            active_node_count = T.ActiveNodeCount();
        }
        
        T_write.Tic<V2Q>();
        T.WriteVertexCoordinates( x.data() );
        T_write.Toc<V2Q>();
        
        if constexpr ( Clisby_T::witnessesQ )
        {
            auto & witness_collector = T.WitnessCollector();
            
            for( auto v : witness_collector )
            {
                witness_stream << v[0] << "\t" << v[1] << "\t" << v[2] << "\t" << v[3] << "\n";
            }
            
            auto & pivot_collector = T.PivotCollector();
            
            for( auto v : pivot_collector )
            {
                pivot_stream << std::get<0>(v) << "\t" << std::get<1>(v) << "\t" << std::get<2>(v) << "\n";
            }
        }
        
        prng = T.RandomEngine();
        
        if( V2Q || (i + 1 == N) )
        {
            e_dev = T.MinMaxEdgeLengthDeviation( x.data() );
        }
        
        bytes = T.AllocatedByteCount();
        
        if constexpr ( V2Q && Clisby_T::countersQ )
        {
            call_counters = T.CallCounters();
        }
        
        if( force_deallocQ )
        {
            T_dealloc.Tic<V1Q>();
            T = Clisby_T();
            T_dealloc.Toc<V1Q>();
            deallocation_time += T_dealloc.Duration();
        }
    }
    clisby_end();
    
    T_sample.Toc();
    
    sampling_time = T_sample.Duration();
    
    total_sampling_time += sampling_time;
    
    sample_end();
    
    if constexpr ( V1Q )
    {
        total_timing = Duration( T_run[0], T_sample[1] );

        kv<t1>("Sample Seconds Elapsed", sampling_time );
        kv<t1>("Attempted Steps", attempt_count );
        kv<t1>("Attempted Steps/Second", Frac<Real>(attempt_count,sampling_time) );
        kv<t1>("Accepted Steps", accept_count );
        kv<t1>("Accepted Steps/Second", Frac<Real>(accept_count,sampling_time) );
        kv<t1>("Acceptance Probability", Frac<Real>(accept_count,attempt_count) );

        log << ",\n" + ct_tabs<t1> + "\"Clisby Tree\" -> <|";
            kv<t2,0>("Byte Count", bytes );
        if constexpr ( V2Q )
        {
            if constexpr ( Clisby_T::countersQ )
            {
                PrintCallCounts<t2>( call_counters );
            }
            
            kv<t2>("Active Node Count", active_node_count );
            kv<t2>("Active Node Percentage", Percentage<Real>(active_node_count,n-1) );
        }
        log << "\n" + ct_tabs<t1> + "|>";
        
        if constexpr ( V2Q )
        {
            log << ",\n" + ct_tabs<t1> + "\"Sample Time Details\" -> <|";
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
        
        kv<t1>(
            "Total Acceptance Probability (w/ Burn-in)",
            Frac<Real>(total_accept_count, total_attempt_count)
        );
        
        if constexpr ( V2Q )
        {
            kv<t1>(
                "Total Attempted Steps (w/o Burn-in)",
                total_attempt_count - burn_in_attempt_count
            );
            
            kv<t1>(
                "Total Acceptance Probability (w/o Burn-in)",
                Frac<Real>(
                    total_accept_count - burn_in_accept_count,
                    total_attempt_count - burn_in_attempt_count
                )
            );
            kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
            kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second);

        }
        
        log << std::flush;
    }
    
    int err = Analyze<t0,my_verbosity>(i);
        
    log << "\n" + ct_tabs<t0> + "|>" << std::flush;
    
    return err;
    
} // Sample
