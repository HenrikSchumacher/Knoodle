public:

cref<BContainer_T> BoundingBoxes()
{
    RequireBoundingBoxes();
    
    return box_coords;
}

void RequireBoundingBoxes()
{
    if( bounding_boxes_computedQ ) { return; }
    
    if( box_coords.Dim(0) != edge_count )
    {
        T = Tree2_T( edge_count );
        box_coords = T.AllocateBoxes();
    }
    
    ComputeBoundingBoxes();
}

private:


void ComputeBoundingBox(
    cptr<Real> v, const Int vertex_count,
    mref<Vector3_T> lo, mref<Vector3_T> hi
)
{
    TOOLS_MAKE_FP_FAST();
    
    lo.Read( v );
    hi.Read( v );

    for( Int i = 1; i < vertex_count; ++i )
    {
        lo.ElementwiseMin( &v[3 * i] );
        hi.ElementwiseMax( &v[3 * i] );
    }
}
