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

void ComputePivotTransform()
{
    // Rotation axis.
    Vector_T u = (X_q - X_p);
    u.Normalize();
    
    const Real cos = std::cos(theta);
    const Real sin = std::sin(theta);
    const Real d = Scalar::One<Real> - cos;
    
    Matrix_T A;
    
    A[0][0] = u[0] * u[0] * d + cos;
    A[0][1] = u[0] * u[1] * d - sin * u[2];
    A[0][2] = u[0] * u[2] * d + sin * u[1];
    
    A[1][0] = u[1] * u[0] * d + sin * u[2];
    A[1][1] = u[1] * u[1] * d + cos;
    A[1][2] = u[1] * u[2] * d - sin * u[0];
    
    A[2][0] = u[2] * u[0] * d - sin * u[1];
    A[2][1] = u[2] * u[1] * d + sin * u[0];
    A[2][2] = u[2] * u[2] * d + cos;
    
    // Compute shift b = X_p - A.X_p.
    Vector_T b = X_p - Dot(A,X_p);
    
    transform = Transform_T( std::move(A), std::move(b) );
}

int Update( const Int pivot_p, const Int pivot_q, const Real angle_theta )
{
    theta = angle_theta;
    p = Min(pivot_p,pivot_q);
    q = Max(pivot_p,pivot_q);
    
    const Int mid_size = q - p - 1;
    const Int rem_size = VertexCount() - mid_size - 2;

    if( (mid_size <= 0) || (rem_size <= 0) )
    {
        return 1;
    }
    
    mid_changedQ = (mid_size <= rem_size);
    
    // TODO: There is maybe a more efficient way to compute the pivot vectors.
    X_p = VertexCoordinates(p);
    X_q = VertexCoordinates(q);
    ComputePivotTransform();
    
    Int  stack [max_depth];
    Int  stack_ptr = -1;
    
    switch( NodeNeedsUpdateQ( 0 ) )
    {
        case UpdateFlag_T::DoNothing:
        {
            // Cannot happen?
            return 1;
        }
        case UpdateFlag_T::Update:
        {
            UpdateNode<true,true>( transform, 0 );
            return 0;
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
    
    return 0;
}
