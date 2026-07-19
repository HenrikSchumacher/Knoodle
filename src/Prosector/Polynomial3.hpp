public:

// This type is used to represent polynomials of the form
//  c[0] + c[1] * eps + c[2] * eps * eps.
// These appear as numerators and denominators of intersection times with the perturbation technique.
struct Polynomial3 final
{
public:
    
    LInt c_0 {0};
    LInt c_1 {0};
    LInt c_3 {0};
    
public:
    
    Polynomial3() = default;
    
    Polynomial3( LInt c_0_, LInt c_1_, LInt c_3_ )
    :   c_0 { c_0_ }
    ,   c_1 { c_1_ }
    ,   c_3 { c_3_ }
    {}
    
    double ToDouble() const
    {
        return double{c_0};
    }
    
    Sign_T Sign() const
    {
        if( c_0 > LInt{0} ) return Sign_T( 1);
        if( c_0 < LInt{0} ) return Sign_T(-1);
        
        if( c_1 > LInt{0} ) return Sign_T( 1);
        if( c_1 < LInt{0} ) return Sign_T(-1);
        
        if( c_3 > LInt{0} ) return Sign_T( 1);
        if( c_3 < LInt{0} ) return Sign_T(-1);
        
        return Sign_T(0);
    }
    
    friend Polynomial3 operator+( cref<Polynomial3> P, cref<Polynomial3> Q )
    {
        return Polynomial3{ P.c_0 + Q.c_0, P.c_1 + Q.c_1, P.c_3 + Q.c_3 };
    }
    
    friend Polynomial3 operator-( cref<Polynomial3> P, cref<Polynomial3> Q )
    {
        return Polynomial3{ P.c_0 - Q.c_0, P.c_1 - Q.c_1, P.c_3 - Q.c_3 };
    }
    
    friend Polynomial3 operator-( cref<Polynomial3> P )
    {
        return Polynomial3{ - P.c_0, - P.c_1, - P.c_3 };
    }
    
    friend std::string ToString( cref<Polynomial3> P )
    {
        std::stringstream s;
        
        s << "Polynomial3{ " << P.c_0 << ", " << P.c_1 << ", " << P.c_3 << " }";
        
        return s.str();
    }
    
}; // class Polynomial3
