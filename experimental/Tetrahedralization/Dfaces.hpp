public:

Int DfaceCount() const { return Int{4} * tet_count; }

struct Dface_T
{
    Int t = Uninitialized;
    Int f = Uninitialized;
    
    constexpr Dface_T( Int t_, Int f_ )
    :   t{t_}, f{f_}
    {}
    
    constexpr Dface_T( IntQ auto df_comp )
    :   t{static_cast<Int>(df_comp) >> lower_bit_count}, f{static_cast<Int>(df_comp) & face_bit_mask}
    {}
    
    constexpr Int Compress() const { return (t << lower_bit_count) | (f & face_bit_mask); }
    
//    constexpr Vertex_T Vertex( IntQ auto i ) const
//    {
//        return { t, face_vertex_lut[static_cast<Size_T>(f)][static_cast<Size_T>(i)] };
//    }
    
    constexpr Vertex_T Vertex( const Zmod3 r ) const
    {
        return { t, face_vertex_lut[static_cast<Size_T>(f)][static_cast<Size_T>(r)] };
    }
    
    constexpr Dedge_T Dedge( IntQ auto i ) const
    {
        return { t, Int{3} * f + static_cast<Int>(i) };
    }
    
    constexpr Vertex_T Opposite() const
    {
        return Vertex_T{ t, f };
    }
    
    constexpr Int Tet() const
    {
        return t;
    }
    
    constexpr bool ValidQ() const
    {
        return TetIndexValidQ(t);
    }
    
    friend std::string ToString( cref<Dface_T> df )
    {
        std::string s ( "Dface_T{" );
//        s += ".t = ";
        s += ToString(df.t);
//        s += ", .f = ";
        s += ", ";
        s += ToString(df.f);
        s += "}";
        return s;
    }
    
}; // struct Dface_T


Gluing_T Gluing( cref<Dface_T> df ) const
{
    return Gluing_T{ tet_glue(df.t,df.f) };
}


Dface_T Flip( cref<Dface_T> df ) const
{
    if( df.t == Uninitialized ) { return { Uninitialized, Uninitialized }; }

    Gluing_T g = Gluing(df);

    if( g.InvalidQ() ) { return { df.t, Uninitialized }; }

    return g.df;
}






//constexpr static Int ToDface( const Int t, const Int f )
//{
//    return (t << 2) | (f & Int{3});
//}
//
//constexpr static Int ToDface( cref<std::pair<Int,Int>> df )
//{
//    return (df.first << 2) | (df.second & Int{3});
//}
//
//constexpr static std::pair<Int,Int> FromDface( const Int df )
//{
//    return { (df >> 2), df & Int{3} };
//}

//std::pair<Int,Int> FlipDface( const Int t, const Int f )
//{
//    if( t == Uninitialized ) { return { Uninitialized, Uninitialized }; }
//
//    const Int g = tet_glue(t,f);
//
//    if( g == Uninitialized ) { return { t, Uninitialized }; }
//
//    auto [t_,f_,r_] = FromGlue(g);
//
//    return { t_, f_ };
//}
//
//
//std::pair<Int,Int> FlipDface( cref<std::pair<Int,Int>> df )
//{
//    return FlipDface(df.first, df.second);
//}
