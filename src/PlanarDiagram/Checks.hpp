bool CheckCrossing( const Int c  ) const
{
    if( c == Uninitialized )
    {
        return true;
    }
    
    if( !InIntervalQ(c,Int(0),max_crossing_count) )
    {
        eprint(ClassName()+"::CheckCrossing: Crossing index c = " + Tools::ToString(c) + " is out of bounds.");
        TOOLS_LOGDUMP(max_crossing_count);
        return false;
    }
    
    if( !CrossingActiveQ(c) )
    {
        return true;
    }
    
    // Check whether all arcs of crossing c are active and have the correct connectivity to c.
    bool C_passedQ = true;
    
    for( bool io : { In, Out } )
    {
        for( bool lr : { Left, Right } )
        {
            const Int a = C_arcs(c,io,lr);

            if( !InIntervalQ(a,Int(0),max_arc_count) )
            {
                eprint(ClassName()+"::CheckCrossing: Arc index a = " + Tools::ToString(a) + " in " + CrossingString(c) + " is out of bounds.");
                TOOLS_LOGDUMP(max_arc_count);
                return false;
            }
            
//            const int A_activeQ = (A_state[a] == ArcState::Active) || (A_state[a] == ArcState::Unchanged);
            
            const bool A_activeQ = ToUnderlying(A_state[a]) & Underlying_T<ArcState>(1);
            
            if( !A_activeQ )
            {
                eprint(ClassName()+"::CheckCrossing: " + ArcString(a) + " attached to active " + CrossingString(c) + " is not active.");
            }
            
            const bool tailtip = ( io == In ) ? Head : Tail;
            
            const bool A_goodQ = (A_cross(a,tailtip) == c);
            
            if( !A_goodQ )
            {
                eprint(ClassName()+"::CheckCrossing: " + ArcString(a) + " is not properly attached to " + CrossingString(c) + ".");
            }
            C_passedQ = C_passedQ && A_activeQ && A_goodQ;
        }
    }
    
//    if( !C_passedQ )
//    {
//        eprint(ClassName()+"::CheckCrossing: Crossing "+ToString(c)+" failed to pass.");
//    }
    
    return C_passedQ;
}

bool CheckAllCrossings() const
{
    bool passedQ = true;

    if( max_crossing_count < Int(0) )
    {
        eprint(ClassName()+"::CheckAllCrossings: max_crossing_count < 0.");
        passedQ = false;
    }
    
    if( crossing_count < Int(0) )
    {
        eprint(ClassName()+"::CheckAllCrossings: crossing_count < 0.");
        passedQ = false;
    }
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        passedQ = passedQ && CheckCrossing(c);
    }
    
    if( passedQ )
    {
        logprint(ClassName()+"::CheckAllCrossings: passed.");
    }
    else
    {
        eprint(ClassName()+"::CheckAllCrossings: failed.");
    }
    return passedQ;
}


bool CheckArc( const Int a ) const
{
    if( a == Uninitialized )
    {
        return true;
    }
    
    if( !InIntervalQ(a,Int(0),max_arc_count) )
    {
        eprint(ClassName()+"::CheckArc: Arc index a = " + Tools::ToString(a) + " is out of bounds.");
        TOOLS_LOGDUMP(max_arc_count);
        return false;
    }
    
    if( A_state[a] == ArcState::Inactive )
    {
        return true;
    }
    
    // Check whether the two crossings of arc a are active and have the correct connectivity to c.
    bool A_passedQ = true;
    

    for( bool headtail : {Tail, Head} )
    {
        const Int c = A_cross(a,headtail);
        
        if( !InIntervalQ(c,Int(0),max_crossing_count) )
        {
            eprint(ClassName()+"::CheckArc: Crossing index c = " + Tools::ToString(c) + " in arc " + ArcString(a) + " is out of bounds.");
            TOOLS_LOGDUMP(max_crossing_count);
            return false;
        }
        
        const bool C_activeQ = CrossingActiveQ(c);
        
        
        if( !C_activeQ )
        {
            eprint(ClassName()+"::CheckArc: " + CrossingString(c) + " in active " + ArcString(a) + " is not active.");
        }
        const bool inout = (headtail == Tail) ? Out : In;
    
        const bool C_goodQ = ( (C_arcs(c,inout,Left) == a) || (C_arcs(c,inout,Right) == a) );
        
        if( !C_goodQ )
        {
            eprint(ClassName()+"::CheckArc: " + CrossingString(c) + " appears in " + ArcString(a) + ", but it is not properly attached to it.");
        }
        
        A_passedQ = A_passedQ && C_activeQ && C_goodQ;
        
    }
    
//    if( !A_passedQ )
//    {
//        eprint( ClassName()+"::CheckArc: Arc "+ToString(a)+" failed to pass.");
//    }
    
    return A_passedQ;
}

