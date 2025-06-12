//Class_T BreakChild( const Int c )
//{
//    print("BreakChild");
//    // Make sure that Reidemeister_I is not applicable;
//    
//    Class_T pd = Canonicalize();
//    
//    PD_VALPRINT("c",c);
//    
//    PD_PRINT( "labels = " + pd.CrossingLabels().ToString() );
//    
//    PD_VALPRINT("pd.CrossingLabelLookUp(c)",pd.CrossingLabelLookUp(c));
//        
//    PD_ASSERT( pd.Break( pd.CrossingLabelLookUp(c) ) );
//    
//    return pd;
//}


bool Break( const Int c )
{
    PD_PRINT("Break");
    PD_VALPRINT("c",c);
    
    PD_ASSERT(CrossingActiveQ(c));
    
    if( CrossingActiveQ(c) )
    {
        PD_VALPRINT("c",c);

        // Make sure that Reidemeister_I is not applicable;

        const bool R_I = Reidemeister_I_at_Crossing(c);

        if( R_I )
        {
            PD_WPRINT("Called Break, but Reidemeister_I was performed on crossing "+ToString(c)+".");
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
        PD_ASSERT(e_0!=e_2);
        PD_ASSERT(e_1!=e_3);

        // Should be ruled out by Reidemeister_I check.
        PD_ASSERT(e_0!=e_3);
        PD_ASSERT(e_1!=e_3);

        Reconnect<Head>(e_0,e_3);
        Reconnect<Head>(e_1,e_2);

        DeactivateCrossing(c);

        return true;
    }
    else
    {
        PD_WPRINT("Break failed.");
        return false;
    }
}
