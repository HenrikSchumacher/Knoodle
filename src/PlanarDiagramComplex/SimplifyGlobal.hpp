public:


struct Simplify_Args_T
{
    Int    min_dist        = 6;
    Int    max_dist        = Scalar::Max<Int>;
    Int    local_opt_level = 4;
    Size_T local_max_iter  = Scalar::Max<Size_T>;
    bool   disconnectQ     = false;
    bool   compressQ       = true;
};

Size_T SimplifyGlobal( Simplify_Args_T args = Simplify_Args_T() )
{
    if( DiagramCount() == Int(0) ) { return 0; }
    
    const Int level = Clamp(args.local_opt_level, Int(0), Int(4));
    
    switch ( level )
    {
        case 0:
        {
            return SimplifyGlobal_impl<0,true,true>(
                args.min_dist, args.max_dist, args.local_max_iter, args.disconnectQ, args.compressQ
            );
        }
        case 1:
        {
            return SimplifyGlobal_impl<1,true,true>(
                args.min_dist, args.max_dist, args.local_max_iter, args.disconnectQ, args.compressQ
            );
        }
        case 2:
        {
            return SimplifyGlobal_impl<2,true,true>(
                args.min_dist, args.max_dist, args.local_max_iter, args.disconnectQ, args.compressQ
            );
        }
        case 3:
        {
            return SimplifyGlobal_impl<3,true,true>(
                args.min_dist, args.max_dist, args.local_max_iter, args.disconnectQ, args.compressQ
            );
        }
        case 4:
        {
            return SimplifyGlobal_impl<4,true,true>(
                args.min_dist, args.max_dist, args.local_max_iter, args.disconnectQ, args.compressQ
            );
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
    const bool   disconnectQ,
    const bool   compressQ
)
{
    TOOLS_PTIMER(timer,MethodName("SimplifyGlobal_impl")
        + "<" + ToString(local_opt_level)+","+ToString(mult_compQ)
        + ">"
        + "(" + ToString(min_dist)
        + "," + ToString(max_dist)
        + "," + ToString(local_max_iter)
        + "," + ToString(disconnectQ)
        + "," + ToString(compressQ)
        + ")");
    
    if constexpr ( debugQ )
    {
        wprint(MethodName("SimplifyGlobal_impl")+": Debug mode active.");
    }

    using ArcSimplifier_T  = ArcSimplifier2<Int,local_opt_level,mult_compQ>;
    using StrandSimplier_T = StrandSimplifier2<Int,pass_R_II_Q,mult_compQ>;
    
    Size_T total_counter = 0;
    
    PD_ASSERT(pd_done.empty());
    PD_ASSERT(pd_todo.empty());
    
    using std::swap;
    pd_done.reserve(pd_list.size());
    pd_todo.reserve(pd_list.size());
  
    swap(pd_list,pd_todo);
//    // Push everything in pd_list onto pd_todo in reverse order.
//    while( !pd_list.empty() )
//    {
//        pd_todo.push_back( std::move(pd_list.back()) );
//        pd_list.pop_back();
//    }
    
    while( !pd_todo.empty() )
    {
        PD_T pd = std::move(pd_todo.back());
        pd_todo.pop_back();
        
        if( pd.InvalidQ() ) { continue; }
        
        if(  pd.proven_minimalQ )
        {
            if constexpr ( debugQ )
            {
                if( !pd.CheckAll() ) { pd_eprint("!pd.CheckAll() when pushed to pd_done."); };
            }
            
            if( pd.arc_count < pd.max_arc_count )
            {
                pd_done.push_back( pd.CreateCompressed() );
            }
            else
            {
                pd.ClearCache();
                pd_done.push_back( std::move(pd) );
            }
            continue;
        }
        
        Size_T old_counter = 0;
        Size_T counter = 0;
        
        // It is very likely that we change the diagram.
        // Also, a stale cache might spoil the simplification.
        // Thus, we proactively delete the cache.
        pd.ClearCache();
        
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
            
            // TODO: If we did strand moves before without success and if simplify_local_changes == 0, then we can break here, too.
            
            if( pd.InvalidQ() ) { break; }
            
            if( !simplify_strandsQ ) { break; }
            
            Size_T o_changes;
            // Reroute overstrands.
            {
                PD_PRINT("Construct StrandSimplier");
                StrandSimplier_T S (*this,pd);
                PD_VALPRINT("pd.max_crossing_count",pd.max_crossing_count);

                o_changes = S.SimplifyStrands(true,dist);
                counter += o_changes;
                
                if( pd.InvalidQ() ) { break; }
                
                if( (o_changes > Size_T(0)) )
                {
                    if( compressQ ) { pd.ConditionalCompress(); } else { pd.ClearCache(); }
                    // TODO: Is clearing the cache really necessary? For example, ArcLeftDarc should still be valid.
                }
            }

            // We can get an invalid diagram if we split off the last component.
            PD_ASSERT(pd.CheckAll());
            
            Size_T u_changes;
            // Reroute overstrands.
            {
                PD_PRINT("Construct StrandSimplier");
                StrandSimplier_T S (*this,pd);
                PD_VALPRINT("pd.max_crossing_count",pd.max_crossing_count);
                
                u_changes = S.SimplifyStrands(false,dist);
                counter += u_changes;
                
                if( pd.InvalidQ() ) { break; }
                
                if( (u_changes > Size_T(0)) )
                {
                    if( compressQ ) { pd.ConditionalCompress(); } else { pd.ClearCache(); }
                    // TODO: Is clearing the cache really necessary? For example, ArcLeftDarc should still be valid.
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
        
        total_counter += counter;

        if( pd.InvalidQ() ) { continue; }
        
        PD_ASSERT(pd.CheckAll());
        
        if constexpr ( debugQ )
        {
            if( !pd.CheckAll() ) { pd_eprint("!pd.CheckAll() after simplification."); };
        }
        
        // TODO: Maybe this is pointless if we call DisconnectSummands() soon.
        if( counter > Size_T(0) )
        {
            // Here we finally do want a fully compressed diagram.
            //  --------------------+
            //                      v
            if( compressQ ) { pd.Compress(); } else { pd.ClearCache(); }
        }
        
        if( pd.CrossingCount() == Int(0) )
        {
            pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
            continue;
        }
        
        
        Size_T disconnects = disconnectQ ? Disconnect(pd) : Size_T(0);
        total_counter += disconnects;
        
//        // We have done as much as we can. Let's consider this diagram done.
//        pd_done.push_back( pd.CreateCompressed() );
        
        
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint("!pd.CheckAll() failed after Disconnect."); }
        }
        
        if( (disconnects > Size_T(0)) || pd.DiagramComponentCount() > Int(1) )
        {
            // Split the diagrams in to diagram components and push them to pd_todo for further simplification.
            Split( std::move(pd), pd_todo );
            continue;
        }
        else
        {
            if( disconnectQ && pd.AlternatingQ() ) { pd.proven_minimalQ = true; }
            
            if( pd.crossing_count < pd.max_crossing_count )
            {
                pd_done.push_back( pd.CreateCompressed() );
            }
            else
            {
                pd.ClearCache();
                
                pd_done.push_back( std::move(pd) );
            }
            continue;
        }
    }  // while( !pd_todo.empty() )
    
    if constexpr ( debugQ )
    {
        if( !pd_list.empty() ) { pd_eprint("!pd_list.empty()"); };
        if( !pd_todo.empty() ) { pd_eprint("!pd_todo.empty()"); };
    }

    swap( pd_list, pd_done );
    
    // Sort big diagrams in front.
    Sort(
        &pd_list[0],
        &pd_list[pd_list.size()],
        []( cref<PD_T> pd_0, cref<PD_T> pd_1 )
        {
            return pd_0.CrossingCount() > pd_1.CrossingCount();
        }
    );
    
    if( total_counter > Size_T(0) ) { this->ClearCache(); }
    
    return total_counter;
}
