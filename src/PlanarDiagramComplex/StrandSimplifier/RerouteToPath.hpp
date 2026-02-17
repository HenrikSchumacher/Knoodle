private:
            
/*!
 * @brief Attempts to reroute the strand. It is typically not save to call this function without the invariants guaranteed by `SimplifyStrands`. Some of the invariants are correct coloring of the arcs and crossings in the current strand and the absense of possible Reidemeister I and II moves along that strand.
 * This is why we make this function private.
 *
 * @param a_first The first arc of the strand on entry as well as as on return.
 *
 * @param a_last The last arc of the strand (included) as well as on entry as on return. Note that this values is a reference and that the passed variable will likely be changedQ!
 */

bool RerouteToPath( const Int a_first, mref<Int> a_last )
{
    PD_TIMER(timer,MethodName("RerouteToPath"));
    
#ifdef PD_DEBUG
    const Int Cr_0 = pd->CrossingCount();
    TOOLS_LOGDUMP(Cr_0);
#endif
    
    PD_VALPRINT("a_first",a_first);
    PD_VALPRINT("a_last",a_last);
    PD_VALPRINT("strand", ShortArcRangeString(a_first,a_last));
    PD_VALPRINT("path", ShortPathString());


//PD_PRINT("Diagram before rerouting:");
//#ifdef PD_DEBUG
//    pd->PrintInfo();
//#endif
//PD_VALPRINT("ArcLeftDarc", pd->ArcLeftDarcs());
//PD_VALPRINT("C_scratch", pd->C_scratch);
//PD_VALPRINT("A_scratch", pd->A_scratch);
   
    // path[0] == a_first. This is not to be crossed.
    
    Int p = 1;
    Int a = a_first;
    
    // At the beginning of the strand we want to avoid inserting a crossing on arc `b` if `b` branches directly off from the current strand.
    //
    //      |         |        |        |
    //      |    a    |        |        |    current strand
    //    --|-------->-------->-------->-------->
    //      |         |        |        |
    //      |         |        |        |
    //            b = path[p]?
    //

    WalkToBranch<Head>(a,p);
    
    // Now `a` is the first arc to be rerouted.
    
    // We do the same at the end now.
    
    Int q = path_length - Int(1);
    Int e = a_last;
    
    WalkToBranch<Tail>(e,q);

    // Now e is the last arc to be rerouted.
    
    // Handle the rerouted arcs.
    
    PD_TIC("While loop for rerouting");

    while( p < q )
    {
        const Int c_0 = pd->A_cross(a,Head);
//        PD_VALPRINT("c_0", CrossingString(c_0));

        const bool side = (pd->C_arcs(c_0,In,Right) != a);
        
        const Int a_2 = pd->C_arcs(c_0,Out,side);
//        PD_VALPRINT("a_2", ArcString(a_2));
        const Int b = path[p];
//        PD_VALPRINT("b", ArcString(b));
        const bool left_to_rightQ = DualArcLeftToRightQ(b);
        
        const Int c_1 = pd->A_cross(b,Head);
//        PD_VALPRINT("c_1", CrossingString(c_1));
        // This is the situation for `a` before rerouting for `side == Right`;
        //
        //
        //                ^a_1
        //      |         |        |        |
        //      |    a    | a_2    |        |   e
        //    --|-------->-------->--......-->-------->
        //      |         ^c_0     |        |
        //      |         |        |        |
        //                |a_0
        //
        
        // a_0 is the vertical incoming arc.
        const Int a_0 = pd->C_arcs(c_0,In , side);
//        PD_VALPRINT("a_0", ArcString(a_0));
        // a_1 is the vertical outgoing arc.
        const Int a_1 = pd->C_arcs(c_0,Out,!side);
//        PD_VALPRINT("a_1", ArcString(a_1));
        // In the cases b == a_0 and b == a_1, can we simply leave everything as it is!
        PD_ASSERT( b != a_2 )
        PD_ASSERT( b != a )
        
#ifdef PD_DEBUG
        if( a == b )
        {
            logvalprint("a",ArcString(a));
            TOOLS_LOGDUMP(p);
            TOOLS_LOGDUMP(q);
            logvalprint("path",ShortPathString());
        }
#endif // PD_DEBUG
        
        //This can happen, but seldomly.
        if( (b == a_0) || (b == a_1) ) [[unlikely]]
        {
            // Go to next arc.
            a = a_2;
            ++p;
            continue;
        }
        
        PD_ASSERT( a_0 != a_1 );
        
        
        // The case `left_to_rightQ == true`.
        // We cross arc `b` from left to right.
        //
        // Situation of `b` before rerouting:
        //
        //             ^
        //             | b_next
        //             |
        //             O
        //             ^
        //             |
        //  ------O----X----O------
        //             |c_1
        //             |
        //             O
        //             ^
        //  >>>>>>     | b
        //             |
        //             X
        //
        // We want to change this into the following:
        //
        //             ^
        //             | b_next
        //             |
        //             O
        //             ^
        //             |
        //  ------O----X----O------
        //             |c_1
        //             |
        //             O
        //             ^
        //             | a_1
        //             |
        //             O
        //             ^
        //    a        |     a_2
        //  ----->O----X----O----->
        //             |c_0
        //             |
        //             O
        //             ^
        //             | b
        //             |
        //             X
        
        Reconnect<Head,false>(a_0,a_1);
        pd->ChangeArcColor_Private( a_1, pd->A_color[b] );
        Reconnect<Head,false>(a_1,b  );
        
        // We have to reconnect the head of b to c_0 manually, since a_0 has forgotten that it is connected to c_0.
        
        pd->A_cross(b,Head) = c_0;
        
        const Int da   = ToDarc(a  ,Head);
        const Int db   = ToDarc(b  ,Head);
        const Int da_1 = ToDarc(a_1,Tail);
        const Int da_2 = ToDarc(a_2,Tail);
        
        // Recompute `c_0`. We have to be aware that the handedness and the positions of the arcs relative to `c_0` can completely change!
        if( left_to_rightQ )
        {
            pd->C_state[c_0] = BooleanToCrossingState(overQ);
            
            // overQ == true
            //
            //             O
            //             ^
            //             | a_1
            //             |
            //             O
            //             ^
            //    a        |     a_2
            //  ----->O---------O----->
            //             |c_0
            //             |
            //             O
            //             ^
            //             | b == path[p]
            //             |
            //             X
            
            // Fortunately, this does not depend on overQ.
            const C_Arcs_T C = { {a_1,a_2}, {a,b} };
            C.Write(pd->C_arcs.data(c_0));
            
//            const Int buffer [4] = { a_1,a_2,a,b };
//            copy_buffer<4>(&buffer[0],C_arcs.data(c_0));

            if constexpr ( lutQ )
            {
                SetLeftDarc(da  , ReverseDarc(da_1));
                SetLeftDarc(da_2, ReverseDarc(db  ));
                SetLeftDarc(db  , ReverseDarc(da  ));
                SetLeftDarc(da_1, ReverseDarc(da_2));
            }
        }
        else // if ( !left_to_rightQ )
        {
//            C_state[c_0] = overQ
//                         ? CrossingState_T::LeftHanded
//                         : CrossingState_T::RightHanded;
            
            pd->C_state[c_0] = BooleanToCrossingState(!overQ);
            
            // overQ == true
            //
            //             O
            //             ^
            //             | a_1
            //             |
            //             O
            //             ^
            //    a_2      |        a
            //  <-----O---------O<-----
            //             |c_0
            //             |
            //             O
            //             ^
            //             | b == path[p]
            //             |
            //             X
            
            // Fortunately, this does not depend on overQ.
            const C_Arcs_T C = { {a_2,a_1}, {b,a} };
            C.Write(pd->C_arcs.data(c_0));
            
//            const Int buffer [4] = { a_2,a_1,b,a };
//            copy_buffer<4>(&buffer[0],C_arcs.data(c_0));
            
            if constexpr ( lutQ )
            {
                SetLeftDarc(da  , ReverseDarc(db  ));
                SetLeftDarc(da_2, ReverseDarc(da_1));
                SetLeftDarc(db  , ReverseDarc(da_2));
                SetLeftDarc(da_1, ReverseDarc(da  ));
            }
        }
        
        AssertCrossing<1>(c_0);
        AssertCrossing<1>(c_1);
        AssertArc<1>(a);
        AssertArc<1>(a_0);
        AssertArc<1>(a_1);
        AssertArc<1>(a_2);
        AssertArc<1>(b);

        // Go to next of arc.
        a = a_2;
        ++p;
    }
    PD_TOC("While loop for rerouting");

    
    // strand_arc_count is just an upper bound to prevent infinite loops.
    CollapseArcRange(a,e,strand_arc_count);

    AssertArc<1>(a);
    AssertArc<0>(e);
    
    // TODO: Is this necessary? If yes, why?
    if constexpr ( lutQ ) { RepairLeftDarc(ToDarc(a,Head)); }
    
    a_last = a;

#ifdef PD_DEBUG
    const Int Cr_1 = pd->CrossingCount();
#endif
    
    PD_ASSERT(Cr_1 < Cr_0);
    
    PD_DPRINT(ToString(Cr_0 - Cr_1) + " crossings removed.");
    
    PD_ASSERT(pd->CheckAll() );
    PD_ASSERT(CheckLeftDarc());
    
    ++change_counter;
    
    return true;
}

