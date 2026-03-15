public:

cref<MultiGraph_T> DiagramComponentLinkComponentGraph() const
{
    const std::string tag ("DiagramComponentLinkComponentGraph");
    
    if( !this->InCacheQ(tag) )
    {
        const auto & A_lc = ArcLinkComponents();
        
        // TODO: In case of few link components, we could assemble the dense adjacency matrix and build the matrix from there.
        
        // Using a hash container to delete duplicates.
        
        SetContainer<std::pair<Int,Int>,Tools::pair_hash<Int,Int>> aggregator;
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                const Int a_0 = C_arcs(c,Out,Left );
                const Int a_1 = C_arcs(c,Out,Right);
                
                const Int lc_0 = A_lc[a_0];
                const Int lc_1 = A_lc[a_1];
                
                if( lc_0 != lc_1 )
                {
                    aggregator.emplace(MinMax(lc_0,lc_1));
                }
            }
        }
        
        typename MultiGraph_T::EdgeContainer_T comp_edges (
            static_cast<Int>(aggregator.size())
        );
        
        Int counter = 0;
        
        for( auto p : aggregator )
        {
            comp_edges(counter,MultiGraph_T::Tail) = p.first;
            comp_edges(counter,MultiGraph_T::Head) = p.second;
            ++counter;
        }
        
        MultiGraph_T G ( LinkComponentCount(), std::move(comp_edges) );
        
        this->SetCache( tag, std::move(G) );
    }
    
    return this->template GetCache<MultiGraph_T>(tag);
}

cref<ComponentMatrix_T> DiagramComponentLinkComponentMatrix() const
{
    return DiagramComponentLinkComponentGraph().ComponentVertexMatrix();
}

Int DiagramComponentCount() const
{
    if( LinkComponentCount() <= Int(1) )
    {
        return Int(1);
    }
    else
    {
        return DiagramComponentLinkComponentMatrix().RowCount();
    }
}


void RequireDiagramComponents() const
{
    TOOLS_PTIMER(timer,MethodName("RequireDiagramComponents"));
    
    cref<ComponentMatrix_T> A = DiagramComponentLinkComponentMatrix();
    
    // dc = diagram component
    // lc = link component
    
    cptr<Int> dc_lc_ptr = A.Outer().data();
    cptr<Int> dc_lc_idx = A.Inner().data();
    
    const Int dc_count  = A.RowCount();
    
    RaggedList<Int,Int> dc_arcs ( dc_count + 1, ArcCount() );
    
    const auto & lc_arcs  = LinkComponentArcs();
    cptr<Int> lc_arc_ptr   = lc_arcs.Pointers().data();
    cptr<Int> lc_arc_idx   = lc_arcs.Elements().data();
    
    for( Int dc = 0; dc < dc_count; ++dc )
    {
        const Int i_begin = dc_lc_ptr[dc    ];
        const Int i_end   = dc_lc_ptr[dc + 1];
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int lc = dc_lc_idx[i];
            
            const Int j_begin = lc_arc_ptr[lc    ];
            const Int j_end   = lc_arc_ptr[lc + 1];
            const Int size    = j_end - j_begin;
            
            dc_arcs.Push( &lc_arc_idx[j_begin], size );
        }
        
        dc_arcs.FinishSublist();
    }

    this->SetCache( "DiagramComponentArcs", std::move(dc_arcs) );
}

cref<RaggedList<Int,Int>> DiagramComponentArcs() const
{
    const std::string tag ("DiagramComponentArcs");
    if(!this->InCacheQ(tag)) { RequireDiagramComponents(); }
    return this->template GetCache<RaggedList<Int,Int>>(tag);
}

//cref<Tensor1<Int,Int>> DiagramComponentArcPointers()
//{
//    const std::string tag ("DiagramComponentArcPointers");
//    if(!this->InCacheQ(tag)) { RequireDiagramComponents(); }
//    return this->template GetCache<Tensor1<Int,Int>>(tag);
//}
//
//cref<Tensor1<Int,Int>> DiagramComponentArcIndices()
//{
//    const std::string tag ("DiagramComponentArcIndices");
//    if(!this->InCacheQ(tag)) { RequireDiagramComponents(); }
//    return this->template GetCache<Tensor1<Int,Int>>(tag);
//}
