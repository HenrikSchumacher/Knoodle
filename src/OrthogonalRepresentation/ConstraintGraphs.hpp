public:

template<bool verboseQ = false>
void ComputeConstraintGraphs() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeConstraintGraphs"));
    
    const Int V_count = VertexCount();
    const Int E_count = E_V.Dim(0);

    RaggedList<Int,Int> DhV_E ( E_count, E_count );
    RaggedList<Int,Int> DvV_E ( E_count, E_count );
    
    RaggedList<Int,Int> DhV_V ( E_count, V_count );
    RaggedList<Int,Int> DvV_V ( E_count, V_count );
    
    Tensor1<Int,Int> E_DhV ( E_count     , Uninitialized );
    Tensor1<Int,Int> E_DvV ( E_count     , Uninitialized );
    
    // We need these in the edge loop below.
    Tensor1<Int,Int> V_DhV ( vertex_count, Uninitialized );
    Tensor1<Int,Int> V_DvV ( vertex_count, Uninitialized );
    
//    EdgeContainer_T DhV_leftright_V ( vertex_count );
//    EdgeContainer_T DvV_bottomtop_V ( vertex_count );

    
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
            DhV_V.Push(w);
            
            while( de != Uninitialized )
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
        
        if( V_dE(v_0,South) == Uninitialized )
        {
            // Collect vertical segment.
            const Int s = DvV_E.SublistCount();
            Int  w = v_0;
            Int de = V_dE(w,North);
            Int v;
            V_DvV[w] = s;
            DvV_V.Push(w);
            
            while( de != Uninitialized )
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
    
    // We use TripleAggregator to let Sparse::MatrixCSR tally the duplicate edges.
    TripleAggregator<Int,Int,Cost_T,Int> DhE_agg ( E_count );
    TripleAggregator<Int,Int,Cost_T,Int> DvE_agg ( E_count );
    
    Tensor1<Int,Int> E_DhE_from ( E_count, Uninitialized );
    Tensor1<Int,Int> E_DvE_from ( E_count, Uninitialized );

    for( Int e = 0; e < E_count; ++e )
    {
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
                DhE_agg.Push(s_0,s_1,Cost_T(1));
                
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
                DvE_agg.Push(s_1,s_0,Cost_T(1));
                
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
                DhE_agg.Push(s_1,s_0,Cost_T(1));
                
                if constexpr ( verboseQ )
                {
                    print("DhE {s_0,s_1} = { " + ToString(s_0) + "," + ToString(s_1) + " } from {c_0,c_1} = { " + ToString(c_0) + "," + ToString(c_1) +  " }");
                }
                break;
            }
            default:
            {
                eprint( MethodName("ComputeConstraintGraphs") + ": Invalid entry of edge direction array TRE_dir detected.");
                break;
            }
        }
    }
    
    // Computing DvV_limit_DhV and DhV_limit_DvV
    
    {
        EdgeContainer_T DvV_bottomtop_DhV ( DvV_E.SublistCount() );
        
        for( Int s = 0; s < DvV_E.SublistCount(); ++s )
        {
            DvV_bottomtop_DhV(s,Tail ) = V_DhV[ DvV_V.Sublist(s).begin()[0] ];
            DvV_bottomtop_DhV(s,Head ) = V_DhV[ DvV_V.Sublist(s).end()[-1]  ];
        }
        
        this->SetCache( "DvV_bottomtop_DhV", std::move(DvV_bottomtop_DhV) );
    }
    
    {
        EdgeContainer_T DhV_leftright_DvV ( DhV_E.SublistCount() );
        
        for( Int s = 0; s < DhV_E.SublistCount(); ++s )
        {
            DhV_leftright_DvV(s,Left ) = V_DvV[ DhV_V.Sublist(s).begin()[0] ];
            DhV_leftright_DvV(s,Right) = V_DvV[ DhV_V.Sublist(s).end()[-1]  ];
        }
        
        this->SetCache( "DhV_leftright_DvV", std::move(DhV_leftright_DvV) );
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

            if constexpr ( verboseQ )
            {
                print("saturating DvE {s_0,s_1} = { " + ToString(V_DvV[v_1]) + "," + ToString(V_DvV[v_0]) + " } from {c_0,c_1} = { " + ToString(v_1) + "," + ToString(v_0) +  " }");
                
                print("saturating DhE {s_0,s_1} = { " + ToString(V_DhV[v_0]) + "," + ToString(V_DhV[v_1]) + " } from {c_0,c_1} = { " + ToString(v_0) + "," + ToString(v_1) +  " }");
            }
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
            
            
            if constexpr ( verboseQ )
            {
                print("saturating DvE {s_0,s_1} = { " + ToString(V_DvV[v_0]) + "," + ToString(V_DvV[v_1]) + " } from {c_0,c_1} = { " + ToString(v_0) + "," + ToString(v_1) +  " }");
                
                print("saturating DhE {s_0,s_1} = { " + ToString(V_DhV[v_0]) + "," + ToString(V_DhV[v_1]) + " } from {c_0,c_1} = { " + ToString(v_0) + "," + ToString(v_1) +  " }");
            }
        }
    }

    
    // We use Sparse::MatrixCSR to tally the duplicates.
    // The counts are stored in D*E_costs for the TopologicalTightening.
    // TODO: I could generate a DiGraph_T from DhE_agg and use DiGraph_T::AdjacencyMatrix to tally the edges and to delete duplicates...
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

