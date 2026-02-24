public:

bool TwoStrandMove(
    const Size_T diagramIdx,
    const Int    crossing,
    const bool   overQ,
    const bool   verboseQ,
    const Int    step
)
{
    using Pass_T = PassSimplifier_T::Pass_T;
    using Path_T = PassSimplifier_T::Path_T;
    
    PD_T & pd = pd_list[diagramIdx];
    
    if( pd.InvalidQ() ) { return false; };
    
    if( pd.ProvenMinimalQ() ) { return false; };
    
    if(
       !InIntervalQ(crossing, Int(0), pd.MaxCrossingCount())
       ||
       !pd.CrossingActiveQ(crossing)
    )
    {
        return false;
    }
    
    PD_ASSERT(pd.CheckAll());
    
    PassSimplifier_T & S = PassSimplifier();
    S.LoadDiagram(pd);
    S.ResetMarks();
    
    auto cleanup = [&S](){
        S.ResetMarks();
        S.Cleanup();
    };
    
    bool successQ = false;
    
    pd.template AssertCrossing<1>(crossing);
    
    C_Arcs_T C = pd.CopyCrossing(crossing);
    
    const bool side = pd.ArcOverQ(C(Out,Right),Tail) == overQ;
    const Int a_1 = C(Out, side);
    const Int a_2 = C(In , side);
    const Int a_3 = C(Out,!side);
    
    if( verboseQ )
    {
        TOOLS_DUMP(pd.CrossingCount());
        TOOLS_DUMP(a_1);
        TOOLS_DUMP(a_2);
        TOOLS_DUMP(a_3);
    }
    
    pd.template AssertArc<1>(a_1);
    pd.template AssertArc<1>(a_2);
    pd.template AssertArc<1>(a_3);
    
    const Int mark_1 = 1;
    const Int mark_2 = 2;
    const Int mark_3 = 3;
    const Int mark_4 = 4;
    
    Pass_T pass_1;
    Pass_T pass_2;
    Pass_T pass_3;
    
    if( verboseQ )
    {
        print("FindPass: pass_1, pass_2, pass_3");
    }
    
    S.template FindPass<true,false,false>( pass_1, a_1, overQ, mark_1 );
    S.template FindPass<true,false,false>( pass_2, a_2, overQ, mark_2 );
    S.template FindPass<true,false,false>( pass_3, a_3, overQ, mark_3 );
    
    if( verboseQ )
    {
        TOOLS_DUMP(pass_1);
        valprint("pass_1 array",S.PassString(pass_1));
        TOOLS_DUMP(pass_2);
        valprint("pass_2 array",S.PassString(pass_2));
        TOOLS_DUMP(pass_3);
        valprint("pass_3 array",S.PassString(pass_3));
    }
    
    PD_ASSERT(pd.CheckAll());
    PD_ASSERT(pd.CheckArcNextArc());
    
    if( step < 1 )
    {
        cleanup();
        return false;
    }
    
    if( verboseQ )
    {
        print("FindPass: pass_4");
    }
    
    Pass_T pass_4 {
        .first     = pass_2.first,
        .last      = pass_3.last,
        .next      = pass_3.next,
        .arc_count = pass_2.arc_count + pass_3.arc_count,
        .mark      = mark_4,
        .overQ     = overQ,
        .activeQ   = pass_2.activeQ && pass_3.activeQ
    };
    
    if( verboseQ )
    {
        print("MarkArcs: pass_4");
    }
    
    S.MarkArcs(pass_4.first,pass_4.last,pass_4.mark);
    
    if( verboseQ )
    {
        TOOLS_DUMP(pass_4);
        //    valprint("pass_4",ToString(pass_4));
        valprint("pass_4 array",S.PassString(pass_4));
        print("Finding shortest path for pass_1.");
    }
    
    if( step < 2 )
    {
        cleanup();
        return false;
    }
    
    Path_T path_1;
    Path_T path_4;
    
    const Int dual_mark_1 = 1;
    
    const bool path_1_foundQ = S.FindShortestPath(
        pass_1.first, pass_1.last, Scalar::Max<Int>, path_1, dual_mark_1, mark_1, mark_4
    );
    
    if( !path_1_foundQ )
    {
        // No path found. This can happen if pass_4 contains all the arcs of an face at one end of pass_1.
        cleanup();
        return false;
    }
    
    if( verboseQ )
    {
        TOOLS_DUMP(path_1_foundQ);
        valprint("path_1",PathString(path_1));
        print("Finding shortest path for pass_4.");
    }
    
    if( step < 3 )
    {
        cleanup();
        return false;
    }
    
    const Int dual_mark_4 = dual_mark_1 + 1;
    
    bool path_4_foundQ = S.FindShortestPath(
        pass_4.first, pass_4.last, Scalar::Max<Int>, path_4, dual_mark_4, mark_4
    );
    
    if( !path_4_foundQ )
    {
        // No path found. This can happen if pass_1 contains all the arcs of an face at one end of pass_4.
        cleanup();
        return false;
    }
    
    if( verboseQ )
    {
        TOOLS_DUMP(path_4_foundQ);
        valprint("path_4",PathString(path_4));
    }
    
    // TODO: Figure this out!
    bool paths_intersectQ = true; // Conservative guess.
    
    Int old_crossing_count = pass_1.CrossingCount() + pass_4.CrossingCount() - 1;
    Int new_crossing_count = path_1.CrossingCount() + path_4.CrossingCount() + paths_intersectQ;
    
    if( verboseQ )
    {
        TOOLS_DUMP(old_crossing_count);
        TOOLS_DUMP(new_crossing_count);
    }
    
    if( new_crossing_count >= old_crossing_count )
    {
        return false;
    }

    // TODO: This makes us lose many opportunities.
    if( path_1.CrossingCount() > pass_1.CrossingCount() )
    {
        if( verboseQ )
        {
            print("path_1.CrossingCount() > pass_1.CrossingCount(). Aborting.");
        }
        cleanup();
        return false;
    }
    
    
    if( step < 4 )
    {
        cleanup();
        return false;
    }
    
    if( verboseQ )
    {
        print("Reroute pass_1 to path_1.");
    }

    bool reroute_1_successQ = S.Reroute(pass_1,path_1);
    
    
    if( verboseQ )
    {
        TOOLS_DUMP(reroute_1_successQ);
        TOOLS_DUMP(pd.CrossingCount());
    }
    
    // TODO: Find out why this is necessary!
    pd.ClearCache();
    S.dA_left = pd.ArcLeftDarcs().data();
    
    
    if( !reroute_1_successQ )
    {
        cleanup();
        return false;
    }
    
    PD_ASSERT(pd.CheckAll());
    PD_ASSERT(pd.CheckArcNextArc());
    
    if( step < 5 )
    {
        cleanup();
        return false;
    }
    
    const Int mark_5 = 5;
//    Pass_T pass_5 = pass_4;
//    pass_5.mark = mark_5;
//     
//    pass_5.arc_count = S.MarkArcs(pass_5.first,pass_5.last,pass_5.mark);
    
    // pass_2 and pass_3 should now be merged after we have rerouted pass_1.
    // The problem here is to find what remains of them:
    // pass_2.last, pass_3.first are definitely changed because they lie adjacent to the crossing `crossing`. But pass_2.last goes into that crossing. By the convention I use Reconnect<Head>(pass_2.last,pass_3.first) for rerouting. So arc pass_2.last should still be existent and lie on the pass we search. (pass_3.first can now anywhere in the diagram in the vivinity of the rerouted pass_1.
    
    
    // It should be impossible that pass_2.last is dislocated by the rerouting so far.
    // At least, that is what I think.
    Pass_T pass_5;
    S.template FindPass<true,false,false>( pass_5, pass_2.last, overQ, mark_5 );
    

    if( verboseQ )
    {
        TOOLS_DUMP(pass_4.arc_count);
        TOOLS_DUMP(pass_5.arc_count);
        valprint("pass_5",S.PassString(pass_5));
        print("Finding shortest path for pass_5.");
    }
    
    if( step < 6 )
    {
        cleanup();
        return false;
    }

    
    const Int dual_mark_5 = 5;
    Path_T path_5;
    
    bool path_5_foundQ = S.FindShortestPath(
        pass_5.first, pass_5.last, Scalar::Max<Int>, path_5, dual_mark_5, mark_5
    );
    
    if( verboseQ )
    {
        TOOLS_DUMP(path_5_foundQ);
        valprint("path_5",PathString(path_5));
    }
    
    if( !path_5_foundQ )
    {
        cleanup();
        return false;
    }
    
    if( step < 7 )
    {
        cleanup();
        return false;
    }
    
    bool reroute_5_successQ = S.Reroute(pass_5,path_5);
    
    // TODO: Find out why this is necessary!
    pd.ClearCache();
    S.dA_left = pd.ArcLeftDarcs().data();
    
    if( verboseQ )
    {
        TOOLS_DUMP(reroute_5_successQ);
        TOOLS_DUMP(pd.CrossingCount());
    }

    PD_ASSERT(pd.CheckAll());
    PD_ASSERT(pd.CheckArcNextArc());
    
    if( !reroute_5_successQ )
    {
        cleanup();
        return false;
    }
    
    successQ = true;

    cleanup();
    
    return successQ;
}
