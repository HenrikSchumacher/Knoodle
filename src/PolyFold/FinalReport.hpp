private:

template<Size_T tab_count>
void FinalReport()
{
    constexpr Size_T t1 = tab_count + 1;
    
    std::string s ( "<|" );
    
    log << "<|";
    
    const LInt sample_attempt_count = total_attempt_count - burn_in_attempt_count;
    const LInt sample_accept_count = skip * N;

    log << kv<t1,0>("Total Time Elapsed", total_timing );
    log << kv<t1>("Burn-in Time Elapsed", burn_in_time );
    log << kv<t1>("Sampling Time Elapsed", sampling_time );
    log << kv<t1>("Analysis Time Elapsed", analysis_time );
    log << kv<t1>("Attempted Steps", sample_attempt_count );
    
    log << kv<t1>(
        "Attempted Steps/Second",
        Frac<Real>(sample_attempt_count,sampling_time)
    );
    log << kv<t1>("Accepted Steps", sample_accept_count );
    
    log << kv<t1>(
        "Accepted Steps/Second",
        Frac<Real>(sample_accept_count,sampling_time)
    );
    log << kv<t1>(
        "Acceptance Probability",
        Frac<Real>(sample_accept_count,sample_attempt_count)
    );

    log << kv<t1>("Accumulated Intersection Flag Counts", acc_intersection_flag_counts );
    
    log << kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
    log << kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second );

    
    log << "\n" + Tabs<tab_count> + "|>";
    
} // FinalReport
