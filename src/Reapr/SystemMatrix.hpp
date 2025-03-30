public:

Matrix_T SystemMatrix( mref<PlanarDiagram_T> pd )
{
    const I_T n = pd.CrossingCount();
    const I_T m = pd.ArcCount();
    
    Aggregator_T agg;
    
    switch ( en_flag )
    {
        case EnergyFlag_T::Bending:
        {
            agg = Aggregator_T( 3 * m + 3 * n );
            BendingHessian_CollectTriples( pd, agg, 0, 0 );
            break;
        }
        case EnergyFlag_T::Dirichlet:
        {
            agg = Aggregator_T( 2 * m + 3 * n );
            DirichletHessian_CollectTriples( pd, agg, 0, 0 );
            break;
        }
    }
    
    ConstraintMatrix_CollectTriples<Op::Id>( pd, agg, m, 0 );
    
    for( I_T i = m; i < m + n; ++i )
    {
        agg.Push( i, i, Scalar::One<Real> );
    }
    
    Matrix_T L ( agg, m+n, m+n, I_T(1), true, true ); // symmetrize

    return L;
}

// TODO: Test this!
void WriteSystemMatrixModifiedValues(
    mref<PlanarDiagram_T> pd, cref<Matrix_T> L, cptr<Real> y, mptr<Real> mod_vals
)
{
    cptr<I_T>  outer      = L.Outer().data();
    cptr<Real> fixed_vals = L.Values().data();
    
    const I_T n = pd.CrossingCount();
    const I_T m = pd.ArcCount();
    
    constexpr Real eps = Scalar::eps<Real>;
    
    for( I_T i = m; i < m + n; ++i )
    {
        const I_T k_begin = outer[i    ];
        const I_T k_end   = outer[i + 1];
        
        PD_ASSERT( k_end > k_begin );
        
        Real mask = static_cast<Real>(y[i] + jump >= eps) + eps;
        
        // Modify lower left block in system matrix.
        for( I_T k = k_begin; k < k_end - 1; ++k )
        {
            mod_vals[k] = mask * fixed_vals[k];
        }
        
        // Modify lower right block in system matrix.
        
        // Caution, this looks weird as we would expect (Real(1)-mask) here.
        // But this _is_ what the semi-smooth Newton algorithm requires!
        mod_vals[k_end - 1] = (mask - Real(1));
    }
}