cref<EdgeContainer_T> DvVBottomTop() const
{
    if( !this->InCacheQ("DvV_bottomtop_DhV") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<EdgeContainer_T>("DvV_bottomtop_DhV");
}

cref<EdgeContainer_T> DhVLeftRight() const
{
    if( !this->InCacheQ("DhV_leftright_DvV") ) { ComputeConstraintGraphs(); }
    
    return this->GetCache<EdgeContainer_T>("DhV_leftright_DvV");
}


private:


void ComputeFaceVHSegments() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeFaceVHSegments"));
    
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
    if( !this->InCacheQ("F_DvV") ) { ComputeFaceVHSegments(); }
    
    return this->GetCache<RaggedList<Int,Int>>("F_DvV");
}

cref<RaggedList<Int,Int>> FaceDhVertices() const
{
    if( !this->InCacheQ("F_DhV") ) { ComputeFaceVHSegments(); }
    
    return this->GetCache<RaggedList<Int,Int>>("F_DhV");
}

cref<RaggedList<Int,Int>> FaceSegments() const
{
    if( !this->InCacheQ("FaceSegments") )
    {
        RaggedList<Int,Int> F_segments ( FaceCount(), E_V.Dim(0) );
        
        auto & E_DvV = EdgeToDvVertex();
        auto & E_DhV = EdgeToDhVertex();
        
        const Int h = Dv().VertexCoordinates();
        
        TraverseAllFaces(
            []( const Int f ){ (void)f; },
            [&E_DvV,&E_DhV,&F_segments,h,this](
                const Int f, const Int k, const Int de
            )
            {
                (void)f;
                (void)k;
                
                auto [e,d] = FromDedge(de);
                
                if( (E_dir[e] == North) || (E_dir[e] == South) )
                {
                    F_segments.Push( E_DvV[e] );
                }
                else
                {
                    F_segments.Push( E_DhV[e] + h );
                }
            },
            [&F_segments]( const Int f )
            {
                (void)f;
                F_segments.FinishSublist();
            }
        );
        
        this->SetCache( "FaceSegments", std::move(F_segments) );
    }
    
    return this->GetCache<RaggedList<Int,Int>>("FaceSegments");
}


//EdgeContainer_T DhV_left_DvV()
//{
//    EdgeContainer_T a (Dh().VertexCount());
//    
//    auto DhV_V =
//    
//    for( Int s = 0; s < Dh().VertexCount(); ++s )
//    {
//        
//    }
//}


// This collects the "column indices" for the constraint matrix for the system
//
//  x_{r_i,l_j} + x_{r_j,l_i} + x_{t_j,b_i} + x_ {t_i,b_j} \geq 0,
//
//  for (i,j) \in S \times S.
//
// We filter out some cases that would lead to infeasible systems.
// We also try to cull equations that are automatically fulfilled by the edges in Dv() and Dh().
// TODO: It would be even nice if we could cull also those equations that are implicitly enforced by paths in the graph.

