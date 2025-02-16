//####################################################################################
//##    Transformations
//####################################################################################


void ComputePivotTransform()
{
    // Rotation axis.
    Vector_T u = (X_q - X_p);

    u.Normalize();
    
    const Real cos = std::cos(theta);
    const Real sin = std::sin(theta);
    const Real d = Scalar::One<Real> - cos;

    Matrix_T & A = transform.Matrix();
    
    
    if constexpr ( use_clang_matrixQ )
    {
        A.Set( 0, 0, u[0] * u[0] * d + cos        );
        A.Set( 0, 1, u[0] * u[1] * d - sin * u[2] );
        A.Set( 0, 2, u[0] * u[2] * d + sin * u[1] );
        
        A.Set( 1, 0, u[1] * u[0] * d + sin * u[2] );
        A.Set( 1, 1, u[1] * u[1] * d + cos        );
        A.Set( 1, 2, u[1] * u[2] * d - sin * u[0] );
        
        A.Set( 2, 0, u[2] * u[0] * d - sin * u[1] );
        A.Set( 2, 1, u[2] * u[1] * d + sin * u[0] );
        A.Set( 2, 2, u[2] * u[2] * d + cos        );
    }
    else
    {
        A(0,0) = u[0] * u[0] * d + cos;
        A(0,1) = u[0] * u[1] * d - sin * u[2];
        A(0,2) = u[0] * u[2] * d + sin * u[1];
        
        A(1,0) = u[1] * u[0] * d + sin * u[2];
        A(1,1) = u[1] * u[1] * d + cos;
        A(1,2) = u[1] * u[2] * d - sin * u[0];
        
        A(2,0) = u[2] * u[0] * d - sin * u[1];
        A(2,1) = u[2] * u[1] * d + sin * u[0];
        A(2,2) = u[2] * u[2] * d + cos;
    }

    
    // Compute shift b = X_p - A.X_p.
    transform.Vector() = X_p - A * X_p;
    
//    transform = Transform_T( std::move(A), std::move(b) );
}

Transform_T NodeTransform( const Int node ) const
{
    if constexpr ( perf_countersQ )
    {
        ++load_counter;
    }
    
    return Transform_T( NodeTransformPtr(node) );
}

Vector_T NodeCenter( const Int node ) const
{
    return Vector_T( NodeCenterPtr(node) );
}

template<bool update_centerQ, bool update_transformQ>
void UpdateNode( cref<Transform_T> f, const Int node )
{
    if constexpr ( update_centerQ )
    {
        f.TransformVector(NodeCenterPtr(node));
        
        if constexpr ( perf_countersQ )
        {
            ++mv_counter;
        }
    }
    
    if constexpr ( update_transformQ )
    {
        // Transformation of a leaf node never needs a change.
        if( LeafNodeQ( node ) )
        {
            return;
        }
        
        if (N_state[node] == NodeState_T::Id )
        {
            f.Write( NodeTransformPtr(node) );
        }
        else
        {
            f.TransformTransform(NodeTransformPtr(node));
            
            if constexpr ( perf_countersQ )
            {
                ++load_counter;
                ++mv_counter;
                ++mm_counter;
            }
        }
        
        N_state[node] = NodeState_T::NonId;
    }
}

void PushTransform( const Int node, const Int L, const Int R )
{
    if( N_state[node] == NodeState_T::Id )
    {
        return;
    }
    
    Transform_T f = NodeTransform(node);
    
    UpdateNode<true,true>( f, L );
    UpdateNode<true,true>( f, R );
    
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
        PullTransforms_Recurssive(from,to);
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
