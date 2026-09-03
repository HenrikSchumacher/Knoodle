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

cref<Tensor1<EdgeCrossing_T,Int>> EdgeCrossings() const
{
    return edge_cross;
}

//cref<Tensor1<Int,Int>> EdgeIntersections() const
//{
//    return edge_intersections;
//}
//
//cref<Tensor1<Int8,Int>> EdgeStates() const
//{
//    return edge_state;
//}

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

cref<ProsectorFlagCounts_T> IntersectionFlagCounts() const
{
    return prosector_flag_counts;
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

Matrix3x3_T InverseTransformationMatrix() const
{
    return Inverse_Kahan(R);
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
            Msgr::wprint("DegenerateEdges","Detected degenerate edge ", edge, ".");
            logvalprint("x", x);
            logvalprint("y", y);
            logvalprint("edge data", EdgeData(edge));
        }
    }
    
    return counter;
}
