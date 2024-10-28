public:

Int LinkComponentCount()
{
    return LinkComponentArcPointers().Size()-1;
}

Int LinkComponentSize( const Int lc )
{
    const auto & lc_arc_ptr = LinkComponentArcPointers();
    
    return (lc_arc_ptr[lc+1] - lc_arc_ptr[lc]);
}

const Tensor1<Int,Int> & LinkComponentArcIndices()
{
    const std::string tag = "LinkComponentArcIndices";
    
    if( !this->InCacheQ(tag) )
    {
        RequireLinkComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & LinkComponentArcPointers()
{
    const std::string tag = "LinkComponentArcPointers";
    
    if( !this->InCacheQ(tag) )
    {
        RequireLinkComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ArcLinkComponents()
{
    const std::string tag = "ArcLinkComponents";
    
    if( !this->InCacheQ(tag) )
    {
        RequireLinkComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ArcLinkComponentPositions()
{
    const std::string tag = "ArcLinkComponentPositions";
    
    if( !this->InCacheQ(tag) )
    {
        RequireLinkComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

// TODO: Test this.
Int ArcDistance( const Int a_0, const Int a_1 )
{
    cptr<Int> A_lc = ArcLinkComponents().data();
    cptr<Int> A_pos  = ArcLinkComponentPositions().data();
    
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
        return -1;
    }
}

// TODO: Test this.
void RequireLinkComponents()
{
    ptic(ClassName()+"::RequireLinkComponents");
    
    const Int m = A_cross.Dimension(0);
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    Tensor1<Int,Int> A_lc ( m, -1 );
    
    // Also, each arc should know its position within the component.
    Tensor1<Int,Int> A_pos  ( m, -1 );
    
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
        while( ( a_ptr < m ) && ( (A_pos[a_ptr] >= 0)  || (!ArcActiveQ(a_ptr)) ) )
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
            A_pos[a] = length;
            
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
    this->SetCache( "ArcLinkComponentPositions", std::move(A_pos)        );
    
    ptoc(ClassName()+"::RequireLinkComponents");
}
