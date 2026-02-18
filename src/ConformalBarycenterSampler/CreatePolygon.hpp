private:

// Common implementation of
//
// - ComputeConformalClosure,
// - CreateRandomClosedPolygon,
//
// - ComputeConformalCentralization,
// - CreateRandomCentralizedPointCloud,
// - CreateRandomCentralizedPointCloud_Detailed.
//
// This function is meant to be called only from there.

template<
    bool x_in_Q, bool x_out_Q,
    bool p_in_Q, bool p_out_Q, bool w_Q, bool y_Q, bool q_Q,
    bool edge_space_Q, bool quot_space_Q
>
void CreatePolygon(
    cptr<Real> x_in, mptr<Real> x_out,
    cptr<Real> p_in, mptr<Real> p_out,
    mptr<Real> w,
    mptr<Real> y,
    mptr<Real> q,
    mref<Real> K_edge_space,
    mref<Real> K_quot_space,
    const bool wrap_aroundQ = false,
    const bool centralizeQ  = true
)
{
    if constexpr ( p_in_Q || x_in_Q )
    {
        if constexpr ( p_in_Q )
        {
            ReadInitialVertexPositions(p_in);
        }
        else
        {
            ReadInitialEdgeVectors(x_in);
        }
    }
    else
    {
        RandomizeInitialEdgeVectors();
    }
    
    if constexpr ( w_Q || y_Q || q_Q || edge_space_Q || quot_space_Q )
    {
        computeConformalClosure<q_Q,quot_space_Q>( centralizeQ );
    }
    
    
    if constexpr ( p_out_Q )
    {
        WriteInitialVertexCoordiantes(p_out);
    }
    
    if constexpr ( x_out_Q > 0 )
    {
        WriteInitialEdgeVectors(x_out);
    }
    
    if constexpr ( w_Q )
    {
        WriteShiftVector(w);
    }
    
    if constexpr ( y_Q )
    {
        WriteEdgeVectors(y);
    }
    
    if constexpr ( q_Q )
    {
        WriteVertexPositions( q, wrap_aroundQ );
    }
    
    if constexpr ( edge_space_Q )
    {
        K_edge_space = EdgeSpaceSamplingWeight();
    }
    
    if constexpr ( quot_space_Q )
    {
        K_quot_space = EdgeQuotientSpaceSamplingWeight();
    }
}

private:

template<bool vertex_pos_Q, bool quot_space_Q>
void computeConformalClosure( const bool centralizedQ )
{
    ComputeInitialShiftVector();

    Optimize();
    
    if constexpr ( vertex_pos_Q )
    {
        ComputeVertexPositions( centralizedQ );
    }
    
    ComputeEdgeSpaceSamplingWeight();
    
    if constexpr ( quot_space_Q )
    {
        ComputeEdgeQuotientSpaceSamplingWeight();
    }
}
