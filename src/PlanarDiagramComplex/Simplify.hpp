public:


struct Simplify_Args_T
{
    Int                local_opt_level  = 4;
    Size_T             local_max_iter   = Scalar::Max<Size_T>;
    DijkstraStrategy_T strategy         = DijkstraStrategy_T::Bidirectional;
    Int                start_max_dist   = Scalar::Max<Int>;
    Int                final_max_dist   = Scalar::Max<Int>;
    bool               disconnectQ      = true;
    bool               splitQ           = true;
    bool               compressQ        = true;
};

friend std::string ToString( cref<Simplify_Args_T> args )
{
    return "{.local_opt_level = " + ToString(args.local_opt_level)
         + ",.local_max_iter = " + ToString(args.local_max_iter)
         + ",.strategy = " + ToString(args.strategy)
         + ",.start_max_dist = " + ToString(args.start_max_dist)
         + ",.final_max_dist = " + ToString(args.final_max_dist)
         + ",.disconnectQ = " + ToString(args.disconnectQ)
         + ",.splitQ = " + ToString(args.splitQ)
         + ",.compressQ = " + ToString(args.compressQ)
         + "}";
}

Size_T Simplify( cref<Simplify_Args_T> args = Simplify_Args_T() )
{
    if( DiagramCount() == Int(0) ) { return 0; }
    
    const Int level = Clamp(args.local_opt_level, Int(0), Int(4));
    
    switch ( level )
    {
        case 0:
        {
            return Simplify_impl<0>(args);
        }
        case 1:
        {
            return Simplify_impl<1>(args);
        }
        case 2:
        {
            return Simplify_impl<2>(args);
        }
        case 3:
        {
            return Simplify_impl<3>(args);
        }
        case 4:
        {
            return Simplify_impl<4>(args);
        }
        default:
        {
            eprint( MethodName("Simplify") + ": local_opt_level = " + ToString(level) + " is invalid." );
            return 0;
        }
    }
    
    return 0;
}

private:

