private:

void ComputeInitialShiftVector()
{
    if constexpr ( zerofy_firstQ )
    {
        w_.SetZero();
        
        for( Int i = 0; i < edge_count_; ++i )
        {
            Vector_T x_i ( x_, i );
            
            const Real r_i = r_[i];
            
            for( Int j = 0; j < AmbDim; ++j )
            {
                w_[j] += x_i[j] * r_i;
            }
        }
    }
    else
    {
        // Overwrite by first summand.
        {
            Vector_T x_i ( x_, 0 );
            
            const Real r_i = r_[0];

            for( Int j = 0; j < AmbDim; ++j )
            {
                w_[j] = x_i[j] * r_i;
            }
        }

        // Add-in the others.
        for( Int i = 1; i < edge_count_; ++i )
        {
            Vector_T x_i ( x_, i );
            
            const Real r_i = r_[i];

            for( Int j = 0; j < AmbDim; ++j )
            {
                w_[j] += x_i[j] * r_i;
            }
        }
    }
    
    // Normalize in that case that r does not sum up to 1.
    w_ *= total_r_inv;
}

public:

/*!
 * @brief Reads an initial guess for the conformal barycenter (to be used in the optimization routine `Optimize`) from the buffer `w`.
 */

void ReadShiftVector( const Real * restrict const w)
{
    w_.Read(w);
    
    // Use Euclidean barycenter as initial guess if the supplied initial guess does not make sense.
    if( Dot(w_,w_) > small_one )
    {
        ComputeInitialShiftVector();
    }
}


/*!@brief Writes the current shift vector (e.g., the conformal barycenter after the optimization has succeeded) to the buffer `w`.
 */

void WriteShiftVector( Real * restrict w ) const
{
    w_.Write(w);
}

/*!@brief Returns the current shift vector (e.g., the conformal barycenter after the optimization has succeeded).
 */
 
const Vector_T & ShiftVector() const
{
    return w_;
}

/*!@brief Returns the current residual of the optimization routine `Optimize`.
 */

Real Residual() const
{
    return residual;
}

/*!@brief After `Optimize` succeeded, this is an upper bound for the distance of `ShiftVector()` to the true conformal barycenter of the open polygon.
 */
Real ErrorEstimator() const
{
    return error_estimator;
}

/*!brief Returns the number of iterations the last call to `Optimize` needed.
 */

Int IterationCount() const
{
    return iter;
}

Int MaxIterationCount() const
{
    return settings_.max_iter;
}

bool SucceededQ() const
{
  return succeededQ;
}


private:

static Real tanhc( const Real t )
{
//    TOOLS_MAKE_FP_FAST();
    
    // Computes tanh(t)/t in a stable way by using a Padé approximation around t = 0.
    constexpr Real a0 = one;
    constexpr Real a1 = Frac<Real>(7,51);
    constexpr Real a2 = Frac<Real>(1,255);
    constexpr Real a3 = Frac<Real>(2,69615);
    constexpr Real a4 = Frac<Real>(1,34459425);
    
    constexpr Real b0 = one;
    constexpr Real b1 = Frac<Real>(8,17);
    constexpr Real b2 = Frac<Real>(7,255);
    constexpr Real b3 = Frac<Real>(4,9945);
    constexpr Real b4 = Frac<Real>(1,765765);
    
    const Real t2 = t * t;
    
    const Real result = ( t2 <= one )
    ? (
        a0 + t2 * (a1 + t2 * (a2 + t2 * (a3 + t2 * a4)))
    )/(
        b0 + t2 * (b1 + t2 * (b2 + t2 * (b3 + t2 * b4)))
    )
    : ( t2 <= static_cast<Real>(7) ) ? std::tanh(t)/t : one/std::abs(t);
    
    return result;
}


