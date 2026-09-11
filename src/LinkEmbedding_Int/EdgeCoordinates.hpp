public:

bool EdgeCoordinatesComputedQ()  const
{
    return edge_coords_computedQ;
}

void RequireEdgeCoordinates() const
{
    if( edge_coords_computedQ ) { return; }

    ComputeEdgeCoordinates();
}
      
cref<EContainer_T> EdgeCoordinates() const
{
    RequireEdgeCoordinates();
    return edge_coords;
}

cptr<IReal> EdgeData( const Int k, const bool l ) const
{
    return l ? edge_coords.data(NextEdge(k)) : edge_coords.data(k);
}

cref<Tensor1<Int,Int>> MortonOrdering() const
{
    RequireEdgeCoordinates();
    return p;
}

private:
    
    void ComputeEdgeCoordinates() const
    {
        TOOLS_PTIMER(timer,MethodName("ComputeEdgeCoordinates"));
        
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
            Msgr::wprint("ComputeEdgeCoordinates", "No vertex coordinates loaded, yet. Call ReadVertexCoordinates first.");
        }
        
        if( edge_coords.Dim(0) != edge_count )
        {
            edge_coords = EContainer_T(edge_count);
        }
        
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
        
        scaling_exponent = m - static_cast<int>(std::ceil(std::log2(MaxModulus())));
        
        // Scaling by a power of 2 does not incur any rounding, execept we have excessively small numbers here.
        scaling_factor = std::pow(Real(2),scaling_exponent);
        
        Vector3_T x;
        Vector3_T y;
        Vector3_T err {0};

        rounding_error = 0;
        
        // We may omit scaling in the floating-point case only if the inputs are integer values and not too big.
        bool scaleQ = !(input_integralQ && (scaling_exponent >= 0));
        
        [[maybe_unused]] IVector3_T int_lo;
        
        if constexpr ( mortonQ )
        {
            if constexpr ( FloatQ<Real> )
            {
                if( scaleQ )
                {
                    int_lo[0] = static_cast<Int>(std::nearbyint(global_lo[0] * scaling_factor));
                    int_lo[1] = static_cast<Int>(std::nearbyint(global_lo[1] * scaling_factor));
                    int_lo[2] = static_cast<Int>(std::nearbyint(global_lo[2] * scaling_factor));
                }
                else
                {
                    // We know that all entries of `lo_global` have exact integer values; so we can simply cast to `IReal`.
                    global_lo.Write( &int_lo[0] );
                }
            }
            else
            {
                global_lo.Write( &int_lo[0] );
            }
        }
        
        for( Int i = 0; i < edge_count; ++i )
        {
            x.Read(vertex_coords.data(i));
            
            if constexpr ( FloatQ<Real> )
            {
                if( scaleQ )
                {
                    y[0] = std::nearbyint(x[0] * scaling_factor);
                    y[1] = std::nearbyint(x[1] * scaling_factor);
                    y[2] = std::nearbyint(x[2] * scaling_factor);
                    
                    err[0] = Max( err[0], Abs(std::fma(-scaling_factor,x[0],y[0])) );
                    err[1] = Max( err[1], Abs(std::fma(-scaling_factor,x[1],y[1])) );
                    err[2] = Max( err[2], Abs(std::fma(-scaling_factor,x[2],y[2])) );
                    
                    if constexpr ( mortonQ ) { y -= int_lo; }
                    
                    y.Write(edge_coords.data(i)); // static_cast<IReal> will be called automaticaly
                }
                else
                {
                    (void)y;
                    if constexpr ( mortonQ ) { x -= int_lo; }
                    // We know that all entries of `x` have exact integer values; so we can simply cast to `IReal`. No rounding error occurs.
                    x.Write(edge_coords.data(i)); // static_cast<IReal> will be called automatically
                }
            }
            else
            {
                (void)y;
                if constexpr ( mortonQ ) { x -= int_lo; }
                x.Write(edge_coords.data(i));   // static_cast<IReal> will be called automatically
            }
            
        } // for( Int i = 0; i < edge_count; ++i )
        
        rounding_error = err.Max();
        
        if constexpr ( mortonQ )
        {
            p.template RequireSize<false>(edge_count);
            p.iota();
            leaf_node_to_edge.template RequireSize<false>(edge_count);
            
            std::sort( &p[0], &p[edge_count],
                [this]( Int i, Int j )
                {
                    return less_Morton<2>(
                        reinterpret_cast<UInt *>(edge_coords.data(i)),
                        reinterpret_cast<UInt *>(edge_coords.data(j))
                    );
                }
            );
            
            for( Int i = 0; i < edge_count; ++i )
            {
                leaf_node_to_edge[i] = p[T.NodeBegin(T.InternalNodeCount() + i)];
            }
        }
        
        edge_coords_computedQ = true;
    }
