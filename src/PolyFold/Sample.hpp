private:

template<Size_T t0, int my_verbosity>
int Sample()
{
    int err;
    
    log << ",\n" + ct_tabs<t0> + "\"Samples\" -> {" << std::flush;;

    log << "\n" + ct_tabs<t0+1>;
    
    err = Sample<t0+1,my_verbosity>(LInt(0));
    
    if( err == 0 )
    {
        for( LInt i = 1; i < N; ++ i )
        {
            log << ",\n" + ct_tabs<t0+1>;
            
            err = Sample<t0+1,my_verbosity>(i);
            
            if( err != 0 )
            {
                goto exit;
            }
        }
    }
    
exit:
    
    log << "\n" + ct_tabs<t0> + "}" << std::flush;
    return err;
}
    
template<Size_T t0, int my_verbosity>
int Sample( const LInt i )
{
    
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    
    constexpr bool V1Q = my_verbosity >= 1;
    constexpr bool V2Q = my_verbosity >= 2;
    
    using FlagCountVec_T = Clisby_T::FlagCountVec_T;
    
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
    FlagCountVec_T counts;
    PRNG_FullState_T full_state;
    
    TimeInterval T_sample (0);
    
    {
        T_clisby.Tic<V2Q>();
        Clisby_T T ( x.data(), n, radius, random_engine );
        T_clisby.Toc<V2Q>();
        
        full_state = T.RandomEngineFullState();
        
        T_fold.Tic<V2Q>();
        // Do polygon folds until we have done at least `skip` accepted steps.
        counts = T.FoldRandom(skip);
        T_fold.Toc<V2Q>();
        
        attempt_count = counts.Total();
        accept_count = counts[0];
        
        total_attempt_count += attempt_count;
        total_accept_count += accept_count;
        
        T_write.Tic<V2Q>();
        T.WriteVertexCoordinates( x.data() );
        T_write.Toc<V2Q>();
        
        random_engine = T.GetRandomEngine();
        
        if( V2Q || (i + 1 == N) )
        {
            e_dev = T.MinMaxEdgeLengthDeviation( x.data() );
        }
        
        T_sample.Toc();
        
        sampling_time = T_sample.Duration();
        
        total_sampling_time += sampling_time;
        
        bytes = T.AllocatedByteCount();
        
        if( force_deallocQ )
        {
            T_dealloc.Tic<V1Q>();
            T = Clisby_T();
            T_dealloc.Toc<V1Q>();
        }
    }
    
    if constexpr ( V1Q )
    {
        total_timing = Duration( T_run[0], T_sample[1] );

        kv<t1>("Sample Seconds Elapsed", sampling_time );
        kv<t1>("Attempted Steps", attempt_count );
        kv<t1>("Attempted Steps/Second", Frac<Real>(attempt_count,sampling_time) );
        kv<t1>("Accepted Steps", accept_count );
        kv<t1>("Accepted Steps/Second", Frac<Real>(accept_count,sampling_time) );
        kv<t1>("Acceptance Probability", Frac<Real>(accept_count,attempt_count) );

        log << ",\n" << ct_tabs<t1> << "\"Clisby Tree\" -> <|";
            kv<t2,0>("Byte Count", bytes );
        if constexpr ( V2Q )
        {
            kv<t2>("Clisby Flag Counts", counts );
        }
        log << "\n" << ct_tabs<t1> << "|>";
        
        if constexpr ( V2Q )
        {
            log << ",\n" << ct_tabs<t1> << "\"Sample Time Details\" -> <|";
                kv<t2,0>("Create Clisby Tree", T_clisby.Duration());
                kv<t2>("Fold", T_fold.Duration());
                kv<t2>("Write Vertex Coordinates", T_write.Duration());
            if( force_deallocQ )
            {
                kv<t2>("Deallocate Clisby Tree", T_dealloc.Duration());
            }
            log << "\n" << ct_tabs<t1> << "|>";

            log << ",\n" << ct_tabs<t1> << "\"PCG64\" -> <|";
                kv<t2,0>("Multiplier", full_state.multiplier);
                kv<t2>("Increment" , full_state.increment );
                kv<t2>("State"     , full_state.state     );
            log << "\n" << ct_tabs<t1> << "|>";
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
