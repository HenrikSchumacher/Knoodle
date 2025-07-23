#include "UpdateSubtree_Recursive.hpp"
#include "UpdateSubtree_ManualStack.hpp"


private:

template<bool pull_transformsQ = true>
void Update( Int start_node )
{
    if constexpr( pull_transformsQ )
    {
        PullTransforms(Root(), start_node);
    }
    
    if constexpr ( manual_stackQ )
    {
        UpdateSubtree_ManualStack(start_node);
    }
    else
    {
        UpdateSubtree_Recursive(start_node);
    }
}

void Update()
{
    this->template Update<false>(Root());
}

//template<bool pull_transformsQ = true>
//void Update( Int root_0, Int root_1 )
//{
//    this->template Update<pull_transformsQ>(root_0);
//    
//    if( root_0 != root_1 )
//    {
//        this->template Update<pull_transformsQ>(root_1);
//    }
//}

private:

// TODO: It would be nice if there were a fast way to set node flags back to NodeFlag::Id, if applicable.
void UndoUpdate()
{
    this->template UndoUpdate<false>(Root());
}

template<bool pull_transformsQ = true>
void UndoUpdate( Int start_node )
{
    InvertTransform( transform );
    
    this->template Update<pull_transformsQ>(start_node);
}

//template<bool pull_transformsQ = true>
//void UndoUpdate( Int start_node_0, Int start_node_1 )
//{
//    InvertTransform( transform );
//    
//    this->template Update<pull_transformsQ>(start_node_0,start_node_1);
//}

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

int LoadPivots(
    std::pair<Int,Int> && pivots, const Real angle_theta, const bool reflectQ_
)
{
    p = Min(pivots.first,pivots.second);
    q = Max(pivots.first,pivots.second);
    theta = angle_theta;
    reflectQ = reflectQ_;
    
    const Int n = VertexCount() ;
    const Int mid_size = q - p - Int(1);
    const Int rem_size = n - mid_size - Int(2);
    
    if( (mid_size <= Int(0)) || (rem_size <= Int(0)) ) [[unlikely]]
    {
        return 1;
    }
    
    mid_changedQ = (mid_size <= rem_size);
    
    p_shifted = p + mid_changedQ;
    q_shifted = q + !mid_changedQ;
    
    // TODO: There is maybe a more efficient way to compute the pivot vectors.
    X_p = VertexCoordinates(p);
    X_q = VertexCoordinates(q);
    transform = PivotTransform( X_p, X_q, theta, reflectQ );
    
    return 0;
}
