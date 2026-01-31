public:

/*!@brief This is the main routine of the class. It is supposed to reroute all over/understrands to shorter strands, if possible. It does so by traversing the diagram and looking for over/understrand. When a complete strand is detected, it runs Dijkstra's algorithm in the dual graph of the diagram _without the currect strand_. If a shorter path is detected, the strand is rerouted. The returned integer is a rough(!) indicator of how many changes accoured. 0 is returned only of no changes have been made and if there is no need to call this function again. A positive value indicates that it would be worthwhile to call this function again (maybe after some further simplifications).
 *
 * @param overQ_ Whether to simplify overstrands (true) or understrands(false).
 *
 * @param max_dist Maximal lengths of rerouted strands. If no rerouting is found that makes the end arcs this close to each other, the rerouting attempt is aborted. Typically, we should set this to "infinity".
 *
 * @tparam R_II_Q Whether to apply small Reidemeister II moves on the way.
 */

template<bool R_II_Q = true>
Size_T SimplifyStrands(
    mref<PD_T> pd_input, bool overQ_, const Int max_dist = Scalar::Max<Int>
)
{
    // TODO: We could interleave overstrand and understrand search: The last arc of an overstrand is always the first arc of an understrand; the last arc of an understrand is always the first arc of an overstrand.
    SetStrandMode(overQ_);
    
    [[maybe_unused]] auto tag = [this,max_dist]()
    {
        return ClassName()+"::Simplify" + (overQ ? "Over" : "Under")  + "Strands<" + ToString(R_II_Q) + ">(" + ToString(max_dist) + ")";
    };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    if( max_dist <= Int(0) ) { return 0; }
    
    PD_PRINT(tag()+ ": Initial number of crossings = " + ToString(pd_input.CrossingCount()) );
    
    Load(pd_input);
    
    const Int pd_max_arc_count = pd->max_arc_count;
    
    // We increase `current_mark` for each strand. This ensures that all entries of D_data, A_mark, C_mark etc. are invalidated. In addition, `Prepare` resets these whenever current_mark is at least half of the maximal value.
    // We typically use Int = int32_t (or greater). So we can handle 2^30-1 strands in one call to `SimplifyStrands`. That should really be enough for all feasible applications.
    
    NewMark();
    a_ptr = 0;
    
    const Int old_mark = current_mark;

    strand_arc_count = 0;
    change_counter = 0;
    
    while( a_ptr < pd_max_arc_count )
    {
        // Search for next arc that is active and has not yet been handled.
        while(
            ( a_ptr < pd_max_arc_count )
            &&
            ( (A_mark(a_ptr) >= old_mark ) || (!ArcActiveQ(a_ptr)) )
        )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= pd_max_arc_count ) { break; }
        
        // Find the beginning of first strand.
        Int a_begin = WalkBackToStrandStart(a_ptr);
        
        // If we arrive at this point again, we break the do loop below.
        const Int a_0 = a_begin;
        
        // We should make sure that a_0 is not a loop.
        if( Reidemeister_I<true>(a_0) )
        {
            ++change_counter;
            continue;
        }
        
        // Might be changed by RerouteToShortestPath_impl.
        // Thus, we need indeed a reference, not a copy here.
        ArcState_T & a_0_state = pd->A_state[a_0];
        
        // Current arc.
        Int a = a_0;
        
        strand_arc_count = 0;
        
        MarkCrossing( pd->A_cross(a,Tail) );
        
        // Traverse forward through all arcs in the link component, until we return where we started.
        do
        {
            ++strand_arc_count;
            
            // Safe guard against integer overflow.
            if( current_mark >= max_mark )
            {
#ifdef PD_TIMINGQ
                const Time stop_time = Clock::now();
                
                Time_SimplifyStrands += Tools::Duration(start_time,stop_time);
#endif
                // Return a positive number to encourage that this function is called again upstream.
                return change_counter+1;
            }
            
            Int c_1 = pd->A_cross(a,Head);
            AssertCrossing<1>(c_1);
            
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
                
                // TODO: We could pass `a` as `a_prev` to `Reidemeister_I`.
                (void)Reidemeister_I<false>(a_next);
                
                PD_ASSERT(a_next != a);
                
                ++change_counter;
                --strand_arc_count;
                
                // TODO: I think it would be safe to continue without checking that `a_0` is still active. We should first complete the current strand.
                
                if( ActiveQ(a_0_state) )
                {
                    continue; // Walk to next arc.
                }
                else
                {
                    break;  // Stop the strand here.
                }
            }

            MarkArc(a);
            
            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
            // This finds out whether we have to reset.
            strand_completeQ = ((side_1 == CrossingRightHandedQ(c_1)) == overQ);
            
            // TODO: This might require already that there are no possible Reidemeister II moves along the strand. (?)
            
            // Check for loops, i.e., a over/understrand that starts and ends at the same crossing. This is akin to a "big Reidemeister I" move on top (or under) the diagram.
            
            if( CrossingMarkedQ(c_1) )
            {
                // Vertex c has been visited before.
                
                
                /*            |
                 *            |
                 *         a  | a_ptr
                 *     -------|-------
                 *         c_1|
                 *            |
                 *            |
                 */

                // TODO: Test this.
                if constexpr ( mult_compQ )
                {
                    // We must insert this check because we might otherwise get problems when there is a configuration like this:
                    
                    /* "Big Hopf Loop"
                     *
                     *            #       #
                     *            #       #
                     *            |       |
                     *            |       |
                     *         a  | a_ptr |
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

                    
                    if( !strand_completeQ || !ArcMarkedQ(a_next) )
                    {
                        RemoveLoop(a,c_1);
                        break;
                    }
                    else
                    {
                        // TODO: What we have here could be a "big Hopf link", no? It should still be possible to reroute this to get rid of all but one crossings on the strand. We could reconnect it like this or increment a "Hopf loop" counter of the corresponding link component.
                        
                        /* "Big Hopf Loop"
                         *
                         *            #       #
                         *            #       #
                         *            |       |
                         *            |       |
                         *         a  | a_ptr |
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
                         *         a  | a_ptr |
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
                        
                        // The Dijkstra algorithm should be able to simplify this. We could of course provide a shorter, slightly more efficient implementation. But I Dijkstra should terminate quite quickly, so we should be happy about getting rid of this mental load. We we just do not break here and let the algorithm continue.
                        
                        
                        // TODO: CAUTION: Also this is possible:
                        
                        /* "Over Unknot"
                         *
                         *
                         *              +-------+
                         *              |       |
                         *       ##-----|-------|----##
                         *              |       |
                         *          a   | a_ptr |
                         *      +-------|-------+
                         *      |    c_1|
                         *      |       |
                         *  ##--|-------|------##
                         *      |       |
                         *      +-------+
                         *
                         */

                        // TODO: I am not entirely sure whether Dijkstra will resolve this correctly.
                    }
            
                }
                else // single component link
                {
                    // The only case I can imagine where `RemoveLoop` won't work is this unknot:
                    
                    /*            +-------+
                     *            |       |
                     *            |       |
                     *         a  | a_ptr |
                     *    +-------|-------+
                     *    |    c_1|
                     *    |       |
                     *    |       |
                     *    +-------+
                     */
                    
                    // However, the earlier call(s) to Reidemeister_I should rule this case out. So we have the case of a "Big Reidemeister I" here, and that is what `RemoveLoop` is made for.
                    
                    RemoveLoop(a,c_1);
                    break;
                }
            }
            
            if constexpr ( R_II_Q )
            {
                if( (!strand_completeQ) && (a != a_begin) )
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
                    
                    if( changedQ )
                    {
                        if( ActiveQ(a_0_state) )
                        {
                            // Reidemeister_II_Backward reset `a` to the previous arc.
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            
            if( strand_completeQ )
            {
                
                // TODO: We could check for Reidemeister II moves like this:
                
                /* overQ == true
                 *
                 *                        +--------+
                 *      |        |        |        |
                 *      |        |   a    | a_next |
                 * ---------------------->|------->|----
                 *      |        |        |c_1     |
                 *      |        |        |        |
                 */
                
                // TODO: I could also run ArcSimplifier_T::Process(a_next).
                // TODO: This might allow us to reroute a longer strand now and to save some work later. (Not sure whether this ammortizes.)
                // TODO: We could also give it the information that a is not a loop. (At least, I think we know this at this point.
                // TODO: The only problem with that would be that ArcSimplifier_T does not tell us how far we have to backtrack a.
                
                bool changedQ = false;

                if( (strand_arc_count > Int(2)) && (max_dist > Int(0)) )
                {
                    changedQ = RerouteToShortestPath_impl(
                        a_begin,a,
                        Min(strand_arc_count-Int(1),max_dist)
                    );
                }
                
                // TODO: Check this.

                // RerouteToShortestPath might deactivate `a_0`, so that we could never return to it. Hence, we rather break the while loop here.
                if( changedQ || (!ActiveQ(a_0_state)) ) { break; }
                
                // Create a new strand.
                strand_arc_count = 0;
                a_begin = a_next;

                NewMark();
            }
            
            // Head of arc gets new current_mark.
            MarkCrossing(c_1);

            AssertArc<1>(a_next);
            AssertArc<1>(a_0);
            AssertArc<1>(a);
            
            a = a_next;
        }
        while( a != a_0 );
        
        // TODO: If the link has multiple components, it can happen that we cycled around a full unlink lying on top (or below) the remaining diagram.
        
//                if( a == a_begin )
//                {
//                    wprint(ClassName()+"::SimplifyStrands: Split unlink detected. We have yet to remove it." );
//                }
        
        NewMark();
        strand_arc_count = 0;
        
        ++a_ptr;
    }
    
#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_SimplifyStrands += Tools::Duration(start_time,stop_time);
#endif
    
    Cleanup();
    
    return change_counter;
    
} // Int SimplifyStrands



void SetStrandMode( const bool overQ_ )
{
    overQ = overQ_;
}
