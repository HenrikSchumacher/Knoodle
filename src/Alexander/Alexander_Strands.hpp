private:



template<bool sparseQ, typename ExtScal, typename ExtInt>
void Alexander_Strands(
    cref<PD_T>    pd,
    cptr<ExtScal> args,
    ExtInt        arg_count,
    mptr<ExtScal> mantissas,
    mptr<ExtInt>  exponents,
    bool          multiply_toQ
) const
{
    TOOLS_PTIC(ClassName()+"::Alexander_Strands" + "<" + ToString(sparseQ) + ">");
    
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

    TOOLS_PTOC(ClassName()+"::Alexander_Strands" + "<" + ToString(sparseQ) + ">");
}

    
template<bool sparseQ, typename ExtScal, typename ExtInt>
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

template<bool sparseQ>
std::pair<Multiplier_T,E_T> &
Alexander_Strands_Normalization( cref<PD_T> pd ) const
{
    const std::string tag ( ClassName() + "::Alexander_Strands_Normalization" + "<" + ToString(sparseQ) + ">" );
    
    using T = std::pair<Multiplier_T,E_T>;
    
    if( !pd.InCacheQ(tag) )
    {
        const Multiplier_T unit;
        
        const Multiplier_T a = Alexander_Strands_Det<sparseQ>( pd, Complex(1) );
        
        Multiplier_T factor ( unit / a );
        
        constexpr Real eval_0 = cSqrt(Real(2));
        Real eval = eval_0;
        Int sqrt_count = 1;
        Multiplier_T c = Alexander_Strands_Det<sparseQ>( pd, Complex(eval) );
        
        while( c.Mantissa2() == Scalar::Zero<Complex> )
        {
            ++sqrt_count;
            eval = std::sqrt(eval);
            c = Alexander_Strands_Det<sparseQ>( pd, eval );
        }
        
        Multiplier_T b = Alexander_Strands_Det<sparseQ>( pd, Complex(Inv(eval)) );
        
        const Multiplier_T d = b / c; // Should be a power of eval * eval.
        
        auto e = ( std::log2(Re(d.Mantissa2())) + d.Exponent2()  ) / std::log2(eval * eval);
        
        E_T exp = static_cast<Int>( std::round(e) );
        
        pd.SetCache( tag, std::pair(factor,exp) );
    }
        
    return pd.template GetCache<T>(tag);
}
