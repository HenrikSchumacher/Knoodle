Tensor1<Int,Int> GaussCode()
{
    ptic(ClassName()+"::GaussCode");
    
    const Int m = A_cross.Dimension(0);
    
    Tensor1<Int,Int> gauss_code ( arc_count );
    
    // We are using C_scratch to keep track of the new crossing's labels.
    C_scratch.Fill(-1);
    
    // We are using A_scratch to keep track of the new crossing's labels.
    A_scratch.Fill(-1);
    
    mptr<Int> C_labels = C_scratch.data();
    mptr<Int> A_labels = A_scratch.data();
    
    Int c_counter = 0;
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

            const Int c = A_cross(a,Tail);

            AssertCrossing(c);
            
            if( C_labels[c] < 0 )
            {
                C_labels[c] = c_counter++;
            }

            gauss_code[a] = (c+1) * (ArcOverQ<Tail>(a) ? Int(1) : Int(-1));
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    ptoc(ClassName()+"::GaussCode");
    
    return gauss_code;
}

Tensor1<Int,Int> OrientedGaussCode()
{
    ptic(ClassName()+"::OrientedGaussCode");
    
    const Int m = A_cross.Dimension(0);
    
    Tensor1<Int,Int> gauss_code ( arc_count );
    
    // We are using C_scratch to keep track of the new crossing's labels.
    C_scratch.Fill(-1);
    
    // We are using A_scratch to keep track of the new crossing's labels.
    A_scratch.Fill(-1);
    
    mptr<Int> C_labels = C_scratch.data();
    mptr<Int> A_labels = A_scratch.data();
    
    Int c_counter = 0;
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

            const Int c = A_cross(a,Tail);

            AssertCrossing(c);
            
            if( C_labels[c] < 0 )
            {
                C_labels[c] = c_counter++;
            }
            
            const bool side = (C_arcs(c,Tail,Right) == a);

            const bool overQ = (side == CrossingRightHandedQ(c) );
            
            gauss_code[a] = ((c << 2) | (Int(side) << 1) | Int(overQ));
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    ptoc(ClassName()+"::OrientedGaussCode");
    
    return gauss_code;
}
