public:

/*!@brief Type to represent an intersection time as a rational function of the form
 *
 *      a[0] + a[1] * eps + a[2] * eps * eps
 *     --------------------------------------
 *      b[0] + b[1] * eps + b[2] * eps * eps
 *
 * This arises during calculation with perturbation.
 * The values of intersection times are of secondary interest; what matters more is `operator<=>` because it allows sorting.
 */
class IntersectionTime final
{
private:
    
    Polynomial3 a;
    Polynomial3 b;
    
public:
    
    IntersectionTime() = default;
    
    IntersectionTime( cref<Polynomial3> numerator, cref<Polynomial3> denominator )
    {
        // Make sure at the time of initialization that the denominator is >= 0!
        // This is important for later < and > comparisons.
        
        if( Sign(denominator) < Sign_T(0) )
        {
            a = -numerator;
            b = -denominator;
        }
        else
        {
            a = numerator;
            b = denominator;
        }
    }
    
//        template<typename ExtInt>
//        IntersectionTime(
//            cref<ExtInt> a_0, cref<ExtInt> a_1, cref<ExtInt> a_2,
//            cref<ExtInt> b_0, cref<ExtInt> b_1, cref<ExtInt> b_2
//        )
//        :   IntersectionTime{ Polynomial3{a_0,a_1,a_2}, Polynomial3{b_0,b_1,b_2} }
//        {}
    
//    double ToDouble() const
//    {
//        return a.ToDouble() / b.ToDouble();
//    }
  
    friend std::strong_ordering operator<=>(
        cref<IntersectionTime> s, cref<IntersectionTime> t
    )
    {
        // We have s = s.a / s.b and t = t.a / t.b;
        // We guarantee that s.b >= 0  and t.b >= 0;
        // If the latter are nonzero, then we have:
        //
        //      s < t  if and only if s.a * t.b < t.a * s.b
        //
        // And this is what we check step by step.
        // We do it in a way that most computations are deferred until they are really needed.
        // In a generic situation, we just check s.a[0] * t.b[0] < t.a[0] * s.b[0].
        
        LLInt lhs;
        LLInt rhs;
        
//        // Order 0
        lhs = long_mul(s.a.c_0, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_0);
        {
            std::strong_ordering r = (lhs <=> rhs);
            if( r != std::strong_ordering::equal ) { return r; }
        }
        
        // For generic real inputs, it is very unlinkly that we arrive here.

        // Order 1
        lhs = long_mul(s.a.c_0, t.b.c_1) + long_mul(s.a.c_1, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_1) + long_mul(s.b.c_1, t.a.c_0);
        {
            std::strong_ordering r = (lhs <=> rhs);
            if( r != std::strong_ordering::equal ) { return r; }
        }
        
        // Order 2
        lhs = long_mul(s.a.c_1, t.b.c_1);
        rhs = long_mul(s.b.c_1, t.a.c_1);
        {
            std::strong_ordering r = (lhs <=> rhs);
            if( r != std::strong_ordering::equal ) { return r; }
        }
        
        // Order 3
        lhs = long_mul(s.a.c_0, t.b.c_3) + long_mul(s.a.c_3, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_3) + long_mul(s.b.c_3, t.a.c_0);
        {
            std::strong_ordering r = (lhs <=> rhs);
            if( r != std::strong_ordering::equal ) { return r; }
        }
        
        // Order 4
        lhs = long_mul(s.a.c_1, t.b.c_3) + long_mul(s.a.c_3, t.b.c_1);
        rhs = long_mul(s.b.c_1, t.a.c_3) + long_mul(s.b.c_3, t.a.c_1);
        {
            std::strong_ordering r = (lhs <=> rhs);
            if( r != std::strong_ordering::equal ) { return r; }
        }
        
        // Order 5 -- not existent.
        
        // Order 6
        lhs = long_mul(s.a.c_3, t.b.c_3);
        rhs = long_mul(s.b.c_3, t.a.c_3);
        {
            std::strong_ordering r = (lhs <=> rhs);
            if( r != std::strong_ordering::equal ) { return r; }
        }
        
        return std::strong_ordering::equal;
    }
    
    friend std::string ToString( cref<IntersectionTime> I )
    {
        return ToString(I.a) + " / " + ToString(I.b);
    }
};
