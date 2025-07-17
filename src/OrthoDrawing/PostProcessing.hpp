public:

Aggregator<Int,Int> SegmentsInfluencedByVirtualEdges()
{
    Aggregator<Int,Int> agg ( Int(2) * virtual_edge_count );
    
    auto & E_DvV = EdgeToDvVertex();
    auto & E_DhV = EdgeToDhVertex();
    
    const Int v_offset = 0;
    const Int h_offset = Dv().VertexCount();
    
    const Int e_count = E_V.Dim(0);
    
    for( Int e = e_count - virtual_edge_count; e < e_count; ++ e )
    {
        const Int v_0 = E_V(e,Tail);
        const Int v_1 = E_V(e,Head);
        
        if( E_dir[e] == North )
        {
            auto [e_0,d_0] = FromDedge(V_dE(v_0,South));
            auto [e_1,d_1] = FromDedge(V_dE(v_1,North));
            
            agg.Push( E_DvV[e_0] + v_offset );
            agg.Push( E_DhV[e_1] + h_offset );
        }
        if( E_dir[e] == South )
        {
            auto [e_0,d_0] = FromDedge(V_dE(v_0,North));
            auto [e_1,d_1] = FromDedge(V_dE(v_1,South));
            
            agg.Push( E_DvV[e_0] + v_offset );
            agg.Push( E_DhV[e_1] + h_offset );
        }
        else if( E_dir[e] == East )
        {
            auto [e_0,d_0] = FromDedge(V_dE(v_0,West ));
            auto [e_1,d_1] = FromDedge(V_dE(v_1,East ));
            
            agg.Push( E_DvV[e_0] + v_offset );
            agg.Push( E_DhV[e_1] + h_offset );
        }
        else if( E_dir[e] == West )
        {
            auto [e_0,d_0] = FromDedge(V_dE(v_0,East ));
            auto [e_1,d_1] = FromDedge(V_dE(v_1,West ));
            
            agg.Push( E_DvV[e_0] + v_offset );
            agg.Push( E_DhV[e_1] + h_offset );
        }
    }
    
    return agg;
}
