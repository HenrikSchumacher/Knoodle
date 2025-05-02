public:

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
        
#ifdef PD_DEBUG
        if( A_visited[a] )
        {
            eprint(ClassName()+"::CreateCompressed: A_visited[a] already! Something must be wrong with this diagram.");
            
            logprint( ArcString(a) );

            PrintInfo();
            
            CheckAll();
        }
#endif
        
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
                eprint(ClassName()+"::CreateCompressed: A_visited[a] already! Something must be wrong with this diagram.");
                
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

PlanarDiagram CreateCompressed2()
{
    TOOLS_PTIC( ClassName()+"::CreateCompressed");
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::CreateCompressed: Trying to compress invalid PlanarDiagram. Returning invalid diagram.");
        return PlanarDiagram();
    }
    
    PlanarDiagram pd ( crossing_count, unlink_count );
    
    // We assume that we start with a valid diagram.
    // So we do not have to compute `crossing_count` or `arc_count`.
    pd.crossing_count    = crossing_count;
    pd.arc_count         = arc_count;
    pd.provably_minimalQ = provably_minimalQ;
    
    const Int m = A_cross.Dimension(0);
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState>       C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState>            A_state_new = pd.A_state.data();
    
    Traverse(
         []( const Int lc, const Int lc_begin ){},
         [&C_arcs_new,&C_state_new,&A_cross_new,&A_state_new,this](
            const Int a,   const Int a_label,
            const Int c_0, const Int c_0_label, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_label, const bool c_1_visitedQ
         )
         {
             A_state_new[a_label] = ArcState::Active;
             
             if( !c_1_visitedQ )
             {
                 C_state_new[c_1_label] = this->C_state[c_1];
             }
             
             const bool side_0 = (this->C_arcs(c_0,Out,Right) == a);
             C_arcs_new(c_0_label,Out,side_0) = a_label;
             
             const bool side_1 = (this->C_arcs(c_1,In,Right) == a);
             C_arcs_new(c_1_label,In,side_1) = a_label;
             
             A_cross_new(a_label,Tail) = c_0_label;
             A_cross_new(a_label,Head) = c_1_label;
         },
         []( const Int lc, const Int lc_begin, const Int lc_end ){}
     );
    
    TOOLS_PTOC( ClassName()+"::CreateCompressed");
    
    return pd;
}

bool CanonicallyOrderedQ()
{
    // An empty list is ordered, of course.s
    if( !ValidQ() )
    {
        return true;
    }
    
    bool orderedQ = true;
    
    Traverse(
        []( const Int lc, const Int lc_begin ){},
        [&orderedQ](
            const Int a,   const Int a_label,
            const Int c_0, const Int c_0_label, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_label, const bool c_1_visitedQ
        )
        {
            orderedQ = orderedQ && (a == a_label) && (c_0 == c_0_label);
        },
        []( const Int lc, const Int lc_begin, const Int lc_end ){}
    );
    
    return orderedQ;
}
