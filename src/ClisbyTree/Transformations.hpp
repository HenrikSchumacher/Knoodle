//#########################################################################################
//##    Transformations
//#########################################################################################
        
static constexpr void ApplyToVectorPtr( cref<Transform_T> f, mptr<Real> x_ptr )
{
    Vector_T x (x_ptr);
    
    Vector_T y = f(x);
    
    y.Write( x_ptr );
}

static constexpr void ApplyToTransformPtr( cref<Transform_T> f, mptr<Real> g_ptr )
{
    Transform_T g ( g_ptr );
    
    Transform_T h = f(g);
    
    h.Write( g_ptr );
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
    
    const Transform_T f = NodeTransform(node);
    
    // TODO: These two transformations could be fused.
    UpdateNode<true,true>( f, L );
    UpdateNode<true,true>( f, R );
    
    ResetTransform(node);
}

void PushAllTransforms()
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
        {}
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
