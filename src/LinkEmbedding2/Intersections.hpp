public:

/*!@brief Return the number of intersections.*/

bool IntersectionsComputedQ() const
{
    return intersections_computedQ;
}

/*!@brief Guarantee that the intersections are computed.*/

template<bool verboseQ = true> // whether to print errors and warnings
[[nodiscard]] int RequireIntersections()
{
    if( intersections_computedQ ) { return 0; }
    
    return ComputeIntersections();
}

/*!@brief (Re)compute the intersections.*/

template<bool verboseQ = true> // whether to print errors and warnings
[[nodiscard]] int ComputeIntersections()
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
        return 0;
    }
    
    if( intersections.capacity() < ToSize_T(2 * EdgeCount()) )
    {
        intersections.reserve( ToSize_T(2 * EdgeCount()) );
    }
    
    edge_ptr.Fill(0);
    
    FindIntersectingEdges_DFS_ManualStack();
    
    if( intersection_count_3D > Int(0) )
    {
        if constexpr ( verboseQ )
        {
            eprint(tag()+": Detected " + ToString(intersection_count_3D) + " cases where line segments intersected in 3D.");
        }
        return 6;
    }
        
    edge_ptr.Accumulate();
    
    intersection_count = static_cast<Int>(intersections.size());
    
    // We are going to use edge_ptr for the assembly; because we are going to modify it, we need a copy.
    edge_ctr.template RequireSize<false>( edge_ptr.Size() );
    edge_ctr.Read( edge_ptr.data() );

    if( edge_intersections.Size() != edge_ptr.Last() )
    {
        edge_intersections = Tensor1<Int   ,Int>( edge_ptr.Last() );
        edge_times         = Tensor1<Time_T,Int>( edge_ptr.Last() );
        edge_state         = Tensor1<Int8  ,Int>( edge_ptr.Last() );
    }
    
    if( intersection_count <= Int(0) ) { return 0; }

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
            sort(
                &edge_times[k_begin],
                &edge_intersections[k_begin],
                &edge_state[k_begin],
                k_end - k_begin
            );
        }
    }
    
    // From now on we can safely cycle around each component and generate vertices, edges, crossings, etc. in their order.

    intersections_computedQ = true;
    
    return 0;
}



private:

void FindIntersectingEdges_DFS_ManualStack()
{
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
                eprint(this->MethodName("FindIntersectingEdges_DFS_ManualStack")+": Stack overflow.");
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
    
} // FindIntersectingEdges_DFS_ManualStack

void ComputeEdgeEdgeIntersection( const Int k, const Int l )
{
    // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
    if( (l != k) && (l != NextEdge(k)) && (k != NextEdge(l)) && !edge_degenerateQ[k] && !edge_degenerateQ[l] )
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
        logprint(tag() + " in verbose mode.");
        TOOLS_LOGDUMP(k);
        TOOLS_LOGDUMP(l);
    }

    // At this point we assume that `k != l` and that they are also not direct neighbors.
    // Also, we may assume that neither edge is degenerate in 3-space.

//    if constexpr ( verboseQ )
//    {
//        TOOLS_LOGDUMP(ToString(x_0));
//        TOOLS_LOGDUMP(ToString(x_1));
//        TOOLS_LOGDUMP(ToString(y_0));
//        TOOLS_LOGDUMP(ToString(y_1));
//    }

    using Flag_T = Prosector_T::Flag_T;
    
    S.LoadSegments(
        k, EdgeData(k,Int(0)), EdgeData(k,Int(1)),
        l, EdgeData(l,Int(0)), EdgeData(l,Int(1))
    );

    Flag_T flag = S.template IntersectionType<verboseQ>();

    if constexpr ( verboseQ ) { TOOLS_LOGDUMP(flag); }

    switch (flag)
    {
        case Flag_T::Empty:         return;
        case Flag_T::Intersection:  break;
        case Flag_T::Error:
        {
            wprint(tag() +": Edges " + ToString(k) + " and " + ToString(l) + " intersect in 3D.");
            // TODO: We need a check for overflow here.
            ++intersection_count_3D;
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