template<Int local_opt_level>
Size_T Simplify_impl( cref<Simplify_Args_T> args )
{
    [[maybe_unused]] auto tag = [this]()
    {
        return this->MethodName("Simplify_impl")
        + "<" + ToString(local_opt_level)
        + ">";
    };
    
//    [[maybe_unused]] auto tag = [&args,this]()
//    {
//        return this->MethodName("Simplify_impl")
//        + "<" + ToString(local_opt_level)
//        + ">"
//        +"({ .local_opt_level = " + ToString(args.local_opt_level)
//        + ", .local_max_iter = " + ToString(args.local_max_iter)
//        + ", .strategy = " + ToString(args.strategy)
//        + ", .start_max_dist = " + ToString(args.start_max_dist)
//        + ", .final_max_dist = " + ToString(args.final_max_dist)
//        + ", .disconnectQ = " + ToString(args.disconnectQ)
//        + ", .splitQ = " + ToString(args.splitQ)
//        + ", .compressQ = " + ToString(args.compressQ)
//        + "})";
//    };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("args",ToString(args));
#endif
    
//    constexpr bool debugQ = true;
    
    if constexpr (debugQ)
    {
        wprint(tag()+": Debug mode active.");
    }

    using ArcSimplifier_Link_T  = ArcSimplifier2<Int,local_opt_level,true>;
    
    // By intializing S here, it will have enough internal memory for all planar diagrams.
    PD_PRINT("Request StrandSimplifier");
    mref<StrandSimplifier_T> S = StrandSimplifier(args.strategy);
    
#ifdef PD_COUNTERS
    S.ResetCounters();
#endif
    
//    // We have to store this value here, because the result of MaxMaxCrossingCount changes if we change pd_list (which we will do frequently).
//    const Int pdc_max_crossing_count = MaxMaxCrossingCount();
    
    Size_T total_change_count = 0;
    
    PD_ASSERT(pd_done.empty());
    PD_ASSERT(pd_todo.empty());
    
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
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed when pushed to pd_done."); };
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
        
//        const Int link_component_count = pd.LinkComponentCount();
//        const bool mult_compQ = (link_component_count > Int(1));
        // It is very likely that we change the diagram.
        // Also, a stale cache might spoil the simplification.
        // Thus, we proactively delete the cache.
        pd.ClearCache();
        // At least this information we can keep.
//        pd.SetCache("LinkComponentCount",link_component_count);
        
        Size_T change_count_old = 0;
        Size_T change_count     = 0;
        Size_T change_count_loc = 0; // counter for local moves.
        // Initialize to something != 0 to signal that we have not yet done any strand simplification.
        Size_T change_count_o   = 1; // counter for overstrand moves.
        Size_T change_count_u   = 1; // counter for understrand moves.
        
        Int max_dist = Max(Int(2),args.start_max_dist);
        const bool simplify_localQ   = (local_opt_level > Int(0));
        const bool simplify_strandsQ = (args.final_max_dist   > Int(0));
        
        do
        {
            change_count_old = change_count;
            
            max_dist = Min(max_dist,args.final_max_dist);
            
            // Since ArcSimplifier_T performs only inexpensive tests, we should use it first.
            if( simplify_localQ && ( (change_count_o > 0) ||  (change_count_u > 0) ) )
            {
                ArcSimplifier_Link_T A ( *this, pd, args.local_max_iter, args.compressQ );
                change_count_loc = A();

                change_count += change_count_loc;
                
                if constexpr (debugQ)
                {
                    if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after local simplification."); };
                }
            }
            
            // If we did strand moves before without success and if simplify_local_changes == 0, then we can break here, too.
            if( (change_count_o == 0) && (change_count_u == 0) && (change_count_loc == 0) ) { break; }
            
            if( pd.InvalidQ() ) { break; }
            
            if( !simplify_strandsQ ) { break; }
            
            // Reroute overstrands.
            change_count_o = S.SimplifyStrands(pd,true,max_dist);
            
            change_count += change_count_o;
            if( pd.InvalidQ() ) { break; }
            if( (change_count_o > 0) && args.compressQ ) { pd.ConditionalCompress(); }

            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyOverStrands."); };
            }
            
            // If we did strand moves before without success and if simplify_local_changes == 0, then we can break here, too.
            if( (change_count_o == 0) && (change_count_u == 0) && (change_count_loc == 0) ) { break; }
            
            // Reroute understrands.
            change_count_u = S.SimplifyStrands(pd,false,max_dist);
            
            change_count += change_count_u;
            if( pd.InvalidQ() ) { break; }
            if( (change_count_u > Size_T(0) ) && args.compressQ) { pd.ConditionalCompress(); }
            
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyUnderStrands."); };
            }
            
            if( max_dist <= Scalar::Max<Int> / Int(2) )
            {
                max_dist *= Int(2);
            }
            else
            {
                max_dist = Scalar::Max<Int>;
            }
        }
        while(
              (change_count > change_count_old)
              ||
              ( (max_dist <= args.final_max_dist) && (max_dist < pd.arc_count) )
        );
        
        total_change_count += change_count;

        if( pd.InvalidQ() ) { continue; }
        
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint("pd.CheckAll() failed after simplification."); };
        }
        
        if( pd.CrossingCount() <= Int(1) )
        {
            pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
            continue;
        }
        
        if( change_count > Size_T(0) ) { pd.ClearCache(); }
        
        Size_T disconnect_count = args.disconnectQ ? Disconnect(pd) : Size_T(0);
        
        if constexpr (debugQ)
        {
            if( args.disconnectQ && !pd.CheckAll() )
            {
                pd_eprint("CheckAll() failed after disconnecting.");
            }
            
            if( args.splitQ )
            {
                logprint("Preparing Split now.");
                
                if( !pd.CheckAll() ) { pd_eprint("pd.CheckAll() failed before splitting."); }
                
                // DEBUGGING
                TOOLS_LOGDUMP(pd.DiagramComponentLinkComponentMatrix().ToTensor2());
                TOOLS_LOGDUMP(pd.DiagramComponentCount());
                
                TOOLS_LOGDUMP(args.splitQ);
                TOOLS_LOGDUMP((disconnect_count > Size_T(0)));
                TOOLS_LOGDUMP((pd.DiagramComponentCount() > Int(1)));
                TOOLS_LOGDUMP((args.splitQ && ( (disconnect_count > Size_T(0)) || (pd.DiagramComponentCount() > Int(1)) )));
                
                TOOLS_LOGDUMP(pd.cache.size());
                TOOLS_LOGDUMP(pd.CacheKeys());
            }
        }
        
        if(
            args.splitQ
            &&
            ( (disconnect_count > Size_T(0)) || (pd.DiagramComponentCount() > Int(1)) )
        )
        {
            // Split the diagrams into diagram components and push them to pd_todo for further simplification.
            Split( std::move(pd), pd_todo, args.disconnectQ );
            continue;
        }
        else
        {
            if( args.disconnectQ && pd.AlternatingQ() )
            {
                pd.proven_minimalQ = true;
            }
            
            if( pd.crossing_count < pd.max_crossing_count )
            {
                pd.Compress();
                
                if constexpr (debugQ)
                {
                    if( !pd.CheckAll() ) { pd_eprint("pd.CheckAll() failed after compression."); };
                }
            }
            // Should be unnecessary.
            pd.ClearCache();
            pd_done.push_back( std::move(pd) );
        }
        
    }  // while( !pd_todo.empty() )
    
    if constexpr (debugQ)
    {
        if( !pd_list.empty() ) { pd_eprint("!pd_list.empty()"); };
        if( !pd_todo.empty() ) { pd_eprint("!pd_todo.empty()"); };
    }

    swap( pd_list, pd_done );

#ifdef PD_COUNTERS
    // We need to save the counters from being erased by this->ClearCache().
    auto S_buffer = std::move(this->GetCache<StrandSimplifier_T>("StrandSimplifier"));
#endif
    
    if( total_change_count > Size_T(0) )
    {
        SortByCrossingCount();
        this->ClearCache();
    }

#ifdef PD_COUNTERS
    this->SetCache("StrandSimplifier",std::move(S_buffer));
#endif
    
    return total_change_count;
}
