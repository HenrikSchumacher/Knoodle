template<bool io>
void SetMatchingPortTo( const Int c, const Int a, const Int b )
{
    mptr<Int> C = C_arcs.data(c,io);
    
    PD_ASSERT( (C[Left] == a) || (C[Right] == a) );
    
    C[C[Right] == a] = b;
}

void SetMatchingPortTo( const Int c, const bool io, const Int a, const Int b )
{
    mptr<Int> C = C_arcs.data(c,io);
    
    PD_ASSERT( (C[Left] == a) || (C[Right] == a) );
    
    C[C[Right] == a] = b;
}

// Often we call Reconnect with `headtail` known at compile time. Can we exploit this by defining the following.
template<bool headtail, bool deactivateQ = true>
void Reconnect( const Int a, const Int b )
{
//    PD_DPRINT(std::string("Reconnect<") + (headtail ? "Head" : "Tail") +  ">(" + ArcString(a) + ", " +  ", " +ArcString(b) + " )" );

    // Read:
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivates b.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.

    const Int c = A_cross(b,headtail);

    A_cross(a,headtail) = c;
    
    SetMatchingPortTo<headtail>(c,b,a);

    if constexpr( deactivateQ )
    {
        DeactivateArc(b);
    }
}

template<bool assertQ = true>
void Reconnect( const Int a, const bool headtail, const Int b )
{
//    PD_DPRINT("Reconnect ( " + ArcString(a) + ", " + (headtail ? "Head" : "Tail") + ", " +ArcString(b) + " )" );
    
    // Read:
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivates b.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    PD_ASSERT(a != b);
    
    PD_ASSERT(ArcActiveQ(a));
//    PD_assert( ArcActiveQ(b) ); // Could have been deactivated already.
    
    const Int c = A_cross(b,headtail);
    
    // This is a hack to suppress warnings in situations where we cannot guarantee that c is intact (but where we can guarantee that it will finally be deactivated.
    if constexpr ( assertQ )
    {
        AssertCrossing(c);
        
        PD_ASSERT(CheckArc(b));
        
        PD_ASSERT(CrossingActiveQ(A_cross(a, headtail)));
        PD_ASSERT(CrossingActiveQ(A_cross(a,!headtail)));
    }

    A_cross(a,headtail) = c;
    
//    mptr<Int> C = C_arcs.data(c,headtail);
//
//    PD_ASSERT( (C[Left] == b) || (C[Right] == b) );
//    
//    C[C[Right] == b] = a;
    
    SetMatchingPortTo(c,headtail,b,a);
    
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
    
    PD_ASSERT( A.Idx() != B.Idx() );
    
    PD_ASSERT( A.ActiveQ() );
//    PD_ASSERT( B.ActiveQ() ); // Could have been deactivated already.
    
    auto C = GetCrossing( B(headtail) );
    
    PD_ASSERT( (C(headtail,Left) == B.Idx()) || (C(headtail,Right) == B.Idx()) );
    
    PD_ASSERT(CheckArc(B.Idx()));
    PD_ASSERT(CheckCrossing(C.Idx()));
    
    PD_ASSERT(C.ActiveQ());
    PD_ASSERT(CrossingActiveQ(A( headtail)));
    PD_ASSERT(CrossingActiveQ(A(!headtail)));
    
    
    A(headtail) = C.Idx();

    const bool side = (C(headtail,Right) == B.Idx());
    
    C(headtail,side) = A.Idx();
    
    DeactivateArc(B.Idx());
}


template<bool headtail>
void Reconnect( ArcView & A, ArcView & B )
{
    // Read:
    // Reconnect arc A with its tip/tail to where B pointed/started.
    // Then deactivates B.
    //
    // Also keeps track of crossings that got touched and that might thus
    // be interesting for further simplification.
    
    PD_ASSERT( A.Idx() != B.Idx() );
    
    PD_ASSERT( A.ActiveQ() );
//    PD_ASSERT( B.ActiveQ() ); // Could have been deactivated already.
    
    auto C = GetCrossing( B(headtail) );
    
    
    PD_ASSERT( (C(headtail,Left) == B.Idx()) || (C(headtail,Right) == B.Idx()) );
    
    PD_ASSERT(CheckArc(B.Idx()));
    PD_ASSERT(CheckCrossing(C.Idx()));
    
    PD_ASSERT(C.ActiveQ());
    PD_ASSERT(CrossingActiveQ(A( headtail)));
    PD_ASSERT(CrossingActiveQ(A(!headtail)));
    
    
    A(headtail) = C.Idx();

    const bool side = (C(headtail,Right) == B.Idx());
    
    C(headtail,side) = A.Idx();
    
    DeactivateArc(B.Idx());
}
