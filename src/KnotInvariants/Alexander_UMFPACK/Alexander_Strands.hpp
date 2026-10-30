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


// TODO: Document this! I don't understand this anymore.

template<bool sparseQ>
std::pair<Multiplier_T,E_T> &
Alexander_Strands_Normalization( cref<PD_T> pd ) const
{
    const std::string tag ( ClassName()+"::Alexander_Strands_Normalization" + "<" + ToString(sparseQ) + ">" );
    
    using T = std::pair<Multiplier_T,E_T>;
    
    if( !pd.InCacheQ(tag) )
    {
        if( pd.LinkComponentCount() > Int(1) )
        {
            pd.SetCache( tag, std::pair(Multiplier_T(),E_T(0)) );
        }
        else
        {
            const Multiplier_T unit;
            
            const Multiplier_T a = Alexander_Strands_Det<sparseQ>( pd, Complex(1) );
            
            Multiplier_T factor ( unit / a );
            
            // The strand determinant is det(t) = +/- t^k * Delta(t) for some integer k. As Delta(1/t) = Delta(t),
            // the quotient det(1/x) / det(x) is x^(-2k), and we read off exp = -k from it at a real point x.
            // If x is a root of Delta, both determinants are rounding noise, and so is the quotient. For example,
            // Delta(sqrt(2)) = 0 whenever 2 t^2 - 5 + 2 t^-2 (the Alexander polynomial of 6_1 at t^2) divides Delta.
            // So we accept the quotient only if it is positive and its logarithm is within tol of an integer
            // (for valid points it is within 1e-10, even at 100k crossings); otherwise we try the next point
            // in the sequence sqrt(2), 2^(1/4), 2^(1/8), ... .
            constexpr Int  max_tries = 8;
            constexpr Real tol       = Real(1e-6);
            
            Real eval   = cSqrt(Real(2));
            bool foundQ = false;
            E_T  exp    = 0;
            
            for( Int try_count = 0; try_count < max_tries; ++try_count )
            {
                const Multiplier_T c = Alexander_Strands_Det<sparseQ>( pd, Complex(eval) );
                const Multiplier_T b = Alexander_Strands_Det<sparseQ>( pd, Complex(Inv(eval)) );
                
                const Multiplier_T d = b / c; // Should be a power of eval * eval.
                
                const Real e = ( std::log2(Abs(d.Mantissa2())) + d.Exponent2() ) / std::log2(eval * eval);
                
                if( (Re(d.Mantissa2()) > Real(0)) && std::isfinite(e) && (Abs(e - std::round(e)) <= tol) )
                {
                    exp    = static_cast<E_T>( std::round(e) );
                    foundQ = true;
                    break;
                }
                
                eval = std::sqrt(eval);
            }
            
            if( !foundQ )
            {
                eprint(MethodName("Alexander_Strands_Normalization") + ": Could not determine the normalizing power of t; the Alexander polynomial seems to vanish at all " + ToString(max_tries) + " evaluation points. Returning NaN.");
                
                factor = Multiplier_T( Scal(std::numeric_limits<Real>::quiet_NaN()) );
            }
            
            pd.SetCache( tag, std::pair(factor,exp) );
        }
    }
        
    return pd.template GetCache<T>(tag);
}
