public:

bool EdgeCoordinatesComputedQ()  const
{
    return edge_coords_computedQ;
}

void RequireEdgeCoordinates()
{
    if( edge_coords_computedQ ) { return; }

    ComputeEdgeCoordinates();
}
      
cref<EContainer_T> EdgeCoordinates()
{
    RequireEdgeCoordinates();
    
    return edge_coords;
}

cref<Real> EdgeData( const Int k, const Int l )
{
    return edge_coords.data(k,l);
}


private:
    
    void ComputeEdgeCoordinates()
    {
        [[maybe_unused]] auto tag = [](){ return MethodName("ComputeEdgeCoordinates"); };
        
        TOOLS_PTIMER(timer,tag());
        
        edge_coords_computedQ    = false;
        bounding_boxes_computedQ = false;
        intersections_computedQ  = false;
        
        scaling_factor           = 1;
        scaling_exponent         = 0;
        rounding_error           = 0;
        intersection_count       = 0;
        intersection_count_3D    = 0;
        
        if( !vertex_coords_loadedQ )
        {
            wprint(tag() + ": No vertex coordinates loaded, yet. Call ReadVertexCoordinates first.");
        }
        
        if( edge_coords.Dim(0) != edge_count )
        {
            edge_coords = EContainer_T(edge_count);
        }
        
        if( edge_degenerateQ.Dim(0) != edge_count )
        {
            edge_degenerateQ = Tensor1<bool,Int>(edge_count);
        }
        
        // TODO: Eventually, we need several versions:
        
        // IReal is always an integral type.
        
        // Case 1. `Real` is a floating-point type
        //   Case 1 a) use scaling + rounding
        //   Case 1 b) just round (but do this only if all inputs are integers.
        // Case 2. `Real` is am integral type
        //   Then we can simply copy.
        
        // Let's handle only case 1.2.a) for now.
        static_assert( FloatQ<Real>, "" );
        static_assert( SignedIntQ<IReal>, "" );
        
        const Real length = Max( Abs(global_lo.Max()), Abs(global_hi.Max()) );
        
        // TODO: Choices of max_bits are quite arbitrary here. Can we make better choices? Can we motivate these choices?
        constexpr int max_bits = SameQ<IReal,Int32> ? 28 : 60;
        
        scaling_exponent = inputs_integralQ ? 0 : max_bits-static_cast<int>(std::ceil(std::log2(length)));
        
        // Scaling by a power of 2 does not incur any rounding, execept we have excessively small numbers here.
        scaling_factor = std::pow(Real(2),scaling_exponent);
        
        Vector3_T x;
        Vector3_T y;
        Vector3_T err {0};
        
        IRealVector3_T z;
        
        rounding_error = 0;
        
        auto read = [&x,&y,&z,&err,this](
            const Int j, mptr<IReal> target_i, mptr<IReal> target_j
        )
        {
            x.Read(vertex_coords.data(j));
            
            y[0] = std::nearbyint(x[0] * scaling_factor) ;
            y[1] = std::nearbyint(x[1] * scaling_factor);
            y[2] = std::nearbyint(x[2] * scaling_factor);

            err[0] = Max( err[0], Abs(std::fma(-scaling_factor,x[0],y[0])) );
            err[1] = Max( err[1], Abs(std::fma(-scaling_factor,x[1],y[1])) );
            err[2] = Max( err[2], Abs(std::fma(-scaling_factor,x[2],y[2])) );
            
            z[0] = static_cast<IReal>(y[0]);
            z[1] = static_cast<IReal>(y[1]);
            z[2] = static_cast<IReal>(y[2]);
            
            z.Write(target_i);
            z.Write(target_j);
        };
        
        for( Int c = 0; c < component_count; ++c )
        {
            const Int i_begin = component_ptr[c  ];
            const Int i_end   = component_ptr[c+1];
                                
            {
                const Int i = i_end-1;
                const Int j = i_begin;
                mptr<IReal> target_i = edge_coords.data(i,1);
                mptr<IReal> target_j = edge_coords.data(j,0);
                read( j, target_i, target_j );
            }
            
            for( Int i = i_begin; i < i_end-1; ++i )
            {
                const Int j = i+1;
                mptr<IReal> target_i = edge_coords.data(i,1);
                mptr<IReal> target_j = &target_i[3];  // = edge_coords.data(j,0)
                read( j, target_i, target_j );
                
                edge_degenerateQ[i] = (target_i[-3] == target_i[0]) && (target_i[-2] == target_i[1]) && (target_i[-1] == target_i[2]);
            }
            
            {
                const Int i = i_end-1;
                mptr<IReal> target_i = edge_coords.data(i,1);
                edge_degenerateQ[i] = (target_i[-3] == target_i[0]) && (target_i[-2] == target_i[1]) && (target_i[-1] == target_i[2]);
            }
            
        } // for( Int c = 0; c < component_count; ++c )
        
        rounding_error = err.Max();
        
        edge_coords_computedQ = true;
    }
