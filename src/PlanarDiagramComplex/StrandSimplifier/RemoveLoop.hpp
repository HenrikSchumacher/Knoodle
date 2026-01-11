private:

void RemoveLoop( const Int e, const Int c_0 )
{
    PD_TIMER(timer,MethodName("RemoveLoop"));
    
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif

    // We can save the lookup here.
//    const Int c_0 = A_cross(e,Head);
    
    // TODO: If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well.
    if constexpr( mult_compQ )
    {
        const Int a = NextArc(e,Head,c_0);
        
        if( A_mark(a) == current_mark )
        {
            const bool side = ArcSide(a,Head);
            
            // Unlink on top.
            if constexpr ( mult_compQ )
            {
                wprint(MethodName("RemoveLoop") + ": We might falsely create an unlink here; it might also be a \"Big Hopf Link\".");
                TOOLS_LOGDUMP(overQ);
                TOOLS_LOGDUMP(ArcOverQ(a,Tail));
                TOOLS_LOGDUMP(ArcOverQ(e,Head));
            }
            
            CreateUnlinkFromArc(a);
            CollapseArcRange(a,e,strand_length);
            pd.DeactivateArc(a);
            
            // overQ == true;
            //                   n_0
            //                 O
            //                 |
            //        e        |        a
            // ####O----->O--------->O----->O######
            //                 |c_0
            //                 |
            //                 O
            //                  s_0
            
            // We read out n_0 and s_0 only now because CollapseArcRange might change the north and south port of c_0.
            
            const Int n_0 = C_arcs(c_0,!side,Left );
            const Int s_0 = C_arcs(c_0, side,Right);

            if( s_0 == n_0 )
            {
                wprint(MethodName("RemoveLoop") + ": s_0 == n_0; this should be impossible.");
                CreateUnlinkFromArc(s_0);
                pd.DeactivateArc(s_0);
                
            }
            else
            {
                if( side )
                {
                    Reconnect<Head>(s_0,n_0);
                }
                else
                {
                    Reconnect<Tail>(s_0,n_0);
                }
            }
            
            pd.DeactivateCrossing(c_0);
            
            ++change_counter;
            
            return;
        }
    }
    
    // side == Left             side == Right
    //
    //           ^                        |
    //           |b                       |
    //        e  |                     e  |
    //      ---->X---->              ---->X---->
    //           |c_0                     |c_0
    //           |                      b |
    //           |                        v
    
    
    const bool side = ArcSide(e,Head);
    const Int  b    = C_arcs(c_0,Out,side);
    
    CollapseArcRange(b,e,strand_length);
    // Now b is guaranteed to be a loop arc. (e == b or e is deactivated.)
    
    (void)Reidemeister_I<false>(b);
    
    ++change_counter;
    
#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_RemoveLoop += Tools::Duration(start_time,stop_time);
#endif
    
    PD_TOC(ClassName()+"::RemoveLoop");
}