Vector_T Barycenter( cref<VectorContainer_T> z, const M_T mode )
{
    switch( mode )
    {
        case M_T::None:             return barycenter<M_T::None>(z);
        case M_T::UnitOnPoints:     return barycenter<M_T::UnitOnPoints>(z);
        case M_T::UnitOnMidpoints:  return barycenter<M_T::UnitOnMidpoints>(z);
        case M_T::UniformOnEdges:   return barycenter<M_T::UniformOnEdges>(z);
    }
}

template<M_T mode>
Vector_T barycenter( cref<VectorContainer_T> z )
{
    TOOLS_MAKE_FP_FAST();
    
    Vector_T barycenter (zero);
    
    if constexpr ( mode == M_T::None ) { return barycenter; }
    
    Vector_T point (zero);
    
    // Do it backwards, so that as much memory as possible is still warm when we do a second loop to subtract the barycenter.
    // Beware that we have to multiply each edge vector with -1.
    for( Int i = edge_count_; i --> Int(0); )
    {
        const Vector_T z_i ( z, i );
        
        const Real r_i = r_[i];
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            const Real delta = - r_i * z_i[j];
            
            if constexpr ( mode == M_T::UnitOnMidpoints )
            {
                barycenter[j] += (point[j] + half * delta);
            }
            else if constexpr ( mode == M_T::UniformOnEdges )
            {
                barycenter[j] += r_i * (point[j] + half * delta);
            }
            
            point[j] += delta;
            
            if constexpr ( mode == M_T::UnitOnPoints )
            {
                barycenter[j] += point[j];
            }
        }
    }

    if constexpr ( mode == M_T::UnitOnPoints )
    {
        barycenter *= Inv<Real>( edge_count_ + Int(1) );
    }
    if constexpr ( mode == M_T::UnitOnMidpoints )
    {
        barycenter *= Inv<Real>( edge_count_ );
    }
    else if constexpr ( mode == M_T::UniformOnEdges )
    {
        barycenter *= total_r_inv;
    }

    return barycenter;
}

// Compute vertex coordinates from unit edge vectors `x`, a starting vector `point` and write the results into `p`.
void writeCoordinates(
    cref<VectorContainer_T> x, mref<Vector_T> point, mptr<Real> p, const bool wrap_aroundQ
)
{
    TOOLS_MAKE_FP_FAST();
    
    for( Int i = 0; i < edge_count_; ++i )
    {
        point.Write( &p[AmbDim * i] );
        
        const Vector_T x_i ( x, i );
        
        const Real r_i = r_[i];
        
        for( Int j = 0; j < AmbDim; ++j )
        {
            point[j] += r_i * x_i[j];
        }
    }
    
    if( wrap_aroundQ )
    {
        point.Write( &p[AmbDim * edge_count_] );
    }
}

void writeCoordinates(
    cref<VectorContainer_T> x, mptr<Real> p, const M_T mode, const bool wrap_aroundQ
)
{
    Vector_T point = Barycenter(x,mode);
    point *= Real(-1);
    writeCoordinates(x, point, p, wrap_aroundQ);
}

private:
    

void PrintWarnings()
{
    static_assert( AmbDim > 1, "Polygons in ambient dimension AmbDim < 2 do not make sense.");
    
    if constexpr ( AmbDim > 12 )
    {
        wprint(this->ClassName()+": The eigensolver employed by this class has been developped specifically for small ambient dimensions AmbDim_ <= 12. If you use this with higher dimensions be aware that the sampling weights for the quotient space may contain significant errors.");
    }
    
    if ( edge_count_ <= AmbDim )
    {
        wprint(this->ClassName()+": Closed polygons with " + ToString(edge_count_) + " edges span an affine subspace of dimension at most " + ToString(edge_count_) + "-1 < ambient dimension = " + ToString(AmbDim)+ ". The current implementation of the sampling weights for the quotient space leads to wrong results. Better reduce the template parameter AmbDim_ to " + ToString(edge_count_ - 1) + "; that will lead to correct weights, also for greater ambient dimensions.");
    }
}
