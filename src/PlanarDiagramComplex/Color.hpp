public:

ColorCounts_T ColorArcCounts() const
{
    ColorCounts_T color_arc_counts;
    
    for( const PD_T & pd : pd_list )
    {
        if( pd.ValidQ() )
        {
            const ColorCounts_T & pd_color_arc_counts = pd.ColorArcCounts();
            
            for( const auto & x : pd_color_arc_counts )
            {
                if( color_arc_counts.contains(x.first) )
                {
                    color_arc_counts[x.first] += x.second;
                }
                else
                {
                    color_arc_counts[x.first]  = x.second;
                }
            }
        }
    }
    
    return color_arc_counts;
}

Int ColorCount() const
{
    return int_cast<int>(ColorArcCounts().size());
}

Int MaxColor() const
{
    Int max_color = 0;
    
    for( const PD_T & pd : pd_list )
    {
        max_color = Max(max_color,pd.MaxColor());
    }
    
    return max_color;
}


MatrixTripleContainer_T<Int,ToSigned<Int>> ColorIntersectionCounts() const
{
    TOOLS_PTIMER(timer,MethodName("ColorIntersectionCounts"));
    
    using I = ToSigned<Int>;
    
    MatrixTripleContainer_T<Int,I> lut;
    
    for( const PD_T & pd : pd_list )
    {
        if( pd.ValidQ() )
        {
            for( Int c = 0; c < pd.max_crossing_count; ++ c )
            {
                if( pd.CrossingActiveQ(c) )
                {
                    pd.template AssertCrossing<1>(c);
                    
                    const Int a = pd.C_arcs(c,Out,Left );
                    const Int b = pd.C_arcs(c,Out,Right);
                    
                    auto [i,j] = MinMax(pd.A_color[a],pd.A_color[b]);
                    
                    Increment( lut, {i,j} );
                }
            }
        }
    }
    return lut;
}

Sparse::MatrixCSR<ToSigned<Int>,Int,Int> ColorIntersectionMatrix( Int thread_count = 1 ) const
{
    TOOLS_PTIMER(timer,MethodName("ColorIntersectionMatrix"));
    
    using I = ToSigned<Int>;
    using Matrix_T = Sparse::MatrixCSR<ToSigned<Int>,Int,Int>;
    
    if( !this->InCacheQ("ColorIntersectionMatrix") )
    {
        auto lut = ColorIntersectionCounts();
        
        if( lut.empty() )
        {
            Matrix_T();
        }
        
        Tensor1<Int,Int> i ( lut.size() );
        Tensor1<Int,Int> j ( lut.size() );
        Tensor1<I  ,Int> a ( lut.size() );
        
        Int k = 0;
        for( auto & x : lut )
        {
            i[k] = x.first.i;
            j[k] = x.first.j;
            a[k] = x.second;
            ++k;
        }
        
        Int n = MaxColor() + Int(1);
        
        this->SetCache(
            "ColorIntersectionMatrix",
            Matrix_T(
                i.Size(), i.data(), j.data(), a.data(), n, n,
                thread_count, false, true, false
            )
        );
    }
    
    return this->template GetCache<Matrix_T>("ColorIntersectionMatrix");
}
