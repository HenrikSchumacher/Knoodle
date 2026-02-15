public:

// Trying to construct the graphs D_x and D_y from Brideman, Battista, Didimo,Liotta, Tamassia, Vismara - Turn-regularity and optimal area drawings of orthogonal.
// We use slightly different notation:
//      Dv := D_x   (graph for the maximal _v_ertical segments)
//      Dh := D_y   (graph for the maximal _h_orizontal segments)
//
// Alas, our graphs Dv and Dh might be slightly different from the ones defined by Brideman et al. because we might put in a few too many edges. In particular, Dv and Dh might not be planar.
// But the optimization problem is a minimum cost tension problem and its dual will be a minimum cost flow, no matter whether Dv or Dh are planar. And even better: we do not need their planar structure to write down the dual problems.

template<bool verboseQ = false>
void ComputeConstraintGraphs() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeConstraintGraphs<" + ToString(verboseQ) +  ">"));
    
    const Int V_count = V_dE.Dim(0);
    const Int E_count = E_V.Dim(0);

    RaggedList<Int,Int> DhV_E ( E_count, E_count );
    RaggedList<Int,Int> DvV_E ( E_count, E_count );
    
    RaggedList<Int,Int> DhV_V ( E_count, V_count );
    RaggedList<Int,Int> DvV_V ( E_count, V_count );
    
    Tensor1<Int,Int> E_DhV ( E_count, Uninitialized );
    Tensor1<Int,Int> E_DvV ( E_count, Uninitialized );
    
    // We need these in the edge loop below.
    Tensor1<Int,Int> V_DhV ( V_count, Uninitialized );
    Tensor1<Int,Int> V_DvV ( V_count, Uninitialized );
    
    
    auto invalid_dedgeQ = [this]( const Int de )
    {
        return  (de == Uninitialized)
                ||
                (   this->settings.soften_virtual_edgesQ
                    &&
                    this->DedgeVirtualQ(de)
                );
    };

    
    const Int C_end = C_A.Dim(0);

    // For each segment we record which vertices and edges are part of it (DhV_V,DhV_E,DvV_V,DvV_E).
    // And vice versa: we record for each vertex or edge to which segment it belongs (V_DhV, V_DvV, E_DhV, E_DvV).
    // We do this by finding vertices that have no edge to the west or south; then we can simply walk to the east or north, until we hit a vertex that has no edge in that direction.
    // We can skip all crossings, since crossings have valence 4 thus cannot be the start or end of any segment.
    // Thus, we start at v_0 = C_end.
    for( Int v_0 = C_end; v_0 < V_end; ++v_0 )
    {
        if( !VertexActiveQ(v_0) ) { continue; }
            
        // TODO: Does this work also for segments with precisely one vertex?
        if( invalid_dedgeQ(V_dE(v_0,West)) )
        {
            // Collect horizontal segment.
            const Int s = DhV_E.SublistCount();
            Int w  = v_0;
            Int de = V_dE(w,East);
            Int v;
            V_DhV[w] = s;
            DhV_V.Push(w);
            
            while( !invalid_dedgeQ(de) )
            {
                auto [e,dir] = FromDedge(de);
                DhV_E.Push(e);
                E_DhV[e] = s;
                
                v  = E_V.data()[de];
                de = V_dE(v,East);
                w  = v;
                V_DhV[w] = s;
                DhV_V.Push(w);
            }

            DhV_E.FinishSublist();
            DhV_V.FinishSublist();
        }
        
        // TODO: Does this work also for segments with precisely one vertex?
        if( invalid_dedgeQ(V_dE(v_0,South)) )
        {
            // Collect vertical segment.
            const Int s = DvV_E.SublistCount();
            Int  w = v_0;
            Int de = V_dE(w,North);
            Int v;
            V_DvV[w] = s;
            DvV_V.Push(w);
            
            while( !invalid_dedgeQ(de) )
            {
                auto [e,dir] = FromDedge(de);
                DvV_E.Push(e);
                E_DvV[e] = s;
                
                v  = E_V.data()[de];
                de = V_dE(v,North);
                w  = v;
                V_DvV[w] = s;
                DvV_V.Push(w);
            }

            DvV_E.FinishSublist();
            DvV_V.FinishSublist();
        }
    }
    
    // Very likely, we have many duplicate edges in Dv and Dh.
    // We use TripleAggregator to let Sparse::MatrixCSR tally the duplicate edges.
    TripleAggregator<Int,Int,Cost_T,Int> DhE_agg ( E_count );
    TripleAggregator<Int,Int,Cost_T,Int> DvE_agg ( E_count );
    
    Tensor1<Int,Int> E_DhE_from ( E_count, Uninitialized );
    Tensor1<Int,Int> E_DvE_from ( E_count, Uninitialized );
    
    cptr<Int> dE_V = E_V.data();

    // For each edge (virtual or not) in the PD, we record which horizontal/vertical segments it connects.
    // These will become some of the edges in the graphs Dv and Dh.
    for( Int e = 0; e < E_end; ++e )
    {
        const Int de_0 = ToDedge(e,Tail);
        const Int de_1 = ToDedge(e,Head);
        
        if( !DedgeActiveQ(de_0) || !DedgeActiveQ(de_1) ) { continue; }
        
        const bool virtualQ = DedgeVirtualQ(de_0) || DedgeVirtualQ(de_1);
        
        if ( settings.soften_virtual_edgesQ && virtualQ ) { continue; }
        
        const Int c_0 = dE_V[de_0];
        const Int c_1 = dE_V[de_1];
        
        switch( E_dir[e] )
        {
            case East:
            {
                const Int s_0 = V_DvV[c_0];
                const Int s_1 = V_DvV[c_1];
                E_DvE_from[e] = DvE_agg.Size();
                DvE_agg.Push(s_0,s_1,Cost_T(!virtualQ));
                
                if constexpr ( verboseQ )
                {
                    print("DvE {s_0,s_1} = { " + ToString(s_0) + "," + ToString(s_1) + " } from {c_0,c_1} = { " + ToString(c_0) + "," + ToString(c_1) +  " }");
                }
                break;
            }
            case North:
            {
                const Int s_0 = V_DhV[c_0];
                const Int s_1 = V_DhV[c_1];
                E_DhE_from[e] = DhE_agg.Size();
                DhE_agg.Push(s_0,s_1,Cost_T(!virtualQ));
                
                if constexpr ( verboseQ )
                {
                    print("DhE {s_0,s_1} = { " + ToString(s_0) + "," + ToString(s_1) + " } from {c_0,c_1} = { " + ToString(c_0) + "," + ToString(c_1) +  " }");
                }
                break;
            }
            case West:
            {
                const Int s_0 = V_DvV[c_0];
                const Int s_1 = V_DvV[c_1];
                E_DvE_from[e] = DvE_agg.Size();
                DvE_agg.Push(s_1,s_0,Cost_T(!virtualQ));
                
                if constexpr ( verboseQ )
                {
                    print("DvE {s_0,s_1} = { " + ToString(s_0) + "," + ToString(s_1) + " } from {c_0,c_1} = { " + ToString(c_0) + "," + ToString(c_1) +  " }");
                }
                break;
            }
            case South:
            {
                const Int s_0 = V_DhV[c_0];
                const Int s_1 = V_DhV[c_1];
                E_DhE_from[e] = DhE_agg.Size();
                DhE_agg.Push(s_1,s_0,Cost_T(!virtualQ));
                
                if constexpr ( verboseQ )
                {
                    print("DhE {s_0,s_1} = { " + ToString(s_0) + "," + ToString(s_1) + " } from {c_0,c_1} = { " + ToString(c_0) + "," + ToString(c_1) +  " }");
                }
                break;
            }
            default:
            {
                eprint( MethodName("ComputeConstraintGraphs") + ": Invalid entry of edge direction array E_dir detected.");
                break;
            }
        }
    }
    
