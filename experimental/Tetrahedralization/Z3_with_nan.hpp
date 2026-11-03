public:

class Z3
{
    using UInt =  UInt8;
    
private:
    
    UInt r = 0;
    
public:
    
    Z3() = default;
    ~Z3() = default;
    
    template<IntQ I>
    Z3( I x )
    {
        switch (x)
        {
            case I{0}:
            {
                r = 0;
                return;
            }
            case I{1}:
            {
                r = 1;
                return;
            }
            case I{2}:
            {
                r = 2;
                return;
            }
            default:
            {
                r = 3;
                return;
            }
        }
    }
    
    constexpr bool ValidQ() const
    {
        return (r < UInt{3});
    }
    
    inline friend constexpr Z3 operator+( Z3 a, Z3 b )
    {
        constexpr UInt lut [4][4] = {{0, 1, 2, 3}, {1, 2, 0, 3}, {2, 0, 1, 3}, {3, 3, 3, 3}};
        Z3 c;
        c.r = lut[a.r][b.r];
        return c.r;
    }
    
    inline friend constexpr Z3 operator-( Z3 a, Z3 b )
    {
        constexpr UInt lut [4][4] = {{0, 2, 1, 3}, {1, 0, 2, 3}, {2, 1, 0, 3}, {3, 3, 3, 3}};
        Z3 c;
        c.r = lut[a.r][b.r];
        return c.r;
    }
    
    inline friend constexpr Z3 operator-( Z3 a )
    {
        constexpr UInt lut [4] = {0, 2, 1, 3};
        Z3 c;
        c.r = lut[a.r];
        return c.r;
    }

    friend constexpr bool operator==( Z3 a, Z3 b )
    {
        return a.r == b.r;
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
        if( !a.ValidQ() ) { return "nan"; }
        
        std::string s ( "(" );
        s += ToString(a.r);
        s += " mod 3)";
        return s;
    }


}; // struct Z3
