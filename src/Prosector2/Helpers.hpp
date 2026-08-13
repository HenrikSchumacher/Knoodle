public:

TOOLS_FORCE_INLINE static Sign_T Sign( const boost::multiprecision::int128_t & z )
{
    return static_cast<Sign_T>(z > 0) - static_cast<Sign_T>(z < 0);
}

TOOLS_FORCE_INLINE static Sign_T Sign( const boost::multiprecision::int256_t & z )
{
    return static_cast<Sign_T>(z > 0) - static_cast<Sign_T>(z < 0);
}

TOOLS_FORCE_INLINE static Sign_T Sign_Perturbed( cref<LVector3_T> cross_prod )
{
    if constexpr ( verboseQ ) { logprint(MethodName("Sign_Perturbed")); }
    Sign_T s = Sign(cross_prod[2]);
    if( s != Sign_T(0) ) { return s; }
    // In a generic situation, we will seldomly arrive at this point.
    s = Sign(cross_prod[0]);
    if( s != Sign_T(0) ) { return s; }
    s = Sign(cross_prod[1]);
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
    
    // Precondition: x_0 != x_1 and the two lines are colinear.
    // Find coordinate direction k so that x_0[k] != x_1[k];
    int k = 0;
    while( (x_0[k] == x_1[k]) && (k < 3) ) { ++k; };
    
    if( k == 3 )
    {
        if constexpr ( verboseQ )
        {
            eprint(MethodName("LinesColinearTest") + ": Line segment is denegerate.");
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
