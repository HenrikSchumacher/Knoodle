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
