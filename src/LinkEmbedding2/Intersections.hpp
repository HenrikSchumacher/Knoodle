public:

/*!@brief Guarantee that the intersections are computed.
 *
 * @param force_recomputeQ If set to `true`, a recomputation of the intersections is enforced, even if the intersections are already computed. (Probably only useful for benchmarking and debugging.)
 *
 * @return A boolean. If `true` is returned, the intersections have been computed successfully. If `false` is returned, then either no vertex coordinates are loaded (check `VertexCoordinatesLoadedQ()`) or some self-intersection of the link in 3-space have been detected (check `IntersectionCount3D()`.)
 */
template<bool verboseQ = true> // whether to print errors and warnings
[[nodiscard]] bool RequireIntersections( bool force_recomputeQ = false )
{
   [[maybe_unused]] auto tag = [](){ return MethodName("RequireIntersections"); };
       
    
    if( force_recomputeQ || !intersections_computedQ )
    {
        ComputeIntersections();
    }
    
    if( !intersections_computedQ )
    {
        if( !vertex_coords_loadedQ )
        {
            wprint(tag() + ": Failed to compute intersections because not vertex coordinates have been loaded, yet." );
        }
        else
        {
            wprint(tag() + ": Failed to compute intersections for an unknown reason." );
        }
        return false;
    }
    
    if( intersection_count_3D > Int(0) )
    {
        wprint(tag() + ": Detected at least "  + ToString(intersection_count_3D)+ " self-intersections in 3-space after perturbation. Link is not an embedding." );
        return false;
    }
    
    return true;
}


/*!@brief Return flag that signals whether the intersections after projecting to the x-y-plane have been loaded already.*/
bool IntersectionsComputedQ() const
{
    return intersections_computedQ;
}

/*!@brief Return the number of intersections after (symbolically perturbed) projection to the x-y-plane.
 *
 * Calls `RequireIntersections()`.
 * */
Int IntersectionCount()
{
    (void)RequireIntersections();
    return intersection_count;
}

/*!@brief Return the number of intersections in the 3-space.
 *
 * Calls `RequireIntersections()`.
 */
Int IntersectionCount3D()
{
    (void)RequireIntersections();
    return intersection_count_3D;
}

/*!@brief Return a vector `p` of size `EdgeCount() + 1` so that `p[e+1] - p[e]` is the number of intersections on edge `e`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int,Int>> EdgePointers()
{
    (void)RequireIntersections();
    return edge_ptr;
}

/*!@brief Return a vector `a` intersection information. The labels of the intersections on edge `e` are `[ a[p[e]],...,a[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int,Int>> EdgeIntersections()
{
    (void)RequireIntersections();
    return edge_intersections;
}

/*!@brief Return a vector `t` with intersection time information. The times of intersection on edge `e` are `[ t[p[e]],...,t[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * Instead of giving the precise times (which are rational functions in the symbolic perturbation paramater `eps` with wide integer coefficient s), these are `double` approximations (for perturbation `eps = 0`).
 *
 * Calls `RequireIntersections()`.
 */
Tensor1<double,Int> EdgeIntersectionTimesAsDouble()
{
    (void)RequireIntersections();
    
    Tensor1<double,Int> result ( edge_times.Size() );
    
    for( Int i = 0; i < edge_times.Size(); ++i )
    {
        result[i] = ToDouble(edge_times[i]);
    }
        
    return result;
}

/*!@brief Return a vector `s` with intersection state information. The states of intersection on edge `e` are `[ s[p[e]],...,s[p[e+1]-1]`, where `p = EdgePointers()`.
 *
 * The state is given by `(h << 1) | b`, where `h` is the handedness crossing and where `b` is `true` if this edge is the upper strand for the crossing `a[p[e]]` (and false otherwise), where `a = EdgeIntersections()`.
 *
 * Calls `RequireIntersections()`.
 */
cref<Tensor1<Int8,Int>> EdgeStates()
{
    (void)RequireIntersections();
    return edge_state;
}

private:

/*!@brief (Re)compute the intersections.*/
template<bool verboseQ = true> // whether to print errors and warnings
void ComputeIntersections()
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ComputeIntersections"); };
    
    TOOLS_PTIMER(timer,tag());
    
    intersections_computedQ  = false;
    intersections.clear();
    intersection_count       = 0;
    intersection_count_3D    = 0;
    
    // Here we do something strange:
    // We hand over edge_coords, a Tensor3 of size edge_count x 2 x 3
    // to a T which is a Tree2_T.
    // The latter expects a Tensor3 of size edge_count x 2 x 2, but it accesses the
    // enties only via operator(i,j,k), so this is safe!
    
    RequireBoundingBoxes();
    
    if( !bounding_boxes_computedQ )
    {
        wprint(tag() + ": Boundung boxes not computed, yet. Aborting.");
    }
    
    {
        Size_T size_estimate = Size_T(2) * ToSize_T(EdgeCount());
        if( intersections.capacity() < size_estimate )
        {
            intersections.reserve( size_estimate );
        }
    }
    
    edge_ptr.Fill(0);
    
    FindIntersectingEdges_DFS();
    
    if( !std::in_range<Int>( Size_T(8) * intersections.size()) )
    {
        eprint(tag() + ": More intersections found (intersections.size() = " + ToString(intersections.size()) + ") than can be handled by integer type Int = " + TypeName<Int> + " = " + std::string(PrettyTypeName<Int>()) + ". Please try again with a wider integer type." );
        
        intersections_computedQ = true;
        return;
    }
    
    // Earlier versions of this function checked `intersection_count_3D > 0` and stopped here or threw warnings or errors. We just let it flow. Computing the other planar intersections might still be worthwhile. Moreover, `RequireIntersections` will finally report on this issue. Makes the control flow simpler and better maintainable.
        
    edge_ptr.Accumulate();
    
    intersection_count = int_cast<Int>(intersections.size());
    
//    Int64 sort_edge_count = 0;
//    Int64 sort_intersection_count = 0;
//    Int64 not_sort_intersection_count = 0;
    {
        TOOLS_PTIMER(sort_timer,MethodName("ComputeIntersections") + ": coarse sorting.");
        
        // We are going to use edge_ptr for the assembly; because we are going to modify it, we need a copy.
        edge_ctr.template RequireSize<false>( edge_ptr.Size() );
        edge_ctr.Read( edge_ptr.data() );
        
        if( edge_intersections.Size() != edge_ptr.Last() )
        {
            edge_intersections = Tensor1<Int   ,Int>( edge_ptr.Last() );
            edge_times         = Tensor1<Time_T,Int>( edge_ptr.Last() );
            edge_state         = Tensor1<Int8  ,Int>( edge_ptr.Last() );
        }

        for( Int k = intersection_count; k --> Int(0);  )
        {
            Intersection_T & inter = intersections[static_cast<Size_T>(k)];
            
            // We have to write BEFORE the positions specified by edge_ctr (and decrease it for the next write;
            
            const Int pos_0 = --edge_ctr[inter.edges[0]+Int(1)];
            const Int pos_1 = --edge_ctr[inter.edges[1]+Int(1)];
            
            edge_intersections[pos_0] = k;
            edge_times        [pos_0] = inter.times[0];
            edge_state        [pos_0] = static_cast<Int8>(inter.handedness << 1) | 1;
            
            edge_intersections[pos_1] = k;
            edge_times        [pos_1] = inter.times[1];
            edge_state        [pos_1] = static_cast<Int8>(inter.handedness << 1) | 0;
        }
    }
    {
        TOOLS_PTIMER(sort_timer,MethodName("ComputeIntersections") + ": fine sorting.");

        // Sort intersections edgewise w.r.t. edge_times.
        ThreeArraySort<Time_T,Int,Int8,Int> sort ( intersection_count );

        for( Int i = 0; i < edge_count; ++i )
        {
            // This is the range of data in edge_intersections/edge_times that belongs to edge i.
            const Int k_begin = edge_ptr[i  ];
            const Int k_end   = edge_ptr[i+1];

            // We need to sort only if there are at least two intersections on that edge.
            if( k_begin + Int(1) < k_end )
            {
//                // DEBUGGING
//                ++sort_edge_count;
//                sort_intersection_count += (k_end - k_begin);
                sort(
                    &edge_times[k_begin],
                    &edge_intersections[k_begin],
                    &edge_state[k_begin],
                    k_end - k_begin
                );
            }
//            else
//            {
//                not_sort_intersection_count += (k_end - k_begin);
//            }
        }
        
//        // DEBUGGING
//        TOOLS_DUMP(sort_edge_count);
//        TOOLS_DUMP(intersection_count);
//        TOOLS_DUMP(sort_intersection_count);
//        TOOLS_DUMP(not_sort_intersection_count);
//        TOOLS_DUMP(Frac<double>(sort_intersection_count,sort_edge_count));
    }
    
    // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.
    intersections_computedQ = true;
}

