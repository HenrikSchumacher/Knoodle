bool strands_same_o()
{
    PD_DPRINT( "strands_same_o()" );
    
    /*               |     a     |             |     a     |
     *            -->|---------->|-->   or  -->----------->--->
     *               |c_0        |c_1          |c_0        |c_1
     */
    
    if( R_II_above() )
    {
        return true;
    }
    
    if( R_II_below() )
    {
        return true;
    }
    
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
    
    
    if( R_IIa_same_o() )
    {
        return true;
    }
    
    return false;
}



bool R_IIa_same_o()
{
    PD_DPRINT( "R_IIa_same_o()" );

    if( ! ((e_2 == s_1) && (e_3 == n_1) && (o_2 == o_3)) )
    {
        return false;
    }
    
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

    
    if( u_0 == u_1 )
    {
        R_IIa_same_o_same_u();
    }
    else
    {
        R_IIa_same_o_opposite_u();
    }

    return true;
}


void R_IIa_same_o_same_u()
{
    PD_DPRINT( "\tR_IIa_same_o_same_u()" );
    
    PD_DPRINT( "\t\tu_0 == u_1" );
    PD_ASSERT( u_0 == u_1 );
    
    
    if( s_2 == w_3 )
    {
        PD_DPRINT( "\t\t\ts_2 == w_3" );
        
/*
*          +--------------+                                                +--------------+
*          |              |                                                |              |
*          |  n_3         |                    n_3                         |  n_3         |
*      w_3 O   O########  |                 +---O########                  +---O########  |
*           \ /        #  |                  \          #                              #  |
*            / c_3     #  |                   \         #                              #  |
*           / \        #  |                    \        #                              #  |
*          O   O       #  |                     \       #                              #  |
*     n_0 /     \ n_1  #  |                      \ n_1  #                              #  |
*        /       \     #  |                       \     #                              #  |
*       O         O    #  |                        O    #                              #  |
*   w_0 |    a    |    #  |          w_0           |    #           w_0                #  |
*  ##O--|->O---O--|->O##  |   ==>   ##O------------|->O##    ==>   ##O-------------->O##  |
*  #    |c_0   c_1| e_1   |         #           c_1| e_1           #                e_1   |
*  #    O         O       |         #              O               #                      |
*  #     \       /        |         #             /                #                      |
*  #  s_0 \     / s_1     |         #            /                 #                      |
*  #       O   O          |         #           /                  #                      |
*  #        \ /           |         #          /                   #                      |
*  #         \ c_2        |         #         /                    #                      |
*  #        / \           |         #        /                     #                      |
*  ########O   O----------+         ########O                      ########O<-------------+
*       w_2     s_2                      w_2                              w_2
*/
        
        // Check for four-crossing move.
        if( w_0 == w_2 )
        {
            PD_DPRINT( "\t\t\tw_0 == w_2" );
            if( e_1 == n_3 )
            {
                PD_DPRINT( "\t\t\t\te_1 == n_3" );
                
                /*
                *            +--------------+
                *            |              |
                *            |  n_3         |
                *            +---O-------+  |
                *                        |  |
                *                        |  |
                *                        |  |
                *                        |  |
                *                        v  |
                *                        |  |
                *                        |  |
                *     w_0                |  |
                *    +-O-------------->O-+  |
                *    |                e_1   |
                *    |                      |
                *    |                      |
                *    ^                      |
                *    |                      |
                *    |                      |
                *    |                      |
                *    |                      |
                *    +-------O<-------------+
                *           w_2
                */
                
                ++pd.unlink_count;
                DeactivateArc(a);
                DeactivateArc(n_0);
                DeactivateArc(n_1);
                DeactivateArc(s_0);
                DeactivateArc(s_1);
                DeactivateArc(e_1); // e_1 == n_3;
                DeactivateArc(w_0); // w_2 == w_0;
                DeactivateArc(w_3); // s_2 == w_3;
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                DeactivateCrossing(c_2);
                DeactivateCrossing(c_3);
                ++pd.R_IIa_counter;
                pd.R_I_counter += 2;
                
                return;
            }
            
            PD_DPRINT( "\t\t\t\te_1 != n_3" );
            PD_ASSERT( e_1 != n_3 );
            
            /*
            *            +--------------+
            *            |              |
            *            |  n_3         |
            *            +---O########  |
            *                        #  |
            *                        #  |
            *                        #  |
            *                        #  |
            *                        #  |
            *                        #  |
            *                        #  |
            *     w_0                #  |
            *    +-O-------------->O##  |
            *    |                e_1   |
            *    |                      |
            *    |                      |
            *    ^                      |
            *    |                      |
            *    |                      |
            *    |                      |
            *    |                      |
            *    +-------O<-------------+
            *           w_2
            */
            
            Reconnect(e_1,Tail,n_3);
            DeactivateArc(a);
            DeactivateArc(n_0);
            DeactivateArc(n_1);
            DeactivateArc(s_0);
            DeactivateArc(s_1);            DeactivateArc(n_0);
            DeactivateArc(n_1);
            DeactivateArc(s_0);
            DeactivateArc(s_1);
            DeactivateArc(w_0); // w_2 == w_0;
            DeactivateArc(w_3); // s_2 == w_3;
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            DeactivateCrossing(c_2);
            DeactivateCrossing(c_3);
            ++pd.R_IIa_counter;
            ++pd.R_I_counter;
            
            AssertArc(e_1);
            
            return;
        }
        
        PD_DPRINT( "\t\t\tw_0 != w_2" );
        PD_ASSERT( w_0 != w_2 );
        
        if( e_1 == n_3 )
        {
            PD_DPRINT( "\t\t\t\te_1 == n_3" );
            
            /*
            *            +--------------+
            *            |              |
            *            |  n_3         |
            *            +---O-------+  |
            *                        |  |
            *                        |  |
            *                        |  |
            *                        |  |
            *                        v  |
            *                        |  |
            *                        |  |
            *     w_0                |  |
            *    ##O-------------->O-+  |
            *    #                e_1   |
            *    #                      |
            *    #                      |
            *    #                      |
            *    #                      |
            *    #                      |
            *    #                      |
            *    #                      |
            *    ########O<-------------+
            *           w_2
            */
            
            Reconnect(w_0,Head,w_2);
            
            DeactivateArc(a);
            DeactivateArc(n_0);
            DeactivateArc(n_1);
            DeactivateArc(s_0);
            DeactivateArc(s_1);
            DeactivateArc(n_3); // e_1 == n_3
            DeactivateArc(w_3); // s_2 == w_3;
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            DeactivateCrossing(c_2);
            DeactivateCrossing(c_3);
            ++pd.R_IIa_counter;
            ++pd.R_I_counter;
            
            AssertArc(w_0);
            
            return;
        }
        
        PD_DPRINT( "\t\t\t\te_1 != n_3" );
        
        /*
        *            +--------------+
        *            |              |
        *            |  n_3         |
        *            +---O########  |
        *                        #  |
        *                        #  |
        *                        #  |
        *                        #  |
        *                        #  |
        *                        #  |
        *                        #  |
        *     w_0                #  |
        *    ##O-------------->O##  |
        *    #                e_1   |
        *    #                      |
        *    #                      |
        *    #                      |
        *    #                      |
        *    #                      |
        *    #                      |
        *    #                      |
        *    ########O<-------------+
        *           w_2
        */
        
        Reconnect(w_0,Head,e_1);
        Reconnect(n_3,Head,w_2);
        
        DeactivateArc(a);
        DeactivateArc(n_0);
        DeactivateArc(n_1);
        DeactivateArc(s_0);
        DeactivateArc(s_1);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        DeactivateCrossing(c_2);
        DeactivateCrossing(c_3);
        ++pd.R_IIa_counter;
        ++pd.twist_counter;
        
        AssertArc(w_0);
        AssertArc(n_3);
        
        return;
        
    } // if( s_2 == w_3 )
    
    
    if( w_2 == n_3 )
    {
        PD_DPRINT( "\t\t\tw_2 == n_3" );
        
        // TODO: Do the four-crossing move.
        
        print("R_IIa_same_o_same_u: We could do a four-crossing move here.");
    }
    
//        PD_DPRINT( "\t\t\t(s_2 != w_3) && (w_2 != n_3)" );
//        PD_ASSERT( (s_2 != w_3) && (w_2 != n_3) );
    
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
    
    Reconnect(w_3,!u_0,n_0);
    Reconnect(w_2, u_0,s_0);
    Reconnect(n_3,!u_1,n_1);
    Reconnect(s_2, u_1,s_1);
    DeactivateCrossing(c_2);
    DeactivateCrossing(c_3);
    ++pd.R_IIa_counter;
    
    AssertArc(a  );
    AssertArc(w_3);
    AssertArc(w_2);
    AssertArc(n_3);
    AssertArc(s_2);
    AssertArc(w_0);
    AssertArc(e_1);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
}


