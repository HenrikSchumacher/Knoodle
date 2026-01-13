bool R_Ia_below()
{
    PD_DPRINT("R_Ia_below()");
    
    if(n_0 != n_1)
    {
        PD_DPRINT("\tn_0 != n_1");
        return false;
    }
    
    PD_DPRINT("\tn_0 == n_1");
    
    /*       +-----------+             +-----------+
     *       |     a     |             |     a     |
     *    -->----------->|-->   or  -->|---------->--->
     *       |c_0        |c_1          |c_0        |c_1
     *       |           |             |           |
     */
    
    if constexpr ( mult_compQ && allow_disconnectsQ )
    {
        if( s_0 == s_1 )
        {
            PD_DPRINT("\t\ts_0 == s_1");
            
            if( w_0 == e_1 )
            {
                PD_DPRINT("\t\t\tw_0 == e_1");
                
                wprint(MethodName("R_Ia_below")+": Split a Hopf link as connected component.");                
                
                DeactivateArc(w_0);
                DeactivateArc(a);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);

                CreateHopfLinkFromArcs(a,n_0,true);
                
                return true;
            }
            
            PD_DPRINT("\t\t\tw_0 != e_1");
            
            wprint(MethodName("R_Ia_below")+": Disconnected a Hopf link as connected summand.");
            
            Reconnect<Head>(w_0,e_1);
            DeactivateArc(a);
            DeactivateArc(n_0);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            CreateHopfLinkFromArcs(a,n_0,false);
            
            return true;
        }
    }
    
    
    load_c_2();
                        
    if( e_2 == s_1 )
    {
        PD_DPRINT("\t\te_2 == s_1");
        
        if(o_0 == o_2)
        {
            PD_DPRINT("\t\t\to_0 == o_2");
            
            if constexpr ( mult_compQ )
            {
                // The following can only happen with more than one component.
                if(w_2 == s_2)
                {
                    PD_DPRINT("\t\t\t\tw_2 == s_2");
                    
                    /*  R_I move.
                     *
                     *           +-----------+             +-----------+
                     *           |           |             |           |
                     *           |     a     |             |     a     |
                     *        -->----------->|-->   or  -->|---------->--->
                     *           |c_0        |c_1          |c_0        |c_1
                     *           O---O   O---O             O---O   O---O
                     *                \ /                       \ /
                     *                 / c_2                     \  c_2
                     *                / \                       / \
                     *           w_2 O   O s_2             w_2 O   O s_2
                     *               |   |                     |   |
                     *               +---+                     +---+
                     */
                    
                    // TODO: We could disconnect-sum a Hopf link here.
                    PD_PRINT(MethodName("R_Ia_below")+": Detected a Hopf link as connected summand.");
                    
                    Reconnect(s_0,u_1,s_1);
                    DeactivateArc(w_2);
                    DeactivateCrossing(c_2);
                    
                    // TODO: Implement counters.
//                    ++pd.R_I_counter;
                    
                    return true;
                }
            }
            
            PD_DPRINT("\t\t\t\tw_2 != s_2");
            
            /*  Current knowledge
             *
             *           +-----------+             +-----------+
             *           |           |             |           |
             *           |     a     |             |     a     |
             *        -->----------->|-->   or  -->|---------->--->
             *           |c_0        |c_1          |c_0        |c_1
             *           O---O   O---O             O---O   O---O
             *                \ /                       \ /
             *                 / c_2                     \  c_2
             *                / \                       / \
             *           w_2 O   O s_2             w_2 O   O s_2
             */
            
            
            if( w_2 == w_0 )
            {
                PD_DPRINT("\t\t\t\t\tw_2 == w_0");
                
                if( s_2 == e_1 )
                {
                    PD_DPRINT("\t\t\t\t\t\ts_2 == e_1");

                    wprint(MethodName("R_Ia_below")+": Seldom case: detected an unlink (three crossings deleted.");
                    
                    /*           +-----------+             +-----------+
                     *           |           |             |           |
                     *           |     a     |     or      |     a     |
                     *     +---->----------->|---->+  +--->|---------->---->+
                     *     |     |c_0        |c_1  |  |    |c_0        |c_1 |
                     *     |     O---O   O---O     |  |    O---O   O---O    |
                     *     |          \ /          |  |         \ /         |
                     *     |           / c_2       |  |          \  c_2     |
                     *     |w_0==w_2  / \  s_2==e_1|  |w_0==w_2 / \  s_2=e_1|
                     *     +---------O   O---------+  +--------O   O--------+
                     */
                    
                    DeactivateArc(a);
                    DeactivateArc(e_1);
                    DeactivateArc(s_0);
                    DeactivateArc(n_0);
                    DeactivateArc(s_1);
                    DeactivateArc(w_0);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_2);
                    CreateUnlinkFromArc(a);
                    
                    // TODO: Implement counters.
//                    ++pd.R_I_counter;
//                    ++pd.R_II_counter;
                    
                    return true;
                }

                PD_DPRINT("\t\t\t\t\t\ts_2 != e_1");
                
                wprint(MethodName("R_Ia_below")+": Seldom case: three crossings deleted.");
                /*           +-----------+             +-----------+
                 *           |           |             |           |
                 *           |     a     |     or      |     a     |
                 *     +---->----------->|---->   +--->|---------->---->
                 *     |     |c_0        |c_1     |    |c_0        |c_1
                 *     |     O---O   O---O        |    O---O   O---O
                 *     |          \ /             |         \ /
                 *     |           / c_2          |          \  c_2
                 *     |w_0==w_2  / \             |w_0==w_2 / \
                 *     +---------O   O s_2        +--------O   O s_2
                 */
                
                Reconnect<Head>(s_2,e_1);
                DeactivateArc(s_0);
                DeactivateArc(n_0);
                DeactivateArc(s_1);
                DeactivateArc(w_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_2);
                
                // TODO: Implement counters.
//                ++pd.R_I_counter;
//                ++pd.R_II_counter;
                
                return true;
            }
            
            PD_DPRINT("\t\t\t\t\tw_2 != w_0");
            
            if( s_2 == e_1 )
            {
                PD_DPRINT("\t\t\t\t\t\ts_2 == e_1");
                
                wprint(MethodName("R_Ia_below")+": Seldom case: three crossings deleted.");
                
                /*           +-----------+             +-----------+
                 *           |           |             |           |
                 *           |     a     |     or      |     a     |
                 *       --->----------->|---->+   --->|---------->---->+
                 *           |c_0        |c_1  |       |c_0        |c_1 |
                 *           O---O   O---O     |       O---O   O---O    |
                 *                \ /          |            \ /         |
                 *                 / c_2       |             \  c_2     |
                 *                / \  s_2==e_1|            / \  s_2=e_1|
                 *           w_2 O   O---------+       w_2 O   O--------+
                 */
                
                Reconnect<Head>(w_0,w_2);
                DeactivateArc(a);
                DeactivateArc(e_1);
                DeactivateArc(s_0);
                DeactivateArc(n_1);
                DeactivateArc(s_1);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_2);
                
                // TODO: Implement counters.
//                ++pd.R_I_counter;
//                ++pd.R_II_counter;
                
                return true;
            }
            
            PD_DPRINT("\t\t\t\t\t\ts_2 != e_1");
            
            
            /*  R_Ia move.
             *
             *           +-----------+             +-----------+
             *           |           |             |           |
             *           |     a     |             |     a     |
             *        -->----------->|-->   or  -->|---------->--->
             *           |c_0        |c_1          |c_0        |c_1
             *           O---O   O---O             O---O   O---O
             *                \ /                       \ /
             *                 / c_2                     \  c_2
             *                / \                       / \
             *           w_2 O   O s_2             w_2 O   O s_2
             */
            
            // We reconnect manually because we have to invert the vertical strands.
                        
            A_cross(w_2,u_1) = c_0;
            A_cross(s_2,u_0) = c_1;
            std::swap(A_cross(n_0,Head),A_cross(n_0,Tail));
            
