public:

Size_T Disconnect()
{
    Size_T counter = 0;
    
    Int diagram_idx = 0;
    
    for( PD_T & pd : pd_list )
    {
        counter += Disconnect( pd );
        ++diagram_idx;
    }
    
    for( PD_T & pd : pd_todo )
    {
        pd_list.push_back( std::move(pd) );
    }
    
    pd_todo.clear();
    
    return counter;
}

/*!@brief This attempts to disconnects summands by using the information on the diagram's faces. Not all connected summands might be visible, though. This does one scan through all the faces of the diagram. Later simplification and further scans might be neccessary.
 
 * Only the local surgery is performed so that all found connected summands will reside as split subdiagrams within the current planar diagram.
 *
 *  The return value is the number of disconnection moves made.
 */

Size_T Disconnect( PD_T & pd )
{
    TOOLS_PTIMER(timer,MethodName("Disconnect"));
    
    if constexpr ( debugQ )
    {
        wprint(MethodName("Disconnect")+": Debug mode active.");
    }
    
    Size_T change_counter = 0;
    // We make copy so that we can manipulate it.
    ArcContainer_T dA_F_buffer = pd.ArcFaces();
    mptr<Int> dA_F       = dA_F_buffer.data();
    const Int dA_count   = Int(2) * pd.max_arc_count;
          Int F_count    = pd.FaceCount();   // Might be increased during this routine.
    
    static_assert( sizeof(Int) >= Size_T(2) * sizeof(bool) );
    mptr<bool> F_state = reinterpret_cast<bool *>(pd.A_scratch.data());
    
    fill_buffer( F_state, true, F_count );
    fill_buffer( &F_state[F_count], false, dA_count - F_count );
  
    Size_T f_max_size = ToSize_T(pd.MaxFaceSize());
    
    std::vector<Int> f_dA;
    f_dA.reserve(f_max_size);
    
    std::vector<Int> f_F;
    f_F.reserve(f_max_size);
    
    std::vector<Int> f_F_compressed;
    f_F_compressed.reserve(f_max_size);
    
    // Stack for the "disconnect jobs".
    std::vector<std::pair<Int,Int>> stack;
    stack.reserve(f_max_size);
    
    AssociativeContainer_T<Int,Int> f_counts;
        
    // Cycle over all darcs to make sure that each face is visited.
    for( Int da_0 = 0; da_0 < dA_count; ++da_0 )
    {
        auto [a_0,d_0] = FromDarc(da_0);
        
        if constexpr( debugQ )
        {
            logprint("############################################");
            TOOLS_LOGDUMP(da_0);
        }
        
        if( !pd.ArcActiveQ(a_0) ) { continue; }
        
        const Int f = dA_F[da_0];
        
        if( !F_state[f] ) { continue; }
        
        if constexpr( debugQ )
        {
            TOOLS_LOGDUMP(f);
        }
        
        // Cycle around face.
        // For each neighbor face count how often it occurs. Also determine the size of the face.
        f_counts.clear();
        f_dA.clear();
        f_F.clear();
        f_F_compressed.clear();
        
        if constexpr( debugQ )
        {
            TOOLS_LOGDUMP(f_F_compressed);
        }
        
        if constexpr( debugQ )
        {
            if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
            logprint("Collecting face stats for face " + ToString(f) + ".");
        }
        {
            Int de = da_0;
            do
            {
                auto [e,d_e] = FromDarc(de);
                
                if( !pd.ArcActiveQ(e) ) { pd_eprint(pd.ArcString(e) + " is not active."); }
                
                PD_ASSERT( dA_F[de] == f );
//                TOOLS_LOGDUMP(de);
                const Int g = dA_F[FlipDarc(de)];
//                TOOLS_LOGDUMP(g);
                f_dA.push_back(de);
                f_F.push_back(g);
                Increment(f_counts,g);
                // We avoid ArcLeftDarc here because the data structure is highly in flux.
                de = pd.LeftDarc(de);
            }
            while( de != da_0 );
        }
        
        
        for( Int g : f_F )
        {
            if( f_counts.contains(g) && (f_counts[g] > Int(1)) )
            {
                f_F_compressed.push_back(g);
            }
        }
        
        if constexpr( debugQ )
        {
            TOOLS_LOGDUMP(f_dA);
            TOOLS_LOGDUMP(f_F);
            logvalprint("f_counts",ToString(f_counts));
            TOOLS_LOGDUMP(f_F_compressed);
            if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
        }
        
        stack.clear();
        
        // We have to make sure that there are no loops because we want to guarantee that the diagram is "reduced" afterwards.
        if( f_dA.size() == Size_T(1) )
        {
            wprint(MethodName("Disconnect") + ": Loop arc detected. We should normally not get here, unless we use Disconnect without running ArcSimplifier or StrandSimplifier first.");
            
            const Int da = da_0;
            auto  [a,d] = FromDarc(da);
            
            PD_ASSERT( pd.A_cross(a,Tail) == pd.A_cross(a,Head) );
            
            const Int c      = pd.A_cross(a,d);
            const Int a_prev = pd.NextArc(a,!d,c);
            const Int a_next = pd.NextArc(a, d,c);
            
            if( a_prev == a_next )
            {
                // We found and 8-shaped unlink.
                
                // Face enclosed by da.
                const Int g = dA_F[FlipDarc(da)];
                // Face enclosed by a_prev.
                const Int h = dA_F[FlipDarc(a_prev)];
                
                PD_ASSERT( f != g );
                PD_ASSERT( h != f );
                PD_ASSERT( h != g );
                PD_ASSERT( dA_F[a_prev] == g );

                pd.DeactivateArc(a_prev);
                pd.DeactivateArc(a);
                pd.DeactivateCrossing(c);
                
                // g and h are no more.
                F_state[g] = false;
                F_state[h] = false;
                CreateUnlinkFromArc(pd,a);
            }
            else
            {
                pd.Reconnect(a_prev,d,a_next);
                pd.DeactivateArc(a);
                pd.DeactivateCrossing(c);
                
                // Caution! If A_cross(a_prev,!d) == A_cross(a_next,d), then we get another Reidemeister I loop!
            }
            
            // We are done with this face; move on.
            F_state[f] = false;
            ++change_counter;
            continue;
        }
        
        if constexpr ( debugQ )
        {
            logprint("Cycling over arcs of face " + ToString(f) + " (size = " + ToString(f_dA.size())+ ").");
            if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
        }
        PD_ASSERT(F_state[f]);
        PD_ASSERT(f_dA.size() > Size_T(1));
        
        // Cycle once more around the face and do the surgery.
        for( Int db : f_dA )
        {
            auto [b,d_b]  = FromDarc(db);
            const Int g   = dA_F[FlipDarc(db)];
            
            if constexpr ( debugQ )
            {
                logprint("------------------------------------");
                
                TOOLS_LOGDUMP(stack);
                TOOLS_LOGDUMP(db);
                logvalprint("b", pd.ArcString(b));
                TOOLS_LOGDUMP(g);
            }
            
            if( !pd.ArcActiveQ(b) || (dA_F[db] != f) ) { continue; }
            
            if( f_counts[g] <= Int(1) )
            {
                if constexpr( debugQ ) { logprint("f_counts[g] <= Int(1)"); }
                continue;
            }
        
            if( stack.empty() || stack.back().second != g )
            {
                stack.push_back({db,g});
                if constexpr( debugQ )
                {
                    logprint(std::string("stack.push_back(") + ToString(g) + ");");
                }
                continue;
            }
             
            const Int da = stack.back().first;
            auto [a,d] = FromDarc(da);
            
            if constexpr ( debugQ )
            {
                logprint("Starting surgery.");
                if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
                
                if( !pd.ArcActiveQ(a) ) { pd_eprint(pd.ArcString(a) + " is not active."); }
                
                TOOLS_LOGDUMP(da);
                logvalprint("a", pd.ArcString(a));
            }

            PD_ASSERT(dA_F[FlipDarc(da)] == dA_F[FlipDarc(db)]);
            
            pd.template AssertArc<1>(a);
            pd.template AssertArc<1>(b);
            PD_ASSERT(d_b == d);
            
            PD_ASSERT( dA_F[ToDarc(a, d)] == f );
            PD_ASSERT( dA_F[ToDarc(a, d)] == f );
            PD_ASSERT( dA_F[ToDarc(a,!d)] == g );
            PD_ASSERT( dA_F[ToDarc(a,!d)] == g );
        
            const Int c_0 = pd.A_cross(a,!d);
            const Int c_1 = pd.A_cross(a, d);
            pd.template AssertCrossing<1>(c_0);
            pd.template AssertCrossing<1>(c_1);
            // We guarantee this by the Reidemeister I check above.
            
            if constexpr ( debugQ )
            {
                logvalprint("c_0", pd.CrossingString(c_0));
                logvalprint("c_1", pd.CrossingString(c_1));
                if(c_0 == c_1) { pd_eprint("c_0 == c_1"); }
                if(!pd.CrossingActiveQ(c_0)) { pd_eprint(pd.CrossingString(c_0) + " is not active."); }
                if(!pd.CrossingActiveQ(c_1)) { pd_eprint(pd.CrossingString(c_1) + " is not active."); }
            }
            
//            const Int a_prev = pd.NextArc(a,!d,c_0);
//            const Int a_next = pd.NextArc(a, d,c_1);
            const Int a_prev = pd.NextArc(a,!d);
            const Int a_next = pd.NextArc(a, d);
            const Int b_prev = pd.NextArc(b,!d);
            const Int b_next = pd.NextArc(b, d);
        
            pd.template AssertArc<1>(a_prev);
            pd.template AssertArc<1>(a_next);
            pd.template AssertArc<1>(b_next);
            pd.template AssertArc<1>(b_prev);
        
            if constexpr ( debugQ )
            {
                TOOLS_LOGDUMP(da);
                logvalprint("a_prev",pd.ArcString(a_prev));
                logvalprint("a     ",pd.ArcString(a     ));
                logvalprint("a_next",pd.ArcString(a_next));
                
                TOOLS_LOGDUMP(db);
                logvalprint("b_prev",pd.ArcString(b_prev));
                logvalprint("b     ",pd.ArcString(b     ));
                logvalprint("b_next",pd.ArcString(b_next));
            }
            
            if( pd.A_cross(b,d) == c_0 )
            {
                PD_PRINT("A_cross(b,d) == A_cross(a,!d)");
                
                // In any case, this will be the last darc visited for this face.
                
                if( pd.A_cross(b,!d) == c_1 )
                {
                    PD_PRINT("A_cross(b,!d) == A_cross(a,d)");
                    logprint("CASE 1");
                    
                    // We split a bigon.
                    
                    /*      #  #  # #  #  #
                    //      ## # ## ## # ##
                    //       ##X##   ##X##
                    //          ^ h_1 /
                    //    a_next \   / b_prev
                    //            \ v
                    //             X c_1
                    //            / ^
                    //           /   \
                    //          v     \
                    //      db +       +     g
                    //         |   f   ^
                    //    .....|.......|..... <<-- We cut here.
                    //         v       |
                    //   g     +       + da
                    //          \     ^
                    //           \   /
                    //            v /
                    //             X  c_0
                    //            ^ \
                    //    a_prev /   \ b_next
                    //          / h_0 v
                    //         X       X
                    */
                    
                    PD_ASSERT( dA_F[ToDarc(a_prev, d)] == g );
                    PD_ASSERT( dA_F[ToDarc(a_next, d)] == g );
                    PD_ASSERT( dA_F[ToDarc(b_prev, d)] == g );
                    PD_ASSERT( dA_F[ToDarc(b_next, d)] == g );
                    
                    // h_0
                    PD_ASSERT( dA_F[ToDarc(a_prev,!d)] == dA_F[ToDarc(b_next,!d)] );
                    // h_1
                    PD_ASSERT( dA_F[ToDarc(a_next,!d)] == dA_F[ToDarc(b_prev,!d)] );
                    
                    if( a_prev == b_next )
                    {
                        const Int h_0 = dA_F[ToDarc(a_prev,!d)];
                        F_state[h_0] = false;
                        pd.DeactivateArc(b_next);
                    }
                    else
                    {
                        pd.Reconnect(a_prev,d,b_next);
                    }
                    
                    if( a_next == b_prev )
                    {
                        const Int h_1 = dA_F[ToDarc(a_next,!d)];
                        F_state[h_1] = false;
                        pd.DeactivateArc(b_prev);
                    }
                    else
                    {
                        pd.Reconnect(a_next,!d,b_prev);
                    }

                    pd.DeactivateArc(a);
                    pd.DeactivateArc(b);
                    pd.DeactivateCrossing(c_0);
                    pd.DeactivateCrossing(c_1);
                
                    if( (a_prev == b_next) && (a_next == b_prev) )
                    {
                        CreateUnlinkFromArc(pd,a);
                    }
                    
                    F_state[f] = false;
                    // In any case, we deactivate a,b; but it does not matter, since we deactivate f anyways.
                    f_counts[g] -= Int(2);
                }
                else
                {
                    PD_PRINT("A_cross(b,!d) != A_cross(a,d)");
                    logprint("CASE 2");
                    
                    /*  #################
                     //  #               #
                     //  #          c_1  #
                     //  #############X###
                     //     \         ^
                     //      \   f   /
                     //    db \     / da
                     //        \   /
                     //         v /
                     //    g     X c_0  g
                     //         ^ \
                     // a_prev /   \ b_next
                     //       /     \
                     //      /   h   \
                     //     /         v
                     //
                     // Beware of possible Reidemeister I move here!
                     */
                    
                    PD_ASSERT( dA_F[ToDarc(a_prev,d)] == g );
                    PD_ASSERT( dA_F[ToDarc(b_next,d)] == g );
                    
                    
                    // This cannot happen because otherwise F_A[dB] != g;
                    PD_ASSERT(db != a_prev);
                    
#ifdef PD_DEBUG
                    {
                        const Int h = dA_F[ToDarc(a_prev,!d)];
                        PD_ASSERT( dA_F[ToDarc(b_next,!d)] == h );
                        PD_ASSERT( f != h );
                        PD_ASSERT( g != h );
                    }
#endif // PD_DEBUG
                    
                    if(a_prev == b_next)
                    {
                        logprint("a_prev == b_next");
                        pd.DeactivateArc(b_next);
                        const Int h = dA_F[ToDarc(a_prev,!d)];
                        F_state[h] = false;
                    }
                    else
                    {
                        logprint("a_prev != b_next");
                        pd.Reconnect(a_prev,d,b_next);
                        // Caution: Could result in a loop arc.
                    }
                    pd.Reconnect(a,!d,b);
                    pd.DeactivateCrossing(c_0);

                    F_state[f] = false;
                    f_counts[g] -= Int(2);
                }
            }
            else
            {
                PD_PRINT("A_cross(b,d) == A_cross(a,!d)");
                
                if( pd.A_cross(b,!d) == c_1 )
                {
                    PD_PRINT("A_cross(b,!d) == A_cross(a,d)");
                    logprint("CASE 3");
                    
                    /*  #################
                    //  #               #
                    //  #c_0            #
                    //  ##X##############
                    //     \         ^
                    //      \   f   /
                    //    da \     / db
                    //        \   /
                    //         v /
                    //    g     X c_1  g
                    //         ^ \
                    // b_prev /   \ a_next
                    //       /     \
                    //      /   h   \
                    //     /         v
                    //
                    // Beware of possible Reidemeister I move here!
                    */
                    
#ifdef PD_DEBUG
                    const Int h = dA_F[ToDarc(a_next,!d)];
                    PD_ASSERT( dA_F[ToDarc(b_prev,!d)] == h );
                    PD_ASSERT( f != h );
                    PD_ASSERT( g != h );
#endif // PD_DEBUG
                    if(b_prev == a_next)
                    {
                        logprint("b_prev == a_next");
                        pd.DeactivateArc(b_prev);
                    }
                    else
                    {
                        logprint("b_prev != a_next");
                        pd.Reconnect(a_next,!d,b_prev);
                        // Caution: Could result in a loop arc.
                    }
                    pd.Reconnect(a,d,b);
                    pd.DeactivateCrossing(c_1);

                    F_state[f] = false;
                    f_counts[g] -= Int(2);
                }
                else
                {
                    PD_PRINT("A_cross(b,!d) != A_cross(a,d)");
                    logprint("CASE 4");
                    
                    /* `a` and `b` do not have any crossing in common.
                     *
                     *                   g
                     *                         c_1
                     *         #######<------###X###
                     *            |             ^
                     *            |             |
                     *            |             |
                     *    g    db |      f      | da    g
                     *            |             |
                     *            |             |
                     *            v             |
                     *         #######------>###X###
                     *                         c_0
                     *                   g
                     */
                    
                    /* `a` and `b` do not have any crossing in common.
                     *  Assuming that d_a == d_b == Head.
                     *
                     *                   g
                     *                         c_1
                     *         #######<------###X###
                     *            |             ^
                     *            |    f_new    |
                     *     g   db +-------------+      g
                     *
                     *            +-------------+ da
                     *            |      f      |
                     *            v             |
                     *         #######------>#######
                     *
                     *                   g
                     */
                    
                    // Create a new face.
                    const Int f_new = F_count;
                    ++F_count;
                    
                    //TODO: Tell all darcs LeftDarc(da),LeftDarc(LeftDarc(da)),...,db in face f that their left face is now f_new;
                    
                    if constexpr (debugQ)
                    {
                        if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
                        logprint("Relabeling faces.");
                    }
                    
                    Int de = da;
                    
                    Size_T iter = 0;
                    
                    do
                    {
                        TOOLS_LOGDUMP(de);
                        // We avoid ArcLeftDarc here because the data structure is highly in flux.
                        de = pd.LeftDarc(de);
                        dA_F[de] = f_new;
                        ++iter;
                    }
                    while( (de != db) && (iter > Size_T(2) * f_dA.size()) );

                    if(iter > Size_T(2) * f_dA.size())
                    {
                        pd_eprint("iter > Size_T(2) * f_dA.size()!!!");
                    }
                    
                    const Int c_a = c_1;
                    const Int c_b = pd.A_cross(b,d);
                    
                    pd.template AssertCrossing<1>(c_a);
                    pd.template AssertCrossing<1>(c_b);
                    
                    if constexpr (debugQ)
                    {
                        if(!pd.CrossingActiveQ(c_a)) { pd_eprint(pd.CrossingString(c_a) + " is not active."); }
                        if(!pd.CrossingActiveQ(c_b)) { pd_eprint(pd.CrossingString(c_b) + " is not active."); }
                           
                        logvalprint("c_a", pd.CrossingString(c_a));
                        logvalprint("c_b", pd.CrossingString(c_b));
                        logvalprint("a", pd.ArcString(a));
                        logvalprint("b", pd.ArcString(b));
                    }
                    
                    pd.A_cross(b,d) = c_a;
                    pd.A_cross(a,d) = c_b;
                    
                    pd.SetMatchingPortTo(c_a,d,a,b);
                    pd.SetMatchingPortTo(c_b,d,b,a);
                    
                    if constexpr (debugQ)
                    {
                        logvalprint("c_a", pd.CrossingString(c_a));
                        logvalprint("c_b", pd.CrossingString(c_b));
                        logvalprint("a", pd.ArcString(a));
                        logvalprint("b", pd.ArcString(b));
                    }
                    
                    F_state[f]     = false;
                    f_counts[g]   -= Int(1);
                    F_state[f_new] = true;
                }
            }
            if constexpr ( debugQ )
            {
                logprint("Finished surgery.");
            }
            
            if constexpr ( debugQ )
            {
                if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
            }

            ++change_counter;
            
            if( f_counts[g] <= Int(1) )
            {
                if constexpr ( debugQ )
                {
                    logprint("Popping darc " + ToString(stack.back()) + " from stack.");
                }
                stack.pop_back();
            }
            
        } // for( Int db : f_dA )
        
        PD_ASSERT(stack.empty());
    }
    
    logprint("Done.");
    
    if( change_counter > Size_T(0) )
    {
        pd.ClearCache();
    }
    return change_counter;
}
