public:

/*!@brief Return flag that signals whether vertex coordinates have been loaded already.*/
bool VertexCoordinatesLoadedQ() const
{
    return vertex_coords_loadedQ;
}

/*!@brief Return the container of the vertex coordinates in the internal ordering and in interleaved for. The coordinates of the `k`-th component are stored in `VertexCoordinates()(i,j)` where `ComponentPointers()[k] <= i < ComponentPointers()[k+1]` and `0 <= j < 3`.*/
cref<VContainer_T> VertexCoordinates() const
{
    return vertex_coords;
}

/*!@brief Read vertex coordinates from external buffer `v` in external ordering defined by `Edges()`. More precisely, we have `VertexCoordinates()(i,j)` is read from `v[3 * Edges()(i,0) + j`.
 *
 * @param v Input buffer. It is assumed of have size `EdgeCount() * 3`.
 *
 * @tparam transformQ If set to `true`, the input vectors are multiplied by the matrix stored in `TransformationMatrix()`.
 */
template<bool transformQ = false>
void ReadVertexCoordinates( cptr<Real> v )
{
    TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates")+"<" + ToString(transformQ) + ">(AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");
    

    vertex_coords_loadedQ    = false;
    edge_coords_computedQ    = false;
    intersections_computedQ  = false;
    bounding_boxes_computedQ = false;

    scaling_factor           = 1;
    scaling_exponent         = 0;
    rounding_error           = 0;
    intersection_count       = 0;
    intersection_count_3D    = 0;
    inputs_integralQ         = IntQ<Real>;
    
    global_lo.Fill( Scalar::Max<Real> );
    global_hi.Fill( Scalar::Min<Real> );

    Vector3_T x;
    Vector3_T y;
    
    // vertex_coords.data(e), &v[AmbDim * edges(e,0)]
    // After loading we want:
    // vertex_data(e,k) = v[AmbDim * edges(e,0) + k]
    // edge_data(e,0,k) = round(vertex_data(e,k));
    
    auto read = [v,&x,&y,this]( const Int e, const Int i )
    {
        if constexpr ( transformQ )
        {
            x.Read( &v[3*i] );
            y = Dot(R,x);
        }
        else
        {
            (void)x;
            y.Read( &v[3*i] );
        }
        
        global_lo.ElementwiseMin(y);
        global_hi.ElementwiseMax(y);
        
        y.Write(vertex_coords.data(e));
    };
    
    if( preorderedQ )
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            read(e,e);
        }
    }
    else // if( !preorderedQ )
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            read(e,edges(e,0));
        }
    }
    
    vertex_coords_loadedQ = true;
}

/*!@brief Load the matrix `A` into the internal transformation matrix and apply it to all vertex coordinates. Already computed bounding boxes and intersections will be erased.
 *
 * @param A Input matrix.
 */
void Transform( cref<Matrix3x3_T> A )
{
    [[maybe_unused]] auto tag = [](){ return MethodName("Transform"); };
    
    TOOLS_PTIMER(timer,tag());
    
    if( !vertex_coords_loadedQ )
    {
        eprint(tag() + ": No vertex coordinates loaded, yet. Call ReadVertexCoordinates first.");
    }
    
    vertex_coords_loadedQ    = false;
    edge_coords_computedQ    = false;
    intersections_computedQ  = false;
    bounding_boxes_computedQ = false;

    scaling_factor           = 1;
    scaling_exponent         = 0;
    rounding_error           = 0;
    intersection_count       = 0;
    intersection_count_3D    = 0;
    inputs_integralQ         = IntQ<Real>;
    
    global_lo.Fill( Scalar::Max<Real> );
    global_hi.Fill( Scalar::Min<Real> );

    Vector3_T x;
    Vector3_T y;
    
    for( Int i = 0; i < edge_count; ++i )
    {
        x.Read( vertex_coords.data(i) );
        y = Dot(A,x);
        global_lo.ElementwiseMin(y);
        global_hi.ElementwiseMax(y);
        y.Write( vertex_coords.data(i) );
    }
    
    // We make it so that we can restore the original coordinates from R (up to rounding errors).
    // That is: we rotate both the coordinates and R by A; then we set R to the rotated matrix.
    SetTransformationMatrix(Dot(A,R));
}

/*!@brief Write vertex coordinates to external buffer `v` in external ordering defined by `Edges()`. More precisely, we have `VertexCoordinates()(i,j)` is written to `v[3 * Edges()(i,0) + j`.
 *
 * @param v Outout buffer. It is assumed to have size `EdgeCount() * 3`..
 */
void WriteVertexCoordinates( mptr<Real> v ) const
{
    TOOLS_PTIMER(timer,MethodName("WriteVertexCoordinates"));
    
    if( preorderedQ )
    {
        vertex_coords.Write(v);
    }
    else
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            copy_buffer<AmbDim>( vertex_coords.data(e), &v[AmbDim * edges(e,0)] );
        }
    }
}
