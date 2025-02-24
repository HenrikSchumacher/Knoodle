private:

template<Size_T t0>
void BurnIn()
{
    constexpr Size_T t1 = t0 + 1;
    
    const Time b_start_time = Clock::now();
    
    Clisby_T T ( x.data(), n, radius, random_engine );
    
    PRNG_FullState_T full_state = T.RandomEngineFullState();
    
    log << "<|";
    
    auto counts = T.FoldRandom(burn_in_accept_count);
    
    const LInt attempt_count = counts.Total();
    const LInt accept_count = counts[0];
    
    total_attempt_count += attempt_count;
    total_accept_count += accept_count;
    
    burn_in_attempt_count = attempt_count;
    
    burn_in_accept_count = accept_count;
    
    const Time b_stop_time = Clock::now();
    
    double timing = Tools::Duration( b_start_time, b_stop_time );
    
    burn_in_time = timing ;
    
    log << kv<t1,0>("Time Elapsed", burn_in_time );
    log << kv<t1>("Attempted Steps", attempt_count );
    log << kv<t1>("Attempted Steps/Second", Frac<Real>(attempt_count,timing) );
    log << kv<t1>("Accepted Steps", accept_count );
    log << kv<t1>("Accepted Steps/Second", Frac<Real>(accept_count,timing) );
    log << kv<t1>("Acceptance Probability", Frac<Real>(accept_count,attempt_count) );
    
    log << kv<t1>("Clisby Tree Byte Count", T.AllocatedByteCount() );
    
    if ( verbosity >= 2 )
    {
        log << kv<t1>("PCG64 Multiplier", full_state.multiplier);
        log << kv<t1>("PCG64 Increment" , full_state.increment );
        log << kv<t1>("PCG64 State"     , full_state.state     );
    }
    
    T.WriteVertexCoordinates( x.data() );
    
    random_engine = T.GetRandomEngine();
    
    e_dev = T.MinMaxEdgeLengthDeviation( x.data() );
    
    log << kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
    log << kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second );
    
    log << "\n" + Tabs<t0> + "|>";
    
} // BurnIn

