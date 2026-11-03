public:

class Z4
{
    using Comp_T    = std::int_fast8_t;
    using Storage_T = std::uint8_t;
    
private:
    
    Storage_T value = 0;
    
public:
    
    constexpr Z4() = default;
    ~Z4() = default;
    
    template<std::integral I> requires (!std::same_as<I, bool>)
    constexpr Z4( I x )
    {
        Comp_T y = static_cast<Comp_T>(x % Int{4});
        value = static_cast<Storage_T>( (y < Comp_T{0}) ? y + Comp_T{4} : y );
    }
    
public:
    
    inline friend constexpr Z4 operator+( Z4 a, Z4 b )
    {
        Z4 c;
        c.value = static_cast<Storage_T>(static_cast<Comp_T>(a.value) + static_cast<Comp_T>(b.value)) & Storage_T{3};
        return c;
    }
    
    inline friend constexpr Z4 operator-( Z4 a, Z4 b )
    {
        Z4 c;
        c.value = static_cast<Storage_T>(static_cast<Comp_T>(a.value) + Comp_T{4} - static_cast<Comp_T>(b.value)) & Storage_T{3};
        return c;
    }

    inline friend constexpr Z4 operator*( Z4 a, Z4 b )
    {
        Z4 c;
        c.value = static_cast<Storage_T>(static_cast<Comp_T>(a.value) * static_cast<Comp_T>(b.value)) & Storage_T{3};
        return c;
    }

    inline friend constexpr bool operator==( Z4 a, Z4 b )
    {
        return a.r == b.r;
    }
    
    inline friend constexpr Z4 operator-( Z4 a )
    {
        Z4 c;
        c.value = static_cast<Storage_T>(Comp_T{4} - static_cast<Comp_T>(a.value)) & Storage_T{3};
        return c;
    }
    
    operator Int () const
    {
        return static_cast<Int>(r);
    }
    
    operator Size_T () const
    {
        return static_cast<Size_T>(value);
    }
    
    friend std::string ToString( Z4 z )
    {
        std::string s ( "(" );
        s += ToString(z.value);
        s += " mod 4)";
        return s;
    }

}; // struct Z4
