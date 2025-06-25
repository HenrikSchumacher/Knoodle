public:

void ComputeConstraintGraphs() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeConstraintGraphs"));
    
    const Int E_count = E_V.Dim(0);

    RaggedList<Int,Int> DhV_E ( E_count, E_count );
    RaggedList<Int,Int> DvV_E ( E_count, E_count );
    
    Tensor1<Int,Int> E_DhV ( E_count     , Uninitialized );
    Tensor1<Int,Int> E_DvV ( E_count     , Uninitialized );
    
    // We need these in the edge loop below.
    Tensor1<Int,Int> V_DhV ( vertex_count, Uninitialized );
    Tensor1<Int,Int> V_DvV ( vertex_count, Uninitialized );

    
    for( Int v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
        // TODO: Segments are also allowed to contain exactly one vertex.
        if( V_dE(v_0,West) == Uninitialized )
        {
            // Collect horizontal segment.
            const Int s = DhV_E.SublistCount();
            Int w  = v_0;
            Int de = V_dE(w,East);
            Int v;
            V_DhV[w] = s;
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDedge(de);
                DhV_E.Push(e);
                E_DhV[e] = s;
                
                v  = E_V.data()[de];
                de = V_dE(v,East);
                w  = v;
                V_DhV[w] = s;
            }

            DhV_E.FinishSublist();
        }
        
        if( V_dE(v_0,South) == Uninitialized )
        {
            // Collect vertical segment.
            const Int s = DvV_E.SublistCount();
            Int  w = v_0;
            Int de = V_dE(w,North);
            Int v;
            V_DvV[w] = s;
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDedge(de);
                DvV_E.Push(e);
                E_DvV[e] = s;
                
                v  = E_V.data()[de];
                de = V_dE(v,North);
                w  = v;
                V_DvV[w] = s;
            }

            DvV_E.FinishSublist();
        }
    }
    
    // We use TripleAggregator to let Sparse::MatrixCSR tally the duplicate edges.
    TripleAggregator<Int,Int,Cost_T,Int> DhE_agg ( E_count );
    TripleAggregator<Int,Int,Cost_T,Int> DvE_agg ( E_count );
    
    Tensor1<Int,Int> E_DhE_from ( E_count, Uninitialized );
    Tensor1<Int,Int> E_DvE_from ( E_count, Uninitialized );

    for( Int e = 0; e < E_count; ++e )
    {
//        if( !ValidIndexQ(e) )
//        {
//            print(ClassName()+"::ComputeConstraintGraphs: invalid edge index " + ToString(e) + ".");
//            continue;
//        }

        const Int de_0 = ToDedge(e,Tail);
        const Int de_1 = ToDedge(e,Head);
        
        if( !DedgeActiveQ(de_0) || !DedgeActiveQ(de_1) ) { continue; }
        
        const Int c_0 = E_V(e,Tail);
        const Int c_1 = E_V(e,Head);
        
        switch( E_dir[e] )
        {
            case East:
            {
                const Int s_0 = V_DvV[c_0];
                const Int s_1 = V_DvV[c_1];
                E_DvE_from[e] = DvE_agg.Size();
                DvE_agg.Push(s_0,s_1,Cost_T(1));
                break;
            }
            case North:
            {
                const Int s_0 = V_DhV[c_0];
                const Int s_1 = V_DhV[c_1];
                E_DhE_from[e] = DhE_agg.Size();
                DhE_agg.Push(s_0,s_1,Cost_T(1));
                break;
            }
            case West:
            {
                const Int s_0 = V_DvV[c_0];
                const Int s_1 = V_DvV[c_1];
                E_DvE_from[e] = DvE_agg.Size();
                DvE_agg.Push(s_1,s_0,Cost_T(1));
                break;
            }
            case South:
            {
                const Int s_0 = V_DhV[c_0];
                const Int s_1 = V_DhV[c_1];
                E_DhE_from[e] = DhE_agg.Size();
                DhE_agg.Push(s_1,s_0,Cost_T(1));
                break;
            }
            default:
            {
                eprint( MethodName("ComputeConstraintGraphs") + ": Invalid entry of edge direction array TRE_dir detected.");
                break;
            }
        }
    }
    
    // Pushing some additional edges here.
    // They come at no cost.
    
    if ( settings.saturate_facesQ )
    {
        const EdgeContainer_T Gl_edges = this->template SaturatingEdges<0>();
        
        for( Int e = 0; e < Gl_edges.Dim(0); ++e )
        {
            const Int v_0 = Gl_edges(e,Tail);
            const Int v_1 = Gl_edges(e,Head);

            DvE_agg.Push( V_DvV[v_1], V_DvV[v_0], Cost_T(0) );
            DhE_agg.Push( V_DhV[v_0], V_DhV[v_1], Cost_T(0) );
        }
    }
    
    if ( settings.saturate_facesQ )
    {
        const EdgeContainer_T Gr_edges = this->template SaturatingEdges<1>();
        
        for( Int e = 0; e < Gr_edges.Dim(0); ++e )
        {
            const Int v_0 = Gr_edges(e,Tail);
            const Int v_1 = Gr_edges(e,Head);
            
            DvE_agg.Push( V_DvV[v_0], V_DvV[v_1], Cost_T(0) );
            DhE_agg.Push( V_DhV[v_0], V_DhV[v_1], Cost_T(0) );
        }
    }

    
    // We use Sparse::MatrixCSR to tally the duplicates.
    // The counts are stored in D*E_costs for the TopologicalTightening.
    {
        const Int n = DhV_E.SublistCount();
        
        Sparse::MatrixCSR<Cost_T,Int,Int> A (DhE_agg,n,n,Int(1),true,0,true);
        
        this->SetCache( "Dh", DiGraph_T( n, A.NonzeroPositions_AoS() ) );
        
        auto [outer,inner,values,assembler,m_,n_] = A.Disband();
        
        this->SetCache( "DhE_costs", std::move(values) );
        
        auto lut = assembler.Transpose().Inner();
        
        Tensor1<Int,Int> E_DhE ( E_count, Uninitialized );
        
        for( Int e = 0; e < E_count; ++e )
        {
            const Int i = E_DhE_from[e];
            
            if( i != Uninitialized )
            {
                E_DhE[e] = lut[i];
            }
        }
        
        this->SetCache( "DhV_E", std::move(DhV_E) );
        this->SetCache( "E_DhE", std::move(E_DhE) );
    }
    
    {
        const Int n = DvV_E.SublistCount();
        
        Sparse::MatrixCSR<Cost_T,Int,Int> A (DvE_agg,n,n,Int(1),true,0,true);

        this->SetCache( "Dv", DiGraph_T( n, A.NonzeroPositions_AoS() ) );
                       
        auto [outer,inner,values,assembler,m_,n_] = A.Disband();
        
        this->SetCache( "DvE_costs", std::move(values) );
        
        auto lut = assembler.Transpose().Inner();
        
        Tensor1<Int,Int> E_DvE  ( E_count, Uninitialized );
        
        for( Int e = 0; e < E_count; ++e )
        {
            const Int i = E_DvE_from[e];
            
            if( i != Uninitialized )
            {
                E_DvE[e] = lut[i];
            }
        }
        
        this->SetCache( "DvV_E", std::move(DvV_E) );
        this->SetCache( "E_DvE", std::move(E_DvE) );
        
    }
    
    // Not sure which of these I really need.
    this->SetCache( "E_DhV", std::move(E_DhV) );
    this->SetCache( "E_DvV", std::move(E_DvV) );
    this->SetCache( "V_DhV", std::move(V_DhV) );
    this->SetCache( "V_DvV", std::move(V_DvV) );
}


