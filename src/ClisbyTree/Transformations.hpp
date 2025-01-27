//#########################################################################################
//##    Transformations
//#########################################################################################
        
void ApplyToVectorPtr( cref<Transform_T> f, mptr<Real> x_ptr )
{
    Vector_T x (x_ptr);
    
    Vector_T y = f(x);
    
    if constexpr ( perf_countersQ )
    {
        ++mv_counter;
    }
    
    y.Write( x_ptr );
}

void ApplyToTransformPtr( cref<Transform_T> f, mptr<Real> g_ptr )
{
    Transform_T g ( g_ptr );
    
    Transform_T h = f(g);

    h.Write( g_ptr );
    
    if constexpr ( perf_countersQ )
    {
        ++load_counter;
        ++mv_counter;
        ++mm_counter;
    }
    
//    Tiny::fixed_dot_mm<AmbDim,AmbDim+1,AmbDim,Overwrite>(
//        &f.Matrix()[0][0], g_ptr, g_ptr
//    );
//    
//    for( Int k = 0; k < AmbDim; ++k )
//    {
//        g_ptr[(AmbDim+1) * k + AmbDim] += f.Vector()[k];
//    }
    
//    using M_T = Tiny::Matrix<AmbDim,AmbDim+1,Real,Int>;
//    
//    M_T B ( g_ptr );
//    
//    M_T C = Dot( f.Matrix(), B );
//    
//    for( Int k = 0; k < AmbDim; ++k )
//    {
//        C[k][AmbDim] += f.Vector()[k];
//    }
//    
//    C.Write( g_ptr );
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

void PullAllTransforms( const Int start_node = -1 )
{
    Int stack [max_depth];

    const Int root = Root();
    Int node = (start_node < 0) ? root : start_node;
    
    Int stack_ptr = -1;
    
    while( (node != root) && (stack_ptr < max_depth) )
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
