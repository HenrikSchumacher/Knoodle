public:

struct Simplify_Args_T
{
    Size_T local_opt_level        = 0;
    DijkstraStrategy_T strategy   = DijkstraStrategy_T::Bidirectional;
    Int    start_max_dist         = Scalar::Max<Int>;
    Int    final_max_dist         = Scalar::Max<Int>;
//    bool   exhaust_strands_firstQ = true;
//    bool restart_after_successQ   = true;
//    bool restart_after_failureQ   = true;
//    bool restart_walk_backQ       = true;
//    bool restart_change_typeQ     = true;
//    bool reroute_markedQ          = false;
    bool disconnectQ              = true;
    bool splitQ                   = true;
    bool compressQ                = true;
//    bool compress_oftenQ          = false;
    
    Energy_T     reapr_energy            = Energy_T::TV;
    Compaction_T reapr_compaction_method = Compaction_T::Length_MCF;
    Size_T       reapr_embedding_trials  = 25;
    Size_T       reapr_rotation_trials   =  1;
//    Reapr_T::Settings_T reapr_settings  = typename Reapr_T::Settings_T();
};

friend std::string ToString( cref<Simplify_Args_T> args )
{
    return std::string("{")
            + ".local_opt_level = " + ToString(args.local_opt_level)
            + ",.strategy = " + ToString(args.strategy)
            + ", .start_max_dist = " + ToString(args.start_max_dist)
            + ", .final_max_dist = " + ToString(args.final_max_dist)
//            + ", .exhaust_strands_firstQ = " + ToString(args.exhaust_strands_firstQ)
//            + ", .restart_after_successQ = " + ToString(args.restart_after_successQ)
//            + ", .restart_after_failureQ = " + ToString(args.restart_after_failureQ)
//            + ", .restart_walk_backQ = " + ToString(args.restart_walk_backQ)
//            + ", .restart_change_typeQ = " + ToString(args.restart_change_typeQ)
//            + ", .reroute_markedQ = " + ToString(args.reroute_markedQ)
            + ", .disconnectQ = " + ToString(args.disconnectQ)
            + ", .splitQ = " + ToString(args.splitQ)
            + ", .compressQ = " + ToString(args.compressQ)
//            + ", .compress_oftenQ = " + ToString(args.compress_oftenQ)
            + ", .reapr_embedding_trials = " + ToString(args.reapr_embedding_trials)
            + ", .reapr_rotation_trials = " + ToString(args.reapr_rotation_trials)
    + "}";
}

// Do some rerouting first, but disconnect and split early to divide-and-conquer.
Size_T Simplify( cref<Simplify_Args_T> args = Simplify_Args_T() )
{
    if( DiagramCount() == Int(0) ) { return 0; }
    
//    return Simplify_impl<15>(args);
    
//    int flag = (args.restart_after_successQ << 0)
//             | (args.restart_after_failureQ << 1)
//             | (args.restart_walk_backQ     << 2)
//             | (args.restart_change_typeQ   << 3);
    
    switch ( args.local_opt_level )
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
            eprint( MethodName("Simplify") + ": local_opt_level = " + ToString(args.local_opt_level) + " is invalid." );
            return 0;
        }
    }
    
    return 0;
}

private:

