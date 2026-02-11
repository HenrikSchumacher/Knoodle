public:

// Solve the linear-quadratic optimization problem with the semi-smooth Newton method.

Tensor1<Real,Int> LevelsQP_SSN( cref<PD_T> pd )
{
    auto x_mu = LevelsQP_SSN_LevelsAndLagrangeMultipliers(pd);
    
    // We have to read the levels values through pd.LinkComponentArcs().Elements()
    
    cptr<Int> perm = pd.LinkComponentArcs().Elements().data();
    
    if( x_mu.Size() > 0 )
    {
        const Int n = pd.ArcCount();
        
        Tensor1<Real,Int> L (n,Real(0));
        
        for( Int k = 0; k < n; ++k )
        {
            L[perm[k]] = x_mu[k];
        }
                                    
        return L;
    }
    else
    {
        return Tensor1<Real,Int>();
    }
}

// Solve the linear-quadratic optimization problem with the semi-smooth Newton method.
// Returns a vector that contains the levels _and_ the Lagrange multipliers.

Tensor1<Real,Int> LevelsQP_SSN_LevelsAndLagrangeMultipliers(
    cref<PD_T> pd
)
{
    TOOLS_PTIMER(timer,MethodName("LevelsQP_SSN_LevelsAndLagrangeMultipliers"));
    
    const Int m = pd.ArcCount();
    const Int n = pd.CrossingCount();
    
    // x[0,...,m[ is the levels.
    // x[m+1,...,m_n[ is the Lagrange multipliers.
//    Tensor1<Real,Int> x     (m+n);
    Tensor1<Real,Int> x     (m+n,Real(0));
    Tensor1<Real,Int> x_tau (m+n);

    // TODO: Remove initialization.
    
    // Stores the result y = L.x.
    Tensor1<Real,Int> y     (m+n,Real(0));
    
    // Stores the result z = F(x).
    Tensor1<Real,Int> z     (m+n);
    fill_buffer(&z[0], Real(0), m );
    fill_buffer(&z[m], jump,    n );
    
    // Update direction.
    Tensor1<Real,Int> u (m+n);
    
    auto L = this->template LevelsQP_SSN_Matrix<Real,UMF_Int,UMF_Int>(pd);
    auto mod_values = L.Values();
    
    // Symbolic factorization
    UMFPACK<Real,UMF_Int> umfpack(
        L.RowCount(), L.ColCount(), L.Outer().data(), L.Inner().data()
    );
    
    // TODO: Is this a good value for the tolerance?
    const Real threshold = Power(m * settings.settings.tolerance,2);
    
    Size_T SSN_iter = 0;
//    Real max_step_size = 0;

//            L.Dot( Scalar::One<Real>, x, Scalar::Zero<Real>, y );
    // In first iteration both x and y are zero at this point.
    
    Real phi_0 = Real(4) * n;
    
    do
    {
        ++SSN_iter;
        
        LevelsQP_SSN_WriteMatrixModifiedValues(
            pd, L, y.data(), mod_values.data()
        );
        
        // Numeric factorization
        umfpack.NumericFactorization( mod_values.data() );
        
        if( umfpack.NumericStatus() != UMFPACK_OK )
        {
            eprint(MethodName("LevelsQP_SSN_LevelsAndLagrangeMultipliers")+": Aborting because numeric factorization failed.");
            
            return Tensor1<Real,Int>();
        }

        // u = - L^{-1} . z = -DF(x)^{-1} . F(x);
        umfpack.template Solve<Op::Id,Flag_T::Minus,Flag_T::Zero>(
            -Scalar::One<Real>, z.data(), Scalar::Zero<Real>, u.data()
        );
        
        Real phi_tau;
        Real tau        = Frac<Real>(1,backtracking_factor);
        int  SSN_b_iter = 0;
        
        // Armijo line search.
        do{
            ++SSN_b_iter;
            
            tau = settings.backtracking_factor * tau;
            
            // x_tau = x + tau * u;
            combine_buffers3<Flag_T::Plus,Flag_T::Generic>(
                Scalar::One<Real>, x.data(), tau, u.data(), x_tau.data(), m + n
            );
            
            // y = L.x_tau
            L.Dot( Scalar::One<Real>, x_tau.data(), Scalar::Zero<Real>, y.data() );
            
            phi_tau = 0;

            for( Int i = 0; i < m; ++i )
            {
                z[i] = y[i];
                
                phi_tau += z[i] * z[i];
            }
            
            for( Int i = m; i < m + n; ++i )
            {
                z[i] = Ramp( y[i] + jump ) - x_tau[i];
                
                phi_tau += z[i] * z[i];
            }
            
            // Now z = F(x_tau).
        }
        while(
            (phi_tau > (Real(1) - settings.armijo_slope * tau) * phi_0)
            &&
            (SSN_b_iter < settings.SSN_max_b_iter)
        );
        
        if( (phi_tau > (Real(1) - settings.armijo_slope * tau) * phi_0) && (SSN_b_iter >= settings.SSN_max_b_iter) )
        {
            wprint(MethodName("LevelsQP_SSN_LevelsAndLagrangeMultipliers")+": Maximal number of backtrackings reached.");
            
            TOOLS_DDUMP( SSN_iter );
            TOOLS_DDUMP( u.FrobeniusNorm() );
            TOOLS_DDUMP( settings.SSN_b_iter );
            TOOLS_DDUMP( tau );
        }
        
        phi_0 = phi_tau;
        
//        max_step_size = Max( tau, max_step_size );
        
        swap( x, x_tau );
    }
    while( (phi_0 > threshold) && (SSN_iter < settings.SSN_max_iter) );
    
    if( (phi_0 > threshold) && (SSN_iter >= settings.SSN_max_iter))
    {
        wprint(MethodName("LevelsQP_SSN_LevelsAndLagrangeMultipliers")+": Maximal number of iterations reached without reaching the stopping criterion.");
    }
    
    return x;
}