bool CheckAllArcs() const
{
    bool passedQ = true;
    
    if( max_arc_count < Int(0) )
    {
        eprint(ClassName()+"::CheckAllArcs: max_arc_count < 0.");
        passedQ = false;
    }
    
    if( arc_count < Int(0) )
    {
        eprint(ClassName()+"::CheckAllArcs: arc_count < 0.");
        passedQ = false;
    }
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        passedQ = passedQ && CheckArc(a);
    }
    
    if( passedQ )
    {
        logprint(ClassName()+"::CheckAllArcs: passed.");
    }
    else
    {
        eprint(ClassName()+"::CheckAllArcs: failed.");
    }
    
    return passedQ;
}

bool CheckVertexDegrees() const
{
    bool passed = true;
    
    Tensor1<Int,Int> d (max_crossing_count,Int(0));
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            ++d[A_cross(a,Tail)];
            ++d[A_cross(a,Head)];
        }
    }
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( CrossingActiveQ(c) )
        {
            if( d[c] != Int(4) )
            {
                passed = false;
                eprint( ClassName()+"::CheckVertexDegrees: degree of " + CrossingString(c) + " is " + Tools::ToString(d[c]) + " != 4.");
            }
        }
        else
        {
            if( d[c] != Int(0) )
            {
                passed = false;
                eprint( ClassName()+"::CheckVertexDegrees: degree of " + CrossingString(c) + " is " + Tools::ToString(d[c]) + " != 0.");
            }
        }
    }

    return passed;
}

bool CheckArcDegrees() const
{
    bool passed = true;
    
    Tensor1<Int,Int> d (max_arc_count,Int(0));
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( CrossingActiveQ(c) )
        {
            ++d[C_arcs(c,Out,Left )];
            ++d[C_arcs(c,Out,Right)];
            ++d[C_arcs(c,In ,Left )];
            ++d[C_arcs(c,In ,Right)];
        }
    }
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) )
        {
            if( d[a] != Int(2) )
            {
                passed = false;
                eprint( ClassName()+"::CheckArcDegrees: degree of " + ArcString(a) + " is " + Tools::ToString(d[a]) + " != 2.");
            }
        }
        else
        {
            if( d[a] != Int(0) )
            {
                passed = false;
                eprint( ClassName()+"::CheckArcDegrees: degree of " + ArcString(a) + " is " + Tools::ToString(d[a]) + " != 0.");
            }
        }
    }

    return passed;
}


bool CheckAll() const
{
    const bool passedQ = CheckAllCrossings() && CheckAllArcs() && CheckVertexDegrees() && CheckArcDegrees();

    return passedQ;
}


public:

void AssertDarc( const Int da ) const
{
    PD_ASSERT(CheckArc  (FromDarc(da).first));
    PD_ASSERT(ArcActiveQ(FromDarc(da).first));
#ifndef PD_DEBUG
    (void)da;
#endif
}

void AssertArc( const Int a ) const
{
    PD_ASSERT(CheckArc  (a));
    PD_ASSERT(ArcActiveQ(a));
#ifndef PD_DEBUG
    (void)a;
#endif
}

void AssertCrossing( const Int c ) const
{
    PD_ASSERT(InIntervalQ(c,Int(0),max_crossing_count));
    PD_ASSERT(CrossingActiveQ(c));
    PD_ASSERT(CheckCrossing(c));
#ifndef PD_DEBUG
    (void)c;
#endif
}


