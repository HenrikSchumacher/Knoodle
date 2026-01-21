public:

Int LinkComponentCount() const
{
    const std::string tag = "LinkComponentCount";
    TOOLS_PTIMER(timer,tag);
    if(!this->InCacheQ(tag)){ ComputeLinkComponents(); }
    return this->template GetCache<Int>(tag);
}

Int LinkComponentSize( const Int lc ) const
{
    return LinkComponentArcs().SublistSize(lc);
}

cref<RaggedList<Int,Int>> LinkComponentArcs() const
{
    const std::string tag = "LinkComponentArcs";
    if(!this->InCacheQ(tag)){ ComputeLinkComponents(); }
    return this->template GetCache<RaggedList<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> ArcLinkComponents() const
{
    const std::string tag = "ArcLinkComponents";
    if(!this->InCacheQ(tag)){ ComputeLinkComponents(); }
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<Tensor1<Int,Int>> ArcPositions() const
{
    const std::string tag = "ArcPositions";
    if(!this->InCacheQ(tag)){ ComputeLinkComponents(); }
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

cref<ArcContainer_T> ArcTraversalFlags() const
{
    const std::string tag = "ArcTraversalFlags";
    if(!this->InCacheQ(tag)){ ComputeLinkComponents(); }
    return this->template GetCache<ArcContainer_T>(tag);
}


// TODO: Test this.
Int ArcDistance( const Int a_0, const Int a_1 ) const
{
    cptr<Int> A_lc  = ArcLinkComponents().data();
    cptr<Int> A_pos = ArcPositions().data();
    
    const Int lc = A_lc[a_0];
    
    if( lc == A_lc[a_1] )
    {
        const Int d = Abs(A_pos[a_0] - A_pos[a_1]);
        
        // TODO: Got an error at this point. How is this possible?
        PD_ASSERT( LinkComponentSize(lc) >= d );
        
        return Min( d, LinkComponentSize(lc) - d);
    }
    else
    {
        return Uninitialized;
    }
}

void RequireLinkComponents() const
{
    if(
        this->InCacheQ("LinkComponentCount")
        &&
        this->InCacheQ("LinkComponentArcs")
        &&
        this->InCacheQ("ArcLinkComponents")
        &&
        this->InCacheQ("ArcPositions")
        &&
        this->InCacheQ("ArcTraversalFlags")
    )
    {
        return;
    }
    else
    {
        ComputeLinkComponents();
    }
}

private:

void ComputeLinkComponents() const
{
    TOOLS_PTIMER(timer,MethodName("ComputeLinkComponents"));
    
    // Data for forming the graph components.
    // Each active arc will appear in precisely one component.
    RaggedList<Int,Int> lc_arcs ( CrossingCount() + Int(1), ArcCount() );
    // Records for each arc with crossings {c_0,c_1} the data
    //      {2 * c_0 + c_0_visitedQ, 2 * c_1 + c_1_visitedQ},
    // where c_0_visitedQ is true if c_0 is visited as tail for the second time
    // and   c_1_visitedQ is true if c_1 is visited as head for the second time.
    
    ArcContainer_T      A_flags ( ArcCount() );
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    Tensor1<Int,Int>    A_lc    ( max_arc_count, Uninitialized );
    
    // Also, each arc should know its position within the lc_arcs.
    Tensor1<Int,Int>    A_pos   ( max_arc_count, Uninitialized );
    
    this->template Traverse_ByNextArc<true,false,0>(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        [&A_lc,&A_pos,&lc_arcs,&A_flags](
            const Int a,   const Int a_label, const Int lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)c_0;
            (void)c_1;
            
            PD_ASSERT(a < A_lc.Dim(0));
            PD_ASSERT(a_label < A_flags.Dim(0));
            A_lc[a]  = lc;
            A_pos[a] = a_label;
            lc_arcs.Push(a);
            A_flags(a_label,Tail) = (Int(c_0_pos) << 1) | Int(c_0_visitedQ);
            A_flags(a_label,Head) = (Int(c_1_pos) << 1) | Int(c_1_visitedQ);
        },
        [&lc_arcs]( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
            lc_arcs.FinishSublist();
        }
     );
    
//     LinkComponentCount is set by `Traverse`.
    this->SetCache( "LinkComponentArcs", std::move(lc_arcs) );
    this->SetCache( "ArcLinkComponents", std::move(A_lc)    );
    this->SetCache( "ArcPositions",      std::move(A_pos)   );
    this->SetCache( "ArcTraversalFlags", std::move(A_flags) );
}




//template<typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
//void TraverseLinkComponentsWithCrossings(
//    LinkCompPre_T  && lc_pre,
//    ArcFun_T       && arc_fun,
//    LinkCompPost_T && lc_post
//) const
//{
//    if( !ValidQ() )
//    {
//        eprint(MethodName("TraverseLinkComponentsWithCrossings") + ": Trying to traverse an invalid planar diagram. Aborting.");
//        return;
//    }
//
//    Int a_counter = 0;
//
//    const auto & lc_arcs = LinkComponentArcs();
//    const Int lc_count   = lc_arcs.SublistCount();
//    cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
//    cptr<Int> lc_arc_idx = lc_arcs.Elements().data();
//    cptr<Int> A_flags    = ArcTraversalFlags().data();
//     
//    for( Int lc = 0; lc < lc_count; ++lc )
//    {
//        const Int k_begin = lc_arc_ptr[lc  ];
//        const Int k_end   = lc_arc_ptr[lc+1];
//
//        lc_pre( lc, lc_arc_idx[k_begin] );
//
//        for( Int k = k_begin; k < k_end; ++k )
//        {
//            const Int a_pos = a_counter++;
//            const Int a     = lc_arc_idx[k];
//            
//            const Int c_0 = A_cross(a,Tail);
//            const Int c_1 = A_cross(a,Head);
//
//            const Int flag_0 = A_flags[Int(2) * a + Int(0)];
//            const Int flag_1 = A_flags[Int(2) * a + Int(1)];
//            
//            arc_fun(
//                a, a_pos, lc,
//                c_0, (flag_0 >> 1), flag_0 & Int(1),
//                c_1, (flag_1 >> 1), flag_1 & Int(1)
//            );
//        }
//
//        lc_post( lc, lc_arc_idx[k_begin], lc_arc_idx[k_end] );
//    }
//}


// Caution: The following turned out to be buggy!

//public:
//
///*!
// * @brief See documentation of `Traverse`. The only difference is that `LinkComponents` are required for this. (This may make the traversal faster, if the `LinkComponents` are already in cache.
// */
//
//template<
//    bool crossingsQ, bool labelsQ,
//    typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T
//>
//void Traverse_ByLinkComponents(
//    LinkCompPre_T  && lc_pre,
//    ArcFun_T       && arc_fun,
//    LinkCompPost_T && lc_post
//)  const
//{
//    auto tag = []()
//    {
//        return MethodName("Traverse_ByLinkComponents")
//        + "<" + (crossingsQ ? "w/ crossings" : "w/o crossings")
//        + "," + (labelsQ ? "w/ labels" : "w/o labels")
//        + ">";
//    };
//    
//    TOOLS_PTIMER(timer,tag());
//    
//    if( InvalidQ() )
//    {
//        eprint(tag() + ": Trying to traverse an invalid planar diagram. Aborting.");
//        
//        // Other methods might assume that this is set.
//        // In particular, calls to `LinkComponentCount` might go into an infinite loop.
//        this->template SetCache<false>("LinkComponentCount",Int(0));
//        return;
//    }
//    
//    RequireLinkComponents();
//    
//    Int a_counter = 0;
//
//    const auto & lc_arcs = LinkComponentArcs();
//    const Int lc_count   = lc_arcs.SublistCount();
//    cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
//    cptr<Int> lc_arc_idx = lc_arcs.Elements().data();
//    cptr<Int> A_flag     = ArcTraversalFlags().data();
//     
//    for( Int lc = 0; lc < lc_count; ++lc )
//    {
//        const Int k_begin = lc_arc_ptr[lc  ];
//        const Int k_end   = lc_arc_ptr[lc+1];
//
//        lc_pre( lc, lc_arc_idx[k_begin] );
//
//        for( Int k = k_begin; k < k_end; ++k )
//        {
//            const Int a_pos = a_counter++;
//            const Int a     = lc_arc_idx[k];
//            
//            const Int c_0 = A_cross(a,Tail);
//            const Int c_1 = A_cross(a,Head);
//
//            const Int flag_0 = A_flag[Int(2) * a + Int(0)];
//            const Int flag_1 = A_flag[Int(2) * a + Int(1)];
//
//            if constexpr ( crossingsQ )
//            {
//                const Int c_0_pos = (flag_0 >> 1);
//                const Int c_1_pos = (flag_1 >> 1);
//
//                arc_fun(
//                    a, a_pos, lc,
//                    c_0, c_0_pos, flag_0 & Int(1),
//                    c_1, c_1_pos, flag_1 & Int(1)
//                );
//                
//                if constexpr ( labelsQ )
//                {
//                    C_scratch[c_0] = c_0_pos;
//                    A_scratch[a]   = a_pos;
//                }
//            }
//            else
//            {
//                arc_fun( a, a_pos, lc );
//                
//                if constexpr ( labelsQ )
//                {
//                    const Int c_0_pos = (flag_0 >> 1);
//                    
//                    C_scratch[c_0] = c_0_pos;
//                    A_scratch[a]   = a_pos;
//                }
//            }
//        }
//
//        lc_post( lc, lc_arc_idx[k_begin], lc_arc_idx[k_end] );
//    }
//}
//
//
///*!
// * @brief Short version of `Traverse_ByNextArc` with only a argument `arc_fun`.
// */
//
//template<bool crossingsQ, bool labelsQ, typename ArcFun_T>
//void Traverse_ByLinkComponents( ArcFun_T && arc_fun )  const
//{
//    this->template Traverse_ByLinkComponents<crossingsQ,labelsQ>(
//        []( const Int lc, const Int lc_begin )
//        {
//            (void)lc;
//            (void)lc_begin;
//        },
//        std::move(arc_fun),
//        []( const Int lc, const Int lc_begin, const Int lc_end )
//        {
//            (void)lc;
//            (void)lc_begin;
//            (void)lc_end;
//        }
//    );
//}
