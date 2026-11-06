private:



template<bool sparseQ, typename ExtScal, IntQ ExtInt>
void Alexander_Strands(
    cref<PD_T>    pd,
    cptr<ExtScal> args,
    ExtInt        arg_count,
    mptr<ExtScal> mantissas,
    mptr<ExtInt>  exponents,
    bool          multiply_toQ
) const
{
    TOOLS_PTIMER(timer,ClassName()+"::Alexander_Strands" + "<" + ToString(sparseQ) + ">");
    
    if( pd.CrossingCount() <= 1 )
    {
        if( !multiply_toQ )
        {
            fill_buffer( mantissas, ExtScal(1), arg_count );
            zerofy_buffer( exponents, arg_count );
        }
    }
    else
    {
        for( ExtInt k = 0; k < arg_count; ++k )
        {
            Alexander_Strands<sparseQ>(
                pd, args[k], mantissas[k], exponents[k], multiply_toQ
            );
        }
    }
}

    
template<bool sparseQ, typename ExtScal, IntQ ExtInt>
void Alexander_Strands(
    cref<PD_T>    pd,
    cref<ExtScal> arg,
    mref<ExtScal> mantissa,
    mref<ExtInt > exponent,
    bool          multiply_toQ
) const
{
    const Scal t = scalar_cast<Scal>(arg);

    auto [factor,exp] = Alexander_Strands_Normalization<sparseQ>(pd);
    
    if( !std::isfinite( Re(factor.Mantissa2()) ) )
    {
        // The normalization failed and has already said so. Return NaN rather than a wrong value.
        mantissa = static_cast<ExtScal>( std::numeric_limits<Real>::quiet_NaN() );
        exponent = ExtInt(0);
        return;
    }
    
    Multiplier_T det = Alexander_Strands_Det<sparseQ>(pd,t);
    
    Multiplier_T pow = Multiplier_T::Power(t,exp);
    
    Multiplier_T alex = factor * det * pow;

    if ( multiply_toQ )
    {
        // TODO: This way of converting factor back and forth is somewhat annoying. Can this be done in a cleaner way?
        
        alex *= Multiplier_T( static_cast<Scal>(mantissa) ) * Multiplier_T::Power(Scal(10), static_cast<Int>(exponent) );
    }
    
    auto [m,e] = alex.MantissaExponent10();

    mantissa = static_cast<ExtScal>(m);
    exponent = static_cast<ExtInt >(e);
}
    

template<bool sparseQ>
Multiplier_T Alexander_Strands_Det( cref<PD_T> pd, cref<Scal> arg) const
{
    if constexpr( sparseQ )
    {
        return Alexander_Strands_Det_Sparse(pd,arg);
    }
    else
    {
        return Alexander_Strands_Det_Dense(pd,arg);
    }
}


public:


// TODO: Document this! I don't understand this anymore.

/*!@brief Compute the normalization of the Alexander polynomial.
 *
 * Let \f$\mathrm{det}\f$ be the output of `Alexander_Strands_Det` at point \f$t\f$. Beware that \f$\mathrm{det}(t)\f$  is not always equal to the Alexander polynomial \f$\Delta(t)\f$ . Instead, the relation is \f$\Delta(t) = a \, t^k \, \mathrm{det}(t)\f$ for some integer exponent \f$k\f$ and some correcting factor \f$a\f$. The job of this function is to compute the pair \f$(a,k)\f$ so that it can be used to normalize `Alexander_Strands_Det`.
 *
 * The idea for extracting \f$k\f$ is to exploit the following symmetry of the Alexander polynomial:
 * \f\[
 *      \Delta(1/t) = \Delta(t).
 * \f\]
 * Hence, we get
 * \f\[
 *      \frac{\mathrm{det}(1/t)}{\mathrm{det}(t)}
 *      =
 *      \frac{ a^{-1} \, t^{k} \, \Delta(1/t) }{ a^{-1} \, t^{-k} \, \Delta(t) }
 *      = t^{2k}
 *   .
 * \f\]
 * So, we merely need to guarantee that \f$t\f$ is neither a root or pole of \f$\Delta\f$ and apply the logarithm:
 * \f\[
 *      k = \log \left( \frac{ \mathrm{det}(1/t) }{ \mathrm{det}(t) } \right) / \log(t^2) .
 * \f\]
 * Since \f$|\Delta(1)| = |\mathrm{det}(1)|  = \pm 1\f$, it suffices to test a sequence that approaches \f$1\f$ on the real line. We do this by starting at \f$t = \sqrt{2}\f$ and recursively set \f$t \leftarrow \sqrt{r}\f$ until \f$|\mathrm{det}(t)|\f$ is neither \f$0\f$ nor \f$\infty\f$.
 *
 * To extract \f$a\f$, we exploit that we use the normalization \f$\Delta(1) = 1\f$. This way we have
 * \f\[
 *      \Delta(1) = a \, \mathrm{det}(1) = a.
 * \f\]
 * Hence,
 * \f\[
 *      a = 1/\mathrm{det}(1).
 * \f\]
 */

