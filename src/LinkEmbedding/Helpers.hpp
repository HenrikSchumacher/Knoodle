public:
    
    static constexpr Int AmbientDimension()
    {
        return 3;
    }
    
    Int CrossingCount() const
    {
        return int_cast<Int>( intersections.size() );
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
    
    cref<Tensor1<bool,Int>> EdgeOverQ() const
    {
        return edge_overQ;
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
