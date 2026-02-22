friend std::string ToString( cref<Pass_T> pass )
{
    return std::string("{ ")
        +   "first = " + ToString(pass.first)
        + ", last = " + ToString(pass.last)
        + ", next = " + ToString(pass.next)
        + ", arc_count = " + ToString(pass.arc_count)
        + ", mark = " + ToString(pass.mark)
        + ", overQ = " + ToString(pass.overQ)
        + ", activeQ = " + ToString(pass.activeQ)
        + " }";
}

//Int WalkBackToPassStart( const Int a_0, const Int overQ_ ) const
//{
//    Int a = a_0;
//    AssertArc<1>(a);
//    
//    print("WalkBackToPassStart");
//    TOOLS_DUMP(overQ_);
//    TOOLS_DUMP(a_0);
//
//    while( ArcOverQ(a,Tail) == overQ_  )
//    {
//        TOOLS_DUMP(a);
//        a = NextArc(a,Tail);
//        AssertArc<1>(a);
//        
//        // If the link has multiple components, it can also happen that the loop pass is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well. So we need a guard against cycling around this unlink forever!
//        if( a == a_0 ) { break; }
//    }
//    
//    logvalprint("Strand start",a);
//    
//    return a;
//}

template<bool find_maximal_passQ = false>
void InitializePass( mref<Pass_T> pass, const Int initial_arc, const bool desired_overQ, const Int mark )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("InitializePass"); };
    
    TOOLS_PTIMER(timer,tag());
    
    Int a = initial_arc;
    AssertArc<1>(a);
    
    if constexpr ( find_maximal_passQ )
    {
        while( ArcOverQ(a,Tail) == desired_overQ  )
        {
            a = NextArc(a,Tail);
            AssertArc<1>(a);
            
            // If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well. So we need a guard against cycling around this unlink forever!
            if( a == initial_arc ) { break; }
        }
        
        // We do not handle the case where (a == initial_arc) and (ArcOverQ(a,Tail) != desired_overQ) because we cannot distinguish the Big Figure-Eights Shaped Unlink and the Big Hopf Link.
    }

    MarkCrossing(pd->A_cross(a,Tail),mark);
    
    pass.first     = a;
    pass.last      = a;
    pass.arc_count = 0;
    pass.mark      = mark;
    pass.overQ     = desired_overQ;
    pass.activeQ   = true;
}


