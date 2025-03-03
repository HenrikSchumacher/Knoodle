void ComputePivotTransform()
{
    // Rotation axis.
    Vector_T u = (X_q - X_p);

    u.Normalize();
    
    if constexpr ( use_quaternionsQ )
    {
        const Real theta_half = Scalar::Half<Real> * theta;
        const Real cos = std::cos(theta_half);
        const Real sin = std::sin(theta_half);
        
        Real Q [7] {cos, sin * u[0], sin * u[1], sin * u[2], 0, 0, 0 };
            
        transform.Read( &Q[0], NodeFlag_T::NonId );
        
        Vector_T b = X_p - transform.Matrix() * X_p;
        
        transform.ForceReadVector(b);
    }
    else
    {
        const Real cos = std::cos(theta);
        const Real sin = std::sin(theta);
        const Real d = Scalar::One<Real> - cos;
        
        
        const Real a [3][3] = {
            {
                u[0] * u[0] * d + cos       ,
                u[0] * u[1] * d - sin * u[2],
                u[0] * u[2] * d + sin * u[1]
            },
            {
                u[1] * u[0] * d + sin * u[2],
                u[1] * u[1] * d + cos       ,
                u[1] * u[2] * d - sin * u[0]
            },
            {
                u[2] * u[0] * d - sin * u[1],
                u[2] * u[1] * d + sin * u[0],
                u[2] * u[2] * d + cos
            }
        };
        
        Matrix_T A;
        
        A.Read( &a[0][0] );
        
        Vector_T b = X_p - A * X_p;
        
        transform.Read( A, b, NodeFlag_T::NonId );
    }
    
//    // Check correctness.
//    
//    constexpr Real TOL = 0.000000001;
//    
//    const Real error_p = (transform(X_p) - X_p).Norm();
//    const Real error_q = (transform(X_q) - X_q).Norm();
//    
//    if( error_p > TOL )
//    {
//        wprint( ClassName() + "::ComputePivotTransform: First pivot vertex is modified by pivot transform; error = " + ToString(error_p) + "." );
//    }
//    
//    if( error_q > TOL )
//    {
//        wprint( ClassName() + "::ComputePivotTransform: Second pivot vertex is modified by pivot transform; error = " + ToString(error_q) + "." );
//    }

}


Vector_T NodeCenterAbsoluteCoordinates( const Int node_ ) const
{
    Int node = node_;
    
    Vector_T x = NodeCenter(node);
    
    const Int root = Root();
    
    while( node != root )
    {
        node = Parent(node);
        
        if constexpr ( use_quaternionsQ )
        {
            // If we use quaternions, then we need to construct only the 3x3 matrix, not the 4x4 matrix. That saves a little time.
            
            x = Transform_T::Transform( NodeTransformPtr(node), NodeFlag(node), x );
        }
        else
        {
            x = NodeTransform(node)(x);
        }
    }
    
    return x;
}

Vector_T VertexCoordinates( const Int vertex ) const
{
    return NodeCenterAbsoluteCoordinates( VertexNode(vertex) );
}

// This version is measurably slower than the previous one.
//Vector_T VertexCoordinates( const Int vertex )
//{
//    Int node = VertexNode(vertex);
//
//    PullTransforms( Root(), node );
//
//    return NodeCenter(node);
//}

/*!
 * @brief Loads the transformation stored in node `node` into a `Transform_T` object.
 *
 */

Transform_T NodeTransform( const Int node ) const
{
    if constexpr ( countersQ )
    {
        call_counters.load_transform += (NodeFlag(node) == NodeFlag_T::NonId);
    }
    
    return Transform_T( NodeTransformPtr(node), NodeFlag(node) );
}

Vector_T NodeCenter( const Int node ) const
{
    return Vector_T( NodeCenterPtr(node) );
}

void UpdateNode( cref<Transform_T> f, const Int node )
{
    if constexpr ( countersQ )
    {
        call_counters.mv += f.TransformVector( NodeCenterPtr(node) );
    }
    else
    {
        (void)f.TransformVector( NodeCenterPtr(node) );
    }
    
    // Transformation of a leaf node never needs a change.
    if( LeafNodeQ( node ) )
    {
        return;
    }
    
    if constexpr ( countersQ )
    {
        bool info = f.TransformTransform( NodeTransformPtr(node), NodeFlag(node) );
        
        call_counters.load_transform += info;
        call_counters.mv             += info;
        call_counters.mm             += info;
    }
    else
    {
        (void)f.TransformTransform( NodeTransformPtr(node), NodeFlag(node) );
    }
}

void PushTransform( const Int node, const Int L, const Int R )
{
    if( N_state[node] == NodeFlag_T::Id )
    {
        return;
    }
    
    Transform_T f = NodeTransform(node);
    
    UpdateNode( f, L );
    UpdateNode( f, R );
    
    ResetTransform(node);
}

// Pushes down all the transformations in the subtree starting at `start_node`.
void PushAllTransforms()
{
    PushTransformsSubtree(Root());
}

void PushTransformsSubtree( const Int start_node )
{
    if constexpr ( use_manual_stackQ )
    {
        PushTransformsSubtree_ManualStack(start_node);
    }
    else
    {
        PushTransformsSubtree_Recursive(start_node);
    }
}

void PushTransformsSubtree_ManualStack( const Int start_node )
{
    this->DepthFirstSearch(
        [this]( const Int node )                    // interior node previsit
        {
            const auto [L,R] = Children( node );
            PushTransform( node, L, R );
        },
        []( const Int node ) { (void)node; },       // interior node postvisit
        []( const Int node ) { (void)node; },       // leaf node previsit
        []( const Int node ) { (void)node; },       // leaf node postvisit
        start_node
    );
}

void PushTransformsSubtree_Recursive( const Int node )
{
    if( InteriorNodeQ(node) )
    {
        const auto [L,R] = Children( node );
        PushTransform( node, L, R );
        PushTransformsSubtree_Recursive( L );
        PushTransformsSubtree_Recursive( R );
    }
}

void PullTransforms( const Int from, const Int to )
{
    if constexpr ( use_manual_stackQ )
    {
        PullTransforms_ManualStack(from,to);
    }
    else
    {
        PullTransforms_Recursive(from,to);
    }
}

// TODO: Test this. And try whether it improves the computation of the pivots.
void PullTransforms_Recursive( const Int from, const Int node )
{
    if( node != from )
    {
        PullTransforms_Recursive( from, Parent(node) );
        
        const auto [L,R] = Children( node );
        
        PushTransform( node, L, R );
    }
}

// TODO: Test this. And try whether it improves the computation of the pivots.
void PullTransforms_ManualStack( const Int from, const Int to )
{
    Int stack [max_depth];
    
    Int node = to;
    
    SInt stack_ptr = -1;
    
    while( (node != from) && (stack_ptr < max_depth) )
    {
        node = Parent(node);
        stack[++stack_ptr] = node;
    }
    
    if( stack_ptr >= max_depth )
    {
        eprint(ClassName() + "::PullTransforms_ManualStack: Stack overflow.");
        return;
    }
    
    while( Int(0) <= stack_ptr )
    {
        node = stack[stack_ptr--];
        const auto [L,R] = Children( node );
        PushTransform( node, L, R );
    }
}
