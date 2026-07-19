
public:

// This type is used to represent polynomials of the form
//  c[0] + c[1] * eps + c[2] * eps * eps.
// These appear as numerators and denominators of intersection times with the perturbation technique.
class Polynomial2 final
{
private:
    
    LInt c[3] = {};
    
public:
    
    template<IntQ I>
    LInt & operator[]( I idx )
    {
        return c[idx];
    }
    
    template<IntQ I>
    LInt operator[]( I idx ) const
    {
        return c[idx];
    }
    
    Polynomial2() = default;
    
    Polynomial2( LInt c_0, LInt c_1, LInt c_2 )
    : c { c_0, c_1, c_2 }
    {}
    
    double ToDouble() const
    {
        return double{c[0]};
    }
    
    Sign_T Sign() const
    {
        if( c[0] > LInt{0} ) return Sign_T( 1);
        if( c[0] < LInt{0} ) return Sign_T(-1);
        
        if( c[1] > LInt{0} ) return Sign_T( 1);
        if( c[1] < LInt{0} ) return Sign_T(-1);
        
        if( c[2] > LInt{0} ) return Sign_T( 1);
        if( c[2] < LInt{0} ) return Sign_T(-1);
        
        return Sign_T(0);
    }
    
    friend Polynomial2 operator+( cref<Polynomial2> P, cref<Polynomial2> Q )
    {
        Polynomial2 R;
        
        for( int idx = 0; idx <= 2; ++idx )
        {
            R[idx] += P[idx] + Q[idx];
        }
        
        return R;
    }
    
    friend Polynomial2 operator-( cref<Polynomial2> P, cref<Polynomial2> Q )
    {
        Polynomial2 R;
        
        for( int idx = 0; idx <= 2; ++idx )
        {
            R[idx] += P[idx] - Q[idx];
        }
        
        return R;
    }
    
    friend std::string ToString( cref<Polynomial2> P )
    {
        std::stringstream s;
        
        s << "Polynomial2{ " << P[0] << ", " << P[1] << ", " << P[2] << " }";
        
        return s.str();
    }
    
}; // class Polynomial2
