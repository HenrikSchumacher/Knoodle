bool R_I_left()
{
    // TODO: We could also delete all possible further loops iteratively.
    PD_DPRINT("R_I_left()");
    
    AssertArc<1>(a);
    AssertCrossing<1>(c_0);
    
    if(n_0 == w_0)
    {
        PD_PRINT("\tn_0 == w_0");
        
        if(s_0 != s_1)
        {
            PD_PRINT("\t\ts_0 != s_1");
            
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
            
            Reconnect<Tail>(a,s_0);
            DeactivateArc(w_0);
            DeactivateCrossing(c_0);
            
            // TODO: Implement counters.
//            ++pd.R_I_counter;

            AssertArc<1>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<0>(w_0);
            AssertArc<1>(n_1);
            AssertArc<1>(e_1);
            AssertArc<1>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<1>(c_1);
            
            return true;
        }

        PD_PRINT("t\ts_0 == s_1");
        PD_ASSERT(s_0 == s_1);
        
        if(e_1 != n_1)
        {
            PD_PRINT("t\t\te_1 != n_1");
            
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
            
            // This keeps e_1 alive; this is what we like to have in forward mode.
            Reconnect<Tail>(e_1,n_1);
            DeactivateArc(w_0);
            DeactivateArc(s_0);
            DeactivateArc(a  );
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
//            pd.R_I_counter += 2;
            
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

        PD_PRINT("t\t\te_1 == n_1");
        PD_ASSERT(e_1 == n_1);
        
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
        CreateUnlinkFromArc(a);
        
        // TODO: Implement counters.
//        pd.R_I_counter += 2;

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
    
    if(s_0 == w_0)
    {
        PD_PRINT("\ts_0 == w_0");
        
        if(n_0 != n_1)
        {
            PD_PRINT("\t\tn_0 != n_1");
            
            /*
             *
             *                n_0         n_1
             *               |           |
             *               |     a     |
             *       w_0 +-->X---------->X--> e_1
             *           |   |c_0        ^c_1
             *           +<--+           |
             *                            s_1
             */
            
            PD_ASSERT(n_0 != e_1);
            PD_ASSERT(n_0 != s_1);
            PD_ASSERT(A_cross(n_0,Tail) != c_1);
            
            Reconnect<Tail>(a,n_0);
            DeactivateArc(w_0);
            DeactivateCrossing(c_0);
            
            // TODO: Implement counters.
//            ++pd.R_I_counter;
            
            AssertArc<1>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<0>(w_0);
            AssertArc<1>(n_1);
            AssertArc<1>(e_1);
            AssertArc<1>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<1>(c_1);

            return true;
        }

        PD_PRINT("\t\tn_0 == n_1");
        PD_ASSERT(n_0 == n_1);
        
        if(e_1 != s_1)
        {
            PD_PRINT("\t\t\te_1 != s_1");
            
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
            
            // This keeps e_1 alive; this is what we like to have in forward mode.
            Reconnect<Tail>(e_1,s_1);
            DeactivateArc(w_0);
            DeactivateArc(n_0);
            DeactivateArc(a  );
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            // TODO: Implement counters.
//            pd.R_I_counter += 2;
            
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

        PD_PRINT("\t\t\te_1 == s_1");
        PD_ASSERT(e_1 == s_1);
        
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
                
        DeactivateArc(e_1);
        DeactivateArc(w_0);
        DeactivateArc(n_0);
        DeactivateArc(a  );
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CreateUnlinkFromArc(a);
        
        // TODO: Implement counters.
//        pd.R_I_counter += 2;
        
        AssertArc<0>(a  );
        AssertArc<0>(n_0);
        AssertArc<0>(s_0);
        AssertArc<0>(w_0);
        AssertArc<0>(n_1);
        AssertArc<0>(e_1);
        AssertArc<0>(s_1);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        PD_ASSERT(pd.CrossingCount()== Int(0));
        pd = PD_T::InvalidDiagram();
        
        return true;
    }
    
    return false;
}