template<bool find_maximal_passQ = true, bool R_II_blockingQ = true, bool R_II_forwardQ = true>
void FindPass( mref<Pass_T> pass, const Int initial_arc, const bool desired_overQ, const Int mark )
{
    [[maybe_unused]] auto tag= [](){ return MethodName("FindPass"); };

    TOOLS_PTIMER(timer,tag());
    
    InitializePass<find_maximal_passQ>( pass, initial_arc, desired_overQ, mark );

//    MarkCrossing(pd->A_cross(pass.last,Tail),mark);

    while( true )
    {
        PD_ASSERT(pd->CheckAll());
        PD_ASSERT(CheckPass(pass,false));
        
        const Int c_0 = pd->A_cross(pass.last,Tail);
        const Int c_1 = pd->A_cross(pass.last,Head);
        AssertCrossing<1>(c_0);
        AssertCrossing<1>(c_1);
        PD_ASSERT(CrossingMark(c_0) == pass.mark);
        
        // Check for forward Reidemeister I move.
        const bool side_1 = (pd->C_arcs(c_1,In,Right) == pass.last);
        pass.next = pd->C_arcs(c_1,Out,!side_1);
        AssertArc<1>(pass.next);
        
        const Int c_2 = pd->A_cross(pass.next,Head);
        AssertCrossing<1>(c_2);
        
        PD_ASSERT(
            (pass.arc_count <= Int(2)) || (pd->A_cross(pass.first,Head) != pd->A_cross(pass.last,Tail))
        );
        
        if( c_2 == c_1 )
        {
            /*               +--------+
             *      |        |        |
             *      |  last  v  next  |
             * ------------->X------->+
             *      |c_0     |c_1 == c_2
             *      |        |
             */
            
            // In any case, the loop would prevent the pass from going further.
            (void)Reidemeister_I<false>(pass.next);
            ++change_counter;
            
            // We have to guarantee that arc `pass.last` is still alive and part of the strand.
            if( !ArcActiveQ(pass.last) )
            {
                PD_PRINT("Aborting because (ArcActiveQ(a) == false) after Reidemeister_I.");
                pass.activeQ = false;
                return;
            }
            
            // It can happen that pass.next == pass.first or NextArc(pass.next) == pass.first.
            // In that case we would have detected a loop and the pass is gone.
            if( !ArcActiveQ(pass.first) )
            {
                PD_PRINT("Aborting because (ArcActiveQ(pass.first) == false) after Reidemeister_I.");
                pass.activeQ = false;
                return;
            }
            
            continue; // Analyze `pass.last` and the new `pass.next` once more.
        }
        
        // If we land here, then `pass.next` is not a loop arc.
        // The arc `pass.last` might still be a loop arc, though. But then CrossingMarkedQ(c_1) == true, and the check for big loops below will detect it and remove it correctly.
        
        // Make arc `pass.last` an official member of the pass.
        ++pass.arc_count;
        MarkArc(pass.last,pass.mark);
        
        // TODO: Expensive!
        PD_ASSERT(CheckPass(pass,true));

        if( CrossingMark(c_1) == pass.mark )
        {
            PD_PRINT("Visiting marked crossing c_1 = " + CrossingString(c_1) + ".");
            // Vertex c_1 has been visited before.
            // This catches also the case when `pass.last` is a loop arc.
            
            
            if( FindPass_HandleLoop<find_maximal_passQ>(pass, c_1) )
            {
                continue;
            }
            else
            {
                pass.activeQ = false;
                return;
            }
        }

        PD_ASSERT(c_0 != c_1); // Because of (CrossingMark(c_1) != pass.mark).
        
        // Check whether the pass is finished.
        const bool pass_endQ = ((side_1 == CrossingRightHandedQ(c_1)) == pass.overQ);
        PD_ASSERT( pass_endQ == (ArcOverQ(pass.last,Head) != pass.overQ) );
        PD_ASSERT( pass_endQ == (ArcOverQ(pass.next,Tail) != pass.overQ) );
        
        // We do not like Reidemeister II patterns along our strand because that would make rerouting difficult.
        if( (!pass_endQ) && (pass.last != pass.first) )
        {
            PD_ASSERT(pass.arc_count >= Int(2));
            
            // overQ == true
            //
            //               +--------+
            //      |        |        |
            //      |        |  last  | next
            // -----X---------------->-------->
            //      |        |c_0     |c_1
            //      |        |        |
            
            // If `pass.last` is deactivated by this, then Reidemeister_II_Backward subtracts 2 from pass.arc_count. The only thing that can go wrong here is that `pass.first` is deactivated by FindPass_Reidemeister_II_Backward (see the function body for details).
            // So, we need the following check here.
            
            if( FindPass_Reidemeister_II_Backward(pass,c_0,c_1,side_1) )
            {
                // Caution: This might change `pass`!
                
                if( !ArcActiveQ(pass.first) )
                {
                    nprint(tag() + ":Aborting because (ArcActiveQ(pass.first) == false) after FindPass_Reidemeister_II_Backward.");
                    pass.activeQ = false;
                    return;
                }
                
                continue; // Analyze `pass.last` and the new `pass.next` once more
            }
        }
        
        if constexpr ( R_II_blockingQ || R_II_forwardQ )
        {
            if ( R_II_forwardQ || pass_endQ )
            {
                if( CrossingMark(c_2) == pass.mark )
                {
                    if( pass.arc_count > Int(2) )
                    {
                        PD_NOTE("CrossingMarkedQ(c_2). We do not attempt Reidemeister_II_Forward. However, this could be a great rerouting opportunity. (strand_arc_count = " + ToString(pass.arc_count) + ")");
                        
                        PD_VALPRINT("pass", ToString(pass));
                    }
                }
                else
                {
                    const Int b = pd->NextArc(pass.next,Head,c_2);
                    
                    PD_ASSERT(CrossingMark(c_2) != pass.mark);
                    PD_ASSERT(pass.last != b);  // Because ArcMarkedQ(a) and CrossingMark(c_2) != pass.mark.
                    
                    if( FindPass_Reidemeister_II_Forward(pass,c_1,c_2,b) )
                    {
                        AssertArc<1>(pass.last);
                        // I think this cannot happen by the preconditions.
                        if( !pd->ArcActiveQ(pass.first) )
                        {
                            PD_NOTE("Aborting because FindPass_Reidemeister_II_Forward deactivated s_begin. This would have been a rerouting opportunity.");
                            // We deleted our strand start.
                            // TODO: Can we recover from that?
                            pass.activeQ = false;
                            return;
                        }
                        else
                        {
                            PD_VALPRINT("pass",ToString(pass));
                            PD_ASSERT(CheckPass(pass,false));
                            continue; // Analyze the modified arc `pass.last` again.
                        }
                    }
                }
            }
        }
        
        AssertArc<1>(pass.last);
        AssertArc<1>(pass.next);
        
        // We are done with pass.last. Mark its head.
        MarkCrossing(c_1,pass.mark);
        
        if( pass_endQ ) { return; }

        PD_PRINT("Moving current arc pass.last to next arc pass.next");
        pass.last = pass.next;

    } // while(true)
}


