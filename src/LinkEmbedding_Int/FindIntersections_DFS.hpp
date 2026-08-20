private:

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
    if( (l != k) && (l != NextEdge(k)) && (k != NextEdge(l)) )
    {
        // Only check for intersection of edge k and l if they are not equal and not direct neighbors.
        // Degenerate edges will be handled correctly by the `Prosector` class.
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

    // We abstracted the call `EdgeData` to make this work also for less explicit data layouts such as it would be in a potential KnotEmbedding_Int class.
    
    Flag_T flag = S.ComputeIntersection(
        k, EdgeData(k,Int(0)), EdgeData(k,Int(1)),
        l, EdgeData(l,Int(0)), EdgeData(l,Int(1))
    );

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
                 std::numeric_limits<Size_T>::max() - Size_T(1)
             ) + Size_T(1);
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

    intersections.push_back( S.GetIntersection() );
}
