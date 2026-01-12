public:

Size_T SimplifyGlobal(
    const Int    min_dist        = 6,
    const Int    max_dist        = std::numeric_limits<Int>::max(),
    const Int    local_opt_level = 4,
    const Size_T local_max_iter  = std::numeric_limits<Int>::max(),
    const bool   compressQ       = true
)
{
    if( DiagramCount() == Int(0) )
    {
        return 0;
    }
    
    const Int level = Clamp(local_opt_level, Int(0), Int(4));
    
    switch ( level )
    {
        case 0:
        {
            return 0;
        }
        case 1:
        {
            return SimplifyGlobal_impl<1,true,true>(min_dist,max_dist,local_max_iter,compressQ);;
        }
        case 2:
        {
            return SimplifyGlobal_impl<2,true,true>(min_dist,max_dist,local_max_iter,compressQ);;
        }
        case 3:
        {
            return SimplifyGlobal_impl<3,true,true>(min_dist,max_dist,local_max_iter,compressQ);;
        }
        case 4:
        {
            return SimplifyGlobal_impl<4,true,true>(min_dist,max_dist,local_max_iter,compressQ);;
        }
        default:
        {
            eprint( MethodName("SimplifyGlobal") + ": Value " + ToString(level) + " is invalid." );
            return 0;
        }
    }
    
    return 0;
}

private:

template<Int local_opt_level, bool pass_R_II_Q, bool mult_compQ>
Size_T SimplifyGlobal_impl(
    const Int    min_dist        = 6,
    const Int    max_dist        = std::numeric_limits<Int>::max(),
    const Size_T local_max_iter  = std::numeric_limits<Int>::max(),
    const bool   compressQ       = true
)
{
    TOOLS_PTIMER(timer,MethodName("SimplifyGlobal_impl")
        + "<" + ToString(local_opt_level)+","+ToString(mult_compQ)
        + ">"
        + "(" + ToString(min_dist)
        + "," + ToString(max_dist)
        + "," + ToString(local_max_iter)
        + "," + ToString(compressQ)
        + ")");
    
    using ArcSimplier_T    = ArcSimplifier2<Int,local_opt_level,mult_compQ>;
    using StrandSimplier_T = StrandSimplifier2<Int,pass_R_II_Q,mult_compQ>;
    
    Size_T total_counter = 0;
    
    // This list is used for the diagrams that cannot processed any further.
    PDList_T pd_list_done;
    
    using std::swap;
    
    PD_ASSERT( pd_list_new.size() == Size_T(0) );
    
    do
    {
        for( PD_T & pd : pd_list )
        {
            if( pd.InvalidQ() )
            {
                continue;
            }
            
            if(  pd.ProvenMinimalQ() )
            {
                pd_list_done.push_back( std::move(pd) );
                continue;
            }
            
            Size_T old_counter = 0;
            Size_T counter = 0;
            
            // It is very likely that we change the diagram.
            // Also, a stale cache might spoil the simplification.
            // Thus, we proactively delete the cache.
            pd.ClearCache();
            
            StrandSimplier_T S (*this,pd);
            
            Int dist = min_dist;
            bool simplify_localQ   = true;
            bool simplify_strandsQ = max_dist > Int(0);
            
            do
            {
                old_counter = counter;
                
                dist = Min(dist,max_dist);
                
                Size_T simplify_local_changes = 0;
                
                // Since ArcSimplier_T performs only inexpensive tests, we should use it first.
                if( simplify_localQ && (local_opt_level > Int(0)) )
                {
                    ArcSimplier_T A ( *this, pd, local_max_iter, false );
                    simplify_local_changes = A();
                    counter += simplify_local_changes;
                    
                    if( simplify_local_changes > Size_T(0) )
                    {
                        pd.ClearCache();
                    }
                    
                    if( compressQ && (simplify_local_changes > Size_T(0)) ) { pd.Compress(); }
                }
                
                if( !simplify_strandsQ ) { break; }
                
                // Reroute overstrands.
                const Size_T o_changes = S.SimplifyStrands(true,dist);
                counter += o_changes;
                if( compressQ && (o_changes > Size_T(0)) ) { pd.Compress(); }
                
                PD_ASSERT(pd.CheckAll());
                
                // Reroute overstrands.
                const Size_T u_changes = S.SimplifyStrands(false,dist);
                counter += u_changes;
                if( compressQ && (u_changes > Size_T(0)) ) { pd.Compress(); }
                
                PD_ASSERT(pd.CheckAll());
                
                if( dist <= Scalar::Max<Int> / Int(2) )
                {
                    dist *= Int(2);
                }
                else
                {
                    dist = Scalar::Max<Int>;
                }
                simplify_localQ = (o_changes + u_changes > Int(0));
            }
            while(
                  (counter > old_counter)
                  ||
                  ( (dist <= max_dist) && (dist < pd.ArcCount()) )
            );
            
            PD_ASSERT(pd.CheckAll());
            
            total_counter += counter;
            
            if( counter > Size_T(0) )
            {
                pd.ClearCache();
            }
            
            if( pd.ValidQ() )
            {
                if( pd.CrossingCount() == Int(0) )
                {
                    pd.proven_minimalQ = true;
                }
                
                pd_list_done.push_back( std::move(pd) );
            }
            else
            {
                wprint(MethodName("SimplifyGlobal_impl")+": A diagram that came out of " + pd.ClassName() + " was identified as invalid and discarded. This is per se not a bad thing to happen." );
            }
        
        } // for( PD_T & pd : pd_list )

        swap( pd_list, pd_list_new );
        pd_list_new = PDList_T();
    }
    while( pd_list.size() != Size_T(0) );
    
    PD_ASSERT( pd_list_new.size() == Size_T(0) );
    PD_ASSERT( pd_list.size()     == Size_T(0) );
    
    swap( pd_list, pd_list_done );
        
    return total_counter;
}

