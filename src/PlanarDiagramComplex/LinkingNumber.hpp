public:

MatrixTripleContainer_T<Int,ToSigned<Int>> LinkingNumbers() const
{
    TOOLS_PTIMER(timer,MethodName("LinkingNumbers"));
    
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
                    
                    if( i != j )
                    {
                        const I val = static_cast<I>(ToUnderlying(pd.C_state[c]));
                        AddTo( lut, {i,j}, val );
                    }
                }
            }
        }
    }
    
    for( auto & x : lut ) { x.second /= I(2); }
    
    return lut;
}


Sparse::MatrixCSR<ToSigned<Int>,Int,Int> LinkingMatrix( Int thread_count = 1 ) const
{
    TOOLS_PTIMER(timer,MethodName("LinkingMatrix"));
    
    using I = ToSigned<Int>;
    using Matrix_T = Sparse::MatrixCSR<ToSigned<Int>,Int,Int>;
    
    if( !this->InCacheQ("LinkingMatrix") )
    {
        auto lut = LinkingNumbers();
        
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
            "LinkingMatrix",
            Matrix_T(
                i.Size(), i.data(), j.data(), a.data(), n, n,
                thread_count, false, true, false
            )
        );
    }
    
    return this->template GetCache<Matrix_T>("LinkingMatrix");
}
