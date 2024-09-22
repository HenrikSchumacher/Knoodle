void R_IIa_same_o_diff_u()
{
    PD_DPRINT( "\tR_IIa_same_o_diff_u()" );
    
    PD_DPRINT( "\t\tu_0 != u_1" );
    PD_ASSERT( u_0 != u_1 );
    
    //    if( (w_0 == w_3) || (w_0 == w_2) || (e_1 == s_2) || (e_1 == n_3) )
    //    {
    //        print("R_IIa_same_o_opposite_u: We could make an additional R_I move here.");
    //    }
    
    // This move is very "rich".
    // If we want to consider all possible ways to chain ot with Reidemeister I moves,
    // then this will be about 16(!) cases.
    

    // This are think we checked before by a_is_2loop.
    PD_ASSERT( w_0 != e_1 );
    
    // This cannot happen because one vertical strand goes up and the other goes down.
    PD_ASSERT( w_2 != n_3 );
    PD_ASSERT( s_2 != w_3 );
    
    // Cannot use Reconnect here because we have to change directions of vertical strands.
    
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
    pd.FlipHandedness(c_0);
    TouchCrossing(c_0);
    
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
    pd.FlipHandedness(c_1);
    TouchCrossing(c_1);
    
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
