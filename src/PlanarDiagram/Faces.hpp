public:

/*!@brief Return a string with the most important information of a face `f`. Meant for debugging.*/
std::string FaceString( const Int f ) const
{
    cptr<Int> F_dA_ptr = FaceDarcs().Pointers().data();
    cptr<Int> F_dA_idx = FaceDarcs().Indices().data();
    
    const Int i_begin = F_dA_ptr[f  ];
    const Int i_end   = F_dA_ptr[f+1];
    
    const Int f_size = i_end - i_begin;
    
    return "face " + ToString(f) + " = " + OutString::FromVector( &F_dA_idx[i_begin], f_size );
    
}

/*!@brief Return the number of faces in the diagram. CAUTION: Faces here are characterized by their boundary components only. If the diagram is connected, then this coincide with the ordinary meaning of faces.*/
Int FaceCount() const
{
//    TOOLS_PTIMER(timer,MethodName("FaceCount"));
    return FaceDarcs().SublistCount();
}

/*!@brief For each face return the list of _darcs_ (i.e., directed arcs) in the ordering they are traversed when cycling counter-clockwise around the face. The orientation of the darcs is so that the corresponding face lies on its left side.*/
cref<RaggedList<Int,Int>> FaceDarcs() const
{
    std::string tag ("FaceDarcs");
//    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<RaggedList<Int,Int>>(tag);
}

/*!@brief For each arc list the two faces. The convention is that `ArcFaces()(a,d)` is the face to the _left_ of the directed arc `ToDarc(a,d)`; in particular, `ArcFaces()(a,1)` lies to the left of the forward arc `ToDarc(a,Head)` of `a`, and `ArcFaces()(a,0)` to its right. (This matches the "right face first" convention documented in `ComputeFaces`, and the face-on-left orientation of `FaceDarcs()`.)*/
cref<ArcContainer_T> ArcFaces()  const
{
    std::string tag ("ArcFaces");
//    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<ArcContainer_T>(tag);
}

/*!@brief Return the index of a face with maximal number of arcs.*/
Int MaximumFace() const
{
    std::string tag ("MaximumFace");
//    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<Int>(tag);
}

/*!@brief Return the maximal size (= number of arcs) of all faces..*/
Int MaxFaceSize() const
{
    std::string tag ("MaxFaceSize");
//    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)) { RequireFaces(); }
    return this->template GetCache<Int>(tag);
}

template<bool lutQ = true, typename ArcFun_T>
TOOLS_FORCE_INLINE void TraverseFaceAtDarc( const Int da_0, ArcFun_T & arc_fun ) const
{
    if( !ArcActiveQ(ArcOfDarc(da_0)) ) { return; }
    
    Int * TOOLS_RESTRICT dA_left_dA = COND(lutQ,ArcLeftDarcs().data(),nullptr);
    
    Int da = da_0;
    do
    {
        // Do some work.
        arc_fun(da);

        // Move to next arc.
        if constexpr( lutQ )
        {
            da = dA_left_dA[da];
        }
        else
        {
            da = LeftDarc(da);
        }
    }
    while( da != da_0 );
}

/*!@brief Make sure that the face information is computed, i.e., the internal values of `FaceDarcs()`, `ArcFaces()`, `MaxFaceSize`, `MaximumFace`, `FaceCount()`. Note that the face information is cached and might get stale. Some other data depending on this might also become cached. So id you want to recompute this, it is in general safer to call `ClearCache()` and to rely on the fact that downstream data function will call `RequireFaces()`.*/
void RequireFaces() const
{
//    TOOLS_PTIMER(timer,MethodName("RequireFaces"));
    
    if(
       !this->InCacheQ( "FaceDarcs")   || !this->InCacheQ( "ArcFaces")
       ||
       !this->InCacheQ( "MaxFaceSize") || !this->InCacheQ( "MaximumFace")
    )
    {
        ComputeFaces();
    }
    
}

