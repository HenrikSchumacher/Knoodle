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
    
    if( a_prev == a_next ) [[unlikely]]
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
    
    Reconnect<Head>(a_prev,a_next); // We keep a_prev alive here.
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
    // TODO: This checks for a backward Reidemeister II move (and it exploits already that both vertical strands go under (if current strand is an overstrand) or go over (if current strand is an understrans).
    
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
    
    /* Assuming overQ == true.
     *
     *               a_0_out           a_1_in
     *             |                 |
     *             |                 |
     *  a_prev     |c_0     a        |     a_next
     *  ---------->----------------->----------->
     *             |                 |c_1
     *             |                 |
     *             | a_0_in          | a_1_out
     */
        
    if( a_0_out == a_1_in )
    {
        PD_DPRINT("Detected Reidemeister II at ingoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        
        PD_ASSERT( a_0_in  != a_next );
        PD_ASSERT( a_1_in  != a_next );
        
        PD_ASSERT( a_0_out != a_prev );
        PD_ASSERT( a_1_out != a_prev );
        
        /*              a_0_out == a_1_in
         *             +---------------->+
         *             ^                 |
         *             |                 |
         *  a_prev     |c_0     a        v     a_next
         *  ---------->----------------->-------------->
         *             ^                 |c_1
         *             |                 |
         *             | a_0_in          v a_1_out
         */
        
        // It could happen that `a_next` is the strand start.
        // We have to take care of this outside the function.
                
        // DEBUGGING
        if( a_prev == a_next ) [[unlikely]]
        {
            wprint(MethodName("Reidemeister_II_Backward")+": a_prev == a_next.");
        }
        
        PD_ASSERT(a_prev != a_next);
        
        // TODO: What gives us the guarantee that a_prev != a_next?
        Reconnect<Head>(a_prev,a_next);
        
        // a_0_in cannot be the strand start (because its head goes under/over.
        // So, it is safe to deactivate it.
        if( a_0_in == a_1_out ) [[unlikely]]
        {
            DeactivateArc(a_0_in);
            CreateUnlinkFromArc(a_0_in);
        }
        else
        {
            Reconnect<Tail>(a_1_out,a_0_in);
            // It could happen that `a_1_out` is the strand start; in this case we could make the maximal strand one arc longer now. However, we keep the strand start where it was. Rerouting a strand that is slightly shorter than it could be does not break the algorithm.
        }
        
        DeactivateArc(a);
        // a_1_in cannot be the strand start (because its head goes under/over. So, this is safe.
        DeactivateArc(a_1_in);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CountReidemeister_II();
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a_next);
        
        ++change_counter;
        
        // Tell StrandSimplify to move back to previous arc, so that marking and loop checking are performed correctly.
        PD_ASSERT(strand_arc_count >= Int(2) );
        strand_arc_count -= 2;
        a = a_prev;
        return true;
    }
    
    if( a_0_in == a_1_out )
    {
        PD_DPRINT("Detected Reidemeister II at outgoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        
        // This cannot happen because we called RemoveLoopPath.
        PD_ASSERT( a_0_out != a_prev );
        
        // This should be guaranteed by the forward Reidemeister I check.
        PD_ASSERT(a_1_in != a_next);
        
//        // A nasty case that is easy to overlook.
        if( a_1_in == a_next ) [[unlikely]]
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
            
            // It could happen that `a_0_out` is the strand start.
            // We have to take care of this outside the function.
            Reconnect<Head>(a_prev,a_0_out);
            // a_next cannot be any arc on the strand because its head goes under/over.
            // So, deactivating it is safe.
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
            
            // It could happen that `a_next` is the strand start.
            // We have to take care of this outside the function.
            
            // This should work also if a_prev == a_0_out. This would imply that a_prev == a_begin.
            // This is why we prefer keeping a_prev alive over concatenating with the unlocked Reidemeister I move.
            
            // DEBUGGING
            if( a_prev == a_next ) [[unlikely]]
            {
                wprint(MethodName("Reidemeister_II_Backward")+": a_prev == a_next.");
            }
            
            PD_ASSERT(a_prev != a_next);
            
            // TODO: What gives us the guarantee that a_prev != a_next?
            Reconnect<Head>(a_prev,a_next);
            
            // `a_1_in` cannot be any arc on the strand because its head goes under/over.
            // So, deactivating it is safe.
            Reconnect<Tail>(a_0_out,a_1_in);
            
            // It could happen that `a_0_out` is a_begin. In this case we could make the maximal strand one arc longer. However, we keep the strand start where it was. Rerouting a strand that is slightly shorter than it could be does not break the algorithm.
            
            CountReidemeister_II();
        }
        
        DeactivateArc(a);
        DeactivateArc(a_0_in);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a_next);
        
        ++change_counter;
        
        // Tell StrandSimplify to move back to previous arc, so that marking and loop checking are performed correctly.
        PD_ASSERT(strand_arc_count >= Int(2) );
        strand_arc_count -= 2;
        a = a_prev;
        return true;
    }

    return false;
}
