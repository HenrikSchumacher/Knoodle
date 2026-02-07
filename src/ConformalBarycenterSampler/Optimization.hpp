private:


    /*!
     * @brief Runs Newton-like algorithm to find the conformal closure of the open polygon (loaded with ReadInitialEdgeVectors or generated with ReadInitialEdgeVectors). Afterwards, the resulting closed polygon can be accessed with routines *EdgeVectors. The conformal barycenter can be accessed with *ShiftVector.
     */

    void Optimize()
    {
        const Int max_iter = Settings().max_iter;
        
        iter = 0;
        
        Shift();
        
        DifferentialAndHessian_Hyperbolic();
        
        SearchDirection_Hyperbolic();
        
        // The main loop.
        while( ( iter < max_iter ) && continueQ )
        {
            ++iter;
            
            LineSearch_Hyperbolic_Potential();
            
            DifferentialAndHessian_Hyperbolic();
            
            SearchDirection_Hyperbolic();
        }
    }
    
    Real Potential()
    {
        const Real zz = Dot(z_,z_);
        
        const Real a = big_one + zz;
        const Real c = (big_one-zz);
        
        const Real b = one/c;
        
        Real value = 0;
        
        for( Int i = 0; i < edge_count_; ++i )
        {
            Vector_T y_i (y_,i);
            
            value += r_[i] * std::log( std::abs( (a - two * Dot(y_i,z_) ) * b ) );
        }
        
        return value * total_r_inv;
    }
    
    
    void LineSearch_Hyperbolic_Residual()
    {
        // 2 F(0)^T.DF(0).u is the derivative of w\mapsto F(w)^T.F(w) at w = 0.
        const Real slope = two * DF_.InnerProduct(F_,u_);
        
        Real tau = one;
        
        const Real u_norm = u_.Norm();
        
        // exponential map shooting from 0 to tau * u_.
        Times( tau * tanhc(tau * u_norm), u_, z_ );

        // Shift the point z_ along -w to get new updated point w
        InverseShift();
        
        // Shift the input measure along w to 0 to simplify gradient, Hessian, and update computation .
        Shift();
        
        const Real squared_residual_at_0 = squared_residual;
        
        DifferentialAndHessian_Hyperbolic();
        
        if( linesearchQ )
        {
            Int backtrackings = 0;
            
            // Armijo condition for squared residual.
            
            ArmijoQ = squared_residual - squared_residual_at_0 - Settings().Armijo_slope_factor * tau * slope < zero;
            
            while( !ArmijoQ && (backtrackings < Settings().max_backtrackings) )
            {
                ++backtrackings;
                
                // Estimate step size from quadratic fit if applicable.
                
                const Real tau_1 = Settings().Armijo_shrink_factor * tau;
                const Real tau_2 = - half * Settings().Armijo_slope_factor * tau * tau * slope / ( squared_residual  - squared_residual_at_0 - tau * slope );
                
                tau = std::max( tau_1, tau_2 );
                
                Times( tau * tanhc(tau * u_norm), u_, z_ );
                
                // Shift the point z_ along -w_ to get new updated point w_.
                InverseShift();
                
                // Shift the input measure along w_ to 0 to simplify gradient, Hessian, and update computation .
                Shift();
                
                DifferentialAndHessian_Hyperbolic();
                
                ArmijoQ = squared_residual - squared_residual_at_0 - Settings().Armijo_slope_factor * tau * slope < 0;
            }
        }
    }
    
    void LineSearch_Hyperbolic_Potential()
    {
        Real tau = one;
        
        const Real u_norm = u_.Norm();
        
        // exponential map shooting from 0 to tau * u_.
        
        Times( tau * tanhc(tau * u_norm), u_, z_ );
        
        if( linesearchQ )
        {
            //Linesearch with potential as merit function.
            
            const Real gamma = Settings().Armijo_shrink_factor;
            
            const Real sigma = Settings().Armijo_slope_factor;
            
            const Real Dphi_0 = g_factor * Dot(F_,u_);
            
            Int backtrackings = 0;
            
            // Compute potential and check Armijo condition.
            
            // const Real phi_0 = 0;
            
            Real phi_tau = Potential();
            
            ArmijoQ = phi_tau /*- phi_0*/ - sigma * tau * Dphi_0 < 0;
            
            
            while( !ArmijoQ && (backtrackings < Settings().max_backtrackings) )
            {
                ++backtrackings;
                
                const Real tau_1 = gamma * tau;
                
                // Estimate step size from quadratic fit if applicable.
                const Real tau_2 = - half * sigma * tau * tau * Dphi_0 / ( phi_tau /*- phi_0*/ - tau * Dphi_0 );
                
                tau = std::max( tau_1, tau_2 );
                
                Times( tau * tanhc(tau * u_norm), u_, z_ );
                
                phi_tau = Potential();
                
                ArmijoQ = phi_tau  /*- phi_0*/ - sigma * tau * Dphi_0 < 0;
            }
        }
        
        // Shift the point z_ along -w_ to get new updated point w_.
        InverseShift();
        
        // Shift the input measure along w to 0 to simplify gradient, Hessian, and update computation .
        Shift();
    }
    
    void DifferentialAndHessian_Hyperbolic()
    {
        // CAUTION: We use a different sign convention as in the paper!
        // Assemble  F = -1/2 y * r.
        // Assemble DF_ = nabla F + regulatization:
        // DF_{jk} = \delta_{jk} - \sum_i x_{i,j} x_{i,k} r_i.
        
        if constexpr ( zerofy_firstQ )
        {
            F_.SetZero();
            DF_.SetZero();

            for( Int i = 0; i < edge_count_; ++i )
            {
                Vector_T y_i ( y_, i );
                
                const Real r_i = r_[i];

                for( Int j = 0; j < AmbDim; ++j )
                {
                    const Real factor = r_i * y_i[j];

                    F_[j] -= factor;

                    for( Int k = j; k < AmbDim; ++k )
                    {
                        DF_[j][k] -= factor * y_i[k];
                    }
                }
            }
            
        }
        else
        {
            // Filling F_ and DF_ with first summand...
            {
                Vector_T y_i ( y_, 0 );
                
                const Real r_i = r_[0];
                
                for( Int j = 0; j < AmbDim; ++j )
                {
                    const Real factor = r_i * y_i[j];
                    
                    F_[j] = - factor;
                    
                    for( Int k = j; k < AmbDim; ++k )
                    {
                        DF_[j][k] = - factor * y_i[k];
                    }
                }
            }
            
            // ... and adding-in the other summands.
            for( Int i = 1; i < edge_count_; ++i )
            {
                Vector_T y_i ( y_, i );
                
                const Real r_i = r_[i];
                
                for( Int j = 0; j < AmbDim; ++j )
                {
                    const Real factor = r_i * y_i[j];
                    
                    F_[j] -= factor;
                    
                    for( Int k = j; k < AmbDim; ++k )
                    {
                        DF_[j][k] -= factor * y_i[k];
                    }
                }
            }
        }
        
        // Normalize for case that the weights in r do not sum to 1.
        
        F_  *= total_r_inv;
        DF_ *= total_r_inv;
        
        squared_residual = Dot(F_,F_);
        
        residual = std::sqrt( squared_residual );
        
        F_ *= half;
        
        // Better add the identity afterwards for precision reasons.
        for( Int j = 0; j < AmbDim; ++j )
        {
            DF_[j][j] += one;
        }
    }
    
    void SearchDirection_Hyperbolic()
    {
        // Make decisions whether to continue.
        if( residual < static_cast<Real>(100.) * Settings().tolerance )
        {
            // We have to compute eigenvalue _before_ we add the regularization.
            
            lambda_min = DF_.SmallestEigenvalue();
            
            q_Newton = four * residual / (lambda_min * lambda_min);
            
            if( q_Newton < one )
            {
                //Kantorovich condition satisfied; this allows to compute an error estimator.
                errorestimator = half * lambda_min * q_Newton;
                //And we should deactivate line search. Otherwise, we may run into precision issues.
                linesearchQ = false;
                continueQ = (errorestimator > Settings().tolerance);
                succeededQ = !continueQ;
            }
            else
            {
                errorestimator = infty;
                linesearchQ = Settings().Armijo_slope_factor > zero;
                //There is no way to reduce the residual below machine epsilon. If the algorithm reaches here, the problem is probably too ill-conditioned to be solved in machine precision.
                continueQ = residual > Settings().give_up_tolerance;
            }
        }
        else
        {
            q_Newton = big_one;
            lambda_min = eps;
            errorestimator = infty;
            linesearchQ = Settings().Armijo_slope_factor > zero;
            continueQ = residual > std::max( Settings().give_up_tolerance, Settings().tolerance );
        }
        
        const Real c = Settings().regularization * squared_residual;
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            for( Int k = j; k < AmbDim; ++k )
            {
                L[j][k] = DF_[j][k] + static_cast<Real>(j==k) * c;
            }
        }
        
        L.Cholesky();
        
        L.CholeskySolve(F_,u_);
        
        u_ *= -one;
    }
    
    void Gradient_Hyperbolic()
    {
        Times(-g_factor_inv, F_, u_);
    }
    
    void Gradient_Planar()
    {
        // The factor 2 is here to reproduce the Abikoff-Ye algorithm (in the absence of linesearch.)
        Times(-two, F_, u_);
    }
    
    void InverseShift()
    {
        // Shifts just the point w.
        
        const Real ww  = Dot(w_,w_);
        const Real wz2 = Dot(w_,z_) * two;
        const Real zz  = Dot(z_,z_);
    
        const Real d = one / (big_one + wz2 + ww * zz);
        
        const Real a = (one - ww) * d;
        const Real b = (one + zz + wz2) * d;
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            w_[j] = a * z_[j] + b * w_[j];
        }
    }
    
