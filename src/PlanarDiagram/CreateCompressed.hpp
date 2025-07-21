public:

/*!
 * @brief Creates a copy of the planar diagram with all inactive crossings and arcs removed.
 *
 * Relabeling is done in order of traversal by routine `Traverse<true,false,0,DefaultTraversalMethod>`.
 */

PlanarDiagram CreateCompressed()
{
    if( !ValidQ() )
    {
        wprint( ClassName()+"::CreateCompressed: Input diagram is invalid. Returning invalid diagram.");
        return PlanarDiagram();
    }
    
    TOOLS_PTIMER(timer,ClassName()+"::CreateCompressed");
    
    PlanarDiagram pd ( crossing_count, unlink_count );
    
    // We assume that we start with a valid diagram.
    // So we do not have to compute `crossing_count` or `arc_count`.
    pd.crossing_count    = crossing_count;
    pd.arc_count         = arc_count;
    pd.proven_minimalQ = proven_minimalQ;
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState>       C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState>            A_state_new = pd.A_state.data();
    
    this->template Traverse<true,false,0,DefaultTraversalMethod>(
        [&C_arcs_new,&C_state_new,&A_cross_new,&A_state_new,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0_visitedQ;
            (void)c_1_visitedQ;

            // TODO: Handle over/under in ArcState.
            A_state_new[a_pos] = ArcState::Active;
//             A_state_new[a_pos] = A_state[a];

            if( !c_0_visitedQ )
            {
                C_state_new[c_0_pos] = this->C_state[c_0];
            }

            const bool side_0 = (this->C_arcs(c_0,Out,Right) == a);
            C_arcs_new(c_0_pos,Out,side_0) = a_pos;
            A_cross_new(a_pos,Tail) = c_0_pos;

            const bool side_1 = (this->C_arcs(c_1,In ,Right) == a);
            C_arcs_new(c_1_pos,In,side_1) = a_pos;
            A_cross_new(a_pos,Head) = c_1_pos;
        }
    );
    
    // `Traverse` computes `LinkComponentCount`. That is definitely a useful quantity, even after relabeling.
    if( this->InCacheQ("LinkComponentCount") )
    {
        pd.template SetCache<false>("LinkComponentCount",LinkComponentCount());
    }
    
    return pd;
}

void Compress()
{
    (*this) = this->CreateCompressed();
}

bool CompressedOrderQ()
{
    // An empty list is ordered, of course.
    if( !ValidQ() )
    {
        return true;
    }
    
    bool orderedQ = true;
    
    this->template Traverse<true,false,0,DefaultTraversalMethod>(
        [&orderedQ](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            orderedQ = orderedQ && (a == a_pos) && (c_0 == c_0_pos);
        }
    );
    
    return orderedQ;
}


PlanarDiagram Canonicalize_Legacy( bool under_crossing_flag = true )
{
    TOOLS_PTIC( ClassName()+"::Canonicalize_Legacy");
    
    PD_ASSERT(CheckAll());
    
    PlanarDiagram pd ( crossing_count, unlink_count );
    
    const Int m = A_cross.Dim(0);
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState>       C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState>            A_state_new = pd.A_state.data();
    
    C_scratch.Fill(Uninitialized);
    mptr<Int> C_pos = C_scratch.data();
    
    mptr<bool> A_visited = reinterpret_cast<bool *>(A_scratch.data());
    fill_buffer(A_visited,false,m);
    
    Int a_counter = 0;
    Int c_counter = 0;
    Int a_ptr     = 0;
    
    Int lc_counter = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
//        while( ( a_ptr < m ) && ( A_visited[a_ptr] || (!ArcActiveQ(a_ptr)) ) )
        while(
            ( a_ptr < m )
            &&
            (
                A_visited[a_ptr]
                ||
                (!ArcActiveQ(a_ptr))
                ||
                (under_crossing_flag ? ArcOverQ<Tail>(a_ptr) : false)
                // Always start with an undercrossing.
            )
        )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m ) { break; }
        
        Int a = a_ptr;
        
        AssertArc(a);
        
#ifdef PD_DEBUG
        if( A_visited[a] )
        {
            eprint(ClassName()+"::Canonicalize_Legacy: A_visited[a] already! Something must be wrong with this diagram.");
            
            logprint( ArcString(a) );

            PrintInfo();
            
            CheckAll();
        }
#endif
        
        {
            const Int c_1 = A_cross(a,Tail);
            
            AssertCrossing(c_1);
            
            if( !ValidIndexQ(C_pos[c_1]) )
            {
                C_pos[c_1] = c_counter;
                C_state_new[c_counter] = C_state[c_1];
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
                eprint(ClassName()+"::Canonicalize_Legacy: A_visited[a] already! Something must be wrong with this diagram.");
                
                logprint( ArcString(a) );

                PrintInfo();
                
                CheckAll();
            }
#endif
            // TODO: Handle over/under in ArcState.
            A_state_new[a_counter] = ArcState::Active;
//            A_state_new[a_counter] = A_state[a];
            
            A_visited[a] = true;
            
            if( !ValidIndexQ(C_pos[c_1]) )
            {
                C_pos[c_1] = c_counter;
                C_state_new[c_counter] = C_state[c_1];
                ++c_counter;
            }
            
            {
                const Int  c    = C_pos[c_0];
                const bool side = (C_arcs(c_0,Out,Right) == a);
                C_arcs_new(c,Out,side) = a_counter;
                A_cross_new(a_counter,Tail) = c;
            }
            
            {
                const Int  c    = C_pos[c_1];
                const bool side = (C_arcs(c_1,In,Right) == a);
                C_arcs_new(c,In,side) = a_counter;
                A_cross_new(a_counter,Head) = c;
            }
            
            a = NextArc<Head>(a);
            
            AssertArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++lc_counter;
        ++a_ptr;
        
    }
    
    pd.proven_minimalQ = proven_minimalQ;
    pd.template SetCache<false>("LinkComponentCounter",lc_counter);
    
    TOOLS_PTOC( ClassName()+"::Canonicalize_Legacy");
    
    return pd;
}
