private:

/*!@brief Run Dijkstra's algorithm to find the shortest path from arc `a_first` to `a_last` in a certain "dual graph `G`.
 *
 * The graph G is defined as follows:
 * Consider the subiagram `D` obtained from the current planar diagram by removing the arcs that bear `current_mark` in `A_mark` except `a_first` and `a_last`. Let `F(D)` be the list of faces in this subdiagram `D`. The vertices of `G` are the arcs in `D`. Two such arcs are connected by an edge of `G` if and only if they share a common neighboring face in `F(D)`.
 *
 * Equivalently, this can be phrased as follows: Let `H` be the diagram obtained from the planar diagram by removing _all_ the arcs that bear `current_mark` in `A_mark`.
 * Arc `a_first` has two neighbor faces in the original planar diagram. In `H` they will be fused to a face `f_begin`. Likewise, `a_last` has two neighbor faces in the original planar diagram. In `H` they will be fused to a face `f_end`.
 * Now consider the dual multigraph `H^*`: its vertices are the faces and its edges correpond to the arcs of `H`: Our goal is to find the shortest path from `f_0` to `f_1` in the graph `H^*`.
 *
 * If an improved path has been found, its length is stored in `path_length` and the actual path is stored in the leading positions of `path`.
 *
 * @param a_first  Starting arc.
 *
 * @param a_last    Final arc to which we want to find a path.
 *
 * @param max_dist Maximal distance we want to travel.
 *
 * @param mark     Indicator that is written to `D_mark`; this avoids having to erase the whole vector `A_from` for each new search. When we call this, we assume that `D_mark` contains only values different from `current_mark`.
 */

Int FindShortestPath_impl(
    const Int a_first, const Int a_last, const Int max_dist, const Mark_T mark
)
{
    PD_TIMER(timer,MethodName("FindShortestPath_impl"));
 
#ifdef PD_TIMINGQ
    const Time start_time = Clock::now();
#endif
    
    PD_ASSERT( mark > Mark_T(0) );
    
    PD_ASSERT( a_first != a_last );
    
    // Instead of a queue we use two stacks: One to hold the next front; and one for the previous (or current) front.
    
    next_front.Reset();
    
    // Push the arc `a_first` twice onto the stack: once with forward orientation, once with backward orientation.
    
    next_front.Push( PD_T::ToDarc(a_first,Head) );
    next_front.Push( PD_T::ToDarc(a_first,Tail) );
    
    D_mark(a_first)   = mark;
    D_from(a_first)   = a_first;
    D_source[a_first] = a_first;
    
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

        // For each `da_0` in `prev_front` visit all its neighbors; put the previously unvisited ones into `next_front`.
        while( !prev_front.EmptyQ() )
        {
            const Int da_0 = prev_front.Pop();
            
            auto [a_0,d_0] = PD_T::FromDarc(da_0);
            
            // Now we run through the boundary arcs of the face using `dA_left` to turn always left.
            // There is only one exception and that is when the arc we get to is part of the strand (which is when `A_mark(a) == mark_`).
            // Then we simply go straight through the crossing.

            // arc a_0 itself does not have to be processed because that's where we are coming from.
            Int da = dA_left[da_0];

            // Travel once around the face with `dA_left`, skipping the arcs `a` that have `D_mark(a) == mark`, because those are already visited.
            do
            {
                auto [a,dir] = PD_T::FromDarc(da);
                AssertArc<1>(a);
                
                const bool part_of_strandQ = (A_mark(a) == mark);
                
                if( Abs(D_mark(a)) != mark )
                {
                    // Neither `da` nor its opposite has been visited, yet.
                    if( a == a_last )
                    {
                        // We visit `a_last` for the first time. Hence, we have found the shortest path!
                        
                        // Mark as visited.
                        D_mark(a) = mark;
                        
                        // Remember from which arc we came
                        D_from(a) = a_0;
                        goto Exit;
                    }
                    
                    // We cannot be at `a_first` because we have D_mark(a)) != mark and D_mark(a_first)) == mark.
                    PD_ASSERT( a != a_first );
                    
                    if( part_of_strandQ )
                    {
                        // This arc is part of the strand itself, but neither `a_first` nor `a_last`. We just skip it and do not make it part of the path.
                    }
                    else
                    {
                        // We make D_mark(a) positive if we cross arc `a` from left to right. Otherwise it is negative.
                        D_mark(a) = dir ? mark : -mark;
                        
                        // Remember the arc we came from, so that we can trace back the path later.
                        D_from(a) = a_0;

                        next_front.Push(PD_T::FlipDarc(da));
                    }
                }
                else // if( Abs(D_mark(a)) == mark )
                {
                    if constexpr ( mult_compQ )
                    {
                        // We have visited this dual arc at least once before for this mark.
                        // Check, whether we visited during the face traversal that started at a_0.
                        if( D_source(a) == a_0 )
                        {
                            break;
                        }
                        // This is to prevent the following issue:
                        
                        // When the diagram becomes disconnected by removing the strand, then there can be a face whose boundary has two components. Even weirder, it can happen that we start at `a_0` and then go to the component of the face boundary that is _not_ connected to `a_0`. This is difficult to imagine, so here is a picture:
                        //
                        //
                        // The == indicates the current overstrand.
                        //
                        //                      +---+
                        //                      |   |
                        //               +------|-------+
                        //   ------+     |      |   |   |   +---
                        //         |     |      +---|---+   |
                        //         | a_0 |          |       |  a_last |
                        //      ---|=================================|---
                        //         |     |          |       |        |
                        //         |     +----------+       |
                        //   ------+                        +---
                        //
                        // If we start at `a_0 = a_first`, then we turn left and get to the split link component, which happens to be a trefoil. Since we ignore all arcs that are part of the strand, we cannot leave this trefoil! This is not too bad because the ideal path would certainly not cross the trefoil. We have just to make sure that we do not cycle indefinitely around the trefoil.
                    }
                }

                if constexpr ( mult_compQ )
                {
                    // Make sure that Abs(D_mark(a)) == mark and D_source(a) == a_0.
                    // This will indicate that we have visited this dual arc for this `mark` and for this face traversal starting at a_0.
                    D_source(a) = a_0;
                }

                // Move to next arc around face.
                if( part_of_strandQ )
                {
                    // If da is part of the current strand, we ignore ignore it.
                    // To make sure that we traverse the face of the subdiagram `D`, we turn around.
                    da = dA_left[PD_T::FlipDarc(da)];
                }
                else
                {
                    da = dA_left[da];
                }
            }
            while( da != da_0 );
        }
        
        // Now we have traversed the whole previous frontier `prev_front`.
    }
    
    // If we get here, then d+1 = max_dist or next_front.EmptyQ().
    
    // next_front.EmptyQ() should actually never happen because we know that there is a path between the arcs!
    
    if( next_front.EmptyQ() )
    {
        wprint(MethodName("FindShortestPath_impl")+": next_front is empty, but shortest path is not found, yet. Apparently, a_first and a_last lie in different diagram components.");
    }
    
    d = max_dist + 1;
    
