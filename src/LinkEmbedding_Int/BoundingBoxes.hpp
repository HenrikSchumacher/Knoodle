public:

cref<BContainer_T> BoundingBoxes() const
{
    RequireBoundingBoxes();
    
    return box_coords;
}

void RequireBoundingBoxes() const
{
    if( bounding_boxes_computedQ ) { return; }
    
    ComputeBoundingBoxes();
}

bool BoundingBoxesComputedQ() const
{
    return bounding_boxes_computedQ;
}

private:


void ComputeBoundingBoxes() const
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
    
    // Here we do something strange:
    // We hand over edge_coords, a Tensor3 of size edge_count x 2 x 3
    // to a T which is a Tree2_T.
    // The latter expects a Tensor3 of size edge_count x 2 x 2, but it accesses the entries only via data(i,j), so this is safe!
    
//    T.template ComputeBoundingBoxes<2,3>( edge_coords.data(), box_coords.data() );
    if constexpr ( mortonQ )
    {
        T.template ComputeBoundingBoxes<2>(
            [this]( Int i, Int j ) { return this->EdgeData(p[i],j); },
            box_coords.data()
        );
    }
    else
    {
        T.template ComputeBoundingBoxes<2>(
            [this]( Int i, Int j ) { return this->EdgeData(i,j); },
            box_coords.data()
        );
    }
    
    bounding_boxes_computedQ = true;
}


bool BoxesIntersectQ( const Int i, const Int j ) const
{
    return T.BoxesIntersectQ( box_coords.data(i), box_coords.data(j) );
}
