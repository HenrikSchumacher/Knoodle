public:

Int LinkComponentCount() const
{
    const std::string tag = "LinkComponentCount";
    
    if( !this->InCacheQ(tag) ){ RequireLinkComponents(); }
    
    return this->template GetCache<Int>(tag);
}

Int LinkComponentSize( const Int lc ) const
{
    const auto & lc_arc_ptr = LinkComponentArcPointers();
    
    return (lc_arc_ptr[lc+1] - lc_arc_ptr[lc]);
}

const Tensor1<Int,Int> & LinkComponentArcIndices() const
{
    const std::string tag = "LinkComponentArcIndices";
    
    if( !this->InCacheQ(tag) ){ RequireLinkComponents(); }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & LinkComponentArcPointers() const
{
    const std::string tag = "LinkComponentArcPointers";
    
    if( !this->InCacheQ(tag) ){ RequireLinkComponents(); }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ArcLinkComponents() const
{
    const std::string tag = "ArcLinkComponents";
    
    if( !this->InCacheQ(tag) ){ RequireLinkComponents(); }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ArcPositions() const
{
    const std::string tag = "ArcPositions";
    
    if( !this->InCacheQ(tag) ){ RequireLinkComponents(); }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor2<Int,Int> & ArcTraversalFlags() const
{
    const std::string tag = "ArcTraversalFlags";
    
    if( !this->InCacheQ(tag) ){ RequireLinkComponents(); }
    
    return this->template GetCache<Tensor2<Int,Int>>(tag);
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

// TODO: Test this.
void RequireLinkComponents() const
{
    TOOLS_PTIC(ClassName()+"::RequireLinkComponents");
    
    const Int m = A_cross.Dimension(0);
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    Tensor1<Int,Int> A_lc ( m, Uninitialized );
    
    // Also, each arc should know its position within the component.
    Tensor1<Int,Int> A_pos  ( m, Uninitialized );
    
    // Data for forming the graph components.
    // Each active arc will appear in precisely one component.
    Tensor1<Int,Int> lc_arc_idx ( ArcCount() );
    Tensor1<Int,Int> lc_arc_ptr ( CrossingCount() + Int(1) );
    Tensor2<Int,Int> A_flags    ( ArcCount(), Int(2) );
    
    
    lc_arc_ptr[0]  = 0;
    
    this->template Traverse<true,false,-1,DefaultTraversalMethod>(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        [&A_lc,&A_pos,&lc_arc_idx,&A_flags](
            const Int a, const Int a_label, const Int lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)c_0;
            (void)c_1;
            
            A_lc[a]  = lc;
            A_pos[a] = a_label;
            lc_arc_idx[a_label] = a;
            A_flags(a_label,Tail) = (Int(c_0_pos) << 1) | Int(c_0_visitedQ);
            A_flags(a_label,Head) = (Int(c_1_pos) << 1) | Int(c_1_visitedQ);
        },
        [&lc_arc_ptr]( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            lc_arc_ptr[lc+1] = lc_end;
        }
     );
    
    // LinkComponentCount is set by `Traverse`.
    lc_arc_ptr.template Resize<true>(LinkComponentCount()+1);
    
    this->SetCache( "LinkComponentArcIndices",   std::move(lc_arc_idx) );
    this->SetCache( "LinkComponentArcPointers",  std::move(lc_arc_ptr) );
    this->SetCache( "ArcLinkComponents",         std::move(A_lc)       );
    this->SetCache( "ArcPositions",              std::move(A_pos)      );
    this->SetCache( "ArcTraversalFlags",         std::move(A_flags)    );
//    this->SetCache( "LinkComponentCount",        lc_count              );
    
    TOOLS_PTOC(ClassName()+"::RequireLinkComponents");
}


void RequireLinkComponents_Legacy()
{
    TOOLS_PTIC(ClassName()+"::RequireLinkComponents_Legacy");
    
    const Int m = A_cross.Dimension(0);
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    Tensor1<Int,Int> A_lc ( m, Uninitialized );
    
    // Also, each arc should know its position within the link.
    Tensor1<Int,Int> A_pos ( m, Uninitialized );
    
    // Data for forming the graph components.
    // Each active arc will appear in precisely one component.
    Tensor1<Int,Int> lc_arc_idx ( ArcCount() );
    Tensor1<Int,Int> lc_arc_ptr ( CrossingCount() + 1 );
    lc_arc_ptr[0]  = 0;
    
    Int lc_counter = 0;
    Int a_counter = 0;
    Int length = 0;
    Int a_ptr = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( ValidIndexQ(A_pos[a_ptr])  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        Int a = a_ptr;
        
        length = 0;

        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            A_lc[a]  = lc_counter;
            A_pos[a] = a_counter;
            lc_arc_idx[a_counter] = a;
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
            ++length;
        }
        while( a != a_ptr );
        
        ++a_ptr;
        ++lc_counter;
        lc_arc_ptr[lc_counter] = a_counter;
    }
    
    lc_arc_ptr.template Resize<true>(lc_counter+1);
    
    this->SetCache( "LinkComponentArcIndices",   std::move(lc_arc_idx) );
    this->SetCache( "LinkComponentArcPointers",  std::move(lc_arc_ptr) );
    this->SetCache( "ArcLinkComponents",         std::move(A_lc)       );
    this->SetCache( "ArcPositions",              std::move(A_pos)      );
    
    // Could have been computed by a TraverseWith/WithoutCrossings.
    this->template SetCache<false>( "LinkComponentCount", lc_counter );
    
    TOOLS_PTOC(ClassName()+"::RequireLinkComponents_Legacy");
}


template<typename LinkCompPre_T, typename ArcFun_T, typename LinkCompPost_T>
void TraverseLinkComponentsWithCrossings(
    LinkCompPre_T  && lc_pre,
    ArcFun_T       && arc_fun,
    LinkCompPost_T && lc_post
) const
{
    if( !ValidQ() )
    {
        eprint(ClassName() + "TraverseLinkComponentsWithCrossings:: Trying to traverse an invalid PlanarDiagram. Aborting.");
        return;
    }

    Int a_counter = 0;

    const Int lc_count = LinkComponentCount();
    cptr<Int> lc_ptr   = LinkComponentArcPointers().data();
    cptr<Int> lc_idx   = LinkComponentArcIndices().data();
    cptr<Int> A_flags  = ArcTraversalFlags().data();
     
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        const Int k_begin = lc_ptr[lc  ];
        const Int k_end   = lc_ptr[lc+1];

        lc_pre( lc, lc_idx[k_begin] );

        for( Int k = k_begin; k < k_end; ++k )
        {
            const Int a_pos = a_counter++;
            const Int a     = lc_idx[k];
            
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);

            const Int flag_0 = A_flags[Int(2) * a + Int(0)];
            const Int flag_1 = A_flags[Int(2) * a + Int(1)];
            
            arc_fun(
                a, a_pos, lc,
                c_0, (flag_0 >> 1), flag_0 & Int(1),
                c_1, (flag_1 >> 1), flag_1 & Int(1)
            );
        }

        lc_post( lc, lc_idx[k_begin], lc_idx[k_end] );
    }
}
