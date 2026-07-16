class Classifier
{
private:
    
    using Scal = Complex64;
    using Real = Real64;
//    using Scal = Real64;
    
    static constexpr Scal T = -1;
    
    Alexander_UMFPACK<Scal,Int> alex;
    
    Scal mantissa;
    Int  exponent;
    
public:
    
    Classifier()
    :   alex( Int(max_crossing_count + Int(1)) ) // Don't use sparse arithmetic
    {}
    
    Invariant_T operator()( cref<PD_T> pd )
    {
        mantissa = 0;
        exponent = 0;
        (void)alex.Alexander(pd, T, mantissa, exponent, false);
        return Round<Size_T>(Abs(mantissa) * Tools::Power(Real(10),exponent));
    }
};
