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
    
//    cref<Tensor1<Real,Int>> EdgeIntersectionTimes() const
//    {
//        return edge_times;
//    }
    
//    cref<Tensor1<Int,Int>> EdgeIntersections() const
//    {
//        return edge_intersections;
//    }
    
//    cref<Tensor1<bool,Int>> EdgeOverQ() const
//    {
//        return edge_overQ;
//    }
    
//    cref<std::vector<Intersection_T>> Intersections() const
//    {
//        return intersections;
//    }
    
    cref<Tree2_T> Tree() const
    {
        return T;
    }
    
//    cref<IntersectionFlagCounts_T> IntersectionFlagCounts() const
//    {
//        return intersection_flag_counts;
//    }

private:

    // In contrast to LinkEmbedding::DegenerateEdgeCount, which counts edges that are degenerate _after projection_, this counts the number of degenerate edges in 3-space.
    Idx DegenerateEdgeCount() const
    {
        Idx counter = 0;
        
        IntVector3_T x;
        IntVector3_T y;
        
        for( Idx edge = 0; edge < edge_count; ++edge )
        {
            x = EdgeVector3(edge,0);
            y = EdgeVector3(edge,1);
            
            const bool degenerateQ = (x[0] == y[0]) && (x[1] == y[1]) && (x[2] == y[2]);
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
