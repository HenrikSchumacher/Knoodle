bool strands_same_o()
{
    PD_DPRINT( "strands_same_o()" );
    
    /*        |     a     |             |     a     |
     *     -->|---------->|-->   or  -->----------->--->
     *        |c_0        |c_1          |c_0        |c_1
     */
    
    if( R_II_above() )
    {
        return true;
    }
    
    if( R_II_below() )
    {
        return true;
    }
    
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
    
    // One of these will definitely work:
    
    if( u_0 == u_1 )
    {
        R_IIa_same_o_same_u();
    }
    else
    {
        R_IIa_same_o_diff_u();
    }
    
    return true;
}
