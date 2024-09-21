bool twist_at_a()
{
    PD_DPRINT( "twist_at_a()" );
    
    AssertArc(a);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
    
    AssertArc(w_0);
    AssertArc(n_0);
    AssertArc(s_0);
    
    AssertArc(n_1);
    AssertArc(e_1);
    AssertArc(s_1);
    
    // Check for twist move. If successful, a, c_0, and c_1 are deleted.
    if( n_0 == s_1 )
    {
        PD_DPRINT( "\tn_0 == s_1" );
        
        if( w_0 == s_0 )
        {
            PD_DPRINT( "\t\tw_0 == s_0" );
            
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
                PD_DPRINT( "\t\t\tn_1 != e_1" );
                
                Reconnect(e_1,Tail,n_1);
                DeactivateArc(w_0);
                DeactivateArc(a );
                DeactivateArc(n_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                ++pd.twist_counter;
                
                AssertArc(e_1);
                
                return true;
            }
            
            PD_DPRINT( "\t\t\tn_1 == e_1" );
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
            
            ++pd.unlink_count;
            DeactivateArc(w_0);
            DeactivateArc(e_1);
            DeactivateArc(a );
            DeactivateArc(n_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            return true;
        }
        
        if( n_1 == e_1 )
        {
            PD_DPRINT( "\t\tn_1 == e_1" );
            
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
            
            Reconnect(w_0,Head,s_0);
            DeactivateArc(n_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            return true;
        }
        
        PD_DPRINT( "\t\t(w_0 != s_0) && (n_1 != e_1)." );
        
        // We have one of these two situations:
        
        /* Case A:
         *
         *
         *               +-------------------------+
         *               |                         |
         *               |            n_1          |
         *               O             O####       |
         *               |      a      |   #       |
         *       w_0 O---X-->O---->O---X-->O e_1   |
         *               |c_0          |c_1        |
         *               O             O           |
         *              s_0            |           |
         *                             +-----------+
         *
         * No matter what the crossing at c_1 is, we can reroute as follows:
         *
         *               +-------------------------+
         *               |         +----------+    |
         *               |         |  n_1     |    |
         *               O         |   O####  |    |
         *               |         |   |   #  |    |
         *       w_0 O---X---------+   |   O<-+    |
         *               |c_0          |  e_1      |
         *               O             O           |
         *              s_0            |           |
         *                             +-----------+
         *
         * Now, no matter what the crossing at c_0 is, we can reroute as follows:
         *
         *
         *                         +----------+
         *                         |  n_1     |
         *                         |   O####  |
         *                         |   |   #  |
         *       w_0 O-------------+   |   O<-+
         *                             |  e_1
         *               O-------------+
         *              s_0
         *
         * a, c_0, and c_1 are deleted.
         * w_0 and e_1 are fused.
         * s_0 and n_1 are fused.
         */
        
        
        /* Case B
         *
         *   +-----------+
         *   |           |
         *   |           |            n_1
         *   |           O             O
         *   |           |      a      |
         *   |   w_0 O---X-->O---->O---X-->O e_1
         *   |       #   |c_0          |c_1
         *   |       ####O             O
         *   |          s_0            |
         *   |                         |
         *   +-------------------------+
         *
         * No matter what the crossing at c_0 is, we can reroute as follows:
         *
         *   +-----------+
         *   |           |
         *   |           |            n_1
         *   |           O             O
         *   |      w_0  |             |
         *   |    +--O   |   +---------X-->O e_1
         *   |    |  #   |c_0|         |c_1
         *   |    |  ####O   |         O
         *   |    |     s_0  |         |
         *   |    +----------+         |
         *   +-------------------------+
         *
         * Now, no matter what the crossing at c_0 is, we can reroute as follows:
         *
         *
         *
         *                            n_1
         *               +-------------O
         *          w_0  |
         *        +--O   |   +------------>O e_1
         *        |  #   |   |
         *        |  ####O   |
         *        |     s_0  |
         *        +----------+
         *
         * a, c_0, and c_1 are deleted.
         * w_0 and e_1 are fused.
         * s_0 and n_1 are fused.
         */
        
        
        Reconnect(w_0,Head,e_1);
        Reconnect(s_0,u_0 ,n_1);
        DeactivateArc(a  );
        DeactivateArc(s_1);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++pd.twist_counter;
        
        AssertArc(w_0);
        AssertArc(s_0);
        
        return true;
    }
    
    // Check for twist move. If successful, a, c_0, and c_1 are deleted.
    if( s_0 == n_1 )
    {
        PD_DPRINT( "\ts_0 == n_1" );
        
        if( w_0 == n_0 )
        {
            PD_DPRINT( "\t\tw_0 == n_0" );
            
            if( e_1 != s_1 )
            {
                PD_DPRINT( "\t\t\te_1 != s_1" );
                
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
                
                Reconnect(e_1,Tail,s_1);
                DeactivateArc(a  );
                DeactivateArc(w_0);
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                pd.R_I_counter += 2;
                
                AssertArc(e_1);
                
                return true;
            }

            
            PD_DPRINT( "\t\t\te_1 == s_1" );
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
            
            ++pd.unlink_count;
            DeactivateArc(w_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            return true;
        }
        
        if( e_1 == s_1 )
        {
            PD_ASSERT( w_0 != n_0 );
            
            PD_DPRINT( "\t\te_1 == s_1" );
            
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
            
            Reconnect(w_0,Head,n_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            AssertArc(w_0);
            
            return true;
        }
        
        PD_DPRINT( "\t\t(w_0 != n_0) && (s_1 != e_1)." );
        PD_ASSERT( (w_0 != n_0) && (s_1 != e_1) );
        
        /* We have one of these two situations:
         *
         * Case A:
         *
         *                             +-----------+
         *                             |           |
         *              n_0            |           |
         *               O             O           |
         *               |      a      |           |
         *       w_0 O---X-->O---->O---X-->O e_1   |
         *               |c_0          |c_1#       |
         *               O             O####       |
         *               |            s_1          |
         *               |                         |
         *               +-------------------------+
         *
         * No matter what the crossing at c_1 is, we can reroute as follows:
         *
         *                             +-----------+
         *                             |           |
         *              n_0            |           |
         *               O             O           |
         *               |             |  e_1      |
         *       w_0 O---X---------+   |   O<-+    |
         *               |c_0      |   |   #  |    |
         *               O         |   O####  |    |
         *               |         |  s_1     |    |
         *               |         +----------+    |
         *               +-------------------------+
         *
         * Now, no matter what the crossing at c_0 is, we can reroute as follows:
         *
         *
         *
         *              n_0
         *               O-------------+
         *                             |  e_1
         *       w_0 O-------------+   |   O--+
         *                c_0      |   |   #  |
         *                         |   O####  |
         *                         |  s_1     |
         *                         +----------+
         *
         *
         * a, c_0, and c_1 are deleted.
         * w_0 and e_1 are fused.
         * n_0 and s_1 are fused.
         */
         
        // Case B is fully analogous. I skip it.
                
        Reconnect(w_0,Head,e_1);
        Reconnect(n_0,!u_0,s_1);
        DeactivateArc(a  );
        DeactivateArc(s_0);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++pd.twist_counter;
        
        AssertArc(w_0);
        AssertArc(n_0);
        
        return true;
    }
    
    return false;
}
