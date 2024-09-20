bool a_is_loop()
{
    PD_DPRINT( "a_is_loop()" );
    
    if( c_0 == c_1 )
    {
        PD_DPRINT( "\tc_0 == c_1" );
        
        if( s_0 == a )
        {
            PD_DPRINT( "\t\ts_0 == a" );
           // This implies s_0 == a

            if( w_0 != n_0 )
            {
                PD_DPRINT( "\t\tw_0 != n_0" );
                
                /*              n_0
                 *               O
                 *               ^
                 *               |
                 *      w_0 O----X--->O
                 *               |    |
                 *               |c_0 |
                 *               O<---+  a
                 */
                
                Reconnect(w_0,Head,n_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                ++pd.R_I_counter;
                
                AssertArc(w_0);
                
                return true;
            }
            else // if( w_0 == n_0 )
            {
                PD_DPRINT( "\t\tw_0 == n_0" );
                
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
                
                return true;
            }
        }
        else
        {
            // This implies n_0 == a.
            
            PD_DPRINT( "\t\tn_0 == a" );
            PD_ASSERT( "n_0 == a" );
            
            if( w_0 != s_0 )
            {
                PD_DPRINT( "\t\t\tw_0 != s_0" );
                
                /*               O----+
                 *               |    |
                 *               |    | a
                 *      w_0 O----X--->O
                 *               |c_0
                 *               v
                 *               O
                 *              s_0
                 */
            
                Reconnect(w_0,Head,s_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                ++pd.R_I_counter;
                
                AssertArc(w_0);
                
                return true;
            }
            else // if( w_0 == s_0 )
            {
                PD_DPRINT( "\t\t\tw_0 == s_0" );
                
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
                
                return true;
            }
        }
    }
    else
    {
        return false;
    }
}
