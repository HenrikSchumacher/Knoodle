public:

/*!
 * @brief Returns an array that tells every arc to which over-strand it belongs.
 *
 *  More precisely, arc `a` belongs to over-strand number `ArcOverStrands()[a]`.
 *
 *  (An over-strand is a maximal consecutive sequence of arcs that pass over.)
 */

Tensor1<Int,Int> ArcOverStrands() const
{
    return ArcStrands<true>();
}

/*!
 * @brief Returns an array that tells every arc to which under-strand it belongs.
 *
 *  More precisely, arc `a` belongs to under-strand number `ArcUnderStrands()[a]`.
 *
 *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
 */

Tensor1<Int,Int> ArcUnderStrands() const
{
    return ArcStrands<false>();
}

private:

template<bool overQ>
Tensor1<Int,Int> ArcStrands() const
{
    ptic(ClassName()+"::" + (overQ ? "Over" : "Under")  + "StrandIndices");
    
    const Int m = A_cross.Dimension(0);
    
    Tensor1<Int ,Int> A_colors    ( m, -1 );
    Tensor1<char,Int> A_visited ( m, false );
    
    Int color = 0;
    Int a_ptr = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( A_visited[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        Int a = a_ptr;
        // Find the beginning of first strand.
        
        // For this, we move backwards along arcs until ArcUnderQ<Tail>(a)/ArcOverQ<Tail>(a)
        while( ArcUnderQ<Tail>(a) != overQ )
        {
            a = NextArc<Tail>(a);
        }
        
        const Int a_0 = a;
        
        // Traverse forward through all arcs in the link component, until we return where we started.
        do
        {
            A_visited[a] = true;
            
            A_colors[a] = color;

            // Whenever arc `a` goes under/over crossing A_cross(a,Head), we have to initialize a new strand.
            
            color += (ArcUnderQ<Head>(a) == overQ);
            
            a = NextArc<Head>(a);
        }
        while( a != a_0 );
        
        ++a_ptr;
    }
    
    ptoc(ClassName()+"::" + (overQ ? "Over" : "Under")  + "StrandIndices");
    
    return A_colors;
}

public:


/*!
 * @brief Returns an array of that tells every crossing which over-strand end or start in is.
 *
 *  More precisely, crossing `c` has the outgoing over-strands `CrossingOverStrands()(c,0,0)` and `CrossingOverStrands()(c,0,1)`
 *  and the incoming over-strands `CrossingOverStrands()(c,1,0)` and `CrossingOverStrands()(c,1,1)`.
 *
 *  (An over-strand is a maximal consecutive sequence of arcs that pass over.)
 */


Tensor3<Int,Int> CrossingOverStrands() const
{
    return CrossingStrands<true>();
}

/*!
 * @brief Returns an array of that tells every crossing which under-strand end or start in is.
 *
 *  More precisely, crossing `c` has the outgoing under-strands `CrossingUnderStrands()(c,0,0)` and `CrossingUnderStrands()(c,0,1)`
 *  and the incoming under-strands `CrossingUnderStrands()(c,1,0)` and `CrossingUnderStrands()(c,1,1)`.
 *
 *  (An under-strand is a maximal consecutive sequence of arcs that pass under.)
 */

Tensor3<Int,Int> CrossingUnderStrands() const
{
    return CrossingStrands<false>();
}

template<bool overQ>
Tensor3<Int,Int> CrossingStrands() const
{
    ptic(ClassName()+"::Crossing" + (overQ ? "Over" : "Under") + "Strands");
    
    const Int n = C_arcs.Dimension(0);
    const Int m = A_cross.Dimension(0);
    
    Tensor3<Int ,Int> C_strands  ( n, 2, 2, -1 );
    Tensor1<char,Int> A_visited ( m, false );
    
    Int counter = 0;
    Int a_ptr   = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( A_visited[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        Int a = a_ptr;
        // Find the beginning of first overstrand.
        // For this, we go back along a until ArcUnderQ<Tail>(a)/ ArcOverQ<Tail>(a)
        while( ArcUnderQ<Tail>(a) != overQ )
        {
            a = NextArc<Tail>(a);
        }
        
        const Int a_0 = a;
        
        // Traverse forward trhough all arcs in the link component, until we return where we started.
        do
        {
            A_visited[a] = true;
            
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);

            C_strands(c_0,Out,(C_arcs(c_0,Out,Right) == a)) = counter;
            C_strands(c_1,In ,(C_arcs(c_1,In ,Right) == a)) = counter;
            
            counter += (ArcUnderQ<Head>(a) == overQ);
            
            a = NextArc<Head>(a);
        }
        while( a != a_0 );
        
        ++a_ptr;
    }
    
    ptoc(ClassName()+"::Crossing" + (overQ ? "Over" : "Under") + "Strands");
    
    return C_strands;
}
