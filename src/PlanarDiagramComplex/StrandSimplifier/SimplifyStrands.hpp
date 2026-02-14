public:


struct SimplifyStrands_Args
{
    Int  max_dist              = Scalar::Max<Int>;
    bool overQ                 = true;
    bool reroute_markedQ       = false;
    bool compressQ             = true;
    Int  compression_threshold = 100;
};

friend std::string ToString( cref<SimplifyStrands_Args> args )
{
    return std::string("{ ")
         +   ".max_dist = " + ToString(args.max_dist)
         + ", .overQ = " + ToString(args.overQ)
         + ", .reroute_markedQ = " + ToString(args.reroute_markedQ)
         + ", .compressQ = " + ToString(args.compressQ)
         + ", .compression_threshold = " + ToString(args.compression_threshold)
         + " }";
}

struct SimplifyStrands_TArgs
{
    bool restart_after_successQ = true;
    bool restart_after_failureQ = true;
    bool restart_walk_backQ     = true;
    bool restart_change_typeQ   = true;
};

friend std::string ToString( cref<SimplifyStrands_TArgs> targs )
{
    return "{.restart_after_successQ = " + ToString(targs.restart_after_successQ)
         + ",.restart_after_failureQ = " + ToString(targs.restart_after_failureQ)
         + ",.restart_walk_backQ = " + ToString(targs.restart_walk_backQ)
         + ",.restart_change_typeQ = " + ToString(targs.restart_change_typeQ)
         + "}";
}

/*!@brief This is the main routine of the class. It is supposed to reroute all over/understrands to shorter strands, if possible. It does so by traversing the diagram and looking for over/understrand. When a complete strand is detected, it runs Dijkstra's algorithm in the dual graph of the diagram _without the currect strand_. If a shorter path is detected, the strand is rerouted. The returned integer is a rough(!) indicator of how many changes accoured. 0 is returned only of no changes have been made and if there is no need to call this function again. A positive value indicates that it would be worthwhile to call this function again (maybe after some further simplifications).
 *
 *
 */

