//#########################################################################################
//##    Tree update
//#########################################################################################

UpdateFlag_T NodeNeedsUpdateQ( const Int node ) const
{
    return NodeNeedsUpdateQ( node, p, q, mid_changedQ );
}

UpdateFlag_T NodeNeedsUpdateQ(
    const Int node, const Int pivot_p, const Int pivot_q, const bool midQ
) const
{
    // Assuming that 0 <= pivot_p < pivot_q < LeafNodeCount();
    
    auto [begin,end] = NodeRange(node);
    
    const bool a =  midQ;
    const bool b = !midQ;
    
    const Int p_ = pivot_p + a;
    const Int q_ = pivot_q + b;
    
    if( (p_ <= begin) && (end <= q_) )
    {
        return UpdateFlag_T(a);
    }
    else if( (end <= p_) || (q_ <= begin) )
    {
        return UpdateFlag_T(b);
    }
    else
    {
        return UpdateFlag_T::Split;
    }
}

int LoadPivots( const Int pivot_p, const Int pivot_q, const Real angle_theta )
{
    p = Min(pivot_p,pivot_q);
    q = Max(pivot_p,pivot_q);
    theta = angle_theta;
    
    const Int n = VertexCount() ;
    const Int mid_size = q - p - 1;
    const Int rem_size = n - mid_size - 2;
    
    if( (mid_size <= 0) || (rem_size <= 0) ) [[unlikely]]
    {
        return 1;
    }
    
    mid_changedQ = (mid_size <= rem_size);
    
    // TODO: There is maybe a more efficient way to compute the pivot vectors.
    X_p = VertexCoordinates(p);
    X_q = VertexCoordinates(q);
    ComputePivotTransform();
    
    return 0;
}

void Update( const Int pivot_p, const Int pivot_q, const Real angle_theta )
{
    int pivot_flag = LoadPivots( pivot_p, pivot_q, angle_theta );
    
    if( pivot_flag == 0 ) [[likely]]
    {
        if constexpr ( use_manual_stackQ )
        {
            Update();
        }
        else
        {
            UpdateSubtree(Root());
        }
    }
}


void UpdateSubtree( const Int node = Root() )
{
    auto [L,R] = Children(node);
    
    PushTransform(node,L,R);
    
    switch( NodeNeedsUpdateQ(node) )
    {
        case UpdateFlag_T::DoNothing:
        {
            break;
        }
        case UpdateFlag_T::Update:
        {
            UpdateNode<true,true>( transform, node );
            break;
        }
        case UpdateFlag_T::Split:
        {
            UpdateSubtree(L);
            UpdateSubtree(R);
            ComputeBall(node);
        }
    }
}

// Returns 0 if successful.
void Update()
{
    Int  stack [max_depth];
    SInt stack_ptr = -1;
    
    switch( NodeNeedsUpdateQ( 0 ) )
    {
        case UpdateFlag_T::DoNothing:
        {
            // Cannot happen?
            return;
        }
        case UpdateFlag_T::Update:
        {
            UpdateNode<true,true>( transform, 0 );
            return;
        }
        case UpdateFlag_T::Split:
        {
            // Push this node as unvisited; it will be split.
            stack[++stack_ptr] = ( 0 << 1 );
        }
    }
    
    while( (0 <= stack_ptr) && (stack_ptr < max_depth - 2) )
    {
        const Int  code     = stack[stack_ptr];
        const Int  node     = (code >> 1);
        const bool visitedQ = (code & 1);
        
        // Only nodes with flag == 2 land on the stack.
        // Thus, this node must be a splitting node.
        
        if( !visitedQ )
        {
            // Remember that we have been here.
            stack[stack_ptr] |= 1;
            
            // If node is on the stack, then it contains changed and unchanged vertices.
            // So, in particular, it cannot contain any leaf nodes.
            
            auto [L,R] = Children(node);
            
            PushTransform( node, L, R );
            
            // We never update both nodes; otherwise, this would not be a split node.
            
            switch( NodeNeedsUpdateQ( R ) )
            {
                case UpdateFlag_T::DoNothing:
                {
                    break;
                }
                case UpdateFlag_T::Update:
                {
                    UpdateNode<true,true>( transform, R );
                    break;
                }
                case UpdateFlag_T::Split:
                {
                    // Push this node as unvisited; it will be split.
                    stack[++stack_ptr] = (R << 1);
                    break;
                }
            }
            
            switch( NodeNeedsUpdateQ( L ) )
            {
                case UpdateFlag_T::DoNothing:
                {
                    break;
                }
                case UpdateFlag_T::Update:
                {
                    UpdateNode<true,true>( transform, L );
                    break;
                }
                case UpdateFlag_T::Split:
                {
                    // Push this node as unvisited; it will be split.
                    stack[++stack_ptr] = (L << 1);
                    break;
                }
            }
        }
        else
        {
            ComputeBall( node );
            
            // Pop this node.
            --stack_ptr;
        }
    }
}
