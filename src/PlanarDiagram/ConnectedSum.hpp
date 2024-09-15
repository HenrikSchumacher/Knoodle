Int ConnectedSummandCount()
{
    return connected_sum_counter;
}

bool ConnectedSum()
{
    Tiny::VectorList<2,Int,Int> duplicates_arcs = DuplicateDualArcs();
    
//    cptr<Int> A [2] = { duplicates_arcs.data(0), duplicates_arcs.data(1) };
    
    const Int n = duplicates_arcs.Dimension(1);
    
    Int counter = 0;
    
    for( Int i = 0; i < n; ++i )
    {
        const Int a_0 = duplicates_arcs[0][i];
        const Int a_1 = duplicates_arcs[1][i];
        
        ++counter;
        
        SplitOffConnectedSum(a_0,a_1);
    }
    
    PD_PRINT( "Split off connected sums        = " + ToString(counter));
    
    if( counter > 0 )
    {
        faces_initialized = false;
        return true;
    }
    else
    {
        return false;
    }
}


void SplitOffConnectedSum( const Int a_0, const Int a_1 )
{
    PD_ASSERT(a_0!=a_1);
    PD_ASSERT(ArcActiveQ(a_0));
    PD_ASSERT(ArcActiveQ(a_1));
    
    // Make a copy of the previous state.
    
    const Int C [2][2] = {
        {A_cross(a_0,0),A_cross(a_1,0)},
        {A_cross(a_0,1),A_cross(a_1,1)},
    };
    
// C[Tail][0]           a_0           C[Head][0]
//           X----------->-----------X
//
//
//           X-----------<-----------X
// C[Head][1]           a_1           C[Tail][1]
    
    PD_ASSERT(CrossingActiveQ(C[0][0]));
    PD_ASSERT(CrossingActiveQ(C[0][1]));
    PD_ASSERT(CrossingActiveQ(C[1][0]));
    PD_ASSERT(CrossingActiveQ(C[1][1]));
    
    
    if(
        (
            (C_arcs(C[Head][0],In,Left ) == a_0)
            ||
            (C_arcs(C[Head][0],In,Right) == a_0)
        )
        &&
        (
            (C_arcs(C[Head][1],In,Left ) == a_1)
            ||
            (C_arcs(C[Head][1],In,Right) == a_1)
        )
    )
    {
        
        const Int side_0 = (C_arcs(C[Head][0],In,Left) == a_0) ? Left : Right;
        const Int side_1 = (C_arcs(C[Head][1],In,Left) == a_1) ? Left : Right;
        
        A_cross(a_0,Head) = C[Head][1];
        A_cross(a_1,Head) = C[Head][0];

        C_arcs(C[Head][0],In,side_0) = a_1;
        C_arcs(C[Head][1],In,side_1) = a_0;
        
        touched_crossings.push_back(C[Head][0]);
        touched_crossings.push_back(C[Head][1]);
        
        touched_crossings.push_back(C[Tail][0]);
        touched_crossings.push_back(C[Tail][1]);
        
        ++connected_sum_counter;
        
//        CheckAll();
    }

}
