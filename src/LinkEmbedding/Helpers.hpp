public:
    
static constexpr Int AmbientDimension()
{
    return 3;
}

Int CrossingCount() const
{
    return intersection_count;
}

Int IntersectionCount() const
{
    return intersection_count;
}

cref<Tensor1<Int,Int>> EdgePointers() const
{
    return edge_ptr;
}

cref<Tensor1<Real,Int>> EdgeIntersectionTimes() const
{
    return edge_times;
}

cref<Tensor1<Int,Int>> EdgeIntersections() const
{
    return edge_intersections;
}

cref<Tensor1<Int8,Int>> EdgeStates() const
{
    return edge_state;
}

cref<std::vector<Intersection_T>> Intersections() const
{
    return intersections;
}

cref<Tree2_T> Tree() const
{
    return T;
}

cref<Vector3_T> SterbenzShift() const
{
    return Sterbenz_shift;
}

cref<IntersectionFlagCounts_T> IntersectionFlagCounts() const
{
    return intersection_flag_counts;
}

bool BoundingBoxesComputedQ()
{
    return bounding_boxes_computedQ;
}

bool IntersectionsComputedQ()
{
    return intersections_computedQ;
}

void SetTransformationMatrix( cref<Matrix3x3_T> A )
{
    R = A;
}

void SetTransformationMatrix( Matrix3x3_T && A )
{
    R = A;
}

cref<Matrix3x3_T> TransformationMatrix() const
{
    return R;
}

template<bool shiftQ = true>
void Transform( cref<Matrix3x3_T> A )
{
    TOOLS_PTIMER(timer,MethodName("Transform"));

    // The transformation the current coordinates already represent. It has to be
    // read before SetTransformationMatrix below overwrites R, because the
    // accumulated transformation is A * R_old -- not A * A.
    const Matrix3x3_T R_old = R;

    Tensor2<Real,Int> v_coords( edge_count, AmbDim );

    WriteVertexCoordinates(v_coords.data());

    // WriteVertexCoordinates hands back the raw edge_coords, which already carry
    // the Sterbenz shift, so that shift has to come off before we rotate.
    // Otherwise ReadVertexCoordinates rotates the old shift along with the
    // geometry and then adds a fresh one on top, and Transform translates the
    // link as well as rotating it. Repeated calls compound the translation and
    // walk the coordinates away from the origin, which spends the mantissa on
    // position instead of shape and makes degenerate projections likelier.
    for( Int i = 0; i < edge_count; ++i )
    {
        for( Int k = 0; k < AmbDim; ++k )
        {
            v_coords(i,k) -= Sterbenz_shift[k];
        }
    }

    SetTransformationMatrix(A);

    this->template ReadVertexCoordinates<true,shiftQ>(v_coords.data());

    // We make it so that we can restore the original coordinates up to shift from R.
    // That is: we rotate both the coordinates and R by A; then we set R to the rotated matrix.
    SetTransformationMatrix(Dot(A,R_old));
}

template<bool shiftQ = true>
[[deprecated("This is a misnomer; changed name to `Transform`")]]
void Rotate( cref<Matrix3x3_T> A )
{
    Transform(A);
}


private:

Int DegenerateEdgeCount() const
{
    Int counter = 0;
    
    Vector2_T x;
    Vector2_T y;
    
    for( Int edge = 0; edge < edge_count; ++edge )
    {
        x = EdgeVector2(edge,0);
        y = EdgeVector2(edge,1);
        
        const Real d2 = SquaredDistance(x,y);
        
        const bool degenerateQ = (d2 <= Real(0));
        counter += degenerateQ;
        
        if( degenerateQ )
        {
            wprint(ClassName()+"::DegenerateEdges: Detected degenerate edge " + ToString(edge) +".");
            logvalprint("x", x);
            logvalprint("y", y);
            logvalprint("edge data", EdgeData(edge));
        }
    }
    
    return counter;
}
