bool CheckCrossing( const Int c  )
{
    if( (c < 0) || (c > initial_crossing_count) )
    {
        eprint(ClassName()+"::CheckCrossing: Crossing index c = " + Tools::ToString(c) + " is out of bounds.");
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
            
            if( (a < 0) || (a > initial_arc_count) )
            {
                eprint(ClassName()+"::CheckArc: Arc index a = " + Tools::ToString(a) + " in crossing " + CrossingString(c) + " is out of bounds.");
                return false;
            }
            
            const int A_activeQ = (A_state[a] == ArcState::Active);
            
            if( !A_activeQ )
            {
                eprint(ClassName()+"::CheckCrossing: arc " + ArcString(a) + " attached to active crossing " + CrossingString(c) + "is not active.");
            }
            
            const bool tailtip = ( io == In ) ? Head : Tail;
            
            const bool A_goodQ = (A_cross(a,tailtip) == c);
            
            if( !A_goodQ )
            {
                eprint(ClassName()+"::CheckCrossing: arc " + ArcString(a) + " is not properly attached to crossing " + CrossingString(c) + ".");
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

bool CheckAllCrossings()
{
    bool passedQ = true;
    
    for( Int c = 0; c < initial_crossing_count; ++c )
    {
        passedQ = passedQ && CheckCrossing(c);
    }
    
    if( passedQ )
    {
        logprint(ClassName() + "::CheckAllCrossings: passed.");
    }
    else
    {
        eprint(ClassName() + "::CheckAllCrossings: failed.");
    }
    return passedQ;
}


bool CheckArc( const Int a  )
{
    if( (a < 0) || (a > initial_arc_count) )
    {
        eprint(ClassName()+"::CheckArc: Arc index a = " + Tools::ToString(a) + " is out of bounds.");
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
        
        if( (c < 0) || (c > initial_crossing_count) )
        {
            eprint(ClassName()+"::CheckArc: Crossing index c = " + Tools::ToString(c) + " in arc " + ArcString(a) + " is out of bounds.");
            return false;
        }
        
        const bool C_activeQ = CrossingActiveQ(c);
        
        
        if( !C_activeQ )
        {
            eprint(ClassName()+"::CheckArc: crossing " + CrossingString(c) + " in active arc " + ArcString(a) + "is not active.");
        }
        const bool inout = (headtail == Tail) ? Out : In;
    
        const bool C_goodQ = ( (C_arcs(c,inout,Left) == a) || (C_arcs(c,inout,Right) == a) );
        
        if( !C_goodQ )
        {
            eprint(ClassName()+"::CheckArc: crossing " + CrossingString(c) + " appears in crossing " + ArcString(a) + ", but it is not properly attached to it.");
        }
        
        A_passedQ = A_passedQ && C_activeQ && C_goodQ;
        
    }
    
//    if( !A_passedQ )
//    {
//        eprint( ClassName() + "::CheckArc: Arc "+ToString(a)+" failed to pass.");
//    }
    
    return A_passedQ;
}

bool CheckAllArcs()
{
    bool passed = true;
    
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        passed = passed && CheckArc(a);
    }
    
    if( passed )
    {
        logprint(ClassName()+"::CheckAllArcs: passed.");
    }
    else
    {
        eprint(ClassName()+"::CheckAllArcs: failed.");
    }
    
    return passed;
}

bool CheckAll()
{
    const bool passed = CheckAllCrossings() && CheckAllArcs();

    return passed;
}
