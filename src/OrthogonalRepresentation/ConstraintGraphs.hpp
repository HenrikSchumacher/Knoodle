private:

void ComputeConstraintGraphs()
{
    TOOLS_PTIC(ClassName()+"::ComputeConstraintGraphs");
    
    Aggregator<Int,Int> S_h_E_ptr_agg ( tre_count );
    Aggregator<Int,Int> S_h_E_idx_agg ( tre_count );
    Aggregator<Int,Int> S_v_E_ptr_agg ( tre_count );
    Aggregator<Int,Int> S_v_E_idx_agg ( tre_count );
    
    E_S_h = Tensor1<Int,Int>( tre_count   , Uninitialized );
    E_S_v = Tensor1<Int,Int>( tre_count   , Uninitialized );
    V_S_h = Tensor1<Int,Int>( vertex_count, Uninitialized );
    V_S_v = Tensor1<Int,Int>( vertex_count, Uninitialized );

    // We use TripleAggregator to let Sparse::MatrixCSR tally the duplicate edges.
    TripleAggregator<Int,Int,Cost_T,Int> D_h_agg ( tre_count );
    TripleAggregator<Int,Int,Cost_T,Int> D_v_agg ( tre_count );
    
    S_h_E_ptr_agg.Push(Int(0));
    S_v_E_ptr_agg.Push(Int(0));
    
    for( Int v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
        // TODO: Segments are also allowed to contain exactly one vertex.
        if( V_dTRE(v_0,West) == Uninitialized )
        {
            // Collect horizontal segment.
            const Int s = S_h_E_ptr_agg.Size() - Int(1);
            Int w  = v_0;
            Int de = V_dTRE(w,East);
            Int v;
            V_S_h[w] = s;
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDedge(de);
                S_h_E_idx_agg.Push(e);
                E_S_h[e] = s;
                
                v  = TRE_V.data()[de];
                de = V_dTRE(v,East);
                w  = v;
                V_S_h[w] = s;
            }

            S_h_E_ptr_agg.Push( S_h_E_idx_agg.Size() );
        }
        
        if( V_dTRE(v_0,South) == Uninitialized )
        {
            // Collect vertical segment.
            const Int s = S_v_E_ptr_agg.Size() - Int(1);
            Int  w = v_0;
            Int de = V_dTRE(w,North);
            Int v;
            V_S_v[w] = s;
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDedge(de);
                S_v_E_idx_agg.Push(e);
                E_S_v[e] = s;
                
                v  = TRE_V.data()[de];
                de = V_dTRE(v,North);
                w  = v;
                V_S_v[w] = s;
            }

            S_v_E_ptr_agg.Push( S_v_E_idx_agg.Size() );
        }
    }
    
    S_h_E_ptr = S_h_E_ptr_agg.Get();
    S_h_E_idx = S_h_E_idx_agg.Get();
    
    S_v_E_ptr = S_v_E_ptr_agg.Get();
    S_v_E_idx = S_v_E_idx_agg.Get();
    

    for( Int e = 0; e < tre_count; ++e )
    {
        if( !ValidIndexQ(e) )
        {
            print(ClassName()+"::ComputeConstraintGraphs: invalid edge index " + ToString(e) + ".");
            continue;
        }
        
        // All virtual edges are active, so we do not have to check them.
        // But the other edges need some check here.
        if( (e < edge_count) && !EdgeActiveQ(e) ) { continue; }
        
        const Int c_0 = TRE_V(e,Tail);
        const Int c_1 = TRE_V(e,Head);
        
        switch( TRE_dir[e] )
        {
            case East:
            {
                const Int s_0 = V_S_v[c_0];
                const Int s_1 = V_S_v[c_1];
                D_v_agg.Push(s_0,s_1,Cost_T(1));
                break;
            }
            case North:
            {
                const Int s_0 = V_S_h[c_0];
                const Int s_1 = V_S_h[c_1];
                D_h_agg.Push(s_0,s_1,Cost_T(1));
                break;
            }
            case West:
            {
                const Int s_0 = V_S_v[c_0];
                const Int s_1 = V_S_v[c_1];
                D_v_agg.Push(s_1,s_0,Cost_T(1));
                break;
            }
            case South:
            {
                const Int s_0 = V_S_h[c_0];
                const Int s_1 = V_S_h[c_1];
                D_h_agg.Push(s_1,s_0,Cost_T(1));
                break;
            }
            default:
            {
                eprint( ClassName()+"::ComputeConstraintGraphs: Invalid entry of edge direction array TRE_dir detected.");
                break;
            }
        }
    }
    
    // TODO: We might want to push some additional edges here.

    
    // We use Sparse::MatrixCSR to tally the duplicates.
    // The counts are stored in D_*_edge_costs for the TopologicalTighening.
    {
        const Int n = S_h_E_ptr.Size() - Int(1);
        Sparse::MatrixCSR<Cost_T,Int,Int> A ( D_h_agg, n, n, Int(1), 1, 0 );
        
        D_h = DiGraph_T( n, A.NonzeroPositions_AoS() );
        
        D_h_edge_costs = A.Values();
    }
    
    {
        const Int n = S_v_E_ptr.Size() - Int(1);
        
        Sparse::MatrixCSR<Cost_T,Int,Int> A ( D_v_agg, n, n, Int(1), 1, 0 );
        
        D_v = DiGraph_T( n, A.NonzeroPositions_AoS() );
        
        D_v_edge_costs = A.Values();
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeConstraintGraphs");
}

struct Segment
{
    Int  s;
    bool horizontalQ;
};

//// This is l(s) from the paper, at least for maximal segments.
//Segment LeftSegment( const Segment s ) const
//{
//    return s.horizontalQ
//           ? Segment(V_S_v[ S_h_E_ptr[s] - 1],false)
//           : s;
//}
//
//// This is r(s) from the paper, at least for maximal segments.
//Segment RightSegment( const Segment s ) const
//{
//    return s.horizontalQ
//           ? Segment(V_S_v[ S_h_E_ptr[s+1] - 1],false)
//           : s;
//}
//
//// This is b(s) from the paper, at least for maximal segments.
//Segment BottomSegment( const Segment s ) const
//{
//    return !s.horizontalQ
//           ? Segment(V_S_h[ S_v_E_ptr[s] - 1],false)
//           : s;
//}
//
//// This is t(s) from the paper, at least for maximal segments.
//Segment TopSegment( const Segment s ) const
//{
//    return !s.horizontalQ
//           ? Segment(V_S_h[ S_v_E_ptr[s+1] - 1],false)
//           : s;
//}


