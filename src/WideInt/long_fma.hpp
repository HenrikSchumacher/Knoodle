public:


/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<int other_limb_count>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+other_limb_count,Limb_T,Comp_T,signQ>
long_fma(
    cref<WideInt> a,
    cref<WideInt<other_limb_count,Limb_T,Comp_T,signQ>> b,
    cref<WideInt> c
)
{
    // We do not want to create a sign extension of a and b.
    if constexpr ( signQ )
    {
        // TODO: The fact that the sign of c also needs to be flipped makes the idea of fma less attractive. fused multiply-subtract also did not work better than switching the sign of c.
        
        if( a.NegativeQ() )
        {
            if( b.NegativeQ() )
            {
                return long_fma_unsigned(-a,-b,c);
            }
            else
            {
                return -long_fma_unsigned(-a,b,-c);
            }
        }
        else
        {
            if( b.NegativeQ() )
            {
                return -long_fma_unsigned(a,-b,-c);
            }
            else
            {
                return long_fma_unsigned(a,b,c);
            }
        }
    }
}

/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs. CAUTION: The result will be only correct if operands `a` and `b` are _nonnegative_.*/
template</*bool checkedQ = true,*/ int other_limb_count>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+other_limb_count,Limb_T,Comp_T,signQ>
long_fma_unsigned(
    cref<WideInt> a,
    cref<WideInt<other_limb_count,Limb_T,Comp_T,signQ>> b,
    cref<WideInt> c
)
{
    assert(!a.NegativeQ());
    assert(!b.NegativeQ());
    
    constexpr Idx m = limb_count;
    constexpr Idx n = static_cast<Idx>(other_limb_count);

    using Result_T  = WideInt<m+n,Limb_T,Comp_T,signQ>;
    
    // See also https://stackoverflow.com/a/1815371/8248900.
    
    Result_T r;
    
    Comp_T X;
    for( Idx i = 0; i < m; ++i )
    {
        X = As_Comp(c[i]);
        for( Idx j = 0; j < n; ++j )
        {
            // The following line cannot overflow because each of the four operands occupy only the lower half of a Comp_T. Proof: Let B = 2^LimbBitCount(). Then each of the four operands is <= B-1.
            // Hence, X <= (B-1) + (B-1) + (B-1) * (B-1) = (B+1) * (B-1) = B^2 - 1 <= 2^CompBitCount() - 1;
            X      = As_Comp(X + As_Comp(r[i+j]) + As_Comp(a[i]) * As_Comp(b[j]) );
            r[i+j] = Lo_Limb(X);
            X      = Hi_Comp(X);
        }
        r[i + n] = Lo_Limb(X);
    }
    
//    if constexpr ( checkedQ  )
//    {
//        if( Hi_Comp(X) != Limb_T{0} )
//        {
//            error("long_fma_unsigned: Overflow");
//        }
//    }
    
    return r;
}



// Overloads for combining with basic integral types.

/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<IntQ Int>
TOOLS_FORCE_INLINE constexpr friend
WideInt<limb_count+sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>
long_fma( cref<Int> a, cref<WideInt> b, cref<Int> c )
{
    using W = WideInt<sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>;
    return long_fma( W{a}, b, W{c} );
}

/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<IntQ Int>
TOOLS_FORCE_INLINE constexpr friend WideInt<limb_count+1,Limb_T,Comp_T,signQ>
long_fma( cref<WideInt> a, cref<Int> b, cref<Int> c )
{
    using W = WideInt<sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>;
    return long_fma( W{b}, a, W{c} );
}

/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<IntQ Int>
TOOLS_FORCE_INLINE constexpr friend WideInt<limb_count+1,Limb_T,Comp_T,signQ>
long_fma( cref<Int> a, cref<WideInt> b, cref<WideInt> c )
{
    using W = WideInt<sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>;
    return long_fma( b, W{a}, c );
}

/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` of appropriate size so that no overflow occurs*/
template<IntQ Int>
TOOLS_FORCE_INLINE constexpr friend WideInt<limb_count+1,Limb_T,Comp_T,signQ>
long_fma( cref<WideInt> a, cref<Int> b, cref<WideInt> c )
{
    using W = WideInt<sizeof(Int)/sizeof(Limb_T),Limb_T,Comp_T,signQ>;
    return long_fma( a, W{b}, c );
}













//TOOLS_FORCE_INLINE constexpr friend
//Prod_T long_fma_1( cref<WideInt> a, cref<WideInt> b, cref<WideInt> c )
//{
//    Prod_T r;
//    const Comp_T X = As_Comp(a[0]) * As_Comp(b[0]) + As_Comp(c[0]);
//    r[0] = Lo_Limb(X);
//    r[1] = Hi_Limb(X);
//    return r;
//}
//
//
//TOOLS_FORCE_INLINE constexpr friend 
//Prod_T long_fma_2( cref<WideInt> a, cref<WideInt> b, cref<WideInt> c )
//{
//    Prod_T r;
//    
//    Comp_T AB_00 = As_Comp(a[0]) * As_Comp(b[0]);
//    Comp_T AB_01 = As_Comp(a[0]) * As_Comp(b[1]);
//    Comp_T AB_10 = As_Comp(a[1]) * As_Comp(b[0]);
//    Comp_T AB_11 = As_Comp(a[1]) * As_Comp(b[1]);
//    
//    Comp_T X = AB_00 + As_Comp(c[0]);
//    r[0]     = Lo_Limb(X);
//    Comp_T Y = As_Comp(Hi_Comp(X) + AB_01 + As_Comp(c[1]));
//    Comp_T Z = As_Comp(Lo_Comp(Y) + AB_10);
//    r[1]     = Lo_Limb(Z);
//    Comp_T W = As_Comp(Hi_Comp(Z) + Hi_Comp(Y) + AB_11);
//    r[2]     = Lo_Limb(W);
//    r[3]     = Hi_Limb(W);
//
//    return r;
//}


