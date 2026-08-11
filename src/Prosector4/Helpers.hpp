public:


//TOOLS_FORCE_INLINE static LVector3_T cross( cref<Vector3_T> a, cref<Vector3_T> b )
//{
//    return LVector3_T {
//        long_det(a[1],a[2],b[1],b[2]),
//        long_det(a[2],a[0],b[2],b[0]),
//        long_det(a[0],a[1],b[0],b[1])
//    };
//}

//TOOLS_FORCE_INLINE static Sign_T Sign_Perturbed( cref<LVector3_T> cross_prod )
//{
//    if constexpr ( verboseQ ) { logprint(MethodName("Sign_Perturbed")); }
//    Sign_T s = Sign(cross_prod[2]);
//    if( s != Sign_T(0) ) { return s; }
//    // In a generic situation, we will seldomly arrive at this point.
//    s = Sign(cross_prod[0]);
//    if( s != Sign_T(0) ) { return s; }
//    s = Sign(cross_prod[1]);
//    if( s != Sign_T(0) ) { return s; }
//    return Sign_T(0);
//}

TOOLS_FORCE_INLINE static Polynomial3 Det_Perturbed( cref<Vector3_T> a, cref<Vector3_T> b )
{
    return Polynomial3 {
        long_det(a[0],a[1],b[0],b[1]),
        long_det(a[1],a[2],b[1],b[2]),
        long_det(a[2],a[0],b[2],b[0])
    };
}

// Lazy evaluation of the signs of the determinants.
TOOLS_FORCE_INLINE static
Sign_T Sign_Perturbed( cref<Vector3_T> a, cref<Vector3_T> b )
{
    if constexpr ( verboseQ ) { logprint(MethodName("Sign_Perturbed")); }
    
    Sign_T s = Sign(long_det(a[0],a[1],b[0],b[1]));
    if( s != Sign_T(0) ) { return s; }
    // In a generic situation, we will seldomly arrive at this point.
    s = Sign(long_det(a[1],a[2],b[1],b[2]));
    if( s != Sign_T(0) ) { return s; }
    s = Sign(long_det(a[2],a[0],b[2],b[0]));
    if( s != Sign_T(0) ) { return s; }
    return Sign_T(0);
}

// Lazy evaluation of the determinants.
TOOLS_FORCE_INLINE static
std::pair<Sign_T,LInt> Sign_Det_Perturbed( cref<Vector3_T> a, cref<Vector3_T> b )
{
    if constexpr ( verboseQ ) { logprint(MethodName("Sign_Det_Perturbed")); }
    
    LInt det = long_det(a[0],a[1],b[0],b[1]);
    Sign_T s = Sign(det);
    if( s != Sign_T(0) ) { return {s,det}; }
    // In a generic situation, we will seldomly arrive at this point.
    s = Sign(long_det(a[1],a[2],b[1],b[2]));
    if( s != Sign_T(0) ) { return {s,det}; }
    s = Sign(long_det(a[2],a[0],b[2],b[0]));
    if( s != Sign_T(0) ) { return {s,det}; }
    return {Sign_T(0),det};
}


// Lazy evaluation of the signs of the determinants, using double arithmetic.
TOOLS_FORCE_INLINE static
Sign_T Sign_Perturbed_Kahan( cref<Vector3_T> a, cref<Vector3_T> b )
{
    if constexpr ( verboseQ ) { logprint(MethodName("Sign_Perturbed_Kahan")); }
    Sign_T s;
    s = DetSign2D_Kahan<Sign_T>(double(a[0]),double(a[1]),double(b[0]),double(b[1]));
    if( s != Sign_T(0) ) { return s; }
    // In a generic situation, we will seldomly arrive at this point.
    s = DetSign2D_Kahan<Sign_T>(double(a[1]),double(a[2]),double(b[1]),double(b[2]));
    if( s != Sign_T(0) ) { return s; }
    s = DetSign2D_Kahan<Sign_T>(double(a[2]),double(a[0]),double(b[2]),double(b[0]));
    if( s != Sign_T(0) ) { return s; }
    return Sign_T(0);
}

private:

bool PointOnLineTest( cref<Vector3_T> z, cref<Vector3_T> a_0, cref<Vector3_T> a_1 )
{
    if constexpr ( verboseQ ) { logprint(MethodName("PointOnLineTest")); }
    // Precondition: a_0 != a_1 and z lies on the line through a_0 and a_1.
    // Find coordinate direction k so that a_0[k] != a_1[k];
    int k = 0;
    while((a_0[k] == a_1[k]) && (k < 3)) { ++k; };
    
    if( k == 3 )
    {
        if constexpr ( verboseQ )
        {
            eprint(MethodName("PointOnLineTest") + ": Line segment is denegerate.");
        }
        return true;
    }
    
    auto [a,b] = MinMax(a_0[k],a_1[k]);
    
    if( (a <= z[k]) && (z[k] <= b) )
    {
        if constexpr ( verboseQ )
        {
            logprint("Point lies in line segment.");
        }
        return true;
    }
    else
    {
        if constexpr ( verboseQ )
        {
            logprint("Point does not lie on line segment.");
        }
        return false;
    }
}

bool LinesColinearTest()
{
    if constexpr ( verboseQ ) { logprint(MethodName("LinesColinearTest")); }
    
    // Precondition: The two lines are colinear.
    // Find coordinate direction `k` so that `x_0[k] != x_1[k]` or `y_0[k] != y_1[k]`;
    int k = 0;
    while( (x_0[k] == x_1[k]) && (y_0[k] == y_1[k]) && (k < 3) ) { ++k; };
    
    if( k == 3 )
    {
        if constexpr ( verboseQ )
        {
            eprint(MethodName("LinesColinearTest") + ": Both line segments are denegerate.");
        }
        return true;
    }
    
    auto [a,b] = MinMax(x_0[k],x_1[k]);
    auto [c,d] = MinMax(y_0[k],y_1[k]);
    
    // Check whether intervals [a,b] and [c,d] intersect.
    bool result =  (a <= d) && (c <= b);
    
    if constexpr ( verboseQ )
    {
        logprint(MethodName("LinesColinearTest") + (result ? "Line segments intersect." : "Line segments do not intersect."));
    }
    
    return result;
}


//bool LinesColinearTest()
//{
//    // Precondition: The two lines are colinear.
//    
//    if constexpr ( verboseQ ) { logprint(MethodName("LinesColinearTest")); }
//    
//    for( int k = 0; k < 3; ++k )
//    {
//        if( (x_0[k] == x_1[k]) && (y_0[k] == y_1[k]) ) { continue; }
//        
//        // If we arrive here, then at least one of the lines is nondenerate and we have
//        // `x_0[k] != x_1[k]` or `y_0[k] != y_1[k]`.
//        // So at least one of the following intervals is nondegnerate.
//        
//        auto [a,b] = MinMax(x_0[k],x_1[k]);
//        auto [c,d] = MinMax(y_0[k],y_1[k]);
//        
//        // Check whether intervals [a,b] and [c,d] intersect.
//        const bool result =  (a <= d) && (c <= b);
//        
//        if constexpr ( verboseQ )
//        {
//            logprint(MethodName("LinesColinearTest") + (result ? "Line segments intersect." : "Line segments do not intersect."));
//        }
//        
//        return result;
//    }
//
//    if constexpr ( verboseQ )
//    {
//        eprint(MethodName("LinesColinearTest") + ": Both line segments are denegerate.");
//    }
//    // If we arrive here, then both intervals are degenerate and colinear. So their intersection is one point, namely `x_0 == x_1 == y_0 == y_1`.
//    return true;
//}
