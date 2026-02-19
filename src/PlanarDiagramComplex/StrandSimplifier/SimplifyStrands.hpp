public:


struct SimplifyStrands_Args
{
    Int  max_dist              = Scalar::Max<Int>;
    bool overQ                 = true;
    bool compressQ             = true;
    Int  compression_threshold = 100;
};

friend std::string ToString( cref<SimplifyStrands_Args> args )
{
    return std::string("{ ")
         +   ".max_dist = " + ToString(args.max_dist)
         + ", .overQ = " + ToString(args.overQ)
         + ", .compressQ = " + ToString(args.compressQ)
         + ", .compression_threshold = " + ToString(args.compression_threshold)
         + " }";
}

struct SimplifyStrands_TArgs
{
    bool restart_after_successQ = true;
    bool restart_after_failureQ = true;
    bool restart_walk_backQ     = true;
    bool interleave_over_underQ = true;
    bool R_II_blockingQ         = true;
    bool R_II_forwardQ          = false;
};

friend std::string ToString( cref<SimplifyStrands_TArgs> targs )
{
    return "{.restart_after_successQ = " + ToString(targs.restart_after_successQ)
         + ",.restart_after_failureQ = " + ToString(targs.restart_after_failureQ)
         + ",.restart_walk_backQ = " + ToString(targs.restart_walk_backQ)
         + ",.interleave_over_underQ = " + ToString(targs.interleave_over_underQ)
         + ",.R_II_blockingQ = " + ToString(targs.R_II_blockingQ)
         + ",.R_II_forwardQ = " + ToString(targs.R_II_forwardQ)
         + "}";
}


// TODO: If a loops is removed that does not finish a component, we can try to restart as well.

/*!@brief This is the main routine of the class. It is supposed to reroute all over/understrands to shorter strands, if possible. It does so by traversing the diagram and looking for over/understrand. When a complete strand is detected, it runs Dijkstra's algorithm in the dual graph of the diagram _without the currect strand_. If a shorter path is detected, the strand is rerouted. The returned integer is a rough(!) indicator of how many changes accoured. 0 is returned only of no changes have been made and if there is no need to call this function again. A positive value indicates that it would be worthwhile to call this function again (maybe after some further simplifications).
 *
 *
 */

