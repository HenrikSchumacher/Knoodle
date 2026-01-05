public:

/*! @brief Cycle around `de_ptr`'s left face and evaluate `edge_fun` on each directed edge. If the template argument `ignore_virtual_edgesQ` is set to true, this will ignore virtual edges and traverse as if these were not existent.
 */

// DEBUGGING
template<
    bool debugQ = false,
    bool verboseQ = false,
    typename EdgeFun_T
>
TOOLS_FORCE_INLINE void TraverseFace(
    const Int de_ptr,
    EdgeFun_T && edge_fun,
    bool ignore_virtual_edgesQ = false
) const
{
    if ( ignore_virtual_edgesQ && DedgeVirtualQ(de_ptr) )
    {
        return;
    }
    
//    // DEBUGGING
//    
//    TOOLS_PTIMER(timer,ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")"));
    
    if constexpr ( verboseQ )
    {
        logprint(ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ".");
    }
    
    Int de_count   = Int(2) * E_V.Dim(0);
    Int de_counter = 0;
    Int de         = de_ptr;
    do
    {
        if constexpr ( debugQ )
        {
            if( de == Uninitialized )
            {
                eprint(ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Attempting to traverse invalid dedge " + ToString(de) + ".");
                return;
            }

            if( !InIntervalQ(de,Int(0),de_count) )
            {
                eprint(ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Attempting to traverse dedge " + ToString(de) + ", which is out of bounds.");
                return;
            }

            if( !DedgeActiveQ(de) )
            {
                eprint(ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Attempting to traverse inactive " + DedgeString(de) + ".");
                return;
            }
        }
        
        if ( ignore_virtual_edgesQ && DedgeVirtualQ(de) )
        {
            if constexpr ( verboseQ )
            {
                logprint("\tIgnoring virtual " + DedgeString(de) + ".");
            }
            Int de_next = E_left_dE.data()[FlipDedge(de)];
            
            if constexpr ( debugQ )
            {
                (void)this->CheckDedge<1,verboseQ>(de_next);
                
//               If de is virtual, then de_next needs to have the same tail as de.
//
//                         ^
//                         |
//                      de |
//                         | de_next
//                -------->X-------->
                
                if( E_V.data()[FlipDedge(de)] != E_V.data()[FlipDedge(de_next)] )
                {
                    eprint(ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": tail of virtual " + DedgeString(de) + " and tail of " +  DedgeString(de_next) +  " do not coincide. Data structure must be corrupted.");
                    return;
                }
            }
            de = de_next;
            return;
        }
        
        if constexpr ( verboseQ )
        {
            logprint("\tTraversing " + DedgeString(de) + ".");
        }
        
        edge_fun(de);
        
        Int de_next = E_left_dE.data()[de];
        
        if constexpr ( debugQ )
        {
            this->CheckDedge<1,verboseQ>(de_next);
            
            if( E_V.data()[de] != E_V.data()[FlipDedge(de_next)] )
            {
                eprint(ClassName()+("::TraverseFace(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Head of " + DedgeString(de) + " and tail of " +  DedgeString(de_next) +  " do not coincide. Data structure must be corrupted.");
                
                return;
            }
        }
        de = de_next;
        de_counter++;
    }
    while( (de != de_ptr) && (de_counter < de_count) );
    
    if( de_counter >= de_count ) [[unlikely]]
    {
        eprint(ClassName()+("::TraverseFace(" + DedgeString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": More dedges traversed (" + ToString(de_counter) +" ) than there are dedges (" + ToString(de_count) + "). Data structure must be corrupted.");
    }
}

/*! @brief Cycle around `de_ptr`'s and count the number of edges.
 */

Int FaceEdgeCount(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::FaceEdgeCount("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    Int f_size = 0;
    
    TraverseFace(
        de_ptr,
        [&f_size]( const Int de )
        {
            (void)de;
            ++f_size;
        },
        ignore_virtual_edgesQ
    );
    
    return f_size;
}

void MarkFaceAsExterior(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkFaceAsExterior("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsExterior(de); },
        ignore_virtual_edgesQ
    );
}

void MarkFaceAsInterior(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkFaceAsInterior("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsInterior(de); },
        ignore_virtual_edgesQ
    );
}

void MarkFaceAsVisited(
   const Int de_ptr,
   bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkFaceAsVisited("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsVisited(de); },
        ignore_virtual_edgesQ
    );
}

void MarkFaceAsUnvisited(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkFaceAsUnvisited("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseFace(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsUnvisited(de); },
        ignore_virtual_edgesQ
    );
}

void RequireFaces( bool ignore_virtual_edgesQ = false ) const
{
    TOOLS_PTIMER(timer,ClassName()+"::RequireFaces("+ ToString(ignore_virtual_edgesQ)+")");
    
//    cptr<Int> dE_left_dE = E_left_dE.data();
    
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
    
    // Same traversal as in PD_T::RequireFaces in the sense that the faces are traversed in the same order.
    
    for( Int de_ptr = 0; de_ptr < dE_count; ++de_ptr )
    {
//        // TODO: We need to check only dE_F[de_ptr].
//        if( (!DedgeActiveQ(de_ptr)) || (dE_F[de_ptr] != yet_to_be_visited) )
//        {
//            continue;
//        }
        
        if( dE_F[de_ptr] != yet_to_be_visited )
        {
            continue;
        }
        
        if( ignore_virtual_edgesQ && DedgeVirtualQ(de_ptr) )
        {
            continue;
        }
        
        const Int f = F_dE.SublistCount();
        
        TraverseFace(
            de_ptr,
            [f,dE_F,&F_dE]
            ( const Int de )
            {
                // Declare current face to be a face of this directed arc.
                dE_F[de] = f;
                // Declare this arc to belong to the current face.
                F_dE.Push(de);
            },
            ignore_virtual_edgesQ
        );
        
//        Int de = de_ptr;
//        do
//        {
//            if( ignore_virtual_edgesQ && DedgeVirtualQ(de) )
//            {
//                de = dE_left_dE[FlipDedge(de)];
//            }
//            else
//            {
//                // Declare current face to be a face of this directed arc.
//                dE_F[de] = f;
//                // Declare this arc to belong to the current face.
//                F_dE.Push(de);
//                // Move to next arc.
//                de = dE_left_dE[de];
//            }
//        }
//        while( de != de_ptr );
        
        F_dE.FinishSublist();
    }
    
    Int f_max = 0;
    Int max_f_size = F_dE.SublistSize(f_max);
    const Int f_count = F_dE.SublistCount();
    
    for( Int f = 1; f < f_count; ++f )
    {
        const Int f_size = F_dE.SublistSize(f);
        
        if( f_size > max_f_size )
        {
            f_max = f;
            max_f_size = f_size;
        }
    }
    
    std::string tag = "(" + ToString(ignore_virtual_edgesQ) + ")";
    
    this->template SetCache<false>( "FaceCount"   + tag, F_dE.SublistCount() );
    this->template SetCache<false>( "MaxFace"     + tag, f_max               );
    this->template SetCache<false>( "MaxFaceSize" + tag, max_f_size          );
    this->template SetCache<false>( "FaceDedges"  + tag, std::move(F_dE)     );
    this->template SetCache<false>( "EdgeFaces"   + tag, std::move(E_F)      );
    
}

Int FaceCount( bool ignore_virtual_edgesQ = false ) const
{
    std::string tag = "FaceCount(" + ToString(ignore_virtual_edgesQ) + ")";
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireFaces(ignore_virtual_edgesQ); }
    return this->GetCache<Int>(tag);
}

Int MaxFace( bool ignore_virtual_edgesQ = false ) const
{
    std::string tag = "MaxFace(" + ToString(ignore_virtual_edgesQ) + ")";
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireFaces(ignore_virtual_edgesQ); }
    return this->GetCache<Int>(tag);
}


Int MaxFaceSize(
    bool ignore_virtual_edgesQ = false
) const
{
    std::string tag = "MaxFaceSize(" + ToString(ignore_virtual_edgesQ) + ")";
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireFaces(ignore_virtual_edgesQ); }
    return this->GetCache<Int>(tag);
}

cref<RaggedList<Int,Int>> FaceDedges(
    bool ignore_virtual_edgesQ = false
) const
{
    std::string tag = "FaceDedges(" + ToString(ignore_virtual_edgesQ) + ")";
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireFaces(ignore_virtual_edgesQ); }
    return this->GetCache<RaggedList<Int,Int>>(tag);
}

cref<EdgeContainer_T> EdgeFaces(
    bool ignore_virtual_edgesQ = false
) const
{
    std::string tag = "EdgeFaces(" + ToString(ignore_virtual_edgesQ) + ")";
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireFaces(ignore_virtual_edgesQ); }
    return this->GetCache<EdgeContainer_T>(tag);
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
//    const Int f_count = FaceCount();
//
//    for( Int f = 0; f < f_count; ++f )
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
    PostVisit_T && post_visit,
    bool ignore_virtual_edgesQ
) const
{
    TOOLS_PTIMER(timer,MethodName("TraverseAllFaces(" + ToString(ignore_virtual_edgesQ) + ")"));
    
//    cptr<Int> dE_left_dE = E_left_dE.data();
    
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
        
        if( ignore_virtual_edgesQ && DedgeVirtualQ(de_ptr) )
        {
            continue;
        }
           
        pre_visit(f);
           
        TraverseFace(
             de_ptr,
             [f,&k,&edge_fun,this]( const Int de)
             {
                 edge_fun(f,k,de);
                 MarkDedgeAsVisited(de);
                 ++k;
             },
             ignore_virtual_edgesQ
        );
//        Int de = de_ptr;
//        do
//        {
//            if( ignore_virtual_edgesQ && DedgeVirtualQ(de) )
//            {
//                de = dE_left_dE[FlipDedge(de)];
//            }
//            else
//            {
//                edge_fun(f,k,de);
//                MarkDedgeAsVisited(de);
//                ++k;
//                // Move to next arc.
//                de = dE_left_dE[de];
//            }
//        }
//        while( de != de_ptr );
        
        post_visit(f);
        ++f;
    }
}

