bool R_II_below()
{
    PD_DPRINT("R_II_below()");
    
    if( s_0 != s_1 )
    {
        return false;
    }
    
    PD_DPRINT("\ts_0 == s_1");
    
    // TODO: If the endpoints of n_0 and n_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
    // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
    // TODO: On the other hand: we need to reference to the two end points anyways to do the reconnecting right...
    
    /*                n_0         n_1                    n_0         n_1
     *               |     a     |                      |     a     |
     *        w_0 -->|---------->|--> e_1   or   w_0 -->----------->---> e_1
     *               |c_0        |c_1                   |c_0        |c_1
     *               |           |                      |           |
     *               +-----------+                      +-----------+
     *                    s_0                                s_0
     */

    // Guaranteed by calling R_II_above first.
    PD_ASSERT(n_0 != n_1);
    
    if constexpr ( optimization_level >= 3 )
    {
        load_c_3();
        
        if( e_3 == n_1 )
        {
            PD_DPRINT("\t\te_3 == n_1");
            
            // We can make an additional R_I move here.
            
            /*              w_3 O   O n_3           w_3 O   O n_3
             *                   \ /                     \ /
             *                    X c_3                   X c_3
             *                   / \                     / \
             *                  O   O                   O   O
             *                 /     \                 /     \
             *                /       \               /       \
             *               O         O             O         O
             *               |         |             |         |
             *               |    a    |             |    a    |
             *        w_0 -->|-------->|-->   or  -->--------->---> e_1
             *               |c_0      |c_1          |c_0      |c_1
             *               +---------+             +---------+
             */
            
            if constexpr ( mult_compQ )
            {
                if( w_3 == n_3 )
                {
                    PD_DPRINT("\t\t\tw_3 == n_3");
                    
                    // These case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
                    PD_ASSERT(w_0 != e_1);
                    PD_ASSERT(w_0 != s_0);
                    PD_ASSERT(e_1 != s_1);
                    
                    AssertCrossing<1>(c_0);
                    AssertCrossing<1>(c_1);
                    AssertArc<1>(w_0);
                    AssertArc<1>(e_1);
                    
                    //... so this is safe.
                    // This keeps e_1 alive, which is likely to be visited next.
                    Reconnect<Tail>(e_1,w_0);
                    DeactivateArc(a);
                    DeactivateArc(n_0);
                    DeactivateArc(s_0);
                    DeactivateArc(n_1);
                    DeactivateArc(w_3);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_3);
                    CreateUnlinkFromArc(w_3);
                    
                    // TODO: Implement counters.
//                    ++pd.R_II_counter;
//                    pd.R_I_counter += 2;
                    
                    AssertArc<0>(a  );
                    AssertArc<0>(n_0);
                    AssertArc<0>(s_0);
                    AssertArc<0>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<1>(e_1);
                    AssertArc<0>(s_1);
                    AssertArc<0>(n_3);
                    AssertArc<0>(e_3);
                    AssertArc<0>(w_3);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);
                    AssertCrossing<0>(c_3);
                    
                    return true;
                }
            }
            
            PD_DPRINT("\t\t\tw_3 != n_3");
            
            /*              w_3 O   O n_3           w_3 O   O n_3
             *                   \ /                     \ /
             *                    X c_3                   X c_3
             *                   / \                     / \
             *                  O   O                   O   O
             *                 /     \                 /     \
             *                /       \               /       \
             *               O         O             O         O
             *               |         |             |         |
             *               |    a    |             |    a    |
             *        w_0 -->|-------->|-->   or  -->--------->---> e_1
             *               |c_0      |c_1          |c_0      |c_1
             *               +---------+             +---------+
             */
            
            if(w_0 == w_3)
            {
                PD_DPRINT("\t\t\t\tw_0 == w_3");
                
                if(e_1 == n_3)
                {
                    PD_DPRINT("\t\t\t\t\te_1 == n_3");
                    
                    /*               w_3     n_3
                     *        +---------O   O---------+
                     *        |          \ /          |
                     *        |           X c_3       |
                     *        |          / \          |
                     *        |         O   O         |
                     *        |        /     \        |
                     *        |       /       \       |
                     *        |      O         O      |
                     *        |      |         |      |
                     *        |      |c_0   c_1|      |
                     *        +--O-->|-------->|-->O--+
                     *        w_0    |    a    |    e_1
                     *               +---------+
                     */
                    
                    DeactivateArc(a);
                    DeactivateArc(n_0);
                    DeactivateArc(n_1);
                    DeactivateArc(s_0);
                    DeactivateArc(w_0);
                    DeactivateArc(e_1);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_3);
                    CreateUnlinkFromArc(a);
                    
                    // TODO: Implement counters.
//                    ++pd.R_II_counter;
//                    ++pd.R_I_counter;
                    
                    AssertArc<0>(a  );
                    AssertArc<0>(n_0);
                    AssertArc<0>(s_0);
                    AssertArc<0>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<0>(e_1);
                    AssertArc<0>(s_1);
                    AssertArc<0>(n_3);
                    AssertArc<0>(e_3);
                    AssertArc<0>(w_3);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);
                    AssertCrossing<0>(c_3);
                    
                    return true;
                }
                
                PD_DPRINT("\t\t\t\t\te_1 != n_3");
                PD_ASSERT(e_1 != n_3);
                
                /*               w_3     n_3
                 *        +---------O   O
                 *        |          \ /
                 *        |           X c_3
                 *        |          / \
                 *        |         O   O
                 *        |        /     \
                 *        |       /       \
                 *        |      O         O
                 *        |      |         |
                 *        |      |c_0   c_1|
                 *        +--O-->|-------->|-->O
                 *        w_0    |    a    |    e_1
                 *               +---------+
                 */
                
                // This keeps e_1 alive, which is likely to be visited next.
                Reconnect<Tail>(e_1,n_3);
                DeactivateArc(a);
                DeactivateArc(n_0);
                DeactivateArc(n_1);
                DeactivateArc(s_0);
                DeactivateArc(w_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                
                // TODO: Implement counters.
//                ++pd.R_II_counter;
//                ++pd.R_I_counter;
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(s_0);
                AssertArc<0>(w_0);
                AssertArc<0>(n_1);
                AssertArc<1>(e_1);
                AssertArc<0>(s_1);
                AssertArc<0>(n_3);
                AssertArc<0>(e_3);
                AssertArc<0>(w_3);
                AssertCrossing<0>(c_0);
                AssertCrossing<0>(c_1);
                AssertCrossing<0>(c_3);
                
                return true;
            }
            
            PD_DPRINT("\t\t\t\tw_0 != w_3");
            PD_ASSERT(w_0 != w_3);
            
            if(e_1 == n_3)
            {
                PD_DPRINT("\t\t\t\t\te_1 == n_3");
                
                /*               w_3     n_3
                 *                  O   O---------+
                 *                   \ /          |
                 *                    X c_3       |
                 *                   / \          |
                 *                  O   O         |
                 *                 /     \        |
                 *                /       \       |
                 *               O         O      |
                 *               |         |      |
                 *               |c_0   c_1|      |
                 *           O-->|-------->|-->O--+
                 *        w_0    |    a    |    e_1
                 *               +---------+
                 */
                
                // This keeps e_1 alive, which is likely to be visited next.
                Reconnect<Tail>(w_3,w_0);
                DeactivateArc(a);
                DeactivateArc(n_0);
                DeactivateArc(n_1);
                DeactivateArc(s_0);
                DeactivateArc(e_1);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_3);
                
                // TODO: Implement counters.