public:

    void Shift()
    {
        // Shifts all entries of x along w and writes the results to y.
        
        const Real ww = Dot(w_,w_);
        
        if( ww <= norm_threshold )
        {
            // If w lies away from the boundary of the ball, we don't normalize output after shift.
            shift<false>(ww);
        }
        else
        {
            // If w lies close to the boundary of the ball, then normalizing the output is a good idea.
            shift<true>(ww);
        }
    }

private:

    template< bool normalizeQ>
    void shift( const Real ww )
    {
        // This function is meant to reduce code duplication.
        // It is only meant to be called directly from Shift.
        
        const Real one_minus_ww = big_one - ww;
        const Real one_plus_ww  = big_one + ww;
        
        for( Int i = 0; i < edge_count_; ++i )
        {
            Vector_T z ( x_, i );
            
            const Real wx2 = two * Dot(w_,z);
            
            const Real d = one / ( one_plus_ww - wx2 );

            const Real a = one_minus_ww * d;
            
            const Real b = (wx2 - two) * d;
            
            for( Int j = 0; j < AmbDim; ++j )
            {
                z[j] = a * z[j] + b* w_[j];
            }
            
            if constexpr ( normalizeQ )
            {
                z.Normalize();
            }
            
            z.Write( y_, i );
        }
        
//        for( Int i = 0; i < edge_count_; ++i )
//        {
//            Vector_T x_i ( x_, i );
//            
//            const Real wx2 = two * Dot(w_,x_i);
//            
//            const Real d = one / ( one_plus_ww - wx2 );
//
//            const Real a = one_minus_ww * d;
//            
//            const Real b = (wx2 - two) * d;
//            
//            for( Int j = 0; j < AmbDim; ++j )
//            {
//                x_i[j] = a * x_i[j] + b* w_[j];
//            }
//            
//            if constexpr ( normalizeQ )
//            {
//                x_i.Normalize();
//            }
//            
//            x_i.Write( y_, i );
//        }
    }