// TODO: It would be even nice if we could delete duplicated equations, too. It happens quite frequently that a face touches a line segment more than once.

Tiny::MatrixList_AoS<4,2,Int,Int> SeparationConstraints() const
{
    using A_T = std::array<Int,2>;
    
    auto & F_DvV = FaceDvVertices();
    auto & F_DhV = FaceDhVertices();
    
    auto & DvV_bt = DvVBottomTop();
    auto & DhV_lr = DhVLeftRight();
    
    // TODO: Better size prediction.
    Aggregator<A_T,Int> agg ( FaceCount() * Int(2) );
    
    // Index shifts/offsets
    const Int h = Dv().VertexCount();

    for( Int f = 0; f < FaceCount(); ++f )
    {
        auto f_DvV = F_DvV.Sublist(f);
        auto f_DhV = F_DhV.Sublist(f);
        
        // There is no way in which a 4-, 5- or 6-face can give us any new information.
        if( (f_DvV.Size() <= Int(3)) || (f_DhV.Size() <= Int(3)) )
        {
            continue;
        }
        
        // Beware, we have to check _both_ f_DvV.Size() and f_DhV.Size() since they are not always equal when we inserted edges by turn regularization. We can get two horizontal edges or two verticals edges next to each other.

        
        auto DvV_load = [&DvV_bt]( Int i )
        {
            return std::array<Int,4>{ i, i, DvV_bt(i,Tail), DvV_bt(i,Head) };
        };
        
        auto DhV_load = [&DhV_lr]( Int i )
        {
            return std::array<Int,4>{ DhV_lr(i,Tail), DhV_lr(i,Head), i, i };
        };
        
        auto push = [&agg,h](
            Int l_i, Int r_i, Int b_i, Int t_i,
            Int l_j, Int r_j, Int b_j, Int t_j
        )
        {
            
            // TODO: Cull constraints that are already satisfied or already excluded by directed paths in Dv() and Dh().
            
            agg.Push( A_T{ r_i    , l_j    } );
            agg.Push( A_T{ r_j    , l_i    } );
            agg.Push( A_T{ t_j + h, b_i + h} );
            agg.Push( A_T{ t_i + h, b_j + h} );
        };
        
        for( Int i : f_DvV )
        {
            auto [l_i,r_i,b_i,t_i] = DvV_load(i);
            
            for( Int j : f_DvV )
            {
                if( i == j ) { continue; };
                // This also excludes the cases l_i == r_j and r_i == l_j as both segments are vertical.

                auto [l_j,r_j,b_j,t_j] = DvV_load(j);
                
                // In these cases there is an edge between i and j in Dv.
                // So this constraint will be satisfied anyways.
                if( (t_j == b_i) || (t_i == b_j) ) { continue; }
                
                
                push( l_i, r_i, b_i, t_i,  l_j, r_j, b_j, t_j );
            }
            
            for( Int j : f_DhV )
            {
                auto [l_j,r_j,b_j,t_j] = DhV_load(j);
                
                // In each of these 4 cases these segments touch each other.
                // We might get infeasible systems if we include this constraint.
                if( (r_i == l_j) || (r_j == l_i) || (t_j == b_i) || (t_i == b_j) )
                {
                    continue;
                }

                push( l_i, r_i, b_i, t_i,  l_j, r_j, b_j, t_j );
            }
        }
     
        for( Int i : f_DhV )
        {
            auto [l_i,r_i,b_i,t_i] = DhV_load(i);
            
            for( Int j : f_DhV )
            {
                if( i == j ) { continue; }
                // This also excludes the cases b_i == t_j and t_i == b_j as both segments are horizontal.
                
                auto [l_j,r_j,b_j,t_j] = DhV_load(j);
                
                // In these cases there is an edge between i and j in Dh.
                // So this constraint will be satisfied anyways.
                if( (r_i == l_j) || (r_j == l_i) ) { continue; }
                
                push( l_i, r_i, b_i, t_i,  l_j, r_j, b_j, t_j );
            }
        }
    }
    
    Tiny::MatrixList_AoS<4,2,Int,Int> a ( agg.Size() / Int(8) );
    
    a.Read( &agg.data()[0][0] );
        
    return a;
}
