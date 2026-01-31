bool R_I_right()
{
    // TODO: We could also delete all possible further loops iteratively.
    
    PD_DPRINT("R_I_right()");
    
    AssertArc<1>(a);
    AssertCrossing<1>(c_0);
    AssertCrossing<1>(c_1);
    
    if(s_1 == e_1)
    {
        PD_PRINT("\ts_1 == e_1");
        
        if(n_0 != n_1)
        {
            PD_PRINT("\t\tn_0 != n_1");
            
            /*                n_0         n_1
             *               |           ^
             *               |     a     |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |   |
             *               v           +---+
             *                s_0
             */
            
            // This keeps n_1 alive; this is what we like to have in forward mode.
            Reconnect<Tail>(n_1,a);
            DeactivateArc(e_1);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            ++pd.R_I_counter;
            
            return true;
        }

        
        PD_PRINT("\t\tn_0 == n_1");
        PD_ASSERT(n_0 == n_1)
        
        if(w_0 != s_0)
        {
            PD_PRINT("\t\t\tw_0 != s_0");
            
            /* A second Reidemeister I move can be performed.
             *
             *               +-----<-----+
             *               |           ^
             *               |     a     |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |   |
             *               v           +---+
             *                s_0
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
            
            DeactivateArc(n_1);
            DeactivateArc(e_1);
            DeactivateArc(a);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            pd.R_I_counter += 2;
            
            return true;
        }
            
        PD_PRINT("\t\t\tw_0 == s_0");
        PD_ASSERT(w_0 == s_0);
        /* We detected an unlink
         *
         *               +-----<-----+
         *               |           ^
         *               |     a     |
         *       w_0 +-->X---------->X-->+ e_1
         *           |   |c_0        |   |
         *           +---v           +---+
         */
        
        DeactivateArc(w_0);
        DeactivateArc(e_1);
        DeactivateArc(n_1);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CreateUnlinkFromArc(a);
        
        // TODO: Implement counters.
//        pd.R_I_counter += 2;
        
        return true;
        
    }

    if(n_1 == e_1)
    {
        PD_PRINT("\tn_1 == e_1");
        
        if(s_0 != s_1)
        {
            PD_PRINT("\t\ts_0 != s_1");
            
            /*
             *
             *                 n_0
             *               ^           +---+
             *               |     a     |   |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |c_1
             *               |           v
             *                 s_0
             */
             
            // This keeps s_1 alive; this is what we like to have in forward mode.
            Reconnect<Tail>(s_1,a);
            DeactivateArc(e_1);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            ++pd.R_I_counter;
            
            return true;
        }
        
        
        PD_PRINT("\t\ts_0 == s_1");
        PD_ASSERT(s_0 == s_1);
        
        if(w_0 != n_0)
        {
            PD_PRINT("\t\t\tw_0 != n_0");
            
            /* A second Reidemeister I move can be performed.
             *
             *                n_0
             *               ^           +---+
             *               |     a     |   |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |c_1
             *               |           v
             *               +-----<-----+
             *                    s_0
             */
            
            if constexpr( forwardQ )
            {
                // This keeps n_0 alive; this is what we like to have in forward mode.
                Reconnect<Tail>(n_0,w_0);
            }
            else
            {
                // This keeps w_0 alive; this is what we like to have in backward mode.
                Reconnect<Head>(w_0,n_0);
            }
            
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            pd.R_I_counter += 2;
            
            return true;
        }
        
        PD_PRINT("\t\t\tw_0 == n_0");
        PD_ASSERT(w_0 == n_0);
        
        /*
         *           +---+           +---+
         *           |   |     a     |   |
         *       w_0 +-->X---------->X-->+ e_1
         *               |c_0        |c_1
         *               |           v
         *               +-----<-----+
         *                    s_0
         */
        
        DeactivateArc(w_0);
        DeactivateArc(e_1);
        DeactivateArc(a  );
        DeactivateArc(s_0);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CreateUnlinkFromArc(a);
        
        // TODO: Implement counters.
//        pd.R_I_counter += 2;
        
        return true;
    }
    
    return false;
}
