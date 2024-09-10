Class_T BreakChild( const Int c )
{
    print("BreakChild");
    // Make sure that Reidemeister_I is not applicable;
    
    Class_T pd = CreateCompressed();
    
    PD_valprint("c",c);
    
    PD_print( "labels = " + pd.CrossingLabels().ToString() );
    
    PD_valprint("pd.CrossingLabelLookUp(c)",pd.CrossingLabelLookUp(c));
        
    PD_assert( pd.Break( pd.CrossingLabelLookUp(c) ) );
    
    return pd;
}


bool Break( const Int c )
{
    PD_print("Break");
    PD_valprint("c",c);
    
    PD_assert(CrossingActiveQ(c));
    
    if( CrossingActiveQ(c) )
    {
        PD_valprint("c",c);

        // Make sure that Reidemeister_I is not applicable;

        const bool R_I = Reidemeister_I(c);

        if( R_I )
        {
            PD_wprint("Called Break, but Reidemeister_I was performed on crossing "+ToString(c)+".");
            return true;
        }

//               v_3 O----<----O       O---->----O v_2
//                       e_3    ^     ^    e_2
//                               \   /
//                                \ /
//        e_0 != v_e             c X               e_0 != v_e
//                                / \
//                               /   \
//                       e_0    /     \    e_1
//               v_0 O---->----O       O----<----O v_1

        const Int e_0 = C_arcs(c,In ,Left );
        const Int e_1 = C_arcs(c,In ,Right);
        const Int e_2 = C_arcs(c,Out,Right);
        const Int e_3 = C_arcs(c,Out,Left );

        // Should not be possible for topological reasons.
        PD_assert(e_0!=e_2);
        PD_assert(e_1!=e_3);

        // Should be ruled out by Reidemeister_I check.
        PD_assert(e_0!=e_3);
        PD_assert(e_1!=e_3);

        Reconnect(e_0,Tip,e_3);
        Reconnect(e_1,Tip,e_2);

        DeactivateCrossing(c);
        DeactivateArc(e_2); // Done by Reconnect.
        DeactivateArc(e_3); // Done by Reconnect.

        return true;
    }
    else
    {
        PD_wprint("Break failed.");
        return false;
    }
}
