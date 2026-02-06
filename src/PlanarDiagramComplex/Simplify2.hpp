public:

struct Simplify2_Args_T
{
    DijkstraStrategy_T strategy = DijkstraStrategy_T::Bidirectional;
    Int  start_max_dist         = Scalar::Max<Int>;
    Int  final_max_dist         = Scalar::Max<Int>;
    bool exhaust_strands_firstQ = true;
    bool restart_after_successQ = true;
    bool restart_after_failureQ = true;
    bool restart_walk_backQ     = false;
    bool restart_change_typeQ   = true;
    bool reroute_markedQ        = false;
    bool disconnectQ            = true;
    bool splitQ                 = true;
    bool compressQ              = true;
    bool compress_oftenQ        = false ;
};

friend std::string ToString( cref<Simplify2_Args_T> args )
{
    return "{ .strategy = " + ToString(args.strategy)
            + ", .start_max_dist = " + ToString(args.start_max_dist)
            + ", .final_max_dist = " + ToString(args.final_max_dist)
            + ", .exhaust_strands_firstQ = " + ToString(args.exhaust_strands_firstQ)
            + ", .restart_after_successQ = " + ToString(args.restart_after_successQ)
            + ", .restart_after_failureQ = " + ToString(args.restart_after_failureQ)
            + ", .restart_walk_backQ = " + ToString(args.restart_walk_backQ)
            + ", .restart_change_typeQ = " + ToString(args.restart_change_typeQ)
            + ", .reroute_markedQ = " + ToString(args.reroute_markedQ)
            + ", .disconnectQ = " + ToString(args.disconnectQ)
            + ", .splitQ = " + ToString(args.splitQ)
            + ", .compressQ = " + ToString(args.compressQ)
            + ", .compress_oftenQ = " + ToString(args.compress_oftenQ)
    + "}";
}

// Do some rerouting first, but disconnect and split early to divide-and-conquer.
Size_T Simplify2( cref<Simplify2_Args_T> args = Simplify2_Args_T() )
{
    if( DiagramCount() == Int(0) ) { return 0; }
    
    int flag = (args.restart_after_successQ << 0)
             | (args.restart_after_failureQ << 1)
             | (args.restart_walk_backQ     << 2)
             | (args.restart_change_typeQ   << 3);
    
    switch ( flag )
    {
        case 0:
        {
            return Simplify2_impl<0>(args);
        }
        case 1:
        {
            return Simplify2_impl<1>(args);
        }
        case 2:
        {
            return Simplify2_impl<2>(args);
        }
        case 3:
        {
            return Simplify2_impl<3>(args);
        }
        case 4:
        {
            return Simplify2_impl<4>(args);
        }
        case 5:
        {
            return Simplify2_impl<5>(args);
        }
        case 6:
        {
            return Simplify2_impl<6>(args);
        }
        case 7:
        {
            return Simplify2_impl<7>(args);
        }
        case 8:
        {
            return Simplify2_impl<8>(args);
        }
        case 9:
        {
            return Simplify2_impl<9>(args);
        }
        case 10:
        {
            return Simplify2_impl<10>(args);
        }
        case 11:
        {
            return Simplify2_impl<11>(args);
        }
        case 12:
        {
            return Simplify2_impl<12>(args);
        }
        case 13:
        {
            return Simplify2_impl<13>(args);
        }
        case 14:
        {
            return Simplify2_impl<14>(args);
        }
        case 15:
        {
            return Simplify2_impl<15>(args);
        }
        default:
        {
            eprint( MethodName("Simplify2") + ": flag = " + ToString(flag) + " is invalid." );
            return 0;
        }
    }
    
    return 0;
}

private:

template<int flag>
Size_T Simplify2_impl( cref<Simplify2_Args_T> args )
{
    constexpr bool restart_after_successQ = (flag >> 0) & 1;
    constexpr bool restart_after_failureQ = (flag >> 1) & 1;
    constexpr bool restart_walk_backQ     = (flag >> 2) & 1;
    constexpr bool restart_change_typeQ   = (flag >> 3) & 1;

    [[maybe_unused]] auto tag = [this]()
    {
        return this->MethodName("Simplify2_impl") + "<" + ToString(flag) + ">";
    };
    
//    [[maybe_unused]] auto tag = [&args]()
//    {
//        return MethodName("Simplify2_impl") + "<" + ToString(flag) + ">" + "(" + ToString(args) + ")";
//    };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("args",ToString(args));
#endif
    
//    constexpr bool debugQ = true;
    
    if constexpr (debugQ) { wprint(tag()+": Debug mode active."); }
    
    // By intializing S here, it will have enough internal memory for all planar diagrams.
    PD_PRINT("Request StrandSimplifier");
    mref<StrandSimplifier_T> S = StrandSimplifier(args.strategy);
    
#ifdef PD_COUNTERS
    S.ResetCounters();
#endif
    
//    // We have to store this value here, because the result of MaxMaxCrossingCount changes if we change pd_list (which we will do frequently).
//    const Int pdc_max_crossing_count = MaxMaxCrossingCount();
    
    Size_T change_count = 0;
    
    PD_ASSERT(pd_done.empty());
    PD_ASSERT(pd_todo.empty());
    
    using std::swap;
    pd_done.reserve(pd_list.size());
    pd_todo.reserve(pd_list.size());
  
    swap(pd_list,pd_todo);
    
    const Int max_dist = Scalar::Max<Int>;
    
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
                pd_done.push_back( std::move(pd) );
            }
            continue;
        }
        
        // It is very likely that we change the diagram.
        // Also, a stale cache might spoil the simplification.
        // Thus, we proactively delete the cache.
        pd.ClearCache();
  
        // Not clear whether local patterns are beneficial.
