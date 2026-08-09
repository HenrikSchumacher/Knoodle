public:

bool BoundingBoxesComputedQ() const
{
    return bounding_boxes_computedQ;
}

void RequireBoundingBoxes()
{
    if( bounding_boxes_computedQ ) { return; }
    
    ComputeBoundingBoxes();
}

cref<BContainer_T> BoundingBoxes()
{
    RequireBoundingBoxes();
    
    return box_coords;
}

private:


void ComputeBoundingBoxes()
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ComputeBoundingBoxes"); };
    
    TOOLS_PTIMER(timer,tag());
    
    bounding_boxes_computedQ = false;
    intersections_computedQ  = false;

    intersection_count       = 0;
    intersection_count_3D    = 0;
    
    RequireEdgeCoordinates();
    
    if( !edge_coords_computedQ )
    {
        wprint(tag() + ": Edge coordinates not computed, yet. Aborting.");
        return;
    }
    
    if( box_coords.Dim(0) != edge_count )
    {
        T = Tree2_T( edge_count );
        box_coords = T.AllocateBoxes();
    }
    
    T.template ComputeBoundingBoxes<2,3>( edge_coords.data(), box_coords.data() );
    
    bounding_boxes_computedQ = true;
}


bool BoxesIntersectQ( const Int i, const Int j ) const
{
    return T.BoxesIntersectQ( box_coords.data(i), box_coords.data(j) );
}