// Only for debugging, I guess.
Aggregator<Turn_T,Int> FaceDedgeRotations() const
{
    TOOLS_PTIMER(timer,MethodName("FaceDedgeRotations()"));
    
    Turn_T rot = 0;
    Aggregator<Turn_T,Int> rotations ( Int(2) * E_V.Dim(0) );
//    Tensor1<Turn_T,Int> rotations ( FaceDedges(false).ElementCount() );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f ){ (void)f; rot = Turn_T(0); },
        [&rotations,dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            (void)f;
            (void)k;
            rotations.Push(rot);
            rot += dE_turn[de];
        },
        []( const Int f ){ (void)f; },
        false
    );
    
    return rotations;
}

Aggregator<Turn_T,Int> FaceRotations() const
{
    TOOLS_PTIMER(timer,MethodName("FaceRotations()"));
    
    Turn_T rot = 0;
    Aggregator<Turn_T,Int> rotations ( Int(2) * E_V.Dim(0) );
    
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllFaces(
        [&rot]( const Int f )
        {
            (void)f;
            rot = Turn_T(0);
        },
        [dE_turn,&rot]( const Int f, const Int k, const Int de )
        {
            (void)f;
            (void)k;
            rot += dE_turn[de];
        },
        [&rotations,&rot]( const Int f )
        {
            (void)f;
            rotations.Push(rot);
        },
        false
    );
    
    return rotations;
}



bool CheckAllFaceTurns() const
{
    bool okayQ = true;
    Turn_T rot = 0;
    cptr<Turn_T> dE_turn = E_turn.data();
    
    Aggregator<Int,Int> face ( max_face_size );
    
    bool exteriorQ = false;
    
    TraverseAllFaces(
        [&face,&rot]( const Int f )
        {
            (void)f;
            face.Clear();
            rot = Turn_T(0);
        },
        [&face,dE_turn,&rot,&exteriorQ,this]( const Int f, const Int k, const Int de )
        {
            (void)f;
            (void)k;
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
        },
        false
    );
    
    return okayQ;
}
