public:

class Z3
{
    using Comp_T    = std::int_fast8_t;
    using Storage_T = std::uint8_t;
    
private:
    
    Storage_T value = 0;
    
public:
    
    constexpr Z3() = default;
    ~Z3() = default;
    
    template<std::integral I> requires (!std::same_as<I, bool>)
    constexpr Z3( I x )
    {
        using U = std::make_unsigned_t<I>;
        
        if( static_cast<U>(x) < U{3} ) // Casting to signed type deals with the check x < 0, too.
        {
            value = static_cast<Storage_T>(x);
        }
        else
        {
            Comp_T y = static_cast<Comp_T>(x % I{3});
            value = static_cast<Storage_T>( (y < Comp_T{0}) ? y + Comp_T{3} : y );
        }
    }
    
public:
    
    inline friend constexpr Z3 operator+( Z3 a, Z3 b )
    {
        Z3 c;
        Comp_T sum = static_cast<Comp_T>(a.value) + static_cast<Comp_T>(b.value);
        c.value = static_cast<Storage_T>( (sum > Comp_T{2}) ? sum - Comp_T{3} : sum );
        return c;
    }
    
    inline friend constexpr Z3 operator-( Z3 a, Z3 b )
    {
        Z3 c;
        Comp_T sum = static_cast<Comp_T>(a.value) - static_cast<Comp_T>(b.value);
        c.value = static_cast<Storage_T>( (sum < Comp_T{0}) ? sum + Comp_T{3} : sum );
        return c;
    }

    inline friend constexpr Z3 operator*( Z3 a, Z3 b )
    {
        Z3 c;
        Comp_T prod = static_cast<Comp_T>(a.value) * static_cast<Comp_T>(b.value);
        c.value = static_cast<Storage_T>( (prod > Comp_T{2}) ? prod - Comp_T{3} : prod );
        return c;
    }

    inline friend constexpr bool operator==( Z3 a, Z3 b )
    {
        return a.value == b.value;
    }
    
    inline friend constexpr Z3 operator-( Z3 a )
    {
        Z3 c;
        Comp_T minus = -static_cast<Comp_T>(a.value);
        c.value = static_cast<Storage_T>( (minus < Comp_T{0}) ? minus + Comp_T{3} : minus );
        return c;
    }
    
    operator Int () const
    {
        return static_cast<Int>(value);
    }
    
    operator Size_T () const
    {
        return static_cast<Size_T>(value);
    }
    
    friend std::string ToString( Z3 a )
    {
        std::string s ( "(" );
        s += ToString(a.value);
        s += " mod 3)";
        return s;
    }

}; // struct Z3
