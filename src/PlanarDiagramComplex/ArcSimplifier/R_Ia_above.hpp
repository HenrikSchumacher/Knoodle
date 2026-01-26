bool R_Ia_above()
{
    PD_DPRINT("R_Ia_above()");
    
    //Check for Reidemeister Ia move.
    if(s_0 != s_1)
    {
        return false;
    }
    
    PD_PRINT("\ts_0 == s_1");
    
    /*       |           |             |           |
     *       |     a     |             |     a     |
     *    -->----------->|-->   or  -->|---------->--->
     *       |c_0        |c_1          |c_0        |c_1
     *       +-----------+             +-----------+
     */

    // This cannot happen because we ran R_Ia_below first.
    PD_ASSERT(n_0 != n_1);
    
    load_c_3();
    
    if(e_3 == n_1)
    {
        PD_PRINT("\t\te_3 == n_1");
        
        if(o_0 == o_3)
        {
            PD_PRINT("\t\t\to_0 == o_3");
            
            if constexpr ( mult_compQ )
            {
                // The following can only happen with more than one component.
                if(w_3 == n_3)
                {
                    PD_PRINT("\t\t\t\tw_3 == n_3");
                    
                    /*            w_3                       w_3
                     *               +---+                     +---+
                     *               |   |                     |   |
                     *               O   O n_3                 O   O
                     *                \ /                       \ /
                     *                 \ c_3                     /  c_3
                     *            n_0 / \ n_1               n_0 / \ n_1
                     *           O---O   O---O             O---O   O---O
                     *           |     a     |             |     a     |
                     *        -->----------->|-->   or  -->|---------->--->
                     *           |c_0        |c_1          |c_0        |c_1
                     *           |           |             |           |
                     *           +-----------+             +-----------+
                     */
                    
                    if constexpr (allow_disconnectsQ)
                    {
                        if(w_0 == e_1)
                        {
                            PD_PRINT("\t\t\t\t\tw_0 == e_1");
                            
                            PD_NOTE(MethodName("R_Ia_above")+": Split a Hopf link as connected component.");
                            
                            DeactivateArc(a);
                            DeactivateArc(e_1);
                            DeactivateArc(s_0);
                            DeactivateArc(n_0);
                            DeactivateArc(n_3);
                            DeactivateArc(n_1);
                            DeactivateCrossing(c_0);
                            DeactivateCrossing(c_1);
                            DeactivateCrossing(c_3);
                            CreateHopfLinkFromArcs(a,n_0,c_0_state);
                            
                            return true;
                        }
                        
                        PD_PRINT("\t\t\t\t\tw_0 != e_1");

                        PD_NOTE(MethodName("R_Ia_above")+": Disconnect a Hopf link as connected summand. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                        
                        /*            w_3                       w_3
                         *               +---+                     +---+
                         *               |   |                     |   |
                         *               O   O n_3                 O   O
                         *                \ /                       \ /
                         *                 \ c_3                     /  c_3
                         *            n_0 / \ n_1               n_0 / \ n_1
                         *           O---O   O---O             O---O   O---O
                         *           |     a     |             |     a     |
                         *        -->----------->|-->   or  -->|---------->--->
                         *           |c_0        |c_1          |c_0        |c_1
                         *           |           |             |           |
                         *           +-----------+             +-----------+
                         */
                        
                        // This keeps e_1 alive, which is likely to be visited next.
                        Reconnect<Tail>(e_1,w_0);
                        DeactivateArc(s_0);
                        DeactivateArc(n_0);
                        DeactivateArc(n_3);
                        DeactivateArc(n_1);
                        DeactivateCrossing(c_0);
                        DeactivateCrossing(c_1);
                        DeactivateCrossing(c_3);
                        CreateHopfLinkFromArcs(a,n_0,c_0_state);

                        return true;
                    }
                    else
                    {
                        
                        Reconnect(n_0,u_0,n_1);
                        DeactivateArc(w_3);
                        DeactivateCrossing(c_3);
                        
                        // TODO: Implement counters.
    //                    ++pd.R_I_counter;
                        
                        AssertArc<1>(a  );
                        AssertArc<1>(n_0);
                        AssertArc<1>(s_0);
                        AssertArc<1>(w_0);
                        AssertArc<0>(n_1);
                        AssertArc<1>(e_1);
                        AssertArc<1>(s_1);
                        AssertArc<0>(n_3); // n_3 == w_3
                        AssertArc<0>(e_3); // e_3 == n_1
                        AssertArc<0>(w_3);
                        AssertCrossing<1>(c_0);
                        AssertCrossing<1>(c_1);
                        AssertCrossing<0>(c_3);
                        
                        return true;
                    }
                    
                    
                }
            } // if( mult_compQ )
            

            PD_PRINT("\t\t\t\tw_3 != n_3");

            /*           w_3 O   O n_3             w_3 O   O n_3
             *                \ /                       \ /
             *                 \ c_3                     / c_3
             *                / \                       / \
             *           O---O   O---O             O---O   O---O
             *           |           |             |           |
             *           |     a     |             |     a     |
             *        -->----------->|-->       -->|---------->--->
             *           |c_0        |c_1          |c_0        |c_1
             *           +-----------+             +-----------+
             */
            
            if( w_0 == w_3 )
            {
                PD_DPRINT("\t\t\t\t\tw_0 == w_3");
                
                if( n_3 == e_1 )
                {
                    PD_DPRINT("\t\t\t\t\t\tn_3 == e_1");
                    
                    PD_NOTE(MethodName("R_Ia_above")+": Seldom case: identified unlink (three crossings deleted. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                    
                    /*      +--------O   O--------+  +--------O   O--------+
                     *      |w_0==w_3 \ / n_3==e_1|  |w_0==w_3 \ / n_3==e_1|
                     *      |          \ c_3      |  |          / c_3      |
                     *      |         / \         |  |         / \         |
                     *      |    O---O   O---O    |  |    O---O   O---O    |
                     *      |    |           |    |  |    |           |    |
                     *      |    |     a     |    |  |    |     a     |    |
                     *      +--->----------->|--->+  +--->|---------->--- >+
                     *           |c_0        |c_1         |c_0        |c_1
                     *           +-----------+            +-----------+
                     */
                    
                    DeactivateArc(a);
                    DeactivateArc(e_1);
                    DeactivateArc(n_0);
                    DeactivateArc(s_0);
                    DeactivateArc(n_1);
                    DeactivateArc(w_0);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_3);
                    CreateUnlinkFromArc(a);
                    
                    // TODO: Implement counters.
//                    ++pd.R_I_counter;
//                    ++pd.R_II_counter;

                    return true;
                }
        
                PD_DPRINT("\t\t\t\t\t\tn_3 != e_1");
                
                PD_DPRINT(MethodName("R_Ia_above")+": Seldom case: three crossings deleted. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                
                /*      +--------O   O n_3        +--------O   O n_3
                 *      |w_0==w_3 \ /             |w_0==w_3 \ /
                 *      |          \ c_3          |          / c_3
                 *      |         / \             |         / \
                 *      |    O---O   O---O        |    O---O   O---O
                 *      |    |           |        |    |           |
                 *      |    |     a     |        |    |     a     |
                 *      +--->----------->|-->     +--->|---------->--->
                 *           |c_0        |c_1          |c_0        |c_1
                 *           +-----------+             +-----------+
                 */
                
                // This keeps e_1 alive, which is likely to be visited next.
                Reconnect<Tail>(e_1,n_3);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(n_1);
                DeactivateArc(w_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                
                // TODO: Implement counters.
//                ++pd.R_I_counter;
//                ++pd.R_II_counter;
                
                return true;
            }

            PD_DPRINT("\t\t\t\t\tw_0 != w_3");
            
            if( n_3 == e_1 )
            {
                PD_DPRINT("\t\t\t\t\t\tn_3 == e_1");
                
                PD_DPRINT(MethodName("R_Ia_above")+": Seldom case: three crossings deleted. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                
                /*           w_3 O   O--------+       w_3 O   O--------+
                 *                \ / n_3==e_1|            \ / n_3==e_1|
                 *                 \ c_3      |             / c_3      |
                 *                / \         |            / \         |
                 *           O---O   O---O    |       O---O   O---O    |
                 *           |           |    |       |           |    |
                 *           |     a     |    |       |     a     |    |
                 *       --->----------->|--->+   --->|---------->--- >+
                 *           |c_0        |c_1         |c_0        |c_1
                 *           +-----------+            +-----------+
                 */
                
                // This keeps w_3 alive, which is likely to be visited next.
                Reconnect<Tail>(w_3,w_0);
                DeactivateArc(a);
                DeactivateArc(e_1);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(n_1);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                
                // TODO: Implement counters.
//                ++pd.R_I_counter;
//                ++pd.R_II_counter;
                
                return true;
            }
            
            PD_DPRINT("\t\t\t\t\t\tn_3 != e_1");
            
            PD_PRINT(MethodName("R_Ia_above")+": Perform an R_Ia move. ( crossing_count = " + ToString(pd.crossing_count) + ")");
            
            /*  R_Ia move.
             *
             *           w_3 O   O n_3             w_3 O   O n_3
             *                \ /                       \ /
             *                 \ c_3                     / c_3
             *                / \                       / \
             *           O---O   O---O             O---O   O---O
             *           |           |             |           |
             *           |     a     |             |     a     |
             *        -->----------->|-->       -->|---------->--->
             *           |c_0        |c_1          |c_0        |c_1
             *           +-----------+             +-----------+
             */
            
            // We reconnect manually because we have to invert the vertical strands.
            
            A_cross(w_3,!u_1) = c_0;
            A_cross(n_3,!u_0) = c_1;
            std::swap(A_cross(s_0,Head),A_cross(s_0,Tail));
            
            C_arcs(c_0, u_0,Right) = C_arcs(c_0, u_0,Left );
            C_arcs(c_0, u_0,Left ) = w_3;
            C_arcs(c_0,!u_0,Left ) = C_arcs(c_0,!u_0,Right);
            C_arcs(c_0,!u_0,Right) = s_0;
            
            C_arcs(c_1, u_1,Right) = C_arcs(c_1, u_1,Left );
            C_arcs(c_1, u_1,Left ) = n_3;
            C_arcs(c_1,!u_1,Left ) = C_arcs(c_1,!u_1,Right);
            C_arcs(c_1,!u_1,Right) = s_1;
            
//            RecomputeArcState(a  );
//            RecomputeArcState(s_0);
//            RecomputeArcState(w_0);
//            RecomputeArcState(e_1);
//            RecomputeArcState(n_3);
//            RecomputeArcState(w_3);
            
            DeactivateArc(n_0);
            DeactivateArc(n_1);
            DeactivateCrossing(c_3);
            
            // TODO: Implement counters.
//            ++pd.R_Ia_counter;
            
            AssertArc<1>(a  );
            AssertArc<0>(n_0);
            AssertArc<1>(s_0);
            AssertArc<1>(w_0);
            AssertArc<0>(n_1);
            AssertArc<1>(e_1);
            AssertArc<1>(n_3);
            AssertArc<1>(w_3);
            AssertCrossing<1>(c_0);
            AssertCrossing<1>(c_1);
            AssertCrossing<0>(c_3);
            
            return true;
        }
        else // if(o_0 != o_3)
        {
            PD_PRINT("\t\t\to_0 != o_3");
            
            /*       w_3 O   O n_3                w_3 O   O n_3
             *            \ /                          \ /
             *             /                            \
             *            / \                          / \
             *       O---O   O---O                O---O   O---O
             *       |           |                |           |
             *   w_0 |     a     |  e_1      w_0  |     a     |  e_1
             *   O-->----------->|-->O   or   O-->|---------->--->O
             *       |c_0        |c_1             |c_0        |c_1
             *       +-----------+                +-----------+
             */
             
            if( w_0 == w_3 )
            {
                PD_PRINT("\t\t\t\tw_0 == w_3");
                
                if( n_3 == e_1 )
                {
                    PD_PRINT("\t\t\t\t\tn_3 == e_1");
                    PD_NOTE(MethodName("R_Ia_above")+": Split a trefoil as link component. ( crossing_count = " + ToString(pd.crossing_count) + ")");

                    /*  +--------O   O---------+     +--------O   O---------+
                     *  |w_0==w_3 \ / n_3==e_1 |     |w_0==w_3 \ / n_3==e_1 |
                     *  |          /           |     |          \           |
                     *  |         / \          |     |         / \          |
                     *  |    O---O   O---O     |     |    O---O   O---O     |
                     *  |    |           |     |     |    |           |     |
                     *  |    |     a     |     |     |    |     a     |     |
                     *  +O-->----------->|-->O-+ or  +O-->|---------->--->O-+
                     *       |c_0        |c_1             |c_0        |c_1
                     *       +-----------+                +-----------+
                     */

                    DeactivateArc(w_0);
                    DeactivateArc(a  );
                    DeactivateArc(e_1);
                    DeactivateArc(n_0);
                    DeactivateArc(s_0);
                    DeactivateArc(n_1);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_3);
                    CreateTrefoilKnotFromArc(a,c_0_state);
                    return true;
                }
                
                PD_PRINT("\t\t\t\t\tn_3 != e_1");
                
                PD_NOTE(MethodName("R_Ia_above")+": Disconnect a trefoil as connected summand. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                
                /*  +--------O   O n_3           +--------O   O n_3
                 *  |w_0==w_3 \ /                |w_0==w_3 \ /
                 *  |          /                 |          \
                 *  |         / \                |         / \
                 *  |    O---O   O---O           |    O---O   O---O
                 *  |    |           |           |    |           |
                 *  |    |     a     |  e_1      |    |     a     |  e_1
                 *  +O-->----------->|-->O   or  +O-->|---------->--->O
                 *       |c_0        |c_1             |c_0        |c_1
                 *       +-----------+                +-----------+
                 */
                
                Reconnect<Head>(n_3,e_1);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(n_1);
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                CreateTrefoilKnotFromArc(a,c_0_state);
                return true;
            }
            
            PD_PRINT("\t\t\t\tw_0 != w_3");
            
            if( n_3 == e_1 )
            {
                PD_PRINT("\t\t\t\t\tn_3 == e_1");
                
                PD_NOTE(MethodName("R_Ia_above")+": Disconnect a trefoil as connected summand. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                                
                /*       w_3 O   O---------+          w_3 O   O---------+
                 *            \ / n_3==e_1 |               \ / n_3==e_1 |
                 *             /           |                \           |
                 *            / \          |               / \          |
                 *       O---O   O---O     |          O---O   O---O     |
                 *       |           |     |          |           |     |
                 *   w_0 |     a     |     |     w_0  |     a     |     |
                 *   O-->----------->|-->O-+ or   O-->|---------->--->O-+
                 *       |c_0        |c_1             |c_0        |c_1
                 *       +-----------+                +-----------+
                 */
                
                // This keeps w_3 alive, which is likely to be visited next.
                Reconnect<Tail>(w_3,w_0);
                DeactivateArc(a  );
                DeactivateArc(e_1);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(n_1);
                
                CreateTrefoilKnotFromArc(a,C_state[c_0]);
                
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                
                return true;
            }
            
            PD_PRINT("\t\t\t\t\tn_3 != e_1");
        }
        
    } // if( e_3 == n_1 )
    
    PD_PRINT("\t\te_3 != n_1");
    
    if constexpr( mult_compQ )
    {
        // The following can only happen with more than one component.
        if(w_3 == n_1)
        {
            PD_PRINT("\t\t\tw_3 == n_1");
            
            if( e_3 == n_3 )
            {
                PD_PRINT("\t\t\t\te_3 == n_3");
                
                /*           +---O   O---+             +---O   O---+
                 *           |    \ /    |             |    \ /    |
                 *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
                 *           |    / \    |             |    / \    |
                 *           |   O---O   |             |   O---O   |
                 *           | e_3 = n_3 |             | e_3 = n_3 |
                 *           O           O             O           O
                 *        -->----------->|-->       -->|---------->--->
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *           |           |             |           |
                 *           +-----------+             +-----------+
                 */
                
                if( w_0 == e_1 )
                {
                    PD_PRINT("\t\t\t\tw_0 == e_1");
                    
                    PD_NOTE(MethodName("R_Ia_above")+": Split a Hopf link as link component. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                    
                    DeactivateArc(w_0);
                    DeactivateArc(a  );
                    DeactivateArc(s_0);
                    DeactivateArc(n_0);
                    DeactivateArc(n_3);
                    DeactivateArc(n_1);
                    
                    CreateHopfLinkFromArcs(a,n_0,c_0_state);
                    
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_3);
                    
                    return true;
                }
                
                PD_PRINT("\t\t\t\tw_0 != e_1");

                /*           +---O   O---+             +---O   O---+
                 *           |    \ /    |             |    \ /    |
                 *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
                 *           |    / \    |             |    / \    |
                 *           |   O---O   |             |   O---O   |
                 *           | e_3 = n_3 |             | e_3 = n_3 |
                 *           O           O             O           O
                 *        -->----------->|-->       -->|---------->--->
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *           |           |             |           |
                 *           +-----------+             +-----------+
                 */
                
                PD_NOTE(MethodName("R_Ia_above")+": Disconnect a Hopf link as connected summand. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                
                // This keeps e_1 alive, which is likely to be visited next.
                Reconnect<Tail>(e_1,w_0);
                DeactivateArc(a  );
                DeactivateArc(s_0);
                DeactivateArc(n_0);
                DeactivateArc(n_3);
                DeactivateArc(n_1);
                
                CreateHopfLinkFromArcs(a,n_0,c_0_state);
                
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_2);
                
                return true;
                
                
                
                // This is the old version that did just the Reidemeister I move.
//                Reconnect(n_1,u_1,n_0);
//                DeactivateArc(e_3);
//                DeactivateCrossing(c_3);
//                
//                // TODO: Implement counters.
////                ++pd.R_I_counter;
//                
//                AssertArc<1>(a  );
//                AssertArc<0>(n_0);
//                AssertArc<1>(s_0);
//                AssertArc<1>(w_0);
//                AssertArc<1>(n_1);
//                AssertArc<1>(e_1);
//                AssertArc<1>(s_1);
//                AssertArc<0>(n_3);
//                AssertArc<0>(e_3);
//                AssertArc<0>(w_3);
//                AssertCrossing<1>(c_0);
//                AssertCrossing<1>(c_1);
//                AssertCrossing<0>(c_3);
//                
//                return true;
            }
            
            PD_PRINT("\t\t\t\te_3 != n_3");
            
            if( w_0 == e_1 )
            {
                PD_PRINT("\t\t\t\t\tw_0 == e_1");
                
                PD_NOTE(MethodName("R_Ia_above")+": Disconnect a Hopf link as connected summand. ( crossing_count = " + ToString(pd.crossing_count) + ")");
                
               /*           +---O   O---+             +---O   O---+
                *           |    \ /    |             |    \ /    |
                *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
                *           |    / \    |             |    / \    |
                *           |   O###O   |             |   O###O   |
                *           | e_3  n_3  |             |  e_3  n_3 |
                *           O           O             O           O
                *      +--->----------->|--->+   +--->|---------->---->+
                *      | c_0|     a     |c_1 |   | c_0|     a     |c_1 |
                *      |    |           |    |   |    |           |    |
                *      |    +-----------+    |   |    +-----------+    |
                *      |                     |   |                     |
                *      +---------------------+   +---------------------+
                */
                
                Reconnect(e_3,u_0,n_3);
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateArc(n_1);
                DeactivateArc(s_0);
                DeactivateArc(n_0);
                
                CreateHopfLinkFromArcs(n_0,a,c_0_state);
            
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                
                return true;
            }
            
            PD_PRINT("\t\t\t\t\tw_0 != e_1");
            
            PD_NOTE(MethodName("R_Ia_above")+": Disconnect a Hopf link and some further subdiagram as connected summands. ( crossing_count = " + ToString(pd.crossing_count) + ")");
            
            /*           +---O   O---+             +---O   O---+
             *           |    \ /    |             |    \ /    |
             *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
             *           |    / \    |             |    / \    |
             *           |   O###O   |             |   O###O   |
             *           | e_3  n_3  |             |  e_3  n_3 |
             *           O           O             O           O
             *        -->----------->|-->       -->|---------->--->
             *        c_0|     a     |c_1       c_0|     a     |c_1
             *           |           |             |           |
             *           +-----------+             +-----------+
             */
            
            
            Reconnect(e_3,u_0,n_3);   // This disconnects the knotted part between e_3 and n_2.
            // The knot now resides as a diagram component in the planar diagram so that it can be  split off by a future split pass. The coloring makes it clear that it is not really a split link component, but a connected summand.
            
            // This keeps e_1 alive, which is likely to be visited next.
            Reconnect<Tail>(e_1,w_0); // This disconnects the Hopf link.
            DeactivateArc(a  );
            DeactivateArc(n_1);
            DeactivateArc(s_0);
            DeactivateArc(n_0);
            
            CreateHopfLinkFromArcs(n_0,a,c_0_state);
        
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            DeactivateCrossing(c_3);
            
            return true;

            
            // Old version without disconnecting.
            
            /*           +---O   O---+             +---O   O---+
             *           |    \ /    |             |    \ /    |
             *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
             *           |    / \    |             |    / \    |
             *           |   O###O   |             |   O###O   |
             *           | e_3  n_3  |             |  e_3  n_3 |
             *           O           O             O           O
             *        -->----------->|-->       -->|---------->--->
             *        c_0|     a     |c_1       c_0|     a     |c_1
             *           |           |             |           |
             *           +-----------+             +-----------+
             *
             *      These can be rerouted to the following two situations:
             *           +-------+                 +-------+
             *           |       |                 |       |
             *           |       |                 |       |
             *           | e_3   |n_3              | e_3   |n_3
             *       n_0 |   O###O             n_0 |   O###O
             *           |   |     n_1             |   |     n_1
             *           O   +-------O             O   +-------O
             *        -->----------->|-->       -->|---------->--->
             *        c_0|     a     |c_1       c_0|     a     |c_1
             *           |           |             |           |
             *           +-----------+             +-----------+
             *
             *      No matter how c_3 is handed, we fuse n_0 and n_1 with their opposite arcs across c_3 and deactivate c_3.
             *
             *      This will probably happen seldomly.
             */
//
//            Reconnect(n_0,u_0,n_3);
//            Reconnect(n_1,u_1,e_3);
//            DeactivateCrossing(c_3);
//            
//            // TODO: Implement counters.
////            ++pd.twist_counter;
//            
//            AssertArc<1>(a  );
//            AssertArc<1>(n_0);
//            AssertArc<1>(s_0);
//            AssertArc<1>(w_0);
//            AssertArc<1>(n_1);
//            AssertArc<1>(e_1);
//            AssertArc<1>(s_1);
//            AssertArc<0>(n_3);
//            AssertArc<0>(e_3);
//            AssertArc<1>(w_3); // w_3 == n_1
//            AssertCrossing<1>(c_0);
//            AssertCrossing<1>(c_1);
//            AssertCrossing<0>(c_3);
//            
//            return true;
        }
        
        PD_PRINT("\t\t\tw_3 != n_1");
        
    } // if constexpr( mult_compQ )

    return false;
}
