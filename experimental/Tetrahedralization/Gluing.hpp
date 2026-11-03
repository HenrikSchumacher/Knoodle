public:

struct Gluing_T
{   
    Dface_T df = Uninitialized;
    Z3   r  = 3;
    
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
