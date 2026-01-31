bool twist_at_a()
{
    PD_DPRINT( "twist_at_a()" );
    
    AssertArc<1>(a  );
    AssertArc<1>(n_0);
    AssertArc<1>(s_0);
    AssertArc<1>(w_0);
    AssertArc<1>(n_1);
    AssertArc<1>(e_1);
    AssertArc<1>(s_1);
    AssertCrossing<1>(c_0);
    AssertCrossing<1>(c_1);
    
    // Check for twist move. If successful, a, c_0, and c_1 are deleted.
    if( n_0 == s_1 )
    {
        PD_PRINT( "\tn_0 == s_1" );
        
        if( w_0 == s_0 )
        {
            PD_PRINT( "\t\tw_0 == s_0" );
            
           /*               +-------------------------+
            *               |                         |
            *               |            n_1          |
            *               O             O####       |
            *               |      a      |   #       |
            *       w_0 O---X-->O---->O---X-->O e_1   |
            *           |   |c_0          |c_1        |
            *           +---O             O           |
            *              s_0            |           |
            *                             +-----------+
            */
            
            if( n_1 != e_1 )
            {
                PD_PRINT( "\t\t\tn_1 != e_1" );
                
                ReconnectAsSuggestedByMode<Tail>(e_1,n_1);
                DeactivateArc(w_0);
                DeactivateArc(a );
                DeactivateArc(n_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
//                ++pd.twist_counter;   // TODO: Implement counters.
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(s_0);
                AssertArc<0>(w_0);
                AssertArc<0>(n_1);
                AssertArc<1>(e_1);
                AssertArc<0>(s_1);
                AssertCrossing<0>(c_0);
                AssertCrossing<0>(c_1);
                
                return true;
            }
            
            PD_PRINT( "\t\t\tn_1 == e_1" );
            PD_ASSERT( n_1 == e_1 );
            
            /*               +-------------------------+
             *               |                         |
             *               |            n_1          |
             *               O             O---+       |
             *               |      a      |   |       |
             *       w_0 O---X-->O---->O---X-->O e_1   |
             *           |   |c_0          |c_1        |
             *           +---O             O           |
             *              s_0            |           |
             *                             +-----------+
             */
            
            DeactivateArc(w_0);
            DeactivateArc(e_1);
            DeactivateArc(a );
            DeactivateArc(n_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            CreateUnlinkFromArc(a);
//            pd.R_I_counter += 2; // TODO: Implement counters.
            
            AssertArc<0>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<0>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<0>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            
            return true;
        }
        
        PD_PRINT( "\t\tw_0 != s_0" );
        
        if( n_1 == e_1 )
        {
            PD_PRINT( "\t\t\tn_1 == e_1" );
            
            PD_ASSERT( w_0 != s_0 );
            
           /*               +-------------------------+
            *               |                         |
            *               |            n_1          |
            *               O             O---+       |
            *               |      a      |   |       |
            *       w_0 O---X-->O---->O---X-->O e_1   |
            *               |c_0          |c_1        |
            *               O             O           |
            *              s_0            |           |
            *                             +-----------+
            */
            
            if constexpr( forwardQ )
            {
                // This keeps s_0 alive; this is what we like to have in forward mode.
                Reconnect<Tail>(s_0,w_0);
            }
            else
            {
                // This keeps w_0 alive; this is what we like to have in backward mode.
                Reconnect<Head>(w_0,s_0);
            }
            
            DeactivateArc(n_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            pd.R_I_counter += 2;
            
            return true;
        }
        
        PD_PRINT( "\t\t\tn_1 != e_1" );
        
        
        PD_ASSERT(w_0 != s_0);
        PD_ASSERT(n_1 != e_1);
        
        /*               +-------------------------+
         *               |                         |
         *               |            n_1          |
         *               O             O####       |
         *               |      a      |   #       |
         *       w_0 O---X-->O---->O---X-->O e_1   |
         *           #   |c_0          |c_1        |
         *           ####O             O           |
         *              s_0            |           |
         *                             +-----------+
         */

        PD_NOTE(MethodName("twist_at_a")+": Disconnect a subdiagram. ( crossing_count = " + ToString(pd.crossing_count) + ")");
        
        // This keeps w_0 alive; this is what we like to have in backward mode.
        Reconnect<Head>(w_0,s_0);
        // This keeps e_1 alive; this is what we like to have in forward mode.
        Reconnect<Tail>(e_1,n_1);
        DeactivateArc(a  );
        DeactivateArc(n_0);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        return true;
    }
    
    // Check for twist move. If successful, a, c_0, and c_1 are deleted.
    if( s_0 == n_1 )
    {
        PD_PRINT( "\ts_0 == n_1" );
        
        if( w_0 == n_0 )
        {
            PD_PRINT( "\t\tw_0 == n_0" );
            
            if( e_1 != s_1 )
            {
                PD_PRINT( "\t\t\te_1 != s_1" );
                
               /*                             +-----------+
                *                             |           |
                *              n_0            |           |
                *           +---O             O           |
                *           |   |      a      |           |
                *       w_0 O---X-->O---->O---X-->O e_1   |
                *               |c_0          |c_1#       |
                *               O             O####       |
                *               |            s_1          |
                *               |                         |
                *               +-------------------------+
                */
                
                if constexpr( forwardQ )
                {
                    // This keeps e_1 alive; this is what we like to have in forward mode.
                    Reconnect<Tail>(e_1,s_1);
                }
                else
                {
                    // This keeps s_1 alive; this is what we like to have in backward mode.
                    Reconnect<Head>(s_1,e_1);
                }
                
                DeactivateArc(a  );
                DeactivateArc(w_0);
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                // TODO: Implement counters.
//                pd.R_I_counter += 2;
                
                return true;
            }

            
            PD_PRINT( "\t\t\te_1 == s_1" );
            PD_ASSERT( e_1 == s_1 );
            
            /*                             +-----------+
             *                             |           |
             *              n_0            |           |
             *           +---O             O           |
             *           |   |      a      |           |
             *       w_0 O---X-->O---->O---X-->O e_1   |
             *               |c_0          |c_1|       |
             *               O             O---+       |
             *               |            s_1          |
             *               |                         |
             *               +-------------------------+
             */
            
            DeactivateArc(w_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            CreateUnlinkFromArc(a);
            
            // TODO: Implement counters.
//            pd.R_I_counter += 2;
            
            AssertArc<0>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<0>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<0>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            
            return true;
        }
        
        if( e_1 == s_1 )
        {
            PD_ASSERT( w_0 != n_0 );
            
            PD_PRINT( "\t\te_1 == s_1" );
            
            /*                             +-----------+
             *                             |           |
             *              n_0            |           |
             *               O             O           |
             *               |      a      |           |
             *       w_0 O---X-->O---->O---X-->O e_1   |
             *               |c_0          |c_1|       |
             *               O             O---+       |
             *               |            s_1          |
             *               |                         |
             *               +-------------------------+
             */
            
            ReconnectAsSuggestedByMode<Tail>(n_0,w_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            pd.R_I_counter += 2;
            
            return true;
        }
        
        PD_PRINT( "\t\t(w_0 != n_0) && (s_1 != e_1)." );
        PD_ASSERT( (w_0 != n_0) && (s_1 != e_1) );
        
        /*                             +-----------+
         *                             |           |
         *              n_0            |           |
         *           ####O             O           |
         *           #   |      a      |           |
         *       w_0 O---X-->O---->O---X-->O e_1   |
         *               |c_0          |c_1#       |
         *               O             O####       |
         *               |            s_1          |
         *               |                         |
         *               +-------------------------+
         */
        
        PD_NOTE(MethodName("twist_at_a")+": Disconnect a subdiagram. ( crossing_count = " + ToString(pd.crossing_count) + ")");

        // This keeps e_1 alive; this is what we like to have in forward mode.
        Reconnect<Tail>(e_1,s_1);
        // This keeps w_0 alive; this is what we like to have in backward mode.
        Reconnect<Head>(w_0,n_0);
        DeactivateArc(a  );
        DeactivateArc(s_0);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        return true;
    }
    
    return false;
}
