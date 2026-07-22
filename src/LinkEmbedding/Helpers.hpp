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
    
    Tensor2<Real,Int> v_coords( edge_count, AmbDim );
    
    WriteVertexCoordinates(v_coords.data());

    SetTransformationMatrix(A);
    
    this->template ReadVertexCoordinates<true,shiftQ>(v_coords.data());
    
    // We make it so that we can restore the original coordinates up to shift from R.
    // That is: we rotate both the coordinates and R by A; then we set R to the rotated matrix.
    SetTransformationMatrix(Dot(A,R));
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
