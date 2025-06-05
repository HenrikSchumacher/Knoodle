private:

void ComputeConstraintGraphs()
{
    TOOLS_PTIC(ClassName() + "::ComputeConstraintGraphs");
 
    // cf. Klau, Mutzel - Optimal Compaction of Orthogonal Grid Drawings
    
    // TODO: Segments are numbered this way:
    // TODO: S_h = {s_0,s_1,...,s_k}; S_v = {s_{k+1},...,s_n}
    // TODO: Alas, my MultiDiGraph class does not like it!
    
 
    Aggregator<Int,Int> S_h_ptr_agg ( EdgeCount() );
    Aggregator<Int,Int> S_h_idx_agg ( EdgeCount() );
    
    Aggregator<Int,Int> S_v_ptr_agg ( EdgeCount() );
    Aggregator<Int,Int> S_v_idx_agg ( EdgeCount() );
    
    E_S_h = Tensor1<Int,Int>( EdgeCount(), Int(-1) );
    E_S_v = Tensor1<Int,Int>( EdgeCount(), Int(-1) );
    
    V_S_h = Tensor1<Int,Int>( VertexCount(), Int(-1) );
    V_S_v = Tensor1<Int,Int>( VertexCount(), Int(-1) );
    
    Aggregator<Int,Int> D_h_agg ( Int(2) * EdgeCount() );
    Aggregator<Int,Int> D_v_agg ( Int(2) * EdgeCount() );
    
//    Int counter_h = 0;
//    Int counter_v = 0;
    
    S_h_ptr_agg.Push(Int(0));
    S_v_ptr_agg.Push(Int(0));
    
    for( Int v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
        // TODO: Segments are also allowed to contain exactly one vertex.
        if( (V_dE(v_0,West) == Int(-1)) && (V_dE(v_0,East) != Int(-1)) )
        {
            // Collect horizontal segment.
            const Int s = S_h_ptr_agg.Size() - Int(1);
            Int w  = v_0;
            Int de = V_dE(w,East);
            Int v;
            V_S_h[w] = s;
            do
            {
                auto [e,dir] = FromDiEdge(de);
                S_h_idx_agg.Push(e);
                E_S_h[e] = s;
                
                v  = E_V.data()[de];
                de = V_dE(v,East);
                w  = v;
                V_S_h[w] = s;
            }
            while( de != Int(-1) );

            S_h_ptr_agg.Push( S_h_idx_agg.Size() );
        }
        
        // TODO: Segments are also allowed to contain exactly one vertex.
        if( (V_dE(v_0,South) == Int(-1)) && (V_dE(v_0,North) != Int(-1)) )
        {
            // Collect vertical segment.
            const Int s = S_v_ptr_agg.Size() - Int(1);
            Int  w = v_0;
            Int de = V_dE(w,North);
            Int v;
            V_S_v[w] = s;
            
            do
            {
                auto [e,dir] = FromDiEdge(de);
                S_v_idx_agg.Push(e);
                E_S_v[e] = s;
                
                v  = E_V.data()[de];
                de = V_dE(v,North);
                w  = v;
                V_S_v[w] = s;
            }
            while( de != Int(-1) );

            S_v_ptr_agg.Push( S_v_idx_agg.Size() );
        }
    }

    
    S_h_ptr = S_h_ptr_agg.Get();
    S_h_idx = S_h_idx_agg.Get();
    
    S_v_ptr = S_v_ptr_agg.Get();
    S_v_idx = S_v_idx_agg.Get();
    
    // This pushes only the directly obvious relation stemming form edges in H.
    // Thise correspond to the sets A_h and A_v from the paper.
    for( Int e = 0; e < edge_count; ++e )
    {
//        print("e = " + ToString(e) + "; E_dir[e] = " + ToString(e) );
        switch( E_dir[e] )
        {
            case East:
            {
                D_v_agg.Push( V_S_v[E_V(e,Tail)] );
                D_v_agg.Push( V_S_v[E_V(e,Head)] );
                break;
            }
            case North:
            {
                D_h_agg.Push( V_S_h[E_V(e,Tail)] );
                D_h_agg.Push( V_S_h[E_V(e,Head)] );
                break;
            }
            case West:
            {
                D_v_agg.Push( V_S_v[E_V(e,Head)] );
                D_v_agg.Push( V_S_v[E_V(e,Tail)] );
                break;
            }
            case South:
            {
                D_h_agg.Push( V_S_h[E_V(e,Head)] );
                D_h_agg.Push( V_S_h[E_V(e,Tail)] );
                break;
            }
            default:
            {
                eprint( ClassName() + "::ComputeConstraintGraphs: Invalid entry of edge direction array E_dir detected.");
                break;
            }
        }
    }
    
//
//    TOOLS_DUMP( D_h_agg.Size() );
//    auto D_h_edges = D_h_agg.Get();
//    TOOLS_DUMP( D_h_edges );
//    TOOLS_DUMP( D_h_agg.Size() );
//    
//    valprint(
//        "D_h_edges by points",
//        ArrayToString( D_h_agg.data(), {D_h_agg.Size(),Int(2)} )
//    );
//    
//    TOOLS_DUMP( D_v_agg.Size() );
//    auto D_v_edges = D_v_agg.Get();
//    TOOLS_DUMP( D_v_edges );
//    TOOLS_DUMP( D_v_agg.Size() );
//    
//    valprint(
//        "D_b_edges by points",
//        ArrayToString( D_v_agg.data(), {D_v_agg.Size()/2,Int(2)} )
//    );
    
    D_h = DiGraph_T( S_h_ptr.Size() - Int(1), D_h_agg.data(), D_h_agg.Size()/2 );
    D_v = DiGraph_T( S_v_ptr.Size() - Int(1), D_v_agg.data(), D_v_agg.Size()/2 );
    
    TOOLS_PTOC(ClassName() + "::ComputeConstraintGraphs");
}

struct Segment
{
    Int  s;
    bool horizontalQ;
};

// This is l(s) from the paper, at least for maximal segments.
Segment LeftSegment( const Segment s ) const
{
    return s.horizontalQ
           ? Segment(V_S_v[ S_h_ptr[s] - 1],false)
           : s;
}

// This is r(s) from the paper, at least for maximal segments.
Segment RightSegment( const Segment s ) const
{
    return s.horizontalQ
           ? Segment(V_S_v[ S_h_ptr[s+1] - 1],false)
           : s;
}

// This is b(s) from the paper, at least for maximal segments.
Segment BottomSegment( const Segment s ) const
{
    return !s.horizontalQ
           ? Segment(V_S_h[ S_v_ptr[s] - 1],false)
           : s;
}

// This is t(s) from the paper, at least for maximal segments.
Segment TopSegment( const Segment s ) const
{
    return !s.horizontalQ
           ? Segment(V_S_h[ S_v_ptr[s+1] - 1],false)
           : s;
}
