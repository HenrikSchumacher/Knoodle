public:
    
/*!@brief Long multiply routine that computes `r = a * b` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs.*/
template<int other_limb_count>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+other_limb_count,Limb_T,Comp_T,signQ>
long_mul( cref<WideInt> a, cref<WideInt<other_limb_count,Limb_T,Comp_T,signQ>> b )
{
    constexpr WideInt c ( Limb_T{0} );
    return long_fma(a,b,c);
}

/*!@brief Long multiply routine that computes `r = a * b` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs.  CAUTION: The result will be only correct if operands `a` and `b` a _nonnegative_.*/
template<int other_limb_count>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+other_limb_count,Limb_T,Comp_T,signQ>
long_mul_unsigned( cref<WideInt> a, cref<WideInt<other_limb_count,Limb_T,Comp_T,signQ>> b )
{
    constexpr WideInt c ( Limb_T{0} );
    return long_fma_unsigned(a,b,c);
}


// Overloads for combining with basic integral types.


/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<IntQ Int>
TOOLS_FORCE_INLINE constexpr friend WideInt<limb_count+1,Limb_T,Comp_T,signQ>
long_mul( cref<WideInt> a, cref<Int> b )
{
    using W = WideInt<sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>;
    return long_mul( a, W{b} );
}

/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<IntQ Int>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>
long_mul( cref<Int> a, cref<WideInt> b )
{
    using W = WideInt<sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>;
    return long_mul( b, W{a} );
}