template<Size_T local_opt_level>
Size_T Simplify_impl( cref<Simplify_Args_T> args )
{
//    constexpr bool debugQ = true;
    
    using TArgs_T = StrandSimplifier_T::SimplifyStrands_TArgs;
    constexpr TArgs_T targs = TArgs_T();

    [[maybe_unused]] auto tag = [this]()
    {
        return this->MethodName("Simplify_impl") + "<" + ToString(local_opt_level) + ">";
    };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("args",ToString(args));
#endif
    
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
    
    using std::swap;
    pd_done.reserve(pd_list.size());
    pd_todo.reserve(pd_list.size());
  
    swap(pd_list,pd_todo);

    Reapr_T reapr ({
        .energy              = args.reapr_energy,
        .ortho_draw_settings = { .compaction_method = args.reapr_compaction_method }
    });

    PD_List_T reapr_list;
    
    while( !pd_todo.empty() )
    {
        PD_T pd = std::move(pd_todo.back());
        pd_todo.pop_back();
        
        // We allow local pattern optimization only in the very first pass for each diagram. It won't help at all in Rattle.
        if( args.local_opt_level > Size_T(0) )
        {
            change_count += ArcSimplifier2<Int,local_opt_level,true>( *this, pd, Scalar::Max<Int>, args.compressQ )();
        }

        auto [strand_change_count, disconnect_count] = this->template SimplifyDiagrammatically<debugQ,targs>( S, pd, args );
        change_count += strand_change_count;
        change_count += disconnect_count;
        
        if( pd.InvalidQ() ) { continue; }

        // If the StrandSimplifier did not find anything, then Disconnect produces a reduced diagram.
        const bool proven_reducedQ = args.disconnectQ && (strand_change_count == Size_T(0));
        
        if( proven_reducedQ )
        {
            PD_ASSERT(pd.ReducedQ());
        }
        
        // Split the diagrams into diagram components and push them to pd_todo for further simplification.

        // Caution: Split is allowed to push minimal diagrams to pd_done.
        if( (strand_change_count != Size_T(0)) || (disconnect_count != Size_T(0)) )
        {
            // If anything upstream changed, then we should better continue working on the split diagrams.
            if( args.splitQ )
            {
                change_count += Split( std::move(pd), pd_todo, proven_reducedQ );
                
            }
            else
            {
                if( proven_reducedQ && pd.AlternatingQ() ) { pd.proven_minimalQ = true; }
                
                PushDiagramToDo( std::move(pd) );
            }
            continue;
        }
        
        // No changes were found so far. We can try reapr or we have to stop here.
        if( (args.reapr_embedding_trials > Size_T(0)) && (args.reapr_rotation_trials > Size_T(0)) )
        {
            if( args.splitQ )
            {
                PD_ASSERT(reapr_list.empty());
                
                change_count += Split( std::move(pd), reapr_list, proven_reducedQ );
                
                // If proven_reducedQ, then Split already filtered out minimal diagrams.
                while( !reapr_list.empty() )
                {
                    PD_T pd_reapr = std::move(reapr_list.back());
                    reapr_list.pop_back();
                    change_count += this->template Rattle<debugQ,targs>( S, reapr, std::move(pd_reapr), args );
                }
                
                PD_ASSERT(reapr_list.empty());
            }
            else
            {
                if( pd.DiagramComponentCount() <= Int(1) )
                {
                    this->template Rattle<debugQ,targs>( S, reapr, std::move(pd), args );
                }
                else
                {
                    // We are not allowed to split; so we cannot do better than pushing this onto the "done pile.
                    PushDiagramDone( std::move(pd) );
                }
            }
        }
        else
        {
            // If no changes were found and if we do not want reapr, then we cannot do better than splittinh and pushing to pd_done.
            if( args.splitQ )
            {
                change_count += Split( std::move(pd), pd_done, proven_reducedQ );
            }
            else
            {
                PushDiagramDone( std::move(pd) );
            }
        }
        
    }  // while( !pd_todo.empty() )
    
    PD_ASSERT(pd_list.empty());
    PD_ASSERT(pd_todo.empty());
    
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
    
    PD_ASSERT(this->CheckAll());
    
    return change_count;
}



