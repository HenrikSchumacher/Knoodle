public:

/*!@brief This type is used to represent polynomials of the form  `c_0 + c_1 * eps + c_3 * eps * eps * eps`. These appear as numerators and denominators of intersection times with the perturbation technique.
 */
struct DepressedCubic final
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
    
    friend Sign_T Sign( cref<DepressedCubic> P)
    {
        Sign_T s;
        s = Sign(P.c_0);
        if( s != Sign_T(0) ) { return s; }
        s = Sign(P.c_1);
        if( s != Sign_T(0) ) { return s; }
        s = Sign(P.c_3);
        if( s != Sign_T(0) ) { return s; }
        return Sign_T(0);
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
        return ToDouble(P.c_0);
        
//        if constexpr ( IntQ<LInt> )
//        {
//            return static_cast<double>(P.c_0);
//        }
//        {
//            return ToDouble(P.c_0);
//        }
    }
    
    friend std::string ToString( cref<DepressedCubic> P )
    {
        std::string s ("DepressedCubic{ ");
        
        s+= ToString(P.c_0);
        s+= ", ";
        s+= ToString(P.c_1);
        s+= ", ";
        s+= ToString(P.c_3);
        s+= " }";

        return s;
    }
    
}; // class DepressedCubic
