private:
            
/*!
 * @brief Attempts to reroute the strand. It is typically not save to call this function without the invariants guaranteed by `SimplifyStrands`. Some of the invariants are correct coloring of the arcs and crossings in the current strand and the absense of possible Reidemeister I and II moves along that strand.
 * This is why we make this function private.
 *
 * @param pass On entry, pass to reroute. On return, the reouted pass.
 */

bool Reroute( mref<Pass_T> pass, mref<Path_T> path )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Reroute"); };
    PD_TIMER(timer,tag());
    
#ifdef PD_DEBUG
    const Int Cr_0 = pd->CrossingCount();
    TOOLS_LOGDUMP(Cr_0);
#endif
    
    if( !pass.ValidQ() ) { return false; }
    
    if( path.Size() < Int(2) ) { return false; }
    
    PD_VALPRINT("pass.first",pass.first);
    PD_VALPRINT("pass.last",pass.last);
    PD_VALPRINT("pass", PassString(pass));
    PD_VALPRINT("path", PathString(path));


//PD_PRINT("Diagram before rerouting:");
//#ifdef PD_DEBUG
//    pd->PrintInfo();
//#endif
//PD_VALPRINT("ArcLeftDarc", pd->ArcLeftDarcs());
//PD_VALPRINT("C_scratch", pd->C_scratch);
//PD_VALPRINT("A_scratch", pd->A_scratch);
    
    Int p = 1;
    Int a = pass.first;
    
    // At the beginning of the strand we want to avoid inserting a crossing on arc `b` if `b` branches directly off from the current pass.
    //
    //      |         |        |        |
    //      |    a    |        |        |    current pass
    //    --|-------->-------->-------->-------->
    //      |         |        |        |
    //      |         |        |        |
    //            b = path[p]?
    //

    WalkToBranch<Head>(path,a,p);
    
    // Now `a` is the first arc to be rerouted.
    
    // We do the same at the end now.
    
    Int q = path.Size() - Int(1);
    Int e = pass.last;
    
    WalkToBranch<Tail>(path,e,q);

    // Now e is the last arc to be rerouted.
    
    // Handle the rerouted arcs.
    
    PD_TIC("While loop for rerouting");

    while( p < q )
    {
        const Int c_0 = pd->A_cross(a,Head);
        PD_VALPRINT("c_0", CrossingString(c_0));

        const bool side = (pd->C_arcs(c_0,In,Right) != a);
        
        const Int a_2 = pd->C_arcs(c_0,Out,side);
        PD_VALPRINT("a_2", ArcString(a_2));
        auto [b,left_to_rightQ] = FromDarc(path[p]);
//        PD_VALPRINT("b", ArcString(b));
        
        const Int c_1 = pd->A_cross(b,Head);
        PD_VALPRINT("c_1", CrossingString(c_1));
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
        PD_VALPRINT("a_0", ArcString(a_0));
        // a_1 is the vertical outgoing arc.
        const Int a_1 = pd->C_arcs(c_0,Out,!side);
        PD_VALPRINT("a_1", ArcString(a_1));
        // In the cases b == a_0 and b == a_1, can we simply leave everything as it is!
        PD_ASSERT( b != a_2 )
        PD_ASSERT( b != a )
        
#ifdef PD_DEBUG
        if( a == b )
        {
            logvalprint("a",ArcString(a));
            TOOLS_LOGDUMP(p);
            TOOLS_LOGDUMP(q);
            logvalprint("path",PathString(path));
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
            pd->C_state[c_0] = BooleanToCrossingState(pass.overQ);
            
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
            //             | b == ArcOfDarc(path[p])
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
            pd->C_state[c_0] = BooleanToCrossingState(!pass.overQ);
            
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
            //             | b == ArcOfDarc(path[p])
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

    [[maybe_unused]] Int deleted_arc_count = CollapseArcRange( a, e, pass.arc_count, pass.mark );

    PD_VALPRINT("deleted_arc_count",deleted_arc_count);

    AssertArc<1>(a);
//    AssertArc<0>(e); // Not guaranteed if path is at least as long as pass.
    
    // TODO: Is this necessary? If yes, why?
    if constexpr ( lutQ ) { RepairLeftDarc(ToDarc(a,Head)); }
    
    pass.last = a;
    pass.arc_count = path.Size() - Int(1);
#ifdef PD_DEBUG
    const Int Cr_1 = pd->CrossingCount();
    
//    if( Cr_1 >= Cr_0 )
//    {
//        wprint(tag() + ": Cr_1 >= Cr_0.");
//    }
#endif // PD_DEBUG
    
    PD_DPRINT(ToString(Cr_0 - Cr_1) + " crossings removed.");
    
    PD_ASSERT(pd->CheckAll() );
    PD_ASSERT(CheckLeftDarc());
    
    ++change_counter;
    
    return true;
}

/*! @brief We move from one end `a` of a pass in direction `headtail` until path `p` starts to branch off from it.
 */

template<bool headtail>
void WalkToBranch( cref<Path_T> path, mref<Int> a, mref<Int> p ) const
{
    PD_TIMER(timer,MethodName("WalkToBranch")+"<" + (headtail ? "Head" : "Tail") + ">");
    // `a` is an arc.
    // `p` points to a position in `path`.
    
    Int c = pd->A_cross(a,headtail);
    Int side = (pd->C_arcs(c,headtail,Right) == a);
    Int b = ArcOfDarc(path[p]);
    
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
        
        b = ArcOfDarc(path[p]);
    }
}
