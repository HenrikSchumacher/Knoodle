public:

/*! @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
 *
 *  @param a The first arc of the input strand.
 *
 *  @param b The last arc of the input strand (included).
 *
 *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
 */

Tensor1<Int,Int> FindShortestPath2( const Int a, const Int b, const Int max_dist )
{
    Prepare();
    MarkArc(a);
    MarkArc(b);
    
    const Int d  = FindShortestPath2_impl(a,b,max_dist);
    
    Tensor1<Int,Int> p;
    
    if( d <= max_dist )
    {
        p = Tensor1<Int,Int> (path_length);
        p.Read( path.data() );
    }
    
    Cleanup();
    
    return p;
}

Tensor1<Int,Int> FindShortestRerouting2( const Int a, const Int b, const Int max_dist )
{
    Prepare();
    strand_length = MarkArcs(a,b);
    
    Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
    const Int d   = FindShortestPath2_impl(a,b,max_dist_);
    
    Tensor1<Int,Int> p;
    
    if( d <= max_dist )
    {
        p = Tensor1<Int,Int> (path_length);
        p.Read( path.data() );
    }
    
    Cleanup();
    
    return p;
}





Int FindShortestPath2_impl( const Int a, const Int b, const Int max_dist )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindShortestPath2_impl"); };
    PD_TIMER(timer,tag());
 
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    PD_ASSERT(CheckDarcLeftDarc());
    
    PD_ASSERT( a != b );
    PD_ASSERT(ArcMarkedQ(a));
    PD_ASSERT(ArcMarkedQ(b));
    
    PD_VALPRINT("a",a);
    PD_VALPRINT("b",b);
    
    // Two-sided graph Dijkstra to find shortest path.
    
    // Instead of two queues we use two pairs stacks: to hold the next fronts (X_next, Y_next) and to holds for the previous  (or current) fronts (X_prev, Y_prev).
    
    X_next.Reset();
    Y_next.Reset();
    
    // Indicate that we met a in this pass during forward stepping.
    SetDualArc(a,Head,a,Tail);
    
    // Indicate that we met b in this pass during backward stepping.
    SetDualArc(b,Tail,b,Head);
    
    Int k   = 0;
    Int X_r = 0;
    Int Y_r = 0;
    Int a_0 = a;
    Int b_0 = b;
    
    PD_PRINT("Pushing arcs of starting face to X_next.");
    {
        PD_ASSERT( ArcMarkedQ(a) );
    
        ++k;
        const Int da = ToDarc(a,Tail);
        
        Int de = dA_left[da];
        auto [e,left_to_rightQ] = FromDarc(de);
        
        while( ArcMarkedQ(e) && (de != da) )
        {
            // a and b share a common face.
            if( e == b )
            {
                SetDualArc(e,Head,a,Head==left_to_rightQ);
                goto Exit;
            }
            de = dA_left[FlipDarc(de)];
            std::tie(e,left_to_rightQ) = FromDarc(de);
        }
        
        PD_VALPRINT("da",da);
        PD_VALPRINT("de",de);
        
        // TODO: Handle this error.
        if( de == da ) { eprint("starting arc  a " + ArcString(a) + " is isolated."); }
        
        //  CAUTION: This is crucial!  |
        //                             V
        if( SweepFace<true >(de, Head, a, a_0, b_0, X_next) ) { goto Exit; }
        ++X_r;
    }
    
    // If we arrive here, then we have the guarantee that a and b are not arcs of the same face (which would mean that we can reroute with 0 crossings).
    
    PD_PRINT("Pushing arcs of end face to Y_next.");
    {
        PD_ASSERT( ArcMarkedQ(b) );
        
        ++k;
        const Int db = ToDarc(b,Head);
        const Int de = LeftUnmarkedDarc(db);
        
        PD_VALPRINT("db",db);
        PD_VALPRINT("de",de);
        
        // TODO: Handle this error.
        if( de == db ) { eprint("starting arc  b " + ArcString(b) + " is isolated."); }

        //  CAUTION: This is crucial!  |
        //                             V
        if( SweepFace<false>(de, Tail, b, b_0, a_0, Y_next) ) { goto Exit; }
        ++Y_r;
    }
   
    // We are good to go. Now run usual sweeps.

    while( (k < max_dist) && (!X_next.EmptyQ()) && (!Y_next.EmptyQ()) )
    {
        ++k;
        bool stopQ;
        
        if constexpr ( strategy == SearchStrategy_T::TwoSided )
        {
            if( X_next.Size() <= Y_next.Size() )
            {
                stopQ = SweepFront( Head, a_0, b_0, X_r, X_prev, X_next );
            }
            else
            {
                stopQ = SweepFront( Tail, b_0, a_0, Y_r, Y_prev, Y_next );
            }
        }
        else // if constexpr ( strategy == SearchStrategy_T::Dijkstra )
        {
            stopQ = SweepFront( Head, a_0, b_0, X_r, X_prev, X_next );
        }
        
        if( stopQ ) { goto Exit; }
        
        PD_VALPRINT("X_next",X_next);
        PD_VALPRINT("Y_next",Y_next);
    }
    
    // If we get here, then d + 1 == max_dist or X_next.EmptyQ() or Y_next.EmptyQ().
    
    // X_next.EmptyQ() and Y_next.EmptyQ() should actually never happen because we know that there is a path between the arcs!
    
    if( X_next.EmptyQ() )
    {
        wprint(tag()+": X_next is empty, but shortest path is not found, yet.");
    }
    if( Y_next.EmptyQ() )
    {
        wprint(tag()+": Y_next is empty, but shortest path is not found, yet.");
    }
    
    k = max_dist + 1;
    
