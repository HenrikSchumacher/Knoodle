bool CheckCrossing( const Int c  )
{
    if( (C_state[c] != Crossing_State::Positive) && (C_state[c] != Crossing_State::Negative) )
    {
        return true;
    }
    
    // Check whether all arcs of crossing c are active and have the correct connectivity to c.
    bool passed = true;
    
    for( bool io : { In, Out } )
    {
        for( bool lr : { Left, Right } )
        {
            const Int a = C_arcs[io][lr][c];
            
            const int active = (A_state[a] == Arc_State::Active);
            
            if( !active )
            {
                print("Check at    " + CrossingString(c) );
                print("Probem with " + ArcString(a) + ": It's not active." );
            }
            
            const bool tailtip = ( io == In ) ? Tip : Tail;
            
            const bool good = (A_crossings[tailtip][a] == c);
            
            if( !good )
            {
                print("Check at    " + CrossingString(c) );
                print("Probem with " + ArcString(a) + ": It's not connected correctly to crossing.");
            }
            passed = passed && active && good;
        }
    }
    
    if( !passed )
    {
        eprint("Crossing "+ToString(c)+" failed to pass CheckCrossing.");
    }
    return passed;
}

bool CheckAllCrossings()
{
    bool passed = true;
    
    for( Int c = 0; c < initial_crossing_count; ++c )
    {
        passed = passed && CheckCrossing(c);
    }
    
    if( passed )
    {
        print("CheckAllCrossings passed.");
    }
    return passed;
}


bool CheckArc( const Int a  )
{
    if( A_state[a] == Arc_State::Inactive )
    {
        return true;
    }
    
    // Check whether the two crossings of arc a are active and have the correct connectivity to c.
    bool passed = true;
    

    for( bool tiptail : {Tail, Tip} )
    {
        const Int c = A_crossings[tiptail][a];
        
        const bool active = ( 
            (C_state[c] == Crossing_State::Positive)
            ||
            (C_state[c] == Crossing_State::Negative)
        );
        
        
        if( !active )
        {
            print("Check at    " + ArcString(a) );
            print("Probem with " + CrossingString(c) + ": It's not active." );
        }
        const bool inout = (tiptail == Tail) ? Out : In;
    
        const bool good = ( (C_arcs[inout][Left ][c] == a) || (C_arcs[inout][Right][c] == a) );
        
        if( !good )
        {
            print("Check at    " + ArcString(a) );
            print("Probem with " + CrossingString(c) + ": It's not connected correctly to arc.");
        }
        
        passed = passed && active && good;
        
    }
    
    if( !passed )
    {
        eprint("Arc "+ToString(a)+" failed to pass CheckArc.");
    }
    
//    PD_assert( passed );
    
    return passed;
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
        print("CheckAllArcs passed.");
    }
    return passed;
}

bool CheckAll()
{
    const bool passed = CheckAllCrossings() && CheckAllArcs();
//    PD_assert( passed );
    return passed;
}
