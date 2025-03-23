public:

struct ClisbyNode
{
//    cref<ClisbyTree> T;
    Int id;
    Int local_id;
//    Int depth;
//    Int column;
    Int begin;
    Int end;
    Vector_T    center;
    Real        radius;
    Transform_T f;
    NodeFlag_T  state;
    bool        interiorQ;
    bool        all_movedQ;
    bool        all_unmovedQ;
    
    ClisbyNode() = default;
    
    ClisbyNode(
        const Int id_, const Int local_id_, cref<ClisbyTree> T,
        const Int p, const Int q, bool midQ
    )
    :   id          ( id_                  )
    ,   local_id    ( local_id_            )
//    ,   depth       ( T.Depth(id)          )
//    ,   column      ( T.Column(id)         )
    ,   begin       ( T.NodeBegin(id)      )
    ,   end         ( T.NodeEnd(id)        )
    ,   center      ( T.NodeBallPtr(id)    )
    ,   radius      ( T.NodeRadius(id)     )
    ,   interiorQ   ( T.InteriorNodeQ(id)  )
    {
        // TODO: Maybe compute the split flags etc.
        
        if( interiorQ )
        {
            f     = T.NodeTransform(id);
            state = T.NodeFlag(id);
        }
        
        const Int p_ = p + midQ;
        const Int q_ = q + !midQ;
        
        if( midQ )
        {
            all_unmovedQ = (end <= p_   ) || (q_  <= begin);
            all_movedQ   = (p_  <= begin) && (end <= q_   );
        }
        else
        {
            all_unmovedQ = (p_  <= begin) && (end <= q_   );
            all_movedQ   = (end <= p_   ) || (q_  <= begin);
        }
        
    }
    
}; // ClisbyNode