template<SimplifyStrands_TArgs targs>
Size_T SimplifyStrands( mref<PD_T> pd_input, cref<SimplifyStrands_Args> args )
{
    [[maybe_unused]] auto tag = [this]() { return this->MethodName("SimplifyStrands"); };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("targs",ToString(targs));
    logvalprint("args ",ToString(args) );
#endif

    if( args.max_dist <= Int(0) ) { return 0; }
    
    PD_PRINT(tag()+ ": Initial number of crossings = " + ToString(pd_input.CrossingCount()) );
    
    if( args.compressQ ) { pd_input.ConditionalCompress(args.compression_threshold); }

    if constexpr ( !targs.interleave_over_underQ )
    {
        SetStrandMode(args.overQ);
    }
    LoadDiagram(pd_input);
    NewStrand();
    
    Int a_ptr = 0;
    change_counter   = 0;

    const Int pd_max_arc_count = pd->MaxArcCount();
    
    PD_VALPRINT("pd->crossing_count",pd->crossing_count);
    
    while( a_ptr < pd_max_arc_count )
    {
        // Search for next arc that is active and has not yet been handled.
        while(
            ( a_ptr < pd_max_arc_count ) && (!ArcActiveQ(a_ptr) || ArcRecentlyMarkedQ(a_ptr) )
        )
        {
            ++a_ptr;
        }

        if( a_ptr >= pd_max_arc_count ) [[unlikely]]
        {
            PD_PRINT("Breaking many loop because we marked all active arcs; we ar done for this round.");
            break;
        }
        
        PD_VALPRINT("a_ptr",a_ptr);
        
        // Find the beginning of strand.
        if constexpr ( targs.interleave_over_underQ )
        {
            const bool overQ_0 = ArcOverQ(a_ptr,Tail);
            const bool overQ_1 = ArcOverQ(a_ptr,Head);
            
            SetStrandMode(overQ_1);
            NewStrand();
            
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
                
                SetStrandBegin( a_ptr );
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
                    SetStrandBegin( WalkBackToStrandStart(NextArc(a_ptr,Tail)) );
                }
                else
                {
                    SetStrandBegin( a_ptr );
                }
            }
        }
        else
        {
            NewStrand();
            SetStrandBegin( WalkBackToStrandStart(a_ptr) );
        }
        
        PD_ASSERT( pd->ArcOverQ(s_begin,Head) == overQ );
        
        // TODO: Is this really necessary? The do loop below eliminates loops automatically.
        // We make sure that `s_begin` is not a loop.
        if( Reidemeister_I<true>(s_begin) )
        {
            ++change_counter;
            continue;   // TODO: Could we also walk to what was NextArc<Head>(s_begin)?
        }
        
        // Current arc.
        Int a = s_begin;
        MarkCrossing(pd->A_cross(a,Tail));
        
        // TODO: I could guard this by a simple counter.
        // Potentially infinite loop. We must take care to break it correctly.
        while( true )
        {
            AssertArc<1>(a);
//            PD_ASSERT(CheckForDanglingMarkedArcs(a));
            
#ifdef PD_DEBUG
            
            const Int fake_strand_arc_count = CountArcsInRange(s_begin,a);
            TOOLS_LOGDUMP(fake_strand_arc_count);
            logvalprint("strand",ShortArcRangeString(s_begin,a));
            if( fake_strand_arc_count != strand_arc_count + Int(1) )
            {
                pd_eprint("fake_strand_arc_count != strand_arc_count + Int(1)");
            }
#endif // PD_DEBUG

            
            // TODO: We could preload c_0. Won't help much, though.
            Int c_0 = pd->A_cross(a,Tail);
            Int c_1 = pd->A_cross(a,Head);
            AssertCrossing<1>(c_0);
            AssertCrossing<1>(c_1);
            PD_ASSERT(CrossingMarkedQ(c_0));
            
            // Check for forward Reidemeister I move.
            const bool side_1 = (pd->C_arcs(c_1,In,Right) == a);
            Int a_next = pd->C_arcs(c_1,Out,!side_1);
            AssertArc<1>(a_next);
            
            Int c_2 = pd->A_cross(a_next,Head);
            AssertCrossing<1>(c_2);

            PD_ASSERT(
                (strand_arc_count <= Int(2)) || (pd->A_cross(s_begin,Head) != pd->A_cross(a,Tail))
            );
            
            
            if( c_2 == c_1 )
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
                if( !ArcActiveQ(a) )
                {
                    PD_PRINT("Breaking because (ArcActiveQ(a) == false) after Reidemeister_I.");
                    break;  // Search for new arc by incrementing a_ptr.
                }
                
                if( !ArcActiveQ(s_begin) )
                {
                    PD_PRINT("Breaking because (ArcActiveQ(s_begin) == false) after Reidemeister_I.");
                    break;  // Search for new arc by incrementing a_ptr.
                }
                
                continue; // Analyze `a` and the new `a_next` once more
            }
            
            // If we land here, then `a_next` is not a loop arc.
            // The arc `a` might still be a loop arc, though. But then CrossingMarkedQ(c_1) == true, and the check for big loops below will detect it and remove it correctly.
            
            // Make arc `a` an official member of the strand.
            ++strand_arc_count;
            MarkArc(a);
            
            // TODO: Expensive1
            PD_ASSERT(CheckStrand<false>(s_begin,a));
            
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
                PD_PRINT("Visiting marked crossing c_1 = " + CrossingString(c_1) + ".");
                // Vertex c has been visited before.
                // This catches also the case when `a` is a loop arc.


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
                    
                    /* Case 2: Big Unlink (s_begin == a_next)
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
                    PD_PRINT("Breaking because we called RemoveLoopPath. We might or might not recover from here, though. (strand_arc_count = " + ToString(strand_arc_count) + ")");
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
                    PD_PRINT("Breaking because we called RemoveLoopPath. We might recover from here, though. (strand_arc_count = " + ToString(strand_arc_count) + ")");
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
            } // if( CrossingMarkedQ(c_1) )
            
            
            // TODO: We might make our life easier if we check whether CrossingMarkedQ(c_2) == true.
            
            PD_ASSERT(!CrossingMarkedQ(c_1)); // We have just checked that.
            PD_ASSERT(c_0 != c_1);             // Because of CrossingMarkedQ(c_0).
            
            
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
                bool changedQ = Reidemeister_II_Backward(c_0,a,c_1,side_1,a_next);
                
                change_counter += changedQ;
                
                // If `a` is deactivated by this, then Reidemeister_II_Backward subtracts 2 from strand_arc_count.
                // TODO: The only thing that can go wrong here is that the strand start `s_begin` is deactivated by Reidemeister_II_Backward (see the function body for details).
                // So, we need the following check here.
                
                if( changedQ )
                {
                    // TODO: This check might be superfluous.
                    if( !ArcActiveQ(a) )
                    {
                        PD_PRINT("Breaking because (ArcActiveQ(a) == false) after Reidemeister_II_Backward.");
                        break;
                    }
                    if( !ArcActiveQ(s_begin) )
                    {
                        PD_PRINT("Breaking because (ArcActiveQ(s_begin) == false) after Reidemeister_II_Backward.");
                        break;
                    }
                    continue; // Analyze `a` and the new `a_next` once more
                }
            }
            

            if constexpr ( targs.R_II_blockingQ || targs.R_II_forwardQ )
            {
                if ( targs.R_II_forwardQ || strand_completeQ )
                {
                    if( CrossingMarkedQ(c_2) )
                    {
                        if( strand_arc_count > Int(2) )
                        {
                            PD_NOTE("CrossingMarkedQ(c_2). We do not attempt Reidemeister_II_Forward. However, this would be a great rerouting opportunity. (strand_arc_count = " + ToString(strand_arc_count) + ")");
                            
                            PD_VALPRINT("strand", ArcRangeString(s_begin,a));
                        }
                    }
                    else
                    {
                        const Int b = pd->NextArc(a_next,Head,c_2);
                        
                        PD_ASSERT(!CrossingMarkedQ(c_2));
                        PD_ASSERT(a != b);  // Because ArcMarkedQ(a) and !CrossingMarkedQ(c_2)
                        
                        // TODO: We might reuse side_1 here.
                        if( Reidemeister_II_Forward(a,c_1,a_next,c_2,b) )
                        {
                            AssertArc<1>(a);
                            // I think this cannot happen by the preconditions.
                            if( !pd->ArcActiveQ(s_begin) )
                            {
                                PD_NOTE("Breaking because Reidemeister_II_Forward deactivated s_begin. This would have been a rerouting opportunity.");
                                // We deleted our strand start.
                                // TODO: Can we recover from that?
                                break;
                            }
                            else
                            {
                                continue; // Analyze the modified arc `a` again.
                            }
                        }
                    }
                }
            }
            
            if( strand_completeQ )
            {
                PD_VALPRINT("strand", ShortArcRangeString(s_begin,a));
                
                // Now try to reroute!
                bool changedQ = false;

                if( (strand_arc_count > Int(2)) && (args.max_dist > Int(0)) )
                {
#ifdef PD_DEBUG
                    PD_ASSERT(CheckStrand(s_begin,a));
                    Int C_0 = pd->crossing_count;
                    pd->PrintInfo();
                    
#endif // PD_DEBUG
                    
                    changedQ = RerouteToShortestPath_impl(
                        s_begin, a, Min(static_cast<Int>(strand_arc_count-Int(1)),args.max_dist)
                    );
#ifdef PD_DEBUG
                    Int C_1 = pd->crossing_count;
                    PD_ASSERT( !changedQ || (C_1 < C_0) );
#endif // PD_DEBUG
                }
                
                if( changedQ )
                {
                    if constexpr( targs.restart_after_successQ )
                    {
                        PD_PRINT("Attempting to start new strand after successful rerouting.");
                    }
                    else
                    {
                        PD_PRINT("Breaking after successful rerouting because of targs.restart_after_successQ == false.");
                        break;
                    }
                    
                    // We may try to start a new strand at `pd->NextArc(a_next,Tail)`, but `a_next` must be active for this.
                    if( !ArcActiveQ(a_next) ) [[unlikely]]
                    {
                        PD_PRINT("Breaking because " + ArcString(a_next)+ " is inactive.");
                        break; // Search new arc by incrementing a_ptr.
                    }
                    
                    // Caution: setting current arc to _new_ last arc of strand.
                    a = pd->NextArc(a_next,Tail);
                }
                else
                {
                    if constexpr( targs.restart_after_failureQ )
                    {
                        PD_PRINT("Attempting to start new strand after aborted rerouting.");
                    }
                    else
                    {
                        PD_PRINT("Breaking after aborted rerouting because of targs.restart_after_failureQ == false.");
                        break;
                    }
                    
                    // We may try to start a new strand at `pd->NextArc(a_next,Tail)`, but `a_next` must be active for this.
                    PD_ASSERT(ArcActiveQ(a_next));
                }
                
                PD_ASSERT(a == pd->NextArc(a_next,Tail));
                PD_ASSERT(pd->ArcOverQ(a_next,Tail) != overQ);
                PD_ASSERT(pd->ArcOverQ(a     ,Head) != overQ);
                
                // Rerouting arcs that we have been marked recently is a bad idea.
                if( ArcRecentlyMarkedQ(a_next) ) [[unlikely]]
                {
                    PD_PRINT("Breaking because " + ArcString(a_next)+ " has been visited recently.");
                    break; // Search new arc by incrementing a_ptr.
                }
                
                if constexpr ( targs.interleave_over_underQ )
                {
                    // We start a strand of opposite type.
                    SetStrandMode(!overQ);
                    PD_PRINT("Changed strand type to " + OverQString() + "strand.");
                }
                else
                {
                    PD_PRINT("Strand type remains at " + OverQString() + "strand.");
                }
                
                NewStrand();
                
                if constexpr ( targs.interleave_over_underQ )
                {
                    if constexpr( targs.restart_walk_backQ )
                    {
                        PD_PRINT("Walking back to strand start from previous arc of " + ArcString(a_next)+ ".");
                        
                        PD_ASSERT(a == pd->NextArc(a_next,Tail));
                        SetStrandBegin( WalkBackToStrandStart(a) );
                    }
                    else
                    {
                        PD_PRINT("Starting at previous arc of " + ArcString(a_next)+ ".");
                        PD_ASSERT(a == pd->NextArc(a_next,Tail));
                        SetStrandBegin( a );
                    }
                }
                else // if constexpr ( !targs.interleave_over_underQ )
                {
                    if constexpr( targs.restart_walk_backQ )
                    {
                        PD_PRINT("Walking back to strand start from " + ArcString(a_next)+ ".");
                        SetStrandBegin( WalkBackToStrandStart(a_next) );
                    }
                    else
                    {
                        PD_PRINT("Starting at " + ArcString(a_next)+ ".");
                        SetStrandBegin( a_next );
                    }
                }
                
                a = s_begin;
                MarkCrossing(pd->A_cross(a,Tail));
                continue; // Analyze arc `a` again, this time as first arc of new strand.
            }
            else
            {
                AssertArc<1>(a_next);
                AssertArc<1>(a);
                
                PD_PRINT("Moving current arc to next arc.");
                a = a_next;
                PD_ASSERT(c_1 == pd->A_cross(a,Tail));
                MarkCrossing(c_1); // `c_1` is now the tail of `a`; thus we mark it.
                continue;
            }
        }
        
        PD_PRINT("Searching for new arc by incrementing a_ptr.");
        // Go back to top and start a new strand.
        ++a_ptr;
    }
    
    PD_VALPRINT("pd->crossing_count",pd->crossing_count);
    
    Cleanup();
    
    return change_counter;
    
} // Int SimplifyStrands