cref<DiGraph_T> Dv() const
{
    if( !this->InCacheQ("Dv") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<DiGraph_T>("Dv");
}

cref<Tensor1<Cost_T,Int>> DvEdgeCosts() const
{
    if( !this->InCacheQ("DvE_costs") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Cost_T,Int>>("DvE_costs");
}

cref<Tensor1<Int,Int>> VertexToDvVertex() const
{
    if( !this->InCacheQ("V_DvV") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Int,Int>>("V_DvV");
}

cref<Tensor1<Int,Int>> EdgeToDvEdge() const
{
    if( !this->InCacheQ("E_DvE") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Int,Int>>("E_DvE");
}

cref<Tensor1<Int,Int>> EdgeToDvVertex() const
{
    if( !this->InCacheQ("E_DvV") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Int,Int>>("E_DvV");
}


cref<DiGraph_T> Dh() const
{
    if( !this->InCacheQ("Dh") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<DiGraph_T>("Dh");
}

cref<Tensor1<Cost_T,Int>> DhEdgeCosts() const
{
    if( !this->InCacheQ("DhE_costs") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Cost_T,Int>>("DhE_costs");
}

cref<Tensor1<Int,Int>> VertexToDhVertex() const
{
    if( !this->InCacheQ("V_DhV") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Int,Int>>("V_DhV");
}

cref<Tensor1<Int,Int>> EdgeToDhEdge() const
{
    if( !this->InCacheQ("E_DhE") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Int,Int>>("E_DhE");
}

cref<Tensor1<Int,Int>> EdgeToDhVertex() const
{
    if( !this->InCacheQ("E_DhV") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<Tensor1<Int,Int>>("E_DhV");
}

private:


void ComputeFaceSegments() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeFaceSegments"));
    
    RaggedList<Int,Int> F_DvV ( FaceCount(), E_V.Dim(0) );
    RaggedList<Int,Int> F_DhV ( FaceCount(), E_V.Dim(0) );
    
    auto & E_DvV = EdgeToDvVertex();
    auto & E_DhV = EdgeToDhVertex();
    
    TraverseAllFaces(
        []( const Int f ){ (void)f; },
        [&E_DvV,&E_DhV,&F_DvV,&F_DhV,this](
            const Int f, const Int k, const Int de
        )
        {
            (void)f;
            (void)k;
            
            auto [e,d] = FromDedge(de);
            
            if( (E_dir[e] == North) || (E_dir[e] == South) )
            {
                F_DvV.Push( E_DvV[e] );
            }
            else
            {
                F_DhV.Push( E_DhV[e] );
            }
        },
        [&F_DvV,&F_DhV]( const Int f )
        {
            (void)f;
            F_DvV.FinishSublist();
            F_DhV.FinishSublist();
        }
    );
    
    this->SetCache( "F_DvV", std::move(F_DvV) );
    this->SetCache( "F_DhV", std::move(F_DhV) );
}

public:

cref<RaggedList<Int,Int>> FaceDvVertices() const
{
    if( !this->InCacheQ("F_DvV") ) { ComputeFaceSegments(); }
    
    return this->GetCache<RaggedList<Int,Int>>("F_DvV");
}

cref<RaggedList<Int,Int>> FaceDhVertices() const
{
    if( !this->InCacheQ("F_DhV") ) { ComputeFaceSegments(); }
    
    return this->GetCache<RaggedList<Int,Int>>("F_DhV");
}