Exit:
    
    // Write the actual path to array `path`.
    // It goes a,...,a_0,b_0,...,b
    
    PD_PRINT("Assembling path");
    PD_VALPRINT("X_r",X_r);
    PD_VALPRINT("Y_r",Y_r);
    PD_VALPRINT("k",k);

    PD_VALPRINT("a",a);
    PD_VALPRINT("a_0",a_0);
    PD_VALPRINT("b_0",b_0);
    PD_VALPRINT("b",b);
    
    PD_VALPRINT("D_data",D_data);
    
    PD_VALPRINT("a_0",a_0);
    PD_VALPRINT("b_0",b_0);
    
    PD_VALPRINT("DualArcFrom(a_0)",DualArcFrom(a_0));
    PD_VALPRINT("DualArcFrom(b_0)",DualArcFrom(b_0));
    
    PD_VALPRINT("DualArcForwardQ(a_0)",DualArcForwardQ(a_0));
    PD_VALPRINT("DualArcForwardQ(b_0)",DualArcForwardQ(b_0));
    
    std::vector<Int> path_0;
    std::vector<Int> path_1;
    
    PD_ASSERT(DualArcForwardQ(a_0) == Head);
    PD_ASSERT(DualArcForwardQ(b_0) == Tail);
    
    if( k == Int(0) ) { pd_eprint("k == Int(0)"); }
    
    if( (Int(1) <= k) && (k <= max_dist) )
    {
        // The only way to get here with `d <= max_dist` is the `goto` above.
        PD_PRINT("First part");
        {
            Int e = a_0;
            while( e != a )
            {
                path_0.push_back(e);
                e = DualArcFrom(e);
            }
            path_0.push_back(a);
        }
        
        PD_PRINT("Second part");
        {
            Int e = b_0;
            while( e != b )
            {
                path_1.push_back(e);
                e = DualArcFrom(e);
            }
            path_1.push_back(b);
        }
        
        PD_VALPRINT("path_0", path_0);
        PD_VALPRINT("path_0.size()", path_0.size());
        PD_VALPRINT("path_1", path_1);
        PD_VALPRINT("path_1.size()", path_1.size());
        
        path_length = static_cast<Int>(path_0.size() + path_1.size());
        
        PD_VALPRINT("path_length", path_length);
        
        PD_PRINT("Merge");
        for( Size_T i = 0; i < path_0.size(); ++i )
        {
            path[path_0.size()-1-i] = path_0[i];
        }
        path[0] = a; // Correcting the start arc.
        
        for( Size_T i = 0; i < path_1.size(); ++i )
        {
            path[path_0.size() + i] = path_1[i];
        }
        path[path_length-1] = b; // Correcting the start arc.
        
        PD_VALPRINT("path", ArrayToString( &path[0], {path_length} ) );
    }
    else
    {
        PD_PRINT("No path found.");
        path_length = 0;
    }

#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_FindShortestPath += Tools::Duration(start_time,stop_time);
#endif

    return k;
}

private:

