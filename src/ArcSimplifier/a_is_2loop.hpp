bool a_is_2loop()
{
    PD_PRINT("a_is_2loop()");
    
    if(w_0 == e_1)
    {
        PD_PRINT("\tw_0 == e_1");
        
        if(o_0 == o_1)
        {
            PD_PRINT("\t\to_0 == o_1");
            
            /* We have a true over- or underloop
             * Looks like this:
             *
             *       +-------------------+              +-------------------+
             *       |                   |              |                   |
             *       |   O###########O   |              |   O###########O   |
             *       |   |     a     |   |              |   |     a     |   |
             *       +-->|---------->|-->+      or      +-->----------->--->+
             *           |c_0        |c_1                   |c_0        |c_1
             *           |           |                      |           |
             *
             *                 or                                 or
             *
             *           |           |                      |           |
             *           |     a     |                      |     a     |
             *       +-->|---------->|-->+      or      +-->----------->--->+
             *       |   |c_0        |c_1|              |   |c_0        |c_1|
             *       |   O###########O   |              |   O###########O   |
             *       |                   |              |                   |
             *       +-------------------+              +-------------------+
             */
            
            // TODO: It might be possible to detect a connected summand here!
            
            // TODO: Feature of detecting over- or underloops is not tested, yet.
            
            if( n_0 == n_1 )
            {
                PD_PRINT("\t\t\tn_0 == n_1");
                
                if( s_0 == s_1 )
                {
                    PD_PRINT("\t\t\t\ts_0 == s_1");
                    
                    // Two unlinks
                    
                    pd.unlink_count += 2;
                    DeactivateArc(w_0);
                    DeactivateArc(n_0);
                    DeactivateArc(s_0);
                    DeactivateArc(a);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    // TODO: Invent some counter here and increment it.

                    AssertArc<0>(a  );
                    AssertArc<0>(n_0);
                    AssertArc<0>(s_0);
                    AssertArc<0>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<0>(e_1);
                    AssertArc<0>(s_1);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);
                    
                    return true;
                }
                else // n_0 == n_1  and  s_0 != s_1
                {
                    PD_PRINT("\t\t\t\ts_0 != s_1");
                    
                    /*             w_0 = e_1                          w_0 = e_1
                     *       +-------------------+              +-------------------+
                     *       |     n_0 = n_1     |              |     n_0 = n_1     |
                     *       |   O-----------O   |              |   O-----------O   |
                     *       |   |           |   |              |   |           |   |
                     *       +-->|---------->|-->+      or      +-->----------->--->+
                     *           |c_0  a     |c_1                   |c_0  a     |c_1
                     *           |           |                      |           |
                     *       s_0 O           O s_1              s_0 O           O s_1
                     */
                    
                    ++pd.unlink_count;
                    Reconnect(s_0,u_0,s_1);
                    DeactivateArc(n_0);
                    DeactivateArc(w_0);
                    DeactivateArc(a);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    ++pd.R_II_counter;

                    AssertArc<0>(a  );
                    AssertArc<0>(n_0);
                    AssertArc<1>(s_0);
                    AssertArc<0>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<0>(e_1);
                    AssertArc<0>(s_1);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);

                    return true;
                }
            }
            else // ( n_0 != n_1 )
            {
                PD_PRINT("\t\t\tn_0 != n_1");
                
                if( s_0 == s_1 )
                {
                    PD_PRINT("\t\t\t\ts_0 == s_1");
                    
                    // n_0 != n_1  and  s_0 -= s_1
                    
                    /*
                     *       n_0 O           O n_1              n_0 O           O n_1
                     *           |           |                      |           |
                     *           |     a     |                      |     a     |
                     *       +-->|---------->|-->+      or      +-->----------->--->+
                     *       |   |c_0        |c_1|              |   |c_0        |c_1|
                     *       |   O-----------O   |              |   O-----------O   |
                     *       |     s_0 = s_1     |              |     s_0 = s_1     |
                     *       +-------------------+              +-------------------+
                     *             w_0 = e_1                          w_0 = e_1
                     */
                    
                    ++pd.unlink_count;
                    Reconnect(n_0,u_1,n_1);
                    DeactivateArc(w_0);
                    DeactivateArc(s_0);
                    DeactivateArc(a);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    ++pd.R_II_counter;
                    
                    AssertArc<0>(a  );
                    AssertArc<1>(n_0);
                    AssertArc<0>(s_0);
                    AssertArc<0>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<0>(e_1);
                    AssertArc<0>(s_1);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);
                    
                    return true;
                }
                else // n_0 != n_1  and  s_0 != s_1
                {
                    PD_PRINT("\t\t\t\ts_0 != s_1");
                    
                    /*
                     *       n_0 O           O n_1              n_0 O           O n_1
                     *           |           |                      |           |
                     *           |     a     |                      |     a     |
                     *       +-->|---------->|-->+      or      +-->----------->--->+
                     *       |   |c_0        |c_1|              |   |c_0        |c_1|
                     *       |   O###########O   |              |   O###########O   |
                     *       |                   |              |                   |
                     *       +-------------------+              +-------------------+
                     *             w_0 = e_1                          w_0 = e_1
                     */
                    
                    ++pd.unlink_count;
                    Reconnect(s_0,u_0,n_0);
                    Reconnect(s_1,u_1,n_1);
                    DeactivateArc(w_0);
                    DeactivateArc(a);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    // TODO: Invent some counter here and increment it.
                    
                    AssertArc<0>(a  );
                    AssertArc<0>(n_0);
                    AssertArc<1>(s_0);
                    AssertArc<0>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<0>(e_1);
                    AssertArc<1>(s_1);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);
                    
                    return true;
                }
            }
        }

        if constexpr ( optimization_level < 3 )
        {
            return false;
        }
        
        PD_PRINT("\t\to_0 != o_1");
        PD_ASSERT(o_0 != o_1);
        /* Looks like this:
         *
         *       +-------------------+              +-------------------+
         *       |                   |              |                   |
         *       |   O###########O   |              |   O###########O   |
         *       |   |     a     |   |              |   |     a     |   |
         *       +-->|---------->--->+      or      +-->----------->|-->+
         *           |c_0        |c_1                   |c_0        |c_1
         *           |           |                      |           |
         *
         *                 or                                 or
         *
         *           |           |                      |           |
         *           |     a     |                      |     a     |
         *       +-->----------->|-->+      or      +-->|---------->--->+
         *       |   |c_0        |c_1|              |   |c_0        |c_1|
         *       |   O###########O   |              |   O###########O   |
         *       |                   |              |                   |
         *       +-------------------+              +-------------------+
         */
        
        // TODO: If additionally the endpoints of n_0 and n_1 or the endpoints of s_0 and s_1 coincide, there are subtle ways to remove the crossing there:

        /*       +-------------------+              +-------------------+
         *       |                   |              |   +--------------+|
         *       |                   |              |   |              ||
         *       |   O##TANGLE###O   |              |   O##TANGLE###O  ||
         *       |   |           |   |              |              /   ||
         *       |   |           |   |              |   +---------+    ||
         *       |   |     a     |   |              |   |     a     +--+|
         *       +-->|----------->---+     ==>      +-->------------|---+
         *           |c_0        |c_1                   |c_0        |c_1
         *           O           O                      O           O
         *            \         /                        \         /
         *             \       /                          \       /
         *              \     /                            \     /
         *               O   O                              +   +
         *                \ /                               |   |
         *                 \ c_2                            |   |
         *                / \                               |   |
         *               O   O                              O   O
         *
         *       +-------------------+              +-------------------+
         *       |                   |              |                   |
         *       |   O##TANGLE###O   |              |   O##TANGLE###O   |
         *       |   |           |   |              |   |           |   |
         *       |   |     a     |   |   untwist    |   |     a     |   |
         *       +-->------------|---+     ==>      +-->------------|---+
         *           |c_0        |c_1                   |c_0        |c_1
         *           O           O                      O           O
         *            \         /                        \         /
         *             \       /                          \       /
         *              \     /                            \     /
         *               O   O                              +   +
         *                \ /                               |   |
         *                 \ c_2                            |   |
         *                / \                               |   |
         *               O   O                              O   O
         */
        
        // TODO: But this comes at the price of an additional crossing load, and it might not be very likely. We better let
        
//        wprint(ClassName()+"::a_is_2loop: A loop twist move is possible here.");
        
        // TODO: We might be able to detect a Hopf-Link.
        
        return false;
        
    }
    
    return false;
}
