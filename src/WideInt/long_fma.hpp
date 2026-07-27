public:


/*!@brief Long fused multiply-add routine that computes `r = a * b + c` and returns `r` in a `WideInt` twice as wide as the inputs.*/
TOOLS_FORCE_INLINE constexpr friend
Prod_T long_fma( cref<WideInt> a, cref<WideInt> b, cref<WideInt> c )
{
    // See also https://stackoverflow.com/a/1815371/8248900.

    // We do not want to create a sign extension of a and b.
    if constexpr ( signQ )
    {
        // TODO: The fact that the sign of c also needs to be flipped makes the idea of fma less attractive. fused multiply-subtract also did not work better than switching the sign of c.
        
        if( a.NegativeQ() )
        {
            if( b.NegativeQ() )
            {
                return long_fma(-a,-b,c);
            }
            else
            {
                return -long_fma(-a,b,-c);
            }
        }
        else
        {
            if( b.NegativeQ() )
            {
                return -long_fma(a,-b,-c);
            }
        }
    }
    
    Prod_T r;

    for( Size_T i = 0; i < limb_count; ++i )
    {
        Comp_T X (c[i]);
        for( Size_T j = 0; j < limb_count; ++j )
        {
            // The following line cannot overflow because all 4 operands occupy only the lower half of a Comp_T. Let B = 2^LimbBitCount(). Then the operands are <= B-1.
            // So X <= (B-1) + (B-1) + (B-1) * (B-1) = (B+1) * (B-1) = B^2 - 1 <= 2^CompBitCount() - 1;
            X      = As_Comp(X + As_Comp(r[i+j]) + As_Comp(a[i]) * As_Comp(b[j]) );
            r[i+j] = Lo_Limb(X);
            X      = Hi_Comp(X);
        }
        r[i + limb_count] = Lo_Limb(X);
    }
    
    return r;
}




TOOLS_FORCE_INLINE constexpr friend
Prod_T long_fma_1( cref<WideInt> a, cref<WideInt> b, cref<WideInt> c )
{
    Prod_T r;
    const Comp_T X = As_Comp(a[0]) * As_Comp(b[0]) + As_Comp(c[0]);
    r[0] = Lo_Limb(X);
    r[1] = Hi_Limb(X);
    return r;
}


TOOLS_FORCE_INLINE constexpr friend 
Prod_T long_fma_2( cref<WideInt> a, cref<WideInt> b, cref<WideInt> c )
{
    Prod_T r;
    
    Comp_T AB_00 = As_Comp(a[0]) * As_Comp(b[0]);
    Comp_T AB_01 = As_Comp(a[0]) * As_Comp(b[1]);
    Comp_T AB_10 = As_Comp(a[1]) * As_Comp(b[0]);
    Comp_T AB_11 = As_Comp(a[1]) * As_Comp(b[1]);
    
    Comp_T X = AB_00 + As_Comp(c[0]);
    r[0]     = Lo_Limb(X);
    Comp_T Y = As_Comp(Hi_Comp(X) + AB_01 + As_Comp(c[1]));
    Comp_T Z = As_Comp(Lo_Comp(Y) + AB_10);
    r[1]     = Lo_Limb(Z);
    Comp_T W = As_Comp(Hi_Comp(Z) + Hi_Comp(Y) + AB_11);
    r[2]     = Lo_Limb(W);
    r[3]     = Hi_Limb(W);

    return r;
}
