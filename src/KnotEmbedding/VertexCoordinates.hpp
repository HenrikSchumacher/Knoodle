public:

template<bool transformQ = false,bool shiftQ = true>
void ReadVertexCoordinates( cptr<Real> v )
{
    TOOLS_PTIMER(timer,Msgr::MethodName("ReadVertexCoordinates") + "<" + to_ct_string(transformQ) + "," + to_ct_string(shiftQ) + ">");

    intersections_computedQ  = false;
    bounding_boxes_computedQ = false;
    intersections.Clear();
    
    Vector3_T lo;
    Vector3_T hi;

    if constexpr ( shiftQ )
    {
        lo.Read( v );
        hi.Read( v );
    }
    else
    {
        (void)lo;
        (void)hi;
    }
    
    Vector3_T x;
    Vector3_T y;
    
    for( Int e = 0; e < edge_count; ++e )
    {
        if constexpr ( transformQ )
        {
            y.Read( &v[AmbDim * e] );
            x = Dot(R,y);
        }
        else
        {
            x.Read( &v[AmbDim * e] );
        }
        if constexpr ( shiftQ )
        {
            lo.ElementwiseMin(x);
            hi.ElementwiseMax(x);
        }
        x.Write(vertex_coords.data(e));
    }

    // Copy the coordinates for the first vertex to the last's.
    copy_buffer<AmbDim>(vertex_coords.data(),vertex_coords.data(edge_count));
    
    this->ComputeSterbenzShift<shiftQ>(lo, hi);
}

template<bool undo_transformQ = false, bool undo_shiftQ = false>
void WriteVertexCoordinates( mptr<Real> v ) const
{
    TOOLS_PTIMER(timer,Msgr::MethodName("WriteVertexCoordinates")+"<" + to_ct_string(undo_transformQ) + "," + to_ct_string(undo_shiftQ) + ">");
    
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
            Vector3_T y = R_inv * x;
            y.Write(target);
        }
        else
        {
            (void)R_inv;
            x.Write(target);
        }
    };
    
    for( Int e = 0; e < edge_count; ++e )
    {
        write(vertex_coords.data(e),&v[AmbDim * e]);
    }
}


template<bool shiftQ = true>
void Transform( cref<Matrix3x3_T> A )
{
    TOOLS_PTIMER(timer,Msgr::MethodName("Transform"));

    // Store new transformation matrix.
    SetTransformationMatrix(Dot(A,R));
    
    [[maybe_unused]] Vector3_T lo { Scalar::Max<Real> };
    [[maybe_unused]] Vector3_T hi { Scalar::Min<Real> };
    
    // We might do twice as many flops here as necessary.
    // But we scan `edge_coords` only twice.
    // Also, we offer the compiler two vector lanes to further instruction parallelism.
    for( Int e = 0; e < edge_count + Int(1); ++e )
    {
        mptr<Real> ptr = vertex_coords.data(e);
        
        Vector3_T x ( ptr );
        
        // Undo Sterbenz shift.
        // Potentially wasteful if no shift had been applied before.
        // But only very little.
        x -= Sterbenz_shift;
        
        // Transform.
        const Vector3_T y = A * x;
        
        // Compute bounding box.
        if constexpr ( shiftQ )
        {
            lo.ElementwiseMin(y);
            hi.ElementwiseMax(y);
        }
        
        y.Write( ptr );
    }

    ComputeSterbenzShift<shiftQ>(lo,hi);
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

        for( Int v = 0; v < edge_count + Int(1); ++v )
        {
            vertex_coords(v,0) += Sterbenz_shift[0];
            vertex_coords(v,1) += Sterbenz_shift[1];
            vertex_coords(v,2) += Sterbenz_shift[2];
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
