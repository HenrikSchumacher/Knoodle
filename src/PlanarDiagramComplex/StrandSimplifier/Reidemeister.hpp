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
    
    PD_DPRINT( MethodName("Reidemeister_I")+" found loop arc " + ArcString(a) );
    
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

// This checks for a backward Reidemeister II move (and it exploits already that both vertical strands go under (if current strand is an overstrand) or go over (if current strand is an understrans).
// It may set a to another pd->NextArc(a,Tail) if a is to be deleted.
// In this case, the strand is shortened appropriately.
bool Reidemeister_II_Backward(
    const Int c_0, mref<Int> a, const Int c_1, const bool side_1, const Int a_next
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Reidemeister_II_Backward"); };
    
    
    PD_DPRINT( tag() + " checks " + ArcString(a) );
    
    // Call preconditions.
    PD_ASSERT( ArcMarkedQ(a));
    PD_ASSERT( CrossingMarkedQ(c_0));
    PD_ASSERT(!CrossingMarkedQ(c_1));
    
    const bool side_0 = (pd->C_arcs(c_0,Out,Right) == a);
    
    //             a == pd->C_arcs(c_0,Out, side_0)
    //        a_prev == pd->C_arcs(c_0,In ,!side_0)
    const Int a_0_out = pd->C_arcs(c_0,Out,!side_0);
    const Int a_0_in  = pd->C_arcs(c_0,In , side_0);
    
    //             a == pd->C_arcs(c_1,In , side_1)
    //        a_next == pd->C_arcs(c_1,Out,!side_1)
    const Int a_1_out = pd->C_arcs(c_1,Out, side_1);
    const Int a_1_in  = pd->C_arcs(c_1,In ,!side_1);
    
    
    // Consequences:
    PD_ASSERT(!ArcMarkedQ(a_next));  // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(!ArcMarkedQ(a_1_out)); // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(!ArcMarkedQ(a_1_in )); // Because of !CrossingMarkedQ(c_1).
    
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
        PD_DPRINT(tag() + " detected move at ingoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        PD_ASSERT( ArcMarkedQ(a_prev));  // Because of  CrossingMarkedQ(c_0).
        
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
            wprint(tag() + ": a_prev == a_next.");
        }
        
        PD_ASSERT(a_prev != a_next);    //  Because ArcMarkedQ(a_prev) and !ArcMarkedQ(a_next).
        PD_ASSERT(!ArcMarkedQ(a_next));
        Reconnect<Head>(a_prev,a_next);
        
        PD_ASSERT(!ArcMarkedQ(a_1_out));
        // a_1_out is unmarked, so it is safe to deactivate it.
        if( a_0_in == a_1_out ) [[unlikely]]
        {
            DeactivateArc(a_1_out);
            CreateUnlinkFromArc(a_1_out);
        }
        else
        {
            Reconnect<Head>(a_0_in,a_1_out);
            // It could happen that `a_1_out` is the strand start; in this case we could make the maximal strand one arc longer now. However, we keep the strand start where it was. Rerouting a strand that is slightly shorter than it could be does not break the algorithm.
        }
        
        // a is the current strand; we can recover if we deactivate it.
        DeactivateArc(a);
        
        PD_ASSERT(!ArcMarkedQ(a_1_in));
        DeactivateArc(a_1_in);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CountReidemeister_II();
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a     );
        AssertArc<0>(a_next);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        ++change_counter;
        
        // Tell StrandSimplify to move back to previous arc, so that marking and loop checking are performed correctly.
        PD_ASSERT(strand_arc_count >= Int(2) );
        strand_arc_count -= 2;
        a = a_prev;

        return true;
    }
    
    if( a_0_in == a_1_out )
    {
        PD_DPRINT(tag() + " detected move at outgoing port of " + CrossingString(c_1) + ".");
        
        const Int a_prev = pd->C_arcs(c_0,In,!side_0);
        PD_ASSERT( ArcMarkedQ(a_prev));  // Because of  CrossingMarkedQ(c_0).
        
        // This cannot happen because we called RemoveLoopPath.
        PD_ASSERT( a_0_out != a_prev );
        
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
            
            // Caution: It could happen that `a_0_out` is the strand start.
            // We have to take care of this outside the function.
            PD_ASSERT(!ArcMarkedQ(a_0_out)); // TODO: Check this!
            Reconnect<Head>(a_prev,a_0_out);
            // a_next is unmarked, so this should be safe.
            PD_ASSERT(!ArcMarkedQ(a_next));
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

            // This should work also if a_prev == a_0_out. This would imply that a_prev == a_begin.
            // This is why we prefer keeping a_prev alive over concatenating with the unlocked Reidemeister I move.
            
            PD_ASSERT(a_prev != a_next); //  Because ArcMarkedQ(a_prev) and !ArcMarkedQ(a_next).
            PD_ASSERT(!ArcMarkedQ(a_next));
            Reconnect<Head>(a_prev,a_next);
            
            // `a_1_in` is unmarked. So, deactivating it is safe.
            PD_ASSERT(!ArcMarkedQ(a_1_in));
            Reconnect<Tail>(a_0_out,a_1_in);
            
            // It could happen that `a_0_out` is a_begin. In this case we could make the maximal strand one arc longer. However, we keep the strand start where it was. Rerouting a strand that is slightly shorter than it could be does not break the algorithm.
            
            CountReidemeister_II();
        }
        
        DeactivateArc(a);       // We can recover from deactivating the current arc.
        PD_ASSERT(!ArcMarkedQ(a_1_out));
        DeactivateArc(a_1_out); // a_1_out is unmarked, so this is safe.
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a     );
        AssertArc<0>(a_next);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        ++change_counter;
        
        // Tell StrandSimplify to move back to previous arc, so that marking and loop checking are performed correctly.
        PD_ASSERT(strand_arc_count >= Int(2) );
        strand_arc_count -= 2;
        a = a_prev;
        return true;
    }

    PD_DPRINT( tag() + " found no changes.");
    
    return false;
}