template<bool debugQ, StrandSimplifier_T::SimplifyStrands_TArgs targs>
Size_T Rattle(
    mref<StrandSimplifier_T> S, mref<Reapr_T> reapr, PD_T && pd, cref<Simplify_Args_T> args
)
{
    [[maybe_unused]] auto tag = [this]() { return this->MethodName("Rattle"); };
    
    TOOLS_PTIMER(timer,tag());
    
    PD_ASSERT(pd.ValidQ());
    PD_ASSERT(!pd.ProvenMinimalQ());
    PD_ASSERT(pd.CrossingCount() >= Int(1));
    PD_ASSERT(pd.DiagramComponentCount() == Int(1));
    
//    // DEBUGGING.
//    if( pd.MinimalQ() )
//    {
//        wprint(MethodName("Rattle") + ": Input diagram is minimal (crossing_count = " + ToString(pd.crossing_count) + "). No point calling Rattle.");
//    }
    
    PD_T pd_1;
    
    Size_T strand_change_count = 0;
    Size_T disconnect_count    = 0;
    
    constexpr Size_T max_projection_iter = 10;
    bool progressQ = false;
    
    for( Size_T iter = 0; iter < args.reapr_embedding_trials; ++iter )
    {
        LinkEmbedding_T emb = reapr.Embedding(pd);
        
        for( Size_T rot = 0; rot < args.reapr_rotation_trials; ++rot )
        {
            Size_T projection_iter = 0;
            int projection_flag = 0;
            
            do
            {
                ++projection_iter;
                emb.Rotate( reapr.RandomRotation() );
                projection_flag = emb.FindIntersections();
            }
            while( (projection_flag!=0) && (projection_iter < max_projection_iter) );
            
            if( projection_flag != 0 )
            {
                eprint(MethodName("Rattle") + ": " + emb.MethodName("FindIntersections")+ " returned invalid status flag for " + ToString(max_projection_iter) + " random rotation matrices. Something must be wrong. Returning an invalid diagram. Check your results carefully.");
                
                return Size_T(0);
            }
            
            PDC_T pdc_new = PDC_T::FromLinkEmbedding(emb);
            
            PD_ASSERT(pdc_new.CheckAll());
            
            // We might get some unlinks here.
            for( Size_T i = 1; i < pdc_new.pd_list.size(); ++i )
            {
                PD_ASSERT(pdc_new.pd_list[i].ProvenUnknotQ());
                PushDiagramDone( std::move(pdc_new.pd_list[i]) );
            }
            
            pd_1 = std::move(pdc_new.pd_list[0]);
            
            std::tie(strand_change_count,disconnect_count) = this->template SimplifyDiagrammatically<debugQ,targs>(S, pd_1, args);
    
            
            // TODO: Can we improve these conditions?
            // TODO: E.g., we could call it a success, if pd_1 is reduced and alternating.
            progressQ = ( pd_1.CrossingCount() < pd.CrossingCount() )
                        ||
                        (disconnect_count > Size_T(0))
                        ||
                        (pd_1.DiagramComponentCount() > Int(1));
            
//            // DEBUGGING.
//            if( !progressQ && pd_1.MinimalQ() )
//            {
//                wprint("Found minimal diagram with " + ToString(pd_1.crossing_count) + " crossings, but we discarded it.");
//            }
            
            if( progressQ ) { break; }
        }
        
        if( progressQ ) { break; }
    }
    
    if( pd_1.InvalidQ() ) { return strand_change_count + disconnect_count; }
    
    Size_T split_count = 0;
    
    if( progressQ )
    {
        // If the StrandSimplifier did not find anything, then Disconnect produces a reduced diagram.
        const bool proven_reducedQ = args.disconnectQ && (strand_change_count == Size_T(0));
        
        if( proven_reducedQ ) { PD_ASSERT(pd.ReducedQ());         }
        
        if( args.splitQ )
        {
            split_count = Split( std::move(pd_1), pd_todo, proven_reducedQ );
        }
        else
        {
            if( proven_reducedQ && pd_1.AlternatingQ() ) { pd_1.proven_minimalQ = true; }
            
            PushDiagramToDo( std::move(pd_1) );
        }
    }
    else
    {
        // If splits are allowed, then this is already split.
        PushDiagramDone( std::move(pd) );
    }
    
    return strand_change_count + disconnect_count + split_count;
}




