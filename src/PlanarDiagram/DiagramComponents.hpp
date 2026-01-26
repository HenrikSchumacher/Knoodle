public:

cref<MultiGraph_T> DiagramComponentLinkComponentGraph() const
{
    const std::string tag ("DiagramComponentLinkComponentGraph");
    
    if( !this->InCacheQ(tag) )
    {
        const auto & A_lc = ArcLinkComponents();
        
        // Using a hash map to delete duplicates.
        // TODO: We could also use a Sparse::BinaryMatrix here.
        // TODO: But using an associative container might be faster because it uses much less memory.
        
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
        
        this->SetCache( tag, MultiGraph_T( LinkComponentCount(), std::move(comp_edges) ) );
    }
    
    return this->template GetCache<MultiGraph_T>(tag);
}

cref<ComponentMatrix_T> DiagramComponentLinkComponentMatrix() const
{
    return DiagramComponentLinkComponentGraph().ComponentVertexMatrix();
}

Int DiagramComponentCount() const
{
    return DiagramComponentLinkComponentMatrix().RowCount();
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


void Split( mref<PD_List_T> pd_list )
{
    TOOLS_PTIMER(timer,MethodName("Split"));
    
    if( CrossingCount() <= Int(0) ) { return; }
    
    cref<ComponentMatrix_T> A = DiagramComponentLinkComponentMatrix();
    
    // dc = diagram component
    // lc = link component
    
    cptr<Int> dc_lc_ptr = A.Outer().data();
    cptr<Int> dc_lc_idx = A.Inner().data();
    
    const Int dc_count  = A.RowCount();
    
    if( dc_count <= Int(1) ) { return; }
    
    const auto & lc_arcs = LinkComponentArcs();
    cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
    cptr<Int> lc_arc_idx = lc_arcs.Elements().data();
    
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

        PlanarDiagram pd (dc_arc_count/Int(2),Int(0));        
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
        
        //DEBUGGING
        if( pd.max_crossing_count != c_counter )
        {
            wprint(MethodName("Split") + ": pd.max_crossing_count != c_counter.");
        }
        if( pd.max_arc_count != a_counter )
        {
            wprint(MethodName("Split") + ": pd.max_arc_count != a_counter.");
        }
        
        pd.crossing_count = c_counter;
        pd.arc_count      = a_counter;
        
        pd_list.push_back( std::move(pd) );
    } // for( Int dc = 0; dc < dc_count; ++dc )
    
    Compress();
}