bool Reidemeister_II_Forward(
     const Int a_prev, const Int c_0, const Int a, const Int c_1, const Int a_next
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Reidemeister_II_Forward"); };
    
    PD_PRINT( tag() + " checks " + ArcString(a) );
 
    AssertArc<1>(a_prev);
    AssertArc<1>(a     );
    AssertArc<1>(a_next);
    AssertCrossing<1>(c_0);
    AssertCrossing<1>(c_1);
    
    // Call preconditions.
    PD_ASSERT(ArcMarkedQ(a_prev));
    PD_ASSERT(!CrossingMarkedQ(c_0));
    PD_ASSERT(!CrossingMarkedQ(c_1));
    
    // Consequences:
    PD_ASSERT(!ArcMarkedQ(a));      // Because of !CrossingMarkedQ(c_0).
    PD_ASSERT(!ArcMarkedQ(a_next)); // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(a_prev != a_next);    // Because of !CrossingMarkedQ(c_1).
    
    // Necessary for a Reidemeister II move: the crossings have opposite handedness
    if( !OppositeHandednessQ(pd->C_state[c_0],pd->C_state[c_1]) )
    {
        PD_PRINT(tag() + " found no changes.");
        return false;
    }
    
//    // Both vertical strands need to go over or under at the same time.
//    if( pd->ArcOverQ(a,Tail) != pd->ArcOverQ(a,Head) )
//    {
//        PD_PRINT(tag() + " found no changes.");
//        return false;
//    }

    const bool side_0 = (pd->C_arcs(c_0,Out,Right) == a);
    const bool side_1 = (pd->C_arcs(c_1,In ,Right) == a);
    
    // Because of OppositeHandednessQ(pd->C_state[c_0],pd->C_state[c_1]) == true, is equivalent to pd->ArcOverQ(a,Tail) != pd->ArcOverQ(a,Head).
    if( side_0 != side_1 )
    {
        PD_PRINT(tag() + " found no changes.");
        return false;
    }
    
    PD_ASSERT(a      == pd->C_arcs(c_0,Out, side_0));
    PD_ASSERT(a_prev == pd->C_arcs(c_0,In ,!side_0));
    const Int a_0_out = pd->C_arcs(c_0,Out,!side_0);
    const Int a_0_in  = pd->C_arcs(c_0,In , side_0);
    
    PD_ASSERT(a      == pd->C_arcs(c_1,In , side_1));
    PD_ASSERT(a_next == pd->C_arcs(c_1,Out,!side_1));
    const Int a_1_out = pd->C_arcs(c_1,Out, side_1);
    const Int a_1_in  = pd->C_arcs(c_1,In ,!side_1);
    
    AssertArc<1>(a_0_out);
    AssertArc<1>(a_0_in );
    AssertArc<1>(a_1_out);
    AssertArc<1>(a_1_in);
    
    PD_VALPRINT("a_prev",ArcString(a_prev ));
    PD_VALPRINT("a     ",ArcString(a      ));
    PD_VALPRINT("a_next",ArcString(a_next ));
    
    PD_VALPRINT("a_0_in ",ArcString(a_0_in ));
    PD_VALPRINT("a_0_out",ArcString(a_0_out));
    PD_VALPRINT("a_1_in ",ArcString(a_1_in ));
    PD_VALPRINT("a_1_out",ArcString(a_1_out));
    
    PD_VALPRINT("c_0",CrossingString(c_0));
    PD_VALPRINT("c_1",CrossingString(c_1));
    
    // Should be obvious:
    PD_ASSERT(a_prev  != a_0_in );
    PD_ASSERT(a_next  != a_1_out);
    PD_ASSERT(a_0_in  != a_1_in );
    PD_ASSERT(a_0_out != a_1_out);
    
    // Call preconditions.
    PD_ASSERT(!ArcMarkedQ(a_0_in )); // Because of !CrossingMarkedQ(c_0).
    PD_ASSERT(!ArcMarkedQ(a_0_out)); // Because of !CrossingMarkedQ(c_0).
    PD_ASSERT(!ArcMarkedQ(a_1_in )); // Because of !CrossingMarkedQ(c_1).
    PD_ASSERT(!ArcMarkedQ(a_1_out)); // Because of !CrossingMarkedQ(c_1).

    
//    PD_ASSERT( a)1)
    
    /* // Picture side_0 == Right and for c_0 left-handed and c_1 right-handed.
     *
     *               a_0_out           a_1_in
     *             ^                 |
     *             |                 |
     *  a_prev     |c_0     a        |     a_next
     *  ---------->----------------->----------->
     *             |                 |c_1
     *             |                 |
     *             | a_0_in          v a_1_out
     */
    
    if( a_0_out == a_1_in )
    {
        PD_PRINT(tag() + " detected move at ingoing port of " + CrossingString(c_1) + ".");
        
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
        
        PD_ASSERT(a_prev != a_next);    // Because ArcMarkedQ(a_prev) and !ArcMarkedQ(a_next)
        PD_ASSERT( ArcMarkedQ(a_prev));
        PD_ASSERT(!ArcMarkedQ(a_next));
        Reconnect<Head>(a_prev,a_next);
        
        if( a_0_in == a_1_out ) [[unlikely]]
        {
            PD_ASSERT(!ArcMarkedQ(a_1_out));
            DeactivateArc(a_1_out);
            CreateUnlinkFromArc(a_0_in);
        }
        else
        {
            PD_ASSERT(!ArcMarkedQ(a_1_out));
            Reconnect<Head>(a_0_in,a_1_out);
        }
        
        PD_ASSERT(!ArcMarkedQ(a));
        DeactivateArc(a);
        PD_ASSERT(!ArcMarkedQ(a_1_in));
        DeactivateArc(a_1_in);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        CountReidemeister_II();
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a     );
        AssertArc<0>(a_next);
        AssertArc<0>(a_1_out);
        AssertArc<0>(a_1_in);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        ++change_counter;
        --strand_arc_count; // We have to shorten the strand because we check the current strand a_prev again.
        return true;
    }
    
    if( a_0_in == a_1_out )
    {
        PD_PRINT(tag() + " detected move at outgoing port of " + CrossingString(c_1) + ".");
        
        // This cannot happen because we called RemoveLoopPath.
        PD_ASSERT( a_0_out != a_prev );
        
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
            
            PD_ASSERT(!ArcMarkedQ(a_0_out));
            Reconnect<Head>(a_prev,a_0_out);
            PD_ASSERT(!ArcMarkedQ(a_next));
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
            

            PD_ASSERT(!ArcMarkedQ(a_next));
            Reconnect<Head>(a_prev,a_next);
            PD_ASSERT(!ArcMarkedQ(a_1_in));
            Reconnect<Tail>(a_0_out,a_1_in);
            
            // It could happen that `a_0_out` is a_begin. In this case we could make the maximal strand one arc longer. However, we keep the strand start where it was. Rerouting a strand that is slightly shorter than it could be does not break the algorithm.
            
            CountReidemeister_II();
        }
        

        PD_ASSERT(!ArcMarkedQ(a));
        DeactivateArc(a);
        PD_ASSERT(!ArcMarkedQ(a_1_out));
        DeactivateArc(a_1_out);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        
        AssertArc<1>(a_prev);
        AssertArc<0>(a     );
        AssertArc<0>(a_next);
        AssertCrossing<0>(c_0);
        AssertCrossing<0>(c_1);
        
        ++change_counter;
        --strand_arc_count; // We have to shorten the strand because we check the current strand a_prev again.
        return true;
    }

    PD_PRINT(tag() + " found no changes.");
    
    AssertArc<1>(a_prev);
    AssertArc<1>(a);
    AssertArc<1>(a_next);
    
    return false;
}
