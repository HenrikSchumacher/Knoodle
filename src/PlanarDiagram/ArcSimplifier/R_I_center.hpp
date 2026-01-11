bool R_I_center()
{
    PD_DPRINT("R_I_center()");
    
    if(c_0 == c_1)
    {
        PD_PRINT("\tc_0 == c_1");
        
        if(s_0 == a)
        {
            PD_PRINT("\t\ts_0 == a");

            if(w_0 != n_0)
            {
                PD_PRINT("\t\tw_0 != n_0");
                
                /*              n_0
                 *               O
                 *               ^
                 *               |
                 *      w_0 O----X--->O
                 *               |    |
                 *               |c_0 |
                 *               O<---+  a
                 */
                
                Reconnect<Head>(w_0,n_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                ++pd.R_I_counter;
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<1>(w_0);
                AssertArc<0>(s_0);
                AssertCrossing<0>(c_0);
                
                return true;
            }
            else // if(w_0 == n_0)
            {
                PD_PRINT("\t\tw_0 == n_0");
                
                /*          +----O
                 *          |    ^
                 *          |    |
                 *      w_0 O----X--->O
                 *               |    |
                 *               |c_0 |
                 *               O<---+  a
                 *              s_0
                 */
                
                ++pd.unlink_count;
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                pd.R_I_counter += 2; // We make two R_I moves instead of one.
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(w_0);
                AssertArc<0>(s_0);
                AssertCrossing<0>(c_0);
                
                return true;
            }
        }
        else
        {
            PD_PRINT("\t\tn_0 == a");
            PD_ASSERT(n_0 == a);
            
            if(w_0 != s_0)
            {
                PD_PRINT("\t\t\tw_0 != s_0");
                
                /*               O----+
                 *               |    |
                 *               |    | a
                 *      w_0 O----X--->O
                 *               |c_0
                 *               v
                 *               O
                 *              s_0
                 */
            
                Reconnect<Head>(w_0,s_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                ++pd.R_I_counter;

                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<1>(w_0);
                AssertArc<0>(s_0);
                AssertCrossing<0>(c_0);
                
                return true;
            }
            else // if(w_0 == s_0)
            {
                PD_PRINT("\t\t\tw_0 == s_0");
                
                /*               O----+
                 *               |    |
                 *               |    | a
                 *      w_0 O----X--->O
                 *          |    |c_0
                 *          |    v
                 *          +----O
                 */
                
                ++pd.unlink_count;
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                
                pd.R_I_counter += 2;
                
                AssertArc<0>(a  );
                AssertArc<0>(n_0);
                AssertArc<0>(w_0);
                AssertArc<0>(s_0);
                AssertCrossing<0>(c_0);

                return true;
            }
        }
    }
    else
    {
        return false;
    }
}
