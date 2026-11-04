public:

struct Gluing_T
{   
    Dface_T df = Uninitialized;
    Z3      r  = 0;
    
    constexpr Gluing_T( Int t_, Int f_, Int r_ )
    :   df {t_,f_}, r {r_}
    {}
    
    constexpr Gluing_T( cref<Dface_T> df_, Z3 r_ )
    :   df {df_}, r {r_}
    {}
    
    constexpr Gluing_T( Int g )
    :   df { g }
    ,   r  { (g >> face_bit_count) & rotation_bit_mask }
    {}
    
    constexpr Int Compress() { return df.Compress() | ((Int{r} & rotation_bit_mask) << face_bit_count); }
  
    constexpr bool ValidQ() const
    {
        return df.ValidQ();
    }
    
    friend std::string ToString( cref<Gluing_T> g )
    {
        std::string s ( "Gluing_T{" );
        s += ToString(g.df);
        s += ", ";
        s += ToString(g.r);
        s += "}";
        return s;
    }
    
//    static constexpr bool ValidQ( Int g )
//    {
//        if constexpr ( SignedIntQ<Int> )
//        {
//            return g >= Int{0};
//        }
//        else
//        {
//            return ( g != Uninitialized ) && ((g & Int{3}) != Int{3});
//        }
//    }
    
}; // struct Gluing_T



//static constexpr Int ToGlue( const Int t, const Int f, const Int r )
//{
//    return (t << 4) | ((f & Int{3}) << 2) | (r & Int{3});
//}
//
//static constexpr std::tuple<Int,Int,Int> FromGlue( const Int g )
//{
//    return { (g >> 4), (g >> 2) & Int{3}, g & Int{3} };
//}
//
//static bool ValidGlueQ ( const Int g )
//{
//    if constexpr ( SignedIntQ<Int> )
//    {
//        return g >= Int{0};
//    }
//    else
//    {
//        return ( g != Uninitialized ) && ((g & Int{3}) != Int{3});
//    }
//}

/*!@brief **EXPERIMENTAL** Attempts to translate TetGluings into the language of SnapPea.
 *
 * The result is a 3-tensor of size `TetCapacity() x 4 x 5`. For each tet indes `t` and face index `f` (in {0,1,2,3}), the vector  v = SnapPeaGluings()(t,f) is a 5-vector with the following meaning:
 * Face number f in tet t is glued to tet v[0] such that the vertices {0,1,2,3} of tet t are glued to vertices v[1],v[2],v[3],v[4] of tet v[0], respectively.
 */
Tensor3<Int,Int> SnapPeaGluings() const
{
    Tensor3<Int,Int> result ( tet_capacity, Int{4}, Int{5} );
    
    for( Int t = 0; t < tet_capacity; ++t )
    {
        if( !TetActiveQ(t) )
        {
            fill_buffer<5>( result.data(t), Uninitialized );
            continue;
        }
            
        for( Int f = 0; f < Int{4}; ++f )
        {
            Dface_T df_0 (t,f);
            
            Gluing_T g = Gluing(df_0);
            
            if( !g.ValidQ() )
            {
                fill_buffer<5>( result.data(t,f), Uninitialized );
                continue;
            }
            
            const Dface_T df_1 = g.df;
            const Z3 r = g.r;
            
            result(t,f,Int{0}) = df_1.t;
            result(t,f,df_0.Opposite().v    + Int{1}) = df_1.Opposite().v;
            result(t,f,df_0.Vertex(Z3{0}).v + Int{1}) = df_1.Vertex(Z3{0}-r).v;
            result(t,f,df_0.Vertex(Z3{1}).v + Int{1}) = df_1.Vertex(Z3{2}-r).v;
            result(t,f,df_0.Vertex(Z3{2}).v + Int{1}) = df_1.Vertex(Z3{1}-r).v;
        }
    }
    
    return result;
}
