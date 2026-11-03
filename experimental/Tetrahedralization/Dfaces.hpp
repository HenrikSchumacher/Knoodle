public:

Int DfaceCount() const { return Int{4} * tet_count; }

struct Dface_T
{
    Int t = Uninitialized;
    Int f = Uninitialized;
    
    Dface_T() = default;
    ~Dface_T() = default;
    
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
    
    constexpr Vertex_T Vertex( const Z3 r ) const
    {
        return { t, face_vertex_lut[static_cast<Size_T>(f)][static_cast<Size_T>(r)] };
    }
    
//    constexpr Dedge_T Dedge( IntQ auto i ) const
//    {
//        return { t, Int{3} * f + static_cast<Int>(i) };
//    }
    
    constexpr Dedge_T Dedge( Z3 r ) const
    {
        return { t, Int{3} * f + static_cast<Int>(r) };
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



// The computes the boundary vertex triples, using the VertexLabels().
// This is not really useful as one would want a gluing-based representation of the boundary, not by vertex triples.
// But it helps a little to test the other code.
Tensor2<Int,Int> BoundaryTriangles() const
{
    TOOLS_PTIMER(timer,MethodName("BoundaryTriangles"));
    
    std::string tag = "BoundaryTriangles";
    
    if( !InCacheQ(tag) )
    {
        using Face_T = std::array<Int,3>;
        
        auto v_labels = VertexLabels();
        mptr<Int> v = v_labels.data();
        
        std::vector<Face_T> agg;
        agg.reserve(tet_count);
        
        for( Int t_0 = 0; t_0 < tet_capacity; ++t_0 )
        {
            if( !TetActiveQ(t_0) ) { continue; }
            
            for( Int f_0 = 0; f_0 < Int{4}; ++f_0 )
            {
                const Dface_T df_0 { t_0, f_0 };
                
                const Gluing_T g = Gluing(df_0);
                
                if( g.ValidQ() ) { continue; }
                
                agg.push_back( Face_T{ v[df_0.Vertex(Z3{0}).Index()], v[df_0.Vertex(Z3{1}).Index()], v[df_0.Vertex(Z3{2}).Index()] });
            }
        }
        
        this->SetCache( tag, std::make_any<Tensor2<Int,Int>>( &agg[Size_T(0)][Size_T(0)], agg.size(), Size_T{3} ) );
    }
    
    return this->template GetCache<Tensor2<Int,Int>>(tag);
}
