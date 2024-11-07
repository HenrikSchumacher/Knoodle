public:

Int FaceCount()
{
    return FaceDirectedArcPointers().Size()-1;
}

std::string FaceString( const Int f ) const
{
    cptr<Int> F_A_ptr = FaceDirectedArcPointers().data();
    cptr<Int> F_A_idx = FaceDirectedArcIndices().data();
    
    const Int i_begin = F_A_ptr[f  ];
    const Int i_end   = F_A_ptr[f+1];
    
    const Int f_size = i_end - i_begin;
    
    return "face " + ToString(f) + " = " + ArrayToString( &F_A_idx[i_begin], { f_size } );
}

cref<Tensor1<Int,Int>> FaceDirectedArcIndices()
{
    const std::string tag = "FaceDirectedArcIndices";
    
    if( !this->InCacheQ(tag) )
    {
        RequireFaces();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> FaceDirectedArcPointers()
{
    const std::string tag = "FaceDirectedArcPointers";
    
    if( !this->InCacheQ(tag) )
    {
        RequireFaces();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
    
}

cref<Tensor2<Int,Int>> ArcFaces()
{
    const std::string tag = "ArcFaces";
    
    if( !this->InCacheQ(tag) )
    {
        RequireFaces();
    }
    
    return this->template GetCache<Tensor2<Int,Int>>(tag);
    
}

void RequireFaces()
{
    ptic(ClassName()+"::RequireFaces");
    
    // TODO: Make ArcLeftArc cached as well?
    
    cptr<Int> A_left = ArcLeftArc().data();
    
//    auto A_left_buffer = ArcLeftArc();
//    cptr<Int> A_left = A_left_buffer.data();

    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    Tensor2<Int,Int> A_faces_buffer (initial_arc_count,2);
    
    mptr<Int> A_face = A_faces_buffer.data();
    
    // Convention: Right face first:
    //
    //            A_faces_buffer(a,0)
    //
    //            <------------|  a
    //
    //            A_faces_buffer(a,1)
    //
    // Entry -1 means "unvisited but to be visited".
    // Entry -2 means "do not visit".
    
    for( Int a = 0; a < initial_arc_count; ++ a )
    {
        const Int A = (a << 1);
        
        if( ArcActiveQ(a) )
        {
            A_face[A         ] = -1;
            A_face[A | Int(1)] = -1;
        }
        else
        {
            A_face[A         ] = -2;
            A_face[A | Int(1)] = -2;
        }
    }
    
    const Int A_count = 2 * initial_arc_count;
    
    Int A_counter = 0;
    
    // Each arc will appear in two faces.
    Tensor1<Int,Int> F_A_idx (2 * arc_count );
    // By Euler's polyhedra formula we have crossing_count - arc_count + face_count = 2.
    // Moreover, we have arc_count = 2 * crossing_count, hence face_count = crossing_count + 2.
    
    // BUT: We are actually interested in face boundary cycles. When we have a disconnected planar diagram, then there may be more than one boundary cycle per face.
    
//    const Int face_count = crossing_count + 2;
//    Tensor1<Int,Int> F_A_ptr ( face_count + 1 );
    
    Tensor1<Int,Int> F_A_ptr ( arc_count );
    
    F_A_ptr[0]  = 0;
    
    Int F_counter = 0;
    
    Int A_finder = 0;
    
    
    while( A_finder < A_count )
    {
        // Start a new graph component.
        while( (A_finder < A_count) && (A_face[A_finder] != -1) )
        {
            ++A_finder;
        }
        
        if( A_finder >= A_count )
        {
            goto exit;
        }

        const Int A_0 = A_finder;
        
        Int A = A_0;
        
        do
        {
            // Declare current face to be a face of this directed arc.
            A_face[A] = F_counter;
            
            // Declare this arc to belong to the current face.
            F_A_idx[A_counter] = A;
            
            // Move to next arc.
            A = A_left[A];
            
            ++A_counter;
        }
        while( A != A_0 );
        
        ++F_counter;
        
        F_A_ptr[F_counter] = A_counter;
    }
    
exit:
    
//    PD_ASSERT( F_counter == face_count );
    
    F_A_ptr.template Resize<true>(F_counter);

    this->SetCache( "FaceDirectedArcIndices", std::move(F_A_idx) );
    this->SetCache( "FaceDirectedArcPointers", std::move(F_A_ptr) );
    this->SetCache( "ArcFaces", std::move(A_faces_buffer) );
    
    ptoc(ClassName()+"::RequireFaces");
}