/*!@brief (Re-)compute face information, i.e., the internal values of `FaceDarcs()`, `ArcFaces()`, `MaxFaceSize`, `MaximumFace`, `FaceCount()`. Note that the face information is cached and might get stale. Some other data depending on this this might also be cached. So id you want to recompute this, it is in general safer to call `ClearCache()` and to rely on the fact that downstream data function will call `RequireFaces()`.*/
void ComputeFaces() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeFaces"));
    
    cptr<Int> dA_left_dA = ArcLeftDarcs().data();

    PD_ASSERT(CheckArcLeftDarcs());
    
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
    // This way the directed arc da = 2 * a + d has its left face in dA_f[da].
    
    
    // An entry with value Uninitialized means "unvisited but to be visited".
    // An entry with value DoNotVisit means "do not visit".
    
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
            dA_F[da         ] = DoNotVisit;
            dA_F[da + Int(1)] = DoNotVisit;
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
    
    this->template SetCache<true>( "FaceDarcs"  , std::move(F_dA)        );
    this->template SetCache<true>( "ArcFaces"   , std::move(dA_F_buffer) );
    this->template SetCache<true>( "MaxFaceSize", max_size               );
    this->template SetCache<true>( "MaximumFace", max_f                  );
}

/*!@brief For each face provide a list of crossings on its boundary (possibly with repetitions) in counter-clockwise order.*/
RaggedList<Int,Int> FaceCrossings() const
{
    auto & F_dA = FaceDarcs();
    RaggedList<Int,Int> F_C ( F_dA.SublistCount(), F_dA.ElementCount() ) ;
    
    for( Int i = 0; i < F_dA.SublistCount(); ++i )
    {
        for( Int da : F_dA.Sublist(i) )
        {
            auto [a,d] = FromDarc(da);
            F_C.Push(A_cross(a,!d)); // Always use the tail of the darc.
        }
        
        F_C.FinishSublist();
    }
    
    return F_C;
}

/*!@brief For each crossing list the faces in the ordering east, north, west, south. (Imagine the crossing turned so that the two outgoing arcs point  north-west and north-east.)*/
Tiny::VectorList_AoS<4,Int,Int> CrossingFaces() const
{
    using Container_T =  Tiny::VectorList_AoS<4,Int,Int>;
    
    std::string tag = "CrossingFaces";
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
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

            /*                              O       O C_arcs(c,Out,Right)
             *                               ^     ^
             *                                \   /
             *                                 \ /
             *    A_F(C_arcs(c,In,Left),Head)   X   A_F(C_arcs(c,Out,Right),Tail)
             *                                 ^ ^
             *                                /   \
             *                               /     \
             *                              O       O
             */
            
            const Int a_1 = C_arcs(c,Out,Right);
            const Int a_0 = C_arcs(c,In ,Left );
            
            C_faces(c,0) = A_F(a_1,Tail);
            C_faces(c,1) = A_F(a_1,Head);
            C_faces(c,2) = A_F(a_0,Head);
            C_faces(c,3) = A_F(a_0,Tail);
        }
    }
    
    return C_faces;
}

/*!@brief For each face return a number +1 or -1 according to a checkerboard coloring. Note that there are 2^c such checkboard colorings, where c is the number of connected components. This function just picks one of them.*/
Tensor1<Int8,Int> CheckerBoardColoring() const
{
    TOOLS_PTIMER(timer,MethodName("CheckerBoardColoring"));
    
    if( ArcCount() != MaxArcCount() )
    {
        eprint(MethodName("CheckerBoardColoring") + ": Diagram contains deactivated arcs. This algorithm uses the class " + MultiGraph_T::ClassName() + " and works only if all arcs are active. We have to abort here. Try it again after you compressed the diagram with `Compress` or `CreateCompressed`.");
        
        return Tensor1<Int8,Int>();
    }
    
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
                color[E.head] = Int8(-color[E.tail]);
            }
        },
        MultiGraph_T::TrivialEdgeFunction      // postvisit
    );
    
    return color;
}


