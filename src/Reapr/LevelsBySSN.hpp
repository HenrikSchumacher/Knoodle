public:

// Solve the linear-quadratic optimization problem with the semi-smooth Newton method.

template<typename Int>
Tensor1<Real,Int> LevelsBySSN( mref<PlanarDiagram<Int>> pd )
{
    auto x_mu = this->LevelsAndLagrangeMultipliersBySSN(pd);
    
    if( x_mu.Size() > 0 )
    {
        return Tensor1<Real,Int>(x_mu.data(),pd.ArcCount());
    }
    else
    {
        Tensor1<Real,Int>();
    }
}

// Solve the linear-quadratic optimization problem with the semi-smooth Newton method.
// Returns a vector that contains the levels _and_ the Lagrange multipliers.

template<typename Int>
Tensor1<Real,Int> LevelsAndLagrangeMultipliersBySSN( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_PTIMER(timer, ClassName() + "::LevelsAndLagrangeMultipliersBySSN<" + TypeName<Int> + ">");
//    TOOLS_PTIC(ClassName() + "::LevelsAndLagrangeMultipliersBySSN<" + TypeName<Int> + ">");
    
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
    Tensor1<Real,Int> u     (m+n);
    
    auto L = this->template LevelsSystemMatrix<UMF_Int,UMF_Int>(pd);
    auto mod_values = L.Values();
    
    // Symbolic factorization
    UMFPACK<Real,UMF_Int> umfpack(
        L.RowCount(), L.ColCount(), L.Outer().data(), L.Inner().data()
    );
    
    // TODO: Is this a good value for the tolerance?
    const Real threshold = Power(m * tolerance,2);
    
    iter = 0;
//    Real max_step_size = 0;

//            L.Dot( Scalar::One<Real>, x, Scalar::Zero<Real>, y );
    // In first iteration both x and y are zero at this point.
    
    Real phi_0 = Real(4) * n;
    
    do
    {
        ++iter;
        
        WriteLevelsSystemMatrixModifiedValues(
            pd, L, y.data(), mod_values.data()
        );
        
        // Numeric factorization
        umfpack.NumericFactorization( mod_values.data() );
        
        if( umfpack.NumericStatus() != UMFPACK_OK )
        {
            eprint(ClassName() + "::LevelsAndLagrangeMultipliersBySSN: Aborting because numeric factorization failed.");
            
            return Tensor1<Real,Int>();
        }

        // u = - L^{-1} . z = -DF(x)^{-1} . F(x);
        umfpack.Solve<Op::Id,Flag_T::Minus,Flag_T::Zero>(
            -Scalar::One<Real>, z.data(), Scalar::Zero<Real>, u.data()
        );
        
        Real phi_tau;
        Real tau    = initial_time_step;
        int  b_iter = 0;
        
        // Armijo line search.
        do{
            ++b_iter;
            
            tau = backtracking_factor * tau;
            
            // x_tau = x + tau * u;
            combine_buffers3<Flag_T::Plus,Flag_T::Generic>(
                Scalar::One<Real>, x.data(), tau, u.data(), x_tau.data(), m + n
            );
            
            // y = L.x_tau
            L.Dot( Scalar::One<Real>, x_tau, Scalar::Zero<Real>, y );
            
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
            (phi_tau > (Real(1) - armijo_slope * tau) * phi_0)
            &&
            (b_iter < max_b_iter)
        );
        
        if( (phi_tau > (Real(1) - armijo_slope * tau) * phi_0) && (b_iter >= max_b_iter) )
        {
            wprint(ClassName() + "::LevelsAndLagrangeMultipliersBySSN<" + TypeName<Int> + ">" + ": Maximal number of backtrackings reached.");
            
            TOOLS_DUMP( iter );
            TOOLS_DUMP( u.FrobeniusNorm() );
            TOOLS_DUMP( b_iter );
            TOOLS_DUMP( tau );
        }
        
        phi_0 = phi_tau;
        
//        max_step_size = Max( tau, max_step_size );
        
        swap( x, x_tau );
    }
    while( (phi_0 > threshold) && (iter < max_iter) );
    
    if( (phi_0 > threshold) && (iter >= max_iter))
    {
        wprint(ClassName() + "::LevelsAndLagrangeMultipliersBySSN<" + TypeName<Int> + ">" + ": Maximal number of iterations reached without reaching the stopping criterion.");
    }
    
//    TOOLS_PTOC(ClassName() + "::LevelsAndLagrangeMultipliersBySSN<" + TypeName<Int> + ">");
    
    return x;
}
