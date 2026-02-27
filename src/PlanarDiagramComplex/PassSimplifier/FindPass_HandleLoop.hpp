template<bool find_maximal_passQ = false>
bool FindPass_HandleLoop( mref<Pass_T> pass, const Int c_1  )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindPass_HandleLoop"); };
    
    PD_TIMER(timer,tag());
    
    PD_ASSERT(pd->CheckAll());
    
//    Int & a = pass.last;
    const bool side = (pd->C_arcs(c_1,In,Right) == pass.last);
    Int a_out = pd->C_arcs(c_1,Out, side); // Might change later.
    
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
        
        
        // CollapseArcRange may mess around with the diagram an take it to an "impossible" state. We better decide now whether we are in the case "Big Unlink" or "Big Hopf Link".
        const bool hopf_linkQ = (ArcOverQ(pass.last,Head) != pass.overQ);
        const CrossingState_T c_1_state  = pd->C_state[c_1];
        
        Int deleted_arc_count = CollapseArcRange( pass.next, pass.last, pass.arc_count, pass.mark );
        
        // CAUTION: At this point the diagram in in an invalid state: pass.next bends back to the west port of c_1.
        PD_ASSERT(pd->C_arcs(c_1,In,side) == pass.next);
        
        // It can happen, that the arcs at the south and north ports of c_1 are altered by CollapseArcRange. This is why we load them now (again).
        const Int a_in  = pd->C_arcs(c_1,In ,!side);
                  a_out = pd->C_arcs(c_1,Out, side);
        
        AssertArc<1>(a_in);
        AssertArc<1>(a_out);
        AssertArc<1>(pass.next);
        AssertArc<0>(pass.last);
        
        
        PD_VALPRINT("c_1",CrossingString(c_1));
        PD_VALPRINT("pass.last",ArcString(pass.last));
        PD_VALPRINT("pass.next",ArcString(pass.next));
        PD_VALPRINT("a_in     ",ArcString(a_in     ));
        PD_VALPRINT("a_out    ",ArcString(a_out    ));
        
        PD_VALPRINT("pd->C_arcs(c_1,Out,Left )",ArcString(pd->C_arcs(c_1,Out,Left )));
        PD_VALPRINT("pd->C_arcs(c_1,Out,Right)",ArcString(pd->C_arcs(c_1,Out,Right)));
        PD_VALPRINT("pd->C_arcs(c_1,In ,Left )",ArcString(pd->C_arcs(c_1,In ,Left )));
        PD_VALPRINT("pd->C_arcs(c_1,In ,Right)",ArcString(pd->C_arcs(c_1,In ,Right)));
        
        DeactivateArc(pass.next);
        ++deleted_arc_count;
        change_counter += static_cast<Size_T>(deleted_arc_count);

        if( hopf_linkQ )
        {
            PD_PRINT("\t\tBig Hopf Link");
            if( a_in == a_out )
            {
                PD_PRINT("\t\ta_in == a_out");
                DeactivateArc(a_out);
                // TODO: If these are the last arcs deactivated, this implicitly creates another unlink. It is a trivial summand, so it does not change topology. But it is an annoyance.
            }
            else
            {
                PD_PRINT("\t\ta_in != a_out");
                Reconnect<Head>(a_in,a_out);
            }
            CreateHopfLinkFromArcs(a_out,pass.last,c_1_state);
        }
        else
        {
            PD_PRINT("\t\tBig Unlink");
            if( a_in == a_out )
            {
                PD_PRINT("\t\ta_in == a_out");
                DeactivateArc(a_out);
                
                // TODO: If this is the last arcs deactivated, this already creates another unlink implicitly. Then we don't have create new one.
                if( pd->CrossingCount() > Int(0) )
                {
                    CreateUnlinkFromArc(a_out);
                }
            }
            else
            {
                PD_PRINT("\t\ta_in != a_out");
                Reconnect<Head>(a_in,a_out);
            }
            CreateUnlinkFromArc(pass.last);
        }
        
        DeactivateCrossing(c_1);
        
        // If a_in is still alive, we could start a new pass from there... but that could mix up a lot in memory access. We better stop here.

        pass.activeQ = false;
        return false;
    }
}
