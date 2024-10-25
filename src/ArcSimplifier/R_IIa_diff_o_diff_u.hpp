bool R_IIa_diff_o_diff_u()
{
    PD_PRINT("R_IIa_diff_o_diff_u()");
    
    PD_PRINT("\t\tu_0 != u_1");
    PD_ASSERT(u_0 != u_1);
    
    AssertArc<1>(a  );
    AssertArc<1>(n_0);
    AssertArc<1>(s_0);
    AssertArc<1>(w_0);
    AssertArc<1>(n_1);
    AssertArc<1>(e_1);
    AssertArc<1>(s_1);
    AssertArc<1>(e_2);
    AssertArc<1>(s_2);
    AssertArc<1>(w_2);
    AssertArc<1>(n_3);
    AssertArc<1>(e_3);
    AssertArc<1>(w_3);
    AssertCrossing<1>(c_0);
    AssertCrossing<1>(c_1);
    AssertCrossing<1>(c_2);
    AssertCrossing<1>(c_3);
    
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
    
    A_cross(n_3,!u_0) = c_1;
    A_cross(s_2, u_0) = c_1;
    A_cross(w_3,!u_1) = c_0;
    A_cross(w_2, u_1) = c_0;

    C_arcs(c_0, u_0,Right) = C_arcs(c_0, u_0,Left );
    C_arcs(c_0, u_0,Left ) = w_3;
    C_arcs(c_0,!u_0,Left ) = C_arcs(c_0,!u_0,Right);
    C_arcs(c_0,!u_0,Right) = w_2;
    
    C_arcs(c_1, u_1,Right) = C_arcs(c_1, u_1,Left );
    C_arcs(c_1, u_1,Left ) = n_3;
    C_arcs(c_1,!u_1,Left ) = C_arcs(c_1,!u_1,Right);
    C_arcs(c_1,!u_1,Right) = s_2;

    if constexpr ( use_flagsQ )
    {
        TouchCrossing(c_0);
        TouchCrossing(c_1);
        
        TouchCrossing(A_cross(a,Tail));
        TouchCrossing(A_cross(a,Head));
        
        TouchCrossing(A_cross(w_0,Tail));
        TouchCrossing(A_cross(w_0,Head));
        
        TouchCrossing(A_cross(e_1,Tail));
        TouchCrossing(A_cross(e_1,Head));
        
        TouchCrossing(A_cross(s_2,Tail));
        TouchCrossing(A_cross(s_2,Head));
        
        TouchCrossing(A_cross(w_2,Tail));
        TouchCrossing(A_cross(w_2,Head));
        
        TouchCrossing(A_cross(n_3,Tail));
        TouchCrossing(A_cross(n_3,Head));
        
        TouchCrossing(A_cross(w_3,Tail));
        TouchCrossing(A_cross(w_3,Head));
    }
    
    DeactivateArc(n_0);
    DeactivateArc(n_1);
    DeactivateArc(s_0);
    DeactivateArc(s_1);
    DeactivateCrossing(c_2);
    DeactivateCrossing(c_3);
    ++pd.R_IIa_counter;
    
    AssertArc<1>(a  );
    AssertArc<0>(n_0);
    AssertArc<0>(s_0);
    AssertArc<1>(w_0);
    AssertArc<0>(n_1);
    AssertArc<1>(e_1);
    AssertArc<0>(s_1);
    AssertArc<0>(e_2);
    AssertArc<1>(s_2);
    AssertArc<1>(w_2);
    AssertArc<1>(n_3);
    AssertArc<0>(e_3);
    AssertArc<1>(w_3);
    AssertCrossing<1>(c_0);
    AssertCrossing<1>(c_1);
    AssertCrossing<0>(c_2);
    AssertCrossing<0>(c_3);
    
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

