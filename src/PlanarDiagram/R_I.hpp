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

private:

/*! @brief Checks whether a Reidemeister I move can be made at crossing `c`,
 *  then applies it (if possible), and returns a Boolean that indicates whether
 *  a change has been made or not.
 */

bool Reidemeister_I_at_Crossing( const Int c )
{
    PD_PRINT("\nReidemeister_I_at_Crossing( c = "+CrossingString(c)+" )");
    
    if( !CrossingActiveQ(c) )
    {
        PD_PRINT("Crossing "+CrossingString(c)+" is not active. Skipping");

        return false;
    }
    
#ifdef PD_COUNTERS
    ++R_I_check_counter;
#endif
    
    // This is precisely the case if two consecutive arcs coincide.
    
    auto C = GetCrossing( c );

    
    // Let's see on which side the loop is.
    bool side;
    
    
    if( C(Out,Left ) == C(In ,Left ) )
    {
        side = Left;
    }
    else if ( C(Out,Right) == C(In ,Right) )
    {
        side = Right;
    }
    else
    {
        return false;
    }
    
    // This is the central assumption here.
    PD_ASSERT( C(Out,side) == C(In ,side) );
    
    const Int a = C(In ,!side);
    const Int b = C(Out,!side);
    const Int d = C(Out, side);   // This is the looping arc (see picture below).
    
    if( a != b )
    {
// Changing this (case side == Left )
//                                        b = C(Out,Right)
//                            +-<---O       O------->---O...
//                           /       ^     ^             w
//                          /         \   /
//        C(Out,Left )     /           \ /
//           = d =        |             /  C                v != w
//        C(In ,Left )     \           / \
//                          \         /   \
//                           \       /     \            v
//                            +--->-O       O---<-------O...
//                                        a = C(In ,Right)
//
// to that:
//
//                                        b = C(Out,Right)
//                            +-<---O       O------->---O...
//                           /       ^     ^          w ^
//                          /         \   /             |
//        C(Out,Left )     /           \ /              |
//           = d =        |             /  C            |
//        C(In ,Left )     \           / \              |  a = C(In ,Right)
//                          \         /   \             |
//                           \       /     \            |
//                            +--->-O       O         v O...
//
        
        // Make arc a point to where arc b pointed before.
        Reconnect<Head>(a,b);
        
        DeactivateArc(d);
        
        DeactivateCrossing(c);
        
        goto exit;
    }
    else
    {
// Otherwise we have this case of an unlink.
//
//                            +-<---O       O--->-+
//                           /       ^     ^   b   \
//                          /         \   /         \
//       C(Out,Left )      /           \ /           \
//          = d =         |             /  C          |  a == b
//       C(In ,Left )      \           / \           /
//                          \         /   \         /
//                           \       /     \   a   /
//                            +--->-O       O-<---+
        ++unlink_count;
        
        DeactivateArc(a);
        DeactivateArc(d);
        
        DeactivateCrossing(c);
        
        goto exit;
    }
   
exit:
    
    ++R_I_counter;
    
    return true;
}