template<bool debugQ, StrandSimplifier_T::SimplifyStrands_TArgs targs>
std::pair<Size_T,Size_T> SimplifyDiagrammatically(
    mref<StrandSimplifier_T> S, mref<PD_T> pd, cref<Simplify_Args_T> args
)
{
    [[maybe_unused]] auto tag = [this](){ return this->MethodName("SimplifyDiagrammatically"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( pd.InvalidQ() ) { return {Size_T(0),Size_T(0)}; }
    
    if(  pd.proven_minimalQ )
    {
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed when pushed to pd_done."); };
        }
        
        if( pd.crossing_count < pd.max_crossing_count )
        {
            PushDiagramDone( pd.CreateCompressed() );
        }
        else
        {
            PushDiagramDone( std::move(pd) );
        }
        
        pd = PD_T::InvalidDiagram();
        return {Size_T(0),Size_T(0)};
    }
    
    PD_ASSERT(pd.ValidQ());
    
    // It is very likely that we change the diagram.
    // Also, a stale cache might spoil the simplification.
    // Thus, we proactively delete the cache.
    pd.ClearCache();

    // Not clear whether local patterns are beneficial.
//        ArcSimplifier2<Int,3,true> A ( *this, pd, Scalar::Max<Size_T>, args.compressQ );
//        Size_T local_change_count =  A();
//        change_count += local_change_count;
    
    
    const Int max_dist = Scalar::Max<Int>;
    
    Size_T strand_change_count = 0;
    
    do
    {
        strand_change_count = 0;
        
        strand_change_count += S.template SimplifyStrands<targs>(pd,{
            .max_dist  = max_dist,
            .overQ     = true,
            .compressQ = args.compressQ
        });
                    
        if( pd.InvalidQ() ) { break; }
        
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyOverStrands."); };
        }
        
        if constexpr ( !targs.restart_change_typeQ || !targs.restart_after_successQ || !targs.restart_after_failureQ )
        {
            // TODO: Filter out duplicate unknots.
            if( pd.crossing_count <= Int(1) )
            {
                CreateUnlink(pd.last_color_deactivated);
                pd = PD_T::InvalidDiagram();
                break;
            }
            
            // Reroute understrands.
            strand_change_count += S.template SimplifyStrands<targs>(pd,{
                .max_dist  = max_dist,
                .overQ     = false,
                .compressQ = args.compressQ
            });
            
            if( pd.InvalidQ() ) { break; }
            
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyUnderStrands."); };
            }
        }
        
        // TODO: Filter out duplicate unknots.
        if( pd.crossing_count <= Int(1) )
        {
            CreateUnlink(pd.last_color_deactivated);
            pd = PD_T::InvalidDiagram();
            break;
        }
    }
    while( /*args.exhaust_strands_firstQ &&*/ (strand_change_count > Size_T(0)) );

    if( pd.InvalidQ() ) { return {strand_change_count,Size_T(0)}; }
    
    Size_T disconnect_count = 0;
    
    // Caution: Disconnect is allowed to push some small diagrams to pd_done.
    if( args.disconnectQ )
    {
        disconnect_count = Disconnect(pd);
    }
    
    return {strand_change_count,disconnect_count};
}


//PD_T ReaprProjection( mref<Reapr_T> reapr, cref<PD_T> pd )
//{
//    TOOLS_PTIMER(timer,MethodName("ReaprProjection"));
//    
//    constexpr Size_T max_projection_iter = 10;
//    Size_T projection_iter = 0;
//    int projection_flag;
//    LinkEmbedding_T emb = reapr.Embedding(pd);
//    
//    do
//    {
//        ++projection_iter;
//        emb.Rotate( reapr.RandomRotation() );
//        projection_flag = emb.FindIntersections();
//    }
//    while( (projection_flag!=0) && (projection_iter < max_projection_iter) );
//    
//
//    if( projection_flag != 0 )
//    {
//        eprint(MethodName("Projection") + ": Link_2D::FindIntersections returned status flag " + ToString(projection_flag) + " != 0 for " + ToString(max_projection_iter) + " random rotation matrices. Something must be wrong. Returning an invalid diagram. Check you results carefully.");
//        
//        return PD_T::InvalidDiagram();
//    }
//    
//    PDC_T pdc_new = PDC_T::FromLinkEmbedding(emb);
//    
//    PD_ASSERT(pdc_new.CheckAll());
//    
//    // We might get some unlinks here.
//    for( Size_T i = 1; i < pdc_new.pd_list.size(); ++i )
//    {
//        PD_ASSERT(pdc_new.pd_list[i].ProvenUnknotQ());
//        PushDiagramDone( std::move(pdc_new.pd_list[i]) );
//    }
//    
//    return std::move(pdc_new.pd_list[0]);
//}
