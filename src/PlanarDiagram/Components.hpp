public:

Int ComponentCount()
{
    RequireComponents();
    
    return comp_arc_ptr.Size()-1;
}

Int ComponentSize( const Int comp )
{
    RequireComponents();
    
    return (comp_arc_ptr[comp+1] - comp_arc_ptr[0]);
}

const Tensor1<Int,Int> & ComponentArcIndices()
{
    RequireComponents();
    
    return comp_arc_idx;
}

const Tensor1<Int,Int> & ComponentArcPointers()
{
    RequireComponents();
    
    return comp_arc_ptr;
}

const Tensor1<Int,Int> & ArcComponents()
{
    RequireComponents();
    
    return A_comp;
}

const Tensor1<Int,Int> & ArcComponentPositions()
{
    RequireComponents();
    
    return A_pos;
}

Int ArcDistance( const Int a_0, const Int a_1 )
{
    RequireComponents();
    
    const Int comp = A_comp[a_0];
    
    if( comp == A_comp[a_1] )
    {
        const Int d = Abs(A_pos[a_0] - A_pos[a_1]);
        
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
    if( comp_initialized )
    {
        return;
    }
    
    ptic(ClassName()+"::RequireComponents");
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    A_comp = Tensor1<Int,Int>( initial_arc_count, -1 );
    
    // Also, each arc should know its position within the component.
    A_pos  = Tensor1<Int,Int>( initial_arc_count, -1 );
    
    // Data for forming the graph components.
    // Each active arc will appear in precisely one component.
    comp_arc_idx     = Tensor1<Int,Int> ( arc_count );
    comp_arc_ptr     = Tensor1<Int,Int> ( arc_count );   // TODO: Refine this upper bound.
    comp_arc_ptr[0]  = 0;
    
    const Int m = A_cross.Dimension(0);
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
            
            a = NextArc(a);
            
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
    
    ptoc(ClassName()+"::RequireComponents");
    
    comp_initialized = true;
}
