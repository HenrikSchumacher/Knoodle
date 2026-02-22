public:

void RerouteMaximalStrandToShortestPath( const Size_T diagramIdx, const Int a, const bool overQ )
{
    using Pass_T = PassSimplifier_T::Pass_T;
    using Path_T = PassSimplifier_T::Path_T;
    
    PD_T & pd = pd_list[diagramIdx];
    
    PassSimplifier_T & S = PassSimplifier();
    
    S.LoadDiagram(pd);
    S.ResetMarks();
    
    Pass_T pass;
    Path_T path;
    
    const Int mark = 1;
    
    S.template FindPass<true,false,false>( pass, a, overQ, mark );
    
    valprint("pass",ToString(pass));
    valprint("pass array",S.ShortArcRangeString(pass.first,pass.last));
    
    A_Cross_T A = pd.CopyArc(pass.first);
    A_Cross_T B = pd.CopyArc(pass.last);
    
    TOOLS_DUMP(A(Tail) != A(Head));
    TOOLS_DUMP(B(Tail) != B(Head));
    
    TOOLS_DUMP(A(Tail) != B(Tail));
    TOOLS_DUMP(A(Tail) != B(Head));
    TOOLS_DUMP(A(Head) != B(Head));
    TOOLS_DUMP(A(Head) != B(Tail));
    
    const Int dual_mark = 1;
    
    const Int max_dist = pass.arc_count-Int(1);
    
    S.FindShortestPath(
        pass.first, pass.last, max_dist, path, dual_mark, mark
    );
    
    valprint("path",ToString(path.container));
    
    bool successQ = false;
    if( (path.Size() <= Int(0)) || (path.Size() > max_dist) )
    {
        PD_DPRINT("No improvement detected. (pass.arc_count = " + ToString(pass.arc_count) + ", path.Size() = " + ToString(path.Size()) + ", max_dist = " + ToString(max_dist) + ")");
        
        successQ =  false;
    }
    else
    {
        successQ = S.Reroute(pass,path);
    }
    
    TOOLS_DUMP(successQ);
}

void TwoStrandMove( const Size_T diagramIdx, const Int c, const bool overQ )
{
    using Pass_T = PassSimplifier_T::Pass_T;
    using Path_T = PassSimplifier_T::Path_T;
    
    PD_T & pd = pd_list[diagramIdx];
    
    PassSimplifier_T & S = PassSimplifier();
    
    S.LoadDiagram(pd);
    
    S.ResetMarks();
    
    C_Arcs_T C = pd.CopyCrossing(c);
    
    const bool side = pd.ArcOverQ(C(Out,Right),Tail) == overQ;
    const Int a_1 = C(Out, side);
    const Int a_2 = C(In , side);
    const Int a_3 = C(Out,!side);
    
    const Int mark_1 = 1;
    const Int mark_2 = 2;
    const Int mark_3 = 3;
    const Int mark_4 = 4;
    
    Pass_T pass_1;
    Pass_T pass_2;
    Pass_T pass_3;

    S.template FindPass<true,false,false>( pass_1, a_1, overQ, mark_1 );
    S.template FindPass<true,false,false>( pass_2, a_2, overQ, mark_2 );
    S.template FindPass<true,false,false>( pass_3, a_3, overQ, mark_3 );
    
    valprint("pass_1",ToString(pass_1));
    valprint("pass_1 array",S.ShortArcRangeString(pass_1.first,pass_1.last));
    valprint("pass_2",ToString(pass_2));
    valprint("pass_2 array",S.ShortArcRangeString(pass_2.first,pass_2.last));
    valprint("pass_3",ToString(pass_3));
    valprint("pass_3 array",S.ShortArcRangeString(pass_3.first,pass_3.last));
    
    Pass_T pass_4 {
        .first     = pass_2.first,
        .last      = pass_3.last,
        .next      = pass_3.next,
        .arc_count = pass_2.arc_count + pass_3.arc_count,
        .mark      = mark_4,
        .overQ     = overQ,
        .activeQ   = pass_2.activeQ && pass_3.activeQ
    };
    
    TOOLS_DUMP(S.MarkArcs(pass_4.first,pass_4.last,pass_4.mark));

    valprint("pass_4",ToString(pass_4));
    valprint("pass_4 array",S.ShortArcRangeString(pass_4.first,pass_4.last));
    
    Path_T path_1;
    Path_T path_4;
    
    const Int dual_mark_1 = 1;
    
    S.FindShortestPath(
        pass_1.first, pass_1.last, Scalar::Max<Int>, path_1,
        dual_mark_1, mark_1, mark_4
    );
    
    valprint("path_1",ToString(path_1.container));
    
    const Int dual_mark_4 = dual_mark_1 + 1;
    
    S.FindShortestPath(
        pass_4.first, pass_4.last, Scalar::Max<Int>, path_4,
        dual_mark_4, mark_4
    );
    
    valprint("path_4",ToString(path_4.container));
    
//   void FindShortestPath_impl(
//       const Int a, const Int b, const Int max_dist, mref<Path_T> path,
//       const Int dual_mark, HiddenFun_T && hiddenQ, ForbiddenFun_T && forbiddenQ
//   )
    
    bool paths_intersectQ = true;
    
    Int old_crossing_count = (pass_1.arc_count + pass_4.arc_count - 3);
    Int new_crossing_count = (path_1.Size() - 2 + path_4.Size() - 2) + paths_intersectQ;
    
    TOOLS_DUMP(old_crossing_count);
    TOOLS_DUMP(new_crossing_count);
    
    bool successQ = S.Reroute(pass_1,path_1);
    
    TOOLS_DUMP(successQ);
}
