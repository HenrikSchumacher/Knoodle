void Reconnect( const Int a, const bool headtail, const Int b )
{
    // Read: 
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivates b.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    const bool io = (headtail==Head) ? In : Out;
    
    PD_ASSERT( a != b );
    
    PD_ASSERT( ArcActiveQ(a) );
//    PD_ASSERT( ArcActiveQ(b) ); // Could have been deactivated already.
    
    const Int c = A_cross(b, headtail);
    
    PD_ASSERT( (C_arcs(c,io,Left) == b) || (C_arcs(c,io,Right) == b) );
    
    PD_ASSERT(CheckArc(b));
    PD_ASSERT(CheckCrossing(c));
    
    PD_ASSERT(CrossingActiveQ(c));
    PD_ASSERT(CrossingActiveQ(A_cross(a, headtail)));
    PD_ASSERT(CrossingActiveQ(A_cross(a,!headtail)));
    
    A_cross(a,headtail) = c;

    const bool lr = (C_arcs(c,io,Left) == b) ? Left : Right;
    
    C_arcs(c,io,lr) = a;
    
#if defined(PD_USE_TOUCHING)
    //touched_crossings.push_back(a);
    touched_crossings.push_back(c);
#endif
    
    DeactivateArc(b);
}


void Reconnect( ArcView & A, const bool headtail, ArcView & B )
{
    // Read:
    // Reconnect arc A with its tip/tail to where B pointed/started.
    // Then deactivates B.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    const bool io = (headtail==Head) ? In : Out;
    
    PD_ASSERT( A.Idx() != B.Idx() );
    
    PD_ASSERT( A.ActiveQ() );
//    PD_ASSERT( B.ActiveQ() ); // Could have been deactivated already.
    
    auto C = Crossing( B(headtail) );
    
    
    PD_ASSERT( (C(io,Left) == B.Idx()) || (C(io,Right) == B.Idx()) );
    
    PD_ASSERT(CheckArc(B.Idx()));
    PD_ASSERT(CheckCrossing(C.Idx()));
    
    PD_ASSERT(C.ActiveQ());
    PD_ASSERT(CrossingActiveQ(A( headtail)));
    PD_ASSERT(CrossingActiveQ(A(!headtail)));
    
    
    A(headtail) = C.Idx();

    const bool lr = (C(io,Left) == B.Idx()) ? Left : Right;
    
    C(io,lr) = A.Idx();
    
#if defined(PD_USE_TOUCHING)
    //touched_crossings.push_back(a);
    touched_crossings.push_back(C.Idx());
#endif
    
    DeactivateArc(B.Idx());
}


