template<bool assertQ = true>
void Reconnect( const Int a, const bool tiptail, const Int b )
{
    PD_DPRINT("Reconnect ( " + ArcString(a) + ", " + (tiptail ? "Head" : "Tail") + ", " +ArcString(b) + " )" );
    
    // Read:
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivates b.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
//    const bool io = (tiptail==Head) ? In : Out;
    const bool io = tiptail;
    
    PD_ASSERT( a != b );
    
    PD_ASSERT( ArcActiveQ(a) );
//    PD_assert( ArcActiveQ(b) ); // Could have been deactivated already.
    
    const Int c = A_cross(b, tiptail);
    
    // This is a hack to suppress warnings in situations where we cannot guarantee that c is intact (but where we can guarantee that it will finally be deactivated.
    if constexpr ( assertQ )
    {
        AssertCrossing(c);
        
        PD_ASSERT(CheckArc(b));
        
        PD_ASSERT( CrossingActiveQ(A_cross(a, tiptail)) );
        PD_ASSERT( CrossingActiveQ(A_cross(a,!tiptail)) );
    }

    
    A_cross(a,tiptail) = c;

    PD_ASSERT( (C_arcs(c,io,Left) == b) || (C_arcs(c,io,Right) == b) );

//    const bool side = (C_arcs(c,io,Left) == b) ? Left : Right;
//    const bool side = (C_arcs(c,io,Left) != b);
    const bool side = (C_arcs(c,io,Right) == b);
    
    C_arcs(c,io,side) = a;
    
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
    
    auto C = GetCrossing( B(headtail) );
    
    
    PD_ASSERT( (C(io,Left) == B.Idx()) || (C(io,Right) == B.Idx()) );
    
    PD_ASSERT(CheckArc(B.Idx()));
    PD_ASSERT(CheckCrossing(C.Idx()));
    
    PD_ASSERT(C.ActiveQ());
    PD_ASSERT(CrossingActiveQ(A( headtail)));
    PD_ASSERT(CrossingActiveQ(A(!headtail)));
    
    
    A(headtail) = C.Idx();

    const bool lr = (C(io,Left) == B.Idx()) ? Left : Right;
    
    C(io,lr) = A.Idx();
    
#ifdef PD_USE_TOUCHING
    //touched_crossings.push_back(a);
    touched_crossings.push_back(C.Idx());
#endif
    
    DeactivateArc(B.Idx());
}


