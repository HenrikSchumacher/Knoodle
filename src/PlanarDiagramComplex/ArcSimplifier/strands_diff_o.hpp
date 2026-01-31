bool strands_diff_o()
{
    PD_DPRINT("strands_diff_o()");
    
    /*       |     a     |             |     a     |
     *    -->|---------->--->   or  -->----------->|-->
     *       |c_0        |c_1          |c_0        |c_1
     */

    if constexpr( optimization_level < Int(3) )
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

    if constexpr( optimization_level < Int(4) )
    {
        return false;
    }
    
    load_c_2();
    load_c_3();
    
    
    //Check for 4-crossing patterns
    if( (e_3 == n_1) && (e_2 == s_1) )
    {
        /*          w_3 O   O n_3
         *               \ /
         *                X c_3
         *               / \
         *              O   O
         *             /     \
         *        n_0 /       \ n_1
         *           O         O
         *           |    a    |
         *    w_0 O--X->O-->O--X->O e_1
         *           |c_0      |c_1
         *           O         O
         *        s_0 \       / s_1
         *             \     /
         *              O   O
         *               \ /
         *                X c_2
         *               / \
         *          w_2 O   O s_2
         */
        
        PD_ASSERT(w_0 != e_1) // We checked that in a_is_2loop
        
//        PD_PRINT("\t\tw_3 != s_2");
        
        // TODO: I think this is too restrictive.
        //Check for Reidemeister IIa move.
        // We have o_0 != o_1, so the following implies o_2 != o_1 and o_3 != o_1.
        if( (o_2 == o_3) && (o_2 == o_0) )
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
            
            if(u_0 == u_1)
            {
                if constexpr ( search_two_triangles_same_u )
                {
                    if( two_triangles_same_u() )
                    {
                        return true;
                    }
                }
                
                return R_IIa_diff_o_same_u();
            }
            else
            {
                return R_IIa_diff_o_diff_u();
            }
        }
    }

    return false;
}




/* Impossible for RIIa when o_1 != o_1 and o_2 == o_3: o_2 == o_1
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
 *    w_0 O---->O-->O--|->O e_1
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

/* Impossible for RIIa when o_1 != o_1:
 *
 *          w_3 O   O n_3
 *               \ /
 *                \ c_3
 *               / \
 *              O   O
 *             /     \
 *        n_0 /       \ n_1
 *           O         O
 *           |    a    |
 *    w_0 O--X->O-->O--X->O e_1
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