void R_IIa_same_o_opposite_u()
{
    PD_DPRINT( "\tR_IIa_same_o_opposite_u()" );
    
    PD_DPRINT( "\t\tu_0 != u_1" );
    PD_ASSERT( u_0 != u_1 );
    

    if( (w_0 == w_3) || (w_0 == w_2) || (e_1 == s_2) || (e_1 == n_3) )
    {
        print("R_IIa_same_o_opposite_u: We could make an additional R_I move here.");
    }
    
    PD_ASSERT( w_0 != e_1 )
    PD_ASSERT( w_2 != n_3 )
    PD_ASSERT( s_2 != w_3 )
    
    
    /*       w_3     n_3                   w_3     n_3
     *          O   O                         O   O
     *           \ /                          |   |
     *            X c_3                       |   |
     *           / \                          |   |
     *          O   O                         +   +
     *     n_0 /     \ n_1                   /     \
     *        /       \                     /       \
     *       O         O                   O         O
     *       |    a    |                   |    a    |
     *    O--X->O---O--X->O     ==>     O--X->O---O--X->O
     *       |c_0   c_1|                   |c_0   c_1|
     *       O         O                   O         O
     *        \       /                     \       /
     *     s_0 \     / s_1                   \     /
     *          O   O                         +   +
     *           \ /                          |   |
     *            X c_2                       |   |
     *           / \                          |   |
     *          O   O                         O   O
     *       w_2     s_2                   w_2     s_2
     */
    
    // Cannot use Reconnect here because of direction changes.
    
    A_cross(w_3,!u_1) = c_0;
    A_cross(w_2, u_1) = c_0;
    A_cross(n_3,!u_0) = c_1;
    A_cross(s_2, u_0) = c_1;
    
    /* By example u_0 = 0
     *
     *          O                   O
     *          |                   ^
     *          |                   |
     *          |                   |
     *    O-----X---->O  ==>  O-----X---->O
     *          |c_0                |c_0
     *          |                   |
     *          v                   |
     *          O                   O
     */
    
    C_arcs(c_0, u_0,Right) = C_arcs(c_0, u_0,Left );
    C_arcs(c_0, u_0,Left ) = w_3;
    
    C_arcs(c_0,!u_0,Left ) = C_arcs(c_0,!u_0,Right);
    C_arcs(c_0,!u_0,Right) = w_2;
    
    /* By example u_1 = 1
     *
     *          O                   O
     *          ^                   |
     *          |                   |
     *          |                   |
     *    O-----X---->O  ==>  O-----X---->O
     *          |c_1                |c_1
     *          |                   |
     *          |                   v
     *          O                   O
     */
    
    C_arcs(c_1, u_1,Right) = C_arcs(c_1, u_1,Left );
    C_arcs(c_1, u_1,Left ) = n_3;
    
    C_arcs(c_1,!u_1,Left ) = C_arcs(c_1,!u_1,Right);
    C_arcs(c_1,!u_1,Right) = s_2;
    
    pd.FlipHandedness(c_0);
    pd.FlipHandedness(c_1);
    
    DeactivateArc(s_0);
    DeactivateArc(s_1);
    DeactivateArc(n_0);
    DeactivateArc(n_1);
    DeactivateCrossing(c_2);
    DeactivateCrossing(c_3);
    ++pd.R_IIa_counter;
    
    AssertArc(w_3);
    AssertArc(w_2);
    AssertArc(n_3);
    AssertArc(s_2);
    AssertArc(w_0);
    AssertArc(e_1);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
}
