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
            

            const bool A_activeQ = ArcActiveQ(a);
            
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
    
    if( !ActiveQ(A_state[a]) )
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
    
//    const Int a_next = NextArc(a,Head);
//    
//    if( !InIntervalQ(a,Int(0),max_arc_count) )
//    {
//        eprint(ClassName()+"::CheckArc: Next arc of " + ArcString(a) + " is out of bounds.");
//        A_passedQ = false;
//    }
//    else if( !ArcActiveQ(a) )
//    {
//        eprint(ClassName()+"::CheckArc: Next arc of " + ArcString(a) + " is inactive.");
//        A_passedQ = false;
//    }
//    else if( A_color[a] != A_color[a_next] )
//    {
//        eprint(ClassName()+"::CheckArc: Color of next arc of " + ArcString(a) + " does not match.");
//        A_passedQ = false;
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
        eprint(MethodName("CheckAllArcs")+": arc_count < 0.");
        passedQ = false;
    }
    
    if( arc_count > max_arc_count )
    {
        eprint(MethodName("CheckAllArcs")+": arc_count > max_arc_count.");
        passedQ = false;
    }
    
    Int active_arc_count = 0;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        active_arc_count += ArcActiveQ(a);
        passedQ = passedQ && CheckArc(a);
    }
    
    if( arc_count != active_arc_count )
    {
        eprint(MethodName("CheckAllArcs")+": arc_count != active_arc_count.");
        TOOLS_LOGDUMP(arc_count);
        TOOLS_LOGDUMP(active_arc_count);
        TOOLS_LOGDUMP(max_arc_count);
        passedQ = false;
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
    const bool passedQ = CheckAllCrossings() && CheckAllArcs() && CheckVertexDegrees() && CheckArcDegrees() && CheckArcColors();

    return passedQ;
}


public:

template<bool must_be_activeQ>
void AssertDarc( const Int da ) const
{
#ifdef PD_DEBUG
        auto [a,d] = FromDarc(da);
    
        if( !InIntervalQ(a,Int(0),max_arc_count) )
        {
            TOOLS_LOGDUMP(max_arc_count);
            pd_eprint("AssertDarc<1>: Arc index " + Tools::ToString(a) + " is out of bounds.");
        }
    
        if constexpr( must_be_activeQ )
        {
            if( !ArcActiveQ(a) )
            {
                pd_eprint("AssertDarc<1>: " + DarcString(da) + " is not active.");
            }
            if( !CheckArc(a) )
            {
                pd_eprint("AssertDarc<1>: " + DarcString(da) + " failed CheckArc.");
            }
        }
        else
        {
            if( ArcActiveQ(a) )
            {
                pd_eprint("AssertDarc<0>: " + DarcString(da) + " is not inactive.");
            }
        }
#else
    (void)da;
#endif
}

template<bool must_be_activeQ = true>
void AssertArc( const Int a ) const
{
#ifdef PD_DEBUG
        if( !InIntervalQ(a,Int(0),max_arc_count) )
        {
            TOOLS_LOGDUMP(max_arc_count);
            pd_eprint("AssertArc<1>: Arc index " + Tools::ToString(a) + " is out of bounds.");
        }
    
        if constexpr( must_be_activeQ )
        {
            if( !ArcActiveQ(a) )
            {
                pd_eprint("AssertArc<1>: " + ArcString(a) + " is not active.");
            }
            if( !CheckArc(a) )
            {
                pd_eprint("AssertArc<1>: " + ArcString(a) + " failed CheckArc.");
            }
        }
        else
        {
            if( ArcActiveQ(a) )
            {
                pd_eprint("AssertArc<0>: " + ArcString(a) + " is not inactive.");
            }
        }
#else
        (void)a;
#endif
}

template<bool must_be_activeQ = true>
void AssertCrossing( const Int c ) const
{
#ifdef PD_DEBUG
    if( !InIntervalQ(c,Int(0),max_crossing_count) )
    {
        TOOLS_LOGDUMP(max_crossing_count);
        pd_eprint("AssertCrossing<1>: Crossing index " + Tools::ToString(c) + " is out of bounds.");
    }
    
    if constexpr( must_be_activeQ )
    {
        if( !CrossingActiveQ(c) )
        {
            pd_eprint("AssertCrossing<1>: " + CrossingString(c) + " is not active.");
        }
        if( !CheckCrossing(c) )
        {
            pd_eprint("AssertCrossing<1>: " + CrossingString(c) + " failed CheckCrossing.");
        }
    }
    else
    {
        if( CrossingActiveQ(c) )
        {
            pd_eprint("AssertCrossing<0>: " + CrossingString(c) + " is not inactive.");
        }
    }
#else
    (void)c;
#endif
}


bool LoopFreeQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) && (A_cross(a,Tail) == A_cross(a,Head)) )
        {
            return false;
        }
    }
    return true;
}

bool CheckProvenMinimalQ() const
{
    if( proven_minimalQ )
    {
        PD_T pd = this->CachelessCopy();
        
        bool alternatingQ = AlternatingQ();
        
        if( !alternatingQ )
        {
            wprint(MethodName("CheckProvenMinimalQ") + ": Diagram is not alternating.");
        }
        
        bool loop_freeQ = LoopFreeQ();
        
        if( !loop_freeQ )
        {
            wprint(MethodName("CheckProvenMinimalQ") + ": Diagram is not loop free.");
        }
        
        return alternatingQ && loop_freeQ;
    }
    
    return true;
}
