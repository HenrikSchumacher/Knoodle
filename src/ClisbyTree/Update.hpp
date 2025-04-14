#include "UpdateSubtree_Recursive.hpp"
#include "UpdateSubtree_ManualStack.hpp"


private:

void Update()
{
    Update(Root());
}

void Update( Int start_node )
{
    if constexpr ( manual_stackQ )
    {
        UpdateSubtree_ManualStack(start_node);
    }
    else
    {
        UpdateSubtree_Recursive(start_node);
    }
}

void Update( Int root_0, Int root_1 )
{
    if( root_0 != root_1 )
    {
        Update();
    }
    else
    {
        // If root_0 == root_1, then we may assume that all moving vertices lie in this one subtree.
        // Implicitly we assume that the path from root_0 to the global root has cleared already.
        Update(root_0);
    }
}

private:

void UndoUpdate()
{
    UndoUpdate(Root());
}

void UndoUpdate( Int start_node )
{
    InvertTransform( transform );
    
    Update(start_node);
}

void UndoUpdate( Int root_0, Int root_1 )
{
    InvertTransform( transform );
    
    if( root_0 != root_1 )
    {
        Update();
    }
    else
    {
        // If root_0 == root_1, then we may assume that all moving vertices lie in this one subtree.
        // Implicitly we assume that the path from root_0 to the global root has cleared already.
        Update(root_0);
    }
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

int LoadPivots(
    const Int pivot_p, const Int pivot_q, const Real angle_theta, const bool reflectQ_
)
{
    p = Min(pivot_p,pivot_q);
    q = Max(pivot_p,pivot_q);
    theta = angle_theta;
    reflectQ = reflectQ_;
    
    const Int n = VertexCount() ;
    const Int mid_size = q - p - 1;
    const Int rem_size = n - mid_size - 2;
    
    if( (mid_size <= 0) || (rem_size <= 0) ) [[unlikely]]
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
