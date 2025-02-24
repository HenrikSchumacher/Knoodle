private:
    
template<Size_T tab_count, int my_verbosity>
int Sample( const LInt i )
{
    Time start_time;
    
    Clisby_T T ( x.data(), n, radius, random_engine );
     
    PRNG_FullState_T full_state = T.RandomEngineFullState();
    
    constexpr Size_T t1 = tab_count + 1;
    
    if constexpr ( my_verbosity >= 1 )
    {
        log<<"<|";
    }
    
    start_time = Clock::now();
    
    // Do polygon folds until we have done at least `skip` accepted steps.
    auto counts = T.FoldRandom(skip);
    
    LInt attempt_count = counts.Total();
    LInt accept_count = counts[0];
    
    total_attempt_count += attempt_count;
    total_accept_count += accept_count;
    
    T.WriteVertexCoordinates( x.data() );
    
    random_engine = T.GetRandomEngine();
    
    if( (verbosity >= 2)  || (i + 1 == N) )
    {
        e_dev = T.MinMaxEdgeLengthDeviation( x.data() );
    }
    
    const Time stop_time = Clock::now();
    
    const double timing = Tools::Duration( start_time, stop_time );
    
    sampling_time += timing;
    
    if constexpr ( my_verbosity >= 1 )
    {
        total_timing = Tools::Duration( run_start_time, stop_time );

        log<<kv<t1,0>("Sample", i );
        log<<kv<t1>("Seconds Elapsed", timing );
        log<<kv<t1>("Attempted Steps", attempt_count );
        log<<kv<t1>("Attempted Steps/Second", Frac<Real>(attempt_count,timing) );
        log<<kv<t1>("Accepted Steps", accept_count );
        log<<kv<t1>("Accepted Steps/Second", Frac<Real>(accept_count,timing) );
        log<<kv<t1>("Acceptance Probability", Frac<Real>(accept_count,attempt_count) );

        log<<kv<t1>("Clisby Tree Byte Count", T.AllocatedByteCount() );
        
        if constexpr ( my_verbosity >= 2 )
        {
            log<<kv<t1>("PCG64 Multiplier",full_state.multiplier);
            log<<kv<t1>("PCG64 Increment",full_state.increment);
            log<<kv<t1>("PCG64 State", full_state.state );
            log<<kv<t1>("Clisby Flag Counts", counts );
        }
        
        log<<kv<t1>("Total Seconds Elapsed", total_timing );
        log<<kv<t1>("Total Attempted Steps (w/ Burn-in)", total_attempt_count );
        
        log<<kv<t1>(
            "Total Acceptance Probability (w/ Burn-in)",
            Frac<Real>(total_accept_count, total_attempt_count)
        );
        
        if constexpr ( my_verbosity >= 2 )
        {
            log<<kv<t1>(
                "Total Attempted Steps (w/o Burn-in)",
                total_attempt_count - burn_in_attempt_count
            );
            
            log<<kv<t1>(
                "Total Acceptance Probability (w/o Burn-in)",
                Frac<Real>(
                    total_accept_count - burn_in_accept_count,
                    total_attempt_count - burn_in_attempt_count
                )
            );
            log<<kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
            log<<kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second);

        }
        log<<std::flush;
    }
    
    if( force_deallocQ )
    {
        T = Clisby_T();
    }
        
    int err = Analyze<tab_count>(i);
        
    if constexpr ( my_verbosity >= 1 )
    {
        log<<"\n" + Tabs<tab_count> + "|>" << std::flush;
    }
    
    return err;
    
} // Sample
