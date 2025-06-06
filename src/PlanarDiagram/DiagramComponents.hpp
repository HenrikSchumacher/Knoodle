public:

cref<MultiGraph_T> DiagramComponentLinkComponentGraph()
{
    const std::string tag ("DiagramComponentLinkComponentGraph");
    
    if( !this->InCacheQ(tag) )
    {
        const auto & A_lc = ArcLinkComponents();
        
        std::unordered_set<std::pair<Int,Int>,Tools::pair_hash<Int,Int>> aggregator;
        
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
        
        this->SetCache( tag,
            MultiGraph_T( LinkComponentCount(), std::move(comp_edges) )
        );
    }
    
    return this->template GetCache<MultiGraph_T>(tag);
}

cref<ComponentMatrix_T> DiagramComponentLinkComponentMatrix()
{
    return DiagramComponentLinkComponentGraph().ComponentVertexMatrix();
}

Int DiagramComponentCount()
{
    return DiagramComponentLinkComponentMatrix().RowCount();
}


void RequireDiagramComponents()
{
    TOOLS_PTIC(ClassName()+"::RequireDiagramComponents");
    
    cref<ComponentMatrix_T> A = DiagramComponentLinkComponentMatrix();
    
    // dc = diagram component
    // lc = link component
    
    cptr<Int> dc_lc_ptr = A.Outer().data();
    cptr<Int> dc_lc_idx = A.Inner().data();
    
    const Int dc_count  = A.RowCount();
    
    Tensor1<Int,Int> dc_arc_ptr ( dc_count + 1 );
    Tensor1<Int,Int> dc_arc_idx ( ArcCount() );
    
    dc_arc_ptr[0] = 0;
    
    cptr<Int> lc_arc_ptr = LinkComponentArcPointers().data();
    cptr<Int> lc_arc_idx = LinkComponentArcIndices().data();
    
    for( Int dc = 0; dc < dc_count; ++dc )
    {
        const Int i_begin = dc_lc_ptr[dc    ];
        const Int i_end   = dc_lc_ptr[dc + 1];
        
        Int p = dc_arc_ptr[dc];
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int lc = dc_lc_idx[i];
            
            const Int j_begin = lc_arc_ptr[lc    ];
            const Int j_end   = lc_arc_ptr[lc + 1];
            const Int size    = j_end - j_begin;
            
            copy_buffer( &lc_arc_idx[j_begin], &dc_arc_idx[p], size );
            
            p += size;
        }
        
        dc_arc_ptr[dc+1] = p;
    }
    
    this->SetCache( "DiagramComponentArcPointers", std::move(dc_arc_ptr) );
    this->SetCache( "DiagramComponentArcIndices" , std::move(dc_arc_idx) );
    
    TOOLS_PTOC(ClassName()+"::RequireDiagramComponents");
}

cref<Tensor1<Int,Int>> DiagramComponentArcPointers()
{
    const std::string tag ("DiagramComponentArcPointers");
    
    if( !this->InCacheQ(tag) )
    {
        RequireDiagramComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> DiagramComponentArcIndices()
{
    const std::string tag ("DiagramComponentArcIndices");
    
    if( !this->InCacheQ(tag) )
    {
        RequireDiagramComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}


void Split( mref<PD_List_T> pd_list )
{
    TOOLS_PTIC(ClassName()+"::Split");
    
    if( CrossingCount() <= Int(0) )
    {
        return;
    }
    
    cref<ComponentMatrix_T> A = DiagramComponentLinkComponentMatrix();
    
    // dc = diagram component
    // lc = link component
    
    cptr<Int> dc_lc_ptr = A.Outer().data();
    cptr<Int> dc_lc_idx = A.Inner().data();
    
    const Int dc_count  = A.RowCount();
    
    if( dc_count <= 1 )
    {
        return;
    }
    
    cptr<Int> lc_arc_ptr = LinkComponentArcPointers().data();
    cptr<Int> lc_arc_idx = LinkComponentArcIndices().data();
    
    C_scratch.Fill(Uninitialized);
    
    mptr<Int> C_labels = C_scratch.data();
    
    for( Int dc = 1; dc < dc_count; ++dc  )
    {
        const Int i_begin = dc_lc_ptr[dc    ];
        const Int i_end   = dc_lc_ptr[dc + 1];
        
        Int dc_arc_count = 0;
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int lc = dc_lc_idx[i];
            
            const Int j_begin = lc_arc_ptr[lc    ];
            const Int j_end   = lc_arc_ptr[lc + 1];
            
            dc_arc_count += j_end - j_begin;
        }

        PlanarDiagram<Int> pd (dc_arc_count/2,0);
        
        Int a_counter = 0;
        Int c_counter = 0;
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int lc = dc_lc_idx[i];
            
            const Int j_begin = lc_arc_ptr[lc    ];
            const Int j_end   = lc_arc_ptr[lc + 1];
            
            for( Int j = j_begin; j < j_end; ++j )
            {
                // Traversing link component `j`.
                const Int a = lc_arc_idx[j];

                // TODO: Handle over/under in ArcState.
//                pd.A_state[a_counter] = ArcState::Active;
                pd.A_state[a_counter] = A_state[a];
                
                const Int c_0 = A_cross(a,Tail);
                const Int c_1 = A_cross(a,Head);
                
                const bool side_0 = (C_arcs(c_0,Out,Right) == a);
                const bool side_1 = (C_arcs(c_1,In ,Right) == a);
                
                DeactivateArc(a);
                
                Int c_0_label;
                Int c_1_label;
                
                // TODO: I think we can get rid of this check.
                if( !ValidIndexQ(C_labels[c_0]) )
                {
                    c_0_label = C_labels[c_0] = c_counter;
                    
                    pd.C_state[c_0_label] = C_state[c_0];
                    
                    ++c_counter;
                }
                else
                {
                    c_0_label = C_labels[c_0];
                    
                    DeactivateCrossing<false>(c_0);
                }
                pd.C_arcs(c_0_label,Out,side_0) = a_counter;
                pd.A_cross(a_counter,Tail) = c_0_label;
                
                if( !ValidIndexQ(C_labels[c_1]) )
                {
                    c_1_label = C_labels[c_1] = c_counter;
                    
                    pd.C_state[c_1_label] = C_state[c_1];
                    
                    ++c_counter;
                }
                else
                {
                    c_1_label = C_labels[c_1];
                    
                    DeactivateCrossing<false>(c_1);
                }
                pd.C_arcs(c_1_label,In,side_1) = a_counter;
                pd.A_cross(a_counter,Head) = c_1_label;
                
                ++a_counter;
            }
        }
        
        pd_list.push_back( std::move(pd) );
    } // for( Int dc = 0; dc < dc_count; ++dc )
    
    CanonicalizeInPlace();
    
    TOOLS_PTOC(ClassName()+"::Split");
}