template<SimplifyStrands_TArgs targs>
Size_T SimplifyStrands( mref<PD_T> pd_input, cref<SimplifyStrands_Args> args )
{
    [[maybe_unused]] auto tag = [this]()
    {
        return this->MethodName("SimplifyStrands");
    };
    
//    [[maybe_unused]] auto tag = [&args,this]()
//    {
//        return this->MethodName("SimplifyStrands") + "<" + ToString(targs) + ">(" + ToString(args) + ")";
//    };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("targs",ToString(targs));
    logvalprint("args ",ToString(args) );
#endif
    
//    // DEBUGGING
//    TOOLS_LOGDUMP(tag());
//    TOOLS_LOGDUMP(pd_input.max_crossing_count);
//    TOOLS_LOGDUMP(pd_input.crossing_count);

    
    if( args.max_dist <= Int(0) ) { return 0; }
    
    PD_PRINT(tag()+ ": Initial number of crossings = " + ToString(pd_input.CrossingCount()) );
    
    if( args.compressQ ) { pd_input.ConditionalCompress(args.compression_threshold); }

    if constexpr ( !targs.restart_change_typeQ )
    {
        SetStrandMode(args.overQ);
    }
    LoadDiagram(pd_input);
    NewStrand();
    
    Int a_ptr = 0;
    change_counter   = 0;

//    const Int pd_max_crossing_count = pd->MaxCrossingCount();
    const Int pd_max_arc_count      = pd->MaxArcCount();
    
    PD_VALPRINT("pd->crossing_count",pd->crossing_count);
    
//    TOOLS_LOGDUMP( (!compress_oftenQ || (pd->arc_count >= pd_max_crossing_count)) );
//    TOOLS_LOGDUMP(compress_oftenQ);
//    TOOLS_LOGDUMP(pd_max_crossing_count);
//    TOOLS_LOGDUMP(pd_max_crossing_count);
    
    while( a_ptr < pd_max_arc_count )
    {
        // Search for next arc that is active and has not yet been handled.
        while(
            ( a_ptr < pd_max_arc_count ) && (!ArcActiveQ(a_ptr) || ArcRecentlyMarkedQ(a_ptr) )
        )
        {
            ++a_ptr;
        }
        
//        TOOLS_LOGDUMP(a_ptr);
        
        if( a_ptr >= pd_max_arc_count ) [[unlikely]] { break; }
        
        // Find the beginning of strand.
        if constexpr ( targs.restart_change_typeQ )
        {
            const bool overQ_0 = ArcOverQ(a_ptr,Tail);
            const bool overQ_1 = ArcOverQ(a_ptr,Head);
            
            SetStrandMode(overQ_1);
            if( NewStrand() ) [[unlikely]] { break; }
            
            if( overQ_0 != overQ_1 )
            {
                /* Definitely the start of an (overQ_1 ? over : under)-strand.
                 * Bad idea to walk backwards.
                 *
                 *          |        |
                 *          | a_ptr  |
                 *  ------->|------->-------->
                 *          |        |
                 *          |        |
                 */
                
                s_begin = a_ptr;
            }
            else
            {
                /* Somewhere in the middle of an (overQ_1 ? over : under)-strand.
                 * We can (should?) move back to the start.
                 *
                 *          |        |
                 *          | a_ptr  |
                 *  ------->-------->-------->
                 *          |        |
                 *          |        |
                 */
                
                if constexpr ( targs.restart_walk_backQ )
                {
                    s_begin = WalkBackToStrandStart( NextArc(a_ptr,Tail) );
                }
                else
                {
                    s_begin = a_ptr;
                }
                
                // TODO: Catch the Big Unlink here.
            }
        }
        else
        {
            if( NewStrand() ) [[unlikely]] { break; }
            s_begin = WalkBackToStrandStart(a_ptr);
            // TODO: Catch the Big Unlink here.
        }
        
        
        //if( s_begin == Uninitialized ) { continue; }
        
#ifdef PD_DEBUG
        const bool s_begin_optimalQ = (pd->ArcOverQ(s_begin,Tail) != overQ);
        
        if( (s_begin != a_ptr) && !s_begin_optimalQ )
        {
            wprint("s_begin is placed suboptimally although s_begin != a_ptr. (s_begin = " + ToString(s_begin) + ", crossing_count = " + ToString(pd->crossing_count) + ")");
        }
        
        if( (s_begin == a_ptr) && !s_begin_optimalQ )
        {
            wprint("We should be able to remove a big loop here, no? (s_begin = " + ToString(s_begin) + ", crossing_count = " + ToString(pd->crossing_count) + ")");
        }
#endif // PD_DEBUG
        
        // TODO: Is this really necessary? The do loop below eliminates loops automatically.
        // We make sure that `s_begin` is not a loop.
        if( Reidemeister_I<true>(s_begin) )
        {
            ++change_counter;
            continue;   // TODO: Could we also walk to what was NextArc<Head>(s_begin)?
        }
        
        // If we arrive at this point again, we break the do loop below.
        const Int anchor = s_begin;

        // Arc `anchor` might be deactivated by RerouteToShortestPath_impl.
        // Thus, we need indeed a reference, not a copy here.
        ArcState_T & anchor_state = pd->A_state[anchor];
        
        // Current arc.
        Int a = s_begin;
        
        MarkCrossing(pd->A_cross(a,Tail));
        
        // Traverse forward through all arcs in the link component, until we return to `anchor`.
        do
        {
            PD_ASSERT(CrossingMarkedQ(pd->A_cross(a,Tail)));
            
            Int c_1 = pd->A_cross(a,Head);
            AssertCrossing<1>(c_1);
        
            // Check for forward Reidemeister I move.
            const bool side_1 = (pd->C_arcs(c_1,In,Right) == a);
            Int a_next = pd->C_arcs(c_1,Out,!side_1);
            AssertArc<1>(a_next);
            
            Int c_next = pd->A_cross(a_next,Head);
            AssertCrossing<1>(c_next);
            
            if( c_next == c_1 )
            {
                // overQ == true
                //
                //                  +-----+
                //      |     |     |     | a_next
                //      |     |  a  |     |
                // ---------------->|-----+
                //      |     |     |c_1
                //      |     |     |

                // overQ == true
                //
                //                  +-----+
                //      |     |     |     | a_next
                //      |     |  a  |     |
                // ---------------->------+
                //      |     |     |c_1
                //      |     |     |
                
                // In any case, the loop would prevent the strand from going further.
                (void)Reidemeister_I<false>(a_next);
                ++change_counter;
                
                // We have to guarantee that arc `a` is still alive and part of the strand.
                // It is a bit strange, that we do not check pd.ArcActiveA(a) here.
                // Reidemeister_I can deactivate arc `a` only if `a` is part of a 2-loop.
                // But that can happen only if `a ==  anchor`.
                if( ActiveQ(anchor_state) )
                {
                    continue; // Analyze `a` and the new `a_next` once more
                }
                else
                {
                    break;  // Search for new anchor.
                }
            }
            
            // If we land here, then `a_next` is not a loop arc.
            // The arc `a` might still be a loop arc, though. But then CrossingMarkedQ(c_1) == true, and the check for big loops below will detect it and remove it correctly.
            
            // Make arc `a` an official member of the strand.
            ++strand_arc_count;
            MarkArc(a);
            
            // Check whether the strand is finished.
            strand_completeQ = ((side_1 == CrossingRightHandedQ(c_1)) == overQ);
            
            PD_ASSERT( strand_completeQ == (ArcOverQ(a     ,Head) != overQ) );
            PD_ASSERT( strand_completeQ == (ArcOverQ(a_next,Tail) != overQ) );


            // Check for a big loop strand.i.e., an over/understrand that starts and ends at the same crossing.
            // TODO: Simplify everything to a single call to RemoveLoopPath(a,c_1) and break.
            // TODO: Don't forget to catch the Big Hopf Link in RemoveLoopPath.
            // TODO: In some cases we might reset the current strand
            
            if( CrossingMarkedQ(c_1) )
            {
                // Vertex c has been visited before.
                // This catches also the case when `a` is a loop arc.

                if constexpr ( mult_compQ )
                {
                    if( !strand_completeQ )
                    {
                        // There are two possibilities:
                        
                        /* Case 1:  (example for overQ = true)
                         *
                         *          |   |   |
                         *      +<--------------+
                         *      |   |   |   |   ^
                         *      |               |
                         *    --|--           --|--
                         *      |               |
                         *    --|--           --|--
                         *      |               ^ s_begin
                         *      v   |   |   | a |
                         *      +-------------->-------->
                         *          |   |   |   ^c_1    a_next
                         *                      |
                         *                      |
                         */
                        
                        /* Case 2: Big Unlink (s_begin == a_nex)
                         *
                         *          |   |   |
                         *      +<--------------+
                         *      |   |   |   |   ^
                         *      |               |
                         *    --|--           --|--
                         *      |               |
                         *    --|--           --|--
                         *      |               ^
                         *      v     a |s_begin|
                         *      +------>------->+
                         *              |c_1
                         *              |
                         */
                        
                        // Both cases can be handled by  RemoveLoopPath.
                        RemoveLoopPath(a,c_1);
                        break;
                        // TODO: Can't we start a new strand nearby?
                    }
                    else if ( a_next != s_begin )
                    {
                        PD_ASSERT(a_next != s_begin);
                        
                        
                        /* Assume overQ = true; just one example:
                         *
                         *          |   |   |
                         *      +---------------+
                         *      |   |   |   |   ^ s_begin
                         *      |               |
                         *    --|--           --|--
                         *      |               |
                         *    --|--           --|--
                         *      |               |
                         *      v   |   |   | a | a_next != s_begin
                         *      +-------------->|-------+
                         *          |   |   |   ^c_1
                         *                      |
                         *                      |
                         */
                        
                        // TODO: We could set strand_arc_count = 0 and a = s_begin are restart the search for this strand.
                        RemoveLoopPath(a,c_1);
                        break;
                    }
                    else // if( strand_completeQ && (a_next == s_begin) )
                    {
                        // The following case cannot occur because we would have run into the if-case strand_completeQ == false before.
                        
                        /* Assume overQ = true; just one example:
                         *
                         *         |   |   |
                         *      +--------------+
                         *      |  |   |   |   ^
                         *      |              |
                         *    --|--          --|--
                         *      |              |
                         *    --|--          --|--
                         *      |              |
                         *      v  |   |   | a | a_next == s_begin
                         *      +------------->|------->
                         *         |   |   |   ^c_1
                         *                     |
                         *                     |
                         */
                        
                        // This is the tiny version of the above
                        
                        /* Example for overQ = true. It cannot occur for the same reason.
                         *
                         *    +-------+
                         *    |       |
                         *    |       |
                         *    |    a  |s_begin == a_next
                         *    +------>|-------
                         *         c_1|
                         *            |
                         *            |
                         *
                         */
                        
                        // However, the earlier call(s) to Reidemeister_I rule this case out as `a_next` cannot be a loop arc.
                        
                        // TODO: Cutting the loop here will probably be better.
                        // TODO: Would  RemoveLoopPath(a,c_1); work?
                        // TODO: Moreover, we could set strand_arc_count = 0 and a = s_begin are restart the search for this strand.
                        
                        
                        // The only other possible configuration here is the Big Hopf Link.
                        // The RerouteToShortestPath_impl will reroute this just fine. We could of course provide a shorter, slightly more efficient implementation that also disconnects the Hopf link.
                        
                        // For now, we just do not break here and let the algorithm continue.
                        // Dijkstra should terminate quite quickly, so we should be happy about getting rid of this mental load, in particular, because this is probably not a frequent case.
                        
                        /* "Big Hopf Link"
                         *
                         *            #       #
                         *            #       #
                         *            |       |
                         *            |       |
                         *         a  |s_begin|
                         *      +-----|-------------+
                         *      |  c_1|       |     |
                         *      |     |       |     |
                         *      |     |       |     |
                         *      |     |       |  +--|--##
                         *  ##--|--+  +-------+  |  |
                         *      |  |             |  |
                         *      +-------------------+
                         *         |             |
                         *         #             #
                         */
                        
                        /* Simplified "Big Hopf Loop"
                         *
                         *            #       #
                         *            #       #
                         *            |       |
                         *            |       |
                         *         a  |s_begin|
                         *      +-----|---+   |
                         *      |  c_1|   |   |
                         *      |     |c_2|   |
                         *      +---------+   |
                         *            |       |  +-----##
                         *  ##-----+  +-------+  |
                         *         |             |
                         *         |             |
                         *         #             #
                         */
                    
                    }
                }
                else // single component link
                {
                    // The only cases I can imagine where ` RemoveLoopPath` won't work is this unknot:
                    
                    /*            +-------+
                     *            |       |
                     *            |       |
                     *         a  |s_begin|== a_next
                     *    +-------|-------+
                     *    |    c_1|
                     *    |       |
                     *    |       |
                     *    +-------+
                     */
                    
                    // However, the earlier call(s) to Reidemeister_I rule this case out as `a_next` cannot be a loop arc.
                    
                    RemoveLoopPath(a,c_1);
                    break;
                }
                
            } // if( CrossingMarkedQ(c_1) )
            
            // We do not like Reidemeister II patterns along our strand because that would make rerouting difficult.
            if( (!strand_completeQ) && (a != s_begin) )
            {
                // overQ == true
                //
                //               +--------+
                //      |        |        |
                //      | a_prev |   a    | a_next
                // -----X---------------->-------->
                //      |        |        |c_1
                //      |        |        |
                
                // Caution: This might change `a`.
                const bool changedQ = Reidemeister_II_Backward(a,c_1,side_1,a_next);
                // If `a` is deactivated by this, then Reidemeister_II_Backward subtracts 2 from strand_arc_count.
                // TODO: The only thing that can go wrong here is that the strand start `s_begin` is deactivated by Reidemeister_II_Backward (see the function body for details).
                // So, we need the following check here.
                
                if( changedQ )
                {
                    if( ActiveQ(anchor_state) && ArcActiveQ(s_begin) )
                    {
                        continue; // Analyze `a` and the new `a_next` once more
                    }
                    else
                    {
                        break; // Search for new anchor.
                    }
                }
            }
            
            if( strand_completeQ )
            {
                // TODO: We could check for Reidemeister II moves like the following.
                // TODO: This might allow us to reroute a longer strand now and to save some work later. (Not sure whether this will ever ammortize.)
                
                /* overQ == true
                 *
                 *                        +--------+
                 *      |        |        |        |
                 *      |        |   a    | a_next |
                 * ---------------------->|------->|----
                 *      |        |        |c_1     |
                 *      |        |        |        |
                 */
                
                
                // Now try to reroute!
                
                bool changedQ = false;

                if( (strand_arc_count > Int(2)) && (args.max_dist > Int(0)) )
                {
#ifdef PD_DEBUG
                    Int C_0 = pd->crossing_count;
#endif
                    changedQ = RerouteToShortestPath_impl( s_begin, a, Min(strand_arc_count-Int(1),args.max_dist) );
#ifdef PD_DEBUG
                    Int C_1 = pd->crossing_count;
                    PD_ASSERT( !changedQ || (C_1 < C_0) );
#endif // PD_DEBUG
                }

                // RerouteToShortestPath_impl might deactivate `anchor`, so that we could never return to it.
                // Hence, we rather break the while loop here.
                if( changedQ && !ActiveQ(anchor_state) ) [[unlikely]] { break; } // Search new anchor.
                
                // Surprisingly, these breaks tend to make it faster!
                if constexpr( !targs.restart_after_successQ ) { if(  changedQ ) { break; } }
                if constexpr( !targs.restart_after_failureQ ) { if( !changedQ ) { break; } }
                
                // Changed or not; in any case we can try to start a new strand at `pd->NextArc(a_next,Tail)`. But `a_next` must be active for this.
                if( changedQ && !ArcActiveQ(a_next) ) [[unlikely]] { break; } // Search new anchor.

                PD_ASSERT(pd->ArcOverQ(a_next,Tail) != overQ);
                
                // Rerouting arcs that we have already touched might be pointless. Here is a guard against that.
                if ( !args.reroute_markedQ )
                {
                    if( ArcRecentlyMarkedQ(a_next) ) [[unlikely]] { break; }
                }
                
                if constexpr ( targs.restart_change_typeQ )
                {
                    // We start a strand of opposite type.
                    SetStrandMode(!overQ);
                }
                
                if( NewStrand() ) [[unlikely]] { break; }
                
                if constexpr ( targs.restart_change_typeQ )
                {
                    if constexpr( targs.restart_walk_backQ )
                    {
                        s_begin = WalkBackToStrandStart(pd->NextArc(a_next,Tail));
                        // TODO: Catch Big Unlink here.
                        //if( s_begin == Uninitialized ) { break; }
                    }
                    else
                    {
                        s_begin = pd->NextArc(a_next,Tail);
                    }
                }
                else // if constexpr ( !targs.restart_change_typeQ )
                {
                    if constexpr( targs.restart_walk_backQ )
                    {
                        s_begin = WalkBackToStrandStart(a_next);
                        // TODO: Catch Big Unlink here.
                        //if( s_begin == Uninitialized ) { break; }
                    }
                    else
                    {
                        s_begin = a_next;
                    }
                }
                
                a = s_begin;
                c_1 = pd->A_cross(a,Tail);
                MarkCrossing(c_1);
                continue; // Analyze a again, this time as first arc of new strand.
            }
            else
            {
                AssertArc<1>(a_next);
                AssertArc<1>(anchor);
                AssertArc<1>(a);
                a = a_next;
                MarkCrossing(c_1); // `c_1` is now the tail of `a`; thus we mark it.
                continue;
            }
        }
        while( a != anchor );
        
        // Go back to top and start a new strand.
        ++a_ptr;
    }
    
    PD_VALPRINT("pd->crossing_count",pd->crossing_count);
    
    Cleanup();
    
    return change_counter;
    
} // Int SimplifyStrands