void FindIntersectingEdges_DFS()
{
    TOOLS_PTIMER(timer,MethodName("FindIntersectingEdges_DFS"));
    
    constexpr Int stack_max_size = Int(4) * max_depth + Int(1);
    constexpr Int stack_limit    = Int(4) * max_depth - Int(4);
    
    Int stack [stack_max_size][2];
    Int stack_ptr = 0;
    stack[stack_ptr][0] = 0;  // Dummy node.
    stack[stack_ptr][1] = 0;  // Dummy node.
    
    // Helper routine to manage the pair_stack.
    auto push = [&stack,&stack_ptr]( const Int i, const Int j )
    {
        ++stack_ptr;
        stack[stack_ptr][0] = i;
        stack[stack_ptr][1] = j;
    };
    
    // Helper routine to manage the pair_stack.
    auto conditional_push = [this,push]( const Int i, const Int j )
    {
        if( this->BoxesIntersectQ(i,j) ) { push(i,j); }
    };
    
    // Helper routine to manage the pair_stack.
    auto pop = [&stack,&stack_ptr]()
    {
        const std::pair result ( stack[stack_ptr][0], stack[stack_ptr][1] );
        stack_ptr--;
        return result;
    };
    
    auto continueQ = [&stack_ptr,this]()
    {
        const bool overflowQ = (stack_ptr >= stack_limit);
        
        if( (Int(0) < stack_ptr) && (!overflowQ) ) [[likely]]
        {
            return true;
        }
        else
        {
            if ( overflowQ ) [[unlikely]]
            {
                eprint(this->MethodName("FindIntersectingEdges_DFS")+": Stack overflow.");
            }
            return false;
        }
    };
    
    push(Int(0),Int(0));
    
    while( continueQ() )
    {
        // Pop from stack.
        
        auto [i,j] = pop();
        
        const bool i_internalQ = T.InternalNodeQ(i);
        const bool j_internalQ = T.InternalNodeQ(j);
        
        // Warning: This assumes that both children in a cluster tree are either defined or empty.
        
        if( i_internalQ || j_internalQ ) // [[likely]]
        {
            auto [L_i,R_i] = Tree2_T::Children(i);
            auto [L_j,R_j] = Tree2_T::Children(j);
            
            // T is a balanced binary tree.
            
            if( i_internalQ == j_internalQ )
            {
                if( i == j )
                {
                    //  Creating 3 blockcluster children, since there is one block that is just the mirror of another one.
                    
                    conditional_push(L_i,R_j);
                    push(R_i,R_j);
                    push(L_i,L_j);
                }
                else
                {
                    // tie breaker: split both clusters
                    conditional_push(R_i,R_j);
                    conditional_push(L_i,R_j);
                    conditional_push(R_i,L_j);
                    conditional_push(L_i,L_j);
                }
            }
            else
            {
                // split only larger cluster
                if( i_internalQ ) // !j_internalQ follows from this.
                {
                    //split cluster i
                    conditional_push(R_i,j);
                    conditional_push(L_i,j);
                }
                else
                {
                    //split cluster j
                    conditional_push(i,R_j);
                    conditional_push(i,L_j);
                }
            }
        }
        else
        {
            ComputeEdgeEdgeIntersection( T.NodeBegin(i), T.NodeBegin(j) );
        }
    }
    
} // FindIntersectingEdges_DFS

void ComputeEdgeEdgeIntersection( const Int k, const Int l )
{
    if( EdgesNeedCheckQ(k,l) )
    {
        this->template ComputeEdgeEdgeIntersection_impl<false>(k,l);
    }
}

template<bool verboseQ>
void ComputeEdgeEdgeIntersection_impl( const Int k, const Int l )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ComputeEdgeEdgeIntersection"); };
    
    if constexpr ( verboseQ )
    {
        logprint(tag() + "( " + ToString(k) +" ," +  ToString(k) + " ).");
    }

    // At this point we assume that `k != l` and that they are also not direct neighbors.
    // Also, we may assume that neither edge is degenerate in 3-space.

    using Flag_T = Prosector_T::Flag_T;
    
    S.LoadLineSements(
        k, EdgeData(k,Int(0)), EdgeData(k,Int(1)),
        l, EdgeData(l,Int(0)), EdgeData(l,Int(1))
    );

    Flag_T flag = S.IntersectionType();

    if constexpr ( verboseQ ) { TOOLS_LOGDUMP(flag); }

    switch (flag)
    {
        case Flag_T::Empty:         return;
        case Flag_T::Intersection:  break;
        case Flag_T::Error:
        {
            eprint(tag() +": Edges " + ToString(k) + " and " + ToString(l) + " intersect in 3D.");
            // Prevent overflow by min - function.
            intersection_count_3D = std::min(
                 intersection_count_3D,
                 std::numeric_limits<Int>::max() - Int(1)
             ) + Int(1);
            return;
        }
        default:
        {
            eprint(tag() + ": This should never happen.");
            return;
        }
    }

    // If we arrive here, then flag == Flag_T::Intersection.

    ++edge_ptr[k + Int(1)];
    ++edge_ptr[l + Int(1)];

    intersections.push_back( S.ComputeIntersection() );
}
