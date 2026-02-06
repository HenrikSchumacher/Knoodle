public:

Size_T RemoveLoops()
{
    Size_T counter = 0;
    
    PD_ASSERT( pd_done.empty() );
    
    for( PD_T & pd : pd_list )
    {
        counter += RemoveLoops(pd);
    }
    
    for( PD_T & pd : pd_done )
    {
        pd_list.push_back( std::move(pd) );
    }
    
    pd_done.clear();
    
    return counter;
}

Size_T RemoveLoops( mref<PD_T> pd )
{
    Size_T loop_count = 0;
    
    if( pd.InvalidQ() ) { return 0; };
    
    if( pd.ProvenMinimalQ() ) { return 0; };
 
    for( Int a = 0; a < pd.max_arc_count; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; }
        
        LoopRemover<Int> R (*this,pd,a,Head);
        
        while( R.Step() ) { ++loop_count; }
    }
    
    if( loop_count > Size_T(0) ) { pd.ClearCache(); }
    
    return loop_count;
}

