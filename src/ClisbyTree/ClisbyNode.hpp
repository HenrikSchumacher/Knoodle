public:

class ClisbyNode
{
    cref<ClisbyTree> T;
    Int id;
    Int local_id;
//    Int depth;
//    Int column;
    Int begin;
    Int end;
    Vector_T    center;
    Real        radius;
    Transform_T f;
    NodeFlag_T state;
    bool        interior_Q;
    bool        contains_moved_Q;
    bool        contains_unmoved_Q;
    
    ClisbyNode( const Int id_, const Int local_id_, cref<ClisbyTree> T )
    ,   id          ( id_                  )
    ,   local_id    ( local_id_            )
//    ,   depth       ( T.Depth(id)          )
//    ,   column      ( T.Column(id)         )
    ,   begin       ( T.NodeBegin(id)      )
    ,   end         ( T.NodeEnd(id)        )
    ,   center      ( T.NodeBallPtr(id)    )
    ,   radius      ( T.NodeRadius(id)     )
    ,   interior_Q  ( T.InteriorNodeQ(id)  )
    {
        // TODO: Maybe compute the split flags etc.
        
        if( interior_Q )
        {
            f     = T.NodeTransform(id);
            state = T.NodeFlag(id);
        }
        
        const Int p_ = T.p + midQ;
        const Int q_ = T.q + !midQ;
        
        if( midQ )
        {
            contains_no_moved_Q   = (end <= p_   ) || (q_  <= begin);
            contains_no_unmoved_Q = (p_  <= begin) && (end <= q_   );
        }
        else
        {
            contains_no_moved_Q   = (p_  <= begin) && (end <= q_   );
            contains_no_unmoved_Q = (end <= p_   ) || (q_  <= begin);
        }
        
    }
    
}; // ClisbyNode
