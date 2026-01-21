private:
    
/*!@brief Collapse the strand made by the arcs `a_begin, pd.NextArc(a_begin,Head),...,a_end` to a single arc. Finally, we reconnect the head of arc `a_begin` to the head of `a_end` and deactivate `a_end`. All the crossings from the head of `a_begin` up to the tail of `a_end`. This routine must only be called, if some precautions have be carried out: all arcs that crossed this strand have to reconnected or deactivated.
 *
 * @param a_begin First arc on strand to be collapsed.
 *
 * @param a_end Last arc on strand to be collapsed.
 *
 * @param arc_count_ An upper bound for the number of arcs on the strand from `a_begin` to `a_end`. This merely serves as fallback to prevent infinite loops.
 */

void CollapseArcRange(
    const Int a_begin, const Int a_end, const Int arc_count_
)
{
//            PD_DPRINT( ClassName()+"::CollapseArcRange" );
    
    // We remove the arcs _backwards_ because of how Reconnect uses PD_ASSERT.
    // (It is legal that the head is inactive, but the tail must be active!)
    // Also, this might be a bit cache friendlier.

    if( a_begin == a_end )
    {
        return;
    }
    
    PD_TIMER(timer,MethodName("CollapseArcRange(" + ToString(a_begin) + "," + ToString(a_end) + "," + ToString(arc_count_) + ")"));
    
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    Int a = a_end;
    
    Int iter = 0;
    
    // arc_count_ is just an upper bound to prevent infinite loops.
    while( (a != a_begin) && (iter < arc_count_) )
    {
        ++iter;
        
        const Int c = A_cross(a,Tail);
        
        const bool side  = (C_arcs(c,Out,Right) == a);
        
        // side == Left         side == Right
        //
        //         |                    ^
        //         |                    |
        // a_prev  |   a        a_prev  |   a
        //    ---->X---->          ---->X---->
        //         |c                   |c
        //         |                    |
        //         v                    |

        const Int a_prev = C_arcs(c,In,!side);
        PD_ASSERT( C_arcs(c,In,side) != C_arcs(c,Out,!side) );
        Reconnect<Head>(C_arcs(c,In,side),C_arcs(c,Out,!side));
        
        DeactivateArc(a);
        
        // Sometimes we cannot guarantee that all arcs at `c` are already deactivated. But they will finally be deleted. Thus, we suppress some asserts here.
        DeactivateCrossing<false>(c);
        
        a = a_prev;
    }
    
    PD_ASSERT( a == a_begin );
    
    PD_ASSERT( iter <= arc_count_ );

    // a_end is already deactivated.
    PD_ASSERT( !pd.ArcActiveQ(a_end) );

    Reconnect<Head,false>(a_begin,a_end);
    
    ++change_counter;
    
#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_CollapseArcRange += Tools::Duration(start_time,stop_time);
#endif
}
