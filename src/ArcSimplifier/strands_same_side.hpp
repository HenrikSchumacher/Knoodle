bool strands_same_side()
{
    PD_DPRINT( "strands_same_side()" );
    
    /*               |     a     |             |     a     |
     *            -->|---------->|-->   or  -->----------->--->
     *               |c_0        |c_1          |c_0        |c_1
     */
    
    // Check for Reidemeister II move.
    if( n_0 == n_1 )
    {
        PD_DPRINT( "\tn_0 == n_1" );
        
        /*               +-----------+             +-----------+
         *               |           |             |           |
         *               |     a     |             |     a     |
         *            -->|---------->|-->   or  -->----------->--->
         *               |c_0        |c_1          |c_0        |c_1
         */
        
        // TODO: If the endpoints of s_0 and s_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
        // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
        // TODO: On the other hand: we need to reference to the two end point anyways to do the reconnecting right...

        if constexpr( mult_compQ )
        {
            if( s_0 == s_1 )
            {
                PD_DPRINT( "\t\ts_0 == s_1" );
                
                /*               n_0                               n_0
                 *          +-----------+                     +-----------+
                 *          |           |                     |           |
                 *          |     a     |                     |     a     |
                 *   w_0 -->|---------->|--> e_1  or   w_0 -->----------->---> e_1
                 *          |c_0        |c_1                  |c_0        |c_1
                 *          |           |                     |           |
                 *          +-----------+                     +-----------+
                 *               s_0         s_1                   s_0
                 */
                
                ++pd.unlink_count;
                Reconnect(w_0,Head,e_1);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                ++pd.R_II_counter;

                AssertArc(w_0);
                AssertArc(s_0);
                
                return true;
            }
        }
        PD_DPRINT( "\t\ts_0 != s_1" );
        
        /*          +-----------+                     +-----------+
         *          |           |                     |           |
         *          |     a     |                     |     a     |
         *   w_0 -->|---------->|--> e_1  or   w_0 -->----------->---> e_1
         *          |c_0        |c_1                  |c_0        |c_1
         *          |           |                     |           |
         *           s_0         s_1                   s_0         s_1
         */
        
        // These case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
        PD_ASSERT( s_0 != s_1 )
        PD_ASSERT( w_0 != e_1 );
        PD_ASSERT( w_0 != s_0 );
        PD_ASSERT( e_1 != s_1 );
        
        // .. so this is safe:
        Reconnect(w_0,Head,e_1);
        Reconnect(s_0,u_0 ,s_1);
        DeactivateArc(n_0);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++pd.R_II_counter;
        
        AssertArc(w_0);
        AssertArc(s_0);
        
        return true;
        
    } // if( n_0 == n_1 )
    
    // Check for Reidemeister II move.
    if( s_0 == s_1 )
    {
        PD_DPRINT( "\ts_0 == s_1" );
        
        // TODO: If the endpoints of n_0 and n_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
        // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
        // TODO: On the other hand: we need to reference to the two end point anyways to do the reconnecting right...
        
        /*                n_0         n_1                    n_0         n_1
         *               |     a     |                      |     a     |
         *        w_0 -->|---------->|--> e_1   or   w_0 -->----------->---> e_1
         *               |c_0        |c_1                   |c_0        |c_1
         *               |           |                      |           |
         *               +-----------+                      +-----------+
         *                    s_0                                s_0
         */
        
        // These case w_0 == e_1, w_0 == n_0, e_1 == n_1 are ruled out already...
        PD_ASSERT( w_0 != e_1 );
        PD_ASSERT( w_0 != n_0 );
        PD_ASSERT( e_1 != n_1 );
        
        // .. so this is safe:
        Reconnect(w_0,Head,e_1);
        
        // We have already checked this above.
        PD_ASSERT( n_0 != n_1 );
        Reconnect(n_0,!u_0,n_1);
        DeactivateArc(s_0);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++pd.R_II_counter;
        
        AssertArc(n_0);
        AssertArc(w_0);
        
        return true;
    }
    
    
    // Try Reidemeister IIa move.
    
    // TODO: If n_0 == n_1 and if the endpoints of s_0 and s_1 coincide, we can remove 3 crossings.
    // TODO: Price: Load 1 additional crossing for test; load 2 more crossings if successful
    
    // TODO: If s_0 == s_1 and if the endpoints of n_0 and n_1 coincide, we can remove 3 crossings.
    // TODO: Price: Load 1 additional crossing for test; load 2 more crossings if successful
    
    PD_ASSERT(s_0 != s_1);
    PD_ASSERT(n_0 != n_1);
    PD_ASSERT(w_0 != e_1);
    
    load_c_2();
    load_c_3();
    
    /*       w_3     n_3             w_3     n_3
     *          O   O                   O   O
     *           \ /                     \ /
     *            X c_3                   X c_3
     *           / \                     / \
     *          O   O                   O   O e_3
     *     n_0 /     e_3           n_0 /
     *        /                       /
     *       O         O n_1         O         O n_1
     *       |    a    |             |    a    |
     *    O--|->O---O--|->O   or  O---->O---O---->O
     *       |c_0   c_1|             |c_0   c_1|
     *       O         O s_1         O         O s_1
     *        \                       \
     *     s_0 \                   s_0 \
     *          O   O e_2               O   O e_2
     *           \ /                     \ /
     *            X c_2                   X c_2
     *           / \                     / \
     *          O   O                   O   O
     *       w_2     s_2             w_2     s_2
     */
    
    //Check for Reidemeister IIa move.
    if( (e_2 == s_1) && (e_3 == n_1) && (o_2 == o_3) )
    {
        PD_DPRINT( "\t(e_2 == s_1) && (e_3 == n_1) && (o_2 == o_3)" );
        
        /*       w_3     n_3             w_3     n_3
         *          O   O                   O   O
         *           \ /                     \ /
         *            / c_3                   / c_3
         *           / \                     / \
         *          O   O                   O   O
         *     n_0 /     \ n_1         n_0 /     \ n_1
         *        /       \               /       \
         *       O         O             O         O
         *       |    a    |             |    a    |
         *    O--|->O---O--|->O   or  O---->O---O---->O
         *       |c_0   c_1|             |c_0   c_1|
         *       O         O             O         O
         *        \       /               \       /
         *     s_0 \     / s_1         s_0 \     / s_1
         *          O   O                   O   O
         *           \ /                     \ /
         *            \ c_2                   \ c_2
         *           / \                     / \
         *          O   O                   O   O
         *       w_2     s_2             w_2     s_2
         *
         *            or                      or
         *
         *       w_3     n_3             w_3     n_3
         *          O   O                   O   O
         *           \ /                     \ /
         *            \ c_3                   \ c_3
         *           / \                     / \
         *          O   O                   O   O
         *     n_0 /     \ n_1         n_0 /     \ n_1
         *        /       \               /       \
         *       O         O             O         O
         *       |    a    |             |    a    |
         *    O--|->O---O--|->O   or  O---->O---O---->O
         *       |c_0   c_1|             |c_0   c_1|
         *       O         O             O         O
         *        \       /               \       /
         *     s_0 \     / s_1         s_0 \     / s_1
         *          O   O                   O   O
         *           \ /                     \ /
         *            / c_2                   / c_2
         *           / \                     / \
         *          O   O                   O   O
         *       w_2     s_2             w_2     s_2
         */
        
        // Reidemeister IIa move


        if( u_0 == u_1 )
        {
            PD_DPRINT( "\t\tu_0 == u_1" );
            AssertArc(a  );
            AssertArc(n_0);
            AssertArc(s_0);
            AssertArc(n_1);
            AssertArc(s_1);
            AssertArc(w_0);
            AssertArc(e_1);
            
            AssertArc(w_3);
            AssertArc(w_2);
            AssertArc(n_3);
            AssertArc(s_2);
            
            PD_ASSERT( n_0 != w_3 );
            PD_ASSERT( s_0 != w_2 );
            PD_ASSERT( n_1 != n_3 );
            PD_ASSERT( s_1 != s_2 );
            
            PD_DPRINT("A");
            Reconnect(n_0, u_0,w_3);
            PD_DPRINT("B");
            Reconnect(s_0,!u_0,w_2);
            PD_DPRINT("C");
            Reconnect(n_1, u_1,n_3);
            PD_DPRINT("D");
            Reconnect(s_1,!u_1,s_2);
            PD_DPRINT("E");
            DeactivateCrossing(c_2);
            DeactivateCrossing(c_3);
            ++pd.R_IIa_counter;
            PD_DPRINT("F");
            AssertArc(a  );
            AssertArc(n_0);
            AssertArc(s_0);
            AssertArc(n_1);
            AssertArc(s_1);
            AssertArc(w_0);
            AssertArc(e_1);
            AssertCrossing(c_0);
            AssertCrossing(c_1);
            
            return true;
        }
        else
        {
            
            // TODO: Here we have to reverse the vertical strands in c_0 and c_1.
            return false;
        }
    }
    
    return false;
}
