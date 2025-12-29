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
    bool        internalQ;
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
    ,   internalQ   ( T.InternalNodeQ(id)  )
    {
        // TODO: Maybe compute the split flags etc.
        
        if( internalQ )
        {
            f = T.NodeTransform(id);
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
    
    
    
    UpdateFlag_T NeedsUpdateQ( const Int pivot_p, const Int pivot_q, const bool midQ ) const
    {
        // Assuming that 0 <= pivot_p < pivot_q < LeafNodeCount();
        
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
    
    
}; // ClisbyNode
