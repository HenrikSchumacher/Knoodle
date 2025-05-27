public:
    
    static constexpr Int AmbientDimension()
    {
        return 3;
    }
    
    Int CrossingCount() const
    {
        return int_cast<Int>( intersections.size() );
    }
                         
    cref<BContainer_T> BoundingBoxes() const
    {
        return box_coords;
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

    void ComputeBoundingBox( cptr<Real> v, mref<Vector3_T> lo, mref<Vector3_T> hi )
    {
        // We assume that input is a link; thus
        const Int vertex_count = edge_count;
        
        lo.Read( v );
        hi.Read( v );
        
        for( Int i = 1; i < vertex_count; ++i )
        {
            lo[0] = Min( lo[0], v[3 * i + 0] );
            hi[0] = Max( hi[0], v[3 * i + 0] );
            
            lo[1] = Min( lo[1], v[3 * i + 1] );
            hi[1] = Max( hi[1], v[3 * i + 1] );
            
            lo[2] = Min( lo[2], v[3 * i + 2] );
            hi[2] = Max( hi[2], v[3 * i + 2] );
        }
    }


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
                wprint(ClassName() + "::DegenerateEdges: Detected degenerate edge " + ToString(edge) +".");
                logvalprint("x", x);
                logvalprint("y", y);
                logvalprint("edge data", EdgeData(edge));
            }
        }
        
        return counter;
    }
