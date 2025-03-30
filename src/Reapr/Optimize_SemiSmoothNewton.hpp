public:

Tensor1<Real,I_T> Optimize_SemiSmoothNewton( mref<PlanarDiagram_T> pd )
{
    const I_T m = pd.ArcCount();
    const I_T n = pd.CrossingCount();
    
    // x[0,...,m[ is the levels.
    // x[m+1,...,m_n[ is the Lagrange multipliers.
//    Tensor1<Real,I_T> x     (m+n);
    Tensor1<Real,I_T> x     (m+n,Real(0));
    Tensor1<Real,I_T> x_tau (m+n);
    
//    WriteReaprFeasibleLevels( pd, x.data() );
//    zerofy_buffer( x.data(m), n );

    // TODO: Remove initialization.
    
    // Stores the result y = L.x.
    Tensor1<Real,I_T> y     (m+n,Real(0));
    
    // Stores the result z = F(x).
    Tensor1<Real,I_T> z     (m+n,Real(0));
    fill_buffer(&z[m], jump, n );
    
    // Update direction.
    Tensor1<Real,I_T> u     (m+n);
    
    Matrix_T L = SystemMatrix(pd);
    Values_T mod_values = L.Values();
    
    // Symbolic factorization
    UMFPACK<Real,I_T> umfpack(
        L.RowCount(), L.ColCount(), L.Outer().data(), L.Inner().data()
    );

    // TODO: Is this a good value for the tolerance?
    const Real threshold = Power(m * tolerance,2);
    
    Int  iter          = 0;
    Real max_step_size = 0;

//            L.Dot( Scalar::One<Real>, x, Scalar::Zero<Real>, y );
    // In first iteration both x and y are zero at this point.
    
    Real phi_0 = Real(4) * n;
    
    do
    {
        ++iter;
        
        WriteSystemMatrixModifiedValues( pd, L, y.data(), mod_values.data() );
        
//        TOOLS_DUMP(mod_values);
        
        // Numeric factorization
        umfpack.NumericFactorization( mod_values.data() );

        // u = - L^{-1} . z = -DF(x)^{-1} . F(x);
        umfpack.Solve<Op::Id,Flag_T::Minus,Flag_T::Zero>(
            -Scalar::One<Real>, z.data(), Scalar::Zero<Real>, u.data()
        );
        
        Real phi_tau;
        Real tau    = initial_time_step;
        Int  b_iter = 0;
        
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

            for( I_T i = 0; i < m; ++i )
            {
                z[i] = y[i];
                
                phi_tau += z[i] * z[i];
            }
            
            for( I_T i = m; i < m + n; ++i )
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
            wprint(ClassName() + "::OptimizeSemiSmoothNewton: Maximal number of backtrackings reached.");
            
            TOOLS_DUMP( iter );
            TOOLS_DUMP( u.FrobeniusNorm() );
            TOOLS_DUMP( b_iter );
            TOOLS_DUMP( tau );
        }
        
        phi_0 = phi_tau;
        
        max_step_size = Max( tau, max_step_size );
        
        swap( x, x_tau );
    }
    while(
        (phi_0 > threshold) && (iter < max_iter)
    );
    
    if( (phi_0 > threshold) && (iter >= max_iter))
    {
        wprint(ClassName() + "::OptimizeSemiSmoothNewton: Maximal number of iterations reached without reaching the stopping criterion/");
    }
    
    return x;
}
