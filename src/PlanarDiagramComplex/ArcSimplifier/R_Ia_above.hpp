bool R_Ia_above()
{
    PD_DPRINT("R_Ia_above()");
    
 
    //Check for Reidemeister Ia move.
    if(s_0 != s_1)
    {
        return false;
    }
    
    
    PD_PRINT("\ts_0 == s_1");
    
    /*       |     a     |
     *    -->----------->|-->
     *       |c_0        |c_1
     *       |           |
     *       +-----------+
     */
    
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
                    
                    /*  R_I move.
                     *            w_3                       w_3
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
                    
                    // TODO: Disconnect Hopf link.
                    PD_PRINT(MethodName("R_Ia_above")+": Detected a Hopf link as connected summand.");
                    
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
                    
                    wprint(MethodName("R_Ia_above")+": Seldom case: detected an unlink (three crossings deleted.");
                    
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
                
                wprint(MethodName("R_Ia_above")+": Seldom case: three crossings deleted.");
                
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
                
                Reconnect<Head>(n_3,e_1);
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
                
                wprint(MethodName("R_Ia_above")+": Seldom case: three crossings deleted.");
                
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
//                
//                // BLAME!!!
//                
//                logvalprint("w_0",ArcString(w_0));
//                logvalprint("n_0",ArcString(n_0));
//                logvalprint("a  ",ArcString(a));
//                logvalprint("s_0",ArcString(s_0));
//                
//                logvalprint("n_1",ArcString(n_1));
//                logvalprint("e_1",ArcString(e_1));
//                logvalprint("s_1",ArcString(s_1)); // s_1 == s_0
//                
//                logvalprint("w_3",ArcString(w_3)); // w_2 == n_1
//                logvalprint("n_3",ArcString(n_3)); // n_3 == e_1
//                logvalprint("e_3",ArcString(e_3));
                
                Reconnect<Head>(w_0,w_3);
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
            
            /* Potential trefoil cases:
             *
             *       w_3 O   O n_3                w_3 O   O n_3
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
                if( n_3 == e_1 )
                {
                    PD_PRINT(MethodName("R_Ia_above")+": Detected a trefoil link component.");
                }
                else
                {
                    PD_PRINT(MethodName("R_Ia_above")+": Detected a trefoil connected summand.");
                }
            }
            else if( n_3 == e_1 )
            {
                PD_PRINT(MethodName("R_Ia_above")+": Detected a trefoil connected summand.");
            }
            
            return false;
        }
        
    } // if( e_3 == n_1 )
    
    if constexpr( mult_compQ )
    {
        // The following can only happen with more than one component.
        if(w_3 == n_1)
        {
            PD_PRINT("\t\tw_3 == n_1");
            
            if( e_3 == n_3 )
            {
                PD_PRINT("\t\t\te_3 == n_3");
                
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
                
                // TODO: We could disconnect-sum a Hopf link here.
                PD_PRINT(MethodName("R_Ia_above")+": Detected a Hopf link as connected summand.");
                
                Reconnect(n_1,u_1,n_0);
                DeactivateArc(e_3);
                DeactivateCrossing(c_3);
                
                // TODO: Implement counters.
//                ++pd.R_I_counter;
                
                AssertArc<1>(a  );
                AssertArc<0>(n_0);
                AssertArc<1>(s_0);
                AssertArc<1>(w_0);
                AssertArc<1>(n_1);
                AssertArc<1>(e_1);
                AssertArc<1>(s_1);
                AssertArc<0>(n_3);
                AssertArc<0>(e_3);
                AssertArc<0>(w_3);
                AssertCrossing<1>(c_0);
                AssertCrossing<1>(c_1);
                AssertCrossing<0>(c_3);
                
                return true;
            }
            
            PD_PRINT("\t\t\te_3 != n_3");
            
            PD_PRINT(MethodName("R_Ia_above")+": Detected a Hopf link and a knot as connected summands.");
            
            /* Two further interesting cases.
             *
             *           +---O   O---+             +---O   O---+
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
            
            Reconnect(n_0,u_0,n_3);
            Reconnect(n_1,u_1,e_3);
            DeactivateCrossing(c_3);
            
            // TODO: Implement counters.
//            ++pd.twist_counter;
            
            AssertArc<1>(a  );
            AssertArc<1>(n_0);
            AssertArc<1>(s_0);
            AssertArc<1>(w_0);
            AssertArc<1>(n_1);
            AssertArc<1>(e_1);
            AssertArc<1>(s_1);
            AssertArc<0>(n_3);
            AssertArc<0>(e_3);
            AssertArc<1>(w_3); // w_3 == n_1
            AssertCrossing<1>(c_0);
            AssertCrossing<1>(c_1);
            AssertCrossing<0>(c_3);
            
            return true;
        }
        
    } // if constexpr( mult_compQ )

    return false;
}
