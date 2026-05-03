public:

struct Simplify_Args_T
{
    bool                compress_initialQ        = true;
    
    UInt8               local_opt_level          = 0;
    DijkstraStrategy_T  strategy                 = DijkstraStrategy_T::Bidirectional;
    Int                 start_max_dist           = Scalar::Max<Int>;
    Int                 final_max_dist           = Scalar::Max<Int>;
    
    bool                rerouteQ                 = true;
    bool                disconnectQ              = true;
    bool                splitQ                   = true;
    bool                compressQ                = true;
    Int                 compression_threshold    = 0;
    
    Size_T              embedding_trials         = 0;
    Size_T              rotation_trials          = 25;
    bool                permute_randomQ          = true;
    Energy_T            energy                   = Energy_T::TV;
    double              scaling                  = 1.;
    
    int                 randomize_bends          = 4;
    bool                randomize_virtual_edgesQ = true;
    Compaction_T        compaction_method        = Compaction_T::Length_MCF;
    
    bool                canonicalizeQ            = true;

//    Reapr_T::Settings_T reapr_settings  = typename Reapr_T::Settings_T();
};


friend std::string ToString( cref<Simplify_Args_T> args )
{
    return std::string("{ ")
            +   "compress_initialQ = " + ToString(args.compress_initialQ)
            + ", local_opt_level = " + ToString(args.local_opt_level)
            + ", strategy = " + ToString(args.strategy)
            + ", start_max_dist = " + ToString(args.start_max_dist)
            + ", final_max_dist = " + ToString(args.final_max_dist)
            + ", disconnectQ = " + ToString(args.disconnectQ)
            + ", splitQ = " + ToString(args.splitQ)
            + ", compressQ = " + ToString(args.compressQ)
            + ", compression_threshold = " + ToString(args.compression_threshold)
    
            + ", embedding_trials = " + ToString(args.embedding_trials)
            + ", rotation_trials = " + ToString(args.rotation_trials)
            + ", permute_randomQ = " + ToString(args.permute_randomQ)
            + ", energy = " + ToString(args.energy)
    
            + ", randomize_bends = " + ToString(args.randomize_bends)
            + ", randomize_virtual_edgesQ = " + ToString(args.randomize_virtual_edgesQ)
            + ", compaction_method = " + ToString(args.compaction_method)
    + " }";
}


/*!@brief Apply diagrammatic simplifications. If `arg.embedding_trials` and `arg.rotation_trials` are set to positive values, then also Reapr (construction of a 3D grid embedding, rotation, projection) is employed.
 */
template<PassSimplifier_T::SimplifyPasses_TArgs targs = typename PassSimplifier_T::SimplifyPasses_TArgs()>
Size_T Simplify( cref<Simplify_Args_T> args = Simplify_Args_T() )
{
    Reapr_T reapr ({
        .permute_randomQ     = args.permute_randomQ,
        .energy              = args.energy,
        .ortho_draw_settings = {
            .randomize_bends          = args.randomize_bends,
            .randomize_virtual_edgesQ = args.randomize_virtual_edgesQ,
            .compaction_method        = args.compaction_method
        },
        .scaling             = args.scaling
    });
    
    return Simplify<targs>( reapr, args );
}

/*!@brief Apply diagrammatic simplifications. If `arg.embedding_trials` and `arg.rotation_trials` are set to positive values, then also Reapr (construction of a 3D grid embedding, rotation, projection) is employed.
 *
 * Beware: The options of the `Reapr` instance `reapr` override some of the options in `args`.
 */
template<PassSimplifier_T::SimplifyPasses_TArgs targs = typename PassSimplifier_T::SimplifyPasses_TArgs()>
Size_T Simplify( mref<Reapr_T> reapr, cref<Simplify_Args_T> args = Simplify_Args_T() )
{
    TOOLS_PTIMER(timer,MethodName("Simplify"));
    
    if( DiagramCount() == Int(0) ) { return 0; }

    switch ( args.local_opt_level )
    {
        case 0:
        {
            return Simplify_impl<0,targs>(reapr,args);
        }
        case 1:
        {
            return Simplify_impl<1,targs>(reapr,args);
        }
        case 2:
        {
            return Simplify_impl<2,targs>(reapr,args);
        }
        case 3:
        {
            return Simplify_impl<3,targs>(reapr,args);
        }
        case 4:
        {
            return Simplify_impl<4,targs>(reapr,args);
        }
        default:
        {
            eprint( MethodName("Simplify") + ": local_opt_level = " + ToString(args.local_opt_level) + " is invalid." );
            return 0;
        }
    }
    
    return 0;
}


