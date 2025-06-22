public:

void ComputeConstraintGraphs()
{
    TOOLS_PTIC(ClassName()+"::ComputeConstraintGraphs");
    
    Aggregator<Int,Int> DhV_E_ptr_agg ( TRE_count );
    Aggregator<Int,Int> DhV_E_idx_agg ( TRE_count );
    Aggregator<Int,Int> DvV_E_ptr_agg ( TRE_count );
    Aggregator<Int,Int> DvV_E_idx_agg ( TRE_count );
    
    E_DhV = Tensor1<Int,Int>( TRE_count   , Uninitialized );
    E_DvV = Tensor1<Int,Int>( TRE_count   , Uninitialized );
    V_DhV = Tensor1<Int,Int>( vertex_count, Uninitialized );
    V_DvV = Tensor1<Int,Int>( vertex_count, Uninitialized );

    // We use TripleAggregator to let Sparse::MatrixCSR tally the duplicate edges.
    TripleAggregator<Int,Int,Cost_T,Int> DhE_agg ( TRE_count );
    TripleAggregator<Int,Int,Cost_T,Int> DvE_agg ( TRE_count );
    
    Tensor1<Int,Int> TRE_DhE_from ( TRE_count, Uninitialized );
    Tensor1<Int,Int> TRE_DvE_from ( TRE_count, Uninitialized );
    
    DhV_E_ptr_agg.Push(Int(0));
    DvV_E_ptr_agg.Push(Int(0));
    
    for( Int v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
        // TODO: Segments are also allowed to contain exactly one vertex.
        if( V_dTRE(v_0,West) == Uninitialized )
        {
            // Collect horizontal segment.
            const Int s = DhV_E_ptr_agg.Size() - Int(1);
            Int w  = v_0;
            Int de = V_dTRE(w,East);
            Int v;
            V_DhV[w] = s;
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDedge(de);
                DhV_E_idx_agg.Push(e);
                E_DhV[e] = s;
                
                v  = TRE_V.data()[de];
                de = V_dTRE(v,East);
                w  = v;
                V_DhV[w] = s;
            }

            DhV_E_ptr_agg.Push( DhV_E_idx_agg.Size() );
        }
        
        if( V_dTRE(v_0,South) == Uninitialized )
        {
            // Collect vertical segment.
            const Int s = DvV_E_ptr_agg.Size() - Int(1);
            Int  w = v_0;
            Int de = V_dTRE(w,North);
            Int v;
            V_DvV[w] = s;
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDedge(de);
                DvV_E_idx_agg.Push(e);
                E_DvV[e] = s;
                
                v  = TRE_V.data()[de];
                de = V_dTRE(v,North);
                w  = v;
                V_DvV[w] = s;
            }

            DvV_E_ptr_agg.Push( DvV_E_idx_agg.Size() );
        }
    }
    
    DhV_E_ptr = DhV_E_ptr_agg.Get();
    DhV_E_idx = DhV_E_idx_agg.Get();
    
    DvV_E_ptr = DvV_E_ptr_agg.Get();
    DvV_E_idx = DvV_E_idx_agg.Get();


    for( Int e = 0; e < TRE_count; ++e )
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
                const Int s_0 = V_DvV[c_0];
                const Int s_1 = V_DvV[c_1];
                TRE_DvE_from[e] = DvE_agg.Size();
                DvE_agg.Push(s_0,s_1,Cost_T(1));
                break;
            }
            case North:
            {
                const Int s_0 = V_DhV[c_0];
                const Int s_1 = V_DhV[c_1];
                TRE_DhE_from[e] = DhE_agg.Size();
                DhE_agg.Push(s_0,s_1,Cost_T(1));
                break;
            }
            case West:
            {
                const Int s_0 = V_DvV[c_0];
                const Int s_1 = V_DvV[c_1];
                TRE_DvE_from[e] = DvE_agg.Size();
                DvE_agg.Push(s_1,s_0,Cost_T(1));
                break;
            }
            case South:
            {
                const Int s_0 = V_DhV[c_0];
                const Int s_1 = V_DhV[c_1];
                TRE_DhE_from[e] = DhE_agg.Size();
                DhE_agg.Push(s_1,s_0,Cost_T(1));
                break;
            }
            default:
            {
                eprint( ClassName()+"::ComputeConstraintGraphs: Invalid entry of edge direction array TRE_dir detected.");
                break;
            }
        }
    }
    
    
    // Pushing some additional edges here.
    // They come at no cost.
    
    for( auto & e : Dv_edge_agg )
    {
        DvE_agg.Push(e[0],e[1],Cost_T(0));
    }
    
    for( auto & e : Dh_edge_agg )
    {
        DhE_agg.Push(e[0],e[1],Cost_T(0));
    }
    
    // We use Sparse::MatrixCSR to tally the duplicates.
    // The counts are stored in D_*_edge_costs for the TopologicalTightening.
    {
        const Int n = DhV_E_ptr.Size() - Int(1);
        Sparse::MatrixCSR<Cost_T,Int,Int> A (DhE_agg,n,n,Int(1),true,0,true);
        Dh = DiGraph_T( n, A.NonzeroPositions_AoS() );
        DhE_costs = A.Values();
        auto [outer,inner,values,assembler,m_,n_] = A.Disband();
        auto lut = assembler.Transpose().Inner();
        TRE_DhE = Tensor1<Int,Int>( TRE_count, Uninitialized );
        for( Int e = 0; e < TRE_count; ++e )
        {
            const Int i = TRE_DhE_from[e];
            if( i != Uninitialized )
            {
                TRE_DhE[e] = lut[i];
            }
        }
    }
    
    {
        const Int n = DvV_E_ptr.Size() - Int(1);
        Sparse::MatrixCSR<Cost_T,Int,Int> A (DvE_agg,n,n,Int(1),true,0,true);
        Dv = DiGraph_T( n, A.NonzeroPositions_AoS() );
        DvE_costs = A.Values();
        auto [outer,inner,values,assembler,m_,n_] = A.Disband();
        auto lut = assembler.Transpose().Inner();
        TRE_DvE = Tensor1<Int,Int>( TRE_count, Uninitialized );
        for( Int e = 0; e < TRE_count; ++e )
        {
            const Int i = TRE_DvE_from[e];
            if( i != Uninitialized )
            {
                TRE_DvE[e] = lut[i];
            }
        }
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeConstraintGraphs");
}
