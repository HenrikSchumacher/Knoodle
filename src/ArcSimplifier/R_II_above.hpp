bool R_II_above()
{
    PD_DPRINT( "R_II_above()" );
    
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
        
        load_c_2();
        
        if( e_2 == s_1 )
        {
            PD_DPRINT( "\t\te_2 == s_1" );
            
            // We can make an additional R_I move here.
            
            /*               +---------+             +---------+
             *               |         |             |         |
             *               |    a    |             |    a    |
             *            -->|-------->|-->   or  -->--------->--->
             *               |c_0      |c_1          |c_0      |c_1
             *               O         O             O         O
             *                \       /               \       /
             *                 \     /                 \     /
             *                  O   O                   O   O
             *                   \ /                     \ /
             *                    X c_2                   X c_2
             *                   / \                     / \
             *              w_2 O   O s_2           w_2 O   O s_2
             *
             */

            if constexpr ( mult_compQ )
            {
                if( w_2 == s_2 )
                {
                    PD_DPRINT( "\t\t\tw_2 == s_2" );
                    
                    // These case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
                    PD_ASSERT( w_0 != e_1 );
                    
                    ++pd.unlink_count;
                    Reconnect(w_0,Head,e_1); //... so this is safe.
                    DeactivateArc(a);
                    DeactivateArc(n_0);
                    DeactivateArc(s_0);
                    DeactivateArc(s_1);
                    DeactivateArc(w_2);
                    DeactivateCrossing(c_0);
                    DeactivateCrossing(c_1);
                    DeactivateCrossing(c_2);
                    ++pd.R_II_counter;
                    pd.R_I_counter += 2;
                    
                    AssertArc<0>(a  );
                    AssertArc<0>(n_0);
                    AssertArc<0>(s_0);
                    AssertArc<1>(w_0);
                    AssertArc<0>(n_1);
                    AssertArc<0>(e_1);
                    AssertArc<0>(s_1);
                    AssertArc<0>(e_2);
                    AssertArc<0>(s_2);
                    AssertArc<0>(w_2);
                    AssertCrossing<0>(c_0);
                    AssertCrossing<0>(c_1);
                    AssertCrossing<0>(c_2);
                    
                    return true;
                }
            }
            
            PD_DPRINT( "\t\t\tw_2 != s_2" );
            
            if( w_0 == w_2  )
            {
                PD_DPRINT( "\t\t\t\tw_0 == w_2" );
                
                if constexpr ( mult_compQ )
                {
                    if( e_1 == s_2  )
                    {
                        PD_DPRINT( "\t\t\t\t\te_1 == s_2" );
                        
                        /* Example for o_1 == 1;
                         *
                         *               +---------+
                         *               |         |
                         *        w_0    |    a    |    e_1
                         *        +--O-->|-------->|-->O--+
                         *        |      |c_0      |c_1   |
                         *        |      O         O      |
                         *        |       \       /       |
                         *        |        \     /        |
                         *        |         O   O         |
                         *        |          \ /          |
                         *        |           X c_2       |
                         *        |          / \          |
                         *        +---------O   O---------+
                         *               w_2     s_2
                         */
                        
                        ++pd.unlink_count;
                        DeactivateArc(a);
                        DeactivateArc(n_0);
                        DeactivateArc(s_0);
                        DeactivateArc(s_1);
                        DeactivateArc(w_0);
                        DeactivateArc(e_1);
                        DeactivateCrossing(c_0);
                        DeactivateCrossing(c_1);
                        DeactivateCrossing(c_2);
                        ++pd.R_II_counter;
                        ++pd.R_I_counter;
                        
                        AssertArc<0>(a  );
                        AssertArc<0>(n_0);
                        AssertArc<0>(s_0);
                        AssertArc<0>(w_0);
                        AssertArc<0>(n_1);
                        AssertArc<0>(e_1);
                        AssertArc<0>(s_1);
                        AssertArc<0>(e_2);
                        AssertArc<0>(s_2);
                        AssertArc<0>(w_2);
                        AssertCrossing<0>(c_0);
                        AssertCrossing<0>(c_1);
                        AssertCrossing<0>(c_2);
                        
                        return true;
                    }
                }
                
                PD_DPRINT( "\t\t\t\t\te_1 != s_2" );
                
                /* Example for o_1 == 1;
                 *
                 *               +---------+
                 *               |         |
                 *        w_0    |    a    |    e_1
                 *        +--O-->|-------->|-->O
                 *        |      |c_0      |c_1
                 *        |      O         O
                 *        |       \       /
                 *        |        \     /
                 *        |         O   O
                 *        |          \ /
                 *        |           X c_2
                 *        |          / \
                 *        +---------O   O
                 *               w_2     s_2
                 */
                
                Reconnect(e_1,Tail,s_2);
                DeactivateArc(a);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(s_1);
                DeactivateArc(w_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_2);
                ++pd.R_II_counter;
                ++pd.R_I_counter;
                
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(s_0);
                AssertArc<0>(w_0);
                AssertArc<0>(n_1);
                AssertArc<1>(e_1);
                AssertArc<0>(s_1);
                AssertArc<0>(e_2);
                AssertArc<0>(s_2);
                AssertArc<0>(w_2);
                AssertCrossing<0>(c_0);
                AssertCrossing<0>(c_1);
                AssertCrossing<0>(c_2);
                
                return true;
            }
            
            PD_DPRINT( "\t\t\t\tw_0 != w_2" );
            PD_ASSERT( w_0 != w_2 );
            
            if( e_1 == s_2  )
            {
                PD_DPRINT( "\t\t\t\t\te_1 == s_2" );
                
                /* Example for o_1 == 1;
                 *
                 *               +---------+
                 *               |         |
                 *        w_0    |    a    |    e_1
                 *           O-->|-------->|-->O--+
                 *               |c_0      |c_1   |
                 *               O         O      |
                 *                \       /       |
                 *                 \     /        |
                 *                  O   O         |
                 *                   \ /          |
                 *                    X c_2       |
                 *                   / \          |
                 *                  O   O---------+
                 *               w_2     s_2
                 */
                
                Reconnect(w_0,Head,w_2);
                DeactivateArc(a);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(s_1);
                DeactivateArc(e_1);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_2);
                ++pd.R_II_counter;
                ++pd.R_I_counter;
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(s_0);
                AssertArc<1>(w_0);
                AssertArc<0>(n_1);
                AssertArc<0>(e_1);
                AssertArc<0>(s_1);
                AssertArc<0>(e_2);
                AssertArc<0>(s_2);
                AssertArc<0>(w_2);
                AssertCrossing<0>(c_0);
                AssertCrossing<0>(c_1);
                AssertCrossing<0>(c_2);
                
                return true;
            }
            
            PD_DPRINT( "\t\t\t\t\te_1 != s_2" );
            PD_ASSERT( e_1 != s_2 );
            
            /* Example for o_1 == 1;
             *
             *               +---------+
             *               |         |
             *        w_0    |    a    |    e_1
             *           O-->|-------->|-->O
             *               |c_0      |c_1
             *               O         O
             *                \       /
             *                 \     /
             *                  O   O
             *                   \ /
             *                    X c_2
             *                   / \
             *                  O   O
             *               w_2     s_2
             */
            
            // These case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
            PD_ASSERT( w_0 != e_1 );
            
            Reconnect(w_0,Head,e_1); // ... so this is safe.

            // Assured by the checks above.
            Reconnect(w_2,u_1,s_2);
            
            DeactivateArc(a);
            DeactivateArc(n_0);
            DeactivateArc(s_0);
            DeactivateArc(s_1);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            DeactivateCrossing(c_2);
            
            ++pd.R_II_counter;
            ++pd.R_I_counter;
            
            AssertArc<0>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<1>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<0>(s_1);
            AssertArc<0>(e_2);
            AssertArc<0>(s_2);
            AssertArc<1>(w_2);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            AssertCrossing<0>(c_2);
            
            return true;
        }
        
        if constexpr ( mult_compQ )
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
                 *               s_0                               s_0
                 */
                
                ++pd.unlink_count;
                Reconnect(w_0,Head,e_1);
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                ++pd.R_II_counter;

                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(s_0);
                AssertArc<1>(w_0);
                AssertArc<0>(n_1);
                AssertArc<0>(e_1);
                AssertArc<0>(s_1);
                AssertCrossing<0>(c_0);
                AssertCrossing<0>(c_1);
                
                return true;
            }
        }
        
        PD_DPRINT( "\t\ts_0 != s_1" );
        PD_ASSERT(s_0 != s_1);
        
        /*          +-----------+                     +-----------+
         *          |           |                     |           |
         *          |     a     |                     |     a     |
         *   w_0 -->|---------->|--> e_1  or   w_0 -->----------->---> e_1
         *          |c_0        |c_1                  |c_0        |c_1
         *          |           |                     |           |
         *           s_0         s_1                   s_0         s_1
         */
        
        // The case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
        PD_ASSERT( s_0 != s_1 );
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
        
        AssertArc<0>(a  );
        AssertArc<0>(n_0);
        AssertArc<1>(s_0);
        AssertArc<1>(w_0);
        AssertArc<0>(n_1);
        AssertArc<0>(e_1);
        AssertArc<0>(s_1);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        return true;
        
    } // if( n_0 == n_1 )
    
    return false;
}
