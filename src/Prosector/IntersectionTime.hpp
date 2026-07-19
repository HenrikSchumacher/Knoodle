public:

class IntersectionTime final
{
public:
    
    // The intersection time is represented as a rational function of the form
    //
    //      a[0] + a[1] * eps + a[2] * eps * eps
    //     --------------------------------------
    //      b[0] + b[1] * eps + b[2] * eps * eps
    //
    // This arises during calculation with perturbation.
    // The values of intersection times are of secondary interest; what matters more is that the relations < and > are declared and accurate.
    
private:
    
    Polynomial3 a;
    Polynomial3 b;
    
public:
    
    IntersectionTime() = default;
    
    IntersectionTime( cref<Polynomial3> numerator, cref<Polynomial3> denominator )
    {
        // Make sure at the time of initialization that the denominator is >= 0!
        // This is important for later < and > comparisons.
        
        if( denominator.Sign() < Sign_T(0) )
        {
//            a.c_0 = -numerator.c_0;
//            a.c_1 = -numerator.c_1;
//            a.c_3 = -numerator.c_2;
//            b.c_0 = -denominator.c_0;
//            b.c_1 = -denominator.c_1;
//            b.c_3 = -denominator.c_2;
            
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
    
    double ToDouble() const
    {
        return a.ToDouble() / b.ToDouble();
    }
  
    friend constexpr std::strong_ordering operator<=>(
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
        
        const LLInt s_a_0 {s.a.c_0};
        const LLInt s_b_0 {s.b.c_0};
        const LLInt t_a_0 {t.a.c_0};
        const LLInt t_b_0 {t.b.c_0};
        
        // Order 0
        lhs = s_a_0 * t_b_0;
        rhs = s_b_0 * t_a_0;
        if( lhs < rhs ) { return std::strong_ordering::less;    }
        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        // For generic real inputs, it is very unlinkly that we arrive here.
        
        const LLInt s_a_1 {s.a.c_1};
        const LLInt s_b_1 {s.b.c_1};
        const LLInt t_a_1 {t.a.c_1};
        const LLInt t_b_1 {t.b.c_1};
        
        // Order 1
        lhs = s_a_0 * t_b_1 + s_a_1 * t_b_0;
        rhs = s_b_0 * t_a_1 + s_b_1 * t_a_0;
        if( lhs < rhs ) { return std::strong_ordering::less;    }
        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        const LLInt s_a_3 {s.a.c_3};
        const LLInt s_b_3 {s.b.c_3};
        const LLInt t_a_3 {t.a.c_3};
        const LLInt t_b_3 {t.b.c_3};
        
        // Order 2
        lhs = s_a_1 * t_b_1;
        rhs = s_b_1 * t_a_1;
        if( lhs < rhs ) { return std::strong_ordering::less;    }
        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        // Order 3
        lhs = s_a_0 * t_b_3 + s_a_3 * t_b_0;
        rhs = s_b_0 * t_a_3 + s_b_3 * t_a_0;
        if( lhs < rhs ) { return std::strong_ordering::less;    }
        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        // Order 4
        lhs = s_a_1 * t_b_3 + s_a_3 * t_b_1;
        rhs = s_b_1 * t_a_3 + s_b_3 * t_a_1;
        if( lhs < rhs ) { return std::strong_ordering::less;    }
        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        // Order 5 -- not existent.
        
        // Order 6
        lhs = s_a_3 * t_b_3;
        rhs = s_b_3 * t_a_3;
        if( lhs < rhs ) { return std::strong_ordering::less;    }
        if( lhs > rhs ) { return std::strong_ordering::greater; }
        
        return std::strong_ordering::equal;
    }
    
    friend std::string ToString( cref<IntersectionTime> I )
    {
        return ToString(I.a) + " / " + ToString(I.b);
    }
};
