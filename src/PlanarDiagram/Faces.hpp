public:

Int FaceCount() const
{
    return FaceDirectedArcPointers().Size()-1;
}

std::string FaceString( const Int f ) const
{
    cptr<Int> F_dA_ptr = FaceDirectedArcPointers().data();
    cptr<Int> F_dA_idx = FaceDirectedArcIndices().data();
    
    const Int i_begin = F_dA_ptr[f  ];
    const Int i_end   = F_dA_ptr[f+1];
    
    const Int f_size = i_end - i_begin;
    
    return "face " + ToString(f) + " = " + ArrayToString( &F_dA_idx[i_begin], { f_size } );
}

cref<Tensor1<Int,Int>> FaceDirectedArcIndices() const
{
    const std::string tag = "FaceDirectedArcIndices";
    
    if( !this->InCacheQ(tag) ) { RequireFaces(); }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> FaceDirectedArcPointers() const
{
    const std::string tag = "FaceDirectedArcPointers";
    
    if( !this->InCacheQ(tag) ) { RequireFaces(); }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<ArcContainer_T> ArcFaces()  const
{
    const std::string tag = "ArcFaces";
    
    if( !this->InCacheQ(tag) ) { RequireFaces(); }
    
    return this->template GetCache<ArcContainer_T>(tag);
}

void RequireFaces() const
{
    TOOLS_PTIC(ClassName()+"::RequireFaces");
    
    cptr<Int> dA_left_dA = ArcLeftArc().data();

    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    ArcContainer_T dA_F_buffer (max_arc_count);
    
    mptr<Int> dA_F = dA_F_buffer.data();
    
    // Convention: _Right_ face first:
    //
    //            A_faces_buffer(a,0)
    //
    //            <------------|  a
    //
    //            A_faces_buffer(a,1)
    //
    // This way the directed arc da = 2 * a + Head has its left face in dA_f[da].
    
    
    // Entry -1 means "unvisited but to be visited".
    // Entry -2 means "do not visit".
    
    for( Int a = 0; a < max_arc_count; ++ a )
    {
        const Int da = Int(2) * a;
        
        if( ArcActiveQ(a) )
        {
            dA_F[da         ] = Int(-1);
            dA_F[da + Int(1)] = Int(-1);
        }
        else
        {
            dA_F[da         ] = Int(-2);
            dA_F[da + Int(1)] = Int(-2);
        }
    }
    
    const Int dA_count = 2 * max_arc_count;
    
    Int dA_counter = 0;
    
    // Each arc will appear in two faces.
    Tensor1<Int,Int> F_dA_idx ( dA_count );
    // By Euler's polyhedra formula we have crossing_count - arc_count + face_count = 2.
    // Moreover, we have arc_count = 2 * crossing_count, hence face_count = crossing_count + 2.
    //    
    //    const Int face_count = crossing_count + 2;
    //    Tensor1<Int,Int> F_dA_ptr ( face_count + 1 );
    //
    // BUT: We are actually interested in face boundary cycles.
    // When we have a disconnected planar diagram, then there may be more than one boundary cycle per face:
    //
    // face_count = crossing_count + 2 * #(connected components)
    //
    // I don't want to count the number of connected components here, so I use an
    // expandable Aggregator<Int,Int>  here -- with default length that will always do for knots.

    Int expected_face_count = crossing_count + Int(2);
    Aggregator<Int,Int> F_dA_ptr_agg ( expected_face_count + Int(1) );
    F_dA_ptr_agg.Push(Int(0));
    
    for( Int da_0 = 0; da_0 < dA_count; ++da_0 )
    {
        if( dA_F[da_0] != Int(-1) ) { continue; }
        
        Int da = da_0;
        do
        {
            // Declare current face to be a face of this directed arc.
            dA_F[da] = F_dA_ptr_agg.Size() - Int(1);
            
            // Declare this arc to belong to the current face.
            F_dA_idx[dA_counter] = da;
            
            // Move to next arc.
            da = dA_left_dA[da];
            
            ++dA_counter;
        }
        while( da != da_0 );
        
        F_dA_ptr_agg.Push(dA_counter);
    }
    
    this->SetCache( "FaceDirectedArcPointers", std::move(F_dA_ptr_agg.Get()) );
    this->SetCache( "FaceDirectedArcIndices" , std::move(F_dA_idx)           );
    this->SetCache( "ArcFaces",                std::move(dA_F_buffer)        );
    
    TOOLS_PTOC(ClassName()+"::RequireFaces");
}


Int OutsideFace()
{
    const std::string tag = "OutsideFace";
    
    if( !this->InCacheQ(tag) )
    {
        auto & F_dA_ptr = FaceDirectedArcPointers();
        
        const Int f_count = FaceCount();
        
        Int max_f = 0;
        Int max_a = F_dA_ptr[max_f+1] - F_dA_ptr[max_f];
        
        for( Int f = 1; f < f_count; ++f )
        {
            const Int a_count = F_dA_ptr[f+1] - F_dA_ptr[f];
            
            if( a_count > max_a )
            {
                max_f = f;
                max_a = a_count;
            }
        }
        
        this->SetCache(tag, max_f);
    }
    
    return this->template GetCache<Int>(tag);
}
