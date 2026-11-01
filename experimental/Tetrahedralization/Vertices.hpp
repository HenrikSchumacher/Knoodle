public:

struct Vertex_T
{
    Int t = Uninitialized;
    Int v = Uninitialized;
    
    Vertex_T( const Int t_, const Int v_ )
    :   t{ t_ }, v{ v_ }
    {}
    
    Vertex_T( const Int idx )
    :   t{ idx >> 2 }, v{ idx & Int{3} }
    {}
    
    constexpr Int Index() const
    {
        return (t << 2) | (v & Int{3});
    }
    
    constexpr Dface_T Opposite() const
    {
        return Dface_T{ t, v };
    }
    
    friend std::string ToString( cref<Vertex_T> V )
    {
        std::string s ( "Vertex_T{" );
        s += ToString(V.t);
        s += ", ";
        s += ToString(V.v);
        s += "}";
        return s;
    }
        
}; // struct Vertex_T

TetVertexContainer_T VertexLabels() const
{
    TOOLS_PTIMER(timer,MethodName("VertexLabels"));
    
//    print("");
//    print("VertexLabels");

    // TODO: Creating a graph object first is somewhat a detour. It would be faster to do the depth first search on the vertex-glue graph directly in this data structure.
    using Graph_T = MultiGraph<Int,Int>;
    
    PairAggregator<Int,Int,Int> edge_agg (Int{12} * tet_count);
    
    for( Int t_0 = 0; t_0 < tet_capacity; ++t_0 )
    {
        if( !TetActiveQ(t_0) ) { continue; }
        
        for( Int f_0 = 0; f_0 < Int{4}; ++f_0 )
        {
            Dface_T df_0 { t_0, f_0 };
//            print("");
//            TOOLS_DUMP(df_0);
//            
//            
//            TOOLS_DUMP(tet_glue);
//            TOOLS_DUMP(df_0.t);
//            TOOLS_DUMP(df_0.f);
//            TOOLS_DUMP(tet_glue(df_0.t,df_0.f));

            const Gluing_T g = Gluing(df_0);
//            TOOLS_DUMP(g);
            
            if( !g.ValidQ() ) { continue; }
            
            Dface_T df_1 = g.df;
            
            // TODO: DO something that we don't generate each edge twice.
            if( df_0.Compress() > df_1.Compress() ) { continue; }

            for( Int i = 0; i < Int{3}; ++i )
            {
                // We need to identity vertex i of df_1 with vertex (g.r + i mod 3) of df_0.
                edge_agg.Push( df_1.Vertex(Zmod3{i}).Index(), df_0.Vertex(g.r - Zmod3{i}).Index() );
            }
        }
    }
    
    TOOLS_DUMP(edge_agg.Capacity());
    TOOLS_DUMP(edge_agg.Size());
    
    Graph_T G ( Int{4} * tet_count, std::move(edge_agg) );
    
    auto v = G.VertexComponentVector();
    
    TetVertexContainer_T v_labels( tet_capacity );
    
    Int vertex_counter = 0;
    
    for( Int t = 0; t < tet_capacity; ++t )
    {
        if( !TetActiveQ(t) )
        {
            fill_buffer<4>(v_labels.data(t),Uninitialized);
        }
        else
        {
            for( Int i = 0; i < Int{4}; ++i ) { v_labels(t,i) = v[vertex_counter++]; }
        }
    }
    
    return v_labels;
}


