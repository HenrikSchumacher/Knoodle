public:

    Int OverPassMoves()
    {
//        Int old_counter = -1;
        Int counter = 0;
//        Int iter = 0;
        
//        while( counter != old_counter )
//        {
//            ++iter;
//            old_counter = counter;
//            
            const auto C_pass_arcs = CrossingOverArcs();
            
            for( Int c = 0; c < initial_crossing_count; ++c )
            {
                counter += PassMove(c,C_pass_arcs);
            }
//            
//        }
        
//        valprint( ClassName() +"::OverPassMoves: iterations", iter );
        
        return counter;
    }

    Int UnderPassMoves()
    {
        const auto C_pass_arcs = CrossingUnderArcs();

        Int counter = 0;
        
        for( Int c = 0; c < initial_crossing_count; ++c )
        {
            counter += PassMove(c,C_pass_arcs);
        }
        
        return counter;
    }

private:

    bool PassMove( const Int c, cref<Tensor3<Int,Int>> C_pass_arcs  )
    {
        if( !CrossingActiveQ(c) )
        {
            PD_print( ClassName()+"PassMove: Crossing "+ToString(c)+" is not active. Skipping");

            return false;
        }
        
        // If we can do a Reidemeister I move, then we should do it.
        
        const bool RI = Reidemeister_I(c);
        
        if( RI )
        {
            return true;
        }
        
        // TODO: Detect potential over/under-arc 8-loop and reduce it to unlink!
        
        if( C_pass_arcs(c,Out,Left) == C_pass_arcs(c,In,Left) )
        {
            print("side = Left");
            
            const Int a_begin = C_arcs(c,Out,Left);
            const Int a_end   = C_arcs(c,In ,Left);
            
            // When we arrive here, we know that a_begin != a_end.
            // Otherwise we would have quit through Reidemeister_I.
            
            PD_assert( a_begin != a_end );
            PD_assert( A_cross(a_begin,Tail) == A_cross(a_end,Tip) );
            
            // We also know that c is not the crossing of an 8-loop.
            PD_assert( C_arcs(c,Out,Right) != C_arcs(c,In,Right) );
            
            // DEBUGGING
            
            if( C_pass_arcs(c,Out,Right) == C_pass_arcs(c,In,Right) )
            {
                wprint("Overpassing 8-loop.");
            }
            
            /*
             *             a_begin
             *       ...X<---         --->X
             *      .        \       /
             *     .          \     /
             *    .            \   /
             *    .             \ /
             *    .         c -->X
             *    .             ^ ^
             *    .            /   \
             *     .          /     \
             *      .        /       \
             *       ...X----         ----X
             *             a_end
             */


            RemoveArcs( a_begin, a_end );
        
            Reconnect(C_arcs(c,In,Right),Tip,C_arcs(c,Out,Right));
            
            DeactivateCrossing(c);
            
            return true;
            
//            return false;
            
        }
        else if ( C_pass_arcs(c,Out,Right) == C_pass_arcs(c,In,Right) )
        {
            print("side = Right");
            const Int a_begin = C_arcs(c,Out,Right);
            const Int a_end   = C_arcs(c,In ,Right);
            
            // When we arrive here, we know that a_begin != a_end.
            // Otherwise we would have quit through Reidemeister_I.
            
            PD_assert( a_begin != a_end );
            PD_assert( A_cross(a_begin,Tail) == A_cross(a_end,Tip) );
            
            // We also know that c is not the crossing of an 8-loop.
            PD_assert( C_arcs(c,Out,Left) != C_arcs(c,In,Left) );
            
            // DEBUGGING
            
            if( C_pass_arcs(c,Out,Left) == C_pass_arcs(c,In,Left) )
            {
                wprint("Overpassing 8-loop.");
            }
            /*
             *              a_begin
             *    X<---         --->X...
             *         \       /        .
             *          \     /          .
             *           \   /            .
             *            \ /             .
             *        c -->X              .
             *            ^ ^             .
             *           /   \            .
             *          /     \          .
             *         /       \        .
             *    X----         ----X...
             *                a_end
             */
            
            RemoveArcs( a_begin, a_end );
//
            Reconnect(C_arcs(c,In,Left),Tip,C_arcs(c,Out,Left));
//            
            DeactivateCrossing(c);
//            
            return true;
            
//            return false;
        }
        else
        {
            return false;
        }
    }

    void RemoveArcs( const Int a_begin, const Int a_end )
    {
        Int a = a_begin;
        
        PD_assert( ArcActiveQ(a_begin) );
        PD_assert( ArcActiveQ(a_end) );
        
        PD_assert( A_cross(a_begin,Tail) == A_cross(a_end,Tip) );
        
        dump( A_cross(a_begin,Tail) );
        
        while( a != a_end )
        {
            const Int c = A_cross(a,Tip);
            
            dump( ArcString(a) );
            dump( CrossingString(c) );
                 
            
//            if( !CrossingActiveQ(c) )
//            {
//                continue;
//            }
            
            
            PD_assert( CrossingActiveQ(c) );
            PD_assert( ArcActiveQ(a) );
            
            PD_assert( (C_arcs(c,In,Left) == a) || (C_arcs(c,In,Right) == a) );
            
            // Find the arcs a_0 and a_1 go through c to the left and the right of a.
            bool side = C_arcs(c,In,Right) == a;
            
            // Arc `a` comes into crossing `c` through the right port.
            
            /*  side == right       side == left
             *
             *        ^                   ^
             *        |                   |
             *        |                   |
             *   a_0  | a_1          a_1  | a_0
             *  ----->+----->       <-----+<-----
             *        ^                   ^
             *        |                   |
             *        | a                 | a
             *        |                   |
             */
            
            const Int a_next = NextArc(a);
            const Int a_0    = C_arcs(c,In ,!side);
            const Int a_1    = C_arcs(c,Out, side);

            PD_assert( ArcActiveQ(a_0) );
            PD_assert( ArcActiveQ(a_1) );
            
            Reconnect(a_0,Tip,a_1);

            DeactivateCrossing(c);
            DeactivateArc(a);
            
            a = a_next;
        }
        
        dump( ArcString(a) );
        dump( A_cross(a_end,Tip) );
        
//        DeactivateArc(a_end);
    }
