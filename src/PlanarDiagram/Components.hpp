public:

Int ComponentCount()
{
    return ComponentArcPointers().Size()-1;
}

Int ComponentSize( const Int comp )
{
    const auto & comp_arc_ptr = ComponentArcPointers();
    
    return (comp_arc_ptr[comp+1] - comp_arc_ptr[comp]);
}

const Tensor1<Int,Int> & ComponentArcIndices()
{
    const std::string tag = "ComponentArcIndices";
    
    if( !this->InCacheQ(tag) )
    {
        RequireComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ComponentArcPointers()
{
    const std::string tag = "ComponentArcPointers";
    
    if( !this->InCacheQ(tag) )
    {
        RequireComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ArcComponents()
{
    const std::string tag = "ArcComponents";
    
    if( !this->InCacheQ(tag) )
    {
        RequireComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

const Tensor1<Int,Int> & ArcComponentPositions()
{
    const std::string tag = "ArcComponentPositions";
    
    if( !this->InCacheQ(tag) )
    {
        RequireComponents();
    }
    
    return this->template GetCache<Tensor1<Int,Int>>(tag);
}

// TODO: Test this.
Int ArcDistance( const Int a_0, const Int a_1 )
{
    cptr<Int> A_comp = ArcComponents().data();
    cptr<Int> A_pos  = ArcComponentPositions().data();
    
    const Int comp = A_comp[a_0];
    
    if( comp == A_comp[a_1] )
    {
        const Int d = Abs(A_pos[a_0] - A_pos[a_1]);
        
        // TODO: Got an error at this point. How is this possible?
        PD_ASSERT( ComponentSize(comp) >= d );
        
        return Min( d, ComponentSize(comp) - d);
    }
    else
    {
        return -1;
    }
}

// TODO: Test this.
void RequireComponents()
{
    ptic(ClassName()+"::RequireComponents");
    
    const Int m = A_cross.Dimension(0);
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    Tensor1<Int,Int> A_comp ( m, -1 );
    
    // Also, each arc should know its position within the component.
    Tensor1<Int,Int> A_pos  ( m, -1 );
    
    // Data for forming the graph components.
    // Each active arc will appear in precisely one component.
    Tensor1<Int,Int> comp_arc_idx ( arc_count );
    Tensor1<Int,Int> comp_arc_ptr ( crossing_count + 1 );
    comp_arc_ptr[0]  = 0;
    
    Int comp_counter = 0;
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
            A_comp[a] = comp_counter;
            A_pos [a] = length;
            
            comp_arc_idx[a_counter] = a;
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
            ++length;
        }
        while( a != a_ptr );
        
        ++a_ptr;
        ++comp_counter;
        comp_arc_ptr[comp_counter] = a_counter;
    }
    
    comp_arc_ptr.template Resize<true>(comp_counter+1);
    
    this->SetCache( "ComponentArcIndices",   std::move(comp_arc_idx) );
    this->SetCache( "ComponentArcPointers",  std::move(comp_arc_ptr) );
    this->SetCache( "ArcComponents",         std::move(A_comp)       );
    this->SetCache( "ArcComponentPositions", std::move(A_pos)        );
    
    ptoc(ClassName()+"::RequireComponents");
}