// Allows be to define and run several imlementation variants to test them
Size_T Simplify_Variant( cref<Simplify_Args_T> args = Simplify_Args_T(), Size_T variant = 0 )
{
    switch( variant )
    {
        case 0:
        {
            return Simplify(args);
        }
        case 1:
        {
            return this->template Simplify<{
                .restart_after_successQ = false,
                .restart_after_failureQ = false,
                .restart_walk_backQ     = false,
                .interleave_over_underQ = false,
                .R_II_blockingQ         = false,
                .R_II_forwardQ          = false
            }>(args);
        }
        case 2:
        {
            return this->template Simplify<{
                .restart_after_successQ = false,
                .restart_after_failureQ = false,
                .restart_walk_backQ     = false,
                .interleave_over_underQ = false,
                .R_II_blockingQ         = true,
                .R_II_forwardQ          = false
            }>(args);
        }
        case 3:
        {
            return this->template Simplify<{
                .restart_after_successQ = false,
                .restart_after_failureQ = false,
                .restart_walk_backQ     = true,
                .interleave_over_underQ = true,
                .R_II_blockingQ         = true,
                .R_II_forwardQ          = false
            }>(args);
        }
        case 4:
        {
            return this->template Simplify<{
                .restart_after_successQ = true,
                .restart_after_failureQ = true,
                .restart_walk_backQ     = true,
                .interleave_over_underQ = true,
                .R_II_blockingQ         = true,
                .R_II_forwardQ          = true
            }>(args);
        }
        default:
        {
            wprint(MethodName("SimplifyVariant") + " variand " + ToString(variant) + " unknown. Using default.");
            return Simplify(args);
        }
    }
}


private:

template<UInt8 local_opt_level, PassSimplifier_T::SimplifyPasses_TArgs targs>
Size_T Simplify_impl( mref<Reapr_T> reapr, cref<Simplify_Args_T> args )
{
//    constexpr bool debugQ = true;
    
//    using TArgs_T = StrandSimplifier_T::SimplifyStrands_TArgs;
//    constexpr TArgs_T targs = TArgs_T();

    [[maybe_unused]] auto tag = [this]()
    {
        return this->MethodName("Simplify_impl") + "<" + ToString(local_opt_level) + ">";
    };
    
    PD_TIMER(timer,tag());

#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("args",ToString(args));
#endif
    
    if constexpr (debugQ) { wprint(tag()+": Debug mode active."); }
    
    // By intializing S here, it will have enough internal memory for all planar diagrams.
    mref<PassSimplifier_T> S = PassSimplifier(args.strategy);
    
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
    
    
    if( args.compress_initialQ )
    {
        // This makes sure that the input is canonical ordering. This can make a huge difference in runtime!
        for( PD_T & pd : pd_todo ) { pd.Compress(); }
    }

    PD_List_T reapr_list;
    
    while( !pd_todo.empty() )
    {
        PD_T pd = std::move(pd_todo.back());
        pd_todo.pop_back();
        
        // We allow local pattern optimization only in the very first pass for each diagram. It won't help at all in Rattle.
        if( args.local_opt_level > UInt8(0) )
        {
            change_count += ArcSimplifier<Int,local_opt_level,true>( *this, pd,
                {
                    .compression_threshold = args.compression_threshold,
                    .compressQ             = args.compressQ
                }
            )();
        }

        auto [pass_change_count, disconnect_count] = this->template SimplifyDiagrammatically<debugQ,targs>( S, pd, args );
        change_count += pass_change_count;
        change_count += disconnect_count;
        
        if( pd.InvalidQ() ) { continue; }

        // If the StrandSimplifier did not find anything, then Disconnect produces a reduced diagram.
        const bool proven_reducedQ = args.disconnectQ && (pass_change_count == Size_T(0));
        
        
        if constexpr (debugQ)
        {
            if( proven_reducedQ && !pd.ReducedQ() )
            {
                eprint(tag()+": proven_reducedQ && !pd.ReducedQ().");
            }
        }
        
        // Split the diagrams into diagram components and push them to pd_todo for further simplification.

        // Caution: Split is allowed to push minimal diagrams to pd_done.
        if( (pass_change_count > Size_T(0)) || (disconnect_count > Size_T(0)) )
        {
            // If anything upstream changed, then we should better continue working on the split diagrams.
            if( args.splitQ )
            {
                change_count += Split( std::move(pd), pd_todo, proven_reducedQ );
                continue;
            }
            else
            {
                if( proven_reducedQ && pd.AlternatingQ() ) { pd.proven_minimalQ = true; }
                
                PushDiagramToDo( std::move(pd) );
                
                continue;
            }
        }
        
        // No changes were found so far. We can try reapr or we have to stop here.
        if( args.rerouteQ && (args.embedding_trials > Size_T(0)) && (args.rotation_trials > Size_T(0)) )
        {
            if( args.splitQ )
            {
                if constexpr (debugQ)
                {
                    if( !reapr_list.empty() ) { eprint(tag() +": !reapr_list.empty() before calling Split."); }
                }
                
                change_count += Split( std::move(pd), reapr_list, proven_reducedQ );
                
                // If proven_reducedQ, then Split already filtered out minimal diagrams.
                while( !reapr_list.empty() )
                {
                    PD_T pd_reapr = std::move(reapr_list.back());
                    reapr_list.pop_back();
                    
                    change_count += this->template Rattle<debugQ,targs>( S, reapr, std::move(pd_reapr), args );
                }
                
                if constexpr (debugQ)
                {
                    if( !reapr_list.empty() ) { eprint(tag() +": !reapr_list.empty() after calling Split."); }
                }
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
    
    if constexpr (debugQ)
    {
        if( !pd_list.empty() ) { pd_eprint("!pd_list.empty()"); };
        if( !pd_todo.empty() ) { pd_eprint("!pd_todo.empty()"); };
    }

    swap( pd_list, pd_done );
    
#ifdef PD_COUNTERS
    // We need to save the counters from being erased by Canonicalize().
    auto S_buffer = std::move(this->GetCache<PassSimplifier_T>("PassSimplifier"));
#endif
    
    if( args.canonicalizeQ )
    {
        Canonicalize();
    }

#ifdef PD_COUNTERS
    this->SetCache("PassSimplifier",std::move(S_buffer));
#endif
    
    if constexpr (debugQ)
    {
        if( !CheckAll() ) { pd_eprint(tag() + ": !CheckAll()."); }
    }
    
    return change_count;
}



template<bool debugQ, PassSimplifier_T::SimplifyPasses_TArgs targs>
Size_T Rattle(
    mref<PassSimplifier_T> S, mref<Reapr_T> reapr, PD_T && pd, cref<Simplify_Args_T> args
)
{
    [[maybe_unused]] auto tag = [this]() { return this->MethodName("Rattle"); };
    
    TOOLS_PTIMER(timer,tag());

    if constexpr (debugQ)
    {
        logprint(tag());
        if( pd.InvalidQ() ) { pd_eprint(tag() + ": pd.InvalidQ()."); }
        if( pd.ProvenMinimalQ() ) { wprint(tag() + ": pd.ProvenMinimalQ()."); }
        if( pd.CrossingCount() <= Int(1) ) { pd_eprint(tag() + ": pd.CrossingCount() <= Int(1)."); }
        if( pd.DiagramComponentCount() != Int(1) ) { pd_eprint(tag() + ": pd.DiagramComponentCount() != Int(1)."); }
        if( !pd.CheckAll() ) { pd_eprint(tag() + ": !pd.CheckAll()."); }
    }

    // TODO: For some reason, reapr.Embedding(pd) will break if args.permute_randomQ == false and args.compressQ == false. So, let's compress here.
    // TODO: It would be great if we did not have to erase, e.g., the face information.
    // TODO: However, typically, we will use args.permute_randomQ == true anyways, and then it does not matter.
    if( !args.permute_randomQ ) { pd.Compress(); }
    
    PD_T pd_1;
    
    Size_T pass_change_count = 0;
    Size_T disconnect_count  = 0;
    
    constexpr Size_T max_projection_iter = 10;
    bool progressQ = false;
    
    // DEBUGGING
    if( args.embedding_trials == Size_T(0) )
    {
        wprint(MethodName("Rattle") + ": Called with embedding_trials = 0.");
    }
    // DEBUGGING
    if( args.rotation_trials == Size_T(0) )
    {
        wprint(MethodName("Rattle") + ": Called with rotation_trials = 0.");
    }
    
    for( Size_T iter = 0; iter < args.embedding_trials; ++iter )
    {
        // We want to exploit here that some information needed for OrthoDraw is already cached.
        // However, this will help only if args.permute_randomQ == false.
        // And it makes sense to do this only if args.permute_randomQ == false and if args.randomize_bends != 0 or args.randomize_virtual_edgesQ == true.
        LinkEmbedding_T emb = reapr.Embedding(pd,reapr.RandomRotation());
        
        for( Size_T rot = 0; rot < args.rotation_trials; ++rot )
        {
            Size_T projection_iter = 0;
            int projection_flag = 0;
            emb.Rotate( reapr.RandomRotation() );
            projection_flag = emb.RequireIntersections();
            
            while( (projection_flag!=0) && (projection_iter < max_projection_iter) )
            {
                ++projection_iter;
                // Rotate is a bit expensive do to an extra allocation and extra copying.
                // But we land here really very, very, very seldomly.
                emb.Rotate( reapr.RandomRotation() );
                projection_flag = emb.RequireIntersections();
            }
            
            if( projection_flag != 0 )
            {
                eprint(MethodName("Rattle") + ": " + emb.MethodName("FindIntersections")+ " returned invalid status flag for " + ToString(max_projection_iter) + " random rotation matrices. Something must be wrong. Returning an invalid diagram. Check your results carefully.");
                
                return Size_T(0);
            }
            
            PDC_T pdc_new ( emb );
            
            if constexpr (debugQ)
            {
                if( !pdc_new.CheckAll() ) { pd_eprint(tag() + ": !pdc_new.CheckAll())."); }
            }
            
            // We might get some unlinks here. We push them to "done", so that they won't be forgotton.
            for( Size_T i = 1; i < pdc_new.pd_list.size(); ++i )
            {
                if constexpr (debugQ)
                {
                    if( !pdc_new.pd_list[i].AnelloQ() )
                    {
                        pd_eprint(tag() + ": !pdc_new.pd_list[" + ToString(i) + "].AnelloQ().");
                    }
                }
                
                PushDiagramDone( std::move(pdc_new.pd_list[i]) );
            }
            
            // TODO: Is pdc_new.pd_list[0] guaranteed to be valid?
            // I don't think so!
            pd_1 = std::move(pdc_new.pd_list[0]);
            
            std::tie(pass_change_count,disconnect_count) = this->template SimplifyDiagrammatically<debugQ,targs>(S, pd_1, args);
    
            
            // TODO: Can we improve these conditions?
            // TODO: E.g., we could call it a success, if pd_1 is reduced and alternating.
            progressQ = ( pd_1.CrossingCount() < pd.CrossingCount() )
                        ||
                        (disconnect_count > Size_T(0))
                        ||
                        (pd_1.DiagramComponentCount() > Int(1));
            
            // Caution: We must stop entirely as soon we made any progress, as pd_done might have been altered.
            if( progressQ ) { break; }
            
//            // No progress, but we can at least change pd to have a new chance next time.
//            if( pd_1.CrossingCount() == pd.CrossingCount() )
//            {
//                pd = std::move(pd_1);
//            }
        }
        
        // Caution: We must stop entirely as soon we made any progress, as pd_done might have been altered.
        if( progressQ ) { break; }
    }
    
    // TODO: Can pd_1.InvalidQ() ever happen? Shall this ever happen?
    // There are a few ways this can happen:
    //  1. args.embedding_trials == 0 or args.rotation_trials == 0. But then we should at least do PushDiagramDone( std::move(pd) ), no?
    //  2. pdc_new.pd_list[0] was invalid. This can happen, for example, if the generated link embedding is a multiple "eight" that can be recognized only as unlink when looking from the side. Indeed, quitting here might be correct.
    //  3. SimplifyDiagrammatically made it invalid. But then it will have pushed something to "done" or "todo". So, quitting here might be correct.
    if( pd_1.InvalidQ() )
    {
        // DEBUGGING
        wprint(MethodName("Rattle") + ": pd_1 is invalid. Returning early.");
        return pass_change_count + disconnect_count;
    }
    
    Size_T split_count = 0;
    
    if( progressQ )
    {
        // If the StrandSimplifier did not find anything, then Disconnect produces a reduced diagram.
        const bool proven_reducedQ = args.disconnectQ && (pass_change_count == Size_T(0));
        
        if constexpr (debugQ)
        {
            if( proven_reducedQ && !pd.ReducedQ() )
            {
                eprint(tag()+": proven_reducedQ && !pd.ReducedQ().");
            }
        }
        
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
        // If splits are allowed, then this is already split; otherwise, we must not split here, either.
        PushDiagramDone( std::move(pd) );
    }
    
    return pass_change_count + disconnect_count + split_count;
}



// Caution: SimplifyDiagrammatically is non-exhaustive! It ends with Disconnect, and this may unlock new pass moves.
template<bool debugQ, PassSimplifier_T::SimplifyPasses_TArgs targs>
std::pair<Size_T,Size_T> SimplifyDiagrammatically(
    mref<PassSimplifier_T> S, mref<PD_T> pd, cref<Simplify_Args_T> args
)
{
    [[maybe_unused]] auto tag = [this](){ return this->MethodName("SimplifyDiagrammatically"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( pd.InvalidQ() ) { return {Size_T(0),Size_T(0)}; }
    
    if(  pd.proven_minimalQ )
    {
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint(tag()+": CheckAll() failed when pushed to pd_done."); };
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
    
    if constexpr (debugQ)
    {
        if( !pd.ValidQ() ) { pd_eprint(tag() +": pd.ValidQ()."); };
    }
    
    // It is very likely that we change the diagram.
    // Also, a stale cache might spoil the simplification.
    // Thus, we proactively delete the cache.
    pd.ClearCache();

    // Not clear whether local patterns are beneficial.
//        ArcSimplifier<Int,3,true> A ( *this, pd, Scalar::Max<Size_T>, args.compressQ );
//        Size_T local_change_count =  A();
//        change_count += local_change_count;
    
    
    const Int max_dist = Scalar::Max<Int>;
    
    Size_T pass_change_count = 0;
    
    if( args.rerouteQ )
    {
        do
        {
            pass_change_count = 0;
            
            // TODO: Check this
            pass_change_count += S.template SimplifyPasses<targs>(pd,{
//            pass_change_count += S.template SimplifyStrands<targs>(pd,{
                .max_dist              = max_dist,
                .overQ                 = true,
                .compressQ             = args.compressQ,
                .compression_threshold = args.compression_threshold
            });
            
            if( pd.InvalidQ() ) { break; }
            
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyOverPasses."); };
            }
            
//            if constexpr ( !targs.interleave_over_underQ || !targs.restart_after_successQ || !targs.restart_after_failureQ )
            if constexpr ( !targs.interleave_over_underQ )
            {
                // Reroute underpasses.
                pass_change_count += S.template SimplifyPasses<targs>(pd,{
//                pass_change_count += S.template SimplifyStrands<targs>(pd,{
                    .max_dist              = max_dist,
                    .overQ                 = false,
                    .compressQ             = args.compressQ,
                    .compression_threshold = args.compression_threshold
                });
                
                if( pd.InvalidQ() ) { break; }
                
                if constexpr (debugQ)
                {
                    if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after SimplifyUnderPasses."); };
                }
            }
        }
        while( pass_change_count > Size_T(0) );
    }

    if( pd.InvalidQ() ) { return {pass_change_count,Size_T(0)}; }
    
    Size_T disconnect_count = 0;
    
    // Caution: Disconnect is allowed to push some small diagrams to pd_done.
    if( args.disconnectQ )
    {
//        Size_T disconnect_iter = 0;
        Size_T local_disconnect_count = 0;
        // TODO: This while loop is nasty. Isn't there a way to disconnect in just one round?
        do
        {
//            ++disconnect_iter;
            local_disconnect_count = Disconnect(pd);
            disconnect_count += local_disconnect_count;
        }
        while( local_disconnect_count > Size_T(0) );
//        
//#ifdef PD_DEBUG
//        if( disconnect_iter > Size_T(2) )
//        {
//            PD_PRINT(tag() + ": Needed " + ToString(disconnect_iter-1) + " rounds of disconnect. (disconnect_count = " + ToString(disconnect_count)+ ", crossing_count = " + ToString(pd.CrossingCount()) + ").");
//        }
//#endif // PD_DEBUG
    }
    
    return {pass_change_count,disconnect_count};
}