template<bool find_maximal_passQ = false>
bool FindPass_HandleLoop( mref<Pass_T> pass, const Int c_1  )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindPass_HandleLoop"); };
    
    TOOLS_PTIMER(timer,tag());
    
    PD_ASSERT(pd->CheckAll());
    
//    Int & a = pass.last;
    const bool side = (pd->C_arcs(c_1,In,Right) == pass.last);
    const Int a_out = pd->C_arcs(c_1,Out, side);
    
//    AssertArc<1>(a_in);
    AssertArc<1>(a_out);
    
    if( pass.next != pass.first )
    {
        PD_PRINT("\tpass.next != pass.first");
        
        /* Loop can be removed and pass can could be restarted or continued.
         *
         *          |       |
         *      +<--------------+
         *      |   |       |   ^
         *      |               |
         *    --|--           --|--
         *      |               ^
         *    --|--             | a_out
         *      |               |
         *      v       | last  |   next
         *      +------>------->X------>
         *              |       ^ c_1
         *             b|       |
         *              |       | a_in
         */
        
        Int deleted_arc_count = CollapseArcRange( a_out, pass.last, pass.arc_count, pass.mark );
        DeactivateArc(a_out);
        ++deleted_arc_count;
        change_counter += static_cast<Size_T>(deleted_arc_count);
        
        AssertArc<0>(a_out);
        AssertArc<0>(pass.last);
        AssertArc<1>(pass.next);

        // It can happen, that the arc at the south port of c_1 bends back to the deleted arc and thus is deactivated by CollapseArcRange. This is why we load it only now,
        const Int a_in  = pd->C_arcs(c_1,In ,!side);
        AssertArc<1>(a_in);

        if( a_in == pass.next )
        {
            PD_PRINT("\t\ta_in == pass.next");
            DeactivateArc(a_in);
            CreateUnlinkFromArc(a_in);
            DeactivateCrossing(c_1);
            pass.activeQ = false;
            return false;
        }
        else
        {
            PD_PRINT("\t\ta_in != pass.next");
            Reconnect<Head>(a_in,pass.next);
            DeactivateCrossing(c_1);
            if( a_out == pass.first )
            {
                PD_PRINT("\t\t\ta_out == pass.first");
                InitializePass<find_maximal_passQ>(pass, a_in, pass.overQ, pass.mark);
            }
            else
            {
                PD_PRINT("\t\t\ta_out != pass.first");
                pass.last  = a_in;
                PD_ASSERT(pass.arc_count >= deleted_arc_count);
                pass.arc_count -= (deleted_arc_count + Int(1));
            }
            return true;
        }
    }
    else
    {
        PD_PRINT("\tpass.next == pass.first");
        
        /* If the pass were a Big Figure-Eight Shaped Unlink, then we would have detected and removed another loop, first. So, this cannot happen.
         *
         * Big Figure-Eight Shaped Unlink:
         *
         *          |   |
         *      +<----------+
         *      |   |   |   |
         *      |         --|--
         *    --|--         ^
         *      |   | last  |   next
         *      +---|------>|---------->+
         *          |       |           |
         *                --|--       --|--
         *                  |   |   |   |
         *                  +-----------+
         *                      |   |
         */
        
        /* Remaining possibilities:
         *
         *          Big Unlink        or        Big Hopf link.
         *
         *          |   |   |                     |   |   |
         *      +<--------------+             +<--------------+
         *      |   |   |   |   ^             |   |   |   |   ^
         *      |               |             |               |
         *    --|--           --|--         --|--           --|--
         *      |               |             |               |
         *    --|--           --|--         --|--           --|--
         *      |       |       ^             |       |       ^
         *      v  last | next  |             v  last | next  |
         *      +------>------->+             +------>|------>+
         *              |c_1                          |c_1
         *              |                             |
         */
        
        // Could be optimized, but is called very seldomly.
        const bool pass_endQ = (ArcOverQ(pass.last,Head) != pass.overQ);
        
        Int deleted_arc_count = CollapseArcRange( pass.next, pass.last, pass.arc_count, pass.mark );
        DeactivateArc(pass.next);
        ++deleted_arc_count;
        change_counter += static_cast<Size_T>(deleted_arc_count);
        
        // It can happen, that the arc at the south port of c_1 bends back to the deleted arc and thus is deactivated by CollapseArcRange. This is why we load it only now.
        const Int a_in  = pd->C_arcs(c_1,In ,!side);
        AssertArc<1>(a_in);
        
        if( a_in == a_out )
        {
            PD_PRINT("\t\ta_in == a_out");
            
            // Only possible if not Hopf link
            DeactivateArc(a_out);
            CreateUnlinkFromArc(a_out);
        }
        else
        {
            PD_PRINT("\t\ta_in != a_out");
            Reconnect<Head>(a_in,a_out);
            if( pass_endQ ) { CreateUnlinkFromArc(a_out); }
        }
        DeactivateCrossing(c_1);
        
        // If a_in is still alive, we could start a new pass from there... but that could mix up a lot in memory access. We better stop here.

        pass.activeQ = false;
        return false;
    }
}

