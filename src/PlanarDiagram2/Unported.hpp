
/*!@brief Construction from `Knot_2D` object.
 *
 * Caution: This assumes that `Knot_2D::FindIntersections` has been called already!
 */

template<typename Real, typename BReal>
explicit PlanarDiagram2( cref<Knot_2D<Real,Int,BReal>> L )
:   PlanarDiagram2( L.CrossingCount() )
{
    static_assert(FloatQ<Real>,"");
    static_assert(FloatQ<BReal>,"");

    ReadFromLink<Real,BReal>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections()
    );
}

/*!@brief Construction from `Link_2D` object.
 *
 * Caution: This assumes that `Link_2D::FindIntersections` has been called already!
 */

template<typename Real, typename BReal>
explicit PlanarDiagram2( cref<Link_2D<Real,Int,BReal>> L )
:   PlanarDiagram2( L.CrossingCount() )
{
    static_assert(FloatQ<Real>,"");
    static_assert(FloatQ<BReal>,"");

    ReadFromLink<Real,BReal>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections()
    );
}

/*! @brief Construction from coordinates.
 */

template<typename Real, typename ExtInt>
PlanarDiagram2( cptr<Real> x, const ExtInt n )
{
    static_assert(FloatQ<Real>,"");
    static_assert(IntQ<ExtInt>,"");

    Knot_2D<Real,Int,Real> L ( n );

    L.ReadVertexCoordinates ( x );

    int err = L.template FindIntersections<true>();

    if( err != 0 )
    {
        eprint(ClassName() + "(): FindIntersections reported error code " + ToString(err) + ". Returning empty PlanarDiagram2.");
        return;
    }

    // Deallocate tree-related data in L to make room for the PlanarDiagram2.
    L.DeleteTree();

    // We delay the allocation until substantial parts of L have been deallocated.
    *this = PlanarDiagram2( L.CrossingCount() );

    ReadFromLink<Real,Real>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections()
    );
}

template<typename Real, typename ExtInt>
PlanarDiagram2( cptr<Real> x, cptr<ExtInt> edges, const ExtInt n )
{
    static_assert(FloatQ<Real>,"");
    static_assert(IntQ<ExtInt>,"");

    using Link_T = Link_2D<Real,Int,Real>;

    Link_T L ( edges, n );

    L.ReadVertexCoordinates ( x );

    int err = L.template FindIntersections<true>();

    if( err != 0 )
    {
        eprint(ClassName() + "(): FindIntersections reported error code " + ToString(err) + ". Returning empty PlanarDiagram2.");
        return;
    }

    // Deallocate tree-related data in L to make room for the PlanarDiagram2.
    L.DeleteTree();

    // We delay the allocation until substantial parts of L have been deallocated.
    *this = PlanarDiagram2( L.CrossingCount() );

    ReadFromLink<Real,Real>(
        L.ComponentCount(),
        L.ComponentPointers().data(),
        L.EdgePointers().data(),
        L.EdgeIntersections().data(),
        L.EdgeOverQ().data(),
        L.Intersections()
    );
}

public:
    
    
/*!
 * @brief Computes the writhe = number of right-handed crossings - number of left-handed crossings.
 */

Int Writhe() const
{
    Int writhe = 0;
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( CrossingRightHandedQ(c) )
        {
            ++writhe;
        }
        else if ( CrossingLeftHandedQ(c) )
        {
            --writhe;
        }
    }
    
    return writhe;
}

Int EulerCharacteristic() const
{
    TOOLS_PTIMER(timer,MethodName("EulerCharacteristic"));
    return CrossingCount() - ArcCount() + FaceCount();
}

template<bool verboseQ = true>
bool EulerCharacteristicValidQ() const
{
    TOOLS_PTIMER(timer,MethodName("EulerCharacteristicValidQ"));
    const Int euler_char  = EulerCharacteristic();
    const Int euler_char0 = Int(2) * DiagramComponentCount();
    
    const bool validQ = (euler_char == euler_char0);
    
    if constexpr ( verboseQ )
    {
        if( !validQ )
        {
            wprint(ClassName()+"::EulerCharacteristicValidQ: Computed Euler characteristic is " + ToString(euler_char) + " != 2 * DiagramComponentCount() = " + ToString(euler_char0) + ". The processed diagram cannot be planar.");
        }
    }
    
    return validQ;
}
    
    
PlanarDiagram2 ChiralityTransform(
    const bool mirrorQ, const bool reverseQ
) const
{
    PlanarDiagram2 pd ( max_crossing_count, unlink_count );
    
    pd.crossing_count  = crossing_count;
    pd.arc_count       = arc_count;
    pd.proven_minimalQ = proven_minimalQ;

    auto & pd_C_arcs  = pd.C_arcs;
    auto & pd_C_state = pd.C_state;
    auto & pd_A_cross = pd.A_cross;
    auto & pd_A_state = pd.A_state;
    
    
    const bool i0 = reverseQ;
    const bool i1 = !reverseQ;
    
    const bool j0 = (mirrorQ != reverseQ);
    const bool j1 = (mirrorQ == reverseQ);

    for( Int c = 0; c < max_crossing_count; ++c )
    {
        pd_C_arcs(c,0,0) = C_arcs(c,i0,j0);
        pd_C_arcs(c,0,1) = C_arcs(c,i0,j1);
        pd_C_arcs(c,1,0) = C_arcs(c,i1,j0);
        pd_C_arcs(c,1,1) = C_arcs(c,i1,j1);
    }
    
    if( mirrorQ )
    {
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            pd_C_state(c) = Flip(C_state(c));
        }
    }
    else
    {
        pd_C_state.Read(C_state.data());
    }
    
    
    const bool k0 = reverseQ;
    const bool k1 = !reverseQ;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        pd_A_cross(a,0) = A_cross(a,k0);
        pd_A_cross(a,1) = A_cross(a,k1);
    }
    
    pd_A_state.Read(A_state.data());
    
    return pd;
}
    
    
/*!@ Sets all entries of all deactivated crossings and arcs to `Uninitialized`.
 */

void CleanseDeactivated()
{
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( !CrossingActiveQ(c) )
        {
            fill_buffer<4>(C_arcs.data(c),Uninitialized);
        }
    }
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            fill_buffer<2>(A_cross.data(a),Uninitialized);
        }
    }
}