//                ++pd.R_II_counter;
//                ++pd.R_I_counter;
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(s_0);
                AssertArc<0>(w_0);
                AssertArc<0>(n_1);
                AssertArc<0>(e_1);
                AssertArc<0>(s_1);
                AssertArc<0>(n_3);
                AssertArc<0>(e_3);
                AssertArc<1>(w_3);
                AssertCrossing<0>(c_0);
                AssertCrossing<0>(c_1);
                AssertCrossing<0>(c_3);
                
                return true;
            }
            
            PD_DPRINT("\t\t\t\t\te_1 != n_3");
            PD_ASSERT(e_1 != n_3);
            
            /*               w_3     n_3
             *                  O   O
             *                   \ /
             *                    X c_3
             *                   / \
             *                  O   O
             *                 /     \
             *                /       \
             *               O         O
             *               |         |
             *               |c_0   c_1|
             *           O-->|-------->|-->O
             *        w_0    |    a    |    e_1
             *               +---------+
             */
            
            // The case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
            PD_ASSERT(w_0 != e_1);
            PD_ASSERT(w_0 != s_0);
            PD_ASSERT(e_1 != s_1);
            
            // ... so this is safe.
            // This keeps e_1 alive, which is likely to be visited next.
            Reconnect<Tail>(e_1,w_0);
            Reconnect(w_3,u_0,n_3); // The ifs above make this safe..
            
            DeactivateArc(a);
            DeactivateArc(n_0);
            DeactivateArc(n_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            DeactivateCrossing(c_3);
            
            // TODO: Implement counters.
//            ++pd.R_II_counter;
//            ++pd.R_I_counter;
            
            AssertArc<0>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<0>(w_0);
            AssertArc<0>(n_1);
            AssertArc<1>(e_1);
            AssertArc<0>(s_1);
            AssertArc<0>(n_3);
            AssertArc<0>(e_3);
            AssertArc<1>(w_3);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            AssertCrossing<0>(c_3);
            
            return true;
        }
        
        
        PD_DPRINT("\t\te_3 != n_1");
    }
    
    PD_DPRINT("The actual R_II move.");
    
    
    // This should be guaranteed by calling R_II_above first.
    PD_ASSERT(n_0 != n_1);
    
    // This should be ruled out by calling a_is_2loop first or by mult_compQ == false.
    PD_ASSERT(w_0 != e_1);
    
    // The cases w_0 == n_0 and e_1 == n_1 are ruled out already by calling R_I_left and R_I_right first.
    
    /*                n_0         n_1                    n_0         n_1
     *               |     a     |                      |     a     |
     *        w_0 -->|---------->|--> e_1   or   w_0 -->----------->---> e_1
     *               |c_0        |c_1                   |c_0        |c_1
     *               |           |                      |           |
     *               +-----------+                      +-----------+
     *                    s_0                                s_0
     */
    
    PD_ASSERT(w_0 != n_0);
    PD_ASSERT(e_1 != n_1);
    // .. so this is safe:
    // This keeps e_1 alive, which is likely to be visited next.
    Reconnect<Tail>(e_1,w_0);
    
    Reconnect(n_0,u_1,n_1);
    DeactivateArc(s_0);
    DeactivateArc(a);
    DeactivateCrossing(c_0);
    DeactivateCrossing(c_1);
    
    // TODO: Implement counters.
//    ++pd.R_II_counter;
    
    AssertArc<0>(a  );
    AssertArc<1>(n_0);
    AssertArc<0>(s_0);
    AssertArc<0>(w_0);
    AssertArc<0>(n_1);
    AssertArc<1>(e_1);
    AssertArc<0>(s_1);
    AssertCrossing<0>(c_0);
    AssertCrossing<0>(c_1);
    
    PD_DPRINT("R_II_below() done.");
    
    return true;
}
