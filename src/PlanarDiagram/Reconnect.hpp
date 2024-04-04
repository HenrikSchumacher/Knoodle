void Reconnect( const Int a, const bool tiptail, const Int b )
{
    // Read: 
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivate b.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    const bool io = (tiptail==Tip) ? In : Out;
    
    PD_assert( a != b );
    
    PD_assert( ArcActiveQ(a) );
    PD_assert( ArcActiveQ(b) );
    
    const Int c = A_cross(b, tiptail);
    
    PD_assert( (C_arcs(c,io,Left) == b) || (C_arcs(c,io,Right) == b) );
    
    PD_assert(CheckArc(b));
    PD_assert(CheckCrossing(c));
    
    PD_assert( CrossingActiveQ(c) );
    PD_assert( CrossingActiveQ(A_cross(a, tiptail)) );
    PD_assert( CrossingActiveQ(A_cross(a,!tiptail)) );
    
    A_cross(a,tiptail) = c;

    const bool lr = (C_arcs(c,io,Left) == b) ? Left : Right;
    
    C_arcs(c,io,lr) = a;
    
    touched_crossings.push_back(c);
//            touched_crossings.push_back(A_cross(a,Tip));
}

