//#########################################################################################
//##    Transformations
//#########################################################################################


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

void ApplyToVectorPtr( cref<Transform_T> f, mptr<Real> x_ptr )
{
    f.TransformVector(x_ptr);
    
    if constexpr ( perf_countersQ )
    {
        ++mv_counter;
    }
}

void ApplyToTransformPtr( cref<Transform_T> f, mptr<Real> g_ptr )
{
    f.TransformTransform(g_ptr);
    
    if constexpr ( perf_countersQ )
    {
        ++load_counter;
        ++mv_counter;
        ++mm_counter;
    }
}

template<bool update_centerQ, bool update_transformQ>
void UpdateNode( cref<Transform_T> f, const Int node )
{
    if constexpr ( update_centerQ )
    {
        ApplyToVectorPtr( f, NodeCenterPtr(node) );
    }
    
    if constexpr ( update_transformQ )
    {
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
            ApplyToTransformPtr( f, NodeTransformPtr(node) );
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

void PushAllTransforms( const Int start_node = -1 )
{
    this->DepthFirstSearch(
        [this]( const Int node )                    // interior node previsit
        {
            const auto [L,R] = Children( node );
            PushTransform( node, L, R );
        },
        []( const Int node )                        // interior node postvisit
        {},
        []( const Int node )                        // leaf node previsit
        {},
        []( const Int node )                        // leaf node postvisit
        {},
        start_node
    );
    
//    // Exploiting here that we have breadth-first ordering.
//    for( Int node = 0; node < InteriorNodeCount(); ++node )
//    {
//        const Int L = LeftChild (node);
//        const Int R = RightChild(node);
//        
//        PushTransform( node, L, R );
//    }
}

// TODO: Test this. And try whether it improves the computation of the pivots.
void PullTransforms( const Int from, const Int to )
{
    Int stack [max_depth];
    
    Int node = to;
    
    Int stack_ptr = -1;
    
    while( (node != from) && (stack_ptr < max_depth) )
    {
        node = Parent(node);
        stack[++stack_ptr] = node;
    }
    
    if( stack_ptr >= max_depth )
    {
        eprint(ClassName() + "::PullAllTransforms: Stack overflow.");
        return;
    }
    
    while( Int(0) <= stack_ptr )
    {
        node = stack[stack_ptr--];
        const auto [L,R] = Children( node );
        PushTransform( node, L, R );
    }

}
