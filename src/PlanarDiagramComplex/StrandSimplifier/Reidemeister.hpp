private:

/*! @brief Checks whether arc `a` is a loop arc. In the affirmative case it performs a Reidemeister I move and returns `true`. Otherwise, it returns `false`.
 *  _Caution:_ This routine is equivalent to `PlanarDiagram<Int>::Reidemeister_I_Private`, except that it uses the modified routine `StrandSimplifier<Int>::Reconnect` that is equivalent to `PlanarDiagram<Int>::Reconnect` except that it makes modified arcs by `PlanarDiagram<Int>::TouchArc`!
 *
 * @param a The arc to check as possibly, to remove.
 *
 * @tparam checkQ Whether the check for `a` being a loop is activated (`true`), which is the default.
 */

template<bool checkQ>
bool Reidemeister_I( const Int a )
{
    const Int c = pd->A_cross(a,Head);
    
    if constexpr( checkQ )
    {
        if( pd->A_cross(a,Tail) != c )
        {
            return false;
        }
    }
    
    PD_DPRINT( MethodName("Reidemeister_I")+" at " + ArcString(a) );
    
    // We assume here that we already know that a is a loop arc.
    
    PD_ASSERT( pd->A_cross(a,Tail) == c );
    
    // const bool side = (C_arcs(c,In,Right) == a);
    
    // This allows a 50% chance that we do not have to load `C_arcs(c,In,Left)` again.
    const bool side = (pd->C_arcs(c,In,Left) != a);
    
    const Int a_next = pd->C_arcs(c,Out,!side);
    const Int a_prev = pd->C_arcs(c,In ,!side);
    
    if( a_prev == a_next )
    {
        //
        //             O-----+ a
        //             |     |
        //             |     |
        //       O-----+-----O
        //       |     |c
        //       |     |
        //       +-----O
        //   a_prev = a_next
        
        DeactivateArc(a);
        DeactivateArc(a_prev);
        DeactivateCrossing(c);
        CreateUnlinkFromArc(a);
        CountReidemeister_I();
        CountReidemeister_I();
        
        return true;
    }
    
    //             O-----+ a
    //             |     |
    //             |     |
    //       O-----+---->O
    // a_prev      |c
    //             V
    //             O
    //              a_next
    
    Reconnect<Head>(a_prev,a_next);
    DeactivateArc(a);
    DeactivateCrossing(c);
    CountReidemeister_I();
    
    AssertArc<1>(a_prev);
    
    return true;
}

bool Reidemeister_II_Backward(
    mref<Int> a, const Int c_1, const bool side_1, const Int a_next
)
{
    // TODO: This checks for a backward Reidemeister II move (and it exploits already that both vertical strands are either overstrands or understrands). But it would maybe help to do a check for a forward Reidemeister II move, because that might stop the strand from ending early. Not sure whether this would be worth it, because we would have to change the logic of `SimplifyStrands` considerably.
    
    PD_DPRINT( MethodName("Reidemeister_II_Backward")+" at " + ArcString(a) );
    
    const Int c_0 = pd->A_cross(a,Tail);
    
    const bool side_0 = (pd->C_arcs(c_0,Out,Right) == a);
    
    //             a == pd->C_arcs(c_0,Out, side_0)
    //        a_prev == pd->C_arcs(c_0,In ,!side_0)
    const Int a_0_out = pd->C_arcs(c_0,Out,!side_0);
    const Int a_0_in  = pd->C_arcs(c_0,In , side_0);
    
    //             a == pd->C_arcs(c_1,In , side_1)
    //        a_next == pd->C_arcs(c_1,Out,!side_1)
    const Int a_1_out = pd->C_arcs(c_1,Out, side_1);
    const Int a_1_in  = pd->C_arcs(c_1,In ,!side_1);
    
    //             O a_0_out         O a_1_in
    //             ^                 |
    //             |                 |
    //  a_prev     |c_0     a        v     a_next
    //  ----->O-------->O------>O-------->O------>
    //             ^                 |c_1
    //             |                 |
    //             |                 v
    //             O a_0_in          O a_1_out
        
    if( a_0_out == a_1_in )
    {
        PD_DPRINT("Detected Reidemeister II at ingoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        
        PD_ASSERT( a_0_in  != a_next );
        PD_ASSERT( a_1_in  != a_next );
        
        PD_ASSERT( a_0_out != a_prev );
        PD_ASSERT( a_1_out != a_prev );
        
        //              a_0_out == a_1_in
        //             O---------------->O
        //             ^                 |
        //             |                 |
        //  a_prev     |c_0     a        v     a_next
        //  ----->O-------->O------>O-------->O------>
        //             ^                 |c_1
        //             |                 |
        //             |                 v
        //             O a_0_in          O a_1_out
        
        Reconnect<Head>(a_prev,a_next);
        
        if( a_0_in == a_1_out )
        {
            DeactivateArc(a_0_in);
            CreateUnlinkFromArc(a_0_in);
        }
        else
        {
            Reconnect<Tail>(a_1_out,a_0_in);
        }
        
        DeactivateArc(a);
        DeactivateArc(a_0_out);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CountReidemeister_II();
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a_next);
        
        ++change_counter;
        
        // Tell StrandSimplify to move back to previous arc, so that coloring and loop checking are performed correctly.
        strand_length -= 2;
        a = a_prev;
        
        PD_ASSERT(strand_length >= Int(0) );
        
        return true;
    }
    
    if( a_0_in == a_1_out )
    {
        PD_DPRINT("Detected Reidemeister II at outgoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        
        // This cannot happen because we called RemoveLoop.
        PD_ASSERT( a_0_out != a_prev );
        
        // TODO: Might be impossible if we call Reidemeister_I on a_next.
        // A nasty case that is easy to overlook.
        if( a_1_in == a_next )
        {
            //             O a_0_out         O<---+ a_1_in == a_next
            //             ^                 |    |
            //             |                 |    |
            //  a_prev     |c_0     a        v    |
            //  ----->O-------->O------>O-------->O
            //             ^                 |c_1
            //             |                 |
            //             |                 v
            //             O<----------------O
            //              a_0_in == a_1_out
            //
            
            Reconnect<Head>(a_prev,a_0_out);
            DeactivateArc(a_next);
            CountReidemeister_I();
            CountReidemeister_I();
        }
        else
        {
            //             O a_0_out         O a_1_in
            //             ^                 |
            //             |                 |
            //  a_prev     |c_0     a        v     a_next
            //  ----->O-------->O------>O-------->O------>
            //             ^                 |c_1
            //             |                 |
            //             |                 v
            //             O<----------------O
            //              a_0_in == a_1_out
            //
            
            Reconnect<Head>(a_prev,a_next);
            Reconnect<Tail>(a_0_out,a_1_in);
            CountReidemeister_II();
        }
        
        DeactivateArc(a);
        DeactivateArc(a_0_in);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a_next);
        
        ++change_counter;
        
        // Tell StrandSimplify to move back to previous arc, so that coloring and loop checking are performed correctly.
        strand_length -= 2;
        a = a_prev;
        
        PD_ASSERT(strand_length >= Int(0) );
        
        return true;
    }

    return false;
}
