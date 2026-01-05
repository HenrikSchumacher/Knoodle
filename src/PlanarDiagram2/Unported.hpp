
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
