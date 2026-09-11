// These checks can be done with the lowest precision (Int).
// They are common to all Prosector classes.

// TODO: This is still flaky. If we have a nondegenerate edge a, then a sequence of degenerate edges, and then again a nondegenerate edge b, then a and b have a vertex in common. But we should ignore this intersection.

private:

bool PointOnLineTest( cref<Vector3_T> z, cref<Vector3_T> a_0, cref<Vector3_T> a_1 )
{
    // Precondition: z lies on the line through a_0 and a_1.
    
    if constexpr ( verboseQ ) { Msgr::logprint("PointOnLineTest"); }
    
    // TODO: Rephrase this as for loop?
//    for( int k = 0; k < 3; ++k )
//    {
//    }
    
    // Find coordinate direction k so that a_0[k] != a_1[k];
    Int k = 0;
    while((a_0[k] == a_1[k]) && (k < Int{3})) { ++k; };
    
    if( k == Int{3} )
    {
        if constexpr ( verboseQ )
        {
            Msgr::logprint("PointOnLineTest", "Line segment is denegerate.");
            TOOLS_LOGDUMP(z);
            TOOLS_LOGDUMP(a_0);
            TOOLS_LOGDUMP(a_1);
        }
        // TODO: Do we really want to count this as an intersection?
        return true;
    }
    
    auto [a,b] = MinMax(a_0[k],a_1[k]);
    
    if( (a <= z[k]) && (z[k] <= b) )
    {
        if constexpr ( verboseQ )
        {
            Msgr::logprint("PointOnLineTest", "Point lies on line segment.");
            TOOLS_LOGDUMP(z);
            TOOLS_LOGDUMP(a_0);
            TOOLS_LOGDUMP(a_1);
        }
        return true;
    }
    else
    {
        if constexpr ( verboseQ )
        {
            Msgr::logprint("PointOnLineTest", "Point does not lie on line segment.");
        }
        return false;
    }
}

bool LinesColinearTest()
{
    // Precondition: The two lines are colinear.
    
    if constexpr ( verboseQ ) { Msgr::logprint("LinesColinearTest"); }
    
    for( Int k = 0; k < Int{3}; ++k )
    {
        if( (x_0[k] == x_1[k]) && (y_0[k] == y_1[k]) ) { continue; }
        
        // If we arrive here, then at least one of the lines is nondenerate and we have
        // `x_0[k] != x_1[k]` or `y_0[k] != y_1[k]`.
        // So at least one of the following intervals is nondegnerate.
        
        auto [a,b] = MinMax(x_0[k],x_1[k]);
        auto [c,d] = MinMax(y_0[k],y_1[k]);
        
        // Check whether intervals [a,b] and [c,d] intersect.
        const bool result =  (a <= d) && (c <= b);
        
        if constexpr ( verboseQ )
        {
            Msgr::logprint("LinesColinearTest", result ? "Line segments intersect." : "Line segments do not intersect.");
        }
        
        return result;
    }

    assert(x_0[0] == x_1[0]);
    assert(x_0[1] == x_1[1]);
    assert(x_0[2] == x_1[2]);
    
    assert(y_0[0] == y_1[0]);
    assert(y_0[1] == y_1[1]);
    assert(y_0[2] == y_1[2]);
    
    assert(x_0[0] == y_0[0]);
    assert(x_0[1] == y_0[1]);
    assert(x_0[2] == y_0[2]);
    
    if constexpr ( verboseQ )
    {
        Msgr::logprint("LinesColinearTest", "Both line segments are denegerate.");
    }
    // If we arrive here, then both intervals are degenerate and colinear. So their intersection is one point, namely `x_0 == x_1 == y_0 == y_1`.
    
    // TODO: Do we really want to count this as an intersection?
    return true;
}
