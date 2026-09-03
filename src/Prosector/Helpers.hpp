public:
    
static void PrintInfo()
{
    std::string s { ClassName() + " uses the following types:" };
    s.append("\n  Int = ").append(TypeName<  Int>);
    s.append("\n LInt = ").append(TypeName< LInt>);
    s.append("\nLLInt = ").append(TypeName<LLInt>);
    logprint(s);
    
//    s.append("\n  Int = ").append(PrettyTypeName<  Int>());
//    s.append("\n LInt = ").append(PrettyTypeName< LInt>());
//    s.append("\nLLInt = ").append(PrettyTypeName<LLInt>());
}

// Not used by anyone.
TOOLS_FORCE_INLINE static LVector3_T cross( cref<Vector3_T> a, cref<Vector3_T> b )
{
    return LVector3_T {
        long_det(a[1],a[2],b[1],b[2]),
        long_det(a[2],a[0],b[2],b[0]),
        long_det(a[0],a[1],b[0],b[1])
    };
}

// Used by Prosector2
TOOLS_FORCE_INLINE static Sign_T Sign_Perturbed( cref<LVector3_T> cross_prod )
{
    if constexpr ( verboseQ )
    {
        Msgr::logprint("Sign_Perturbed");
    }
    
    Sign_T s;
    s = Sign<Sign_T>(cross_prod[2]);
    if( s != Sign_T(0) ) { return s; }
    s = Sign<Sign_T>(cross_prod[0]);    // Will seldomly get here.
    if( s != Sign_T(0) ) { return s; }
    s = Sign<Sign_T>(cross_prod[1]);
    if( s != Sign_T(0) ) { return s; }
    return Sign_T(0);
}

// Used by Prosector4
TOOLS_FORCE_INLINE static DepressedCubic Det_Perturbed( cref<Vector3_T> a, cref<Vector3_T> b )
{
    return DepressedCubic {
        long_det(a[0],a[1],b[0],b[1]),
        long_det(a[1],a[2],b[1],b[2]),
        long_det(a[2],a[0],b[2],b[0])
    };
}

// Used by Prosector4
// Lazy evaluation of the signs of the determinants.
TOOLS_FORCE_INLINE static
Sign_T Sign_Perturbed( cref<Vector3_T> a, cref<Vector3_T> b )
{
    if constexpr ( verboseQ )
    {
        Msgr::logprint("Sign_Perturbed");
    }
    
    Sign_T s;
    s = Sign<Sign_T>(long_det(a[0],a[1],b[0],b[1]));
    if( s != Sign_T(0) ) { return s; }
    s = Sign<Sign_T>(long_det(a[1],a[2],b[1],b[2])); // Will seldomly get here.
    if( s != Sign_T(0) ) { return s; }
    s = Sign<Sign_T>(long_det(a[2],a[0],b[2],b[0]));
    if( s != Sign_T(0) ) { return s; }
    return Sign_T(0);
}

// Used by Prosector4
// Lazy evaluation of the determinants.
TOOLS_FORCE_INLINE static
std::pair<Sign_T,LInt> Sign_Det_Perturbed( cref<Vector3_T> a, cref<Vector3_T> b )
{
    if constexpr ( verboseQ )
    {
        Msgr::logprint("Sign_Det_Perturbed");
    }
    
    LInt det = long_det(a[0],a[1],b[0],b[1]);
    Sign_T s = Sign<Sign_T>(det);
    if( s != Sign_T(0) ) { return {s,det}; }
    // In a generic situation, we will seldomly arrive at this point.
    s = Sign<Sign_T>(long_det(a[1],a[2],b[1],b[2]));
    if( s != Sign_T(0) ) { return {s,det}; }
    s = Sign<Sign_T>(long_det(a[2],a[0],b[2],b[0]));
    if( s != Sign_T(0) ) { return {s,det}; }
    return {Sign_T(0),det};
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
    if( s != Sign_T(0) ) { return s; }
    // In a generic situation, we will seldomly arrive at this point.
    s = DetSign2D_Kahan<Sign_T>(double(a[1]),double(a[2]),double(b[1]),double(b[2]));
    if( s != Sign_T(0) ) { return s; }
    s = DetSign2D_Kahan<Sign_T>(double(a[2]),double(a[0]),double(b[2]),double(b[0]));
    if( s != Sign_T(0) ) { return s; }
    return Sign_T(0);
}
