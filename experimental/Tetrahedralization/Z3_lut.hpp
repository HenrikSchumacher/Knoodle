public:

class Z3
{
    using Value_T = int;
    
private:
    
    Value_T r = 0;
    
public:
    
    constexpr Z3() = default;
    ~Z3() = default;
    
    template<IntQ I>
    constexpr Z3( I x )
    {
        r = x % Int{3};
        r += Int{r < Int{0}} * Int{3};
    }
    
private:
    
    static constexpr Value_T mod3 [6] = { 0, 1, 2, 0, 1, 2 };
    
public:
    
    inline friend constexpr Z3 operator+( Z3 a, Z3 b )
    {
        Z3 c;
        c.r = mod3[a.r + b.r];
        return c;
    }
    
    inline friend constexpr Z3 operator-( Z3 a, Z3 b )
    {
        Z3 c;
        c.r = mod3[Value_T{3} + a.r - b.r];
        return c;
    }

    inline friend constexpr Z3 operator*( Z3 a, Z3 b )
    {
        Z3 c;
        c.r = mod3[a.r * b.r];
        return c;
    }

    inline friend constexpr bool operator==( Z3 a, Z3 b )
    {
        return a.r == b.r;
    }
    
    inline friend constexpr Z3 operator-( Z3 a )
    {
        Z3 c;
        c.r = mod3[Value_T{3} - a.r];
        return c;
    }
    
    operator Int () const
    {
        return static_cast<Int>(r);
    }
    
    operator Size_T () const
    {
        return static_cast<Size_T>(r);
    }
    
    friend std::string ToString( Z3 a )
    {
        std::string s ( "(" );
        s += ToString(a.r);
        s += " mod 3)";
        return s;
    }

}; // struct Z3
