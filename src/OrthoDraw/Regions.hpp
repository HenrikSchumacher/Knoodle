public:

/*! @brief Cycle around `de_ptr`'s left region and evaluate `edge_fun` on each directed edge. If the template argument `ignore_virtual_edgesQ` is set to true, this will ignore virtual edges and traverse as if these were not existent.
 */

template<
    bool debugQ = false,
    bool verboseQ = false,
    typename EdgeFun_T
>
TOOLS_FORCE_INLINE void TraverseRegion(
    const Int de_ptr,
    EdgeFun_T && edge_fun,
    bool ignore_virtual_edgesQ = false
) const
{
    if ( ignore_virtual_edgesQ && DedgeVirtualQ(de_ptr) )
    {
        return;
    }
    
    if constexpr ( verboseQ )
    {
        logprint(ClassName()+("::TraverseRegion(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ".");
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
                eprint(ClassName()+("::TraverseRegion(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Attempting to traverse invalid dedge " + ToString(de) + ".");
                return;
            }

            if( !InIntervalQ(de,Int(0),de_count) )
            {
                eprint(ClassName()+("::TraverseRegion(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Attempting to traverse dedge " + ToString(de) + ", which is out of bounds.");
                return;
            }

            if( !DedgeActiveQ(de) )
            {
                eprint(ClassName()+("::TraverseRegion(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Attempting to traverse inactive " + DedgeString(de) + ".");
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
                    
                    error(ClassName()+("::TraverseRegion(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": tail of virtual " + DedgeString(de) + " and tail of " +  DedgeString(de_next) +  " do not coincide. Data structure must be corrupted.");
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
                error(ClassName()+("::TraverseRegion(" + ToString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": Head of " + DedgeString(de) + " and tail of " +  DedgeString(de_next) +  " do not coincide. Data structure must be corrupted.");
                
                return;
            }
        }
        de = de_next;
        de_counter++;
    }
    while( (de != de_ptr) && (de_counter <= de_count) );
    
    if( de_counter > de_count ) [[unlikely]]
    {
        error(ClassName()+("::TraverseRegion(" + DedgeString(de_ptr) + "," + ToString(ignore_virtual_edgesQ) + ")") + ": More dedges traversed (" + ToString(de_counter) +") than there are dedges (" + ToString(de_count) + "). Data structure must be corrupted.");
    }
}

/*! @brief Cycle around `de_ptr`'s and count the number of edges.
 */

Int RegionEdgeCount(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::RegionEdgeCount("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    Int r_size = 0;
    
    TraverseRegion(
        de_ptr,
        [&r_size]( const Int de )
        {
            (void)de;
            ++r_size;
        },
        ignore_virtual_edgesQ
    );
    
    return r_size;
}

void MarkRegionAsExterior(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkRegionAsExterior("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseRegion(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsExterior(de); },
        ignore_virtual_edgesQ
    );
}

void MarkRegionAsInterior(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkRegionAsInterior("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseRegion(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsInterior(de); },
        ignore_virtual_edgesQ
    );
}

void MarkRegionAsVisited(
   const Int de_ptr,
   bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkRegionAsVisited("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseRegion(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsVisited(de); },
        ignore_virtual_edgesQ
    );
}

void MarkRegionAsUnvisited(
    const Int de_ptr,
    bool ignore_virtual_edgesQ = false
)
{
//    TOOLS_PTIMER(timer,ClassName()+"::MarkRegionAsUnvisited("+DedgeString(de_ptr)+","+ ToString(ignore_virtual_edgesQ)+")");
    
    TraverseRegion(
        de_ptr,
        [this]( const Int de ) { MarkDedgeAsUnvisited(de); },
        ignore_virtual_edgesQ
    );
}

void RequireRegions( bool ignore_virtual_edgesQ = false ) const
{
    TOOLS_PTIMER(timer,ClassName()+"::RequireRegions("+ ToString(ignore_virtual_edgesQ)+")");
    
//    cptr<Int> dE_left_dE = E_left_dE.data();
    
    // These are going to become edges of the dual graph(s). One dual edge for each arc.
    EdgeContainer_T E_R ( E_V.Dim(0) );
    
    mptr<Int> dE_R = E_R.data();
    
    // Convention: _Right_ region first:
    //
    //            E_R(e,0)
    //
    //            <------------|  e
    //
    //            E_R(e,1)
    //
    // This way the directed edge de = 2 * e + Head has its left region in dE_R[de].

    
    constexpr Int yet_to_be_visited = Uninitialized - Int(1);
    
    const Int dE_count = Int(2) * E_R.Dim(0);
    
    for( Int de = 0; de < dE_count; ++de )
    {
        if( DedgeActiveQ(de) )
        {
            dE_R[de] = yet_to_be_visited;
        }
        else
        {
            dE_R[de] = Uninitialized;
        }
    }
        
    RaggedList<Int,Int> R_dE ( dE_count + Int(1),  dE_count );
    
    // Same traversal as in PD_T::RequireRegions in the sense that the regions are traversed in the same order.
    
    for( Int de_ptr = 0; de_ptr < dE_count; ++de_ptr )
    {
//        // TODO: We need to check only dE_R[de_ptr].
//        if( (!DedgeActiveQ(de_ptr)) || (dE_R[de_ptr] != yet_to_be_visited) )
//        {
//            continue;
//        }
        
        if( dE_R[de_ptr] != yet_to_be_visited )
        {
            continue;
        }
        
        if( ignore_virtual_edgesQ && DedgeVirtualQ(de_ptr) )
        {
            continue;
        }
        
        const Int r = R_dE.SublistCount();
        
        TraverseRegion(
            de_ptr,
            [r,dE_R,&R_dE]
            ( const Int de )
            {
                // Declare current region to be a region of this directed arc.
                dE_R[de] = r;
                // Declare this arc to belong to the current region.
                R_dE.Push(de);
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
//                // Declare current region to be a region of this directed arc.
//                dE_R[de] = r;
//                // Declare this arc to belong to the current region.
//                R_dE.Push(de);
//                // Move to next arc.
//                de = dE_left_dE[de];
//            }
//        }
//        while( de != de_ptr );
        
        R_dE.FinishSublist();
    }
    
    Int r_max = 0;
    Int max_r_size = R_dE.SublistSize(r_max);
    const Int r_count = R_dE.SublistCount();
    
    for( Int r = 1; r < r_count; ++r )
    {
        const Int r_size = R_dE.SublistSize(r);
        
        if( r_size > max_r_size )
        {
            r_max = r;
            max_r_size = r_size;
        }
    }
    
    std::string tag = "(" + ToString(ignore_virtual_edgesQ) + ")";
    
    this->template SetCache<false>( "RegionCount"   + tag, R_dE.SublistCount() );
    this->template SetCache<false>( "MaxRegion"     + tag, r_max               );
    this->template SetCache<false>( "MaxRegionSize" + tag, max_r_size          );
    this->template SetCache<false>( "RegionDedges"  + tag, std::move(R_dE)     );
    this->template SetCache<false>( "EdgeRegions"   + tag, std::move(E_R)      );
    
}

Int RegionCount( bool ignore_virtual_edgesQ = false ) const
{
    std::string tag = "RegionCount(" + ToString(ignore_virtual_edgesQ) + ")";
//    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireRegions(ignore_virtual_edgesQ); }
    return this->GetCache<Int>(tag);
}

Int MaxRegion( bool ignore_virtual_edgesQ = false ) const
{
    std::string tag = "MaxRegion(" + ToString(ignore_virtual_edgesQ) + ")";
//    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireRegions(ignore_virtual_edgesQ); }
    return this->GetCache<Int>(tag);
}


Int MaxRegionSize(
    bool ignore_virtual_edgesQ = false
) const
{
    std::string tag = "MaxRegionSize(" + ToString(ignore_virtual_edgesQ) + ")";
//    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireRegions(ignore_virtual_edgesQ); }
    return this->GetCache<Int>(tag);
}

cref<RaggedList<Int,Int>> RegionDedges(
    bool ignore_virtual_edgesQ = false
) const
{
    std::string tag = "RegionDedges(" + ToString(ignore_virtual_edgesQ) + ")";
//    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireRegions(ignore_virtual_edgesQ); }
    return this->GetCache<RaggedList<Int,Int>>(tag);
}

cref<EdgeContainer_T> EdgeRegions(
    bool ignore_virtual_edgesQ = false
) const
{
    std::string tag = "EdgeRegions(" + ToString(ignore_virtual_edgesQ) + ")";
//    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) ) { RequireRegions(ignore_virtual_edgesQ); }
    return this->GetCache<EdgeContainer_T>(tag);
}

//template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
//void TraverseAllRegions(
//    PreVisit_T  && pre_visit,
//    EdgeFun_T   && edge_fun,
//    PostVisit_T && post_visit
//) const
//{
//    TOOLS_PTIMER(timer,ClassName()+"::TraverseAllRegions");
//
//    auto & R_dE_ptr = RegionDedges().Pointers();
//    auto & R_dE_idx = RegionDedges().Elements();
//    const Int r_count = RegionCount();
//
//    for( Int r = 0; r < r_count; ++r )
//    {
//        const Int k_begin = R_dE_ptr[r    ];
//        const Int k_end   = R_dE_ptr[r + 1];
//        
//        pre_visit(r);
//        
//        for( Int k = k_begin; k < k_end; ++k )
//        {
//            const Int de = R_dE_idx[k];
//            
//            edge_fun(r,k,de);
//        }
//        
//        post_visit(r);
//    }
//}

template<typename PreVisit_T, typename EdgeFun_T, typename PostVisit_T>
void TraverseAllRegions(
    PreVisit_T  && pre_visit,
    EdgeFun_T   && edge_fun,
    PostVisit_T && post_visit,
    bool ignore_virtual_edgesQ
) const
{
    TOOLS_PTIMER(timer,MethodName("TraverseAllRegions(" + ToString(ignore_virtual_edgesQ) + ")"));
    
//    cptr<Int> dE_left_dE = E_left_dE.data();
    
    const Int dE_count = Int(2) * E_left_dE.Dim(0);

    for( Int de = 0; de < dE_count; ++de )
    {
        MarkDedgeAsUnvisited(de);
    }
    
    Int r = 0;
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
           
        pre_visit(r);
           
        TraverseRegion(
             de_ptr,
             [r,&k,&edge_fun,this]( const Int de)
             {
                 edge_fun(r,k,de);
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
//                edge_fun(r,k,de);
//                MarkDedgeAsVisited(de);
//                ++k;
//                // Move to next arc.
//                de = dE_left_dE[de];
//            }
//        }
//        while( de != de_ptr );
        
        post_visit(r);
        ++r;
    }
}

// Only for debugging, I guess.
Aggregator<Turn_T,Int> RegionDedgeRotations() const
{
    TOOLS_PTIMER(timer,MethodName("RegionDedgeRotations()"));
    
    Turn_T rot = 0;
    Aggregator<Turn_T,Int> rotations ( Int(2) * E_V.Dim(0) );
//    Tensor1<Turn_T,Int> rotations ( RegionDedges(false).ElementCount() );
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllRegions(
        [&rot]( const Int r ){ (void)r; rot = Turn_T(0); },
        [&rotations,dE_turn,&rot]( const Int r, const Int k, const Int de )
        {
            (void)r;
            (void)k;
            rotations.Push(rot);
            rot += dE_turn[de];
        },
        []( const Int r ){ (void)r; },
        false
    );
    
    return rotations;
}

Aggregator<Turn_T,Int> RegionRotations() const
{
    TOOLS_PTIMER(timer,MethodName("RegionRotations()"));
    
    Turn_T rot = 0;
    Aggregator<Turn_T,Int> rotations ( Int(2) * E_V.Dim(0) );
    
    cptr<Turn_T> dE_turn = E_turn.data();
    
    TraverseAllRegions(
        [&rot]( const Int r )
        {
            (void)r;
            rot = Turn_T(0);
        },
        [dE_turn,&rot]( const Int r, const Int k, const Int de )
        {
            (void)r;
            (void)k;
            rot += dE_turn[de];
        },
        [&rotations,&rot]( const Int r )
        {
            (void)r;
            rotations.Push(rot);
        },
        false
    );
    
    return rotations;
}



bool CheckAllRegionTurns() const
{
    bool okayQ = true;
    Turn_T rot = 0;
    cptr<Turn_T> dE_turn = E_turn.data();
    
    Aggregator<Int,Int> region ( max_face_size );
    
    bool exteriorQ = false;
    
    TraverseAllRegions(
        [&region,&rot]( const Int r )
        {
            (void)r;
            region.Clear();
            rot = Turn_T(0);
        },
        [&region,dE_turn,&rot,&exteriorQ,this]( const Int r, const Int k, const Int de )
        {
            (void)r;
            (void)k;
            region.Push(de);
            rot += dE_turn[de];
            exteriorQ = DedgeExteriorQ(de);
        },
        [&region,&rot,&okayQ,&exteriorQ]( const Int r )
        {
            if( exteriorQ )
            {
                bool this_okayQ = (rot == Turn_T(-4));
                
                if( !this_okayQ )
                {
                    eprint(ClassName() + "::CheckAllRegionTurns: region r = " + ToString(r) + " is exterior region with incorrect rotation number.");
                    TOOLS_DDUMP(rot);
                    TOOLS_DDUMP(region);
                }
                
                okayQ = okayQ && this_okayQ;
            }
            else
            {
                bool this_okayQ = ( rot == Turn_T( 4) );
                
                if( !this_okayQ )
                {
                    eprint(ClassName() + "::CheckAllRegionTurns: region r = " + ToString(r) + " is interior region with incorrect rotation number.");
                    TOOLS_DDUMP(rot);
                    TOOLS_DDUMP(region);
                }
                okayQ = okayQ && this_okayQ;
            }
        },
        false
    );
    
    return okayQ;
}
