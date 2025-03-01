Int VertexCount() const
{
    return LeafNodeCount();
}

Real HardSphereDiameter() const
{
    return hard_sphere_diam;
}

Real PrescibedEdgeLength() const
{
    return prescribed_edge_length;
}

Int VertexNode( const Int vertex ) const
{
    return PrimitiveNode( vertex );
}

cref<NodeContainer_T> NodeTransforms() const
{
    return N_transform;
}

mref<NodeContainer_T> NodeTransforms()
{
    return N_transform;
}

cref<NodeContainer_T> NodeBalls() const
{
    return N_ball;
}

mref<NodeContainer_T> NodeBalls()
{
    return N_ball;
}


cref<Tensor1<NodeState_T,Int>> NodeStates() const
{
    return N_state;
}

mref<Tensor1<NodeState_T,Int>> NodeStates()
{
    return N_state;
}

cref<NodeState_T> NodeState( const Int node ) const
{
    return N_state[node];
}

mref<NodeState_T> NodeState( const Int node )
{
    return N_state[node];
}

cptr<Real> NodeTransformPtr( const Int node ) const
{
    return &N_transform.data()[TransformDim * node];
}

mptr<Real> NodeTransformPtr( const Int node )
{
    return &N_transform.data()[TransformDim * node];
}

cptr<Real> NodeCenterPtr( const Int node ) const
{
    return &N_ball.data()[BallDim * node];
}

mptr<Real> NodeCenterPtr( const Int node )
{
    return &N_ball.data()[BallDim * node];
}

cptr<Real> NodeBallPtr( const Int node ) const
{
    return &N_ball.data()[(AmbDim+1) * node];
}

mptr<Real> NodeBallPtr( const Int node )
{
    return &N_ball.data()[(AmbDim+1) * node];
}

Real NodeRadius( const Int node ) const
{
    return N_ball.data()[(AmbDim + 1) * node + AmbDim];
}

mref<Real> NodeRadius( const Int node )
{
    return N_ball.data()[(AmbDim + 1) * node + AmbDim];
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

// This version is measurably slower than the previous one.
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
    return i ? q : p;
}


Int Witness( const bool i ) const
{
    return i ? witness_1 : witness_0;
}


//Size_T MatrixMatrixCounter() const
//{
//    return call_counters.mm;
//}
//
//Size_T MatrixVectorCounter() const
//{
//    return call_counters.mv;
//}
//
//Size_T TransformLoadCounter() const
//{
//    return call_counters.load_transform;
//}

CallCounters_T CallCounters()
{
    return call_counters;
}
