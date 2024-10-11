bool strands_diff_o()
{
    PD_PRINT("strands_diff_o()");
    
    /*       |     a     |             |     a     |
     *    -->|---------->--->   or  -->----------->|-->
     *       |c_0        |c_1          |c_0        |c_1
     */

    if constexpr( tested_crossing_count < 3 )
    {
        return false;
    }
    
    if( R_Ia_below() )
    {
        return true;
    }
    
    if( R_Ia_above() )
    {
        return true;
    }

    if constexpr( tested_crossing_count < 4 )
    {
        return false;
    }
    
    load_c_2();
    load_c_3();
    
    //Check for Reidemeister IIa move.
    if(
        (e_3 == n_1) && (e_2 == s_1) && (o_2 == o_3) && ( (o_2 == o_0) || (o_2 != o_1) )
    )
    {
        /* Example: o_0, o_2, o_3 are the same
         *
         *          w_3 O   O n_3
         *               \ /
         *                / c_3
         *               / \
         *              O   O
         *             /     \
         *        n_0 /       \ n_1
         *           O         O
         *           |    a    |
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
        
        if( u_0 == u_1 )
        {
            return R_IIa_diff_o_same_u();
        }
        else
        {
            return R_IIa_diff_o_diff_u();
        }
    }

    return false;
}