Exit:
    
    // Write the actual path to array `path`.
    
    if( (Int(0) <= d) && (d <= max_dist) )
    {
        // The only way to get here with `d <= max_dist` is the `goto` above.
        
        if( Abs(D_mark(a_last)) != mark )
        {
            pd_eprint(MethodName("FindShortestPath_impl") + "We should never get here.");
            TOOLS_LOGDUMP(d);
            TOOLS_LOGDUMP(max_dist);
            TOOLS_LOGDUMP(a_last);
            TOOLS_LOGDUMP(mark);
            TOOLS_LOGDUMP(current_mark);
            TOOLS_LOGDUMP(D_mark(a_last));
            TOOLS_LOGDUMP(D_from(a_last));
        }
        
        Int a = a_last;
        
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

//    private:
//
//        // Only meant for debugging. Don't do this in production!
//        Tensor1<Int,Int> GetShortestPath_impl(
//            const Int a_first, const Int a_last, const Int max_dist, const Int mark_
//        )
//        {
//            const Int d = FindShortestPath_impl( a_first, a_last, max_dist, mark_ );
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




public:

///*!
// * @brief Attempts to find the arcs that make up a minimally rerouted strand. This routine is only meant for the visualization of a few paths. Don't use this in production as this is quite slow!
// *
// * @param a_first The first arc of the input strand.
// *
// * @param a_last The last arc of the input strand (included).
// *
// * @param max_dist Maximal length of the path we are looking for. If no path exists that satisfies this length constraint, then an empty list is returned.
// */
//
//Tensor1<Int,Int> FindShortestPath(
//    const Int a_first, const Int a_last, const Int max_dist
//)
//{
//    Prepare();
//
//    // TODO: Why do we do this??
//    // We reset the mark.
//    current_mark = 1;
//
//    Int a = a_first;
//    Int b = a_last;
//
//    strand_length = MarkArcs(a,b,current_mark);
//
//    Int max_dist_ = Min(Ramp(strand_length - Int(2)),max_dist);
//
//    const Int d = FindShortestPath_impl(a,b,max_dist_,current_mark);
//
//    Tensor1<Int,Int> p;
//
//    if( d <= max_dist )
//    {
//        p = Tensor1<Int,Int> (path_length);
//
//        p.Read(path.data());
//    }
//
//    Cleanup();
//
//    return p;
//}
