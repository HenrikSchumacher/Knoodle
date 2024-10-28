private:

/*! @brief Applies a Reidemeister I move to arc `a` _without checking_ that `a` is a loop arc.
 */

void Reidemeister_I( const Int a )
{
    PD_PRINT( ClassName() + "::Reidemeister_I at " + ArcString(a) );
    
    const Int c = A_cross(a,Head);
    
    // We assume here that we already know that a is a loop arc.
    
    PD_ASSERT( A_cross(a,Tail) == c );
    
    const bool side = (C_arcs(c,In,Right) == a);
    
    const Int a_next = C_arcs(c,Out,!side);
    const Int a_prev = C_arcs(c,In ,!side);
    
    if( a_prev == a_next )
    {
        ++unlink_count;
        DeactivateArc(a);
        DeactivateArc(a_prev);
        DeactivateCrossing(c);
        R_I_counter += 2;
        
        return;
    }
    
    Reconnect<Head>(a_prev,a_next);
    DeactivateArc(a);
    DeactivateCrossing(c);
    ++R_I_counter;
    
    AssertArc(a_prev);
    
    return;
}
