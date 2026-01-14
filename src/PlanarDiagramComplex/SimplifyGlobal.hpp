public:

Size_T SimplifyGlobal(
    const Int    min_dist        = 6,
    const Int    max_dist        = Scalar::Max<Int>,
    const Int    local_opt_level = 4,
    const Size_T local_max_iter  = Scalar::Max<Int>,
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
            return SimplifyGlobal_impl<1,true,true>(min_dist,max_dist,local_max_iter,compressQ);
        }
        case 2:
        {
            return SimplifyGlobal_impl<2,true,true>(min_dist,max_dist,local_max_iter,compressQ);
        }
        case 3:
        {
            return SimplifyGlobal_impl<3,true,true>(min_dist,max_dist,local_max_iter,compressQ);
        }
        case 4:
        {
            return SimplifyGlobal_impl<4,true,true>(min_dist,max_dist,local_max_iter,compressQ);
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
    const Int    min_dist,
    const Int    max_dist,
    const Size_T local_max_iter,
    const bool   compressQ
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
    

    using ArcSimplifier_T  = ArcSimplifier2<Int,local_opt_level,mult_compQ>;
    using StrandSimplier_T = StrandSimplifier2<Int,pass_R_II_Q,mult_compQ>;
    
    Size_T total_counter = 0;
    
    PD_ASSERT(pd_done.empty());
    PD_ASSERT(pd_todo.empty());
    
    using std::swap;
    
    do
    {
        for( PD_T & pd : pd_list )
        {
            if( pd.InvalidQ() ) { continue; }
            
            if(  pd.ProvenMinimalQ() )
            {
                pd_done.push_back( std::move(pd) );
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
            bool simplify_strandsQ = (max_dist > Int(0));
            
            do
            {
                old_counter = counter;
                
                dist = Min(dist,max_dist);
                
                Size_T simplify_local_changes = 0;
                
                // Since ArcSimplifier_T performs only inexpensive tests, we should use it first.
                if( simplify_localQ && (local_opt_level > Int(0)) )
                {
                    ArcSimplifier_T A ( *this, pd, local_max_iter, compressQ );
                    simplify_local_changes = A();
                    counter += simplify_local_changes;
                }
                
                // TODO: If we did strand moves before without success and if counter == old_counter, then we can break here, too.
                
                if( pd.InvalidQ() ) { break; }
                
                if( !simplify_strandsQ ) { break; }
                
                // Reroute overstrands.
                const Size_T o_changes = S.SimplifyStrands(true,dist);
                counter += o_changes;
                
                if( pd.InvalidQ() ) { break; }
                
                if( (o_changes > Size_T(0)) )
                {
                    if( compressQ )
                    {
                        // TODO: See whether this works better than an unconditional compress.
                        pd.ConditionalCompress();
                    }
                    else
                    {
                        pd.ClearCache();
                    }
                }

                // We can get an invalid diagram if we split off the last component.
                PD_ASSERT(pd.CheckAll());
                
                // Reroute overstrands.
                const Size_T u_changes = S.SimplifyStrands(false,dist);
                counter += u_changes;
                
                if( pd.InvalidQ() ) { break; }
                
                if( (u_changes > Size_T(0)) )
                {
                    if( compressQ )
                    {
                        // TODO: See whether this works better than an unconditional compress.
                        pd.ConditionalCompress();
                    }
                    else
                    {
                        pd.ClearCache();
                    }
                }
                
                // We can get an invalid diagram if we split off the last component.
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
            
            if( pd.InvalidQ() ) continue;
            
            PD_ASSERT(pd.CheckAll());
            
            total_counter += counter;
            
            if( counter > Size_T(0) )
            {
                if( compressQ )
                {
                    // Here we finally do want a fully compressed diagram.
                    pd.Compress();
                }
                else
                {
                    pd.ClearCache();
                }
            }
            
            if( pd.CrossingCount() == Int(0) )
            {
                pd.proven_minimalQ = true;
            }
            
            pd_done.push_back( std::move(pd) );
            
        } // for( PD_T & pd : pd_list )

        swap( pd_list, pd_todo );
        pd_todo.clear();
    }
    while( !pd_list.empty() );
    
    PD_ASSERT( pd_list.empty() );
    PD_ASSERT( pd_todo.empty() );

    swap( pd_list, pd_done );
    
    return total_counter;
}

