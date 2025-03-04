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

cref<NodeTransformContainer_T> NodeTransforms() const
{
    return N_transform;
}

mref<NodeTransformContainer_T> NodeTransforms()
{
    return N_transform;
}

cref<NodeBallContainer_T> NodeBalls() const
{
    return N_ball;
}

mref<NodeBallContainer_T> NodeBalls()
{
    return N_ball;
}

cref<NodeFlagContainer_T> NodeFlags() const
{
    return N_state;
}

mref<NodeFlagContainer_T> NodeFlags()
{
    return N_state;
}

cref<NodeFlag_T> NodeFlag( const Int node ) const
{
    return N_state[node];
}

mref<NodeFlag_T> NodeFlag( const Int node )
{
    return N_state[node];
}

bool NodeActiveQ( const Int node ) const
{
    return N_state[node] == NodeFlag_T::NonId;
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
    return witness[i];
}

cref<WitnessVector_T> Witness() const
{
    return witness;
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
