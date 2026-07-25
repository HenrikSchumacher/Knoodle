public:

TOOLS_FORCE_INLINE static Sign_T Sign( const Int128 & z )
{
    return static_cast<Sign_T>(z > 0) - static_cast<Sign_T>(z < 0);
}

TOOLS_FORCE_INLINE static Sign_T Sign( const Int256 & z )
{
    return static_cast<Sign_T>(z > 0) - static_cast<Sign_T>(z < 0);
}


TOOLS_FORCE_INLINE static LVector3_T Cross( cref<Vector3_T> x, cref<Vector3_T> y )
{
    LInt x_0 { x[0] };
    LInt x_1 { x[1] };
    LInt x_2 { x[2] };
    
    LInt y_0 { y[0] };
    LInt y_1 { y[1] };
    LInt y_2 { y[2] };
    
    LVector3_T result {
        x_1 * y_2 - x_2 * y_1,
        x_2 * y_0 - x_0 * y_2,
        x_0 * y_1 - x_1 * y_0
    };
    if constexpr ( verboseQ )
    {
        logprint(MethodName("Cross")+ " -> { " + Tools::ToString(result[0]) +  ", " + Tools::ToString(result[1]) + ", " + Tools::ToString(result[2]) + " }" );
    }
    return result;
}

static Polynomial3 Det_Perturbed( cref<Vector3_T> x, cref<Vector3_T> y )
{
    LVector3_T z = Prosector::Cross(x,y);
    
    return Polynomial3{ z[2], z[0], z[1] };
}

TOOLS_FORCE_INLINE static Sign_T DetSign( Int a, Int b, Int c, Int d )
{
    const LInt det = LInt{a} * LInt{d} - LInt{b} * LInt{c};
    if( det > 0 ) { return Sign_T( 1); }
    if( det < 0 ) { return Sign_T(-1); }
    return Sign_T(0);
}

// Computes the limit of the determinant of u and v after being projected to the x-y-plane along the perturbed vector `{eps, eps * eps, 1}` for eps -> 0 from the right.

TOOLS_FORCE_INLINE static Sign_T DetSign_Perturbed( cref<Vector3_T> u, cref<Vector3_T> v )
{
    if constexpr ( verboseQ ) { logprint(MethodName("DetSign_Perturbed")); }
    Sign_T sign;
    
    if constexpr ( verboseQ ) {logprint("a"); }
    sign = DetSign(u[0],u[1],v[0],v[1]);
    if( sign != Sign_T(0) ) { return sign; }
    
    // In a generic situation, we will seldomly arrive at this point.
    if constexpr ( verboseQ ) { logprint("b"); }
    sign = DetSign(u[1],u[2],v[1],v[2]);
    if( sign != Sign_T(0) ) { return sign; }
    
    if constexpr ( verboseQ ) { logprint("c"); }
    sign = DetSign(u[2],u[0],v[2],v[0]);
    if( sign != Sign_T(0) ) { return sign; }
    
    // u and v a collinear in 3-space.
    
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
