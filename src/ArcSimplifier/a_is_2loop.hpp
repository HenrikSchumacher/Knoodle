bool a_is_2loop()
{
    PD_PRINT( "a_is_2loop()" );
    
    if( w_0 == e_1 )
    {
        PD_PRINT( "\tw_0 == e_1" );
        
        if( o_0 == o_1 )
        {
            PD_PRINT( "\t\to_0 == o_1" );
            
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
            
            wprint("Feature of detecting over- or underloops is not tested, yet.");
            
            ++pd.unlink_count;
            Reconnect(s_0,u_0,n_0);
            Reconnect(s_1,u_1,n_1);
            DeactivateArc(w_0);
            DeactivateArc(e_1);
            DeactivateArc(a);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            // TODO: Invent some counter here and increment it.

            AssertArc<1>(a  );
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

        if constexpr ( optimization_level < 3 )
        {
            return false;
        }
        
        PD_PRINT( "\t\to_0 != o_1" );
        PD_ASSERT( o_0 != o_1 );
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
         *                 \ c_2                            |   |2
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
         *                 \ c_2                            |   |2
         *                / \                               |   |
         *               O   O                              O   O
         */
        
        // TODO: But this comes at the price of an additional crossing load, and it might not be very likely. Also, I am working only on connected diagrams, so I have no test code for this.
        
        return false;
        
    }
    
    return false;
}
