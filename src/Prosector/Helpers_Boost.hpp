public:
    
static void PrintInfo()
{
    std::string s { ClassName() + " uses the following types:" };
    s.append("\n  Int = ").append(TypeName<  Int>);
    s.append("\n LInt = ").append(TypeName< LInt>);
    s.append("\nLLInt = ").append(TypeName<LLInt>);
    logprint(s);
}

// Used by Prosector4
TOOLS_FORCE_INLINE static DepressedCubic Det_Perturbed( cref<LVector3_T> a, cref<LVector3_T> b )
{
    return DepressedCubic {
        a[0] * b[1] - a[1] * b[0],
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2]
    };
}

// Used by Prosector4
// Lazy evaluation of the signs of the determinants.
TOOLS_FORCE_INLINE static
Sign_T Sign_Perturbed( cref<LVector3_T> a, cref<LVector3_T> b )
{
    if constexpr ( verboseQ )
    {
        Msgr::logprint("Sign_Perturbed");
    }
    
    Sign_T s;
    s = Sign<Sign_T>( a[0] * b[1] - a[1] * b[0] );
    if( !ZeroQ(s) ) { return s; }
    s = Sign<Sign_T>( a[1] * b[2] - a[2] * b[1] ); // Will seldomly get here.
    if( !ZeroQ(s) ) { return s; }
    s = Sign<Sign_T>( a[2] * b[0] - a[0] * b[2] );
    if( !ZeroQ(s) ) { return s; }
    return Sign_T{0};
}

// Used by Prosector4
// Lazy evaluation of the determinants.
TOOLS_FORCE_INLINE static
std::pair<Sign_T,LInt> Sign_Det_Perturbed( cref<LVector3_T> a, cref<LVector3_T> b )
{
    if constexpr ( verboseQ )
    {
        Msgr::logprint("Sign_Det_Perturbed");
    }
    
    LInt det = a[0] * b[1] - a[1] * b[0];
    Sign_T s = Sign<Sign_T>( det );
    if( !ZeroQ(s) ) { return {s,det}; }
    // In a generic situation, we will seldomly arrive at this point.
    s = Sign<Sign_T>( a[1] * b[2] - a[2] * b[1] );
    if( !ZeroQ(s) ) { return {s,det}; }
    s = Sign<Sign_T>( a[2] * b[0] - a[0] * b[2] );
    if( !ZeroQ(s) ) { return {s,det}; }
    return {Sign_T{0},det};
}


// Lazy evaluation of the signs of the determinants, using double arithmetic.
TOOLS_FORCE_INLINE static
Sign_T Sign_Perturbed_Kahan( cref<Vector3_T> a, cref<Vector3_T> b )
{
    if constexpr ( verboseQ )
    {
        Msgr::logprint("Sign_Perturbed_Kahan");
    }

    Sign_T s;
    s = DetSign2D_Kahan<Sign_T>(double(a[0]),double(a[1]),double(b[0]),double(b[1]));
    if( !ZeroQ(s) ) { return s; }
    // In a generic situation, we will seldomly arrive at this point.
    s = DetSign2D_Kahan<Sign_T>(double(a[1]),double(a[2]),double(b[1]),double(b[2]));
    if( !ZeroQ(s) ) { return s; }
    s = DetSign2D_Kahan<Sign_T>(double(a[2]),double(a[0]),double(b[2]),double(b[0]));
    if( !ZeroQ(s) ) { return s; }
    return Sign_T(0);
}