/*! @brief We move from one and `a` of a strand in direction `headtail` until path `p` starts to branch off from it.
 */

template<bool headtail>
void WalkToBranch( mref<Int> a, mref<Int> p ) const
{
    PD_TIMER(timer,MethodName("WalkToBranch")+"<" + (headtail ? "Head" : "Tail") + ">");
    // `a` is an arc.
    // `p` points to a position in `path`.
    
    Int c = pd->A_cross(a,headtail);
    Int side = (pd->C_arcs(c,headtail,Right) == a);
    Int b = path[p];
    
    while(
        (pd->C_arcs(c, headtail,!side) == b)
        ||
        (pd->C_arcs(c,!headtail, side) == b)
    )
    {
        a = pd->C_arcs(c,!headtail,!side);
        c = pd->A_cross(a,headtail);
        side = (pd->C_arcs(c,headtail,Right) == a);
        
        p = headtail ? p + Int(1) : p - Int(1);
        
        b = path[p];
    }
}


/*! @brief Attempts to reroute the strand to a shortest path. It is typically not safe to call this function without the invariants guaranteed by `SimplifyStrands`. Some of the invariants are correct marking of the arcs and crossings in the current strand and the absense of possible Reidemeister I and II moves along that strand.
 *  This is why we make this function private.
 *
 *  This implicitly _assumes_ that we can travel from `a_first` to `a_last` by `NextArc(-,Head)`. Otherwise, the behavior is undefined.
 *
 *  @param a The first arc of the strand on entry as well as as on return.
 *
 *  @param b The last arc of the strand (included) on entry as well as on return. Note that this value is a reference and that the passed variable will likely be changedQ!
 */

