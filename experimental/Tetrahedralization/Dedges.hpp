public:

struct Dedge_T
{
    Int t = Uninitialized;
    Int e = Uninitialized;
    
    Dedge_T() = default;
    ~Dedge_T() = default;
    
    constexpr Dedge_T( Int t_, Int e_ )
    :   t{t_}, e{e_ & dedge_bit_mask }
    {}
    
    constexpr Dedge_T( Int de_compressed )
    :   t{de_compressed >> lower_bit_count}, e{de_compressed & dedge_bit_mask}
    {}
    
    constexpr Int Compress() { return (t << lower_bit_count) | (e & dedge_bit_mask); }
    
    constexpr Vertex_T Head() const
    {
        return { t, dedge_vertex_lut[static_cast<Size_T>(e)][1] };
    }
    
    constexpr Vertex_T Tail() const
    {
        return { t, dedge_vertex_lut[static_cast<Size_T>(e)][0] };
    }
    
    constexpr Dface_T Dface() const
    {
        return { t, dedge_face_lut[static_cast<Size_T>(e)] };
    }
    
    constexpr Dedge_T Reverse() const
    {
        return { t, dedge_rev_dedge_lut[static_cast<Size_T>(e)] };
    }
    
    constexpr Dedge_T Opposite() const
    {
        return { t, dedge_opp_dedge_lut[static_cast<Size_T>(e)] };
    }
    
    constexpr Z3 UndirectedEdgeIndexInFace() const
    {
        return dedge_mod3[static_cast<Size_T>(e)];
    }
    
    constexpr bool ValidQ() const
    {
        return TetIndexValidQ(t) && ( e <= dedge_bit_mask );
    }
    
    friend std::string ToString( cref<Dedge_T> de )
    {
        std::string s ( "Dedge_T{" );
        s += ".t = ";
        s += ToString(de.t);
        s += ", .e = ";
        s += ToString(de.e);
        s += "}";
        return s;
    }
    
}; // struct Dedge_T


// **INCOMPLETE**
Dedge_T CycleForward( cref<Dedge_T> de_0 ) const
{
    Dface_T df_0 = de_0.Dface();
    
    Gluing_T g = Gluing(df_0);
    
    if( !g.ValidQ() ) { return Dedge_T{}; }
    
    Dface_T df_1 = g.df;
    
//    TOOLS_DUMP(de_0);
//    TOOLS_DUMP(de_0.Tail());
//    TOOLS_DUMP(de_0.Head());
//    TOOLS_DUMP(VertexLabel(de_0.Tail()));
//    TOOLS_DUMP(VertexLabel(de_0.Head()));
    
    // Get local index of de.e as unsigned edge in df_0.
    const Z3 e_0 = de_0.UndirectedEdgeIndexInFace();
    // Get use gluing to ransport that local index to df_1.
    const Z3 e_1 = (Z3{1} - e_0) - g.r; // TODO: or + g.r?
  
    Dedge_T de_1 = df_1.Dedge(e_1).Reverse();
    
//    TOOLS_DUMP(de_1);
//    TOOLS_DUMP(de_1.Tail());
//    TOOLS_DUMP(de_1.Head());
//    TOOLS_DUMP(VertexLabel(de_1.Tail()));
//    TOOLS_DUMP(VertexLabel(de_1.Head()));

    return de_1;
}

TetDedgeContainer_T DedgeLabels() const
{
    TOOLS_PTIMER(timer,MethodName("DedgeLabels"));
    
    std::string tag = "DedgeLabels";
    
    if( !InCacheQ(tag) )
    {
        TetDedgeContainer_T de_labels( tet_capacity, Unvisited );

        Int de_counter = 0;
        
        for( Int t = 0; t < tet_capacity; ++t )
        {
            if( !TetActiveQ(t) )
            {
                for( Int e = 0; e < Int{12}; ++e )
                {
                    de_labels(t,e) = Uninitialized;
                }
                continue;
            }
            
            for( Int e = 0; e < Int{12}; ++e )
            {
                if( de_labels(t,e) != Unvisited ) { continue; }
                
                const Dedge_T de_0 (t,e);
                Dedge_T de = de_0;
                
                // Cycle around dedge de_0 in forward direction.
                do
                {
                    de_labels(de.t,de.e) = de_counter++;
                    const Dedge_T de_rev = de.Reverse();
                    de_labels(de_rev.t,de_rev.e) = de_counter++;
                    de = CycleForward(de);
                }
                while( de.ValidQ() && (de_labels(de.t,de.e) == Unvisited) );
            
                if( de.ValidQ() ) { continue; }
                
//                logprint("Reverse cycling.");
                
                // If we arrive here, then de.Face() is a boundary face. We have to make sure that the incomplete edge cycle is completed.
                
                // Cycle around dedge de_0 in backward direction.
                de = CycleForward(de_0.Reverse());
                while( de.ValidQ() && (de_labels(de.t,de.e) == Unvisited) )
                {
                    de_labels(de.t,de.e) = de_counter++;
                    const Dedge_T de_rev = de.Reverse();
                    de_labels(de_rev.t,de_rev.e) = de_counter++;
                    de = CycleForward(de);
                }
            }
        }
     
        this->SetCache( tag, std::move(de_labels) );
    }
    
    return this->template GetCache<TetDedgeContainer_T>(tag);
}

Int DedgeLabel( cref<Dedge_T> de ) const
{
    return DedgeLabels()(de.t,de.e);
}




//static constexpr ToDedge( const Int t, const Int e )
//{
//    return (t << 3) | (e & Int{7}));
//}
//
//static constexpr std::pair<Int,Int> FromDedge( const Int de )
//{
//    return { de >> 3, de & Int{7} };
//}
//
//static constexpr ReverseDedge( const Int de )
//{
//    return (de & ~Int{7}) | dedge_rev_dege_lut[de & Int{7}];
//}
//
//static constexpr OppositeDedge( const Int de )
//{
//    return (de & ~Int{7}) | dedge_opp_dege_lut[de & Int{7}];
//}
//
//
//static constexpr DedgeToDface( const Int de )
//{
//    auto [t, e] = FromDedge(de);
//    
//    return ToDface( t, dedge_face_lut[e] );
//}
//
