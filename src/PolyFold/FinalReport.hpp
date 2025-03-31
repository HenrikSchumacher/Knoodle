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
        kv<t2>("Snapshots", total_snapshot_time );
    log << "\n" + ct_tabs<t1> + "|>";
    
    if( force_deallocQ )
    {
        log << ",\n" + ct_tabs<t1> + "\"Allocation Time Details\" -> <|";
                kv<t2,0>("Allocation Time", allocation_time );
                kv<t2>("Deallocation Time", deallocation_time );
        log << "\n" + ct_tabs<t1> + "|>";
    }
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

    if ( checksQ )
    {
//        kv<t1>("Accumulated Intersection Flag Counts", acc_intersec_counts );
        
        using F_T = Link_T::Intersector_T::F_T;
        
        auto get = [this]( F_T flag )
        {
            return acc_intersec_counts[ToUnderlying(flag)];
        };
        
        log << ",\n" + ct_tabs<t1> + "\"Accumulated Intersection Flag Counts\" -> <|";
            kv<t2,0>("Empty Intersection", get(F_T::Empty));
            kv<t2>("Transversal Intersection", get(F_T::Transversal) );
            kv<t2>("Intersections on Corner of First Edge", get(F_T::AtCorner0) );
            kv<t2>("Intersections on Corner of Second Edge", get(F_T::AtCorner1) );
            kv<t2>("Intersections on Corners of Both Edges", get(F_T::CornerCorner) );
            kv<t2>("Interval-like Intersections", get(F_T::Interval) );
            kv<t2>("Spatial Intersections", get(F_T::Spatial) );
        log << "\n" + ct_tabs<t1> + "|>";
        
        {
            Size_T cnt = get(F_T::AtCorner0) + get(F_T::AtCorner0) + get(F_T::CornerCorner);
            if( cnt != 0 )
            {
                eprint(ClassName()+"::FinalReport: PlanarLineSegmentIntersector detected " + ToString(cnt) + " spatial intersections.");
            }
            cnt = get(F_T::AtCorner0) + get(F_T::AtCorner0) + get(F_T::CornerCorner);
            if( cnt != 0 )
            {
                wprint(ClassName()+"::FinalReport: PlanarLineSegmentIntersector detected " + ToString(cnt) + " corner cases.");
            }
            cnt = get(F_T::Interval);
            if( cnt != 0 )
            {
                wprint(ClassName()+"::FinalReport: PlanarLineSegmentIntersector detected " + ToString(cnt) + " interval-like intersections.");
            }
        }
    }
    
    kv<t1>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
    kv<t1>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second );

    
    log << "\n" + ct_tabs<t0> + "|>";
    
} // FinalReport
