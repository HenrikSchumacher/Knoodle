public:

struct Pair_T
{
    Limb_T value {0};
    Limb_T carry {0};
    
    constexpr Pair_T() = default;
    
    constexpr Pair_T( cref<Limb_T> v, cref<Limb_T> c )
    :   value {v}
    ,   carry {c}
    {}
    
    constexpr explicit Pair_T( cref<Dimb_T> x )
    :   value {Lo_Limb(x)}
    ,   carry {Hi_Limb(x)}
    {}
    
    friend std::string ToString( cref<Pair_T> p )
    {
        return std::string("{ .value = ") +ToString(p.value) + ", .carry = " + ToString(p.carry) +" }";
    }
};


TOOLS_FORCE_INLINE static constexpr
Dimb_T add_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b )
{
    return static_cast<Dimb_T>(Dimb_T{a} + Dimb_T{b});
}

TOOLS_FORCE_INLINE static constexpr
Pair_T add_as_Pair ( cref<Limb_T> a, cref<Limb_T> b )
{
    return Pair_T{add_as_Dimb(a,b)};
}


TOOLS_FORCE_INLINE static constexpr
Dimb_T add_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
{
    return static_cast<Dimb_T>(Dimb_T{a} + Dimb_T{b} + Dimb_T{c});
}

TOOLS_FORCE_INLINE static constexpr
Pair_T add_as_Pair ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
{
    return Pair_T{add_as_Dimb(a,b,c)};
}


TOOLS_FORCE_INLINE static constexpr
Dimb_T mul_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b )
{
    return Dimb_T{a} * Dimb_T{b};
}

TOOLS_FORCE_INLINE static constexpr
Pair_T mul_as_Pair ( cref<Limb_T> a, cref<Limb_T> b )
{
    return Pair_T{mul_as_Dimb(a,b)};
}


TOOLS_FORCE_INLINE static constexpr
Dimb_T fma_as_Dimb ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
{
    return (Dimb_T{a} * Dimb_T{b}) + Dimb_T{c};
}

TOOLS_FORCE_INLINE static constexpr
Pair_T fma_as_Pair ( cref<Limb_T> a, cref<Limb_T> b, cref<Limb_T> c )
{
    return Pair_T{fma_as_Dimb(a,b,c)};
}
