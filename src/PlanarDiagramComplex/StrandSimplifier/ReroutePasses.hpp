public:

/*!@brief This is the main routine of the class. It is supposed to reroute all over/understrands to shorter strands, if possible. It does so by traversing the diagram and looking for over/understrand. When a complete strand is detected, it runs Dijkstra's algorithm in the dual graph of the diagram _without the currect strand_. If a shorter path is detected, the strand is rerouted. The returned integer is a rough(!) indicator of how many changes accoured. 0 is returned only of no changes have been made and if there is no need to call this function again. A positive value indicates that it would be worthwhile to call this function again (maybe after some further simplifications).
 *
 * @param overQ_ Whether to simplify overstrands (true) or understrands(false).
 *
 * @param max_dist Maximal lengths of rerouted strands. If no rerouting is found that makes the end arcs this close to each other, the rerouting attempt is aborted. Typically, we should set this to "infinity".
 *
 */

Size_T ReroutePasses(
    bool overQ_, const Int max_dist = std::numeric_limits<Int>::max()
)
{
    // TODO: Maybe I should break this function into a few smaller pieces.
    
    SetStrandMode(overQ_);
    
    TOOLS_PTIMER(timer,ClassName()+"::Reroute" + (overQ ? "Over" : "Under")  + "Passes(" + ToString(max_dist) + ")" );
    
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    if( max_dist <= Int(0) ) { return 0; }
    
    Prepare();
    
    const Int m = A_cross.Dim(0);
    
    // We increase `current_mark` for each strand. This ensures that all entries of D_mark, D_from, A_visited_from, A_mark, C_mark etc. are invalidated. In addition, `Prepare` resets these whenever current_mark is at least half of the maximal integer of type Int (rounded down).
    // We typically use Int = int32_t (or greater). So we can handle 2^30-1 strands in one call to `ReroutePasses`. That should really be enough for all feasible applications.
    
    ++current_mark;
    a_ptr = 0;
    
    const Mark_T mark_0 = current_mark;

    strand_length = 0;
    change_counter = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while(
            ( a_ptr < m )
            &&
            ( (A_mark(a_ptr) >= mark_0 ) || (!pd.ArcActiveQ(a_ptr)) )
        )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m ) { break; }
        
        // Find the beginning of first strand.
        Int a_begin = WalkBackToStrandStart(a_ptr);
        
        // If we arrive at this point again, we break the do loop below.
        const Int a_0 = a_begin;
        
        // We should make sure that a_0 is not a loop.
        if( Reidemeister_I<true>(a_0) )
        {
//                    RepairArcLeftArcs();
            ++change_counter;
            continue;
        }
        
        // Might be changed by RerouteToShortestPath_impl.
        // Thus, we need indeed a reference, not a copy here.
        ArcState_T & a_0_state = A_state[a_0];
        
        // Current arc.
        Int a = a_0;
        
        strand_length = 0;
        
        MarkCrossing( A_cross(a,Tail) );
        
        // Traverse forward through all arcs in the link component, until we return where we started.
        do
        {
            ++strand_length;
            
            // Safe guard against integer overflow.
            if( current_mark == std::numeric_limits<Mark_T>::max() )
            {
#ifdef PD_TIMINGQ
                const Time stop_time = Clock::now();
                
                Time_ReroutePasses += Tools::Duration(start_time,stop_time);
#endif
                
                // Return a positive number to encourage that this function is called again upstream.
                return change_counter+1;
            }
            
            Int c_1 = A_cross(a,Head);

            AssertCrossing<1>(c_1);
            
            const bool side_1 = (C_arcs(c_1,In,Right) == a);
            
            Int a_next = C_arcs(c_1,Out,!side_1);
            AssertArc<1>(a_next);
            
            Int c_next = A_cross(a_next,Head);
            AssertCrossing<1>(c_next);
            
            // TODO: Not sure whether this adds anything good. RemoveLoop is also able to remove Reidemeister I loops. It just does it in a slightly different ways.
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
                --strand_length;
//                        RepairArcLeftArcs();
                
                // TODO: I think it would be safe to continue without checking that `a_0` is still active. We should first complete the current strand.
                
                if( a_0_state.ActiveQ() )
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            
            // Arc gets current current_mark.
            A_mark(a) = current_mark;
            
            
            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
            // This finds out whether we have to reset.
            strand_completeQ = ((side_1 == pd.CrossingRightHandedQ(c_1)) == overQ);
            
            // TODO: This might require already that there are no possible Reidemeister II moves along the strand. (?)
            
            // Check for loops, i.e., a over/understrand that starts and ends at the same crossing. This is akin to a "big Reidemeister I" move on top (or under) the diagram.
            
            if( C_mark(c_1) == current_mark )
            {
                // Vertex c has been visted before.
                
                
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


                    if( !strand_completeQ || (A_mark(a_next) != current_mark) )
                    {
                        RemoveLoop(a,c_1);
//                                RepairArcLeftArcs();
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
                    // The only case I can imagine where `RemoveLoop` won't wotk is this unknot:
                    
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
//                            RepairArcLeftArcs();
                    break;
                }
            }

            // Checking for Reidemeister II moves like this:
            
            // overQ == true
            //
            //                  +--------+
            //      |     |     |        |
            //      |     |  a  | a_next |
            // ---------------->|------->|----
            //      |     |     |c_1     |
            //      |     |     |        |
            
            // TODO: It might even be worth to do this before RemoveLoop is called.

            if constexpr ( R_II_Q )
            {
                if( (!strand_completeQ) && (a != a_begin) )
                {
                    // Caution: This might change `a`.
                    const bool changedQ = Reidemeister_II_Backward(a,c_1,side_1,a_next);
                    
                    if( changedQ )
                    {
//                                RepairArcLeftArcs();
                        
                        if( a_0_state.ActiveQ() )
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
                bool changedQ = false;

                if( (strand_length > Int(1)) && (max_dist > Int(0)) )
                {
                    changedQ = RerouteToShortestPath_impl(
                        a_begin,a,
                        Min(strand_length-Int(1),max_dist),
                        current_mark
                    );
                    
//                            if( changedQ ) { RepairArcLeftArcs(); }
                }
                
                // TODO: Check this.

                // RerouteToShortestPath might deactivate `a_0`, so that we could never return to it. Hence, we rather break the while loop here.
                if( changedQ || (!a_0_state.ActiveQ()) ) { break; }
                
                // Create a new strand.
                strand_length = 0;
                a_begin = a_next;

                ++current_mark;
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
        
//        if( a == a_begin )
//        {
//            wprint(ClassName()+"::ReroutePasses: Split unlink detected. We have yet to remove it." );
//        }
        
        ++current_mark;
        strand_length = 0;
        
        ++a_ptr;
    }
    
#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_ReroutePasses += Tools::Duration(start_time,stop_time);
#endif
    
    Cleanup();
    
    return change_counter;
    
}

//Int ReroutePasses( bool overQ_, const Int max_dist = std::numeric_limits<Int>::max() )
//{
//    // TODO: Maybe I should break this function into a few smaller pieces.
//    
//    SetStrandMode(overQ_);
//    
//    TOOLS_PTIMER(timer,MethodName("ReroutePasses") + (overQ ? "Over" : "Under")  + "Strands<" + ToString(R_II_Q) + ">(" + ToString(max_dist) + ")" );
//    
//#ifdef PD_TIMINGQ
//    const Time start_time = Clock::now();
//#endif
//    if( max_dist <= Int(0) ) { return 0; }
//    
//    Prepare();
//    
//    const Int m = A_cross.Dim(0);
//    
//    // We increase `current_mark` for each strand. This ensures that all entries of D_mark,D_from, A_visited_from, A_mark, C_mark etc. are invalidated. In addition, `Prepare` resets these whenever `current_mark` is at least half of the maximal integer of type Int (rounded down).
//    // We typically use Int = int32_t (or greater). So we can handle 2^30-1 strands in one call to `ReroutePasses`. That should really be enough for all feasible applications.
//    
//    ++current_mark;
//    a_ptr = 0;
//    
//    const Mark_T mark_0 = current_mark;
//
//    strand_length = 0;
//    change_counter = 0;
//    
//    // The main loop.
//    while( a_ptr < m )
//    {
//        // Search for next arc that is active and has not yet been handled.
//        while(
//            ( a_ptr < m )
//            &&
//            ( (A_mark(a_ptr) >= mark_0 ) || (!pd.ArcActiveQ(a_ptr)) )
//        )
//        {
//            ++a_ptr;
//        }
//        
//        if( a_ptr >= m ) { break; }
//        
//        // Find the beginning of first strand.
//        Int a_begin = WalkBackToStrandStart(a_ptr);
//        
//        // If we arrive at this point again, we break the do loop below.
//        const Int a_0 = a_begin;
//        
//        // We should make sure that a_0 is not a loop.
//        if( Reidemeister_I<true>(a_0) )
//        {
//            RepairArcLeftArcs();
//            ++change_counter;
//            continue;
//        }
//        
//        // Might be changed by RerouteToShortestPath_impl.
//        // Thus, we need indeed a reference, not a copy here.
//        mref<ArcState_T> a_0_state = A_state[a_0];
//        
//        // Current arc.
//        Int a = a_0;
//        strand_length = 0;
//        MarkCrossing( A_cross(a,Tail) );
//        
//        // Traverse forward through all arcs in the link component, until we return where we started.
//        do
//        {
//            ++strand_length;
//            
//            // Safe guard against integer overflow.
//            if( current_mark == std::numeric_limits<Mark_T>::max() )
//            {
//#ifdef PD_TIMINGQ
//                const Time stop_time = Clock::now();
//                Time_ReroutePasses += Tools::Duration(start_time,stop_time);
//#endif
//                // Return a positive number to encourage that this function is called again upstream.
//                return change_counter+1;
//            }
//            
//            const Int  c_1    = A_cross(a,Head);
//            const bool side_1 = ArcSide(a,Head);
//            AssertCrossing<1>(c_1);
//            
//            Int a_next = C_arcs(c_1,Out,!side_1);
//            AssertArc<1>(a_next);
//            
//            Int c_next = A_cross(a_next,Head);
//            AssertCrossing<1>(c_next);
//            
//            // TODO: Not sure whether this adds anything good. RemoveLoop is also able to remove Reidemeister I loops. It just does it in a slightly different way.
//            if( c_next == c_1 )
//            {
//                // overQ == true
//                //
//                //                  +-----+
//                //      |     |     |     | a_next
//                //      |     |  a  |     |
//                // ---------------->|-----+
//                //      |     |     |c_1
//                //      |     |     |
//
//                // overQ == true
//                //
//                //                  +-----+
//                //      |     |     |     | a_next
//                //      |     |  a  |     |
//                // ---------------->------+
//                //      |     |     |c_1
//                //      |     |     |
//                
//                // In any case, the loop would prevent the strand from going further.
//                
//                // TODO: We could pass `a` as `a_prev` to `Reidemeister_I`.
//                (void)Reidemeister_I<false>(a_next);
//                
//                PD_ASSERT(a_next != a);
//                
//                ++change_counter;
//                --strand_length;
//                RepairArcLeftArcs();
//                
//                // TODO: I think it would be safe to continue without checking that `a_0` is still active; we could simply complete the current strand.
//                
//                if( a_0_state.ActiveQ() )
//                {
//                    continue;
//                }
//                else
//                {
//                    break;
//                }
//            }
//            
//            // Arc gets current current_mark.
//            A_mark(a) = current_mark;
//            
//            
//            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to reset and create a new strand.
//            // This finds out whether we have to reset.
//            strand_completeQ = ((side_1 == pd.CrossingRightHandedQ(c_1)) == overQ);
//            
//            // TODO: This might require already that there are no possible Reidemeister II moves along the strand. (?)
//            
//            // Check for loops, i.e., a over/understrand that starts and ends at the same crossing. This is akin to a "big Reidemeister I" move on top (or under) the diagram.
//            
//            if( C_mark(c_1) == current_mark )
//            {
//                // Vertex c has been visited before.
//
//                
//                // We catch
//                // TODO: Test this.
//                if constexpr ( mult_compQ )
//                {
//                    // Catch the "Big Hopf Link" here, but not the "Big Unlink".
//                    
//                    if( !strand_completeQ  || (A_mark(a_next) != current_mark) )
//                    {
//                        wprint(MethodName("ReroutePasses") + ": We might have overlooked a \"Big Hopf Unlink\" from arc " + ArcString(a_ptr) + " to " + ArcString(a_ptr) + ".");
//                        
//                        pd.PrintInfo();
//                        
//                        /* "Big Hopf Link"
//                         *
//                         *            #       #
//                         *            #       #
//                         *            |       |
//                         *            |       |
//                         *         a  | a_ptr |
//                         *      +---->|------------>+
//                         *      ^  c_1|       |     |
//                         *      |     |       |     |
//                         *      |     |       |     |
//                         *      |     |       |  +--|--##
//                         *  ##--|--+  +-------+  |  |
//                         *      |  |             |  v
//                         *      +-------------------+
//                         *         |             |
//                         *         #             #
//                         */
//                        
//                        RemoveLoop(a,c_1);
//                        RepairArcLeftArcs();
//                        break;
//                        
//                        
//                        /* Simplified "Big Hopf Loop"
//                         *
//                         *            #       #
//                         *            #       #
//                         *            |       |
//                         *            |       |
//                         *         a  | a_ptr |
//                         *      +-----|---+   |
//                         *      |  c_1|   |   |
//                         *      |     |c_2|   |
//                         *      +---------+   |
//                         *            |       |  +-----##
//                         *  ##-----+  +-------+  |
//                         *         |             |
//                         *         |             |
//                         *         #             #
//                         */
//                        
//                        // The Dijkstra algorithm should be able to simplify this. We could of course provide a shorter, slightly more efficient implementation. But Dijkstra should terminate quite quickly, so we should be happy about getting rid of this mental load. We we just do not break here and let the algorithm continue.
//                        
//                        
//                    }
//                    else
//                    {
//                        wprint(MethodName("ReroutePasses") + ": We might have overlooked a \"Big Hopf Unlink\" from arc " + ArcString(a_ptr) + " to " + ArcString(a_ptr) + ".");
//                        
//                        pd.PrintInfo();
//                        
//                        // TODO: CAUTION: Also this is possible:
//                        
//                        /* "Big Over Unlink"
//                         *
//                         *
//                         *              +-------+
//                         *              |       |
//                         *       ##-----|-------|----##
//                         *              |       |
//                         *          a   | a_ptr |
//                         *      +-------|-------+
//                         *      |    c_1|
//                         *      |       |
//                         *  ##--|-------|------##
//                         *      |       |
//                         *      +-------+
//                         *
//                         */
//
//                        // TODO: I am not entirely sure whether Dijkstra will resolve this correctly.
//                    }
//            
//                }
//                else // single component link
//                {
//                    if( !strand_completeQ )
//                    {
//                        eprint(MethodName("SimplyStrands")+": !strand_completeQ while mult_compQ == false. This should be possible.");
//                    }
//                       
//                    // The only case I can imagine where `RemoveLoop` won't work is this unknot:
//                    
//                    /*            +-------+
//                     *            |       |
//                     *            |       |
//                     *         a  | a_ptr |
//                     *    +-------|-------+
//                     *    |    c_1|
//                     *    |       |
//                     *    |       |
//                     *    +-------+
//                     */
//                    
//                    // However, the earlier call(s) to Reidemeister_I should rule this case out. So we have the case of a "Big Reidemeister I" here, and that is what `RemoveLoop` is made for.
//                    
//                    RemoveLoop(a,c_1);
//                    RepairArcLeftArcs();
//                    break;
//                }
//            }
//
//            // Checking for Reidemeister II moves like this:
//            
//            // overQ == true
//            //
//            //                  +--------+
//            //      |     |     |        |
//            //      |     |  a  | a_next |
//            // ---------------->|------->|----
//            //      |     |     |c_1     |
//            //      |     |     |        |
//            
//            // TODO: It might even be worth to do this before RemoveLoop is called.
//
//            if constexpr ( R_II_Q )
//            {
//                if( (!strand_completeQ) && (a != a_begin) )
//                {
//                    // Caution: This might change `a`.
//                    const bool changedQ = Reidemeister_II_Backward(a,c_1,side_1,a_next);
//                    
//                    if( changedQ )
//                    {
//                        RepairArcLeftArcs();
//                        
//                        if( a_0_state.ActiveQ() )
//                        {
//                            // Reidemeister_II_Backward reset `a` to the previous arc.
//                            continue;
//                        }
//                        else
//                        {
//                            break;
//                        }
//                    }
//                }
//            }
//            
//            if( strand_completeQ )
//            {
//                bool changedQ = false;
//
//                if( (strand_length > Int(1)) && (max_dist > Int(0)) )
//                {
//                    changedQ = RerouteToShortestPath_impl(
//                        a_begin,a,
//                        Min(strand_length-Int(1),max_dist),
//                        current_mark
//                    );
//                    
//                    if( changedQ ) { RepairArcLeftArcs(); }
//                }
//                
//                // TODO: Check this.
//
//                // RerouteToShortestPath might deactivate `a_0`, so that we could never return to it. Hence, we rather break the while loop here.
//                if( changedQ || (!a_0_state.ActiveQ()) ) { break; }
//                
//                // Create a new strand.
//                strand_length = 0;
//                a_begin = a_next;
//
//                ++current_mark;
//            }
//            
//            // Head of arc gets new mark.
//            MarkCrossing(c_1);
//
//            AssertArc<1>(a_next);
//            AssertArc<1>(a_0);
//            AssertArc<1>(a);
//            
//            a = a_next;
//            
//        }
//        while( a != a_0 );
//        
//        // TODO: If the link has multiple components, it can happen that we cycled around a full unlink lying on top (or below) the remaining diagram.
//        
////        if( a == a_begin )
////        {
////            wprint(ClassName()+"::ReroutePasses: Split unlink detected. We have yet to remove it." );
////        }
//        
//        ++current_mark;
//        strand_length = 0;
//        
//        ++a_ptr;
//    }
//    
//#ifdef PD_TIMINGQ
//    const Time stop_time = Clock::now();
//    
//    Time_ReroutePasses += Tools::Duration(start_time,stop_time);
//#endif
//    
//    Cleanup();
//    
//    return change_counter;
//    
//} // ReroutePasses

void SetStrandMode( const bool overQ_ )
{
    overQ = overQ_;
}


private:

void Prepare()
{
    PD_TIMER(timer,MethodName("Prepare"));
    
    PD_ASSERT(pd.CheckAll());
    PD_ASSERT(pd.CheckLeftDarc());
    PD_ASSERT(pd.CheckRightDarc());
    
    if( current_mark >= std::numeric_limits<Mark_T>::max()/2 )
    {
        C_mark.Fill(Mark_T(0));
        A_mark.Fill(Mark_T(0));
        
        D_mark.Fill(Mark_T(0));
        D_from.Fill(Int(0));

        current_mark = Mark_T(0);
    }
    
    // D_source will be read or written to only if D_mark[a] == mark.
    
    // Refresh dA_left
    dA_left = pd.ArcLeftDarc().data();
    
    PD_ASSERT(CheckDarcLeftDarc());
}

void Cleanup()
{
    PD_TIMER(timer,MethodName("Cleanup"));
    
    dA_left = nullptr;
    
    pd.ClearCache("ArcLeftArc");
}
