public:

    /*!@brief After the conformal closure has been computed, this returns the reweighting factor to be used for sampling on the space of closed polygons _without_ modding-out the rotation group.
     */

    Real EdgeSpaceSamplingWeight() const
    {
        return edge_space_sampling_weight;
    }

/*!@brief After the conformal closure has been computed, this returns the reweighting factor to be used for sampling on the space of closed polygons modulo the rotation group.
 */

    Real EdgeQuotientSpaceSamplingWeight() const
    {
        return edge_quotient_space_sampling_weight;
    }

private:

    Real EdgeQuotientSpaceSamplingCorrection() const
    {
        Tiny::SelfAdjointMatrix<AmbDim, Real, Int> Sigma;
        
        // We fill only the upper triangle of Sigma, because that's the only thing that the function Eigenvalues needs.
      
        if constexpr ( zerofy_firstQ )
        {
            Sigma.SetZero();
            
            for( Int i = 0; i < edge_count_; ++i )
            {
                Vector_T y_i ( y_, i );
                
                const Real rho_squared = rho_[i] * rho_[i];
                
                for( Int j = 0; j < AmbDim; ++j )
                {
                    const Real factor = rho_squared * y_i[j];
                    
                    for( Int k = j; k < AmbDim; ++k )
                    {
                        Sigma[j][k] += factor * y_i[k];
                    }
                }
            }
        }
        else
        {
            // Overwrite for i = 0.
            {
                constexpr Int i = 0;
                
                Vector_T y_i ( y_, i );
                
                const Real rho_squared = rho_[i] * rho_[i];
                
                for( Int j = 0; j < AmbDim; ++j )
                {
                    const Real factor = rho_squared * y_i[j];

                    for( Int k = j; k < AmbDim; ++k )
                    {
                        Sigma[j][k] = factor * y_i[k];
                    }
                }
            }

            // Now we add-in the other entries.
            for( Int i = 1; i < edge_count_; ++i )
            {
                Vector_T y_i ( y_, i );
                
                const Real rho_squared = rho_[i] * rho_[i];
                
                for( Int j = 0; j < AmbDim; ++j )
                {
                    const Real factor = rho_squared * y_i[j];

                    for( Int k = j; k < AmbDim; ++k )
                    {
                        Sigma[j][k] += factor * y_i[k];
                    }
                }
            }
        }
        
        Real det;
        
        if constexpr ( AmbDim == 2 )
        {
            det = Abs( Sigma[0][0] * Sigma[1][1] - Sigma[0][1] * Sigma[0][1] );
        }
        if constexpr ( AmbDim == 3 )
        {
            // Exploiting that
            //      (lambda[0] + lambda[1]) * (lambda[0] + lambda[2]) * (lambda[1] + lambda[2])
            //      =
            //      ( tr(Sigma*Sigma) - tr(Sigma)*tr(Sigma) ) *  tr(Sigma)/2 - det(Sigma)
            //  Thus, it can be expressed by as third-order polynomial in the entries of the matrix.
            
            const Real S_00 = Sigma[0][0] * Sigma[0][0];
            const Real S_11 = Sigma[1][1] * Sigma[1][1];
            const Real S_22 = Sigma[2][2] * Sigma[2][2];
            
            const Real S_10 = Sigma[0][1] * Sigma[0][1];
            const Real S_20 = Sigma[0][2] * Sigma[0][2];
            const Real S_21 = Sigma[1][2] * Sigma[1][2];
            
            det = Abs(
                  Sigma[0][0] * ( S_11 + S_22 - S_10 - S_20 )
                + Sigma[1][1] * ( S_00 + S_22 - S_10 - S_21 )
                + Sigma[2][2] * ( S_00 + S_11 - S_20 - S_21 )
                + two * (Sigma[0][0]*Sigma[1][1]*Sigma[2][2] - Sigma[0][1]*Sigma[0][2]*Sigma[1][2])
            );
        }
        else
        {
            Tiny::Vector<AmbDim,Real,Int> lambda;
            
            // Compute eigenvalues by QR algorithm.
            
            Sigma.Eigenvalues( lambda );
            
            det = one;
            
            for( Int j = 0; j < AmbDim; ++j )
            {
                for( Int k = j+1; k < AmbDim; ++k )
                {
                    det *= (lambda(j)+lambda(k));
                }
            }
        }
        
        return edge_quotient_space_sampling_helper * Inv(Sqrt(det));
    }

private:

    void ComputeEdgeSpaceSamplingHelper()
    {
        edge_space_sampling_helper
            =
            Frac(
                GammaQuotient(
                    static_cast<Real>((AmbDim-1) * (edge_count_-1)),
                    Frac<Real>( AmbDim, 2 )
                ),
                Power( static_cast<Real>(2) * Sqrt( Scalar::Pi<Real>), AmbDim )
            );
    }

    void ComputeEdgeQuotientSpaceSamplingHelper()
    {
        edge_quotient_space_sampling_helper
            =
            Frac<Real>(
                std::exp2( Scalar::Quarter<Real> * static_cast<Real>( AmbDim * (AmbDim-1) ) ),
                SOVolume<Real>(AmbDim)
            );
    }

    void ComputeEdgeSpaceSamplingWeight() const
    {
        // Shifts all entries of x along y and writes the results to y.
        // Mind that x and y are stored in SoA fashion, i.e., as matrix of size AmbDim x point_count.
        
        SquareMatrix_T cbar  (zero);
        SquareMatrix_T gamma (zero);
        
        Real prod = one;
        
        const Real ww = Dot(w_,w_);
        
        const Real one_plus_ww = big_one + ww;
        
        for( Int i = 0; i < edge_count_; ++i )
        {
            Vector_T y_i ( y_, i );
            
            const Real wy = Dot(w_,y_i);
            
            const Real factor = one_plus_ww + two * wy;
            
            // Multiplying by one_plus_ww_inv so that prod does not grow so quickly.
            prod *= factor;
            
            const Real r_i = r_[i];
            const Real r_over_rho_i = r_i / rho_[i];
            const Real r_over_rho_i_squared = r_over_rho_i * r_over_rho_i;
            
            for( Int j = 0; j < AmbDim; ++j )
            {
                for( Int k = 0; k < AmbDim; ++k )
                {
                    const Real scratch = static_cast<Real>(j==k) - y_i[j] * y_i[k];
                    
                    gamma[j][k] += r_over_rho_i_squared * scratch;
                    
                    cbar [j][k] += r_i * scratch;
                }
            }
        }
        
        // We can simply absorb the factor std::pow(2/(one_minus_ww),d) into the function chi.
        //  cbar *= static_cast<Real>(2)/(one_minus_ww);
        
        edge_space_sampling_weight =
            edge_space_sampling_helper
            *
            Power( prod, static_cast<Int>(AmbDim-1) ) * Sqrt(gamma.Det()) / cbar.Det();
    }

    void ComputeEdgeQuotientSpaceSamplingWeight() const
    {
        edge_quotient_space_sampling_weight = EdgeSpaceSamplingWeight() * EdgeQuotientSpaceSamplingCorrection();
    }
