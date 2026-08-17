public:

/*!@brief This type is used to represent polynomials of the form  `c_0 + c_1 * eps + c_3 * eps * eps * eps`. These appear as numerators and denominators of intersection times with the perturbation technique.
 */
class DepressedCubic final
{
public:
    
    LInt c_0 {0};
    LInt c_1 {0};
    LInt c_3 {0};
    
public:
    
    DepressedCubic() = default;
    
    DepressedCubic( LInt c_0_, LInt c_1_, LInt c_3_ )
    :   c_0 { c_0_ }
    ,   c_1 { c_1_ }
    ,   c_3 { c_3_ }
    {}
    
    template<typename R = Sign_T>
    friend R Sign( cref<DepressedCubic> P )
    {
        R s;
        s = Sign<R>(P.c_0);
        if( s != R(0) ) { return s; }
        s = Sign<R>(P.c_1);
        if( s != R(0) ) { return s; }
        s = Sign<R>(P.c_3);
        if( s != R(0) ) { return s; }
        return R(0);
    }
    
    friend DepressedCubic operator+( cref<DepressedCubic> P, cref<DepressedCubic> Q )
    {
        return DepressedCubic{ P.c_0 + Q.c_0, P.c_1 + Q.c_1, P.c_3 + Q.c_3 };
    }
    
    friend DepressedCubic operator-( cref<DepressedCubic> P, cref<DepressedCubic> Q )
    {
        return DepressedCubic{ P.c_0 - Q.c_0, P.c_1 - Q.c_1, P.c_3 - Q.c_3 };
    }
    
    friend DepressedCubic operator-( cref<DepressedCubic> P )
    {
        return DepressedCubic{ -P.c_0, -P.c_1, -P.c_3 };
    }
    
    friend double ToDouble( cref<DepressedCubic> P )
    {
        using Tools::ToDouble;
        
        if constexpr ( std::is_convertible<LInt,double>::value )
        {
            return static_cast<double>(P.c_0);
        }
        else
        {
            return ToDouble(P.c_0);
        }
    }
    
    friend std::string ToString( cref<DepressedCubic> P )
    {
        using Tools::ToString;
        
        std::stringstream s;
        s << "DepressedCubic{ " << ToString(P.c_0) << ", " << ToString(P.c_1) << ", " << ToString(P.c_3) << " }";
        return s.str();
    }
    
}; // class DepressedCubic
