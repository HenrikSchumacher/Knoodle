public:

Size_T SimplifyLocal2( const bool compressQ )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("SimplifyLocal2"); };
    
    TOOLS_PTIMER(timer,tag());
    
    Size_T total_change_count = 0;
    
    PD_ASSERT(pd_done.empty());
    PD_ASSERT(pd_todo.empty());
    
    mref<StrandSimplifier_T> S = StrandSimplifier();
    
    using std::swap;
    pd_done.reserve(pd_list.size());
    pd_todo.reserve(pd_list.size());
  
    swap(pd_list,pd_todo);
    
    while( !pd_todo.empty() )
    {
        PD_T pd = std::move(pd_todo.back());
        pd_todo.pop_back();
        
        if( pd.InvalidQ() ) { continue; }
        
        if(  pd.proven_minimalQ )
        {
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint(tag() + ": CheckAll() failed when pushed to pd_done."); };
            }
            
            if( pd.crossing_count < pd.max_crossing_count )
            {
                pd_done.push_back( pd.CreateCompressed() );
            }
            else
            {
                // TODO: I don't think that pd.ClearCache() is meaningful here.
//                pd.ClearCache();
                pd_done.push_back( std::move(pd) );
            }
            continue;
        }
        // It is very likely that we change the diagram.
        // Also, a stale cache might spoil the simplification.
        // Thus, we proactively delete the cache.
        pd.ClearCache();
        
        total_change_count += S.SimplifyLocal(pd,compressQ);

        if( pd.InvalidQ() ) { continue; }
        
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint(tag() + ":pd.CheckAll() failed after simplification."); };
        }
        
        if( pd.CrossingCount() <= Int(1) )
        {
            pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
            continue;
        }
            
        if( pd.crossing_count < pd.max_crossing_count )
        {
            pd.Compress();
            
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint(tag() + ": pd.CheckAll() failed after compression."); };
            }
        }
        // Should be unnecessary.
        pd.ClearCache();
        pd_done.push_back( std::move(pd) );
        
    } // while( !pd_todo.empty() )
    
    swap( pd_list, pd_done );
    
    if( total_change_count > Size_T(0) )
    {
        SortByCrossingCount();
        // SortByCrossingCount clears the cache already.
//        this->ClearCache();
    }
    
    return total_change_count;
}

