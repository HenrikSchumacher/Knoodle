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

//Vector_T VertexCoordinates( const Int vertex )
//{
//    Int node = VertexNode(vertex);
//    
//    PullTransforms( Root(), node );
//    
//    return NodeCenter(node);
//}

void WriteVertexCoordinates( mptr<Real> X )
{
    PushAllTransforms();
    
    // Copy leave nodes.
    for( Int vertex = 0; vertex < VertexCount(); ++vertex )
    {
        const Int node = PrimitiveNode(vertex);

        copy_buffer<AmbDim>( NodeCenterPtr(node), &X[AmbDim*vertex] );
    }
}

Tensor2<Real,Int> VertexCoordinates()
{
    PushAllTransforms();

    const Int n = VertexCount();
    
    Tensor2<Real,Int> X ( n, AmbDim );
    
    WriteVertexCoordinates( X.data() );
    
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


Size_T MatrixMatrixCounter() const
{
    return mm_counter;
}

Size_T MatrixVectorCounter() const
{
    return mv_counter;
}

Size_T TransformLoadCounter() const
{
    return load_counter;
}