public:

template<typename R = Real, typename I = Int, typename J = Int>
Sparse::MatrixCSR<R,I,J> LevelsQP_SSN_Matrix( cref<PD_T> pd ) const
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIMER(timer,MethodName("LevelsQP_SSN_Matrix")+"<" + TypeName<R> + "," + TypeName<I> + "," + TypeName<J> + ">");
    
    const I n = int_cast<I>(pd.CrossingCount());
    const I m = int_cast<I>(pd.ArcCount());
    
    using Aggregator_T = TripleAggregator<I,I,R,J>;
    
    Aggregator_T agg;
    
    switch ( settings.energy )
    {
        case Energy_T::Bending:
        {
            agg = Aggregator_T( J(3) * m + J(3) * n );
            BendingHessian_CollectTriples( pd, agg, I(0), I(0) );
            break;
        }
        case Energy_T::Dirichlet:
        {
            agg = Aggregator_T( J(2) * m + J(3) * n );
            DirichletHessian_CollectTriples( pd, agg, I(0), I(0) );
            break;
        }
        default:
        {
            wprint(MethodName("LevelsQP_SSN_Matrix")+": Energy flag " + ToString(settings.energy) + " is unknown or invalid for LevelsQP_SSN_Matrix. Returning empty matrix." );
            

            return Sparse::MatrixCSR<R,I,J>();
        }
    }
    
    LevelsConstraintMatrix_CollectTriples<Op::Id>( pd, agg, m, I(0) );
    
    for( I i = m; i < m + n; ++i )
    {
        agg.Push( i, i, Scalar::One<R> );
    }
    
    Sparse::MatrixCSR<R,I,J> L ( agg, m+n, m+n, I(1), true, true ); // symmetrize
    
    return L;
}

// TODO: Test this!

template<typename R = Real, typename I = Int, typename J = Int>
void LevelsQP_SSN_WriteMatrixModifiedValues(
    cref<PD_T> pd,
    mref<Sparse::MatrixCSR<R,I,J>> L,
    cptr<R> y,
    mptr<R> mod_vals
) const
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    static_assert(IntQ<J>,"");
    
    TOOLS_PTIMER(timer,MethodName("LevelsQP_SSN_WriteMatrixModifiedValues")
        + "<" + TypeName<R>
        + "," + TypeName<I>
        + "," + TypeName<J>
        + ">"
    );
    
    cptr<J>    outer      = L.Outer().data();
    cptr<Real> fixed_vals = L.Values().data();
    
    const I n = int_cast<I>(pd.CrossingCount());
    const I m = int_cast<I>(pd.ArcCount());
    
    constexpr R eps = Scalar::eps<R>;
    
    for( I i = m; i < m + n; ++i )
    {
        const J k_begin = outer[i    ];
        const J k_end   = outer[i + 1];
        
        PD_ASSERT( k_end > k_begin );
        
        R mask = static_cast<R>(y[i] + jump >= eps) + eps;
        
        // Modify lower left block in system matrix.
        for( J k = k_begin; k < k_end - 1; ++k )
        {
            mod_vals[k] = mask * fixed_vals[k];
        }
        
        // Modify lower right block in system matrix.
        
        // Caution, this looks weird as we would expect (R(1)-mask) here.
        // But this _is_ what the semi-smooth Newton algorithm requires!
        mod_vals[k_end - 1] = (mask - R(1));
    }
}

