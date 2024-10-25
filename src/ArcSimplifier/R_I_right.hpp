bool R_I_right()
{
    PD_PRINT("R_I_right()");
    
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
            
            Reconnect<Head>(a,n_1);
            DeactivateArc(e_1);
            DeactivateCrossing(c_1);
            ++pd.R_I_counter;
            
            AssertArc<1>(a  );
            AssertArc<1>(n_0);
            AssertArc<1>(s_0);
            AssertArc<1>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<0>(s_1);
            AssertCrossing<1>(c_0);
            AssertCrossing<0>(c_1);
            
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
            
            Reconnect<Head>(w_0,s_0);
            DeactivateArc(n_1);
            DeactivateArc(e_1);
            DeactivateArc(a);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            AssertArc<0>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<1>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<0>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            
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
        
        ++pd.unlink_count;
        DeactivateArc(w_0);
        DeactivateArc(e_1);
        DeactivateArc(n_1);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        pd.R_I_counter += 2;
        
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

    if(n_1 == e_1)
    {
        PD_PRINT("\tn_1 == e_1");
        
        if(s_0 != s_1)
        {
            PD_PRINT("\t\ts_0 != s_1");
            
            /* A second Reidemeister I move can be performed.
             *
             *                 n_0
             *               ^           +---+
             *               |     a     |   |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |c_1
             *               |           v
             *                 s_0
             */
             
            Reconnect<Tail>(s_1,a);
            DeactivateArc(e_1);
            DeactivateCrossing(c_1);
            ++pd.R_I_counter;
            
            AssertArc<0>(a  );
            AssertArc<1>(n_0);
            AssertArc<1>(s_0);
            AssertArc<1>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<1>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            
            return true;
        }
        
        
        PD_PRINT("\t\ts_0 == s_1");
        PD_ASSERT(s_0 == s_1);
        
        if(w_0 != n_0)
        {
            PD_PRINT("\t\t\tw_0 != n_0");
            
            /*                n_0
             *               ^           +---+
             *               |     a     |   |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |c_1
             *               |           v
             *               +-----<-----+
             *                    s_0
             */
            
            Reconnect<Head>(w_0,n_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            pd.R_I_counter += 2;
            
            AssertArc<0>(a  );
            AssertArc<0>(n_0);
            AssertArc<0>(s_0);
            AssertArc<1>(w_0);
            AssertArc<0>(n_1);
            AssertArc<0>(e_1);
            AssertArc<0>(s_1);
            AssertCrossing<0>(c_0);
            AssertCrossing<0>(c_1);
            
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
        
        ++pd.unlink_count;
        DeactivateArc(w_0);
        DeactivateArc(e_1);
        DeactivateArc(a  );
        DeactivateArc(s_0);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        pd.R_I_counter += 2;
        
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
    
    return false;
}