template<bool sparseQ>
std::pair<Multiplier_T,Exponent_T> &
Alexander_Strands_Normalization( cref<PD_T> pd ) const
{
    const std::string tag ( ClassName()+"::Alexander_Strands_Normalization<" + ToString(sparseQ) + ">" );
    
    using T = std::pair<Multiplier_T,Exponent_T>;
    
    if( !pd.InCacheQ(tag) )
    {
        if( pd.LinkComponentCount() > Int(1) )
        {
            pd.SetCache( tag, std::pair(Multiplier_T(),Exponent_T(0)) );
        }
        else
        {
            // For `t = 1` we have `Delta(1) = a * det(1)`.
            // We normalize the Alexander polynomial so that `\Delta(1) = 1`. Hence, a / det(1).
            Multiplier_T a = Multiplier_T() / Alexander_Strands_Det<sparseQ>( pd, Complex(1) );
            
            // The strand determinant is det(t) = +/- t^k * Delta(t) for some integer k. As Delta(1/t) = Delta(t),
            // the quotient det(1/t) / det(t) is t^(-2k), and we read off exp = -k from it at a real point t.
            // If t is a root of Delta, both determinants are rounding noise, and so is the quotient. For example,
            // Delta(sqrt(2)) = 0 whenever 2 t^2 - 5 + 2 t^-2 (the Alexander polynomial of 6_1 at t^2) divides Delta.
            // So we guarantee that c = det(t) and b = det(1/t) are finite and neither too close nor too far fromt the origin
            // by
            
            
            // So we accept the quotient only if it is positive and its logarithm is within tol of an integer
            // (for valid points it is within 1e-10, even at 100k crossings); otherwise we try the next point
            // in the sequence sqrt(2), 2^(1/4), 2^(1/8), ...
            // Since |det(1)| = |Delta(t)| = 1, this will terminate after a finite number of steps. If it does not, then something is fundementally wrong with the det(1).
            

            Size_T failure_count = 0;
            constexpr Size_T max_failure_count = 8;
            constexpr Real tol = 0.0001;
            
            Real t_squared = cSqrt(Real(2));
            Real t = cSqrt(t_squared);        // We start with the fourth root of 2.
            Exponent_T k = 0;
            
            Multiplier_T c;
            Multiplier_T b;
            
            while( failure_count <  max_failure_count )
            {
                // We break the loop if `t` is so close to `1` that taking a further root won't change it.
                if( t >= t_squared  )
                {
                    eprint(MethodName("Alexander_Strands_Normalization"), ": Could not determine the normalizing power of t. Returning NaN.");
                    
                    a = Multiplier_T( Scal(std::numeric_limits<Real>::quiet_NaN()) );
                    break;
                }
                
                c = Alexander_Strands_Det<sparseQ>( pd, Complex(t) );
                
//                TOOLS_DUMP(c.Mantissa10());
//                TOOLS_DUMP(c.Exponent10());
                
                // We don't want t to be a root or pole of Alexander_Strands_Det.
                // So a small or big magnitude of `c` tells us that we have to move `t` closer to 1.
                if( !FiniteQ(c.Mantissa2()) || (c.Exponent2() <= Int{-10}) || (b.Exponent2() >= Int{10}) )
                {
                    t_squared = t;
                    t = std::sqrt(t);
                    continue;
                }
                
                b = Alexander_Strands_Det<sparseQ>( pd, Complex(Inv(t)) );
                
//                TOOLS_DUMP(b.Mantissa10());
//                TOOLS_DUMP(b.Exponent10());
                
                // We don't want 1/t to be a root or pole of Alexander_Strands_Det.
                // So a small or big magnitude of `b` tells us that we have to move `t` closer to 1.
                if( !FiniteQ(b.Mantissa2()) || (b.Exponent2() <= Int{-10}) || (b.Exponent2() >= Int{10}) )
                {
                    t_squared = t;
                    t = std::sqrt(t);
                    continue;
                }

                // This division should be save now
                const Multiplier_T d = b / c; // Should be a power of `t_squared = t * t`.
                    
//                TOOLS_DUMP(d.Mantissa10());
//                TOOLS_DUMP(d.Exponent10());
                
                assert(FiniteQ(d.Mantissa2()));
                
                // When we arrive here, d must b finite and cannot be 0; so also its mantissa is nonzero.
                const Real e = ( std::log2(Abs(d.Mantissa2())) + static_cast<Real>(d.Exponent2()) ) / std::log2(t_squared);
                
                assert(FiniteQ(e));
                
                const Real e_rounded = std::round(e);
                
                if( Abs(e - e_rounded) > tol )
                {
                    ++failure_count;
                    continue;
                }
                
                k = static_cast<Exponent_T>( std::round(e) );
                break;
            }
            
            if( failure_count > max_failure_count )
            {
                Msgr::eprint("Alexander_Strands_Normalization", "Could not determine the normalizing power of t. There seems to be too much rounding error. Returning NaN.");
                
                a = Multiplier_T( Scal(std::numeric_limits<Real>::quiet_NaN()) );
            }
            
            pd.SetCache(tag, std::pair(a,k));
        }
    }
        
    return pd.template GetCache<T>(tag);
}
