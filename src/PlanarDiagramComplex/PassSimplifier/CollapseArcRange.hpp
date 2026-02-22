private:
    
/*!@brief Collapse the strand made by the arcs `a_begin, pd->NextArc(a_begin,Head),...,a_end` to the single arc  `a_begin`. Finally, we reconnect the head of arc `a_begin` to the head of `a_end` and deactivate `a_end`. All the crossings from the head of `a_begin` up to the tail of `a_end`. This routine must only be called, if some precautions have be carried out: all arcs that crossed this strand have to be reconnected or deactivated. Returns the number of arcs in `a_begin, pd->NextArc(a_begin,Head),...,a_end` that were deleted.
 *
 * @param a_begin First arc on strand to be collapsed.
 *
 * @param a_end Last arc on strand to be collapsed.
 *
 * @param arc_count_ An upper bound for the number of arcs on the strand from `a_begin` to `a_end`. This merely serves as fallback to prevent infinite loops.
 */

//Int CollapseArcRange(
//    const Int a_begin, const Int a_end, const Int arc_count_
//)
//{
//    return CollapseArcRange( a_begin, a_end, arc_count_, current_mark );
//}

Int CollapseArcRange(
    const Int a_begin, const Int a_end, const Int arc_count_, [[maybe_unused]] const Int mark
)
{
//            PD_DPRINT( ClassName()+"::CollapseArcRange" );
    
    // We remove the arcs _backwards_ because of how Reconnect uses PD_ASSERT.
    // (It is legal that the head is inactive, but the tail must be active!)
    // Also, this might be a bit cache friendlier.

    if( a_begin == a_end ) { return Int(0); }
    
    PD_TIMER(timer,MethodName("CollapseArcRange(" + ToString(a_begin) + "," + ToString(a_end) + "," + ToString(arc_count_) + "," + ToString(mark) + ")"));
    
    Int a = a_end;
    
    Int iter = 0;
    
    // arc_count_ is just an upper bound to prevent infinite loops.
    while( (a != a_begin) && (iter < arc_count_) )
    {
        PD_VALPRINT("a",ArcString(a));
        ++iter;
        
        const Int c = pd->A_cross(a,Tail);
        
        const bool side  = (pd->C_arcs(c,Out,Right) == a);
        
        // side == Left         side == Right
        //
        //         | b_0                ^ b_1
        //         |                    |
        // a_prev  |   a        a_prev  |   a
        //    ---->X---->          ---->X---->
        //         |c                   |c
        //         |                    |
        //         v b_1                | b_0

        const Int b_1    = pd->C_arcs(c,Out,!side);
        const Int a_prev = pd->C_arcs(c,In,!side);
        const Int b_0    = pd->C_arcs(c,In , side);
        
        PD_VALPRINT("b_0   ",ArcString(b_0)   );
        PD_VALPRINT("a_prev",ArcString(a_prev));
        PD_VALPRINT("b_1   ",ArcString(b_1)   );
        
        PD_ASSERT(b_0 != b_1);
        PD_ASSERT(ArcMark(b_0) != mark);
        PD_ASSERT(ArcMark(b_1) != mark);
        Reconnect<Head>(b_0,b_1);
        
        DeactivateArc(a);
        
        // Sometimes we cannot guarantee that all arcs at `c` are already deactivated. But they will finally be deleted. Thus, we suppress some asserts here.
        DeactivateCrossing<false>(c);
        
        a = a_prev;
    }
    
    PD_ASSERT( a == a_begin );
    
    PD_ASSERT( iter <= arc_count_ );

    // a_end is already deactivated.
    PD_ASSERT( !pd->ArcActiveQ(a_end) );

    Reconnect<Head,false>(a_begin,a_end);
    
    ++change_counter;
    
    return iter;
}
