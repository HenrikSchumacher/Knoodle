public:

template<bool transformQ = false,bool shiftQ = true>
void ReadVertexCoordinates( cptr<Real> v )
{
    TOOLS_PTIMER(timer,MethodName("ReadVertexCoordinates")+"<" + ToString(transformQ) + "," + ToString(shiftQ) + ">(AoS, " + (preorderedQ ? "preordered" : "unordered") + ")");

    intersections_computedQ  = false;
    bounding_boxes_computedQ = false;
    intersections.clear();
    
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
    
    if( preorderedQ )
    {
//                logprint("preordered");
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
                
                if constexpr ( transformQ )
                {
                    y.Read(source);
                    x = Dot(R,y);
                }
                else
                {
                    x.Read(source);
                }
                
                if constexpr ( shiftQ )
                {
                    lo.ElementwiseMin(x);
                    hi.ElementwiseMax(x);
                }

                x.Write(target_i);
                x.Write(target_j);
            }

            {
                const Int i = i_end-1;
                const Int j = i_begin;

                mptr<Real> target_i = edge_coords.data(i,1);
                mptr<Real> target_j = edge_coords.data(j,0);
              
                if constexpr ( transformQ )
                {
                    y.Read( &v[3*j] );
                    x = Dot(R,y);
                }
                else
                {
                    x.Read( &v[3*j] );
                }
                
                if constexpr ( shiftQ )
                {
                    lo.ElementwiseMin(x);
                    hi.ElementwiseMax(x);
                }
                
                x.Write(target_i);
                x.Write(target_j);
            }
        }
    }
    else
    {
//                logprint("not preordered");
        
        for( Int e = 0; e < edge_count; ++e )
        {
            const Int i = edges(e,0);
            const Int j = edges(e,1);

            mptr<Real> target_i = edge_coords.data(e,0);
            mptr<Real> target_j = &target_i[3]; // = edge_coords.data(e,1);
          
            if constexpr ( transformQ )
            {
                y.Read( &v[3*i] );
                x = Dot(R,y);
            }
            else
            {
                x.Read( &v[3*i] );
            }
            
            if constexpr ( shiftQ )
            {
                lo.ElementwiseMin(x);
                hi.ElementwiseMax(x);
            }
            
            x.Write(target_i);
            
            if constexpr ( transformQ )
            {
                y.Read( &v[3*j] );
                x = Dot(R,y);
            }
            else
            {
                x.Read( &v[3*j] );
            }
            
            // We can skip the ElementwiseMin/ElementwiseMax here because every vertex is supposed to appear precisely once as a tail of an edge.
            
            x.Write(target_j);
        }
    }
    
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
        Sterbenz_shift[0] = 0;
        Sterbenz_shift[1] = 0;
        Sterbenz_shift[2] = 0;
    }
    
//            logvalprint("edge_coords",edge_coords);
}

void WriteVertexCoordinates( mptr<Real> v ) const
{
    TOOLS_PTIMER(timer,MethodName("WriteVertexCoordinates"));
    
    if( preorderedQ )
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            copy_buffer<AmbDim>( edge_coords.data(e), &v[AmbDim * e] );
        }
    }
    else
    {
        for( Int e = 0; e < edge_count; ++e )
        {
            copy_buffer<AmbDim>( edge_coords.data(e), &v[AmbDim * edges(e,0)] );
        }
    }
}



template<bool shiftQ = true>
void Transform( cref<Matrix3x3_T> A )
{
    TOOLS_PTIMER(timer,MethodName("Transform"));
    
    Tensor2<Real,Int> v_coords( edge_count, AmbDim );
    
    WriteVertexCoordinates(v_coords.data());

    Matrix_T R_old = R;
    
    SetTransformationMatrix(A);
    
    this->template ReadVertexCoordinates<true,shiftQ>(v_coords.data());
    
    // We make it so that we can restore the original coordinates up to shift from R.
    // That is: we rotate both the coordinates and R by A; then we set R to the rotated matrix.
    SetTransformationMatrix(Dot(A,R_old));
}
