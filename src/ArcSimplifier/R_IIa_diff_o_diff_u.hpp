bool R_IIa_diff_o_diff_u()
{
    PD_DPRINT( "R_IIa_diff_o_diff_u()" );
    
    PD_DPRINT( "\t\tu_0 != u_1" );
    
    /* Example: o_0, o_2, o_3 are the same; u_0 != u_1; u_1 == 1
     *
     *          w_3 O   O n_3
     *               \ /
     *                / c_3
     *               / \
     *              O   O
     *             /     \
     *        n_0 /       \ n_1
     *           O         O
     *           |    a    ^
     *    w_0 O--|->O-->O---->O e_1
     *           vc_0      |c_1
     *           O         O
     *        s_0 \       / s_1
     *             \     /
     *              O   O
     *               \ /
     *                \ c_2
     *               / \
     *          w_2 O   O s_2
     */
    
    // TODO: Check for four-crossing moves.
    
    // Cannot use Reconnect here because we have to change directions of vertical strands.
    
    A_cross(w_3,!u_1) = c_0;
    A_cross(n_3,!u_0) = c_1;
    A_cross(w_2, u_1) = c_0;
    A_cross(s_2, u_0) = c_1;
    
    C_arcs(c_0, u_0,Right) = C_arcs(c_0, u_0,Left );
    C_arcs(c_0, u_0,Left ) = w_3;
    C_arcs(c_0,!u_0,Left ) = C_arcs(c_0,!u_0,Right);
    C_arcs(c_0,!u_0,Right) = w_2;
    TouchCrossing(c_0);
    
    C_arcs(c_1, u_1,Right) = C_arcs(c_1, u_1,Left );
    C_arcs(c_1, u_1,Left ) = n_3;
    C_arcs(c_1,!u_1,Left ) = C_arcs(c_1,!u_1,Right);
    C_arcs(c_1,!u_1,Right) = s_2;
    TouchCrossing(c_1);
    
    DeactivateArc(n_0);
    DeactivateArc(n_1);
    DeactivateArc(s_0);
    DeactivateArc(s_1);
    DeactivateCrossing(c_2);
    DeactivateCrossing(c_3);
    ++pd.R_IIa_counter;
    
    AssertArc(a);
    AssertArc(w_3);
    AssertArc(n_3);
    AssertArc(w_2);
    AssertArc(s_2);
    AssertArc(w_0);
    AssertArc(e_1);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
    
    return true;
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

