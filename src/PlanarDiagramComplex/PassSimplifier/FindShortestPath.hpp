public:

/*! @brief Attempts to find the shortest path between the faces created by merging the two faces of arc`a` and the faces created by merging the two faces of arc `b`.
 *
 *  @param a The one end arc of the shortest path we are looking for.
 *
 *  @param b The other end arc of the shortest path we are looking for.
 *
 *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
 */

Path_T FindShortestPath( mref<PD_T> pd_input, const Int a, const Int b, const Int max_dist )
{
    LoadDiagram(pd_input);
    MarkArc(a);
    MarkArc(b);
    
    Path_T p;
    const bool successQ = FindShortestPath(a,b,max_dist,p);
    Cleanup();
    
    return successQ ? p : Path_T();
}

/*! @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow! (It has to find and mark a the currect path between `a` and `b`, if existent.
 *
 *  @param a The first arc of the input strand.
 *
 *  @param b The last arc of the input strand (included).
 *
 *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
 */

Path_T FindShortestRerouting( mref<PD_T> pd_input, const Int a, const Int b, const Int max_dist )
{
    LoadDiagram(pd_input);
    Int arc_count = MarkArcs(a,b);
    
    Int max_dist_0 = Min(Ramp(arc_count - Int(2)),max_dist);
    
    Path_T p;
    const bool successQ = FindShortestPath(a,b,max_dist_0,p);
    Cleanup();
    
    return successQ ? p : Path_T();
}


private:

// Variant compatible with old version.
bool FindShortestPath(
    const Int a, const Int b, const Int max_dist, mref<Path_T> path
)
{
    return FindShortestPath( a, b, max_dist, path, current_mark,
        [this]( const Int e ){
            return A_mark[e] == current_mark;
        },
        []( const Int e )
        {
            (void)e;
            return false;
        }
    );
}

bool FindShortestPath(
    const Int a, const Int b, const Int max_dist, mref<Path_T> path,
    const Int dual_mark, const Int hidden_mark
)
{
    return FindShortestPath( a, b, max_dist, path, dual_mark,
        [this,hidden_mark]( const Int e )
        {
            return A_mark[e] == hidden_mark;
        },
        []( const Int e )
        {
            (void)e;
            return false;
        }
    );
}

bool FindShortestPath(
    const Int a, const Int b, const Int max_dist, mref<Path_T> path,
    const Int dual_mark, const Int hidden_mark, const Int forbidden_mark
)
{
    return FindShortestPath( a, b, max_dist, path, dual_mark,
        [this,hidden_mark]( const Int e )
        {
            return A_mark[e] == hidden_mark;
        },
        [this,forbidden_mark]( const Int e )
        {
            return A_mark[e] == forbidden_mark;
        }
    );
}

