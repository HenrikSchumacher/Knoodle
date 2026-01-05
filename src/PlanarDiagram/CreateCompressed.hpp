public:

/*!
 * @brief Creates a copy of the planar diagram with all inactive crossings and arcs removed.
 *
 * Relabeling is done in order of traversal by routine `Traverse<true,false,0>`.
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
    pd.crossing_count  = crossing_count;
    pd.arc_count       = arc_count;
    pd.proven_minimalQ = proven_minimalQ;
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState>       C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState>            A_state_new = pd.A_state.data();
    
    this->template Traverse<true,false,0>(
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
    
    this->template Traverse<true,false,0>(
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
