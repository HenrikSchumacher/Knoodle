private:


std::string PDCodeString( mref<PD_T> PD ) const
{
    if( PD.CrossingCount() <= 0 )
    {
        return std::string();
    }
    
    auto pdcode = PD.PDCode();
    
    cptr<Int> a = pdcode.data();
    
    std::string s;
    s+= "\ns ";
    s+= ToString(PD.ProvablyMinimalQ());
    
    for( Int i = 0; i < pdcode.Dimension(0); ++i )
    {
        s+= VectorString<5>(&a[5 * i + 0], "\n", "\t", "" );
    }
    
    return s;
}

template<Size_T tab_count = 0>
int Analyze( const LInt i )
{
    (void)i;
    
    constexpr Size_T t1 = tab_count + 1;
    
    std::string s;
    
    Time start_time;
    
    start_time = Clock::now();
    
    Link_T L ( n );

    // Read coordinates into `Link_T` object `L`...
    L.ReadVertexCoordinates ( x.data() );
    
    int err = L.FindIntersections();
     
    const IntersectionFlagCounts_T intersection_flag_counts = L.IntersectionFlagCounts();
    
    acc_intersection_flag_counts += intersection_flag_counts;
    
    if( verbosity >= 1 )
    {
        log<<kv<t1>("Link Byte Count", L.ByteCount());
    }
    
    if( (err != 0) || (verbosity >= 2) )
    {
        log<<kv<t1>("Intersection Flag Counts", intersection_flag_counts);
        log<<kv<t1>("Accumulated Intersection Flag Counts", acc_intersection_flag_counts);
        log<<std::flush;
    }
    
    if( err != 0 )
    {
        log<<kv<t1>("FindIntersections Error Flag", err);
        log<<std::flush;
        return err;
    }
    
    // Deallocate tree-related data in L to make room for the PlanarDiagram.
    if( force_deallocQ )
    {
        L.DeleteTree();
    }
    
    // We delay the allocation until substantial parts of L have been deallocated.
    PlanarDiagram PD( L );
    
    // Delete remainder of L to make room for the simplification.
    if( force_deallocQ )
    {
        L = Link_T();
    }
    
    if( verbosity >= 1 )
    {
        log<<kv<t1>("PlanarDiagram Byte Count (Before Simplification)", PD.ByteCount());
        
        log<<kv<t1>(
            "PlanarDiagram Crossing Count (Before Simplification)", PD.CrossingCount()
        );
        
        log<<std::flush;
    }
    
    PD_list.clear();

    PD.Simplify5( PD_list );
    
    if ( verbosity >= 2 )
    {
        log<<kv<t1>("PlanarDiagram Byte Count (After Simplification)", PD.ByteCount());

        log<<kv<t1>(
            "PlanarDiagram Crossing Count (After Simplification)", PD.CrossingCount()
        );
        
        log<<std::flush;
    }
    
    // Writing the PD codes to file.
    pds << "k";
    
    Int crossing_count = PD.CrossingCount();
    
    pds << PDCodeString(PD);
    
    for( auto & P : PD_list )
    {
        pds << PDCodeString(P);
        crossing_count += P.CrossingCount();
    }
    
    pds << "\n";
    pds << std::flush;
    
    if( force_deallocQ )
    {
        PD = PD_T();
    }
    
    const Time stop_time = Clock::now();
    
    const double timing = Tools::Duration( start_time, stop_time );
    
    analysis_time += timing;
    
    if ( verbosity >= 1 )
    {
        log<<kv<t1>("Analysis Seconds Elapsed", timing);
    }

    return 0;
} // Analyze
