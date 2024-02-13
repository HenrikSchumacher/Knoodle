Class_T BreakChild( const Int c )
{
    print("BreakChild");
    // Make sure that Reidemeister_I is not applicable;
    
    Class_T pd = CreateCompressed();
    
    valprint("c",c);
    
    print( "labels = " + pd.CrossingLabels().ToString() );
    
    valprint("pd.CrossingLabelLookUp(c)",pd.CrossingLabelLookUp(c));
        
    PD_assert( pd.Break( pd.CrossingLabelLookUp(c) ) );
    
    return pd;
}


bool Break( const Int c )
{
    print("Break");
    valprint("c",c);
    
    PD_assert(CrossingActive(c));
    
    if( CrossingActive(c) )
    {
        valprint("c",c);

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

//        const Int v_0 = A_cross(e_0,Tail);
//        const Int v_1 = A_cross(e_1,Tail);
//
//        const Int v_2 = A_cross(e_2,Tip );
//        const Int v_3 = A_cross(e_3,Tip );
//
//        valprint("v_0",v_0);
//        valprint("v_1",v_1);
//        valprint("v_2",v_2);
//        valprint("v_3",v_3);
//
//        PD_assert(v_0!=v_3);
//        PD_assert(v_1!=v_2);

        Reconnect(e_0,Tip,e_3);
        Reconnect(e_1,Tip,e_2);

        DeactivateCrossing(c);
        DeactivateArc(e_2);
        DeactivateArc(e_3);

        return true;
    }
    else
    {
        wprint("Break failed.");
        return false;
    }
}
