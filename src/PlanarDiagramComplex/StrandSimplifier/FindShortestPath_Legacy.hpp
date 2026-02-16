

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

Int FindShortestPath_Legacy_impl( const Int a_begin, const Int a_end, const Int max_dist )
{
    PD_TIMER(timer,MethodName("FindShortestPath_Legacy_impl"));
    
    PD_ASSERT(CheckLeftDarc());
    
    PD_ASSERT( a_begin != a_end );
    
    // Instead of a queue we use two stacks: One to hold the next front; and one for the previous (or current) front.
    
    X_front.Reset();
    
    // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
    
    X_front.Push(ToDarc(a_begin,Head));
    X_front.Push(ToDarc(a_begin,Tail));
    
    SetDualArc(a_begin,Head,Head,a_begin);
    
    // This is needed to prevent us from running in circles when cycling around faces.
    // See comments below.
    pd->A_scratch[a_begin] = a_begin;
    
    Int r = 0;
    
    while( (r < max_dist) && (!X_front.EmptyQ()) )
    {
        // Actually, X_front must never become empty. Otherwise something is wrong.
        std::swap( prev_front, X_front );
        
        X_front.Reset();
        
        // We don't want paths of length max_dist.
        // The elements of prev_front have distance r.
        // So the elements we push onto X_front will have distance r+1.
        
        ++r;

        while( !prev_front.EmptyQ() )
        {
            const Int da_0 = prev_front.Pop();
            auto [a_0,d_0] = FromDarc(da_0);
            
            // Now we run through the boundary arcs of the face using `LeftDarc` to turn always left.
            // There is only one exception and that is when the arc we get to is part of the strand (which is when `A_mark(a) == mark`).
            // Then we simply go straight through the crossing.

            // arc a_0 itself does not have to be processed because that's where we are coming from.
            Int da = LeftDarc(da_0);

            do
            {
                auto [a,d] = FromDarc(da);
                
                AssertArc<1>(a);
                
                const bool part_of_strandQ = ArcMarkedQ(a);

                // Check whether `a` has not been visited, yet.
                if( !DualArcMarkedQ(a) )
                {
                    if( a == a_end )
                    {
                        SetDualArc(a,d,Head,a_0);
                        goto Exit;
                    }
                    
                    PD_ASSERT( a != a_begin );
                    
                    if( part_of_strandQ )
                    {
                        // This arc is part of the strand itself, but neither a_begin nor a_end. We just skip it and do not make it part of the path.
                    }
                    else
                    {
                        SetDualArc(a,d,Head,a_0);
                        X_front.Push(ReverseDarc(da));
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
                    
                    // Wouldn't be easier to start with ReverseDarc(da_0)?
                    
                    if( pd->A_scratch[a] == a_0 ) { break; }
                }
                
                if constexpr ( mult_compQ ) { pd->A_scratch[a] = a_0; }

                if( part_of_strandQ )
                {
                    // If da is part of the current strand, we ignore i
                    da = LeftDarc(ReverseDarc(da));
                }
                else
                {
                    da = LeftDarc(da);
                }
            }
            while( da != da_0 );
        }
    }
    
    // If we get here, then r+1 = max_dist or X_front.EmptyQ().
    
    // X_front.EmptyQ() should actually never happen because we know that there is a path between the arcs!
    
    if( X_front.EmptyQ() )
    {
        wprint(MethodName("FindShortestPath_impl")+": X_front is empty, but shortest path is not found, yet.");
    }
    
    r = max_dist + 1;
    
Exit:
    
    // Write the actual path to array `path`.
    
    if( (Int(0) <= r) && (r <= max_dist) )
    {
        // The only way to get here with `r <= max_dist` is the `goto` above.
        if( !DualArcMarkedQ(a_end) )
        {
            pd_eprint(MethodName("FindShortestPath_impl") +": DualArcMarkedQ(a_end).");
            TOOLS_LOGDUMP(r);
            TOOLS_LOGDUMP(max_dist);
            TOOLS_LOGDUMP(a_end);
            TOOLS_LOGDUMP(DualArcMarkedQ(a_end));
            TOOLS_LOGDUMP(DualArcFrom(a_end));
        }
        
        Int a = a_end;
        
        path_length = r+1;
        
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

    return r;
}
