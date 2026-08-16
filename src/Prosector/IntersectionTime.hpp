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
    
    DepressedCubic a;
    DepressedCubic b;
    
public:
    
    IntersectionTime() = default;
    
    IntersectionTime( cref<DepressedCubic> numerator, cref<DepressedCubic> denominator )
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
    }
    
//        template<typename ExtInt>
//        IntersectionTime(
//            cref<ExtInt> a_0, cref<ExtInt> a_1, cref<ExtInt> a_2,
//            cref<ExtInt> b_0, cref<ExtInt> b_1, cref<ExtInt> b_2
//        )
//        :   IntersectionTime{ DepressedCubic{a_0,a_1,a_2}, DepressedCubic{b_0,b_1,b_2} }
//        {}
    

    friend double ToDouble( cref<IntersectionTime> t )
    {
        return ToDouble(t.a) / ToDouble(t.b);
    }
    
    
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
        LLInt delta;
        
        // Order 0
        // TODO: The leading order terms of the numerators and denominators should be nonnegative due to fact that the intersection times should lie in [0,1] to leading order and due the normalization of the ratios. Hence we can spare some conditionals and some bit twiddling by using long_mul_unsigned. Alas, the branch prediction seems to guess the branches very well, so I do not see much difference in the timings.
        
//        assert(!NegativeQ(s.a.c_0));
//        assert(!NegativeQ(s.b.c_0));
//        assert(!NegativeQ(t.a.c_0));
//        assert(!NegativeQ(t.b.c_0));
        
        lhs = long_mul(s.a.c_0, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_0);
        delta = lhs - rhs;
        if( NegativeQ(delta) ) { return std::strong_ordering::less;    }
        if( !ZeroQ(delta)    ) { return std::strong_ordering::greater; }

//        if( lhs < rhs ) { return std::strong_ordering::less;    }
//        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        // For generic real inputs, it is very unlikely that we arrive here.
        
        // Order 1
        lhs = long_mul(s.a.c_0, t.b.c_1) + long_mul(s.a.c_1, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_1) + long_mul(s.b.c_1, t.a.c_0);
        delta = lhs - rhs;
        if( NegativeQ(delta) ) { return std::strong_ordering::less;    }
        if( !ZeroQ(delta)    ) { return std::strong_ordering::greater; }
        
        // Order 2
        lhs = long_mul(s.a.c_1, t.b.c_1);
        rhs = long_mul(s.b.c_1, t.a.c_1);
        delta = lhs - rhs;
        if( NegativeQ(delta) ) { return std::strong_ordering::less;    }
        if( !ZeroQ(delta)    ) { return std::strong_ordering::greater; }
            
        // Order 3
        lhs = long_mul(s.a.c_0, t.b.c_3) + long_mul(s.a.c_3, t.b.c_0);
        rhs = long_mul(s.b.c_0, t.a.c_3) + long_mul(s.b.c_3, t.a.c_0);
        delta = lhs - rhs;
        if( NegativeQ(delta) ) { return std::strong_ordering::less;    }
        if( !ZeroQ(delta)    ) { return std::strong_ordering::greater; }

        // Order 4
        lhs = long_mul(s.a.c_1, t.b.c_3) + long_mul(s.a.c_3, t.b.c_1);
        rhs = long_mul(s.b.c_1, t.a.c_3) + long_mul(s.b.c_3, t.a.c_1);
        delta = lhs - rhs;
        if( NegativeQ(delta) ) { return std::strong_ordering::less;    }
        if( !ZeroQ(delta)    ) { return std::strong_ordering::greater; }
        
        // Order 5 -- not existent.

        // Order 6
        lhs = long_mul(s.a.c_3, t.b.c_3);
        rhs = long_mul(s.b.c_3, t.a.c_3);
        delta = lhs - rhs;
        if( NegativeQ(delta) ) { return std::strong_ordering::less;    }
        if( !ZeroQ(delta)    ) { return std::strong_ordering::greater; }
        
        // We should never come here.
        assert(false);

        return std::strong_ordering::equal;
    }
    
    friend std::string ToString( cref<IntersectionTime> I )
    {
        return ToString(I.a) + " / " + ToString(I.b);
    }
    
}; // class IntersectionTime
