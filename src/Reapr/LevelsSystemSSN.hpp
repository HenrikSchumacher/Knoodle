public:

template<typename I, typename J, typename Int>
Sparse::MatrixCSR<Real,I,J> LevelsSystemMatrix( mref<PlanarDiagram<Int>> pd )
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIC(ClassName()+"::LevelsSystemMatrix<" + TypeName<Int> + ">");
    
    const I n = int_cast<I>(pd.CrossingCount());
    const I m = int_cast<I>(pd.ArcCount());
    
    using Aggregator_T = TripleAggregator<I,I,Real,J>;
    
    Aggregator_T agg;
    
    switch ( en_flag )
    {
        case EnergyFlag_T::Bending:
        {
            agg = Aggregator_T( J(3) * m + J(3) * n );
            BendingHessian_CollectTriples( pd, agg, I(0), I(0) );
            break;
        }
        case EnergyFlag_T::Dirichlet:
        {
            agg = Aggregator_T( J(2) * m + J(3) * n );
            DirichletHessian_CollectTriples( pd, agg, I(0), I(0) );
            break;
        }
        default:
        {
            wprint(ClassName()+"::LevelsSystemMatrix: Unknown or invalid energy flag. Using DirichletHessian to prevent system matrix from being singular." );
            
            agg = Aggregator_T( J(2) * m + J(3) * n );
            DirichletHessian_CollectTriples( pd, agg, I(0), I(0) );
            break;
        }
    }
    
    LevelsConstraintMatrix_CollectTriples<Op::Id>( pd, agg, m, I(0) );
    
    for( I i = m; i < m + n; ++i )
    {
        agg.Push( i, i, Scalar::One<Real> );
    }
    
    Sparse::MatrixCSR<Real,I,J> L ( agg, m+n, m+n, I(1), true, true ); // symmetrize

    TOOLS_PTOC(ClassName()+"::LevelsSystemMatrix<" + TypeName<Int> + ">");
    
    return L;
}

// TODO: Test this!

template<typename I, typename J, typename Int>
void WriteLevelsSystemMatrixModifiedValues(
    mref<PlanarDiagram<Int>> pd,
    cref<Sparse::MatrixCSR<Real,I,J>> L,
    cptr<Real> y,
    mptr<Real> mod_vals
)
{
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIC( ClassName()+"::WriteLevelsSystemMatrixModifiedValues"
       + "<" + TypeName<I>
       + "," + TypeName<J>
       + "," + TypeName<Int>
       + ">"
    );
    
    cptr<J>    outer      = L.Outer().data();
    cptr<Real> fixed_vals = L.Values().data();
    
    const I n = int_cast<I>(pd.CrossingCount());
    const I m = int_cast<I>(pd.ArcCount());
    
    constexpr Real eps = Scalar::eps<Real>;
    
    for( I i = m; i < m + n; ++i )
    {
        const J k_begin = outer[i    ];
        const J k_end   = outer[i + 1];
        
        PD_ASSERT( k_end > k_begin );
        
        Real mask = static_cast<Real>(y[i] + jump >= eps) + eps;
        
        // Modify lower left block in system matrix.
        for( J k = k_begin; k < k_end - 1; ++k )
        {
            mod_vals[k] = mask * fixed_vals[k];
        }
        
        // Modify lower right block in system matrix.
        
        // Caution, this looks weird as we would expect (Real(1)-mask) here.
        // But this _is_ what the semi-smooth Newton algorithm requires!
        mod_vals[k_end - 1] = (mask - Real(1));
    }
    
    TOOLS_PTOC( ClassName()+"::WriteLevelsSystemMatrixModifiedValues"
       + "<" + TypeName<I>
       + "," + TypeName<J>
       + "," + TypeName<Int>
       + ">"
    );
}
