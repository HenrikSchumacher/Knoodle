public:

std::string FaceString( const Int f ) const
{
    cptr<Int> F_dA_ptr = FaceDarcs().Pointers().data();
    cptr<Int> F_dA_idx = FaceDarcs().Indices().data();
    
    const Int i_begin = F_dA_ptr[f  ];
    const Int i_end   = F_dA_ptr[f+1];
    
    const Int f_size = i_end - i_begin;
    
    return "face " + ToString(f) + " = " + ArrayToString( &F_dA_idx[i_begin], { f_size } );
}


Int FaceCount() const
{
    TOOLS_PTIMER(timer,MethodName("FaceCount"));
    return FaceDarcs().SublistCount();
}

cref<RaggedList<Int,Int>> FaceDarcs() const
{
    std::string tag ("FaceDarcs");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<RaggedList<Int,Int>>(tag);
}

cref<ArcContainer_T> ArcFaces()  const
{
    std::string tag ("ArcFaces");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<ArcContainer_T>(tag);
}

Int MaximumFace() const
{
    std::string tag ("MaximumFace");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<Int>(tag);
}

Int MaxFaceSize() const
{
    std::string tag ("MaxFaceSize");
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<Int>(tag);
}

void RequireFaces() const
{
    TOOLS_PTIMER(timer,MethodName("RequireFaces"));
    
    cptr<Int> dA_left_dA = ArcLeftDarcs().data();

    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    ArcContainer_T dA_F_buffer (max_arc_count, Uninitialized );
    
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
    
    
    // Entry Uninitialized means "unvisited but to be visited".
    // Entry Uninitialized - 1 means "do not visit".
    
    for( Int a = 0; a < max_arc_count; ++ a )
    {
        const Int da = Int(2) * a;
        
        if( ArcActiveQ(a) )
        {
            dA_F[da         ] = Uninitialized;
            dA_F[da + Int(1)] = Uninitialized;
        }
        else
        {
            dA_F[da         ] = Uninitialized - Int(1);
            dA_F[da + Int(1)] = Uninitialized - Int(1);
        }
    }
    
    const Int dA_count = 2 * max_arc_count;
    
//    Int dA_counter = 0;
    
    // Each arc will appear in two faces.
//    Tensor1<Int,Int> F_dA_idx ( dA_count );
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
//    Aggregator<Int,Int> F_dA_ptr_agg ( expected_face_count + Int(1) );
//    F_dA_ptr_agg.Push(Int(0));
    
    RaggedList<Int,Int> F_dA ( expected_face_count + Int(1), dA_count );
    
    Int max_f    = 0;
    Int max_size = 0;
    
    for( Int da_0 = 0; da_0 < dA_count; ++da_0 )
    {
        if( dA_F[da_0] != Uninitialized ) { continue; }
        
        const Int count_0 = F_dA.ElementCount();
        
        const Int f = F_dA.SublistCount();
        
        Int da = da_0;
        do
        {
            // Declare current face to be a face of this directed arc.
//            dA_F[da] = F_dA_ptr_agg.Size() - Int(1);
            dA_F[da] = f;
            
            // Declare this arc to belong to the current face.
//            F_dA_idx[dA_counter] = da;
            F_dA.Push(da);
            
            // Move to next arc.
            da = dA_left_dA[da];
            
//            ++dA_counter;
        }
        while( da != da_0 );
        
        const Int count_1 = F_dA.ElementCount();
        const Int f_size = count_1 - count_0;
        
        if( f_size > max_size )
        {
            max_f = f;
            max_size = f_size;
        }
        
        F_dA.FinishSublist();
    }
    
    this->SetCache( "FaceDarcs"  , std::move(F_dA)        );
    this->SetCache( "ArcFaces"   , std::move(dA_F_buffer) );
    this->SetCache( "MaxFaceSize", max_size               );
    this->SetCache( "MaximumFace", max_f                  );
}


cref<Tiny::VectorList_AoS<4,Int,Int>> CrossingFaces() const
{
    using Container_T =  Tiny::VectorList_AoS<4,Int,Int>;
    
    std::string tag = "CrossingFaces";
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
    if(!this->InCacheQ(tag))
    {
        Container_T C_faces ( max_crossing_count );
        
        const auto & A_F = ArcFaces();
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( !CrossingActiveQ(c) )
            {
                C_faces(c,0) = Uninitialized;
                C_faces(c,1) = Uninitialized;
                C_faces(c,2) = Uninitialized;
                C_faces(c,3) = Uninitialized;
            }
            else
            {

//                              O       O C_arcs(c,Out,Right)
//                               ^     ^
//                                \   /
//                                 \ /
//    A_F(C_arcs(c,In,Left),Head)   X   A_F(C_arcs(c,Out,Right),Tail)
//                                 ^ ^
//                                /   \
//                               /     \
//                              O       O
                const Int a_1 = C_arcs(c,Out,Right);
                const Int a_0 = C_arcs(c,In ,Left );
                
                C_faces(c,0) = A_F(a_1,Tail);
                C_faces(c,1) = A_F(a_1,Head);
                C_faces(c,2) = A_F(a_0,Head);
                C_faces(c,3) = A_F(a_0,Tail);
            }
        }
        
        this->SetCache(tag,std::move(C_faces));
    }
    return this->template GetCache<Container_T>(tag);
}


Tensor1<Int8,Int> CheckerBoardColoring()
{
    TOOLS_PTIMER(timer,MethodName("CheckerBoardColoring"));
    
    MultiGraph_T G ( FaceCount(), ArcFaces() );
    
    using DedgeNode = MultiGraph_T::DedgeNode;
    
    Tensor1<Int8,Int> color( FaceCount(), Int8(0) );
    
    G.DepthFirstSearch(
        MultiGraph_T::TrivialEdgeFunction,     // discover
        MultiGraph_T::TrivialEdgeFunction,     // rediscover
        [&color]( cref<DedgeNode> E )          // previsit
        {
            if( E.tail == MultiGraph_T::UninitializedVertex )
            {
                color[E.head] = Int8(1);
            }
            else
            {
                color[E.head] = -color[E.tail];
            }
        },
        MultiGraph_T::TrivialEdgeFunction      // postvisit
    );
    
    return color;
}