//    // Computing DvV_limit_DhV. (For every vertical segment, record the two horizontal segments at its ends.)
//    // TODO: Potential candidate for erasure.
//    {
//        EdgeContainer_T DvV_bottomtop_DhV ( DvV_E.SublistCount() );
//        
//        const Int s_count = DvV_E.SublistCount();
//        
//        for( Int s = 0; s < s_count; ++s )
//        {
//            DvV_bottomtop_DhV(s,Tail ) = V_DhV[ DvV_V.Sublist(s).begin()[0] ];
//            DvV_bottomtop_DhV(s,Head ) = V_DhV[ DvV_V.Sublist(s).end()[-1]  ];
//        }
//        
//        this->SetCache( "DvV_bottomtop_DhV", std::move(DvV_bottomtop_DhV) );
//    }
//    
//    // Computing DhV_limit_DvV. (For every vertical segment, record the two horizontal segments at its ends.)
//    // TODO: Potential candidate for erasure.
//    {
//        EdgeContainer_T DhV_leftright_DvV ( DhV_E.SublistCount() );
//        
//        const Int s_count = DhV_E.SublistCount();
//        
//        for( Int s = 0; s < s_count; ++s )
//        {
//            DhV_leftright_DvV(s,Left ) = V_DvV[ DhV_V.Sublist(s).begin()[0] ];
//            DhV_leftright_DvV(s,Right) = V_DvV[ DhV_V.Sublist(s).end()[-1]  ];
//        }
//        
//        this->SetCache( "DhV_leftright_DvV", std::move(DhV_leftright_DvV) );
//    }
    
    // Collecting the saturating edges of Gl. They have zero costs.
    if ( settings.saturate_regionsQ )
    {
        const EdgeContainer_T Gl_edges = this->template SaturatingEdges<0>();
        
        const Int Gl_E_count = Gl_edges.Dim(0);
        
        for( Int e = 0; e < Gl_E_count; ++e )
        {
            const Int v_0 = Gl_edges(e,Tail);
            const Int v_1 = Gl_edges(e,Head);

            // TODO: To get the same graph Dv = D_x as in We ought to collect a saturating edge into Dv only if one of the vertical segments it connects is _unconstrained_.
            // BEWARE: we have to reverse the direction of edges here!
            DvE_agg.Push( V_DvV[v_1], V_DvV[v_0], Cost_T(0) );
            
            // TODO: We ought to collect a saturating edge into Dv only if one of the horizontal segments it connects is _unconstrained_.
            DhE_agg.Push( V_DhV[v_0], V_DhV[v_1], Cost_T(0) );

            if constexpr ( verboseQ )
            {
                print("saturating DvE {s_0,s_1} = { " + ToString(V_DvV[v_1]) + "," + ToString(V_DvV[v_0]) + " } from {c_0,c_1} = { " + ToString(v_1) + "," + ToString(v_0) +  " }");
                
                print("saturating DhE {s_0,s_1} = { " + ToString(V_DhV[v_0]) + "," + ToString(V_DhV[v_1]) + " } from {c_0,c_1} = { " + ToString(v_0) + "," + ToString(v_1) +  " }");
            }
        }
    }
    
    // Collecting the saturating edges of Gr. They have zero costs.
    if ( settings.saturate_regionsQ )
    {
        const EdgeContainer_T Gr_edges = this->template SaturatingEdges<1>();
        
        const Int Gr_E_count = Gr_edges.Dim(0);
        
        for( Int e = 0; e < Gr_E_count; ++e )
        {
            const Int v_0 = Gr_edges(e,Tail);
            const Int v_1 = Gr_edges(e,Head);
            
            // TODO: We ought to collect a saturating edge into Dv only if one of the vertical segments it connects is _unconstrained_.
            DvE_agg.Push( V_DvV[v_0], V_DvV[v_1], Cost_T(0) );
            // TODO: We ought to collect a saturating edge into Dv only if one of the horizontal segments it connects is _unconstrained_.
            DhE_agg.Push( V_DhV[v_0], V_DhV[v_1], Cost_T(0) );
            
            if constexpr ( verboseQ )
            {
                print("saturating DvE {s_0,s_1} = { " + ToString(V_DvV[v_0]) + "," + ToString(V_DvV[v_1]) + " } from {c_0,c_1} = { " + ToString(v_0) + "," + ToString(v_1) +  " }");
                
                print("saturating DhE {s_0,s_1} = { " + ToString(V_DhV[v_0]) + "," + ToString(V_DhV[v_1]) + " } from {c_0,c_1} = { " + ToString(v_0) + "," + ToString(v_1) +  " }");
            }
        }
    }

    
    // We use Sparse::MatrixCSR to tally the duplicate edge in Dh.
    // The counts are stored in DhE_costs for the TopologicalTightening.
    {
        const Int n = DhV_E.SublistCount();

        Sparse::MatrixCSR<Cost_T,Int,Int> A (DhE_agg,n,n,Int(1),true,0,true);

        if( n > Int(0) )
        {
            this->SetCache( "Dh", DiGraph_T( n, A.NonzeroPositions_AoS() ) );
        }
        else
        {
            this->SetCache( "Dh", DiGraph_T() );
        }

        auto [outer,inner,values,assembler,m_,n_] = A.Disband();

        this->SetCache( "DhE_costs", std::move(values) );
        
        Tensor1<Int,Int> E_DhE ( E_count, Uninitialized );
        
        if( n > Int(0) )
        {
            auto lut = assembler.Transpose().Inner();
            
            for( Int e = 0; e < E_count; ++e )
            {
                if( !EdgeActiveQ(e) ) { continue; }
                
                const Int i = E_DhE_from[e];
                
                if( i != Uninitialized )
                {
                    E_DhE[e] = lut[i];
                }
            }
        }

        this->SetCache( "DhV_E", std::move(DhV_E) );
        this->SetCache( "E_DhE", std::move(E_DhE) );
    }
    
    // We use Sparse::MatrixCSR to tally the duplicate edge in Dv.
    // The counts are stored in DvE_costs for the TopologicalTightening.
    {
        const Int n = DvV_E.SublistCount();
        
        Sparse::MatrixCSR<Cost_T,Int,Int> A (DvE_agg,n,n,Int(1),true,0,true);

        if( n > Int(0) )
        {
            this->SetCache( "Dv", DiGraph_T( n, A.NonzeroPositions_AoS() ) );
        }
        else
        {
            this->SetCache( "Dv", DiGraph_T() );
        }
        
        auto [outer,inner,values,assembler,m_,n_] = A.Disband();
        
        this->SetCache( "DvE_costs", std::move(values) );
        
        Tensor1<Int,Int> E_DvE  ( E_count, Uninitialized );
        
        if( n > Int(0) )
        {
            auto lut = assembler.Transpose().Inner();
            
            for( Int e = 0; e < E_count; ++e )
            {
                if( !EdgeActiveQ(e) ) { continue; }
                
                const Int i = E_DvE_from[e];
                
                if( i != Uninitialized )
                {
                    E_DvE[e] = lut[i];
                }
            }
        }
        
        this->SetCache( "DvV_E", std::move(DvV_E) );
        this->SetCache( "E_DvE", std::move(E_DvE) );
    }
    
    // Not sure which of these I really need.
    this->SetCache( "E_DhV", std::move(E_DhV) );
    this->SetCache( "E_DvV", std::move(E_DvV) );
    
    this->SetCache( "DhV_V", std::move(DhV_V) );
    this->SetCache( "V_DhV", std::move(V_DhV) );
    
    this->SetCache( "DvV_V", std::move(DvV_V) );
    this->SetCache( "V_DvV", std::move(V_DvV) );
}


