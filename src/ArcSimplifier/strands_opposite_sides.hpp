bool strands_opposite_sides()
{
    PD_DPRINT("strands_opposite_sides()");
    
    /*       |     a     |             |     a     |
     *    -->|---------->--->   or  -->----------->|-->
     *       |c_0        |c_1          |c_0        |c_1
     */
    
    //Check for Reidemeister Ia move.
    if( n_0 == n_1 )
    {
        PD_DPRINT( "\tn_0 == n_1" );
        
        /*       +-----------+             +-----------+
         *       |     a     |             |     a     |
         *    -->----------->|-->   or  -->|---------->--->
         *       |c_0        |c_1          |c_0        |c_1
         *       |           |             |           |
         */
        
        load_c_2();
                            
        if( e_2 == s_1 )
        {
            PD_DPRINT( "\t\te_2 == s_1" );
            
            if( o_0 == o_2 )
            {
                PD_DPRINT( "\t\to_0 == o_2" );
                
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
                 *               O   O s_2                 O   O s_2
                 */
                
                // TODO: Do the R_Ia surgery.
                // I skip this for now, because it does not have _that_ much impact.
                
//                DeactivateCrossing(c_2);
//                ++R_Ia_counter;
//                return true;
            }
            else
            {
                PD_DPRINT( "\t\to_0 != o_2" );
                
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
                 */
                
                if( (w_0 == w_2) || (e_1 == s_2) )
                {
                    PD_DPRINT( "\t\t\t(w_0 == w_2) || (e_1 == s_2)" );
                    
                    // TODO: trefoil as connect summand detected
                    // How to store this info?
                }
            }
            
            return false;
        }
        
        // This can only happen, if the planar diagram has multiple components!
        if constexpr( mult_compQ )
        {
            if( w_2 == s_1 )
            {
                PD_DPRINT( "\t\tw_2 == s_1" );
                
                /* Two further interesting cases.
                 *
                 *           +-----------+             +-----------+
                 *           |           |             |           |
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *        -->----------->|-->       -->|---------->--->
                 *           O   O###O   O             O   O###O   O
                 *           |    \ /    |             |    \ /    |
                 *       s_0 |     X c_2 | s_1         |     X c_2 | s_1
                 *           |    / \    |             |    / \    |
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
                ++pd.twist_counter;
                
                AssertArc(a);
                AssertArc(n_0);
                AssertArc(n_1);
                AssertArc(s_0);
                AssertArc(s_1);
                AssertArc(w_0);
                AssertArc(e_1);
                AssertCrossing(c_0);
                AssertCrossing(c_1);
                
                return true;
            }
        }

        return false;
    }
    
    //Check for Reidemeister Ia move.
    if( s_0 == s_1 )
    {
        PD_DPRINT( "\ts_0 == s_1" );
        
        /*       |     a     |
         *    -->----------->|-->
         *       |c_0        |c_1
         *       |           |
         *       +-----------+
         */
        
        load_c_3();
        
        if( e_3 == n_1 )
        {
            PD_DPRINT( "\t\te_3 == n_1" );
            
            if( o_0 == o_3 )
            {
                PD_DPRINT( "\t\t\to_0 == o_3" );
                
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
                
                // TODO: Do the R_Ia surgery.
                // I skip this for now, because it does not have _that_ much impact.
                
//                DeactivateCrossing(c_3);
//                ++R_Ia_counter;
//
//                return true;
                
                return false;
            }
            else
            {
                PD_DPRINT( "\t\t\to_0 != o_3" );
                
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
                 
                if( (w_0 == w_3) || (n_3 == e_1) )
                {
                    // TODO: trefoil as connect summand detected
                    // How to store this info?
                    
                    return false;
                }
                
            }
            
        } // if( e_3 == n_1 )
        
        // This can only happen, if the planar diagram has multiple components!
        if constexpr( mult_compQ )
        {
            if( w_3 == n_1 )
            {
                PD_DPRINT( "\t\tw_3 == n_1" );
                
                /* Two further interesting cases.
                 *
                 *           +---O   O---+             +---O   O---+
                 *           |    \ /    |             |    \ /    |
                 *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
                 *           |    / \    |             |    / \    |
                 *           |   O###O   |             |   O###O   |
                 *           |           |             |           |
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
                 *           |       |                 |       |
                 *       n_0 |   O###O             n_0 |   O###O
                 *           |   |     n_1             |   |     n_1
                 *           O   +-------O             O   +-------O
                 *        -->----------->|-->       -->|---------->--->
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *           |           |             |           |
                 *           +-----------+             +-----------+
                 *
                 *      No matter how c_3 is handed, we fuse n_0 and n_1 with their opposite arcs accross c_3 and deactivate c_3.
                 *
                 *      This will probably happen seldomly.
                 */
                
                Reconnect(n_0,u_0,e_3);
                Reconnect(n_1,u_1,w_3);
                DeactivateCrossing(c_2);
                ++pd.twist_counter;
                
                AssertArc(a);
                AssertArc(n_0);
                AssertArc(n_1);
                AssertArc(s_0);
                AssertArc(s_1);
                AssertArc(w_0);
                AssertArc(e_1);
                AssertCrossing(c_0);
                AssertCrossing(c_1);
                
                return true;
            }
            
        } // if constexpr( mult_compQ )

        return false;
    }

    load_c_2();
    load_c_3();
    
    //Check for Reidemeister IIa move.
    if( (e_3 == n_1) && (e_2 == s_1) && (o_2 == o_3) && ( (o_2 == o_0) || (o_2 != o_1) ) )
    {
        PD_DPRINT( "\t(e_3 == n_1) && (e_2 == s_1) && (o_2 == o_3) && ( (o_2 == o_0) || (o_2 != o_1) )" );
        
        /*          w_3 O   O n_3             w_3 O   O n_3
         *               \ /                       \ /
         *                / c_3                     \ c_3
         *               / \                       / \
         *              O   O                     O   O
         *             /     \                   /     \
         *        n_0 /       \ n_1         n_0 /       \ n_1
         *           O         O               O         O
         *           |    a    |               |    a    |
         *        O--|->O-->O---->O         O---->O-->O--|->O
         *           |c_0      |c_1            |c_0      |c_1
         *           O         O               O         O
         *        s_0 \       / s_1         s_0 \       / s_1
         *             \     /                   \     /
         *              O   O                     O   O
         *               \ /                       \ /
         *                \ c_2                     / c_2
         *               / \                       / \
         *          w_2 O   O s_2             w_2 O   O s_2
         */
         
        // Reidemeister IIa move
        
        if( u_0 == u_1 )
        {
            Reconnect(s_0,!u_0,w_2);
            Reconnect(n_0, u_0,w_3);
            Reconnect(s_1,!u_1,s_2);
            Reconnect(n_1, u_1,n_3);
            DeactivateCrossing(c_2);
            DeactivateCrossing(c_3);
            ++pd.R_IIa_counter;
            
            AssertArc(a);
            AssertArc(s_0);
            AssertArc(n_0);
            AssertArc(s_1);
            AssertArc(n_1);
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

/*
 *              +--------------+
 *              |  n_3         |
 *          w_3 O   O########  |
 *               \ /        #  |
 *                / c_3     #  |
 *               / \        #  |
 *              O   O       #  |
 *             /     \      #  |
 *        n_0 /       \ n_1 #  |
 *           O         O    #  |
 *           |    a    | e_1#  |
 *    w_0 O--|->O-->O---->O##  |
 *           |c_0      |c_1    |
 *           O         O       |
 *        s_0 \       / s_1    |
 *             \     /         |
 *              O   O          |
 *               \ /           |
 *                / c_2        |
 *               / \           |
 *          w_2 O   O----------+
 *                   s_2
 */

//            ||
//            \/
            
/*
 *                  +---------+
 *                  |         |
 *                  O######## |
 *                 n_3      # |
 *                          # |
 *                          # |
 *                          # |
 *                          # |
 *                          # |
 *                          # |
 *                       e_1# |
 *    w_0 O-------------->O## |
 *                  +---------+
 *                 /   +-------+
 *                /   / s_1    |
 *               /   /         |
 *              O   O          |
 *               \ /           |
 *                / c_2        |
 *               / \           |
 *          w_2 O   O----------+
 *                   s_2
 */

//            ||
//            \/
            
/*
 *                  +---------+
 *                  |         |
 *                  O######## |
 *                 n_3      # |
 *                          # |
 *                          # |
 *                          # |
 *                          # |
 *                          # |
 *                          # |
 *                       e_1# |
 *    w_0 O-------------->O## |
 *                  +---------+
 *                 /
 *                /
 *               /
 *              +
 *              |
 *              |
 *              |
 *          w_2 O
 *
 */

//################################


/*
 *              +--------------+
 *              |  n_3         |
 *          w_3 O   O########  |
 *               \ /        #  |
 *                / c_3     #  |
 *               / \        #  |
 *              O   O       #  |
 *             /     \      #  |
 *        n_0 /       \ n_1 #  |
 *           O         O    #  |
 *           |    a    | e_1#  |x
 *    w_0 O---->O-->O--|->O##  |
 *           |c_0      |c_1    |
 *           O         O       |
 *        s_0 \       / s_1    |
 *             \     /         |
 *              O   O          |
 *               \ /           |
 *                / c_2        |
 *               / \           |
 *          w_2 O   O----------+
 *                   s_2
 */

//            ||
//            \/

/*
 *
 *
 *                  O########
 *                  |       #
 *                  |       #
 *                  |       #
 *                  +       #
 *                   \      #
 *                    \ n_1 #
 *                     O    #
 *                     | e_1#
 *    w_0 O------------|->O##
 *                     |c_1
 *                     O
 *                    / s_1
 *                   /
 *                  O
 *                 /
 *                / c_2
 *               /
 *          w_2 O
 *
 */

//            ||
//            \/

/*
 *                  +----------+
 *                  |          |
 *                  O########  |
 *                 n_1      #  |
 *                          #  |
 *                          #  |
 *                          #  |
 *                          #  |
 *                          #  |
 *                          #  |
 *                       e_1#  |
 *    w_0 O-------------->O##  |
 *                             |
 *                             |
 *                             |
 *                             |
 *                             |
 *                             |
 *                             |
 *                             |
 *          w_2 O--------------+
 *
 */

