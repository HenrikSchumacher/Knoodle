public:

template<bool transformQ = false,bool shiftQ = true>
void ReadVertexCoordinates( cptr<Real> v )
{
    TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates")+"<" + ToString(transformQ) + "," + ToString(shiftQ) + ">(AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");

    intersections_computedQ  = false;
    bounding_boxes_computedQ = false;
    intersections.clear();
    
    [[maybe_unused]] Vector3_T lo { Scalar::Max<Real> };
    [[maybe_unused]] Vector3_T hi { Scalar::Min<Real> };

    if( preorderedQ )
    {
//        logprint("preordered");
        
        auto read = [&lo,&hi,this]( cptr<Real> source, mptr<Real> target_i, mptr<Real> target_j )
        {
            Vector3_T x;
            
            if constexpr ( transformQ )
            {
                Vector3_T y (source);
                x = Dot(R,y);
            }
            else
            {
                (void)this;
                x.Read(source);
            }
            
            if constexpr ( shiftQ )
            {
                lo.ElementwiseMin(x);
                hi.ElementwiseMax(x);
            }
            else
            {
                (void)lo;
                (void)hi;
            }

            x.Write(target_i);
            x.Write(target_j);
        };
        
        for( Int c = 0; c < component_count; ++c )
        {
            const Int i_begin = component_ptr[c  ];
            const Int i_end   = component_ptr[c+1];
                                
            for( Int i = i_begin; i < i_end-1; ++i )
            {
                const Int j = i+1;

                cptr<Real> source   = &v[AmbDim * j];
                mptr<Real> target_i = edge_coords.data(i,Int(1));
                mptr<Real> target_j = &target_i[AmbDim];  // = edge_coords.data(j,0)
                
                read(source, target_i, target_j);
            }
            // Wrap-around.
            {
                const Int i = i_end-1;
                const Int j = i_begin;

                cptr<Real> source   = &v[AmbDim * j];
                mptr<Real> target_i = edge_coords.data(i,Int(1));
                mptr<Real> target_j = edge_coords.data(j,Int(0));
              
                read(source, target_i, target_j);
            }
        }
    }
    else
    {
//        logprint("not preordered");
        
        Vector3_T x_0;
        Vector3_T x_1;
        
        for( Int e = 0; e < edge_count; ++e )
        {
            const Int i = edges(e,0);
            const Int j = edges(e,1);

            mptr<Real> target_i = edge_coords.data(e,0);
            mptr<Real> target_j = &target_i[AmbDim]; // = edge_coords.data(e,1);
          
            if constexpr ( transformQ )
            {
                Vector3_T y_0 ( &v[AmbDim * i] );
                Vector3_T y_1 ( &v[AmbDim * j] );
                x_0 = Dot(R,y_0);
                x_1 = Dot(R,y_1);
            }
            else
            {
                x_0.Read( &v[AmbDim * i] );
                x_1.Read( &v[AmbDim * j] );
            }
            if constexpr ( shiftQ )
            {
                lo.ElementwiseMin(x_0);
                hi.ElementwiseMax(x_0);
                // We can skip the ElementwiseMin/ElementwiseMax for x_1 because every vertex is supposed to appear precisely once as a tail of an edge.
            }
            x_0.Write(target_i);
            x_1.Write(target_j);
        }
    }
    
    this->ComputeSterbenzShift<shiftQ>(lo, hi);
}

template<bool undo_transformQ = false, bool undo_shiftQ = false>
void WriteVertexCoordinates( mptr<Real> v ) const
{
    TOOLS_PTIMER(timer,MethodName("WriteVertexCoordinates")+"<" + ToString(undo_transformQ) + "," + ToString(undo_shiftQ) + ">(AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");
    
    [[maybe_unused]] Matrix3x3_T R_inv;
    
    if constexpr ( undo_transformQ )
    {
        R_inv = InverseTransformationMatrix();
    }
    
    auto write = [&R_inv,this]( cptr<Real> source, mptr<Real> target )
    {
        Vector3_T x;
        x.Read(source);
        
        if constexpr ( undo_shiftQ )
        {
            x -= this->Sterbenz_shift;
        }
        else
        {
            (void)this;
        }
        
        if constexpr ( undo_transformQ )
        {
            Vector3_T y = Dot(R_inv,x);
            y.Write(target);
        }
        else
        {
            (void)R_inv;
            x.Write(target);
        }
    };
    
    if( preorderedQ )
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            write( edge_coords.data(e), &v[AmbDim * e] );
        }
    }
    else
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            write( edge_coords.data(e), &v[AmbDim * edges(e,Int(0))] );
        }
    }
}