//            if( u_0 )
//            {
//                // Change c_0;
//                /* By example u_0 = 1
//                 *
//                 *          O                   O
//                 *          ^                   |
//                 *          |                   |
//                 *          |                   |
//                 *    O-----X---->O  ==>  O-----X---->O
//                 *          |c_0                |c_0
//                 *          |                   |
//                 *          |                   v
//                 *          O                   O
//                 */
//                
//                C_arcs(c_0,In ,Left ) = n_0;
//                C_arcs(c_0,In ,Right) = C_arcs(c_0,In ,Left );
//                C_arcs(c_0,Out,Left ) = C_arcs(c_0,Out,Right);
//                C_arcs(c_0,Out,Right) = w_2;
//            }
//            else
//            {
//                // Change c_0;
//                /* By example u_0 = 0
//                 *
//                 *          O                   O
//                 *          |                   ^
//                 *          |                   |
//                 *          |                   |
//                 *    O-----X---->O  ==>  O-----X---->O
//                 *          |c_0                |c_0
//                 *          |                   |
//                 *          v                   |
//                 *          O                   O
//                 */
//                
//                // Careful, we have to do this in a specified order.
//                C_arcs(c_0,Out,Right) = C_arcs(c_0,Out,Left );
//                C_arcs(c_0,Out,Left ) = n_0;
//                C_arcs(c_0,In ,Left ) = C_arcs(c_0,In ,Right);
//                C_arcs(c_0,In ,Right) = w_2;
//            }
            
            // Careful, we have to do this in a specified order.
            C_arcs(c_0, u_0,Right) = C_arcs(c_0, u_0,Left );
            C_arcs(c_0, u_0,Left ) = n_0;
            C_arcs(c_0,!u_0,Left ) = C_arcs(c_0,!u_0,Right);
            C_arcs(c_0,!u_0,Right) = w_2;
            

