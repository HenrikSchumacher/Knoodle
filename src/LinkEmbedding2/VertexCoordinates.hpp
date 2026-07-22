public:

bool VertexCoordinatesLoadedQ() const
{
    return vertex_coords_loadedQ;
}

cref<VContainer_T> VertexCoordinates() const
{
    return vertex_coords;
}

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
