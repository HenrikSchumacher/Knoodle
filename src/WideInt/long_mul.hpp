public:
    
TOOLS_FORCE_INLINE constexpr friend  Prod_T long_mul( cref<WideInt> a, cref<WideInt> b )
{
    // See also https://stackoverflow.com/a/1815371/8248900.
    
    constexpr WideInt c ( Limb_T{0} );
    
    return long_fma(a,b,c);
    
//    // Beware, the follwoing is incorrect if a or b are negative.
//    
//    if constexpr ( limb_count == Idx(1) )
//    {
//        return long_mul_1(a,b);
//    }
//    
//    if constexpr ( limb_count == Idx(2) )
//    {
//        return long_mul_2(a,b);
//    }
    
//    Prod_T r {};
//    
//    for( Idx i = 0; i < limb_count; ++i )
//    {
//        Comp_T X = 0;
//        for( Idx j = 0; j < limb_count; ++j )
//        {
//            X      = As_Comp(X + As_Comp(r[i+j]) + As_Comp(a[i]) * As_Comp(b[j]) );
//            r[i+j] = Lo_Limb(X);
//            X      = Hi_Comp(X);
//        }
//        r[i + limb_count] = Lo_Limb(X);
//    }
//    
//    return r;
}


//private:
//
//TOOLS_FORCE_INLINE constexpr friend  Prod_T long_mul_1( cref<WideInt> a, cref<WideInt> b )
//{
//    Prod_T r;
//    const Comp_T X = As_Comp(a[0]) * As_Comp(b[0]);
//    r[0] = Lo_Limb(X);
//    r[1] = Hi_Limb(X);
//    return r;
//}
//
//TOOLS_FORCE_INLINE constexpr friend  Prod_T long_mul_2( cref<WideInt> a, cref<WideInt> b )
//{
//    Prod_T r;
//    Comp_T R_2;
//    Comp_T X;
//    X    = As_Comp(As_Comp(a[0]) * As_Comp(b[0]));
//    r[0] = Lo_Limb(X);
//    X    = As_Comp(As_Comp(a[0]) * As_Comp(b[1]) + Hi_Comp(X));
//    R_2  = Hi_Comp(X);
//    X    = As_Comp(As_Comp(a[1]) * As_Comp(b[0]) + Lo_Comp(X));
//    r[1] = Lo_Limb(X);
//    X    = As_Comp(As_Comp(a[1]) * As_Comp(b[1]) + Hi_Comp(X) + R_2);
//    r[2] = Lo_Limb(X);
//    r[3] = Hi_Limb(X);
//    return r;
//}
