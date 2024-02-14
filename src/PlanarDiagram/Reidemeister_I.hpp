Int Reidemeister_I()
{
    // One round of Reidemeister_I checks.
    
    Int count = 0;
    
    for( Int c = 0; c < C_arcs.Size(); ++c )
    {
        count += Reidemeister_I(c);
    }
    
    return count;
}

bool Reidemeister_I( const Int c )
{
    PD_print("\nReidemeister_I( c = "+CrossingString(c)+" )");

    // Checks whether a Reidemeister I move can be made at the c-th crossing, then applies it (if possible), and returns a Boolean that indicates whether a change has been made or not.
    // This is precisely the case if two consecutive arcs coincide.
    
    if( !CrossingActiveQ(c) )
    {
        PD_print("Crossing "+ToString(c)+" is not active. Skipping");

        return false;
    }
    
    // Let's see on which side the loop is.
    bool side;
    
    const Tiny::Matrix<2,2,Int,Int> A ( C_arcs.data(c) );
    
    if( A[Out][Left ] == A[In ][Left ] )
    {
        side = Left;
    }
    else if ( A[Out][Right] == A[In ][Right] )
    {
        side = Right;
    }
    else
    {
        return false;
    }
    
    // This is the central assumption here.
    PD_assert( A[Out][side] == A[In ][side] );
    
    const Int a = A[In ][!side];
    const Int b = A[Out][!side];
    const Int d = A[Out][ side];   // This is the looping arc (see picture below).
    
    if( a != b )
    {
// Changing this (case side == Right )
//                                        b = A[Out][Right]
//                            +-<---O       O------->---O...
//                           /       ^     ^             w
//                          /         \   /
//        A[Out][Left]     /           \ /
//          = d =         |             /  c                v != w
//        A[In ][Left]     \           / \
//                          \         /   \
//                           \       /     \            v
//                            +--->-O       O---<-------O...
//                                        a = A[In][Right]
//
// to that:
//
//                                        b = A[Out][Right]
//                            +-<---O       O------->---O...
//                           /       ^     ^          w ^
//                          /         \   /             |
//        A[Out][Left]     /           \ /              |
//          = d =         |             /  c            |
//        A[In ][Left]     \           / \              |  a = A[In][Right]
//                          \         /   \             |
//                           \       /     \            |
//                            +--->-O       O         v O...
//
        
        // Make arc a point to where arc b pointed before.
        Reconnect(a, Tip, b);
        
        DeactivateArc(d);
        DeactivateArc(b);
        
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
//       A[Out][Left]      /           \ /           \
//          = d =         |             /  c          |  a == b
//       A[In ][Left]      \           / \           /
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
    
//    CheckAll();
    
    
    ++R_I_counter;
    
    return true;
}