//            if( u_1 )
//            {
//                // Change c_1;
//                /* By example u_1 = 1
//                 *
//                 *          O                   O
//                 *          ^                   |
//                 *          |                   |
//                 *          |                   |
//                 *    O-----X---->O  ==>  O-----X---->O
//                 *          |c_1                |c_1
//                 *          |                   |
//                 *          |                   v
//                 *          O                   O
//                 */
//                
//                C_arcs(c_1,In ,Right) = C_arcs(c_1,In ,Left );
//                C_arcs(c_1,In ,Left ) = n_1;
//                C_arcs(c_1,Out,Left ) = C_arcs(c_1,Out,Right);
//                C_arcs(c_1,Out,Right) = s_2;
//            }
//            else
//            {
//                // Change c_1;
//                /* By example u_1 = 0
//                 *
//                 *          O                   O
//                 *          |                   ^
//                 *          |                   |
//                 *          |                   |
//                 *    O-----X---->O  ==>  O-----X---->O
//                 *          |c_1                |c_1
//                 *          |                   |
//                 *          v                   |
//                 *          O                   O
//                 */
//
//                // Careful, we have to do this in a specified order.
//                C_arcs(c_1,Out,Right) = C_arcs(c_1,Out,Left );
//                C_arcs(c_1,Out,Left ) = n_1;
//                C_arcs(c_1,In ,Left ) = C_arcs(c_1,In ,Right);
//                C_arcs(c_1,In ,Right) = s_2;
//            }
            
            // Careful, we have to do this in a specified order.
            C_arcs(c_1, u_1,Right) = C_arcs(c_1, u_1,Left );
            C_arcs(c_1, u_1,Left ) = n_1;
            C_arcs(c_1,!u_1,Left ) = C_arcs(c_1,!u_1,Right);
            C_arcs(c_1,!u_1,Right) = s_2;
            
//            RecomputeArcState(a  );
//            RecomputeArcState(n_0);
//            RecomputeArcState(w_0);
//            RecomputeArcState(e_1);
//            RecomputeArcState(s_2);
//            RecomputeArcState(w_2);

            DeactivateArc(s_0);
            DeactivateArc(s_1);
            DeactivateCrossing(c_2);
            
            // TODO: Implement counters.
