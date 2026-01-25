public:

/*! @brief Attempts to find the shortest path between the faces created by merging the two faces of arc`a` and the faces created by merging the two faces of arc `b`.
 *
 *  @param a The one end arc of the shortest path we are looking for.
 *
 *  @param b The other end arc of the shortest path we are looking for.
 *
 *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
 */

Tensor1<Int,Int> FindShortestPath(
    mref<PD_T> pd_input, const Int a, const Int b, const Int max_dist
)
{
    Load(pd_input);
    MarkArc(a);
    MarkArc(b);
    
    const Int d  = FindShortestPath(a,b,max_dist);
    
    Tensor1<Int,Int> p;
    
    if( d <= max_dist )
    {
        p = Tensor1<Int,Int> (path_length);
        p.Read( path.data() );
    }
    
    Cleanup();
    
    return p;
}

/*! @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow! (It has to find and mark a the currect path between `a` and `b`, if existent.
 *
 *  @param a The first arc of the input strand.
 *
 *  @param b The last arc of the input strand (included).
 *
 *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
 */

Tensor1<Int,Int> FindShortestRerouting(
    mref<PD_T> pd_input, const Int a, const Int b, const Int max_dist
)
{
    Load(pd_input);
    strand_length = MarkArcs(a,b);
    
    Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
    const Int d   = FindShortestPath(a,b,max_dist_);
    
    Tensor1<Int,Int> p;
    
    if( d <= max_dist )
    {
        p = Tensor1<Int,Int> (path_length);
        p.Read( path.data() );
    }
    
    Cleanup();
    
    return p;
}

private:

Int FindShortestPath( const Int a, const Int b, const Int max_dist )
{
    if( strategy == Strategy_T::DijkstraLegacy )
    {
        return FindShortestPath_DijkstraLegacy_impl(a,b,max_dist);
    }
    else
    {
        return FindShortestPath_impl(a,b,max_dist);
    }
}

private:

Int FindShortestPath_impl( const Int a, const Int b, const Int max_dist )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("FindShortestPath_impl"); };
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
    
    // Instead of two queues we use two stacks: to hold the next fronts (X_front, Y_front).
    // We also need a stack prev_front to hold the previous front.
    
    X_front.Reset();
    Y_front.Reset();
    
    // Indicate that we met a in this pass during forward stepping.
    SetDualArc(a,Head,a,Tail);
    
    // Indicate that we met b in this pass during backward stepping.
    SetDualArc(b,Tail,b,Head);
    
    Int k   = 0;
    Int X_r = 0;
    Int Y_r = 0;
    Int a_0 = a;
    Int b_0 = b;
    
    PD_PRINT("Pushing arcs of starting face to X_front.");
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
        if( SweepFace<true >(de, Head, a, a_0, b_0, X_front) ) { goto Exit; }
        ++X_r;
    }
    
    // If we arrive here, then we have the guarantee that a and b are not arcs of the same face (which would mean that we can reroute with 0 crossings).
    
    PD_PRINT("Pushing arcs of end face to Y_front.");
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
        if( SweepFace<false>(de, Tail, b, b_0, a_0, Y_front) ) { goto Exit; }
        ++Y_r;
    }
   
    // We are good to go. Now run usual sweeps.

    while( (k < max_dist) && (!X_front.EmptyQ()) && (!Y_front.EmptyQ()) )
    {
        ++k;
        bool XQ;
        
        switch( strategy )
        {
            case Strategy_T::TwoSided:
            {
                XQ = (X_front.Size() <= Y_front.Size());
                break;
            }
            case Strategy_T::Alternating:
            {
                XQ = static_cast<bool>(k % Int(2));
                break;
            }
            case Strategy_T::Dijkstra:
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
            stopQ = SweepFront( Head, a_0, b_0, X_r, X_front );
        }
        else
        {
            stopQ = SweepFront( Tail, b_0, a_0, Y_r, Y_front );
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
    
//    PD_VALPRINT("D_data",D_data);
    
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
    
    if( (Int(1) <= k) && (k <= max_dist) )
    {
        // The only way to get here with `k <= max_dist` is the `goto` above.
        
        path_length = k + Int(1);
        Int e = a_0;
        for( Int p = X_r + Int(1); p --> Int(0); )
        {
            path[p] = e;
            e = DualArcFrom(e);
        }
        PD_ASSERT(path[0] == a);
        
        e = b_0;
        for( Int p = X_r + Int(1); p < path_length; ++p )
        {
            path[p] = e;
            e = DualArcFrom(e);
        }
        PD_ASSERT(path[k] == b);
        
        PD_VALPRINT("path", ArrayToString( &path[0], {path_length} ) );
        
//        PD_PRINT("X_path");
//        {
//            Int X_e = a_0;
//            while( X_e != a )
//            {
//                X_path.push_back(X_e);
//                X_e = DualArcFrom(X_e);
//            }
//            X_path.push_back(a);
//        }
//        PD_VALPRINT("X_path", X_path);
//        PD_VALPRINT("X_path.size()", X_path.size());
//        PD_ASSERT(std::cmp_equal(X_path.size(), X_r + Int(1)));
//        
//        PD_PRINT("Y_path");
//        {
//            Int Y_e = b_0;
//            while( e != b )
//            {
//                Y_path.push_back(Y_e);
//                Y_e = DualArcFrom(Y_e);
//            }
//            Y_path.push_back(b);
//        }
//        PD_VALPRINT("Y_path", Y_path);
//        PD_VALPRINT("Y_path.size()", Y_path.size());
//        PD_ASSERT(std::cmp_equal(Y_path.size(), Y_r + Int(1)));
//        
//        path_length = static_cast<Int>(X_path.size() + Y_path.size());
//        
//        PD_VALPRINT("path_length", path_length);
//        
//        PD_PRINT("Merge");
//        for( Size_T i = 0; i < X_path.size(); ++i )
//        {
//            path[X_path.size()-1-i] = X_path[i];
//        }
//        path[0] = a; // Correcting the start arc.
//        
//        for( Size_T i = 0; i < Y_path.size(); ++i )
//        {
//            path[X_path.size() + i] = Y_path[i];
//        }
//        path[path_length-1] = b; // Correcting the end arc.
//        
//        PD_VALPRINT("path", ArrayToString( &path[0], {path_length} ) );
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
    mref<Stack<Int,Int>> next_front
)
{
    PD_PRINT("SweepFront");
    PD_VALPRINT("forwardQ", forwardQ);
    PD_VALPRINT("a_0", a_0);
    PD_VALPRINT("b_0", b_0);
    PD_VALPRINT("r", r);
    
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
        
        PD_ASSERT(!ArcMarkedQ(e_0) );

        if( SweepFace(de_0,forwardQ,e_0,a_0,b_0,next_front) ) { return true; }
        
        PD_VALPRINT("prev_front", prev_front);
        PD_VALPRINT("next_front", next_front);
    }
    
    ++r;

    return false;
}
