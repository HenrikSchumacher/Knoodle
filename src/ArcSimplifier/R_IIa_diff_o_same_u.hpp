bool R_IIa_diff_o_same_u()
{
    PD_DPRINT( "R_IIa_diff_o_same_u()" );
    
    // TODO: Debug this!
    
    PD_DPRINT( "\t\tu_0 == u_1" );
    
    /* Example: o_0, o_2, o_3 are the same; u_0 == u_1; u_1 == 1
     *
     *          w_3 O   O n_3
     *               \ /
     *                / c_3
     *               / \
     *              O   O
     *             /     \
     *        n_0 /       \ n_1
     *           O         O
     *           ^    a    ^
     *    w_0 O--|->O-->O---->O e_1
     *           |c_0      |c_1
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

    Reconnect(w_3,!u_0,n_0);
    Reconnect(w_2, u_0,s_0);
    Reconnect(n_3,!u_1,n_1);
    Reconnect(s_2, u_1,s_1);
    pd.FlipHandedness(c_0);
    pd.FlipHandedness(c_1);
    DeactivateCrossing(c_2);
    DeactivateCrossing(c_3);
    ++pd.R_IIa_counter;
    
    AssertArc(a);
    AssertArc(w_3);
    AssertArc(n_3);
    AssertArc(w_2);
    AssertArc(s_2);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
    
    return true;
}
