public:
    
/*!@brief Return the ambient diemension (== 3).*/
static constexpr Int AmbientDimension()
{
    return 3;
}

/*!@brief Return the number of crossings/intersections in the x-y-Plane.*/
Int CrossingCount() const
{
    return int_cast<Int>( intersections.size() );
}

/*!@brief Whether the link embedding is in valid state.*/
bool ValidQ() const
{
    return (component_ptr.Size() >= Int(2));
}

/*!@brief The maximum modulus of all vertex coordinates. Used for computing `RelativeRoundingError.`*/
Real MaxModulus() const
{
    return max_modulus;
}

/*!@brief Return the scaling factor that was used the last time integer edge coordinates were computed. Defaults to `1`.*/
Real ScalingFactor() const
{
    return scaling_factor;
}

/*!@brief Return the scaling exponent that was used the last time integer edge coordinates were computed. Defaults to `0`.*/
int ScalingExponent() const
{
    return scaling_exponent;
}

/*!@brief Return the maximum absolute rounding that occurred the last time integer edge coordinates were computed. Defaults to `0`.*/
Real RoundingError() const
{
    return rounding_error;
}

/*!@brief Return the maximum relative rounding that occurred the last time integer edge coordinates were computed. Defaults to `0`.*/
Real RelativeRoundingError() const
{
    if constexpr ( FloatQ<Real> )
    {
        return (rounding_error * scaling_factor) / max_modulus;
    }
    else
    {
        return 0;
    }
}

/*!@brief Set the transformation matrix currently used by `ReadVertexCoordinates` and `WriteVertexCoordinates`.*/
void SetTransformationMatrix( cref<Matrix3x3_T> A )
{
    R = A;
}

/*!@brief Set the transformation matrix currently used by `ReadVertexCoordinates` and `WriteVertexCoordinates`.*/
void SetTransformationMatrix( Matrix3x3_T && A )
{
    R = std::move(A);
}

/*!@brief Return the transformation matrix currently used by `ReadVertexCoordinates` and `WriteVertexCoordinates`.*/
cref<Matrix3x3_T> TransformationMatrix() const
{
    return R;
}

/*!@brief Return the inverse of transformation matrix currently used by `ReadVertexCoordinates` and `WriteVertexCoordinates`.*/
Matrix3x3_T InverseTransformationMatrix() const
{
    return Inverse_Kahan(R);
}

/*!@brief Return the binary tree used for accelerating the planar intersection detection.*/
cref<Tree2_T> Tree() const
{
    return T;
}

/*!@brief Deallocate all data that is not stricly needed after the intersections have been found. This includes not only the tree, but also the containers for edge coordinates, bounding boxes, and intersection times. Use this in very memory constrained scenarios before handing this class over to `PlanarDiagram` or `PlanarDiagramComplex`.*/
void DeleteTree()
{
    T           = Tree2_T();
    edge_coords = EContainer_T();
    box_coords  = BContainer_T();
    edge_coords_computedQ    = false;
    bounding_boxes_computedQ = false;
    
    // Strictly speaking, this is not part of the tree, but it is not necessary anymore, once the intersections are computed (and sorted).
    
    edge_times = Tensor1<Time_T,Int>();
}
