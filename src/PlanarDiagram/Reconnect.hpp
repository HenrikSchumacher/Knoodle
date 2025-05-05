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
    PD_DPRINT(std::string("Reconnect<") + (headtail ? "Head" : "Tail") +  ">(" + ArcString(a) + ", " +ArcString(b) + " )" );

    // Read:
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivates b.

    PD_ASSERT(a != b);
    PD_ASSERT(ArcActiveQ(a));
    
    const Int c = A_cross(b,headtail);

    A_cross(a,headtail) = c;
    
    SetMatchingPortTo<headtail>(c,b,a);

    // TODO: Handle over/under in ArcState.
    if constexpr( deactivateQ )
    {
        DeactivateArc(b);
    }
}

template<bool assertQ = true>
void Reconnect( const Int a, const bool headtail, const Int b )
{
    PD_DPRINT("Reconnect ( " + ArcString(a) + ", " + (headtail ? "Head" : "Tail") + ", " +ArcString(b) + " )" );
    
    // Read:
    // Reconnect arc a with its tip/tail to where b pointed/started.
    // Then deactivates b.
    
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
    
    // TODO: Handle over/under in ArcState.
    SetMatchingPortTo(c,headtail,b,a);
    
    DeactivateArc(b);
}
