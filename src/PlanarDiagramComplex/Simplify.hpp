public:


struct Simplify_Args_T
{
    Int    min_dist        = 6;
    Int    max_dist        = Scalar::Max<Int>;
    Int    local_opt_level = 4;
    Size_T local_max_iter  = Scalar::Max<Size_T>;
    bool   disconnectQ     = false;
    bool   splitQ          = true;
    bool   compressQ       = true;
};

Size_T Simplify( cref<Simplify_Args_T> args = Simplify_Args_T() )
{
    if( DiagramCount() == Int(0) ) { return 0; }
    
    const Int level = Clamp(args.local_opt_level, Int(0), Int(4));
    
    switch ( level )
    {
        case 0:
        {
            return Simplify_impl<0,true>(args);
        }
        case 1:
        {
            return Simplify_impl<1,true>(args);
        }
        case 2:
        {
            return Simplify_impl<2,true>(args);
        }
        case 3:
        {
            return Simplify_impl<3,true>(args);
        }
        case 4:
        {
            return Simplify_impl<4,true>(args);
        }
        default:
        {
            eprint( MethodName("Simplify") + ": Value " + ToString(level) + " is invalid." );
            return 0;
        }
    }
    
    return 0;
}

private:

template<Int local_opt_level, bool pass_R_II_Q>
Size_T Simplify_impl( cref<Simplify_Args_T> args )
{
    [[maybe_unused]] auto tag = [&args]()
    {
        return MethodName("Simplify_impl")
        + "<" + ToString(local_opt_level)
        + ">"
        +"({ .min_dist = " + ToString(args.min_dist)
        + ", .max_dist = " + ToString(args.max_dist)
        + ", .disconnectQ = " + ToString(args.disconnectQ)
        + ", .splitQ = " + ToString(args.splitQ)
        + ", .compressQ = " + ToString(args.compressQ)
        + "})";
    };
    
    TOOLS_PTIMER(timer,tag());
    
//    constexpr bool debugQ = true;
    
    if constexpr ( debugQ )
    {
        wprint(tag()+": Debug mode active.");
    }

//    using ArcSimplifier_Knot_T  = ArcSimplifier2<Int,local_opt_level,false>;
    using ArcSimplifier_Link_T  = ArcSimplifier2<Int,local_opt_level,true >;
//    
//    using StrandSimplier_Knot_T = StrandSimplifier2<Int,pass_R_II_Q,false>;
    using StrandSimplier_Link_T = StrandSimplifier2<Int,pass_R_II_Q,true >;
    
    Size_T total_change_count = 0;
    
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
        
        Int dist = args.min_dist;
        const bool simplify_localQ   = (local_opt_level > Int(0));
        const bool simplify_strandsQ = (args.max_dist   > Int(0));
        
        do
        {
            change_count_old = change_count;
            
            dist = Min(dist,args.max_dist);
            
            // Since ArcSimplifier_T performs only inexpensive tests, we should use it first.
            if( simplify_localQ && ( (change_count_o > 0) ||  (change_count_u > 0) ) )
            {
                ArcSimplifier_Link_T A ( *this, pd, args.local_max_iter, args.compressQ );
                change_count_loc = A();
                
//                if( mult_compQ )
//                {
//                    ArcSimplifier_Link_T A ( *this, pd, args.local_max_iter, args.compressQ );
//                    change_count_loc = A();
//                }
//                else
//                {
//                    ArcSimplifier_Knot_T A ( *this, pd, args.local_max_iter, args.compressQ );
//                    change_count_loc = A();
//                }
                
                change_count += change_count_loc;
                
                if constexpr ( debugQ )
                {
                    if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after local simplification."); };
                }
            }
            
            // If we did strand moves before without success and if simplify_local_changes == 0, then we can break here, too.
            if( (change_count_o == 0) && (change_count_u == 0) && (change_count_loc == 0) )
            {
                break;
            }
            
            if( pd.InvalidQ() ) { break; }
            
            if( !simplify_strandsQ ) { break; }
            
            // Reroute overstrands.
            {
                PD_PRINT("Construct StrandSimplier");
                
                StrandSimplier_Link_T S (*this,pd);
                change_count_o  = S.SimplifyStrands(true,dist);
                
//                if( mult_compQ )
//                {
//                    StrandSimplier_Link_T S (*this,pd);
//                    change_count_o  = S.SimplifyStrands(true,dist);
//                }
//                else
//                {
//                    StrandSimplier_Knot_T S (*this,pd);
//                    change_count_o  = S.SimplifyStrands(true,dist);
//                }
                
                change_count += change_count_o;
                
                if( pd.InvalidQ() ) { break; }
                
                if( change_count_o > 0 )
                {
                    if( args.compressQ ) { pd.ConditionalCompress(); } else { pd.ClearCache(); }
                    // TODO: Is clearing the cache really necessary? For example, ArcLeftDarc should still be valid.
                }
                
            }

            if constexpr ( debugQ )
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after overstrand simplification."); };
            }
            
            // Reroute overstrands.
            {
                PD_PRINT("Construct StrandSimplier");
                
                StrandSimplier_Link_T S (*this,pd);
                change_count_u  = S.SimplifyStrands(false,dist);
                
//                if( mult_compQ )
//                {
//                    StrandSimplier_Link_T S (*this,pd);
//                    change_count_u  = S.SimplifyStrands(false,dist);
//                }
//                else
//                {
//                    StrandSimplier_Knot_T S (*this,pd);
//                    change_count_u  = S.SimplifyStrands(false,dist);
//                }

                change_count   += change_count_u;
                
                if( pd.InvalidQ() ) { break; }
                
                if( change_count_u > Size_T(0) )
                {
                    if( args.compressQ ) { pd.ConditionalCompress(); } else { pd.ClearCache(); }
                    // TODO: Is clearing the cache really necessary? For example, ArcLeftDarc should still be valid.

                }
            }
            
            if constexpr ( debugQ )
            {
                if( !pd.CheckAll() ) { pd_eprint("CheckAll() failed after understrand simplification."); };
            }

            
            if( dist <= Scalar::Max<Int> / Int(2) )
            {
                dist *= Int(2);
            }
            else
            {
                dist = Scalar::Max<Int>;
            }
        }
        while(
              (change_count > change_count_old)
              ||
              ( (dist <= args.max_dist) && (dist < pd.ArcCount()) )
        );
        
        total_change_count += change_count;

        if( pd.InvalidQ() ) { continue; }
        
        if constexpr ( debugQ )
        {
            if( !pd.CheckAll() ) { pd_eprint("!pd.CheckAll() after simplification."); };
        }
        
        if( pd.CrossingCount() == Int(0) )
        {
            pd_done.push_back( PD_T::Unknot(pd.last_color_deactivated) );
            continue;
        }
        
        if( change_count > Size_T(0) )
        {
            pd.ClearCache();
        }
        
        Size_T disconnect_count = args.disconnectQ ? Disconnect(pd) : Size_T(0);
        total_change_count += disconnect_count;
        
        if constexpr ( debugQ )
        {
            if( args.disconnectQ && !pd.CheckAll() )
            {
                pd_eprint("CheckAll() failed after disconnecting.");
            }
            
            if( args.splitQ )
            {
                logprint("Preparing Split now.");
                
                if( !pd.CheckAll() ) { pd_eprint("!pd.CheckAll() before splitting."); };
                
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
            Split( std::move(pd), pd_todo );
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
                
                if constexpr ( debugQ )
                {
                    if( !pd.CheckAll() ) { pd_eprint("pd.CheckAll() failed after compression."); };
                }
            }
            // Should be unnecessary.
            pd.ClearCache();
            pd_done.push_back( std::move(pd) );
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
    
    if( total_change_count > Size_T(0) ) { this->ClearCache(); }
    
    return total_change_count;
}
