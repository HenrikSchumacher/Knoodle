bool R_I_left()
{
    PD_DPRINT( "R_I_left()" );
    
    AssertArc(a);
    AssertCrossing(c_0);
    
    if( n_0 == w_0 )
    {
        PD_DPRINT( "\tn_0 == w_0" );
        
        if( s_0 != s_1 )
        {
            PD_DPRINT( "\t\ts_0 != s_1" );
            
            /* Looks like this:
             *
             *                            n_1
             *           +---+           |
             *           |   |     a     v
             *       w_0 +-->X---------->X--> e_1
             *               |c_0        |c_1
             *               |           v
             *                s_0         s_1
             */
            
            Reconnect(a,Tail,s_0);
            DeactivateArc(w_0);
            DeactivateCrossing(c_0);
            ++pd.R_I_counter;
            
            AssertArc(a  );
            AssertArc(n_1);
            AssertArc(e_1);
            AssertArc(s_1);
            AssertCrossing(c_1);
            
            return true;
        }

        PD_DPRINT( "t\ts_0 == s_1" );
        PD_ASSERT( s_0 == s_1 );
        
        if( e_1 != n_1 )
        {
            PD_DPRINT( "t\t\te_1 != n_1" );
            
            /* A second Reidemeister I move can be performed.
             *
             *                            n_1
             *           +---+           |
             *           |   |     a     v
             *       w_0 +-->X---------->X--> e_1
             *               |c_0        |c_1
             *               |           v
             *               +-----<-----+
             8                    s_0
             */
            
            Reconnect(e_1,Tail,n_1);
            DeactivateArc(w_0);
            DeactivateArc(s_0);
            DeactivateArc(a  );
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            AssertArc(e_1);
            
            return true;
        }

        PD_DPRINT( "t\t\te_1 == n_1" );
        PD_ASSERT( e_1 == n_1 );
        
        /* A second Reidemeister I move can be performed.
         *
         *
         *           +---+           +---+
         *           |   |     a     v   |
         *       w_0 +-->X---------->X-->+ e_1
         *               |c_0        |c_1
         *               |           v
         *               +-----<-----+
         *                     s_0
         */
        
        DeactivateArc(w_0);
        DeactivateArc(e_1);
        DeactivateArc(s_0);
        DeactivateArc(a  );
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        pd.R_I_counter += 2;
        
        ++pd.unlink_count;
        
        return true;
    }
    
    if( s_0 == w_0 )
    {
        PD_DPRINT( "\ts_0 == w_0" );
        
        if( n_0 != n_1 )
        {
            PD_DPRINT( "\t\tn_0 != n_1" );
            
            /* A second Reidemeister I move can be performed.
             *
             *                n_0         n_1
             *               |           |
             *               |     a     |
             *       w_0 +-->X---------->X--> e_1
             *           |   |c_0        ^c_1
             *           +<--+           |
             *                            s_1
             */
            
            Reconnect(a,Tail,n_0);
            DeactivateArc(w_0);
            DeactivateCrossing(c_0);
            ++pd.R_I_counter;
            
            AssertArc(a  );
            AssertArc(n_1);
            AssertArc(e_1);
            AssertArc(s_1);
            AssertCrossing(c_1);

            return true;
        }

        PD_DPRINT( "\t\tn_0 == n_1" );
        PD_ASSERT( n_0 == n_1 );
        
        if( e_1 != s_1 )
        {
            PD_DPRINT( "\t\t\te_1 != s_1" );
            
            /* A second Reidemeister I move can be performed.
             *
             *                    n_0
             *               +-----<-----+
             *               |           |
             *               |     a     |
             *       w_0 +-->X---------->X--> e_1
             *           |   |c_0        ^c_1
             *           +<--+           |
             *                            s_1
             */
            
            Reconnect(e_1,Tail,s_1);
            DeactivateArc(w_0);
            DeactivateArc(n_0);
            DeactivateArc(a  );
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            AssertArc(e_1);
            
            return true;
        }

        PD_DPRINT( "\t\t\te_1 == s_1" );
        PD_ASSERT( e_1 == s_1 );
        
        /* A second Reidemeister I move can be performed.
         *
         *                    n_0
         *               +-----<-----+
         *               |           |
         *               |     a     |
         *       w_0 +-->X---------->X-->+e_1
         *           |   |c_0        ^c_1|
         *           +<--+           +---+
         *
         */
        
        ++pd.unlink_count;
        DeactivateArc(e_1);
        DeactivateArc(w_0);
        DeactivateArc(n_0);
        DeactivateArc(a  );
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        pd.R_I_counter += 2;
        
        return true;
    }
    
    return false;
}
