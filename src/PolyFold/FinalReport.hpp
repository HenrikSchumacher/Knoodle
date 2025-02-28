private:

template<Size_T t0>
void FinalReport()
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    
    log << ",\n" + ct_tabs<t0> + "\"Final Report\" -> <|";
    
    std::string s ( "<|" );
        
    const LInt sample_attempt_count = total_attempt_count - burn_in_attempt_count;
    const LInt sample_accept_count = skip * N;

    kv<t1,0>("Total Seconds Elapsed", total_timing );
    log << ",\n" + ct_tabs<t1> + "\"Total Time Details\" -> <|";
        kv<t2,0>("Burn-in", burn_in_time );
        kv<t2>("Sampling", total_sampling_time );
        kv<t2>("Analysis", total_analysis_time );
    log << "\n" + ct_tabs<t1> + "|>";
    
    kv<t1>("Attempted Steps", sample_attempt_count );
    kv<t1>(
        "Attempted Steps/Second",
        Frac<Real>(sample_attempt_count,total_sampling_time)
    );
    kv<t1>("Accepted Steps", sample_accept_count );
    
    kv<t1>(
        "Accepted Steps/Second",
        Frac<Real>(sample_accept_count,total_sampling_time)
    );
    kv<t1>(
        "Acceptance Probability",
        Frac<Real>(sample_accept_count,sample_attempt_count)
    );

    if ( do_checksQ )
    {
        kv<t1>("Accumulated Intersection Flag Counts", acc_intersection_flag_counts );
    }
    
    kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
    kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second );

    
    log << "\n" + ct_tabs<t0> + "|>";
    
} // FinalReport