template<typename HiddenFun_T, typename ForbiddenFun_T>
bool FindShortestPath(
    const Int a, const Int b, const Int max_dist, mref<Path_T> path,
    const Int dual_mark, HiddenFun_T && hiddenQ, ForbiddenFun_T && forbiddenQ
)
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindShortestPath"); };
    PD_TIMER(timer,tag());
    
    PD_ASSERT(CheckLeftDarc());
    
    PD_ASSERT(a != b);
    PD_ASSERT(hiddenQ(a));
    PD_ASSERT(hiddenQ(b));
    
    PD_VALPRINT("a",a);
    PD_VALPRINT("b",b);
    
    // Two-sided graph Dijkstra to find shortest path.
    
    // Instead of two queues we use two stacks: to hold the next fronts (X_front, Y_front).
    // We also need a stack prev_front to hold the previous front.
    
    X_front.Reset();
    Y_front.Reset();
    
    // Indicate that we met a in this pass during forward stepping.
    SetDualArc(a,Tail,Head,a,dual_mark);
    
    // Indicate that we met b in this pass during backward stepping.
    SetDualArc(b,Head,Tail,b,dual_mark);
    
    Int k   = 0;
    Int X_r = 0;
    Int Y_r = 0;
    Int a_0 = a;
    Int b_0 = b;
    
    PD_PRINT("Pushing arcs of starting face to X_front.");
    {
        PD_ASSERT( hiddenQ(a) );
    
        ++k;
        const Int da = ToDarc(a,Tail);
        const Int de_0 = LeftDarc(da);
        Int de = de_0;
        bool successQ = false;
        
        do
        {
            auto [e,d] = FromDarc(de);
            // a and b share a common face.
            if( e == b )
            {
                SetDualArc(e,d,Head,a,dual_mark);
                goto Exit;
            }
            else if( hiddenQ(e) )
            {
                de = LeftDarc(ReverseDarc(de));
            }
            else if( forbiddenQ(e) )
            {
                de = LeftDarc(de);
            }
            else
            {
                successQ = true;
                break;
            }
        }
        while( de != de_0 );
            
        PD_VALPRINT("da",da);
        PD_VALPRINT("de",de);
        
        // TODO: Handle this error.
        if( !successQ )
        {
            PD_PRINT("Starting arc a = " + ArcString(a) + " is isolated.");
            path.Resize(Int(0));
            return false;
        }
        
        // CAUTION: DualArcMarkedQ(ArcOfDarc(de),dual_mark) must be false here; otherwise, not all neighbor faces will be traversed.
        
        // CAUTION: This is crucial!                        |
        //                                                  V
        if( this->template SweepFace<true,false>( de, Head, a, a_0, b_0, X_front, dual_mark, hiddenQ, forbiddenQ ) )
        {
            goto Exit;
        }
        ++X_r;
    }
    
    // If we arrive here, then we have the guarantee that a and b are not arcs of the same face (which would mean that we can reroute with 0 crossings).

    PD_PRINT("Pushing arcs of end face to Y_front.");
    {
        PD_ASSERT( hiddenQ(b) );
        
        ++k;
        const Int db   = ToDarc(b,Head);
        const Int de_0 = LeftDarc(db);
        Int de = de_0;
        bool successQ = false;
        
        do
        {
            auto [e,d] = FromDarc(de);
            
            if( hiddenQ(e) )
            {
                de = LeftDarc(ReverseDarc(de));
            }
            else if( forbiddenQ(e) )
            {
                de = LeftDarc(de);
            }
            else
            {
                successQ = true;
                break;
            }
        }
        while( de != de_0 );
        
        PD_VALPRINT("db",db);
        PD_VALPRINT("de",de);
        
        // TODO: Handle this error.
        if( !successQ )
        {
            PD_PRINT("Starting arc b = " + ArcString(b) + " is isolated.");
            path.Resize(Int(0));
            return false;
        }
        
        // CAUTION: DualArcMarkedQ(ArcOfDarc(de)) must be false here; otherwise, not all neighbor faces will be traversed.
        
        //  CAUTION: This is crucial!                      |
        //                                                 V
        if( this->template SweepFace<false,true>(de, Tail, b, b_0, a_0, Y_front, dual_mark, hiddenQ, forbiddenQ) )
        {
            goto Exit;
        }
        ++Y_r;
    }
    
#ifdef PD_COUNTERS
    // We only record initial faces if we completed both of them.
    RecordInitialFaceSize(X_front.Size());
    RecordInitialFaceSize(Y_front.Size());
#endif
   
    // We are good to go. Now run usual sweeps.

    while( (k < max_dist) && (!X_front.EmptyQ()) && (!Y_front.EmptyQ()) )
    {
        ++k;
        bool XQ;
        
        switch( strategy )
        {
            case DijkstraStrategy_T::Bidirectional:
            {
                XQ = (X_front.Size() <= Y_front.Size());
                break;
            }
            case DijkstraStrategy_T::Alternating:
            {
                XQ = static_cast<bool>(k % Int(2));
                break;
            }
            case DijkstraStrategy_T::Unidirectional:
            {
                XQ = true;
                break;
            }
            default:
            {
                XQ = (X_front.Size() <= Y_front.Size());
                break;
            }
        }
        
        PD_VALPRINT("X_front",X_front);
        PD_VALPRINT("Y_front",Y_front);
        
        bool stopQ;
        
        if( XQ )
        {
            stopQ = SweepFront( Head, a_0, b_0, X_r, X_front, dual_mark, hiddenQ, forbiddenQ );
        }
        else
        {
            stopQ = SweepFront( Tail, b_0, a_0, Y_r, Y_front, dual_mark, hiddenQ, forbiddenQ );
        }
        
        if( stopQ ) { goto Exit; }
        
        PD_VALPRINT("X_front",X_front);
        PD_VALPRINT("Y_front",Y_front);
    }
    
    // If we get here, then d + 1 == max_dist or X_front.EmptyQ() or Y_front.EmptyQ().
    
    // X_front.EmptyQ() and Y_front.EmptyQ() should actually never happen because we know that there is a path between the arcs!
    
    if( X_front.EmptyQ() ) { wprint(tag()+": X_front is empty, but shortest path is not found, yet."); }
    if( Y_front.EmptyQ() ) { wprint(tag()+": Y_front is empty, but shortest path is not found, yet."); }
    
    k = max_dist + 1;
    
