private:

void RemoveLoop( const Int e, const Int c_0 )
{
    PD_TIMER(timer,MethodName("RemoveLoop"));
    
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    // DEBUGGING
#ifdef PD_DEBUG
    int mark_counter = 0;
    mark_counter += (A_mark[pd->C_arcs(c_0,Out,Left )] == A_mark[e]);
    mark_counter += (A_mark[pd->C_arcs(c_0,Out,Right)] == A_mark[e]);
    mark_counter += (A_mark[pd->C_arcs(c_0,In ,Left )] == A_mark[e]);
    mark_counter += (A_mark[pd->C_arcs(c_0,In ,Right)] == A_mark[e]);
    
    if ( mark_counter >= 4 ) // This should never happen.
    {
        wprint(MethodName("RemoveLoop") + " with " + ((mark_counter = 3) ? "T" : "X" ) + "-junction; strand_length = " + ToString(strand_length));
        TOOLS_LOGDUMP(c_0);
        TOOLS_LOGDUMP(e  );
        TOOLS_LOGDUMP(pd->C_arcs(c_0,Out,Left ));
        TOOLS_LOGDUMP(pd->C_arcs(c_0,Out,Right));
        TOOLS_LOGDUMP(pd->C_arcs(c_0,In ,Left ));
        TOOLS_LOGDUMP(pd->C_arcs(c_0,In ,Right));
        TOOLS_LOGDUMP(ArcMarkedQ(e));
        TOOLS_LOGDUMP(ArcMarkedQ(pd->C_arcs(c_0,Out,Left )));
        TOOLS_LOGDUMP(ArcMarkedQ(pd->C_arcs(c_0,Out,Right)));
        TOOLS_LOGDUMP(ArcMarkedQ(pd->C_arcs(c_0,In ,Left )));
        TOOLS_LOGDUMP(ArcMarkedQ(pd->C_arcs(c_0,In ,Right)));
    }
#endif // PD_DEBUG
    
    // TODO: If the link has multiple components, it can also happen that the loop strand is an unknot that lies on top (or under) the remaining diagram. We have to take care of this as well.
    if constexpr( mult_compQ )
    {
        const Int a = NextArc(e,Head,c_0);
        
        if( ArcMarkedQ(a) )
        {
            const bool u_0 = (pd->C_arcs(c_0,Out,Right) == a);
            
            CollapseArcRange(a,e,strand_length);
            DeactivateArc(a);
            CreateUnlinkFromArc(a);
            
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
            
            const Int n_0 = pd->C_arcs(c_0,!u_0,Left );
            const Int s_0 = pd->C_arcs(c_0, u_0,Right);

            if( s_0 == n_0 )
            {
                DeactivateArc(s_0);
                CreateUnlinkFromArc(s_0);
            }
            else
            {
                if( u_0 )
                {
                    Reconnect<Head>(s_0,n_0);
                }
                else
                {
                    Reconnect<Tail>(s_0,n_0);
                }
            }
            
            DeactivateCrossing(c_0);
            
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
    
    const bool side = (pd->C_arcs(c_0,In,Right) == e);
              
    const Int b = pd->C_arcs(c_0,Out,side);
    
    CollapseArcRange(b,e,strand_length);

    // Now b is guaranteed to be a loop arc. (e == b or e is deactivated.)
    
    (void)Reidemeister_I<false>(b);
    
    ++change_counter;
    
#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_RemoveLoop += Tools::Duration(start_time,stop_time);
#endif
}
