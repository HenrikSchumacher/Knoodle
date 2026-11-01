public:

struct Dedge_T
{
    Int t = Uninitialized;
    Int e = Uninitialized;
    
    constexpr Dedge_T( Int t_, Int e_ )
    :   t{t_}, e{e_ & dedge_bit_mask }
    {}
    
    constexpr Dedge_T( Int de_compressed )
    :   t{de_compressed >> lower_bit_count}, e{de_compressed & dedge_bit_mask}
    {}
    
    constexpr Int Compress() { return (t << lower_bit_count) | (e & dedge_bit_mask); }
    
    constexpr Vertex_T Head() const
    {
        return { t, dedge_vertex_lut[e][1] };
    }
    
    constexpr Vertex_T Tail() const
    {
        return { t, dedge_vertex_lut[e][0] };
    }
    
    constexpr Dface_T Dface() const
    {
        return { t, dedge_face_lut[e] };
    }
    
    constexpr Dedge_T Reverse() const
    {
        return { t, dedge_rev_dedge_lut[e] };
    }
    
    constexpr Dedge_T Opposite()
    {
        return { t, dedge_opp_dedge_lut[e] };
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

Dedge_T CycleForward( cref<Dedge_T> de )
{
    Dface_T df_0 = de.Reverse().Dface();
    
    Gluing_T g = Glueing(df_0);
    
    Dface_T df_1 = g.df;
    
    // TODO: Find out the local index e_1.
    const Int e_1 = 0;
    
    return Dedge_T{df_1.t,e_1};
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
