public:

/*!@brief Controls `struct` to hold the settings for the `Simplify` routine of `PlanarDiagramComplex`. */

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
    
    int                 randomize_bends          = 2;
    bool                randomize_virtual_edgesQ = true;
    Compaction_T        compaction_method        = Compaction_T::Length_MCF;
    
    bool                canonicalizeQ            = true;
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
    
    if( DiagramCount() == Int{0} ) { return 0; }

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
            Msgr::eprint("Simplify", "local_opt_level = " , args.local_opt_level, " is invalid." );
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
            Msgr::wprint("SimplifyVariant", " variant ", variant, " unknown. Using default.");
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

    [[maybe_unused]] constexpr auto tag = ct_string("Simplify_impl") + "<" + to_ct_string(local_opt_level) + ">"
    
    PD_TIMER(timer,MethodName(tag));

#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("args",ToString(args));
#endif
    
    if constexpr (debugQ) { wprint(tag,": Debug mode active."); }
    
    // By intializing S here, it will have enough internal memory for all planar diagrams.
    mref<PassSimplifier_T> S = GetPassSimplifier(args.strategy);
    S.Allocate(this->TotalCrossingCount());
    
    
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
        const bool proven_reducedQ = args.disconnectQ && (pass_change_count == Size_T{0});
        
        
        if constexpr (debugQ)
        {
            if( proven_reducedQ && !pd.ReducedQ() )
            {
                Msgr::eprint(tag,": proven_reducedQ && !pd.ReducedQ().");
            }
        }
        
        // Split the diagrams into diagram components and push them to pd_todo for further simplification.

        // Caution: Split is allowed to push minimal diagrams to pd_done.
        if( (pass_change_count > Size_T{0}) || (disconnect_count > Size_T{0}) )
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
        if( args.rerouteQ && (args.embedding_trials > Size_T{0}) && (args.rotation_trials > Size_T{0}) )
        {
            if( args.splitQ )
            {
                if constexpr (debugQ)
                {
                    if( !reapr_list.empty() )
                    {
                        Msgr::eprint(tag, "!reapr_list.empty() before calling Split.");
                    }
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
                    if( !reapr_list.empty() )
                    {
                        Msgr::eprint(tag, "!reapr_list.empty() after calling Split."
                        );
                    }
                }
            }
            else
            {
                if( pd.DiagramComponentCount() <= Int{1} )
                {
                    change_count += this->template Rattle<debugQ,targs>( S, reapr, std::move(pd), args );
                }
                else
                {
                    // We are not allowed to split; so we cannot do better than pushing this onto the "done" pile.
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
        if( !pd_list.empty() ) { pd_eprint(MethodName(tag), ": !pd_list.empty()"); };
        if( !pd_todo.empty() ) { pd_eprint(MethodName(tag), ": !pd_todo.empty()"); };
    }

    swap( pd_list, pd_done );
    
    if( args.canonicalizeQ )
    {
#ifdef PD_COUNTERS
        // We need to save the counters from being erased by Canonicalize().
        auto S_buffer = std::move(this->GetCache<PassSimplifier_T>("PassSimplifier"));
#endif
        Canonicalize();
        
#ifdef PD_COUNTERS
        this->SetCache("PassSimplifier",std::move(S_buffer));
#endif
    }

    
    if constexpr (debugQ)
    {
        if( !CheckAll() ) { pd_eprint(MethodName(tag), ": !CheckAll()."); }
    }
    
    return change_count;
}



/*!@brief Write everything needed to reproduce a `Rattle` projection failure.
 */
void DumpRattleFailure(
    cref<PD_T> pd, mref<LinkEmbedding_T> emb, mref<Reapr_T> reapr,
    cref<Simplify_Args_T> args, const int projection_flag
)
{
    /*!When `FindIntersections` keeps failing, `Rattle` gives up and returns a diagram it has told the caller not to trust. The state that would explain
     * *why* -- the intermediate diagram and the 3D embedding whose projection went
     * degenerate -- is local to this function and is destroyed on return, so a
     * user's bug report can only ever be the message above. That is not enough to
     * reproduce: these failures depend on the random embedding, and the interesting settings are not visible from the command line.
     *
     * So dump them. The diagram goes out as a signed, colored pd code that the CLI
     * tools read back directly, and the embedding through `LinkEmbedding::WriteToFile`
     * at full precision, so the exact geometry can be reloaded and re-projected.
     *
     * Costs nothing on the happy path -- it is only ever reached after the failure
     * has already been reported. Writes at most `max_dumps` bundles per process so
     * a long batch run cannot fill a disk, and honours `KNOODLE_DUMP_DIR` for the
     * destination (default: parent directory of Tools::logger.LogFile()).
     */
    static constexpr int max_dumps = 8;
    
    // TODO: Henrik speaking: I really, really do not like nonconstant statics. They easily produce Heisenbugs, in particular, in shared library environments in which libraries may have been built by different versions of this code. Also, it is not clear whether several instances of this class shall have their one counter, e.g., when they run in multi-threaded applications.
    static std::atomic<int> dump_counter { 0 };

    const int n = dump_counter.fetch_add(1);
    if( n >= max_dumps ) { return; }

    try
    {
        // Using the same path as the log file per default.
        // Log file writes to user's home directory per default because working directories for libraries may be unpredictable.

        std::filesystem::path dir {
            Tools::Profiler::GetLogger().LogFile().parent_path()
        };
        
        if( const char * d = std::getenv("KNOODLE_DUMP_DIR") ) { dir = d; }

        const std::string stem = "rattle-failure-" + ToString(n);
        const std::filesystem::path base = dir / stem;

        // 1. The context, in one readable file.
        {
            std::ofstream s ( base.string() + ".txt" );
            s << "Rattle projection failure\n"
              << "=========================\n\n"
              << "FindIntersections returned status flag " << projection_flag
              << " for every one of the random rotations tried, so Rattle gave up\n"
              << "and returned an invalid diagram.\n\n"
              << "Files in this bundle:\n"
              << "  " << stem << ".pd.tsv   the diagram being simplified (signed, colored pd code)\n"
              << "  " << stem << ".xyz      the 3D embedding whose projection failed\n\n"
              << "The .xyz carries '#color' headers, which is what separates the link's\n"
              << "components -- they cannot be dropped without fusing the components into\n"
              << "one polyline. LinkEmbedding::FromInString reads them; note that some\n"
              << "knoodlesimplify builds cannot yet parse '#color' on input.\n\n"
              << "diagram:\n"
              << "  crossings          = " << pd.CrossingCount() << "\n"
              << "  arcs               = " << pd.ArcCount() << "\n"
              << "  link components    = " << pd.LinkComponentCount() << "\n"
              << "  diagram components = " << pd.DiagramComponentCount() << "\n\n"
              << "Transformation matrix = " << ToString(emb.TransformationMatrix()) << "\n"
              << "Sterbenz shift = " << ToString(emb.SterbenzShift()) << "\n"
              << "embedding:\n"
              << "  edges              = " << emb.EdgeCount() << "\n\n"
              << "Simplify args:\n  " << ToString(args) << "\n\n"
              << "Reapr settings:\n  " << ToString(reapr.Settings()) << "\n";
        }

        // 2. The diagram, in a form the CLI tools can read straight back.
        {
            std::ofstream s ( base.string() + ".pd.tsv" );
            auto code = pd.template PDCode<Int>();
            s << OutString::FromMatrix<Format::Matrix::TSV>(
                code.ReadAccess(), code.Dim(0), code.Dim(1)
            );
        }

        // 3. The exact geometry, at full precision, so it can be re-projected.
        (void)emb.WriteToFile( base.string() + ".xyz", true );

        Msgr::wprint("Rattle", ": wrote a failure bundle to ", base.string(), ".{txt,pd.tsv,xyz} -- please attach these to any bug report." );
    }
    catch( ... )
    {
        // Diagnostics must never make a bad situation worse.
    }
}

template<bool debugQ, PassSimplifier_T::SimplifyPasses_TArgs targs>
Size_T Rattle(
    mref<PassSimplifier_T> S, mref<Reapr_T> reapr, PD_T && pd, cref<Simplify_Args_T> args
)
{
    [[maybe_unused]] constexpr auto tag = ct_string("Rattle");
    TOOLS_PTIMER(timer,MethodName(tag));

    if constexpr (debugQ)
    {
        logprint(tag);
        if( pd.InvalidQ() ) { pd_eprint(MethodName(tag), ": pd.InvalidQ()."); }
        if( pd.ProvenMinimalQ() ) { Msgr::wprint(tag, ": pd.ProvenMinimalQ()."); }
        if( pd.CrossingCount() <= Int{1} ) { pd_eprint(MethodName(tag), ": pd.CrossingCount() <= Int{1}."); }
        if( pd.DiagramComponentCount() != Int{1} ) { pd_eprint(MethodName(tag), ": pd.DiagramComponentCount() != Int{1}."); }
        if( !pd.CheckAll() ) { pd_eprint(MethodName(tag), ": !pd.CheckAll()."); }
    }
    
    if( pd.InvalidQ() ) { return 0; }

    // We are paranoid here. Rattle should actually not be called if we do not want any reapr trials at all.
    if( (args.embedding_trials == Size_T{0}) || (args.rotation_trials == Size_T{0}) )
    {
        PushDiagramDone( std::move(pd) );
        return 0;
    }
    
    // For some reason, reapr.Embedding(pd) will break if args.permute_randomQ == false and args.compressQ == false. So, let's compress here.
    if( !args.permute_randomQ ) { pd.Compress(); }
    
    PD_T pd_1;
    
    Size_T pass_change_count = 0;
    Size_T disconnect_count  = 0;
    
    constexpr Size_T max_projection_iter = 10;
    const bool rotateQ = args.rotation_trials > Size_T{0};
    bool progressQ = false;
    
    Tensor2<typename LinkEmbedding_T::Real,Int> x;
    
    for( Size_T iter = 0; iter < args.embedding_trials; ++iter )
    {
        // We want to exploit here that some information needed for OrthoDraw is already cached.
        // However, this will help only if args.permute_randomQ == false.
        // And it makes sense to do this only if args.permute_randomQ == false and if args.randomize_bends != 0 or args.randomize_virtual_edgesQ == true.
//        LinkEmbedding_T emb = reapr.Embedding(pd,reapr.RandomRotation());
        
        LinkEmbedding_T emb = rotateQ ? reapr.Embedding(pd) : reapr.Embedding(pd,reapr.RandomRotation());
        
        if( rotateQ )
        {
            // We deliberately make a copy here because successive rotations and Sterbenz shifts have the potential to lose a lot of precision.
            x.template RequireSize<false>(emb.EdgeCount(), Int{3});
            emb.WriteVertexCoordinates(x.data());
        }
        
        for( Size_T rot = 0; rot < args.rotation_trials; ++rot )
        {
            int projection_flag = 0;
            
            for( Size_T pr_iter = 0; pr_iter < max_projection_iter; ++pr_iter )
            {
                // We deliberately do not use `emb.Transform(reapr.RandomRotation())` because successive rotations and Sterbenz shifts have the potential to lose a lot of precision.
                emb.SetTransformationMatrix(reapr.RandomRotation());
                emb.template ReadVertexCoordinates<true>(x.data());
                projection_flag = emb.RequireIntersections();
                
                if( projection_flag == 0 ) { break; }
                
                DumpRattleFailure( pd, emb, reapr, args, projection_flag );
            }
            
            if( projection_flag != 0 )
            {
                Msgr::eprint(tag,  emb.MethodName("FindIntersections"), " returned invalid status flag for ", max_projection_iter, " random rotation matrices. Something must be wrong. Returning an invalid diagram. Check your results carefully.");

                // Although we did not succeed in simplifying this, we need to push it to the list of diagrams that are "done"; otherwise we would lose it.
                PushDiagramDone( std::move(pd) );
                return Size_T{0};
            }
            
            PDC_T pdc_new ( emb );
            
            if constexpr (debugQ)
            {
                if( !pdc_new.CheckAll() )
                {
                    pd_eprint(MethodName(tag), ": !pdc_new.CheckAll()).");
                }
            }
            
            // We might get some unlinks here. We push them to "done", so that they won't be forgotton.
            for( Size_T i = 1; i < pdc_new.pd_list.size(); ++i )
            {
                if constexpr (debugQ)
                {
                    if( !pdc_new.pd_list[i].AnelloQ() )
                    {
                        pd_eprint(MethodName(tag), ": !pdc_new.pd_list[", i, "].AnelloQ().");
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
                        (disconnect_count > Size_T{0})
                        ||
                        (pd_1.DiagramComponentCount() > Int{1});
            
            // Caution: We must stop entirely as soon we made any progress, as pd_done might have been altered.
            if( progressQ ) { break; }
        }
        
        // Caution: We must stop entirely as soon we made any progress, as pd_done might have been altered.
        if( progressQ ) { break; }
    }
    
    // There are a few ways in which pd_1.InvalidQ() == true can happen:
    //  1. args.embedding_trials == 0 or args.rotation_trials == 0. But this is ruled out by an if statement above.
    //  2. pdc_new.pd_list[0] was invalid. This can happen, for example, if the generated link embedding is a multiple "eight" that can be recognized only as unlink when looking from the side. Indeed, quitting here might be correct.
    //  3. SimplifyDiagrammatically made it invalid. But then it will have pushed something to "done" or "todo". So, quitting here is correct, too.
    if( pd_1.InvalidQ() )
    {
        return pass_change_count + disconnect_count;
    }
    
    Size_T split_count = 0;
    
    if( progressQ )
    {
        // If the StrandSimplifier did not find anything, then Disconnect produces a reduced diagram.
        const bool proven_reducedQ = args.disconnectQ && (pass_change_count == Size_T{0});
        
        if constexpr (debugQ)
        {
            if( proven_reducedQ && !pd.ReducedQ() )
            {
                Msgr::eprint(tag, "proven_reducedQ && !pd.ReducedQ().");
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
    [[maybe_unused]] constexpr auto tag = ct_string("SimplifyDiagrammatically");
    
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
    if( pd.InvalidQ() ) { return {Size_T{0},Size_T{0}}; }
    
    if(  pd.proven_minimalQ )
    {
        if constexpr (debugQ)
        {
            if( !pd.CheckAll() ) { pd_eprint(MethodName(tag), ": CheckAll() failed when pushed to pd_done."); };
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
        return {Size_T{0},Size_T{0}};
    }
    
    if constexpr (debugQ)
    {
        if( !pd.ValidQ() ) { pd_eprint(MethodName(tag), ": pd.ValidQ()."); };
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
                .max_dist              = max_dist,
                .overQ                 = true,
                .compressQ             = args.compressQ,
                .compression_threshold = args.compression_threshold
            });
            
            if( pd.InvalidQ() ) { break; }
            
            if constexpr (debugQ)
            {
                if( !pd.CheckAll() ) { pd_eprint(MethodName(tag), ": CheckAll() failed after SimplifyOverPasses."); };
            }
            
//            if constexpr ( !targs.interleave_over_underQ || !targs.restart_after_successQ || !targs.restart_after_failureQ )
            if constexpr ( !targs.interleave_over_underQ )
            {
                // Reroute underpasses.
                pass_change_count += S.template SimplifyPasses<targs>(pd,{
                    .max_dist              = max_dist,
                    .overQ                 = false,
                    .compressQ             = args.compressQ,
                    .compression_threshold = args.compression_threshold
                });
                
                if( pd.InvalidQ() ) { break; }
                
                if constexpr (debugQ)
                {
                    if( !pd.CheckAll() ) { pd_eprint(MethodName(tag), ": CheckAll() failed after SimplifyUnderPasses."); };
                }
            }
        }
        while( pass_change_count > Size_T{0} );
    }

    if( pd.InvalidQ() ) { return {pass_change_count,Size_T{0}}; }
    
    Size_T disconnect_count = 0;
    
    // Caution: Disconnect is allowed to push some small diagrams to pd_done.
    if( args.disconnectQ )
    {
        Size_T local_disconnect_count = 0;
        // TODO: This while loop is nasty. Isn't there a way to disconnect in just one round?
        do
        {
            local_disconnect_count = Disconnect(pd);
            disconnect_count += local_disconnect_count;
        }
        while( local_disconnect_count > Size_T{0} );
    }
    
    return {pass_change_count,disconnect_count};
}