//        ArcSimplifier2<Int,3,true> A ( *this, pd, Scalar::Max<Size_T>, args.compressQ );
//        Size_T local_change_count =  A();
//        change_count += local_change_count;
        
        Size_T strand_change_count = 0;
        
        do
        {
            strand_change_count = 0;
            
            strand_change_count += S.template SimplifyStrands2<{
                .restart_after_successQ = restart_after_successQ,
                .restart_after_failureQ = restart_after_failureQ,
                .restart_walk_backQ     = restart_walk_backQ,
                .restart_change_typeQ   = restart_change_typeQ
            }>(pd,{
                .max_dist               = max_dist,
                .overQ                  = true,
                .reroute_markedQ        = args.reroute_markedQ,
                .compressQ              = args.compressQ,
                .compress_oftenQ        = args.compress_oftenQ
            });
                        
            if( pd.InvalidQ() ) { break; }
            
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyOverStrands."); };
            }
            
            if constexpr ( !restart_change_typeQ || !restart_after_successQ || !restart_after_failureQ )
            {
                // TODO: Filter out duplicate unknots.
                if( pd.crossing_count <= Int(1) )
                {
                    pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
                    pd = PD_T::InvalidDiagram();
                    break;
                }
                
                // Reroute understrands.
                strand_change_count += S.template SimplifyStrands2<{
                    .restart_after_successQ = restart_after_successQ,
                    .restart_after_failureQ = restart_after_failureQ,
                    .restart_walk_backQ     = restart_walk_backQ,
                    .restart_change_typeQ   = restart_change_typeQ
                }>(pd,{
                    .max_dist               = max_dist,
                    .overQ                  = false,
                    .reroute_markedQ        = args.reroute_markedQ,
                    .compressQ              = args.compressQ,
                    .compress_oftenQ        = args.compress_oftenQ
                });
                
                if( pd.InvalidQ() ) { break; }
                
                if constexpr (debugQ)
                {
                    if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyUnderStrands."); };
                }
            }
            
//            // DEBUGGING
//            TOOLS_DUMP(CountTrefoils(pd));

            
            change_count += strand_change_count;
            
            // TODO: Filter out duplicate unknots.
            if( pd.crossing_count <= Int(1) )
            {
                pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
                pd = PD_T::InvalidDiagram();
                break;
            }
        }
        while( args.exhaust_strands_firstQ && (strand_change_count > Size_T(0)) );

        if( pd.InvalidQ() ) { continue; }
        
        Size_T disconnect_count = 0;
        
        // Caution: Disconnect is allowed to push some small diagrams to pd_done.
        if( args.disconnectQ )
        {
            disconnect_count = Disconnect(pd);
        }
        change_count += disconnect_count;
        
        if( pd.InvalidQ() ) { continue; }
        
        // If the StrandSimplifier did not find anything, then Disconnect produces a reduced diagram.
        const bool proven_reducedQ = args.disconnectQ && (strand_change_count == Size_T(0));
        
        if( proven_reducedQ )
        {
            PD_ASSERT(pd.ReducedQ());
        }
        
        // Split the diagrams into diagram components and push them to pd_todo for further simplification.

        Size_T split_count = 0;
        // Caution: Split is allowed to push minimal diagrams to pd_done.
        if( (strand_change_count == Size_T(0)) && (disconnect_count == Size_T(0)) )
        {
            // If no changes were found, we cannot do better than pushing to pd_done.
            if( args.splitQ )
            {
                split_count = Split( std::move(pd), pd_done, proven_reducedQ );
            }
            else
            {
                pd_done.push_back( std::move(pd) );
            }
        }
        else
        {
            // If anything upstream changed, then we should better continue working on the split diagrams.
            if( args.splitQ )
            {
                split_count = Split( std::move(pd), pd_todo, proven_reducedQ );
            }
            else
            {
                pd_todo.push_back( std::move(pd) );
            }
        }
        change_count += split_count;
        
        PD_ASSERT(pd.InvalidQ());
        
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
    
    if( change_count > Size_T(0) )
    {
        SortByCrossingCount();
        this->ClearCache();
    }

#ifdef PD_COUNTERS
    this->SetCache("StrandSimplifier",std::move(S_buffer));
#endif
    
    return change_count;
}
