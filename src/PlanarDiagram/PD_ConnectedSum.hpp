Int ConnectedSummandCount()
{
    return connected_sum_counter;
}

bool ConnectedSum()
{
    Tiny::VectorList<2,Int,Int> duplicates_arcs = DuplicateDualArcs();
    
    cptr<Int> A [2] = { duplicates_arcs.data(0), duplicates_arcs.data(1) };
    
    const Int n = duplicates_arcs.Dimension(1);
    
    Int counter = 0;
    
    for( Int i = 0; i < n; ++i )
    {
        const Int a_0 = A[0][i];
        const Int a_1 = A[1][i];
        
        ++counter;
        SplitOffConnectedSum(a_0,a_1);
    }
    
    PD_print( "Split off connected sums        = " + ToString(counter));
    
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
// C[Tail][0]           a_0           C[Tip ][0]
//           X----------->-----------X
//
//
//           X-----------<-----------X
// C[Tip ][1]           a_1           C[Tail][1]
    
    PD_assert(a_0!=a_1);
    PD_assert(ArcActive(a_0));
    PD_assert(ArcActive(a_1));
    
    // Make a copy of the previous state.
    const Int C [2][2] = {
        {A_crossings[0][a_0],A_crossings[0][a_1]},
        {A_crossings[1][a_0],A_crossings[1][a_1]},
    };
    
//    print( ArcString(a_0));
//    print( ArcString(a_1));
    
    PD_assert(CrossingActive(C[0][0]));
    PD_assert(CrossingActive(C[0][1]));
    PD_assert(CrossingActive(C[1][0]));
    PD_assert(CrossingActive(C[1][1]));
    
//    print( CrossingString(C[Tip][0]) );
//    print( CrossingString(C[Tip][1]) );
    
//    PD_assert( (C_arcs[_in][Left][c_0] == a_0) || (C_arcs[_in][Right][c_0] == a_0) );
//    PD_assert( (C_arcs[_in][Left][c_1] == a_1) || (C_arcs[_in][Right][c_1] == a_1) );
    
    if(
       ( (C_arcs[_in][Left][ C[Tip][0] ] == a_0) || (C_arcs[_in][Right][ C[Tip][0] ] == a_0) )
       &&
       ( (C_arcs[_in][Left][ C[Tip][1] ] == a_1) || (C_arcs[_in][Right][ C[Tip][1] ] == a_1) )
       )
    {
        
        const Int side_0 = (C_arcs[_in][Left][ C[Tip][0] ] == a_0) ? Left : Right;
        const Int side_1 = (C_arcs[_in][Left][ C[Tip][1] ] == a_1) ? Left : Right;
        
        A_crossings[Tip][a_0] = C[Tip][1];
        A_crossings[Tip][a_1] = C[Tip][0];

        C_arcs[_in][side_0][ C[Tip][0] ] = a_1;
        C_arcs[_in][side_1][ C[Tip][1] ] = a_0;
        
        touched_crossings.push_back(C[Tip ][0]);
        touched_crossings.push_back(C[Tip ][1]);
        
        touched_crossings.push_back(C[Tail][0]);
        touched_crossings.push_back(C[Tail][1]);
        
        ++connected_sum_counter;
        
//        CheckAll();
    }

}
