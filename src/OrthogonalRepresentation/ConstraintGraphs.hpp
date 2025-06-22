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

struct Segment
{
    Int  s;
    bool horizontalQ;
};

//// This is l(s) from the paper, at least for maximal segments.
//Segment LeftSegment( const Segment s ) const
//{
//    return s.horizontalQ
//           ? Segment(V_DvV[ DhV_E_ptr[s] - 1],false)
//           : s;
//}
//
//// This is r(s) from the paper, at least for maximal segments.
//Segment RightSegment( const Segment s ) const
//{
//    return s.horizontalQ
//           ? Segment(V_DvV[ DhV_E_ptr[s+1] - 1],false)
//           : s;
//}
//
//// This is b(s) from the paper, at least for maximal segments.
//Segment BottomSegment( const Segment s ) const
//{
//    return !s.horizontalQ
//           ? Segment(V_DhV[ DvV_E_ptr[s] - 1],false)
//           : s;
//}
//
//// This is t(s) from the paper, at least for maximal segments.
//Segment TopSegment( const Segment s ) const
//{
//    return !s.horizontalQ
//           ? Segment(V_DhV[ DvV_E_ptr[s+1] - 1],false)
//           : s;
//}


public:

void DvClearAddedEdges()
{
    Dv_edge_agg.clear();
}

void DvPushVertexPair( const Int i, const Int j )
{
    Dv_edge_agg.push_back({V_DvV[i],V_DvV[j]});
}

void DvPushEdgePair( const Int i, const Int j )
{
    Dv_edge_agg.push_back({E_DvV[i],E_DvV[j]});
}

void DvPushDedgePair( const Int da, const Int db )
{
    if( (da < Int(0)) || (da >= Int(2) * edge_count ) )
    {
        eprint(ClassName()+"::DvVPushDedgePair: dedge index " + ToString(da) + " is out of bounds.");
        return;
    }
    
    if( (db < Int(0)) || (db >= Int(2) * edge_count ) )
    {
        eprint(ClassName()+"::DvVPushDedgePair: dedge index " + ToString(db) + " is out of bounds.");
        return;
    }
    
    auto [a,d_a] = FromDedge(da);
    auto [b,d_b] = FromDedge(db);
    
    if( (E_dir(a) != Dir_T(1)) && (E_dir(a) != Dir_T(3)) )
    {
        eprint(ClassName()+"::DvVPushDedgePair: dedge " + ToString(da) + " is not vertical.");
        return;
    }
    
    if( (E_dir(b) != Dir_T(1)) && (E_dir(b) != Dir_T(3)) )
    {
        eprint(ClassName()+"::DvVPushDedgePair: dedge " + ToString(db) + " is not vertical.");
        return;
    }
    
    Dv_edge_agg.push_back({E_DvV[a],E_DvV[b]});
}

EdgeContainer_T DvAddedEdges()
{
    const Int n = int_cast<Int>(Dv_edge_agg.size());
                                
    EdgeContainer_T result ( n );
    
    for( Int i = 0; i < n; ++i )
    {
        result(i,0) = Dv_edge_agg[i][0];
        result(i,1) = Dv_edge_agg[i][1];
    }
    
    return result;
}



void DhPushVertexPair( const Int i, const Int j )
{
    Dh_edge_agg.push_back({V_DhV[i],V_DhV[j]});
}

void DhPushEdgePair( const Int i, const Int j )
{
    Dh_edge_agg.push_back({E_DhV[i],E_DhV[j]});
}


void DhPushDedgePair( const Int da, const Int db )
{
    if( (da < Int(0)) || (da >= Int(2) * edge_count ) )
    {
        eprint(ClassName()+"::DhPushDedgePair: dedge index " + ToString(da) + " is out of bounds.");
        return;
    }
    
    if( (db < Int(0)) || (db >= Int(2) * edge_count ) )
    {
        eprint(ClassName()+"::DhPushDedgePair: dedge index " + ToString(db) + " is out of bounds.");
        return;
    }
    
    auto [a,d_a] = FromDedge(da);
    auto [b,d_b] = FromDedge(db);
    
    if( (E_dir(a) != Dir_T(0)) && (E_dir(a) != Dir_T(2)) )
    {
        eprint(ClassName()+"::DhPushDedgePair: dedge " + ToString(da) + " is not horizontal.");
        return;
    }
    
    if( (E_dir(b) != Dir_T(0)) && (E_dir(b) != Dir_T(2)) )
    {
        eprint(ClassName()+"::DhPushDedgePair: dedge " + ToString(db) + " is not horizontal.");
        return;
    }
    
    Dh_edge_agg.push_back({E_DhV[a],E_DhV[b]});
}


EdgeContainer_T DhAddedEdges()
{
    const Int n = int_cast<Int>(Dh_edge_agg.size());
                                
    EdgeContainer_T result ( n );
    
    for( Int i = 0; i < n; ++i )
    {
        result(i,0) = Dh_edge_agg[i][0];
        result(i,1) = Dh_edge_agg[i][1];
    }
    
    return result;
}

void DhClearAddedEdges()
{
    Dh_edge_agg.clear();
}


bool Test_TRE_DhE() const
{
    bool okayQ = true;
    
    for( Int e = 0; e < TRE_count; ++e )
    {
        const Int i = TRE_DhE[e];
        
        if( i != Uninitialized )
        {
            bool s = (E_dir[e] == North);
            bool b =
                ( V_DhV[TRE_V(e,Tail)] == Dh.Edges()(i,!s) )
                &&
                ( V_DhV[TRE_V(e,Head)] == Dh.Edges()(i, s) );
            
            if( !b )
            {
                eprint(ClassName()+"::Test_TRE_DhE: TRE_count of tredge " + ToString(e) + " does not point to correct edge in graph Dh edge.");
                TOOLS_DUMP(e);
                TOOLS_DUMP(V_DhV[TRE_V(e,Tail)]);
                TOOLS_DUMP(V_DhV[TRE_V(e,Head)]);
                
                TOOLS_DUMP(i);
                TOOLS_DUMP(Dh.Edges()(i,!s));
                TOOLS_DUMP(Dh.Edges()(i, s));
            }
            
            okayQ = okayQ && b;
        }
    }
    
    return okayQ;
}


bool Test_TRE_DvE() const
{
    bool okayQ = true;
    
    for( Int e = 0; e < TRE_count; ++e )
    {
        const Int i = TRE_DvE[e];
        
        if( i != Uninitialized )
        {
            bool s = (E_dir[e] == East);
            bool b =
                ( V_DvV[TRE_V(e,Tail)] == Dv.Edges()(i,!s) )
                &&
                ( V_DvV[TRE_V(e,Head)] == Dv.Edges()(i, s) );
            
            if( !b )
            {
                eprint(ClassName()+"::Test_TRE_DvE: TRE_count of tredge " + ToString(e) + " does not point to correct edge in graph Dv edge.");
                TOOLS_DUMP(e);
                TOOLS_DUMP(V_DvV[TRE_V(e,Tail)]);
                TOOLS_DUMP(V_DvV[TRE_V(e,Head)]);
                
                TOOLS_DUMP(i);
                TOOLS_DUMP(Dv.Edges()(i,!s));
                TOOLS_DUMP(Dv.Edges()(i, s));
            }
            
            okayQ = okayQ && b;
        }
    }
    
    return okayQ;
}
