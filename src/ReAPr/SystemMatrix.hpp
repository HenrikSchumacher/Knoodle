public:

mref<Matrix_T> SystemMatrixPattern( mref<PlanarDiagram_T> pd )
{
    const std::string tag ("SystemMatrixPattern");
    
    if( !pd.InCacheQ(tag) )
    {
        const I_T n = pd.CrossingCount();
        const I_T m = pd.ArcCount();
        
        Aggregator_T agg ( 5 * m + 5 * n );
        
        HessianMatrix_Triples( pd, agg, 0, 0 );
        
//        ConstraintMatrix_Triples<Op::Trans>( pd, agg, 0, m );
        
        ConstraintMatrix_Triples<Op::Id   >( pd, agg, m, 0 );
        
        for( I_T i = m; i < m + n; ++i )
        {
            agg.Push( i, i, Scalar::One<Real> );
        }
        
        agg.Finalize();
        
        Matrix_T L ( agg, m+n, m+n, I_T(1), true, true );
        
//        L.SortInner();
        
        pd.SetCache( tag, std::move(L) );
    }

    return pd.template GetCache<Matrix_T>(tag);
}


mref<Values_T> SystemMatrixValues(
    mref<PlanarDiagram_T> pd
)
{
    const std::string tag ("SystemMatrixValues");
    
    if( !pd.InCacheQ(tag) )
    {
        Tensor1<Real,I_T> vals ( SystemMatrixPattern(pd).Values() );
        
        pd.SetCache( tag, std::move(vals) );
    }
    
    return pd.template GetCache<Values_T>(tag);
}

// TODO: Test this!
mref<Values_T> SystemMatrixValues(
    mref<PlanarDiagram_T> pd, cptr<Real> y
)
{
    mref<Values_T> vals = SystemMatrixValues( pd );
    
    mptr<Real> v = vals.data();
    
    auto & L = SystemMatrixPattern(pd);
    
    cptr<I_T>  outer        = L.Outer().data();
    cptr<Real> fixed_values = L.Values().data();
    
    const I_T n = pd.CrossingCount();
    const I_T m = pd.ArcCount();
    
    constexpr Real eps = Scalar::eps<Real>;
    
    for( I_T i = m; i < m + n; ++i )
    {
        const I_T k_begin = outer[i    ];
        const I_T k_end   = outer[i + 1];
        
        PD_ASSERT( k_end > k_begin );
        
        Real mask = static_cast<Real>(y[i] + 2 >= eps) + eps;
        
        for( I_T k = k_begin; k < k_end - 1; ++k )
        {
            v[k] = mask * fixed_values[k];
        }
        
        // Caution, this looks weird as we would expect (Real(1)-mask) here.
        // But this _is_ what the semi-smooth Newton algorithm requires!
        v[k_end - 1] = (mask - Real(1));
    }
    
    return vals;
}
