public:

// This rotation must be orientation preserving.
template<typename R = Real, typename I = Int>
Tiny::Matrix<3,3,R,I> RandomRotation()
{
    static_assert(FloatQ<R>,"");
    static_assert(IntQ<I>,"");
    
    using Vector_T = Tiny::Vector<3,R,I>;
    using Matrix_T = Tiny::Matrix<3,3,R,I>;
    
    std::normal_distribution<R> gaussian {R(0),R(1)};
    std::uniform_real_distribution<R> angle_dist {R(0), Scalar::Pi<R>};
    
    Vector_T u;
    R u_squared;
    do
    {
        u[0] = gaussian(random_engine);
        u[1] = gaussian(random_engine);
        u[2] = gaussian(random_engine);
        u_squared = u.NormSquared();
    }
    while( u_squared <= R(0) );
        
    u /= Sqrt(u_squared);
    
    // Code copied from ClisbyTree.
    const R angle = angle_dist(random_engine);
    
    const R cos = std::cos(angle);
    const R sin = std::sin(angle);
    
    Matrix_T A;
    
    const R d = Real(1) - cos;
    
    A[0][0] = u[0] * u[0] * d + cos       ;
    A[0][1] = u[0] * u[1] * d - sin * u[2];
    A[0][2] = u[0] * u[2] * d + sin * u[1];
    
    A[1][0] = u[1] * u[0] * d + sin * u[2];
    A[1][1] = u[1] * u[1] * d + cos       ;
    A[1][2] = u[1] * u[2] * d - sin * u[0];
    
    A[2][0] = u[2] * u[0] * d - sin * u[1];
    A[2][1] = u[2] * u[1] * d + sin * u[0];
    A[2][2] = u[2] * u[2] * d + cos       ;
 
    return A;
}
