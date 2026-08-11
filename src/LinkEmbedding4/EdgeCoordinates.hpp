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

cptr<IReal> EdgeData( const Int k, const Int l )
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
        
        constexpr int n = sizeof(IReal) * CHAR_BIT;
        constexpr int k = 4;
        constexpr int m = n - k;
        
        /*! Suppose that `IReal` has `n` bits.
         *  Let `m` be the maximal number of bits that we allow for positive numbers.
         *  That leaves `k = n - m` extra bits.
         *
         *  We have to be able to do two things:
         *
         *  -# Compute `det_3`, a sum of three terms of the form `p * (a * d - b * c)`, where `p`, `u`, `v` are `IReal`s. The term `(a * d - b * c)` is computed in a `WideInt` with `3 * n` bits.
         *  The biggest possible modulus for `p`, `a`, `b`, `c`, `d` is `A = 2^m`.
         *  The biggest possible modulus for `det_3` is `6 * A * A * A`.
         *  This is bounded from above by `8 * 2^m * 2^m * 2^m`, which requires at mist `3 * m + 3` bits.
         *  We have `3 * n - 1` bits available in the `WideInt` that stores `det_3`.
         *  That means a reserve of `(3 * n - 1) - (3m + 3) = 3 * (n - m) - 4 = 3 * k - 4` bits.
         *  So `k >= 2` should be sufficient for this and for any bit count `n`.
         *
         *  -# Compute 2x2 determinants `a * d - b * c`, where `a`, `b`, `c`, `d` are WideInts that represent by themself 2x2 determinants of `n`-bit numbers with modulus of at most `m` bits.
         *  The biggest possible modulus for `a`, `b`, `c`, `d` is `A = 2 * 2^(2 * m)`.
         *  The biggest possible modulus for `a * d - b * c` is `B = 2 * A * A = 2^(4 * m + 2)`, which requires `4 * m + 2` bits.
         *  In the final result type we have `4 * n - 1` bits available.
         *  That means a reserve of `(4 * n - 1) - (4 * m + 2) = 4 * k - 3` bits.
         *  So `k >= 1` should be sufficient for this and for any bit count `n`.
         */
        
        scaling_exponent = m - static_cast<int>(std::ceil(std::log2(length)));
        
        // Scaling by a power of 2 does not incur any rounding, execept we have excessively small numbers here.
        scaling_factor = std::pow(Real(2),scaling_exponent);
        
        Vector3_T x;
        Vector3_T y;
        Vector3_T err {0};
        
        IRealVector3_T z;
        
        rounding_error = 0;
        
        // We may omit scaling in the floating-point case only if the inputs are integer values and not too big.
        bool scaleQ = !(input_integralQ && (scaling_exponent >= 0));
        
        auto read = [&x,&y,&z,&err,scaleQ,this](
            const Int j, mptr<IReal> target_i, mptr<IReal> target_j
        )
        {
            x.Read(vertex_coords.data(j));
            
            if constexpr ( FloatQ<Real> )
            {
                if( scaleQ )
                {
                    y[0] = std::nearbyint(x[0] * scaling_factor) ;
                    y[1] = std::nearbyint(x[1] * scaling_factor);
                    y[2] = std::nearbyint(x[2] * scaling_factor);
                    
                    err[0] = Max( err[0], Abs(std::fma(-scaling_factor,x[0],y[0])) );
                    err[1] = Max( err[1], Abs(std::fma(-scaling_factor,x[1],y[1])) );
                    err[2] = Max( err[2], Abs(std::fma(-scaling_factor,x[2],y[2])) );
                    
                    z.Read(y.data()); // static_cast<IReal> will be called automaticaly
                }
                else
                {
                    // We know that all entries of `x` have exact integer values; so we can simply cast to `IReal`. No rounding error occurs.
                    z.Read(x.data()); // static_cast<IReal> will be called automaticaly
                }
            }
            else
            {
                z.Read(x.data());   // static_cast<IReal> will be called automaticaly
            }
            
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
            }
            
        } // for( Int c = 0; c < component_count; ++c )
        
        rounding_error = err.Max();
        
        edge_coords_computedQ = true;
    }