Int LeftUnmarkedDarc( const Int de_0 )
{
    Int de = dA_left[de_0];
    
    while( (de != de_0) && ArcMarkedQ(ArcOfDarc(de)) )
    {
        de = dA_left[FlipDarc(de)];
    }
    
    if( ArcMarkedQ(ArcOfDarc(de)) )
    {
        wprint( MethodName("LeftUnmarkedDarc") + ": Cannot find proper darc.");
    }
    
    return de;
}

template<bool first_faceQ = false>
bool SweepFace(
    const Int de_0, const bool forwardQ, const Int from,
    mref<Int> a_0, mref<Int> b_0,
    mref<Stack<Int,Int>> next
)
{
    PD_PRINT("SweepFace; de_0 = " + ToString(de_0) + ".");
    
    PD_ASSERT(!ArcMarkedQ(ArcOfDarc(de_0)));
    
    Int de = de_0;
    do
    {
        auto [e,left_to_rightQ] = FromDarc(de);
        
        // Only important for sweep the sweep over the starting face.
        // If we hit the target b_0 already here, then a_0 and b_0 share a common face
        // and their distance is 0.
        if constexpr ( first_faceQ )
        {
            if( e == b_0 ) { return true; }
        }
        
        // This is to ignore marked arcs.
        if( ArcMarkedQ(e) )
        {
            de = dA_left[FlipDarc(de)];
            continue;
        }
        
        // Check whether `e` has not been visited, yet.
        if( !DualArcMarkedQ(e) )
        {
            PD_PRINT("Arc " + ToString(e) + " is unvisited; marking as visited.");

            // Beware that dual arcs with forwardQ == false have to be traversed in reverse way when the path is rerouted. This is why we may have to flip left_to_rightQ here.
            SetDualArc( e, forwardQ, from, forwardQ == left_to_rightQ );
            
            Int de_next = FlipDarc(de);
            PD_PRINT("Pushing darc de_next = " + ToString(de_next) + " to stack." );
            next.Push(de_next);
        }
        else if( DualArcForwardQ(e) != forwardQ )
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
        
        de = dA_left[de];
    }
    while( de != de_0 );
    
    PD_PRINT("SweepFace; de_0 = " + ToString(de_0) + " done.");
    PD_VALPRINT("next", next);
    
    return false;
}

bool SweepFront(
    const bool   forwardQ,
    mref<Int>    a_0,
    mref<Int>    b_0,
    mref<Int>    r,
    mref<Stack<Int,Int>> prev,
    mref<Stack<Int,Int>> next
)
{
    PD_PRINT("SweepFront");
    PD_VALPRINT("forwardQ", forwardQ);
    PD_VALPRINT("a_0", a_0);
    PD_VALPRINT("b_0", b_0);
    PD_VALPRINT("r", r);
    
    using std::swap;
    swap( prev, next );
    next.Reset();
    
    PD_VALPRINT("prev", prev);
    PD_VALPRINT("next", next);
    
    // The tips of elements of prev have distance r from e_first.
    
    std::vector<Int> visited; // Collect all visited dual arcs.

    while( !prev.EmptyQ() )
    {
        const Int de_0 = prev.Pop();
        const Int e_0  = ArcOfDarc(de_0);
        PD_PRINT("Popped de_from = " + ToString(de_0) + " from `prev`.");
        
        PD_VALPRINT("prev", prev);
        
        PD_ASSERT(!ArcMarkedQ(e_0) );

        if( SweepFace(de_0,forwardQ,e_0,a_0,b_0,next) ) { return true; }
        
        PD_VALPRINT("prev", prev);
        PD_VALPRINT("next", next);
    }
    
    ++r;

    return false;
}






//        path_length = d + Int(1);
//
//        {
//            Int e = a_0;
//            for( Int pos = X_r; pos >= Int(0); --pos ) // CAUTION: Unconventional loop.
//            {
//                path[pos] = e;
//                e = D_from(e);
//            }
//        }
//        {
//            Int e = b_0;
//            for( Int pos = X_r + Int(1); pos < path_length; ++pos )
//            {
//                path[pos] = e;
//                e = D_from(e);
//            }
//        }
//
////        print("Is this right?");
////        valprint("path", ArrayToString( &path[0], {path_length} ) );
//        logprint("Is this right?");
//        logvalprint("path", ArrayToString( &path[0], {path_length} ) );