/*!@brief For each color return a list for winding numbers for each face. That is, `ColorFaceWindingNumbers(color)[f]` is the winding number of the subdiagram of color `color` around face `f`.
 *
 * Note that we do not really specify an external face, so these winding numbers are not uniquely defined. Moreover, if the diagram is not connected, one would have to pick an external face for each connected component. The algorithm chooses such external faces automatically depth-first traversal of the dual graph. For a connected diagram you can make face `f0` the external face by subtracting `ColorFaceWindingNumbers(color])[f0]` from `ColorFaceWindingNumbers(color])[f]` for all faces `f`.
 *
 *  Preconditions: This algorithm uses the class `MultiGraph_T` and works only if all arcs. This precondition can be satisfied by calling `Compress` first or by creating a new diagram with `CreateCompressed`."
 *
 *  @return If the routine succeeds, it returns a `Tensor1` object of size equal to the number of faces. If it fails (e.g., because the diagram is invalied or when the diagram contains inactive arcs), then an empty container is returned.
 */
cref<Tensor1<ToSigned<Int>,Int>> ColorFaceWindingNumbers( const Int color ) const
{
    TOOLS_PTIMER(timer,MethodName("ColorFaceWindingNumbers"));
    
    std::string tag = "ColorFaceWindingNumbers";
    
    if(!this->InCacheQ(tag)) { ComputeColorFaceWindingNumbers(); }
    
    using Count_T     = ToSigned<Int>;
    using Container_T = AssociativeContainer<Int,Tensor1<Count_T,Int>>;
    
    const auto & a = this->template GetCache<Container_T>(tag);
    
    if( (color != InvalidColor) && (a.contains(color)) )
    {
        return a.at(color);
    }
    else
    {
        return a.at(InvalidColor);
    }
}

private:
    
void ComputeColorFaceWindingNumbers() const
{
    using Count_T   = ToSigned<Int>;
    using DedgeNode = MultiGraph_T::DedgeNode;
    
    AssociativeContainer<Int,Tensor1<Count_T,Int>> container;
    
    if( InvalidQ() )
    {
        this->SetCache("ColorFaceWindingNumbers",std::move(container));
        return;
    }
    
    if( ArcCount() != MaxArcCount() )
    {
        eprint(MethodName("ComputeColorFaceWindingNumbers") + ": Diagram contains deactivated arcs. This algorithm uses the class " + MultiGraph_T::ClassName() + " and works only if all arcs are active. We have to abort here. Try it again after you compressed the diagram with `Compress` or `CreateCompressed`.");
        
        this->SetCache("ColorFaceWindingNumbers",std::move(container));
        return;
    }
    
    ColorCounts_T color_arc_counts = ColorArcCounts();
    const Int face_count = FaceCount();
    
    for( auto [col,count] : color_arc_counts )
    {
        container[col] = Tensor1<Count_T,Int> ( face_count, Count_T(0) );
    }
    
    // CAUTION: We add a extra key to store the result to be returned in the case of a query to a nonexistent color.
    container[InvalidColor] = Tensor1<ToSigned<Int>,Int>();
    
    MultiGraph_T G ( face_count, ArcFaces() );
    
    G.DepthFirstSearch(
        MultiGraph_T::TrivialEdgeFunction,     // discover
        MultiGraph_T::TrivialEdgeFunction,     // rediscover
        [&container,this]( cref<DedgeNode> E ) // previsit
        {
            if( E.tail == MultiGraph_T::UninitializedVertex ) { return; }
            
            auto [e,d] = MultiGraph_T::FromDedge(E.de);
            const Int e_col = A_color[e];
            
            for( auto & [col,w] : container )
            {
                if( col == InvalidColor ) { continue; }
                
                w[E.head] = w[E.tail] +
                    ( (col != e_col) ? Count_T(0) : (d  ? Count_T(-1) : Count_T(1)) );
                
                // If d == Head, then we go from the right face of arc e to the left face. (Mind the slightly odd convention to place the right face first in a dual edge.)
            }
        },
        MultiGraph_T::TrivialEdgeFunction      // postvisit
    );
    
    this->SetCache("ColorFaceWindingNumbers", std::move(container));
}
