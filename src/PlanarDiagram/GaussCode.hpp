template<typename T = Int>
Tensor1<T,Int> GaussCode()
{
    ptic(ClassName()+"::GaussCode<" + TypeName<T> + ">" );
    
    static_assert( SignedIntQ<T>, "" );
    
    if( (Size_T(crossing_count) + 1 ) > Size_T(std::numeric_limits<T>::max())   )
    {
        eprint(ClassName() + "::GaussCode: Requested type T cannot store Gauss code for this diagram.");
        
        ptoc(ClassName()+"::GaussCode<" + TypeName<T> + ">" );
        
        return Tensor1<T,Int>();
    }
        
    const Int m = A_cross.Dimension(0);
    
    Tensor1<T,Int> gauss_code ( arc_count );
    
    // We are using C_scratch to keep track of the new crossing's labels.
    C_scratch.Fill(-1);
    
    // We are using A_scratch to keep track of the new crossing's labels.
    A_scratch.Fill(-1);
    
    mptr<Int> C_labels = C_scratch.data();
    mptr<Int> A_labels = A_scratch.data();
    
    T   c_counter = 0;
    Int a_counter = 0;
    Int a_ptr     = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( (A_labels[a_ptr] >= 0)  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        Int a = a_ptr;
        
        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            A_labels[a] = a_counter;

            const T c = static_cast<T>(A_cross(a,Tail));

            AssertCrossing(c);
            
            if( C_labels[c] < 0 )
            {
                C_labels[c] = c_counter++;
            }

            gauss_code[a] = ( ArcOverQ<Tail>(a) ? (c+T(1)) : -(c+T(1)) );
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    ptoc(ClassName()+"::GaussCode<" + TypeName<T> + ">" );
    
    return gauss_code;
}

template<typename T = Int>
Tensor1<T,Int> OrientedGaussCode()
{
    ptic(ClassName()+"::OrientedGaussCode<" + TypeName<T> + ">" );
    
    static_assert( IntQ<T>, "" );
    
    if(
       (4 * Size_T( Ramp(crossing_count - 1) ) + 3 ) > Size_T(std::numeric_limits<T>::max())
    )
    {
        eprint(ClassName() + "::OrientedGaussCode: Requested type T cannot store oriented Gauss code for this diagram.");
        
        ptoc(ClassName()+"::OrientedGaussCode<" + TypeName<T> + ">" );
        
        return Tensor1<T,Int>();
    }
    
    const Int m = A_cross.Dimension(0);
    
    Tensor1<T,Int> gauss_code ( arc_count );
    
    // We are using C_scratch to keep track of the new crossing's labels.
    C_scratch.Fill(-1);
    
    // We are using A_scratch to keep track of the new crossing's labels.
    A_scratch.Fill(-1);
    
    mptr<Int> C_labels = C_scratch.data();
    mptr<Int> A_labels = A_scratch.data();
    
    T   c_counter = 0;
    Int a_counter = 0;
    Int a_ptr     = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( (A_labels[a_ptr] >= 0)  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        Int a = a_ptr;
        
        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            A_labels[a] = a_counter;

            const T c = static_cast<T>(A_cross(a,Tail));

            AssertCrossing(c);
            
            if( C_labels[c] < 0 )
            {
                C_labels[c] = c_counter++;
            }
            
            const bool side = (C_arcs(c,Tail,Right) == a);

            const bool overQ = (side == CrossingRightHandedQ(c) );
            
            gauss_code[a] = (c << 2) | (T(side) << 1) | T(overQ);
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    ptoc(ClassName()+"::OrientedGaussCode<" + TypeName<T> + ">" );
    
    return gauss_code;
}