Exit:
    
    // Write the actual path to array `path`.
    // It goes a,...,a_0,b_0,...,b
    
    PD_PRINT("Assembling path");

    PD_VALPRINT("a",a);
    PD_VALPRINT("a_0",a_0);
    PD_VALPRINT("b_0",b_0);
    PD_VALPRINT("b",b);
    
    PD_VALPRINT("a_0",a_0);
    PD_VALPRINT("b_0",b_0);
    
    PD_VALPRINT("DualArcFrom(a_0)",DualArcFrom(a_0));
    PD_VALPRINT("DualArcFrom(b_0)",DualArcFrom(b_0));
    
    PD_VALPRINT("DualArcForwardQ(a_0)",DualArcForwardQ(a_0));
    PD_VALPRINT("DualArcForwardQ(b_0)",DualArcForwardQ(b_0));
    
    std::vector<Int> X_path;
    std::vector<Int> Y_path;
    
    PD_ASSERT(DualArcForwardQ(a_0) == Head);
    PD_ASSERT(DualArcForwardQ(b_0) == Tail);
    
    if( k == Int(0) ) { pd_eprint("k == Int(0)"); }
    
    PD_VALPRINT("X_r",X_r);
    PD_VALPRINT("Y_r",Y_r);
    PD_VALPRINT("k",k);
    PD_ASSERT(k == X_r + Y_r + Int(1));
    
//    PD_VALPRINT("D_data",ToString(D_data));
    
    if( (Int(1) <= k) && (k <= max_dist) )
    {
        // The only way to get here with `k <= max_dist` is the `goto` above.
        
        path.Resize(k + Int(1));
        Int e = a_0;
        for( Int p = X_r + Int(1); p --> Int(0); )
        {
            path[p] = ToDarc(e,DualArcLeftToRightQ(e));
            e = DualArcFrom(e);
        }
        PD_ASSERT(ArcOfDarc(path[0]) == a);
        PD_ASSERT(path[0] == ToDarc(a,Tail));
        
        e = b_0;
        for( Int p = X_r + Int(1); p < path.Size(); ++p )
        {
            path[p] = ToDarc(e,DualArcLeftToRightQ(e));
            e = DualArcFrom(e);
        }
        PD_ASSERT(ArcOfDarc(path[k]) == b);
        PD_ASSERT(path[k] == ToDarc(b,Tail));
        PD_VALPRINT("path",PathString(path));
        PD_VALPRINT("path.Size()",path.Size());
        
        return true;
    }
    else
    {
        PD_PRINT("No path found.");
        path.Resize(Int(0));
        
        return false;
    }
}

private:

template<
    bool first_faceQ = false, bool second_faceQ = false,
    typename HiddenFun_T, typename ForbiddenFun_T
