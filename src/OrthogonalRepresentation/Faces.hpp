public:

/*! @brief Cycle around `de_ptr`'s left face and evaluate `edge_fun` on each directed edge.
 */

template<typename EdgeFun_T>
void TraverseFace( const Int de_ptr, EdgeFun_T && edge_fun ) const
{
    Int de = de_ptr;
    do
    {
        edge_fun(de);
        de = E_left_dE.data()[de];
    }
    while( de != de_ptr );
}

/*! @brief Cycle around `de_ptr`'s and count the number of edges.
 */

Int FaceEdgeCount( const  Int de_ptr )
{
    Int de_counter = 0;
    
    TraverseFace(
        de_ptr,
        [&de_counter]( const Int de ) { ++de_counter; }
    );
    
    return de_counter;
}


void MarkDedgeAsExterior( const Int de ) const
{
    E_flag.data()[de] |= EdgeExteriorMask;
}

void MarkDedgeAsInterior( const Int de ) const
{
    E_flag.data()[de] &= ~EdgeExteriorMask;
}

void MarkDedgeAsVisited( const Int de ) const
{
    E_flag.data()[de] |= EdgeVisitedMask;
}

void MarkDedgeAsUnvisited( const Int de ) const
{
    E_flag.data()[de] &= ~EdgeVisitedMask;
}

void MarkDedgeAsUnconstrained( const Int de ) const
{
    E_flag.data()[de] |= EdgeUnconstrainedMask;
}

void MarkDedgeAsConstrained( const Int de ) const
{
    E_flag.data()[de] &= ~EdgeUnconstrainedMask;
}



void MarkFaceAsExterior( const Int de_ptr ) const
{
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsExterior(de); }
    );
}


void MarkFaceAsInterior( const Int de_ptr ) const
{
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsInterior(de); }
    );
}

void MarkFaceAsVisited( const Int de_ptr ) const
{
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsVisited(de); }
    );
}

void MarkFaceAsUnvisited( const Int de_ptr ) const
{
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsUnvisited(de); }
    );
}

void RequireFaces() const
{
    TOOLS_PTIMER(timer,ClassName()+"::RequireFaces");
    
    cptr<Int> dE_left_dE = E_left_dE.data();
    
    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    EdgeContainer_T E_F ( E_V.Dim(0) );
    
    mptr<Int> dE_F = E_F.data();
    
    // Convention: _Right_ face first:
    //
    //            E_F(e,0)
    //
    //            <------------|  e
    //
    //            E_F(e,1)
    //
    // This way the directed edge de = 2 * e + Head has its left face in dE_F[de].

    
    constexpr Int yet_to_be_visited = Uninitialized - Int(1);
    
    const Int dE_count = Int(2) * E_F.Dim(0);
    
    for( Int de = 0; de < dE_count; ++de )
    {
        if( DedgeActiveQ(de) )
        {
            dE_F[de] = yet_to_be_visited;
        }
        else
        {
            dE_F[de] = Uninitialized;
        }
    }
        
    RaggedList<Int,Int> F_dE ( dE_count + Int(1),  dE_count );
    
    // Same traversal as in PlanarDiagram_T::RequireFaces in the sense that the faces are traversed in the same order.
    
    for( Int de_ptr = 0; de_ptr < dE_count; ++de_ptr )
    {
        // TODO: We need to check only dE_F[de_ptr].
        if( (!DedgeActiveQ(de_ptr)) || (dE_F[de_ptr] != yet_to_be_visited) )
        {
            continue;
        }
        const Int f = F_dE.SublistCount();
        Int de = de_ptr;
        do
        {
            // Declare current face to be a face of this directed arc.
            dE_F[de] = f;
            // Declare this arc to belong to the current face.
            F_dE.Push(de);
            // Move to next arc.
            de = dE_left_dE[de];
        }
        while( de != de_ptr );
        
        F_dE.FinishSublist();
    }
    
    Int f_max = 0;
    Int max_f_size = F_dE.SublistSize(f_max);
    
    for( Int f = 1; f < F_dE.SublistCount(); ++f )
    {
        const Int f_size = F_dE.SublistSize(f);
        
        if( f_size > max_f_size )
        {
            f_max = f;
            max_f_size = f_size;
        }
    }
    
    this->SetCache( "FaceCount"  , F_dE.SublistCount() );
    this->SetCache( "MaxFace"    , f_max               );
    this->SetCache( "MaxFaceSize", max_f_size          );
    this->SetCache( "FaceDedges" , std::move(F_dE)     );
    this->SetCache( "EdgeFaces"  , std::move(E_F)      );
    
}

Int FaceCount() const
{
    TOOLS_PTIMER(timer,MethodName("FaceCount"));
    if( !this->InCacheQ("FaceCount") ) { RequireFaces(); }
    return this->GetCache<Int>("FaceCount");
}

Int MaxFace() const
{
    TOOLS_PTIMER(timer,MethodName("MaxFace"));
    if( !this->InCacheQ("MaxFace") ) { RequireFaces(); }
    return this->GetCache<Int>("MaxFace");
}

Int MaxFaceSize() const
{
    TOOLS_PTIMER(timer,MethodName("MaxFaceSize"));
    if( !this->InCacheQ("MaxFaceSize") ) { RequireFaces(); }
    return this->GetCache<Int>("MaxFaceSize");
}