template<bool trivial_strand_warningQ = true>
bool CheckPass( cref<Pass_T> pass, const bool pass_closedQ )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("CheckPass"); };
    
    logprint(tag() + " " + ToString(pass));
    

    if( pd == nullptr )
    {
        eprint(tag() + ": No diagram loaded.");
        return false;
    }
    
    if( !pass.activeQ ) { return true; }
    
    auto arc_passesQ = [this,&pass,&tag]( const Int a, const bool arc_closedQ, const bool first_arcQ, const bool last_arcQ )
    {
        if( !ArcActiveQ(a) )
        {
            eprint(tag() + ": " + ArcString(a) + " is inactive.");
            return false;
        }
        
        const Int c_0 = pd->A_cross(a,Tail);
        const Int c_1 = pd->A_cross(a,Head);
        
        if( CrossingMark(c_0) != pass.mark )
        {
            eprint(tag() + ": Tail crossing of " + ArcString(a) + " has mark " + ToString(CrossingMark(c_0)) + ", but its mark should be " + ToString(pass.mark) + ".");
            return false;
        }
        
        if( !first_arcQ && (pd->ArcOverQ(a,Tail) != pass.overQ) )
        {
            eprint(tag() + ": Current pass is supposed to be an " + OverQString(pass.overQ) + "pass, but " + ArcString(a) + " does not go " + OverQString(pass.overQ) + " its " + (Tail ? "head" : "tail" )+ ".");
            return false;
        }
        
        
        if ( arc_closedQ )
        {
            if( CrossingMark(c_1) != pass.mark )
            {
                eprint(tag() + ": Head crossing of " + ArcString(a) + " has mark " + ToString(CrossingMark(c_0)) + ", but its mark should be " + ToString(pass.mark) + ".");
                return false;
            }
            
            if( ArcMark(a) != pass.mark )
            {
                eprint(tag() + ": " + ArcString(a) + " has mark " + ToString(ArcMark(a)) + ", but its mark should be " + ToString(pass.mark) + ".");
                return false;
            }
        }
        
        if( !last_arcQ && (pd->ArcOverQ(a,Head) != pass.overQ) )
        {
            eprint(tag() + ": Current pass is supposed to be an " + OverQString(pass.overQ) + "pass, but " + ArcString(a) + " does not go " + OverQString(pass.overQ) + " its " + (Head ? "head" : "tail" )+ ".");
            return false;
        }
        
        return true;
    };
    
    const Int pd_max_arc_count = pd->MaxArcCount();
    
    bool passedQ = true;
    
    Int arc_counter = 0;
    Int a           = pass.first;
    
    while ( (a != pass.last) && (arc_counter < pd_max_arc_count ) )
    {
        ++arc_counter;
        
        const bool arc_closedQ = pass_closedQ || (arc_counter < pass.arc_count);
        
        passedQ = passedQ && arc_passesQ( a, arc_closedQ, arc_counter == Int(1), arc_counter == pass.arc_count );
        
        // We use the safe implementation of NextArc here.
        a = pd->NextArc(a,Head);
    }
    ++arc_counter;
    
    if( a != pass.last )
    {
        eprint(tag() + ": After traversing pass for MaxArcCount() =  " + ToString(pd_max_arc_count) + " steps the end is still not reached.");
        
        passedQ = false;
    }
    
    if( arc_counter != pass.arc_count + Int(!pass_closedQ) )
    {
        eprint(tag() + ": Counted " + ToString(arc_counter)+ " arcs in " + (pass_closedQ ? "closed" : "open") + " pass, but pass.arc_count = " + ToString(pass.arc_count) + ".");
        passedQ = false;
    }

    if( passedQ )
    {
        TOOLS_LOGDUMP(arc_counter);
        TOOLS_LOGDUMP(pass.arc_count);
        PD_VALPRINT("pass",ArcRangeString(pass.first,pass.last));
        
        logprint(tag() + " passed.");
        
        return true;
    }
    else
    {
        TOOLS_LOGDUMP(arc_counter);
        TOOLS_LOGDUMP(pass.arc_count);
        PD_VALPRINT("pass",ArcRangeString(pass.first,pass.last));
        
        (tag() + " failed.");
        
        return false;
    }
}



