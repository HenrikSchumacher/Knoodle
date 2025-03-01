public:

class ClisbyNode
{
    cref<ClisbyTree> T;
    Int id;
    Int depth;
    Int column;
    Int begin;
    Int end;
    Transform_T f;
    Vector_T    center;
    Real        radius;
    
    NodeState_T state;
    
    ClisbyNode( Int node, cref<ClisbyTree> T_ )
    :   T      ( T_                     )
    ,   id     (node                    )
    ,   depth  ( T.Depth(node)          )
    ,   column ( T.Column(node)         )
    ,   begin  ( T.NodeBegin(node)      )
    ,   end    ( T.NodeEnd(node)        )
    ,   f      ( T.NodeTransform(node)  )
    ,   center ( T.NodeBallPtr(node)    )
    ,   radius ( T.NodeRadius(node)     )
    ,   state  ( T.NodeState(node)      )
    {
        // TODO: Maybe compute the split flags etc.
    }
    
    // Get the child nods updated by current node's transformation.
    std::pair<ClisbyNode,ClisbyNode> Children() const
    {
        ClisbyNode L( 2 * id + 1, T );
        ClisbyNode R( 2 * id + 2, T );
        
        if( state == NodeState_T::Nonid )
        {
            L.center = f(L.center);
            L.f = (L.state == NodeState_T::Nonid) ? f(L.f) : f;
            L.state  = NodeState_T::Nonid;
            
            R.center = f(R.center);
            R.f = (R.state == NodeState_T::Nonid) ? f(R.f) : f;
            R.state  = NodeState_T::Nonid;
        }
        
        return std::pair(L,R);
    }
    
}; // ClisbyNode