cref<RaggedList<Int,Int>> FaceDedges() const
{
    TOOLS_PTIMER(timer,MethodName("FaceDedges"));
    if( !this->InCacheQ("FaceDedges") ) { RequireFaces(); }
    return this->GetCache<RaggedList<Int,Int>>("FaceDedges");
}

cref<EdgeContainer_T> EdgeFaces() const
{
    TOOLS_PTIMER(timer,MethodName("EdgeFaces"));
    if( !this->InCacheQ("EdgeFaces") ) { RequireFaces(); }
    return this->GetCache<EdgeContainer_T>("EdgeFaces");
}

//template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
//void TraverseAllFaces(
//    PreVisit_T  && pre_visit,
//    EdgeFun_T   && edge_fun,
//    PostVisit_T && post_visit
//) const
//{
//    TOOLS_PTIMER(timer,ClassName()+"::TraverseAllFaces");
//    
//    auto & F_dE_ptr = FaceDedges().Pointers();
//    auto & F_dE_idx = FaceDedges().Elements();
//    
//    for( Int f = 0; f < FaceCount(); ++f )
//    {
//        const Int k_begin = F_dE_ptr[f    ];
//        const Int k_end   = F_dE_ptr[f + 1];
//        
//        pre_visit(f);
//        
//        for( Int k = k_begin; k < k_end; ++k )
//        {
//            const Int de = F_dE_idx[k];
//            
//            edge_fun(f,k,de);
//        }
//        
//        post_visit(f);
//    }
//}

template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
void TraverseAllFaces(
    PreVisit_T  && pre_visit,
    EdgeFun_T   && edge_fun,
    PostVisit_T && post_visit
) const
{
    TOOLS_PTIMER(timer,MethodName("TraverseAllFaces"));
    
    cptr<Int> dE_left_dE = E_left_dE.data();
    
    const Int dE_count = Int(2) * E_left_dE.Dim(0);

    for( Int de = 0; de < dE_count; ++de )
    {
        MarkDedgeAsUnvisited(de);
    }
    
    Int f = 0;
    Int k = 0;
    
    for( Int de_ptr = 0; de_ptr < dE_count; ++de_ptr )
    {
        if( !DedgeActiveQ(de_ptr) || DedgeVisitedQ(de_ptr) )
        {
            continue;
        }
           
        pre_visit(f);
           
        Int de = de_ptr;
        do
        {
            edge_fun(f,k,de);
            MarkDedgeAsVisited(de);
            ++k;
            // Move to next arc.
            de = dE_left_dE[de];
        }
        while( de != de_ptr );
        
        post_visit(f);
        ++f;
    }
}

// Only for debugging, I guess.
Tensor1<Turn_T,Int> FaceDedgeRotations() const
{
    TOOLS_PTIMER(timer,MethodName("FaceDedgeRotations"));
    
    Turn_T rot = 0;
    Tensor1<Turn_T,Int> rotations ( FaceDedges().ElementCount() );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f ){ (void)f; rot = Turn_T(0); },
        [&rotations,dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            rotations[k] = rot;
            rot += dE_turn[de];
        },
        []( const Int f ){ (void)f; }
    );
    
    return rotations;
}

Tensor1<Turn_T,Int> FaceRotations() const
{
    TOOLS_PTIMER(timer,MethodName("FaceRotations"));
    
    Turn_T rot = 0;
    Tensor1<Turn_T,Int> rotations ( FaceCount() );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f ){ rot = Turn_T(0); },
        [dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            rot += dE_turn[de];
        },
        [&rotations,&rot]( const Int f ){ rotations[f] = rot; }
    );
    
    return rotations;
}


// This one requires precomputation of faces and thus should better not called during turn regularization.
bool CheckAllFaceTurns() const
{
    bool okayQ = true;
    Turn_T rot = 0;
    cptr<Turn_T> dE_turn = E_turn.data();
    
    Aggregator<Int,Int> face ( max_face_size );
    
    bool exteriorQ = false;
    
    TraverseAllFaces(
        [&face,&rot]( const Int f ){
            face.Clear();
            rot = Turn_T(0);
        },
        [&face,dE_turn,&rot,&exteriorQ,this]( const Int f, const Int k, const Int de )
        {
            face.Push(de);
            rot += dE_turn[de];
            exteriorQ = DedgeExteriorQ(de);
        },
        [&face,&rot,&okayQ,&exteriorQ]( const Int f )
        {
            if( exteriorQ )
            {
                bool this_okayQ = (rot == Turn_T(-4));
                
                if( !this_okayQ )
                {
                    eprint(ClassName() + "::CheckAllFaceTurns: face f = " + ToString(f) + " is exterior face with incorrect rotation number.");
                    TOOLS_DDUMP(rot);
                    TOOLS_DDUMP(face);
                }
                
                okayQ = okayQ && this_okayQ;
            }
            else
            {
                bool this_okayQ = ( rot == Turn_T( 4) );
                
                if( !this_okayQ )
                {
                    eprint(ClassName() + "::CheckAllFaceTurns: face f = " + ToString(f) + " is interior face with incorrect rotation number.");
                    TOOLS_DDUMP(rot);
                    TOOLS_DDUMP(face);
                }
                okayQ = okayQ && this_okayQ;
            }
        }
    );
    
    return okayQ;
}
