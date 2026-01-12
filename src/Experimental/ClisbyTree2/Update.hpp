#include "UpdateSubtree_Recursive.hpp"

private:

void Update()
{
    UpdateSubtree_Recursive(Root());
}

// TODO: It would be nice if there were a fast way to set node flags back to NodeFlag::Id, if applicable.
void UndoUpdate()
{
    InvertTransform( transform );
    UpdateSubtree_Recursive(Root());
}

public:

UpdateFlag_T NodeNeedsUpdateQ( const Int node ) const
{
    return NodeNeedsUpdateQ( node, p, q, mid_changedQ );
}

UpdateFlag_T NodeNeedsUpdateQ(
    const Int node, const Int pivot_p, const Int pivot_q, const bool midQ
) const
{
    // Assuming that 0 <= pivot_p < pivot_q < LeafNodeCount();
    
    auto [begin,end] = NodeRange(node);
    
    const bool a =  midQ;
    const bool b = !midQ;
    
    const Int p_ = pivot_p + a;
    const Int q_ = pivot_q + b;
    
    if( (p_ <= begin) && (end <= q_) )
    {
        return UpdateFlag_T(a);
    }
    else if( (end <= p_) || (q_ <= begin) )
    {
        return UpdateFlag_T(b);
    }
    else
    {
        return UpdateFlag_T::Split;
    }
}

private:

FoldFlag_T LoadPivots(
    std::pair<Int,Int> && pivots, const Real angle_theta, const bool reflectQ_
)
{
    std::tie(p,q) = MinMax(pivots);
    theta = angle_theta;
    reflectQ = reflectQ_;
    
    const Int n = VertexCount() ;
    const Int mid_size = q - p - Int(1);
    const Int rem_size = n - mid_size - Int(2);
    
    if( (mid_size <= Int(0)) || (rem_size <= Int(0)) ) [[unlikely]]
    {
        return FoldFlag_T::RejectedByPivots;
    }
    
    mid_changedQ = (mid_size <= rem_size);
    
    p_shifted = p + mid_changedQ;
    q_shifted = q + !mid_changedQ;
    
    // TODO: There is maybe a more efficient way to compute the pivot vectors.
    X_p = VertexCoordinates(p);
    X_q = VertexCoordinates(q);
    transform = PivotTransform( X_p, X_q, theta, reflectQ );
    
    return FoldFlag_T::Accepted;
}
