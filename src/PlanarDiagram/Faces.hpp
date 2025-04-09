public:

Int FaceCount() const
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

cref<Tensor1<Int,Int>> FaceDirectedArcIndices() const
{
    const std::string tag = "FaceDirectedArcIndices";
    
    if( !this->InCacheQ(tag) )
    {
        RequireFaces();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> FaceDirectedArcPointers() const
{
    const std::string tag = "FaceDirectedArcPointers";
    
    if( !this->InCacheQ(tag) )
    {
        RequireFaces();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
    
}

cref<Tensor2<Int,Int>> ArcFaces()  const
{
    const std::string tag = "ArcFaces";
    
    if( !this->InCacheQ(tag) )
    {
        RequireFaces();
    }
    
    return this->template GetCache<Tensor2<Int,Int>>(tag);
    
}

void RequireFaces() const
{
    TOOLS_PTIC(ClassName()+"::RequireFaces");
    
    // TODO: Make ArcLeftArc cached as well?
    
    cptr<Int> A_left = ArcLeftArc().data();
    
//    auto A_left_buffer = ArcLeftArc();
//    cptr<Int> A_left = A_left_buffer.data();

    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    Tensor2<Int,Int> A_faces_buffer (initial_arc_count,2);
    
    mptr<Int> A_face = A_faces_buffer.data();
    
    // TODO: WHY???!!!
    
    // Convention: _Right_ face first:
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
    //    
    //    const Int face_count = crossing_count + 2;
    //    Tensor1<Int,Int> F_A_ptr ( face_count + 1 );
    //
    // BUT: We are actually interested in face boundary cycles.
    // When we have a disconnected planar diagram, then there may be more than one boundary cycle per face!
    
    // Euler's polyhedra formula
    //
    // F = V + 2 * (#connected components)
    //
    // and we have E = 2 * V.
    
    //
    //
    // Instead we use 2 * arc_count as upper bound for the number of faces.
    //
    // A simple loop (one arc, two faces) shows that this upper bound can be achieves.

    std::vector<Int> F_A_ptr_agg;
    F_A_ptr_agg.reserve( crossing_count + 3 );
    F_A_ptr_agg.push_back(Int(0));
    
//    Int F_counter = 0;
    
    Int A_finder = 0;
    
    while( A_finder < A_count )
    {
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
//            F_A_ptr_agg.push_back(A);
            F_A_idx[A_counter] = A;
            
            // Move to next arc.
            A = A_left[A];
            
            ++A_counter;
        }
        while( A != A_0 );
        
//        ++F_counter;
//        F_A_ptr[F_counter] = A_counter;
        
        F_A_ptr_agg.push_back(A_counter);
    }
    
exit:
    
    
    Tensor1<Int,Int> F_A_ptr ( F_A_ptr_agg.size() );
    
    F_A_ptr.Read( &F_A_ptr_agg[0] );
    
//    F_A_ptr.template Resize<true>(F_counter+1);
    
    this->SetCache( "FaceDirectedArcIndices" , std::move(F_A_idx) );
    this->SetCache( "FaceDirectedArcPointers", std::move(F_A_ptr) );
    this->SetCache( "ArcFaces", std::move(A_faces_buffer) );
    
    TOOLS_PTOC(ClassName()+"::RequireFaces");
}


Int OutsideFace()
{
    const std::string tag = "OutsideFace";
    
    if( !this->InCacheQ(tag) )
    {
        auto & FA_ptr = FaceDirectedArcPointers();
        
        const Int f_count = FaceCount();
        
        Int max_f = 0;
        Int max_a = FA_ptr[max_f+1] - FA_ptr[max_f];
        
        for( Int f = 1; f < f_count; ++f )
        {
            const Int a_count = FA_ptr[f+1] - FA_ptr[f];
            
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
