void load_c_0()
{
    /*              n_0
     *               O
     *               |
     *       w_0 O---X-->O a
     *               |c_0
     *               O
     *              s_0
     */
    
    // Whether the vertical strand at c_0 points upwards.
    u_0 = (C_arcs(c_0,Out,Right) == a);
    
    n_0 = C_arcs( c_0, !u_0,  Left  );
    s_0 = C_arcs( c_0,  u_0,  Right );
    w_0 = C_arcs( c_0,  In , !u_0   );
    
    AssertArc(n_0);
    AssertArc(s_0);
    AssertArc(w_0);
    

    PD_VALPRINT("c_0",CrossingString(c_0));
    PD_VALPRINT("n_0",ArcString(n_0));
    PD_VALPRINT("w_0",ArcString(w_0));
    PD_VALPRINT("s_0",ArcString(s_0));
    PD_VALPRINT("u_0",u_0);
    // Remark: We are _not_ loading o_0 here, because this is not needed for the Reidemeister I moves. This may safe a cache miss.
}

void load_c_1()
{
    // Whether the vertical strand at c_1 points upwards.
    u_1 = (C_arcs(c_1,In ,Left ) == a);
    
    n_1 = C_arcs(c_1,!u_1, Left );
    e_1 = C_arcs(c_1, Out, u_1  );
    s_1 = C_arcs(c_1, u_1, Right);
    
    AssertArc(n_1);
    AssertArc(e_1);
    AssertArc(s_1);
    
    PD_VALPRINT("c_1",CrossingString(c_1));
    PD_VALPRINT("n_1",ArcString(n_1));
    PD_VALPRINT("e_1",ArcString(e_1));
    PD_VALPRINT("s_1",ArcString(s_1));
    PD_VALPRINT("u_1",u_1);
    
    // Remark: We are _not_ loading o_1 here, because this is not needed for the Reidemeister I moves. This may safe a cache miss.
}

void load_c_2()
{
    // TODO: Check this thoroughly!
    
    /*          n_0       n_1
     *           O         O
     *           |    a    |
     *    w_0 O--X->O-->O--X->O e_1
     *           |c_0      |c_1
     *           O         O
     *       s_0 |        s_1
     *           |
     *           O
     *           |
     *    w_2 O--X--O e_2
     *           |c_2
     *           O s_2
     */
     
    c_2 = A_cross(s_0,!u_0);
    
    AssertCrossing(c_2);
    
    const bool b_2 = (s_0 == C_arcs(c_2,!u_0,Right));
    
    PD_ASSERT( s_0 == C_arcs(c_2,!u_0,b_2) );
    
    s_2 = C_arcs(c_2, u_0,!b_2); // opposite to n_2 == s_0.
    
    e_2 = C_arcs(c_2, b_2, u_0);
    w_2 = C_arcs(c_2,!b_2,!u_0); // opposite to e_2
    
    // u_2 == u_0
    o_2 = (u_0 == ( C_state[c_2] == CrossingState::LeftHanded ));
    
    AssertArc(e_2);
    AssertArc(s_2);
    AssertArc(w_2);
    
    PD_VALPRINT("c_2",CrossingString(c_2));
    PD_VALPRINT("e_2",ArcString(e_2));
    PD_VALPRINT("s_2",ArcString(s_2));
    PD_VALPRINT("w_2",ArcString(w_2));
    PD_VALPRINT("o_2",o_2);
}

void load_c_3()
{
    // TODO: Check this thoroughly!
    
    /*           O n_3
     *           |
     *    w_3 O--X--O e_3
     *           |c_3
     *           O
     *           |
     *       n_0 |       n_1
     *           O         O
     *           |    a    |
     *    w_0 O--X->O-->O--X->O e_1
     *           |c_0      |c_1
     *           O         O
     *          s_0       s_1
     */
     
    c_3 = A_cross(n_0,u_0);
    
    AssertCrossing(c_3);
    
    const bool b_3 = (n_0 == C_arcs(c_3,u_0,Right));
    
    PD_ASSERT( n_0 == C_arcs(c_3,u_0,b_3) );
    
    n_3 = C_arcs(c_3,!u_0,!b_3); // opposite to s_3 == n_0.
    
    e_3 = C_arcs(c_3,!b_3, u_0);
    w_3 = C_arcs(c_3, b_3,!u_0); // opposite to e_3

    // u_3 == u_0
    o_3 = (u_0 == ( C_state[c_3] == CrossingState::LeftHanded ));
    
    AssertArc(e_3);
    AssertArc(n_3);
    AssertArc(w_3);
    
    
    PD_VALPRINT("c_3",CrossingString(c_3));
    PD_VALPRINT("e_3",ArcString(e_3));
    PD_VALPRINT("n_3",ArcString(n_3));
    PD_VALPRINT("w_3",ArcString(w_3));
    PD_VALPRINT("o_3",o_3);
}
