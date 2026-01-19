bool four_pattern_same_u()
{
    PD_DPRINT( "four_pattern_same_u()" );
    
    // We need the crossings to be pairwise distinct here.
    PD_ASSERT(c_0 != c_1);
    PD_ASSERT(c_0 != c_2);
    PD_ASSERT(c_0 != c_3);
    PD_ASSERT(c_1 != c_2);
    PD_ASSERT(c_1 != c_3);
    PD_ASSERT(c_2 != c_3);
    
    if( w_3 == s_2 )
    {
        PD_PRINT("\t\t\tw_3 == s_2");
        
        PD_NOTE(MethodName("four_pattern_same_u")+": We could disconnect up to two disconnected summands here. ( crossing_count = " + ToString(pd.crossing_count) + ")");
        
        /*              +<-------------+
         *              |  n_3         ^
         *          w_3 O   O########  |
         *               \ /        #  |
         *                X c_3     #  |
         *               / \        #  |
         *              O   O       #  |
         *             /     \      #  |
         *        n_0 /       \ n_1 #  |
         *           O         O    #  |
         *    w_0    |    a    | e_1#  |
         *     ###O--X->O-->O--X->O##  |
         *     #     vc_0      vc_1    |
         *     #     O         O       |
         *     #  s_0 \       / s_1    |
         *     #       \     /         |
         *     #        O   O          |
         *     #         \ /           |
         *     #          X c_2        |
         *     #         / \           |
         *     #########O   O--------->+
         *          w_2       s_2
         */

        const bool bottom_left_trivialQ = (w_0 == w_2);
        const bool top_right_trivialQ   = (n_3 == e_1);
        
        // Needed below; otherwise we might get conflicts in the following if's.
        PD_ASSERT(w_0 != e_1);
        PD_ASSERT(w_0 != n_3);
        PD_ASSERT(w_2 != e_1);
        PD_ASSERT(w_2 != n_3);
        
        if( bottom_left_trivialQ )
        {
            DeactivateArc(w_2);
        }
        else
        {
            Reconnect<Tail>(w_2,w_0);   // Disconnect the bottom-left subdiagram
        }
        
        if( top_right_trivialQ )
        {
            DeactivateArc(e_1);
        }
        else
        {
            // This keeps e_1 alive, which is likely to be visited next.
            Reconnect<Tail>(e_1,n_3);   // Disconnect the top-right subdiagram
        }
        
        DeactivateArc(a  );
        DeactivateArc(n_0);
        DeactivateArc(s_0);
        DeactivateArc(s_2);
        DeactivateArc(n_1);
        DeactivateArc(s_1);
        
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        DeactivateCrossing(c_2);
        DeactivateCrossing(c_3);
        
        // I checked the following in the Mathematica notebook Dev_FourPattern.dev with the old implementation of PlanarDiagram.
        
        bool trefoilQ       =  (c_0_state == c_1_state)
                            && (c_0_state == c_2_state)
                            && (c_0_state == c_3_state);
        bool figure_eightQ  =  (c_0_state != c_1_state)
                            && (c_0_state == c_2_state)
                            && (c_0_state != c_2_state);
        
        const int nontrivial_count = !bottom_left_trivialQ + !top_right_trivialQ;
        
        if( trefoilQ )
        {
            // The remainder is a trefoil.
            CreateTrefoilKnotFromArc(a,c_0_state);
            
            switch( nontrivial_count )
            {
                case 0:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Identified as trefoil knot.");
                    return true;
                }
                case 1:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a trefoil knot.");
                    return true;
                    
                }
                case 2:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a trefoil knot and a further diagram.");
                    return true;
                }
            }
        }
        else if( figure_eightQ )
        {
            // The remainder is a figure-eight knot.
            CreateFigureEightKnotFromArc(a);
            PD_NOTE(MethodName("four_pattern_same_u")+": Found a figure-eight knot.");
            
            switch( nontrivial_count )
            {
                case 0:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Identified as figure-eight knot.");
                    return true;
                }
                case 1:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a figure-eight knot.");
                    return true;
                    
                }
                case 2:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a figure-eight knot and a further diagram.");
                    return true;
                }
            }
        }
        else
        {
            switch( nontrivial_count )
            {
                case 0:
                {
                    CreateUnlinkFromArc(a);
                    PD_ASSERT(pd.InvalidQ());
                    PD_NOTE(MethodName("four_pattern_same_u")+": Identified as unlink.");
                    return true;
                }
                case 1:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Removed four crossings.");
                    return true;
                    
                }
                case 2:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a connected summand.");
                    return true;
                }
            }
        }
        
        return true;
    }
    
    PD_PRINT("\t\t\tw_3 != s_2");
    
    if( w_2 == n_3 )
    {
        PD_PRINT("\t\t\t\tw_2 == n_3");
        
        PD_PRINT(MethodName("four_pattern_same_u")+": We could disconnect up to two disconnected summands here. ( crossing_count = " + ToString(pd.crossing_count) + ")");
        
        /*   +<-------------+
         *   |          w_3 |
         *   | #########O   O n_3
         *   | #         \ /
         *   | #          X c_3
         *   | #         / \
         *   | #        O   O
         *   | #       /     \
         *   | #  n_0 /       \ n_1
         *   | #     O         O
         *   | #     ^    a    ^ e_1
         *   | ###O--X->O-->O--X->O###
         *   |w_0    |c_0      |c_1  #
         *   |       O         O     #
         *   |    s_0 \       / s_1  #
         *   |         \     /       #
         *   |          O   O        #
         *   |           \ /         #
         *   |            X c_2      #
         *   v           / \         #
         *   +--------->O   O#########
         *          w_2       s_2
         */
        
        const bool top_left_trivialQ     = (w_0 == w_3);
        const bool bottom_right_trivialQ = (s_2 == e_1);
        
        // Needed below; otherwise we might get conflicts in the following if's.
        PD_ASSERT(w_0 != e_1);
        PD_ASSERT(w_0 != s_2);
        PD_ASSERT(w_3 != e_1);
        PD_ASSERT(w_3 != s_2);
        
        if( top_left_trivialQ )
        {
            DeactivateArc(w_3);         // This deactivates also w_0 == w_3.
        }
        else
        {
            Reconnect<Tail>(w_3,w_0);   // Disconnect the top-left subdiagram
        }
        
        if( bottom_right_trivialQ )
        {
            DeactivateArc(e_1);         // This deactivates also s_2 == e_1.
        }
        else
        {
            // This keeps e_1 alive, which is likely to be visited next.
            Reconnect<Tail>(e_1,s_2);   // Disconnect the bottom-right subdiagram
        }

        DeactivateArc(a  );
        DeactivateArc(s_0);
        DeactivateArc(n_0);
        DeactivateArc(n_3);
        DeactivateArc(s_1);
        DeactivateArc(n_1);
        
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        DeactivateCrossing(c_2);
        DeactivateCrossing(c_3);
        
        // I checked the following in the Mathematica notebook Dev_FourPattern.dev with the old implementation of PlanarDiagram.
        bool trefoilQ       =  (c_0_state == c_1_state)
                            && (c_0_state == c_2_state)
                            && (c_0_state == c_3_state);
        bool figure_eightQ  =  (c_0_state != c_1_state)
                            && (c_0_state != c_2_state)
                            && (c_0_state == c_2_state);

        const int nontrivial_count = !top_left_trivialQ + !bottom_right_trivialQ;
        
        if( trefoilQ )
        {
            // The remainder is a trefoil.
            CreateTrefoilKnotFromArc(a,c_0_state);
            
            switch( nontrivial_count )
            {
                case 0:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Identified as trefoil knot.");
                    return true;
                }
                case 1:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a trefoil knot.");
                    return true;
                    
                }
                case 2:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a trefoil knot and a further diagram.");
                    return true;
                }
            }
        }
        else if( figure_eightQ )
        {
            // The remainder is a figure-eight knot.
            CreateFigureEightKnotFromArc(a);
            PD_NOTE(MethodName("four_pattern_same_u")+": Found a figure-eight knot.");
            
            switch( nontrivial_count )
            {
                case 0:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Identified as figure-eight knot.");
                    return true;
                }
                case 1:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a figure-eight knot.");
                    return true;
                    
                }
                case 2:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a figure-eight knot and a further diagram.");
                    return true;
                }
            }
        }
        else
        {
            switch( nontrivial_count )
            {
                case 0:
                {
                    CreateUnlinkFromArc(a);
                    PD_ASSERT(pd.InvalidQ());
                    PD_NOTE(MethodName("four_pattern_same_u")+": Identified as unlink.");
                    return true;
                }
                case 1:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Removed four crossings.");
                    return true;
                    
                }
                case 2:
                {
                    PD_NOTE(MethodName("four_pattern_same_u")+": Disconnected a connected summand.");
                    return true;
                }
            }
        }

        // We should never get here.
        PD_ASSERT(false);
        return true;
    }

    PD_PRINT("\t\t\tw_2 != n_3");
    
    return false;
}



/*
 *                 n_3
 *          w_3 O   O########
 *               \ /        #
 *                X c_3     #
 *               / \        #
 *              O   O       #
 *             /     \      #
 *        n_0 /       \ n_1 #
 *           O         O    #
 *    w_0    |    a    ^ e_1#
 *        O--X->O-->O--X->O##
 *           vc_0      |c_1
 *           O         O
 *        s_0 \       / s_1
 *             \     /
 *              O   O
 *               \ /
 *                X c_2
 *               / \
 *              O   O
 *          w_2       s_2
 */
