public:

/*!@brief Returns the number of edges.
 */

Int EdgeCount() const
{
  return edge_count_;
}

/*!@brief Returns the list of edge lengths.
 */

const Weights_T & EdgeLengths() const
{
    return r_;
}

/*!@brief Reads a new list of edge lengths from buffer `r`.
 *
 * @param r Buffer containing the new edge lengths; assumed to have length `this->EdgeCount()`.
 */

void ReadEdgeLengths( const Real * const r )
{
    r_.Read(r);
    
    total_r_inv = Inv( r_.Total() );
}

/*!@brief Returns the list of weights for the Riemannian metrics on the product of unit spheres.
 */

const Weights_T & Rho() const
{
    return rho_;
}

/*!@brief Reads a new list of edge weights for the Riemannian metric from buffer `rho`.
 *
 * @param rho Buffer containing the new weights; assumed to have length `this->EdgeCount()`.
 */


void ReadRho( const Real * const rho )
{
    rho_.Read(rho);
}

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
    return errorestimator;
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


private:

static Real tanhc( const Real t )
{
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
