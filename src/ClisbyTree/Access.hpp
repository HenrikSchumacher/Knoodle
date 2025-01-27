//#########################################################################################
//##    Access methods
//#########################################################################################

Int VertexCount() const
{
    return LeafNodeCount();
}

Real Radius() const
{
    return r;
}

Real SquaredRadius() const
{
    return r2;
}

Int VertexNode( const Int vertex ) const
{
    return PrimitiveNode( vertex );
}

cref<NodeContainer_T> NodeData() const
{
    return N_data;
}

mref<NodeContainer_T> NodeData()
{
    return N_data;
}

cptr<Real> NodeData( const Int node ) const
{
    return &N_data.data()[NodeDim * node];
}

mptr<Real> NodeData( const Int node )
{
    return &N_data.data()[NodeDim * node];
}

cref<Tensor1<NodeState_T,Int>> NodeStates() const
{
    return N_state;
}

mref<Tensor1<NodeState_T,Int>> NodeStates()
{
    return N_state;
}

NodeState_T NodeState( const Int node ) const
{
    return N_state[node];
}

cptr<Real> NodeTransformPtr( const Int node ) const
{
    return &N_data.data()[NodeDim * node + AmbDim + 1];
}

mptr<Real> NodeTransformPtr( const Int node )
{
    return &N_data.data()[NodeDim * node + AmbDim + 1];
}

cptr<Real> NodeCenterPtr( const Int node ) const
{
    return &N_data.data()[NodeDim * node];
}

mptr<Real> NodeCenterPtr( const Int node )
{
    return &N_data.data()[NodeDim * node];
}

Real NodeRadius( const Int node ) const
{
    return N_data.data()[NodeDim * node + AmbDim];
}

mref<Real> NodeRadius( const Int node )
{
    return N_data.data()[NodeDim * node + AmbDim];
}

Transform_T NodeTransform( const Int node ) const
{
    return Transform_T( NodeTransformPtr(node) );
}

Vector_T NodeCenter( const Int node ) const
{
    return Vector_T( NodeCenterPtr(node) );
}

void ReadVertexCoordinates( cptr<Real> V )
{
    ptic(ClassName() + "::ReadVertexCoordinates");
        
    this->DepthFirstSearch(
        []( const Int node )                    // interior node previsit
        {},
        [this]( const Int node )                // interior node postvisit
        {
            ComputeBall(node);
            ResetTransform(node);
        },
        [this,V]( const Int node )              // leaf node previsit
        {
            const Int vertex = NodeBegin(node);

            InitializeNodeFromVertex( node, &V[AmbDim * vertex] );
        },
        []( const Int node )                    // leaf node postvisit
        {}
    );
    
//    // Compute leave nodes.
//    for( Int vertex = 0; vertex < LeafNodeCount(); ++vertex )
//    {
//        const Int node = PrimitiveNode(vertex);
//
//        InitializeNodeFromVertex( node, &V[AmbDim * vertex] );
//    }
    
//    // Compute interior nodes.
//    for( Int node = InteriorNodeCount(); node --> 0;  )
//    {
//        ComputeBall(node);
//        ResetTransform(node);
//    }
    
    ptoc(ClassName() + "::ReadVertexCoordinates");
}


Vector_T VertexCoordinates( const Int vertex ) const
{
    Int node = VertexNode(vertex);
    
    Vector_T x = NodeCenter(node);
    
    while( node != Root() )
    {
        node = Parent(node);
        
        if( N_state[node] == NodeState_T::NonId )
        {
            x = NodeTransform(node)(x);
        }
    }
    
    return x;
}

Tensor2<Real,Int> VertexCoordinates()
{
    PushAllTransforms();

    const Int n = VertexCount();
    
    Tensor2<Real,Int> X ( n, AmbDim );
    
    // Copy leave nodes.
    for( Int vertex = 0; vertex < LeafNodeCount(); ++vertex )
    {
        const Int node = PrimitiveNode(vertex);

        copy_buffer<AmbDim>( NodeCenterPtr(node), &X.data()[AmbDim*vertex] );
    }
    
    return X;
}


Int Pivot( const bool i ) const 
{
    if( i )
    {
        return q;
    }
    else
    {
        return p;
    }
}


Int Witness( const bool i ) const
{
    if( i )
    {
        return witness_1;
    }
    else
    {
        return witness_0;
    }
}
