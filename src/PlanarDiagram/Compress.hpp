/*!
 * @brief Creates a copy of the planar diagram with all inactive crossings and arcs removed.
 *
 * Relabeling is done as follows:
 * First active arc becomes first arc in new planar diagram.
 * The _tail_ of this arc becomes the new first crossing.
 * Then we follow all arcs in the component with `NextArc<Head>(a)`.
 * The new labels increase by one for each visited arc.
 * Same for the crossings.
 */

PlanarDiagram CreateCompressed()
{
    TOOLS_PTIC( ClassName()+"::CreateCompressed");
    
    PD_ASSERT(CheckAll());
    
    PlanarDiagram pd ( crossing_count, unlink_count );
    
    const Int m = A_cross.Dimension(0);
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState>       C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState>            A_state_new = pd.A_state.data();
    
    C_scratch.Fill(-1);
    mptr<Int> C_labels = C_scratch.data();
    
    mptr<bool> A_visited = reinterpret_cast<bool *>(A_scratch.data());
    fill_buffer(A_visited,false,m);
    
    Int a_counter = 0;
    Int c_counter = 0;
    Int a_ptr     = 0;
    
    Int comp_counter = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( A_visited[a_ptr] || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        ++comp_counter;
        
        Int a = a_ptr;
        
        AssertArc(a);
        
        if( A_visited[a] )
        {
            eprint(ClassName()+"::CreateCompressed: A_visited[a]!!!!!");
        }
        
        {
            const Int c_0 = A_cross(a,Tail);
            
            AssertCrossing(c_0);
            
            if( C_labels[c_0] < Int(0) )
            {
                const Int c = C_labels[c_0] = c_counter;
                
                C_state_new[c] = C_state[c_0];
                
                ++c_counter;
            }
        }
        
        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            const Int c_0 = A_cross(a,Tail);
            const Int c_1 = A_cross(a,Head);
            
            AssertCrossing(c_0);
            AssertCrossing(c_1);

#ifdef PD_DEBUG
            if( A_visited[a] )
            {
                eprint(ClassName()+"::CreateCompressed: A_visited[a].");
                
                logprint( ArcString(a) );

                PrintInfo();
                
                CheckAll();

            }
#endif
            
            A_state_new[a_counter] = ArcState::Active;
            A_visited[a] = true;
            
            if( C_labels[c_1] < Int(0) )
            {
                const Int c = C_labels[c_1] = c_counter;
                
                C_state_new[c] = C_state[c_1];
                
                ++c_counter;
            }
            
            {
                const Int  c  = C_labels[c_0];
                const bool side = (C_arcs(c_0,Out,Right) == a);
                
                C_arcs_new(c,Out,side) = a_counter;
                
                A_cross_new(a_counter,Tail) = c;
            }
            
            {
                const Int  c  = C_labels[c_1];
                const bool side = (C_arcs(c_1,In,Right) == a);
                
                C_arcs_new(c,In,side) = a_counter;
                
                A_cross_new(a_counter,Head) = c;
            }
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    pd.provably_minimalQ = provably_minimalQ;
    
    TOOLS_PTOC( ClassName()+"::CreateCompressed");
    
    return pd;
}

bool ArcsCanonicallyOrderedQ() const
{
    bool orderedQ = true;

    Int a = 0;
    
    while( orderedQ && (a + Int(1) < arc_count) )
    {
        orderedQ = orderedQ && ArcActiveQ(a) && (NextArc<Head>(a) == a + Int(1));
    }
    
    if( arc_count > Int(0) )
    {
        a = arc_count - Int(1) ;
        
        orderedQ = orderedQ && ArcActiveQ(a) && (NextArc<Head>(a) == Int(0));
    }
    
    return orderedQ;
}
    
