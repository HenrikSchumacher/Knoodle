public:

/*!@brief **EXPERIMENTAL** Type to represent an intersection time as a rational function of the form
 *
 *      a[0] + a[1] * eps + a[2] * eps * eps
 *     --------------------------------------
 *      b[0] + b[1] * eps + b[2] * eps * eps
 *
 * This arises during calculation with perturbation.
 * The values of intersection times are of secondary interest; what matters more is `operator<=>` because it allows sorting.
 */
class IntersectionTime_Hybrid final
{
private:
    
    DepressedCubic a;
    DepressedCubic b;
    double t;
    
public:
    
    IntersectionTime_Hybrid() = default;
    
    IntersectionTime_Hybrid( cref<DepressedCubic> numerator, cref<DepressedCubic> denominator )
    {
        // Make sure at the time of initialization that the denominator is >= 0!
        // This is important for later < and > comparisons.
        
        if( Sign<Sign_T>(denominator) < Sign_T(0) )
        {
            a = -numerator;
            b = -denominator;
        }
        else
        {
            a = numerator;
            b = denominator;
        }
        
        t = ToDouble(a)/ToDouble(b);
    }
    
    friend std::strong_ordering operator<=>(
        cref<IntersectionTime_Hybrid> s, cref<IntersectionTime_Hybrid> t
    )
    {
        {
            // Computing the ration a / b of two integers has very low precision loss (empirically, way less then ten mu), provided 0 <= a <= b, which is the case here.
            // So we exploit here that comparison in `double` is good enough most of the time and that it is much faster than the wide integer arithmetic below.
            constexpr double tol = 1.4901161193847656e-8;
            
            const double delta = s.t - t.t;
            
            if( Abs(delta) > tol )
            {
                switch( Sign<Sign_T>(delta) )
                {
                    case Sign_T{-1}: return std::strong_ordering::less;
                    case Sign_T{ 1}: return std::strong_ordering::greater;
                    default        : return std::strong_ordering::equal;
                }
            }
        }
        
        // From hereon, we do the same as in IntersectionTime.hpp.
        
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
        LLInt delta;
        
        // Order 0
        // The leading order terms of the numerators and denominators should be nonnegative due to fact that the intersection times should lie in [0,1] to leading order and due the normalization of the ratios. Hence we can spare some conditionals and some bit twiddling by using long_mul_unsigned. Alas, the branch prediction seems to guess the branches very well, so I do not see much difference in the timings.
        
        assert(!s.a.c_0.NegativeQ());
        assert(!s.b.c_0.NegativeQ());
        assert(!t.a.c_0.NegativeQ());
        assert(!t.b.c_0.NegativeQ());
        
        lhs = long_mul_unsigned(s.a.c_0, t.b.c_0);
        rhs = long_mul_unsigned(s.b.c_0, t.a.c_0);
        delta = lhs - rhs;
        if( delta.NegativeQ() ) { return std::strong_ordering::less;    }
        if( !delta.ZeroQ()    ) { return std::strong_ordering::greater; }

//        if( lhs < rhs ) { return std::strong_ordering::less;    }
//        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        // For generic real inputs, it is very unlikely that we arrive here.
        
        // Order 1
        lhs = long_mul(s.a.c_0, t.b.c_1) + long_mul(s.a.c_1, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_1) + long_mul(s.b.c_1, t.a.c_0);
        delta = lhs - rhs;
        if( delta.NegativeQ() ) { return std::strong_ordering::less;    }
        if( !delta.ZeroQ()    ) { return std::strong_ordering::greater; }
        
        // Order 2
        lhs = long_mul(s.a.c_1, t.b.c_1);
        rhs = long_mul(s.b.c_1, t.a.c_1);
        delta = lhs - rhs;
        if( delta.NegativeQ() ) { return std::strong_ordering::less;    }
        if( !delta.ZeroQ()    ) { return std::strong_ordering::greater; }
        
        // Order 3
        lhs = long_mul(s.a.c_0, t.b.c_3) + long_mul(s.a.c_3, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_3) + long_mul(s.b.c_3, t.a.c_0);
        delta = lhs - rhs;
        if( delta.NegativeQ() ) { return std::strong_ordering::less;    }
        if( !delta.ZeroQ()    ) { return std::strong_ordering::greater; }
        
        // Order 4
        lhs = long_mul(s.a.c_1, t.b.c_3) + long_mul(s.a.c_3, t.b.c_1);
        rhs = long_mul(s.b.c_1, t.a.c_3) + long_mul(s.b.c_3, t.a.c_1);
        delta = lhs - rhs;
        if( delta.NegativeQ() ) { return std::strong_ordering::less;    }
        if( !delta.ZeroQ()    ) { return std::strong_ordering::greater; }
        
        // Order 5 -- not existent.
        
        // Order 6
        lhs = long_mul(s.a.c_3, t.b.c_3);
        rhs = long_mul(s.b.c_3, t.a.c_3);
        delta = lhs - rhs;
        if( delta.NegativeQ() ) { return std::strong_ordering::less;    }
        if( !delta.ZeroQ()    ) { return std::strong_ordering::greater; }
        
        // We should never come here.
        assert(false);
        
        return std::strong_ordering::equal;
    }
    
    friend double ToDouble( cref<IntersectionTime_Hybrid> T )
    {
        return T.t;
    }
    
    friend std::string ToString( cref<IntersectionTime_Hybrid> I )
    {
        return ToString(I.a) + " / " + ToString(I.b);
    }
    
}; // class IntersectionTime_Hybrid
