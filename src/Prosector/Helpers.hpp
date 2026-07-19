public:

static Sign_T Sign( const LInt z )
{
    return static_cast<Sign_T>(z > LInt{0}) - static_cast<Sign_T>(z < LInt{0});
}

static Sign_T Sign( const LLInt z )
{
    return static_cast<Sign_T>(z > LLInt{0}) - static_cast<Sign_T>(z < LLInt{0});
}


static LVector3_T Cross( cref<Vector3_T> x, cref<Vector3_T> y )
{
    LInt x_0 { x[0] };
    LInt x_1 { x[1] };
    LInt x_2 { x[2] };
    
    LInt y_0 { y[0] };
    LInt y_1 { y[1] };
    LInt y_2 { y[2] };
    
    return LVector3_T {
        x_1 * y_2 - x_2 * y_1,
        x_2 * y_0 - x_0 * y_2,
        x_0 * y_1 - x_1 * y_0
    };
}

static Polynomial3 Det_Perturbed( cref<Vector3_T> x, cref<Vector3_T> y )
{
    LVector3_T z = Prosector::Cross(x,y);
    
    return Polynomial3{ z[2], z[0], z[1] };
}

static Sign_T DetSign( Int a, Int b, Int c, Int d )
{
    const LInt det = LInt{a} * LInt{d} - LInt{b} * LInt{c};
    if( det > LInt{0} ) { return Sign_T( 1); }
    if( det < LInt{0} ) { return Sign_T(-1); }
    return Sign_T(0);
}

// Computes the limit of the determinant of u and v after being projected to the x-y-plane along the perturbed vector `{eps, eps * eps, 1}` for eps -> 0 from the right.

static Sign_T DetSign_Perturbed( cref<Vector3_T> u, cref<Vector3_T> v )
{
    Sign_T sign;
    
    sign = DetSign(u[0],u[1],v[0],v[1]);
    if( sign != Sign_T(0) ) { return sign; }
    
    // In a generic situation, we will seldomly arrive at this point.
    
    sign = DetSign(u[1],u[2],v[1],v[2]);
    if( sign != Sign_T(0) ) { return sign; }
    
    sign = DetSign(u[2],u[0],v[2],v[0]);
    if( sign != Sign_T(0) ) { return sign; }
    
    // u and v a collinear in 3-space.
    
    return Sign_T(0);
}
