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


void ComputeBoundingBox( cptr<Real> v, mref<Vector3_T> lo, mref<Vector3_T> hi )
{
    TOOLS_MAKE_FP_FAST();
    
    // We assume that input is a link; thus
    const Int vertex_count = edge_count;
    
    lo.Read( v );
    hi.Read( v );
    
    for( Int i = 1; i < vertex_count; ++i )
    {
        lo[0] = Min( lo[0], v[3 * i + 0] );
        lo[1] = Min( lo[1], v[3 * i + 1] );
        lo[2] = Min( lo[2], v[3 * i + 2] );
        
        hi[0] = Max( hi[0], v[3 * i + 0] );
        hi[1] = Max( hi[1], v[3 * i + 1] );
        hi[2] = Max( hi[2], v[3 * i + 2] );
    }
}