bool RerouteToShortestPath_impl( const Int a, mref<Int> b, const Int max_dist )
{
    PD_TIMER(timer,MethodName("RerouteToShortestPath_impl"));
    
    PD_VALPRINT("change_counter",change_counter);
    
    PD_ASSERT(pd->CheckAll());
    PD_ASSERT(CheckStrand(a,b));
    
    
    // We don't like loops of any kind here.
    PD_ASSERT(pd->A_cross(a,Tail) != pd->A_cross(a,Head));
    PD_ASSERT(pd->A_cross(b,Tail) != pd->A_cross(b,Head));
    PD_ASSERT(pd->A_cross(a,Tail) != pd->A_cross(b,Tail));
    PD_ASSERT(pd->A_cross(a,Tail) != pd->A_cross(b,Head));
    PD_ASSERT(pd->A_cross(a,Head) != pd->A_cross(b,Head));
    
//    PD_ASSERT(pd->A_cross(a,Head) != pd->A_cross(b,Tail));
    
#ifdef PD_DEBUG
    if( pd->A_cross(a,Head) == pd->A_cross(b,Tail) )
    {
        TOOLS_LOGDUMP(strand_arc_count);
        TOOLS_LOGDUMP(CountArcsInRange(a,b));
        logvalprint("strand",ShortArcRangeString(a,b));
    }
#endif // PD_DEBUG
    
    Int path_arc_count;
    
    if( strategy == DijkstraStrategy_T::Legacy )
    {
        path_arc_count = FindShortestPath_Legacy_impl(a,b,max_dist);
    }
    else
    {
        path_arc_count = FindShortestPath_impl(a,b,max_dist);
    }
    
#ifdef PD_COUNTERS
    RecordPreStrandSize(strand_arc_count);
    RecordPostStrandSize(path_arc_count);
#endif
        
    if( (path_arc_count < Int(0)) || (path_arc_count > max_dist) )
    {
        PD_DPRINT("No improvement detected. (strand_arc_count = " + ToString(strand_arc_count) + ", path_arc_count = " + ToString(path_arc_count) + ", max_dist = " + ToString(max_dist) + ")");
        return false;
    }
    
    bool successQ = RerouteToPath(a,b);
    
    return successQ;
    
}



//public:
//    
//    /*!
//     * @brief Attempts to reroute the input strand. It returns the first and last arcs of the rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
//     *
//     * @param a_first The first arc of the input strand.
//     *
//     * @param a_last The last arc of the input strand (included).
//     *
//     * @param overQ_ Whether the input strand is over (`true`) or under (`true`). Caution: The routine won't work correctly, if the input arc range is not a over/understrand according to this flag!
//     */
//    
//    std::array<Int,2> RerouteToShortestPath(
//        mref<PD_T> pd_input, const Int a, const Int b, bool overQ_
//    )
//    {
//        LoadDiagram(pd_input);
//        ResetMark();
//        SetStrandMode(overQ_);
//        
//        strand_arc_count = MarkArcs(a,b);
//        
//        RerouteToShortestPath_impl(a,b,strand_arc_count-Int(1));
//        
//        Cleanup();
//        
//        return {a,b};
//    }