template<bool shiftQ = true>
void Transform( cref<Matrix3x3_T> A )
{
    TOOLS_PTIMER(timer,MethodName("Transform"));

    // Store new transformation matrix.
    SetTransformationMatrix(Dot(A,R));
    
    [[maybe_unused]] Vector3_T lo_0 { Scalar::Max<Real> };
    [[maybe_unused]] Vector3_T lo_1 { Scalar::Max<Real> };
    [[maybe_unused]] Vector3_T hi_0 { Scalar::Min<Real> };
    [[maybe_unused]] Vector3_T hi_1 { Scalar::Min<Real> };
    
    // We might do twice as many flops here as necessary.
    // But we scan `edge_coords` only twice and do not use any newly allocated buffers.
    // Also, we offer the compiler two vector lanes to further instruction parallelism.
    for( Int e = 0; e < edge_count; ++e )
    {
        mptr<Real> ptr_0 = edge_coords.data(e,Int(0));
        mptr<Real> ptr_1 = edge_coords.data(e,Int(1));
        
        Vector3_T x_0 ( ptr_0 );
        Vector3_T x_1 ( ptr_1 );
        
        // Undo Sterbenz shift.
        // Potentially wasteful if no shift had been applied before.
        // But only very little.
        x_0 -= Sterbenz_shift;
        x_1 -= Sterbenz_shift;
        
        // Transform.
        const Vector3_T y_0 = Dot(A,x_0);
        const Vector3_T y_1 = Dot(A,x_1);
        
        // Compute bounding box.
        if constexpr ( shiftQ )
        {
            lo_0.ElementwiseMin(y_0);
            lo_1.ElementwiseMin(y_1);
            hi_0.ElementwiseMax(y_0);
            hi_1.ElementwiseMax(y_1);
        }
        
        y_0.Write(ptr_0);
        y_1.Write(ptr_1);
    }
    
    if constexpr ( shiftQ )
    {
        lo_0.ElementwiseMin(lo_1);
        hi_0.ElementwiseMax(hi_1);
    }
    ComputeSterbenzShift<shiftQ>(lo_0,hi_0);
}


private:

template<bool shiftQ>
void ComputeSterbenzShift( cref<Vector3_T> lo, cref<Vector3_T> hi )
{
    if constexpr ( shiftQ )
    {
        TOOLS_MAKE_FP_STRICT();
        
        // https://en.wikipedia.org/wiki/Sterbenz_lemma
            
        // Apply Sterbenz shift.
        Sterbenz_shift[0] = std::fma(-Real(2), lo[0], hi[0]);
        Sterbenz_shift[1] = std::fma(-Real(2), lo[1], hi[1]);
        Sterbenz_shift[2] = std::fma(-Real(2), lo[2], hi[2]);

        for( Int e = 0; e < edge_count; ++e )
        {
            edge_coords(e,0,0) += Sterbenz_shift[0];
            edge_coords(e,0,1) += Sterbenz_shift[1];
            edge_coords(e,0,2) += Sterbenz_shift[2];
            edge_coords(e,1,0) += Sterbenz_shift[0];
            edge_coords(e,1,1) += Sterbenz_shift[1];
            edge_coords(e,1,2) += Sterbenz_shift[2];
        }
    }
    else
    {
        (void) lo;
        (void) hi;
        Sterbenz_shift[0] = 0;
        Sterbenz_shift[1] = 0;
        Sterbenz_shift[2] = 0;
    }
}
