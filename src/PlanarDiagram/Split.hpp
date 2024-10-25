public:

void Split( mref<std::vector<PlanarDiagram<Int>>> PD_list )
{
    const auto & A_comp = ArcComponents();
    
    std::unordered_set<std::pair<Int,Int>,Tools::pair_hash<Int,Int>> aggregator;
    
    for( Int c = 0; c < initial_crossing_count; ++c )
    {
        if( CrossingActiveQ(c) )
        {
            aggregator.emplace(
                MinMax( A_comp[C_arcs(c,Out,Left)], A_comp[C_arcs(c,Out,Right)] )
            );
        }
    }
                                                             
                                                             
    Tensor2<Int,Int> comp_edges ( static_cast<Int>(aggregator.size()), Int(2) );
    
    Int counter = 0;
    
    for( auto p : aggregator )
    {
        if( p.first != p.second )
        {
            print("{" + ToString(p.first) + "," + ToString(p.second) + "}");
            comp_edges(counter,0) = p.first;
            comp_edges(counter,1) = p.second;
            ++counter;
        }
    }
    
    dump(comp_edges);
    
    Multigraph<Int> G ( ComponentCount(), std::move(comp_edges) );
    
    const auto & split_comp_ptr = G.ComponentMatrix().Outer();
//    const auto & split_comp_idx = G.ComponentMatrix().Inner();
    
    const Int split_comp_count = G.ComponentMatrix().RowCount();
    
    Tensor1<Int,Int> split_comp_sizes ( split_comp_count );
    
    const auto & comp_ptr = ComponentArcPointers();
//    const auto & comp_idx = ComponentArcIndices();
    
    for( Int split_comp = 0; split_comp < split_comp_count; ++split_comp )
    {
        const Int k_begin = split_comp_ptr[split_comp    ];
        const Int k_end   = split_comp_ptr[split_comp + 1];
        
        for( Int k = k_begin; k < k_end; ++k )
        {
            split_comp_sizes[split_comp] += comp_ptr[k+1] - comp_ptr[k];
        }
    }
    
    dump(split_comp_sizes);
}

