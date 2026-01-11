/*! @brief Checks the arc states, assuming that the activity status of each arc is correct.
 */

bool CheckArcState( const Int a ) const
{
    if( !ArcActiveQ(a) )
    {
        return true;
    }
    
    for( bool headtail : {Tail,Head} )
    {
        if( ArcSide(a,headtail) != ArcSide_Reference(a,headtail) )
        {
            eprint(MethodName("CheckArcState")+": ArcSide(a," + (headtail ? "Head" : "Tail") +") of " + ArcString(a) + " is incorrect.");
            return false;
        }
    }
    for( bool headtail : {Tail,Head} )
    {
        if( ArcRightHandedQ(a,headtail) != ArcRightHandedQ_Reference(a,headtail) )
        {
            eprint(MethodName("CheckArcState")+": ArcRightHandedQ(a," + (headtail ? "Head" : "Tail") +") is incorrect.");
            return false;
        }
    }
    for( bool headtail : {Tail,Head} )
    {
        if( ArcLeftHandedQ(a,headtail) != ArcLeftHandedQ_Reference(a,headtail) )
        {
            eprint(MethodName("CheckArcState")+": ArcLeftHandedQ(a," + (headtail ? "Head" : "Tail") +") is incorrect.");
            return false;
        }
    }
    for( bool headtail : {Tail,Head} )
    {
        if( ArcOverQ(a,headtail) != ArcOverQ_Reference(a,headtail) )
        {
            eprint(MethodName("CheckArcState")+": ArcOverQ(a," + (headtail ? "Head" : "Tail") +") is incorrect.");
            return false;
        }
    }
    for( bool headtail : {Tail,Head} )
    {
        if( ArcUnderQ(a,headtail) != ArcUnderQ_Reference(a,headtail) )
        {
            eprint(MethodName("CheckArcState")+": ArcUnderQ(a," + (headtail ? "Head" : "Tail") +") is incorrect.");
            return false;
        }
    }
    
    return true;
}

bool CheckArcStates() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !CheckArcState(a) )
        {
            return false;
        }
    }
    
    return true;
}


/*! @brief Computes the arc states, assuming that the activity status of each arc is correct.
 */

void ComputeArcStates()
{
    ClearCache();

    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);

            A_state[a].Set(Tail,ArcSide_Reference(a,Tail),C_state[c_0]);
            A_state[a].Set(Head,ArcSide_Reference(a,Head),C_state[c_1]);
        }
    }
}


Int LeftDarc( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int  c    = A_cross(a,d);
    const bool side = ArcSide(a,d);
    AssertCrossing(c);
    
    
    /*
     *   O     O    d    == Head  == Right
     *    ^   ^     side == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,side,!d);
    AssertArc(b);
    
    return ToDarc(b,!side);
}


Int RightDarc( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int  c    = A_cross(a,d);
    const bool side = ArcSide(a,d);
    AssertCrossing(c);
    
    /*
     *   O     O    d    == Head  == Right
     *    ^   ^     side == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,!side,d);
    AssertArc(b);
    
    return ToDarc(b,side);
}




bool ArcSide( const Int a, const bool headtail )  const
{
    return A_state[a].Side(headtail);
}

bool ArcRightHandedQ( const Int a, const bool headtail )  const
{
    return A_state[a].RightHandedQ(headtail);
}

bool ArcLeftHandedQ( const Int a, const bool headtail )  const
{
    return A_state[a].LeftHandedQ(headtail);
}

bool ArcUnderQ( const Int a, const bool headtail )  const
{
    return A_state[a].UnderQ(headtail);
}

bool ArcOverQ( const Int a, const bool headtail )  const
{
    return A_state[a].OverQ(headtail);
}
