public:
    
/*! @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
 *
 *  @param a The first arc of the input strand.
 *
 *  @param b The last arc of the input strand (included).
 *
 *  @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
 */

Tensor1<Int,Int> FindShortestPath( const Int a, const Int b, const Int max_dist )
{
    Prepare();

    strand_length = MarkArcs(a,b);

    Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
    
    const Int d = FindShortestPath_impl(a,b,max_dist_);
    
    Tensor1<Int,Int> p;
    
    if( d <= max_dist )
    {
        p = Tensor1<Int,Int> (path_length);
        p.Read( path.data() );
    }
    
    Cleanup();
    
    return p;
}

Tensor1<Int,Int> FindShortestRerouting( const Int a, const Int b, const Int max_dist )
{
    Prepare();
    strand_length = MarkArcs(a,b);
    
    Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
    const Int d   = FindShortestPath_impl(a,b,max_dist_);
    
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
    
/*!
 * Run Dijkstra's algorithm to find the shortest path from arc `a_begin` to `a_end` in the graph G, where the vertices of G are the arcs and where there is an edge between two such vertices if the corresponding arcs share a common face.
 *
 * If an improved path has been found, its length is stored in `path_length` and the actual path is stored in the leading positions of `path`.
 *
 * @param a_begin  Starting arc.
 *
 * @param a_end    Final arc to which we want to find a path
 *
 * @param max_dist Maximal distance we want to travel
 */

Int FindShortestPath_impl( const Int a_begin, const Int a_end, const Int max_dist )
{
    PD_TIMER(timer,MethodName("FindShortestPath_impl"));
 
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    PD_ASSERT(CheckDarcLeftDarc());
    
    PD_ASSERT( a_begin != a_end );
    
    // Instead of a queue we use two stacks: One to hold the next front; and one for the previous (or current) front.
    
    X_next.Reset();
    
    // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
    
    X_next.Push(ToDarc(a_begin,Head));
    X_next.Push(ToDarc(a_begin,Tail));
    
    SetDualArc(a_begin,Head,a_begin,Head);
    
    // This is needed to prevent us from running in circles when cycling around faces.
    // See comments below.
    D_mark2[a_begin] = a_begin;
    
    Int d = 0;
    
    while( (d < max_dist) && (!X_next.EmptyQ()) )
    {
        // Actually, X_next must never become empty. Otherwise something is wrong.
        std::swap( X_prev, X_next );
        
        X_next.Reset();
        
        // We don't want paths of length max_dist.
        // The elements of X_prev have distance d.
        // So the elements we push onto X_next will have distance d+1.
        
        ++d;

        while( !X_prev.EmptyQ() )
        {
            const Int da_0 = X_prev.Pop();
            auto [a_0,d_0] = FromDarc(da_0);
            
            // Now we run through the boundary arcs of the face using `dA_left` to turn always left.
            // There is only one exception and that is when the arc we get to is part of the strand (which is when `A_mark(a) == mark`).
            // Then we simply go straight through the crossing.

            // arc a_0 itself does not have to be processed because that's where we are coming from.
            Int da = dA_left[da_0];

            do
            {
                auto [a,left_to_rightQ] = FromDarc(da);
                
                AssertArc<1>(a);
                
                const bool part_of_strandQ = ArcMarkedQ(a);

                // Check whether `a` has not been visited, yet.
                if( !DualArcMarkedQ(a) )
                {
                    if( a == a_end )
                    {
                        SetDualArc(a,Head,a_0,left_to_rightQ);
                        goto Exit;
                    }
                    
                    PD_ASSERT( a != a_begin );
                    
                    if( part_of_strandQ )
                    {
                        // This arc is part of the strand itself, but neither a_begin nor a_end. We just skip it and do not make it part of the path.
                    }
                    else
                    {
                        SetDualArc(a,Head,a_0,left_to_rightQ);
                        X_next.Push(FlipDarc(da));
                    }
                }
                else if constexpr ( mult_compQ )
                {
                    // When the diagram becomes disconnected by removing the strand, then there can be a face whose boundary has two components. Even weirder, it can happen that we start at `a_0` and then go to the component of the face boundary that is _not_ connected to `a_0`. This is difficult to imagine, so here is a picture:
                    
                    
                    // The == indicates the current overstrand.
                    //
                    // If we start at `a_0 = a_begin`, then we turn left and get to the split link component, which happens to be a trefoil. Since we ignore all arcs that are part of the strand, we cannot leave this trefoil! This is not too bad because the ideal path would certainly not cross the trefoil. We have just to make sure that we do not cycle indefinitely around the trefoil.
                    //
                    //                      +---+
                    //                      |   |
                    //               +------|-------+
                    //   ------+     |      |   |   |   +---
                    //         |     |      +---|---+   |
                    //         | a_0 |          |       |  a_end |
                    //      ---|=================================|---
                    //         |     |          |       |        |
                    //         |     +----------+       |
                    //   ------+                        +---
                    
                    // Wouldn't be easier to start with FlipDarc(da_0)?
                    
                    if( D_mark2[a] == a_0 ) { break; }
                }
                
                if constexpr ( mult_compQ ) { D_mark2[a] = a_0; }

                if( part_of_strandQ )
                {
                    // If da is part of the current strand, we ignore i
                    da = dA_left[FlipDarc(da)];
                }
                else
                {
                    da = dA_left[da];
                }
            }
            while( da != da_0 );
        }
    }
    
    // If we get here, then d+1 = max_dist or X_next.EmptyQ().
    
    // X_next.EmptyQ() should actually never happen because we know that there is a path between the arcs!
    
    if( X_next.EmptyQ() )
    {
        wprint(MethodName("FindShortestPath_impl")+": X_next is empty, but shortest path is not found, yet.");
    }
    
    d = max_dist + 1;
    
Exit:
    
    // Write the actual path to array `path`.
    
    if( (Int(0) <= d) && (d <= max_dist) )
    {
        // The only way to get here with `d <= max_dist` is the `goto` above.
        if( !DualArcMarkedQ(a_end) )
        {
            pd_eprint(MethodName("FindShortestPath_impl") +": DualArcMarkedQ(a_end).");
            TOOLS_LOGDUMP(d);
            TOOLS_LOGDUMP(max_dist);
            TOOLS_LOGDUMP(a_end);
            TOOLS_LOGDUMP(DualArcMarkedQ(a_end));
            TOOLS_LOGDUMP(DualArcFrom(a_end));
        }
        
        Int a = a_end;
        
        path_length = d+1;
        
        for( Int i = 0; i < path_length; ++i )
        {
            path[path_length-1-i] = a;
            a = DualArcFrom(a);
        }
    }
    else
    {
        path_length = 0;
    }

#ifdef PD_TIMINGQ
    const Time stop_time = Clock::now();
    
    Time_FindShortestPath += Tools::Duration(start_time,stop_time);
#endif

    return d;
}