//            ++pd.R_Ia_counter;
            
            AssertArc<1>(a  );
            AssertArc<1>(n_0);
            AssertArc<0>(s_0);
            AssertArc<1>(w_0);
            AssertArc<1>(e_1);
            AssertArc<0>(s_1);
            AssertArc<1>(s_2);
            AssertArc<1>(w_2);
            AssertCrossing<1>(c_0);
            AssertCrossing<1>(c_1);
            AssertCrossing<0>(c_2);
            
            return true;

        } // if( o_0 == o_2 )

        PD_PRINT("\t\t\to_0 != o_2");
        
        /*
         *           +-----------+             +-----------+
         *           |           |             |           |
         *           |     a     |             |     a     |
         *        -->----------->|-->       -->|---------->--->
         *           |c_0        |c_1          |c_0        |c_1
         *           O---O   O---O             O---O   O---O
         *                \ /                       \ /
         *                 \ c_2                     /  c_2
         *                / \                       / \
         *               O   O s_2                 O   O s_2
         */
        
        // TODO: trefoil as connect summand detected
//        // TODO: How to store/use this info?
//        if( (w_0 == w_2) || (e_1 == s_2) )
//        {
////            PD_PRINT( "\t\t\t\t(w_0 == w_2) || (e_1 == s_2)" );
//            
//            logprint( "Detected a trefoil connected summand." );
//
//            return false;
//        }
        
        return false;
    }
    
    // This can only happen, if the planar diagram has multiple components!
    if constexpr( mult_compQ )
    {
        if(w_2 == s_1)
        {
            PD_PRINT("\t\tw_2 == s_1");
             
            
            if(e_2 == s_2)
            {
                PD_PRINT("\t\t\te_2 == s_2");
                
                // TODO: We could disconnect-sum a Hopf link here.
                PD_PRINT(MethodName("R_Ia_below")+": Detected a Hopf link as connected summand.");
                
                /*           +-----------+             +-----------+
                 *           |           |             |           |
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *        -->----------->|-->       -->|---------->--->
                 *           O   O---O   O             O   O---O   O
                 *           | e_2\ /s_2 |             | e_2\ /s_2 |
                 *       s_0 |     X c_2 | s_1         |     X c_2 | s_1
                 *           |    / \    |             |    / \    |
                 *           +---O   O---+             +---O   O---+
                 */
                
                Reconnect(s_0,u_1,s_1);
                DeactivateArc(e_2);
                DeactivateCrossing(c_2);
                
                // TODO: Implement counters.
//                ++pd.R_I_counter;
                
                return true;
            }
            
            PD_DPRINT("\t\t\te_2 != s_2");
            
            PD_PRINT(MethodName("R_Ia_below")+": Detected a Hopf link and a knot as connected summands.");
            
            /* Two further interesting cases.
             *
             *           +-----------+             +-----------+
             *           |           |             |           |
             *        c_0|     a     |c_1       c_0|     a     |c_1
             *        -->----------->|-->       -->|---------->--->
             *           O   O###O   O             O   O###O   O
             *           | e_2\ /s_2 |             |    \ /    |
             *       s_0 |     X c_2 | s_1         |     X c_2 | s_1
             *           |    / \    | w_2         |    / \    |
             *           +---O   O---+             +---O   O---+
             *
             *      These can be rerouted to the following two situations:
             *           +-----------+             +-----------+
             *           |           |             |           |
             *        c_0|     a     |c_1       c_0|     a     |c_1
             *        -->----------->|-->       -->|---------->--->
             *           O   +-------O             O   +-------O
             *           |   |     s_1             |   |     s_1
             *       s_0 |   O###O             s_0 |   O###O
             *           |       |                 |       |
             *           +-------+                 +-------+
             *
             *      No matter how c_2 is handed, we fuse s_0 and s_1 with their opposite arcs accross c_2 and deactivate c_2.
             *
             *      This will probably happen seldomly.
             */
            
            Reconnect(s_0,!u_0,s_2);
            Reconnect(s_1,!u_1,e_2);
            DeactivateCrossing(c_2);
            
            // TODO: Implement counters.
//            ++pd.twist_counter;
            
            AssertArc<1>(a  );
            AssertArc<1>(n_0);
            AssertArc<1>(s_0);
            AssertArc<1>(w_0);
            AssertArc<1>(n_1);
            AssertArc<1>(e_1);
            AssertArc<1>(s_1);
            AssertArc<0>(e_2);
            AssertArc<0>(s_2);
            AssertArc<0>(w_2);
            AssertCrossing<1>(c_0);
            AssertCrossing<1>(c_1);
            AssertCrossing<0>(c_2);
            
            return true;
        }
    }

    return false;
}
