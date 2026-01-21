/*! @brief Checks whether a Reidemeister I move can be made at crossing `c`,
 *  then applies it (if possible), and returns a Boolean that indicates whether
 *  a change has been made or not.
 */

bool Reidemeister_I( const Int c )
{
    PD_PRINT("\n" + ClassName()+"::Reidemeister_I( c = " + CrossingString(c) + " )");
    
    if( !CrossingActiveQ(c) )
    {
        PD_PRINT("Crossing "+CrossingString(c)+" is not active. Skipping");

        return false;
    }
    
    // This is precisely the case if two consecutive arcs coincide.
    
    auto C = GetCrossing(c);

    
    // Let's see on which side the loop is.
    bool side;
    
    
    if(C(Out,Left ) == C(In ,Left ))
    {
        side = Left;
    }
    else if (C(Out,Right) == C(In ,Right))
    {
        side = Right;
    }
    else
    {
        return false;
    }
    
    // This is the central assumption here.
    PD_ASSERT(C(Out,side) == C(In ,side));
    
    const Int a = C(In ,!side);
    const Int b = C(Out,!side);
    const Int d = C(Out, side);   // This is the looping arc (see picture below).
    
    if( a != b )
    {
/* Changing this (case side == Left )
 *                                        b = C(Out,Right)
 *                            +-<---O       O------->---O...
 *                           /       ^     ^             w
 *                          /         \   /
 *        C(Out,Left )     /           \ /
 *           = d =        |             /  C                v != w
 *        C(In ,Left )     \           / \
 *                          \         /   \
 *                           \       /     \            v
 *                            +--->-O       O---<-------O...
 *                                        a = C(In ,Right)
 *
 * to that:
 *
 *                                        b = C(Out,Right)
 *                            +-<---O       O------->---O...
 *                           /       ^     ^          w ^
 *                          /         \   /             |
 *        C(Out,Left )     /           \ /              |
 *           = d =        |             /  C            |
 *        C(In ,Left )     \           / \              |  a = C(In ,Right)
 *                          \         /   \             |
 *                           \       /     \            |
 *                            +--->-O       O         v O...
 */
        
        // Make arc a point to where arc b pointed before.
        Reconnect<Head>(a,b);
        
        DeactivateArc(d);
        
        DeactivateCrossing(c);
        
        goto Exit;
    }
    else
    {
/* Otherwise we have this case of an unlink.
 *
 *                            +-<---O       O--->-+
 *                           /       ^     ^   b   \
 *                          /         \   /         \
 *       C(Out,Left )      /           \ /           \
 *          = d =         |             /  C          |  a == b
 *       C(In ,Left )      \           / \           /
 *                          \         /   \         /
 *                           \       /     \   a   /
 *                            +--->-O       O-<---+
 */
        ++pd.unlink_count;
        
        DeactivateArc(a);
        DeactivateArc(d);
        
        DeactivateCrossing(c);
        
        goto Exit;
    }
   
Exit:
    
    ++pd.R_I_counter;
    
    return true;
}