// This checks for a backward Reidemeister II move (and it exploits already that both vertical strands go under (if pass is an overpass) or go over (if pass is an underpass).
// It may set pass.last to another pd->NextArc(pass.last,Tail) if pass.last is to be deleted.
// In this case, the pass is shortened appropriately.
bool FindPass_Reidemeister_II_Backward(
    mref<Pass_T> pass, const Int c_0, const Int c_1, bool side_1
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindPass_Reidemeister_II_Backward"); };
    
    PD_DPRINT( tag() + " checks " + ArcString(pass.last) );
    
    // Call preconditions.
    PD_ASSERT(ArcMarkedQ(pass.last));
    PD_ASSERT(CrossingMark(c_0) == pass.mark);
    PD_ASSERT(CrossingMark(c_1) != pass.mark);
    
    const bool side_0 = (pd->C_arcs(c_0,Out,Right) == pass.last);
//    const bool side_1 = (pd->C_arcs(c_1,In ,Right) == pass.last);
    
    //     pass.last == pd->C_arcs(c_0,Out, side_0)
    //        a_prev == pd->C_arcs(c_0,In ,!side_0)
    const Int a_0_out = pd->C_arcs(c_0,Out,!side_0);
    const Int a_0_in  = pd->C_arcs(c_0,In , side_0);
    
    //     pass.last == pd->C_arcs(c_1,In , side_1)
    //     pass.next == pd->C_arcs(c_1,Out,!side_1)
    const Int a_1_out = pd->C_arcs(c_1,Out, side_1);
    const Int a_1_in  = pd->C_arcs(c_1,In ,!side_1);
    
    
    // Consequences:
    PD_ASSERT(ArcMark(pass.next) != pass.mark); // Because of CrossingMark(c_1) != pass.mark.
    PD_ASSERT(ArcMark(a_1_out)   != pass.mark); // Because of CrossingMark(c_1) != pass.mark.
    PD_ASSERT(ArcMark(a_1_in )   != pass.mark); // Because of CrossingMark(c_1) != pass.mark.
    
    PD_ASSERT(pass.next != pass.first);
    PD_ASSERT(a_1_out   != pass.first);
    PD_ASSERT(a_1_out   != pass.first);
    

    if( ArcMark(a_0_out) == pass.mark ) [[unlikely]]
    {
        eprint(tag() + ": ArcMark(a_0_out) == pass.mark; we thought this cannot happen.");
    }
    
    /* Assuming overQ == true.
     *
     *               a_0_out           a_1_in
     *             |                 |
     *             |                 |
     *    a_prev   |c_0   last       |   next
     *  ---------->----------------->----------->
     *             |                 |c_1
     *             |                 |
     *             | a_0_in          | a_1_out
     */
        
    if( a_0_out == a_1_in )
    {
        PD_DPRINT(tag() + " detected move at ingoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);

        if( a_0_out == a_prev ) [[unlikely]]
        {
            eprint(tag() + ": a_0_out == a_prev.");
        }
        
        // The obvious things
        PD_ASSERT(a_0_in  != pass.next);
        PD_ASSERT(a_1_in  != pass.next);
        PD_ASSERT(a_0_out != a_prev   );
        PD_ASSERT(a_1_out != a_prev   );

        PD_ASSERT(ArcMark(a_prev) == pass.mark);  // Because of CrossingMark(c_0) == pass.mark.
        PD_ASSERT(a_prev != pass.next);
        Reconnect<Head>(a_prev,pass.next);
        
        // a_1_out is unmarked, so it is safe to deactivate it.
        if( a_0_in == a_1_out ) [[unlikely]]
        {
            
            /*              a_0_out == a_1_in
             *             +---------------->+
             *             ^                 |
             *             |                 |
             *  a_prev     |c_0  last        v    next
             *  ---------->----------------->-------------->
             *             ^                 |c_1
             *             |                 |
             *             |a_0_in == a_1_out|
             *             +-----------------+
             */
            
            DeactivateArc(a_1_out);
            CreateUnlinkFromArc(a_1_out);
        }
        else
        {
            
            /*              a_0_out == a_1_in
             *             +---------------->+
             *             ^                 |
             *             |                 |
             *  a_prev     |c_0   last       v     next
             *  ---------->----------------->-------------->
             *             ^                 |c_1
             *             |                 |
             *             | a_0_in          v a_1_out
             */
            
            Reconnect<Head>(a_0_in,a_1_out);
        }
        
        DeactivateArc(pass.last);
        
        // a_1_in is unmarked, so it is safe to deactivate it.
        DeactivateArc(a_1_in);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CountReidemeister_II();
        
        AssertArc<1>(a_prev);
        AssertArc<0>(pass.last);
        AssertArc<0>(pass.next);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        ++change_counter;
        
        // Correct pass.
        PD_ASSERT(pass.arc_count >= Int(2) );
        pass.arc_count -= Int(2);
        pass.last = a_prev;
        return true;
    }
    
    if( a_0_in == a_1_out )
    {
        PD_DPRINT(tag() + " detected move at outgoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        
        if( a_0_out == a_prev ) [[unlikely]]
        {
            eprint(tag() + ": a_0_out == a_prev.");
        }
        
        PD_ASSERT(ArcMark(a_prev) == pass.mark);  // Because of  CrossingMark(c_0) == mark.
        
//        // A nasty case that is easy to overlook.
        if( a_1_in == pass.next )
        {
            /*             & a_0_out         +<------+ a_1_in == a_next
             *             |                 |       |
             *             |                 |       |
             *  a_prev     |c_0   last       v       |
             *  ---------->------------------------->+
             *             ^                 |c_1
             *             |                 |
             *             |                 v
             *             +<----------------+
             *              a_0_in == a_1_out
             */
            

            PD_ASSERT(a_0_out == a_prev);
            Reconnect<Head>(a_prev,a_0_out);
            PD_ASSERT(ArcMark(pass.next) != pass.mark);
            DeactivateArc(pass.next); // a_next is unmarked, so this should be safe.
            CountReidemeister_I();
            CountReidemeister_I();
        }
        else
        {
            /*             ^ a_0_out         | a_1_in
             *             |                 |
             *             |                 |
             *    a_prev   |c_0   last       v    next
             *  ---------->----------------->------------>
             *             ^                 |c_1
             *             |                 |
             *             |                 v
             *             +<----------------+
             *              a_0_in == a_1_out
             */

            
            PD_ASSERT(a_0_out != a_1_in); // Otherwise, we would have been stuck in the other case.
            // a_prev is marked, a_next is not; so this is safe.
            Reconnect<Head>(a_prev,pass.next);
            // a_1_in is unmarked, so it is safe to deactivate it.
            Reconnect<Tail>(a_0_out,a_1_in);
            CountReidemeister_II();
        }
        
        DeactivateArc(pass.last);
        PD_ASSERT(ArcMark(a_1_out) != pass.mark);
        DeactivateArc(a_1_out); // a_1_out is unmarked, so this is safe.
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        AssertArc<1>(a_prev);
        AssertArc<0>(pass.last);
        AssertArc<0>(pass.next);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        ++change_counter;
        
        // Correct pass.
        PD_ASSERT(pass.arc_count >= Int(2) );
        pass.arc_count -= Int(2);
        pass.last = a_prev;
        return true;
    }

    PD_DPRINT( tag() + " found no changes.");
    
    return false;
}


bool FindPass_Reidemeister_II_Forward(
    mref<Pass_T> pass, const Int c_1, const Int c_2, const Int b
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindPass_Reidemeister_II_Forward"); };
    
    PD_PRINT( tag() + " checks " + ArcString(pass.next) );
 
    AssertArc<1>(pass.last);
    AssertArc<1>(pass.next);
    AssertArc<1>(b);
    AssertCrossing<1>(c_1);
    AssertCrossing<1>(c_2);
    
    // Call preconditions.
    PD_ASSERT(ArcMarkedQ(pass.last));
    PD_ASSERT(!CrossingMarkedQ(c_1));
    PD_ASSERT(!CrossingMarkedQ(c_2));
    
    // Consequences:
    PD_ASSERT(!ArcMarkedQ(pass.next));      // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(!ArcMarkedQ(b)); // Because of !CrossingMarkedQ(c_2).
    PD_ASSERT(pass.last != b);    // Because of !CrossingMarkedQ(c_2).
    
    // Necessary for a Reidemeister II move: the crossings have opposite handedness
    if( !OppositeHandednessQ(pd->C_state[c_1],pd->C_state[c_2]) )
    {
        PD_PRINT(tag() + " found no changes.");
        return false;
    }
    
//    // Both vertical strands need to go over or under at the same time.
//    if( pd->ArcOverQ(a,Tail) != pd->ArcOverQ(a,Head) )
//    {
//        PD_PRINT(tag() + " found no changes.");
//        return false;
//    }

    const bool side_1 = (pd->C_arcs(c_1,Out,Right) == pass.next);
    const bool side_2 = (pd->C_arcs(c_2,In ,Right) == pass.next);
    
    // Because of OppositeHandednessQ(pd->C_state[c_1],pd->C_state[c_2]) == true, is equivalent to pd->ArcOverQ(a,Tail) != pd->ArcOverQ(a,Head).
    if( side_1 != side_2 )
    {
        PD_PRINT(tag() + " found no changes.");
        return false;
    }
    
    PD_ASSERT(pass.next == pd->C_arcs(c_1,Out, side_1));
    PD_ASSERT(pass.last == pd->C_arcs(c_1,In ,!side_1));
    const Int a_1_out = pd->C_arcs(c_1,Out,!side_1);
    const Int a_1_in  = pd->C_arcs(c_1,In , side_1);
    
    PD_ASSERT(pass.next == pd->C_arcs(c_2,In , side_2));
    PD_ASSERT(b         == pd->C_arcs(c_2,Out,!side_2));
    const Int a_2_out = pd->C_arcs(c_2,Out, side_2);
    const Int a_2_in  = pd->C_arcs(c_2,In ,!side_2);
    
    AssertArc<1>(a_1_out);
    AssertArc<1>(a_1_in );
    AssertArc<1>(a_2_out);
    AssertArc<1>(a_2_in);
    
    PD_VALPRINT("pass.last",ArcString(pass.last));
    PD_VALPRINT("pass.next",ArcString(pass.next));
    PD_VALPRINT("b        ",ArcString(b        ));
    
    PD_VALPRINT("a_1_in ",ArcString(a_1_in ));
    PD_VALPRINT("a_1_out",ArcString(a_1_out));
    PD_VALPRINT("a_2_in ",ArcString(a_2_in ));
    PD_VALPRINT("a_2_out",ArcString(a_2_out));
    
    PD_VALPRINT("c_1",CrossingString(c_1));
    PD_VALPRINT("c_2",CrossingString(c_2));
    
    // Should be obvious:
    PD_ASSERT(pass.last != a_1_in );
    PD_ASSERT(b         != a_2_out);
    PD_ASSERT(a_1_in  != a_2_in );
    PD_ASSERT(a_1_out != a_2_out);
    
    // Call preconditions.
    PD_ASSERT(!ArcMarkedQ(a_1_in )); // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(!ArcMarkedQ(a_1_out)); // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(!ArcMarkedQ(a_2_in )); // Because of !CrossingMarkedQ(c_2).
    PD_ASSERT(!ArcMarkedQ(a_2_out)); // Because of !CrossingMarkedQ(c_2).

    
    /* // Picture side_1 == Right and for c_1 left-handed and c_2 right-handed.
     *
     *               a_1_out           a_2_in
     *             ^                 |
     *             |                 |
     *     last    |c_1   next       |     b
     *  ---------->----------------->----------->
     *             |                 |c_2
     *             |                 |
     *             | a_1_in          v a_2_out
     */
    
    if( a_1_out == a_2_in )
    {
        PD_PRINT(tag() + " detected move at ingoing port of " + CrossingString(c_2) + ".");
        
        PD_ASSERT( a_1_in  != b );
        PD_ASSERT( a_2_in  != b );
        
        PD_ASSERT( a_1_out != pass.last );
        PD_ASSERT( a_2_out != pass.last );
        
        /*              a_2_out == a_2_in
         *             +---------------->+
         *             ^                 |
         *             |                 |
         *     last    |c_1    next      v      b
         *  ---------->----------------->-------------->
         *             ^                 |c_2
         *             |                 |
         *             | a_1_in          v a_2_out
         */
        
        // It could happen that `b` is the strand start.
        // We have to take care of this outside the function.
        
        PD_ASSERT(pass.last != b);    // Because ArcMarkedQ(pass.last) and !ArcMarkedQ(b)
        PD_ASSERT( ArcMarkedQ(pass.last));
        PD_ASSERT(!ArcMarkedQ(b));
        Reconnect<Head>(pass.last,b);
        
        if( a_1_in == a_2_out ) [[unlikely]]
        {
            PD_ASSERT(!ArcMarkedQ(a_2_out));
            DeactivateArc(a_2_out);
            CreateUnlinkFromArc(a_1_in);
        }
        else
        {
            PD_ASSERT(!ArcMarkedQ(a_2_out));
            Reconnect<Head>(a_1_in,a_2_out);
        }
        
        PD_ASSERT(!ArcMarkedQ(pass.next));
        DeactivateArc(pass.next);
        PD_ASSERT(!ArcMarkedQ(a_2_in));
        DeactivateArc(a_2_in);
        DeactivateCrossing(c_1);
        DeactivateCrossing(c_2);
        CountReidemeister_II();
        
        AssertArc<1>(pass.last);
        AssertArc<0>(pass.next);
        AssertArc<0>(b);
        AssertArc<0>(a_2_out);
        AssertArc<0>(a_2_in);
        AssertCrossing<0>(c_1);
        AssertCrossing<0>(c_2);
        
        ++change_counter;
        --pass.arc_count; // We have to shorten the strand because we check the current strand pass.last again.
        return true;
    }
    
    if( a_1_in == a_2_out )
    {
        PD_PRINT(tag() + " detected move at outgoing port of " + CrossingString(c_2) + ".");
        
        // This cannot happen because we called FindPass_HandleLoop.
        PD_ASSERT( a_1_out != pass.last );
        
//        // A nasty case that is easy to overlook.
        if( a_2_in == b ) [[unlikely]]
        {
            //               a_1_out         +<---+ a_2_in == b
            //             ^                 |    |
            //             |                 |    |
            //     last    |c_1    next      v    |
            //  ---------->----------------->---->+
            //             ^                 |c_2
            //             |                 |
            //             |                 v
            //             +<----------------+
            //              a_1_in == a_2_out
            //
            
            PD_ASSERT(!ArcMarkedQ(a_1_out));
            Reconnect<Head>(pass.last,a_1_out);
            PD_ASSERT(!ArcMarkedQ(b));
            DeactivateArc(b);
            CountReidemeister_I();
            CountReidemeister_I();
        }
        else
        {
            //             O a_1_out         O a_2_in
            //             ^                 |
            //             |                 |
            //    last     |c_1    next      v     b
            //  ---------->----------------->------------>
            //             ^                 |c_2
            //             |                 |
            //             |                 v
            //             O<----------------O
            //              a_1_in == a_2_out
            //
            
            // It could happen that `b == pass.first`.
            // We have to take care of this outside the function.
            
            // This should work also if pass.last == a_1_out. This would imply that a_prev == a_begin.
            // This is why we prefer keeping a_prev alive over concatenating with the unlocked Reidemeister I move.
            
            PD_ASSERT(!ArcMarkedQ(b));
            Reconnect<Head>(pass.last,b);
            PD_ASSERT(!ArcMarkedQ(a_2_in));
            Reconnect<Tail>(a_1_out,a_2_in);
            
            // It could happen that `a_1_out == pass.first`. In this case we could make the maximal strand one arc longer. However, we keep the strand start where it was. Rerouting a strand that is slightly shorter than it could be does not break the algorithm.
            
            CountReidemeister_II();
        }

        PD_ASSERT(!ArcMarkedQ(pass.next));
        DeactivateArc(pass.next);
        PD_ASSERT(!ArcMarkedQ(a_2_out));
        DeactivateArc(a_2_out);
        DeactivateCrossing(c_1);
        DeactivateCrossing(c_2);
        
        AssertArc<1>(pass.last);
        AssertArc<0>(pass.next);
        AssertArc<0>(b);
        AssertCrossing<0>(c_1);
        AssertCrossing<0>(c_2);
        
        ++change_counter;
        --pass.arc_count; // We have to shorten the strand because we check the current strand pass.last again.
        return true;
    }

    PD_PRINT(tag() + " found no changes.");
    
    AssertArc<1>(pass.last);
    AssertArc<1>(pass.next);
    AssertArc<1>(b);
    
    return false;
}
