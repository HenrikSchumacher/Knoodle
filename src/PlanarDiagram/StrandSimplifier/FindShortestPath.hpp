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
     *
     * @param mark    Indicator that is written to `D_mark`; this avoids having to erase the whole vector `D_from` for each new search. When we call this, we assume that `D_mark` contains only values different from `current_mark`.
     *
     */
    
    Int FindShortestPath_impl(
        const Int a_begin, const Int a_end, const Int max_dist, const Mark_T mark
    )
    {
        PD_TIMER(timer,MethodName("FindShortestPath_impl"));
     
#ifdef PD_TIMINGQ
        const Time start_time = Clock::now();
#endif
        
        PD_ASSERT( mark > Mark_T(0) );
        
        PD_ASSERT( a_begin != a_end );
        
        // Instead of a queue we use two stacks: One to hold the next front; and one for the previous (or current) front.
        
        next_front.Reset();
        
        // Push the arc `a_begin` twice onto the stack: once with forward orientation, once with backward orientation.
        
        next_front.Push( ToDarc(a_begin,Head) );
        next_front.Push( ToDarc(a_begin,Tail) );
        
        D_mark(a_begin) = mark;
        D_from(a_begin) = a_begin;
        
        // This is needed to prevent us from running in circles when cycling around faces.
        // See comments below.
        A_source[a_begin] = a_begin;
        
        Int d = 0;
        
        while( (d < max_dist) && (!next_front.EmptyQ()) )
        {
            // Actually, next_front must never become empty. Otherwise something is wrong.
            std::swap( prev_front, next_front );
            
            next_front.Reset();
            
            // We don't want paths of length max_dist.
            // The elements of prev_front have distance d.
            // So the elements we push onto next_front will have distance d+1.
            
            ++d;

            while( !prev_front.EmptyQ() )
            {
                const Int da_0 = prev_front.Pop();
                
                auto [a_0,d_0] = PD_T::FromDarc(da_0);
                
                // Now we run through the boundary arcs of the face using `dA_left` to turn always left.
                // There is only one exception and that is when the arc we get to is part of the strand (which is when `A_mark(a) == mark`).
                // Then we simply go straight through the crossing.

                // arc a_0 itself does not have to be processed because that's where we are coming from.
                Int da = dA_left[da_0];

                do
                {
                    auto [a,dir] = PD_T::FromDarc(da);
                    
                    AssertArc<1>(a);
                    
                    const bool part_of_strandQ = (A_mark(a) == mark);

                    // Check whether `a` has not been visited, yet.
                    if( Abs(D_mark(a)) != mark )
                    {
                        if( a == a_end )
                        {
                            // Mark as visited.
                            D_mark(a) = mark;
                            
                            // Remember from which arc we came
                            D_from(a) = a_0;
                            
                            goto Exit;
                        }
                        
                        PD_ASSERT( a != a_begin );
                        
                        if( part_of_strandQ )
                        {
                            // This arc is part of the strand itself, but neither a_begin nor a_end. We just skip it and do not make it part of the path.
                        }
                        else
                        {
                            // We make D_mark(a) positive if we cross arc `a` from left to right. Otherwise it is negative.
                            
                            D_mark(a) = dir ? mark : -mark;
                            
                            // Remember the arc we came from.
                            D_from(a) = a_0;

                            next_front.Push(FlipDarc(da));
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
                        
                        if( A_source[a] == a_0 )
                        {
                            break;
                        }
                    }
                    
                    if constexpr ( mult_compQ )
                    {
                        A_source[a] = a_0;
                    }

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
        
        // If we get here, then d+1 = max_dist or next_front.EmptyQ().
        
        // next_front.EmptyQ() should actually never happen because we know that there is a path between the arcs!
        
        if( next_front.EmptyQ() )
        {
            wprint(ClassName()+"::ShortestPath: next_front is empty, but shortest path is not found, yet.");
        }
        
        d = max_dist + 1;
        
    Exit:
        
        // Write the actual path to array `path`.
        
        if( (Int(0) <= d) && (d <= max_dist) )
        {
            // The only way to get here with `d <= max_dist` is the `goto` above.
            
            if( Abs(D_mark(a_end)) != mark )
            {
                pd_eprint(ClassName()+"::FindShortestPath_impl");
                TOOLS_LOGDUMP(d);
                TOOLS_LOGDUMP(max_dist);
                TOOLS_LOGDUMP(a_end);
                TOOLS_LOGDUMP(mark);
                TOOLS_LOGDUMP(current_mark);
                TOOLS_LOGDUMP(D_mark(a_end));
                TOOLS_LOGDUMP(D_from(a_end));
            }
            
            Int a = a_end;
            
            path_length = d+1;
            
            for( Int i = 0; i < path_length; ++i )
            {
                path[path_length-1-i] = a;
                
                a = D_from(a);
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




//public:
//    
//    /*!
//     * @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
//     *
//     * @param a_first The first arc of the input strand.
//     *
//     * @param a_last The last arc of the input strand (included).
//     *
//     * @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
//     */
//    
//    Tensor1<Int,Int> FindShortestPath(
//        const Int a_first, const Int a_last, const Int max_dist
//    )
//    {
//        Prepare();
//        
//        current_mark = 1;
//        
//        Int a = a_first;
//        Int b = a_last;
//        
//        strand_length = ColorArcs(a,b,current_mark);
//
//        Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
//        
//        const Int d = FindShortestPath_impl(a,b,max_dist_,current_mark);
//        
//        Tensor1<Int,Int> p;
//        
//        if( d <= max_dist )
//        {
//            p = Tensor1<Int,Int> (path_length);
//            
//            p.Read( path.data() );
//        }
//        
//        Cleanup();
//        
//        return p;
//    }
    

//    private:
//
//        // Only meant for debugging. Don't do this in production!
//        Tensor1<Int,Int> GetShortestPath_impl(
//            const Int a_first, const Int a_last, const Int max_dist, const Int mark
//        )
//        {
//            const Int d = FindShortestPath_impl( a_first, a_last, max_dist, mark );
//
//            if( d <= max_dist )
//            {
//                Tensor1<Int,Int> path_out (path_length);
//
//                path_out.Read( path.data() );
//
//                return path_out;
//            }
//            else
//            {
//                return Tensor1<Int,Int>();
//            }
//        }