cref<DiGraph_T> Dv() const
{
    std::string tag ("Dv");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<DiGraph_T>(tag);
}

cref<Tensor1<Cost_T,Int>> DvEdgeCosts() const
{
    std::string tag ("DvE_costs");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Cost_T,Int>>(tag);
}

cref<Tensor1<Int,Int>> VertexToDvVertex() const
{
    std::string tag ("V_DvV");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> EdgeToDvEdge() const
{
    std::string tag ("E_DvE");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> EdgeToDvVertex() const
{
    std::string tag ("E_DvV");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}


cref<DiGraph_T> Dh() const
{
    std::string tag ("Dh");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<DiGraph_T>(tag);
}

cref<Tensor1<Cost_T,Int>> DhEdgeCosts() const
{
    std::string tag ("DhE_costs");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Cost_T,Int>>(tag);
}

cref<Tensor1<Int,Int>> VertexToDhVertex() const
{
    std::string tag ("V_DhV");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> EdgeToDhEdge() const
{
    std::string tag ("E_DhE");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> EdgeToDhVertex() const
{
    std::string tag ("E_DhV");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { ComputeConstraintGraphs(); }
    return this->GetCache<Tensor1<Int,Int>>(tag);
}


cref<RaggedList<Int,Int>> VerticalSegmentEdges() const
{
    if( !this->InCacheQ("DvV_E") ) { ComputeConstraintGraphs(); }
    return this->GetCache<RaggedList<Int,Int>>("DvV_E");
}

cref<RaggedList<Int,Int>> HorizontalSegmentEdges() const
{
    if( !this->InCacheQ("DhV_E") ) { ComputeConstraintGraphs(); }
    return this->GetCache<RaggedList<Int,Int>>("DhV_E");
}

cref<RaggedList<Int,Int>> VerticalSegmentVertices() const
{
    if( !this->InCacheQ("DvV_V") ) { ComputeConstraintGraphs(); }
    return this->GetCache<RaggedList<Int,Int>>("DvV_V");
}

cref<RaggedList<Int,Int>> HorizontalSegmentVertices() const
{
    if( !this->InCacheQ("DhV_V") ) { ComputeConstraintGraphs(); }
    return this->GetCache<RaggedList<Int,Int>>("DhV_V");
}

//// TODO: Potential candidate for erasure.
//cref<EdgeContainer_T> DvVBottomTop() const
//{
//    if( !this->InCacheQ("DvV_bottomtop_DhV") ) { ComputeConstraintGraphs(); }
//    return this->GetCache<EdgeContainer_T>("DvV_bottomtop_DhV");
//}
//
//// TODO: Potential candidate for erasure.
//cref<EdgeContainer_T> DhVLeftRight() const
//{
//    if( !this->InCacheQ("DhV_leftright_DvV") ) { ComputeConstraintGraphs(); }
//    return this->GetCache<EdgeContainer_T>("DhV_leftright_DvV");
//}
//
//
//private:
//
//// Not what I need, I guess. This cycles around faces of the PD with the virtual edges added.
//// But we need to cycle around faces in Dv and Dh to create the flow networks.
//void ComputeRegionVHSegments() const
//{
//    TOOLS_PTIMER(timer,MethodName("ComputeRegionVHSegments"));
//    
//    const bool soften_virtual_edgesQ = settings.soften_virtual_edgesQ;
//    
//    Int f_count = RegionCount(soften_virtual_edgesQ);
//    
//    RaggedList<Int,Int> F_DvV ( f_count, E_V.Dim(0) );
//    RaggedList<Int,Int> F_DhV ( f_count, E_V.Dim(0) );
//    
//    auto & E_DvV = EdgeToDvVertex();
//    auto & E_DhV = EdgeToDhVertex();
//    
//    TraverseAllRegions(
//        []( const Int f ){ (void)f; },
//        [&E_DvV,&E_DhV,&F_DvV,&F_DhV,this](
//            const Int f, const Int k, const Int de
//        )
//        {
//            (void)f;
//            (void)k;
//            
//            auto [e,d] = FromDedge(de);
//            
//            if( (E_dir[e] == North) || (E_dir[e] == South) )
//            {
//                F_DvV.Push( E_DvV[e] );
//            }
//            else
//            {
//                F_DhV.Push( E_DhV[e] );
//            }
//        },
//        [&F_DvV,&F_DhV]( const Int f )
//        {
//            (void)f;
//            F_DvV.FinishSublist();
//            F_DhV.FinishSublist();
//        },
//        soften_virtual_edgesQ
//    );
//    
//    this->SetCache( "F_DvV", std::move(F_DvV) );
//    this->SetCache( "F_DhV", std::move(F_DhV) );
//}
//
//
//public:
//
//
//// Not what I need, I guess as this does not respect saturating edges.
//// TODO: Potential candidate for erasure.
//cref<RaggedList<Int,Int>> RegionDvVertices() const
//{
//    if( !this->InCacheQ("F_DvV") ) { ComputeRegionVHSegments(); }
//    
//    return this->GetCache<RaggedList<Int,Int>>("F_DvV");
//}
//
//// Not what I need, I guess as this does not respect saturating edges.
//// TODO: Potential candidate for erasure.
//cref<RaggedList<Int,Int>> RegionDhVertices() const
//{
//    if( !this->InCacheQ("F_DhV") ) { ComputeRegionVHSegments(); }
//    
//    return this->GetCache<RaggedList<Int,Int>>("F_DhV");
//}
//
//// Not what I need, I guess as this does not respect saturating edges.
//// TODO: Potential candidate for erasure.
//cref<RaggedList<Int,Int>> RegionSegments() const
//{
//    if( !this->InCacheQ("RegionSegments") )
//    {
//        const bool soften_virtual_edgesQ = settings.soften_virtual_edgesQ;
//        
//        RaggedList<Int,Int> F_segments ( RegionCount(soften_virtual_edgesQ), E_V.Dim(0) );
//        
//        auto & E_DvV = EdgeToDvVertex();
//        auto & E_DhV = EdgeToDhVertex();
//        
//        const Int h = Dv().VertexCoordinates();
//        
//        TraverseAllRegions(
//            []( const Int f ){ (void)f; },
//            [&E_DvV,&E_DhV,&F_segments,h,this](
//                const Int f, const Int k, const Int de
//            )
//            {
//                (void)f;
//                (void)k;
//                
//                auto [e,d] = FromDedge(de);
//                
//                if( (E_dir[e] == North) || (E_dir[e] == South) )
//                {
//                    F_segments.Push( E_DvV[e] );
//                }
//                else
//                {
//                    F_segments.Push( E_DhV[e] + h );
//                }
//            },
//            [&F_segments]( const Int f )
//            {
//                (void)f;
//                F_segments.FinishSublist();
//            },
//            soften_virtual_edgesQ
//        );
//        
//        this->SetCache( "RegionSegments", std::move(F_segments) );
//    }
//    
//    return this->GetCache<RaggedList<Int,Int>>("RegionSegments");
//}