>
TOOLS_FORCE_INLINE bool SweepFace(
    const Int de_0, const bool forwardQ, const Int from,
    mref<Int> a_0, mref<Int> b_0, mref<Stack<Int,Int>> next,
    const Int dual_mark, cref<HiddenFun_T> hiddenQ, cref<ForbiddenFun_T> forbiddenQ
)
{
    PD_TIMER(timer,MethodName("SweepFace")+ "( de_0 = " + ToString(de_0) + ", forwardQ = " +ToString(forwardQ)+", from = " + ToString(from) + ",..., dual_mark = " + ToString(dual_mark) + " )");
    
#ifdef PD_COUNTERS
    Int f_size = 0;
#endif

#ifdef PD_DEBUG
    {
        auto [e_0,d_0] = FromDarc(de_0);
        // We never push dual arcs to the stack whose primal arcs are hidden or forbidden.
        PD_ASSERT(!hiddenQ(e_0));
        PD_ASSERT(!forbiddenQ(e_0));
    }
#endif // PD_DEBUG
    
    Int de;
    
    if constexpr ( first_faceQ || second_faceQ )
    {
        de = de_0;
    }
    else // if constexpr ( !first_faceQ && !second_faceQ )
    {
        auto [e_0,d_0] = FromDarc(de_0);
        
        //  We only push dual arcs to stack that we have explored already.
        PD_ASSERT(DualArcMark(e_0) == dual_mark);
        PD_ASSERT(DualArcForwardQ(e_0) == forwardQ);
        
        if( DualArcVisitedTwiceQ(e_0) )
        {
            // We visited this face already. We can tell because e_0 has been visited from both sides.
            return false;
        }
        
        // We move to next darc because DualArcMarkedQ(ArcOfDarc(de_0),dual_mark) is guaranteed to be true.
        de = LeftDarc(de_0);
    }
    
    do
    {
        auto [e,d] = FromDarc(de);
        
//        TOOLS_LOGDUMP(de);
//        TOOLS_LOGDUMP(e);
//        TOOLS_LOGDUMP(pd->ArcString(e));
//        TOOLS_LOGDUMP(DualArcMark(e));
//        TOOLS_LOGDUMP(DualArcMarkedQ(e,dual_mark));
//        TOOLS_LOGDUMP(DualArcForwardQ(e));
//        TOOLS_LOGDUMP(DualArcDirection(e));
//        TOOLS_LOGDUMP(DualArcFrom(e));
//        TOOLS_LOGDUMP(A_mark[e]);
//        TOOLS_LOGDUMP(hiddenQ(e));
//        TOOLS_LOGDUMP(forbiddenQ(e));

        // Only important for sweeping over the starting face.
        // If we hit the target b_0 already here, then a_0 and b_0 share a common face
        // and their distance is 0.
        if constexpr ( first_faceQ ) { if( e == b_0 ) { return true; } }
        
        if( hiddenQ(e) )
        {
            PD_PRINT("The underlying arc of darc " + ToString(de) + " is hidden. Moving to LeftDarc(ReverseDarc(de) = " + ToString(LeftDarc(ReverseDarc(de))) + ".");
            de = LeftDarc(ReverseDarc(de));
            continue;
        }
        else if ( forbiddenQ(e) )
        {
            PD_PRINT("The underlying arc of darc " + ToString(de) + " is forbidden. Moving to LeftDarc(de) = " + ToString(LeftDarc(de)) + ".");
                     
            de = LeftDarc(de);
            continue;
        }
        
#ifdef PD_COUNTERS
        ++f_size;
        RecordDualArc(de);
#endif
        
        // Check whether `e` has not been visited, yet.
        if( !DualArcMarkedQ(e,dual_mark) )
        {
            PD_PRINT("Arc " + ToString(e) + " is unvisited; marking as visited.");
            
            // Beware that dual arcs with forwardQ == false have to be traversed in reverse way when the path is rerouted. This is why we may have to flip left_to_rightQ here.
            SetDualArc(e,d,forwardQ,from,dual_mark);
            
            Int de_next = ReverseDarc(de);
            PD_PRINT("Pushing darc de_next = " + ToString(de_next) + " to stack." );
            next.Push(de_next);
        }
        else // if( DualArcMarkedQ(e,dual_mark) )
        {
            if( DualArcForwardQ(e) != forwardQ )
            {
                // We met this arc during stepping in the other direction.
                // So, we have found a shortest path.
                
                PD_PRINT("Found shortest path at e = " + ToString(e) + ".");
                
                PD_PRINT("SweepFace: setting a_0 = from = " + ToString(from) + "; b_0 = e = " + ToString(from) + ".)");
                
                // Return the two points that touch.
                a_0 = from;
                b_0 = e;
                return true;
            }
            else // if( DualArcForwardQ(e) == forwardQ )
            {
                DualArcMarkAsVisitedTwice(e);

                // If we have already visited this arc during stepping in the this direction, then we have done something wrong upstream.
                PD_ASSERT(DualArcDirection(e) != d);
            }
        }
        
        de = LeftDarc(de);
    }
    while( de != de_0 );
    
    PD_PRINT("SweepFace; de_0 = " + ToString(de_0) + " done.");
    PD_VALPRINT("next", next);
    
#ifdef PD_COUNTERS
    // We record only faces that we have completely traversed.
    RecordFaceSize(f_size);
#endif
    
    return false;
}

template<typename HiddenFun_T, typename ForbiddenFun_T>
bool SweepFront(
    const bool forwardQ, mref<Int> a_0, mref<Int> b_0, mref<Int> r, mref<Stack<Int,Int>> next_front,
    const Int dual_mark, cref<HiddenFun_T> hiddenQ, cref<ForbiddenFun_T> forbiddenQ
)
{
    PD_TIMER(timer,MethodName("SweepFront") + "( forwardQ = " + ToString(forwardQ) + ", a_0 = " + ToString(a_0) +", b_0 = " + ToString(b_0) + ", r = " + ToString(r) + ", ... )");
//    PD_PRINT("SweepFront");
//    PD_VALPRINT("forwardQ", forwardQ);
//    PD_VALPRINT("a_0", a_0);
//    PD_VALPRINT("b_0", b_0);
//    PD_VALPRINT("r", r);
    
    using std::swap;
    swap( prev_front, next_front );
    next_front.Reset();
    
    PD_VALPRINT("prev_front", prev_front);
    PD_VALPRINT("next_front", next_front);
    
    // The tips of elements of prev have distance r from e_first.
    
    std::vector<Int> visited; // Collect all visited dual arcs.

    while( !prev_front.EmptyQ() )
    {
        const Int de_0 = prev_front.Pop();
        const Int e_0  = ArcOfDarc(de_0);
        PD_PRINT("Popped de_from = " + ToString(de_0) + " from `prev_front`.");
        
        PD_VALPRINT("prev_front", prev_front);
        
        PD_ASSERT(!hiddenQ(e_0));
        PD_ASSERT(!forbiddenQ(e_0));

        if( SweepFace(de_0,forwardQ,e_0,a_0,b_0,next_front,dual_mark,hiddenQ,forbiddenQ) )
        {
            return true;
        }
        
        PD_VALPRINT("prev_front", prev_front);
        PD_VALPRINT("next_front", next_front);
    }
    
    ++r;

    return false;
}
