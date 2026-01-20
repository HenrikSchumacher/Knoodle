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
    auto tag = [](){ return MethodName("Disconnect"); };
    
    TOOLS_PTIMER(timer,tag());
    
    
//    // DEBUGGING
//    constexpr bool debugQ = true;
    
    if constexpr ( debugQ )
    {
        wprint(tag() + ": Debug mode active.");
        
        if( !pd.CheckAll() )
        {
            pd_eprint("pd.CheckAll() failed on entry.");
        }
    }
    
    Size_T change_counter = 0;
    // We make copy so that we can manipulate it.
    ArcContainer_T dA_F_buffer = pd.ArcFaces();
    mptr<Int> dA_F       = dA_F_buffer.data();
    const Int dA_count   = Int(2) * pd.max_arc_count;
          Int F_count    = pd.FaceCount();   // Might be increased during this routine.
    
    // Using A_scratch for face flags. The array should be large anough for additional faces.
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
    
    std::vector<Int> f_dA_compressed;
    f_dA_compressed.reserve(f_max_size);
    
    // Stack for the "disconnect jobs".
    std::vector<std::pair<Int,Int>> stack;
    stack.reserve(f_max_size);
    
    using I = ToSigned<Int>; // security measure so that we do not decrement below 0 and wrap around.
    AssociativeContainer_T<I,Int> f_counts;
    
    auto remove_loop = [&pd,dA_F,this]( const Int da )
    {
        Size_T counter = 0;
        LoopRemover<Int> R (*this,pd,da);
        const Int d = R.Direction();

        while( R.Step() )
        {
            const Int b_0 = R.DeactivatedArc0();
            const Int b_1 = R.DeactivatedArc1();
            // Might be unnecessary.
            dA_F[ToDarc(b_0, d)] = DoNotVisit;
            dA_F[ToDarc(b_0,!d)] = DoNotVisit;
            dA_F[ToDarc(b_1, d)] = DoNotVisit;
            dA_F[ToDarc(b_1,!d)] = DoNotVisit;
            
            // We are done with darc da_0; move on.
            ++counter;
        }
        
        return counter;
    };
        
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
        
        if constexpr ( debugQ )
        {
            if( !pd.CheckAll() )
            {
                pd_eprint("pd.CheckAll() failed when starting to process da_0 = " + pd.DarcString(da_0) +".");
            }
        }
        
        if constexpr ( debugQ ) { logprint("Handle loop arc."); }
        // Handle loop arc.
        {
            Size_T da_0_loop_count = remove_loop(da_0);
            
            change_counter += da_0_loop_count;
            
            if constexpr (debugQ)
            {
                if( da_0_loop_count > Size_T(0) )
                {
                    nprint(tag() + ":remove_loop(da_0) removed " + ToString(da_0_loop_count) + " loop.");
                }
                if( !pd.CheckAll() )
                {
                    pd_eprint("pd.CheckAll() failed after running remove_loop(da_0).");
                }
            }
            
            if( da_0_loop_count > Size_T(0) ) { continue; }
        }
        
        // da_0 cannot be a loop arc.
        
        const Int f = dA_F[da_0];
        
        if( !PD_T::ValidIndexQ(f) ) { continue; }

        if( !F_state[f] ) { continue; }
        
        // Collecting face stats for face f.
        // For each neighbor face count how often it occurs. Also determine the size of the face.
        f_counts.clear();
        f_dA.clear();
        f_F.clear();
        stack.clear();
        
        if constexpr( debugQ )
        {
            f_F_compressed.clear();
            f_dA_compressed.clear();
            
            logprint("Collecting face stats for face f = " + ToString(f) + ".");
            if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
        }
        
        // We avoid ArcLeftDarc here because the data structure is highly in flux.
        {
            Int de = da_0;
            do
            {
                auto [e,d_e] = FromDarc(de);
                
                if( !pd.ArcActiveQ(e) )
                {
                    pd_eprint(MethodName("Disconnect") + ", during collecting face stats of face f = " + ToString(f) + ": " + pd.ArcString(e) + " is not active.");
                }
                
                if( dA_F[de] != f )
                {
                    eprint(MethodName("Disconnect") + ", during collecting face stats of face f = " + ToString(f) + ": dA_F[de] != f (de = " + ToString(de) + ", dA_F[de] = " + ToString(dA_F[de]) + "); dA_F must be stale.");
                    
                    pd.PrintInfo();
                    
                    TOOLS_LOGDUMP(da_0);
                    TOOLS_LOGDUMP(de);
                    TOOLS_LOGDUMP(dA_F[de]);
                    TOOLS_LOGDUMP(dA_F[FlipDarc(de)]);
                    
                    TOOLS_LOGDUMP(pd.ArcFaces());
                    TOOLS_LOGDUMP(F_count);
                    logvalprint("F_state",ArrayToString(F_state,{Int(2) * pd.max_arc_count}));
                    
                    pd_eprint(MethodName("Disconnect") + ": End of error.");
                }
                
                const Int g = dA_F[FlipDarc(de)];
                f_dA.push_back(de);
                f_F.push_back(g);
                Increment(f_counts,g);
                de = pd.LeftDarc(de);
            }
            while( de != da_0 );
        }
        
        if constexpr( debugQ )
        {
            for( Int g : f_F )
            {
                if( f_counts.contains(g) && (f_counts[g] > I(1)) )
                {
                    f_F_compressed.push_back(g);
                }
            }
            
            TOOLS_LOGDUMP(f_dA);
            TOOLS_LOGDUMP(f_F);
            logvalprint("f_counts",ToString(f_counts));
            TOOLS_LOGDUMP(f_F_compressed);
            
            logprint("Cycling over arcs of face f = " + ToString(f) + " (size = " + ToString(f_dA.size())+ ").");
            
            if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
            
            if( f_dA.size() <= Size_T(1) ) { pd_eprint("f_dA.size() <= Size_T(1)"); }
        }
        
        // Cycle once more around the face and do the surgery.
        for( Int db : f_dA )
        {
            auto [b,d]  = FromDarc(db);
            const Int g = dA_F[FlipDarc(db)];
            
            if constexpr ( debugQ )
            {
                logprint("------------------------------------");
                
                TOOLS_LOGDUMP(stack);
                TOOLS_LOGDUMP(db);
                logvalprint("b", pd.ArcString(b));
                TOOLS_LOGDUMP(g);
            }
            
            if( !pd.ArcActiveQ(b) || (dA_F[db] != f) ) { continue; }
            
            if( f_counts[g] <= I(1) )
            {
                if constexpr( debugQ ) { logprint("f_counts[g] <= Int(1)"); }
                continue;
            }
        
            if( stack.empty() || stack.back().second != g )
            {
                stack.push_back({db,g});
                if constexpr( debugQ )
                {
                    logprint(std::string("stack.push_back({") + ToString(db) + "," + ToString(g) + "});");
                }
                continue;
            }
             
            
            const Int da = stack.back().first;
            auto [a,d_a] = FromDarc(da);
            
            if constexpr ( debugQ )
            {
                if( stack.back().second != g )
                {
                    pd_eprint("Face " + ToString(stack.back().second) + " on stack does not match g = " + ToString(g) + ".");
                    TOOLS_LOGDUMP(stack);
                }
                
                logprint("Starting surgery.");
                if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
            }
            
            if( !pd.ArcActiveQ(a) )
            {
                if constexpr ( debugQ )
                {
                    wprint(pd.ArcString(a) + " from the stack is not active. Maybe it should have been erased earlier? We pop it from stack now and continue.");
                    
                    TOOLS_LOGDUMP(dA_F[da]);
                    TOOLS_LOGDUMP(dA_F[FlipDarc(da)]);
                    TOOLS_LOGDUMP(stack);
                    TOOLS_LOGDUMP(f_F);
                    TOOLS_LOGDUMP(f_dA);
                }
                stack.pop_back();
                continue;
            }
            
            if constexpr ( debugQ )
            {
                if( d_a != d ) { pd_eprint("d_a != d."); }
                
                TOOLS_LOGDUMP(d);
                TOOLS_LOGDUMP(da);
                logvalprint("a", pd.ArcString(a));
                TOOLS_LOGDUMP(db);
                logvalprint("b", pd.ArcString(b));
                TOOLS_LOGDUMP(f);
                TOOLS_LOGDUMP(g);
                TOOLS_LOGDUMP(dA_F[ToDarc(a, d)]);
                TOOLS_LOGDUMP(dA_F[ToDarc(a,!d)]);
                TOOLS_LOGDUMP(dA_F[ToDarc(b, d)]);
                TOOLS_LOGDUMP(dA_F[ToDarc(b,!d)]);
            }

            pd.template AssertArc<1>(a);
            pd.template AssertArc<1>(b);
            PD_ASSERT(d_b == d);
            
            PD_ASSERT( dA_F[ToDarc(a, d)] == f );
            PD_ASSERT( dA_F[ToDarc(b, d)] == f );
            PD_ASSERT( dA_F[ToDarc(a,!d)] == g );
            PD_ASSERT( dA_F[ToDarc(b,!d)] == g );
        
            {
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
                F_state[f_new] = true;
                ++F_count;

                if constexpr (debugQ)
                {
                    nprint("Creating new face f_new = " + ToString(f_new) +".");
                    
                    if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
                    logprint("Relabeling faces.");
                    
                    nprint("Tell all darcs between da = " + ToString(da) + " (excluded) and db = " + ToString(db) + " (included) to have new face f_new = " + ToString(f_new) +".");
                }
                
                //Tell all darcs LeftDarc(da),LeftDarc(LeftDarc(da)),...,db in face f that their left face is now f_new.
                {

                    Int de = da;
                    do
                    {
                        // We avoid ArcLeftDarc here because the data structure is highly in flux.
                        de = pd.LeftDarc(de);
                        
                        if constexpr (debugQ)
                        {
                            TOOLS_LOGDUMP(de);
                            TOOLS_LOGDUMP(dA_F[de]);
                            TOOLS_LOGDUMP(dA_F[FlipDarc(de)]);
                        }
                        
                        const Int h = dA_F[FlipDarc(de)];
                        
                        auto iterator = std::find(stack.begin(), stack.end(), std::pair{de,h} );
                        if( iterator != stack.end() )
                        {
                            // Should be impossible.
                            eprint("{ "+ToString(de)+","+ToString(h)+"} is still on stack!");
                        }
//                        nprint("Setting dA_F[de] = f_new; (de = " +ToString(de) +", f_new = " + ToString(f_new) + ").");
                        dA_F[de] = f_new;
                        
                        if( f_counts.contains(h) ) { --f_counts[h]; }
                    }
                    while( de != db );
                }
                
                const Int c_a = pd.A_cross(a,d);
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
                
                ++change_counter;
                f_counts[g]   -= I(1); // db does not belong to face f anymore.
                
                /*!@ Remove the loops that we might have created.
                 * We can simply do do `remove_loop(da)` and `remove_loop(db)` without changing `f_counts` any further. To see this, we distinguish two (possibly nondisjoint) cases:
                 *
                 *      Case B: If `da` is a loop after disconnecting, then `db` pointed into `da`, i.e., `db` was the last arc in the face to be considered anyways.
                 *
                 *      Case A: If `db` is a loop edge, it does not matter for face `f`, because `db` is not on the boundary of `f` anymore. In fact, the whole algorithm was designed to guarantee this.
                 *
                 */
                 
                const Size_T da_loop_count = remove_loop(da);
                
                if constexpr (debugQ)
                {
                    if( da_loop_count > Size_T(0) )
                    {
                        nprint("remove_loop(da) removed " + ToString(da_loop_count) + " loop.");
                    }
                    if( !pd.CheckAll() )
                    {
                        pd_eprint("pd.CheckAll() failed after running remove_loop(da).");
                    }
                }
                
                const Size_T db_loop_count = remove_loop(db);
                
                if constexpr (debugQ)
                {
                    if( db_loop_count > Size_T(0) )
                    {
                        nprint("remove_loop(db) removed " + ToString(db_loop_count) + " loop.");
                    }
                    if( !pd.CheckAll() )
                    {
                        pd_eprint("pd.CheckAll() failed after running remove_loop(db).");
                    }
                }
                
                change_counter += (da_loop_count + db_loop_count);
            }
            
            if constexpr ( debugQ )
            {
                logprint("Finished surgery.");
                if( !pd.CheckAll() ) { pd_eprint("!CheckAll()"); }
            }
            
            if( f_counts[g] <= (1) )
            {
                if constexpr ( debugQ )
                {
                    logprint("Popping darc " + ToString(stack.back()) + " from stack.");
                }
                stack.pop_back();
            }
            
        } // for( Int db : f_dA )
        
        // We are done with this face and do not have to visit it again.
        F_state[f]     = false;
        
        if constexpr ( debugQ )
        {
            if( !stack.empty() )
            {
                wprint(tag() + ": !stack.empty().");
                TOOLS_LOGDUMP(stack);
                TOOLS_LOGDUMP(F_count);
            }
            
            for( auto & x : f_counts )
            {
                if( x.second >= I(2) )
                {
                    wprint(tag() + ": Face " + ToString(x.first) + " has count " + ToString(x.second) + " >= 2 in f_counts."  );
                    logvalprint("f_counts",ToString(f_counts));
                    break;
                }
                
                if( x.second < I(0) )
                {
                    wprint(tag() + ": Face " + ToString(x.first) + " has negative count " + ToString(x.second) + " in f_counts."  );
                    logvalprint("f_counts",ToString(f_counts));
                    break;
                }
            }
        }
    }
    
    if( change_counter > Size_T(0) )
    {
        pd.ClearCache();
    }
    
    if constexpr ( debugQ )
    {
        TOOLS_DUMP(change_counter);
        TOOLS_LOGDUMP(change_counter);
    }
    
    return change_counter;
}
