bool a_is_2loop()
{
    PD_PRINT("a_is_2loop()");
    
    if(w_0 == e_1)
    {
        PD_PRINT("\tw_0 == e_1");
        
        /* We have a true over- or underloop. Looks like one of these:
         *
         *       +-------------------+                  O###########O
         *       |                   |                  |c_0        |c_1
         *       |   O###########O   |              +-->X---------->X-->+
         *       |   |     a     |   |              |   |     a     |   |
         *       +-->X---------->X-->+      or      |   O###########O   |
         *           |c_0        |c_1               |                   |
         *           O###########O                  +-------------------+
         */
        
        // We can disconnect up to two subdiagram (marked by ### and top and bottom.
        // Moreover, if o_0 != o_1, we might also disconnect a Hopf link.
        // Alas, for o_0 == o_1 we might get two unlinks, instead.
        
        DeactivateArc(w_0);
        DeactivateArc(a);
        
        if( n_0 == n_1 )
        {
            PD_PRINT("\t\tn_0 == n_1");
            
            if( s_0 == s_1 )
            {
                PD_PRINT("\t\t\ts_0 == s_1");
                
               /*       +-------------------+
                *       |                   |
                *       |   O-----------O   |
                *       |   |     a     |   |
                *       +-->X---------->X-->+
                *           |c_0        |c_1
                *           O-----------O
                */
                
                DeactivateArc(n_0);
                DeactivateArc(s_0);
                
                if( o_0 == o_1 )
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Identified two unlinks.");
                    CreateUnlinkFromArc(a);
                    CreateUnlinkFromArc(n_0);
                }
                else
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Identified a Hopf link.");
                    CreateHopfLinkFromArcs(a,n_0,c_0_state);
                }
            }
            else
            {
                PD_PRINT("\t\t\ts_0 != s_1");
                
                /*       +-------------------+
                 *       |                   |
                 *       |   O-----------O   |
                 *       |   |     a     |   |
                 *       +-->X---------->X-->+
                 *           |c_0        |c_1
                 *           O###########O
                 */
                
                Reconnect(s_0,u_0,s_1);
                DeactivateArc(n_0);
                
                if( o_0 == o_1 )
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Split an unlink as diagram components.");
                    CreateUnlinkFromArc(a);
                }
                else
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Disconnected a Hopf link as connected summand.");
                    CreateHopfLinkFromArcs(a,n_0,c_0_state);
                }
            }
        }
        else
        {
            PD_PRINT("\t\tn_0 != n_1");
            if( s_0 == s_1 )
            {
                PD_PRINT("\t\t\ts_0 == s_1");
                
                /*       +-------------------+
                 *       |                   |
                 *       |   O###########O   |
                 *       |   |     a     |   |
                 *       +-->X---------->X-->+
                 *           |c_0        |c_1
                 *           O-----------O
                 */
                
                Reconnect(n_1,u_0,n_0);
                DeactivateArc(s_0);
                
                if( o_0 == o_1 )
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Split an unlink as diagram components.");
                    CreateUnlinkFromArc(a);
                }
                else
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Disconnected a Hopf link as connected summand.");
                    CreateHopfLinkFromArcs(a,s_0,c_0_state);
                }
            }
            else
            {
                PD_PRINT("\t\t\ts_0 != s_1");
                
                /*       +-------------------+
                 *       |                   |
                 *       |   O###########O   |
                 *       |   |     a     |   |
                 *       +-->X---------->X-->+
                 *           |c_0        |c_1
                 *           O###########O
                 */
                
                Reconnect(n_1,u_0,n_0);
                Reconnect(s_0,u_0,s_1);
                
                if( o_0 == o_1 )
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Identified an unlink and disconnected a subdiagram as connected summand.");
                    CreateUnlinkFromArc(a);
                }
                else
                {
                    PD_NOTE(MethodName("a_is_2loop")+": Disconnected a Hopf link and a further subdiagram as connected summand.");
                    CreateHopfLinkFromArcs(a,s_0,c_0_state);
                }
            }
        }
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        return true;
    }
    
    PD_PRINT("\tw_0 != e_1");
    
    return false;
}