//void RemoveLoop( const Int e, const Int c_0 )
//{
//    PD_TIMER(timer,MethodName("RemoveLoop"));
//    
//#ifdef PD_TIMINGQ
//    const Time start_time = Clock::now();
//#endif
//    
//    const Int a = NextArc(e,Head,c_0);
//
//
//    
//    if constexpr( mult_compQ )
//    {
//        // If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well.
//        if( A_mark(a) == current_mark )
//        {
//            if( ArcOverQ(a,Head) == overQ )
//            {
//                // overQ == true;
//                //                   n_0
//                //                 O
//                //                 |
//                //        e        |        a
//                // ####O----->O--------->O----->O######
//                //                 |c_0
//                //                 |
//                //                 O
//                //                  s_0
//                
//                // TODO: CHECK THIS THOROUGHLY!
//                const bool side = ArcSide(a,Tail);
//
//                pdc.CreateUnlinkFromArc(a);
//                CollapseArcRange(a,e,strand_length);
//                pd.DeactivateArc(a);
//                
//                // We read out n_0 and s_0 only now because CollapseArcRange might change the north and south port of c_0.
//                const Int n_0 = C_arcs(c_0,!side,Left );
//                const Int s_0 = C_arcs(c_0, side,Right);
//
//                if( s_0 == n_0 )
//                {
//                    eprint(MethodName("RemoveLoop") + "s_0 == n_0 while A_mark(a) == current_mark and ArcOverQ(a,Head) == overQ. This should be impossible.");
//                    // Should be impossible.
//                    ++pd.unlink_count;
//                    pd.DeactivateArc(s_0);
//                }
//                else
//                {
//                    if( side )
//                    {
//                        Reconnect<Head,true>(s_0,n_0);
//                    }
//                    else
//                    {
//                        Reconnect<Tail,true>(s_0,n_0);
//                    }
//                }
//                pd.DeactivateCrossing(c_0);
//                return;
//            }
//            else
//            {
//                // This could be a "Big Hopf Link" or a "Big Over Unlink"
//                
//                // overQ == true;
//                //                   n_0
//                //                 O
//                //                 |
//                //        e        |        a
//                // ####O----->O----|---->O----->O######
//                //                 |c_0
//                //                 |
//                //                 O
//                //                  s_0
//                
//                const bool side = ArcSide(a,Tail);
//
//                const Int b_out = C_arcs(c_0,Out,!side);
//                const Int b_in  = C_arcs(c_0,In , side);
//                
//                if( A_mark(b_out) == current_mark )
//                {
//                    /* "Big Figure-8 Unlink" like so:
//                     *
//                     *              +-------+
//                     *              |       |
//                     *       ##-----|-------|----##
//                     *              |       |
//                     *          e   |   a   |
//                     *      +-------|------>+
//                     *      |    c_0|
//                     *      |       |
//                     *  ##--|-------|------##
//                     *      |       |
//                     *      +-------+
//                     *
//                     */
//                    
//                    eprint(MethodName("RemoveLoop") + ": We have overlooked a \"Big Unlink\" from arc " + ArcString(a) + " to " + ArcString(e) + ".");
//                    
//                    // TODO: Maybe do
//                    // TODO:    CreateUnlinkFromArc(a);
//                    // TODO:    Set path to [a, b_in].
//                    // TODO:    RerouteToPath(a,b_in);
//                    // TODO:    Set path to [b_out, e].
//                    // TODO:    RerouteToPath(b_out,e);
//                    // TODO:    DeleteArc(a);
//                    // TODO:    DeleteArc(v);
//                    // TODO:    DeleteCrossing(c_0);
//                }
//                else
//                {
//                    /* "Big Hopf Link"
//                     *
//                     *            #       #
//                     *            #       #
//                     *            |       |
//                     *            |       |
//                     *         e  |   a   |
//                     *      +---->|------------>+
//                     *      ^  c_0|       |     |
//                     *      |     |       |     |
//                     *      |     |       |     |
//                     *      |     |       |  +--|--##
//                     *  ##--|--+  +-------+  |  |
//                     *      |  |             |  v
//                     *      +-------------------+
//                     *         |             |
//                     *         #             #
//                     */
//                    
//                    eprint(MethodName("RemoveLoop") + ": We have overlooked a \"Big Hopf Link\" from arc " + ArcString(a) + " to " + ArcString(e) + ".");
//                    
//                    // TODO: Beware the case a == e. (Should be impossible.)
//                    
//                    // TODO: Maybe do
//                    // TODO:    CreateUnlinkFromArc(a);
//                    // TODO:    Set path to [a, 2 * b_in + ??, e].
//                    // TODO:    RerouteToPath(a,2);
//                    // TODO:    Reidemeister_1(a);
//                }
//                
//                return;
//            }
//        }
//    }
//    
//    // With only one component the case A_mark(a) == current_mark cannot happen.
//    PD_ASSERT(A_mark(a) != current_mark);
//    
//    // side == Left             side == Right
//    //
//    //           ^                        |
//    //           |b                       |
//    //        e  |                     e  |
//    //      ---->X---->              ---->X---->
//    //           |c_0                     |c_0
//    //           |                      b |
//    //           |                        v
//    
//    const bool side = ArcSide(e,Head);
//    const Int b = C_arcs(c_0,Out,side);
//    CollapseArcRange(b,e,strand_length);
//    // Now b is guaranteed to be a loop arc. (e == b or e is deactivated.
//    (void)Reidemeister_I<false>(b);
//    
//    ++change_counter;
//    
//#ifdef PD_TIMINGQ
//    const Time stop_time = Clock::now();
//    Time_RemoveLoop += Tools::Duration(start_time,stop_time);
//#endif
//}
