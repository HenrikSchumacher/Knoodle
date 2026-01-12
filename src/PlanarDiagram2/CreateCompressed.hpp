public:

/*!
 * @brief Creates a copy of the planar diagram with all inactive crossings and arcs removed.
 *
 * Relabeling is done in order of traversal by routine `Traverse`.
 *
 * @tparam recolorQ Whether the arc colors shall be recomputed.
 */

template<bool recolorQ = false>
PD_T CreateCompressed()
{
    if( !ValidQ() )
    {
        wprint(MethodName("CreateCompressed")+": Input diagram is invalid. Returning invalid diagram.");
        return InvalidDiagram();
    }
    
    TOOLS_PTIMER(timer,MethodName("CreateCompressed")+"<" + ToString(recolorQ) + ">");
    
    PD_T pd ( crossing_count );
    
    // We assume that we start with a valid diagram.
    // So we do not have to compute `crossing_count` or `arc_count`.
    pd.crossing_count  = crossing_count;
    pd.arc_count       = arc_count;
    pd.proven_minimalQ = proven_minimalQ;
    
    if constexpr ( !recolorQ )
    {
        pd.color_palette = color_palette;
    }
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState_T>     C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState_T>          A_state_new = pd.A_state.data();
    mptr<Int>                 A_color_new = pd.A_color.data();
    
    this->template Traverse<true>(
        [&pd]( const Int lc, const Int lc_begin )
        {
            (void)lc_begin;
            if constexpr( recolorQ )
            {
                pd.color_palette.insert(lc);
            }
            else
            {
                (void)pd;
                (void)lc;
            }
        },
        [&C_arcs_new,&C_state_new,&A_cross_new,&A_state_new,A_color_new,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_1;
            (void)c_1_visitedQ;

            if constexpr( recolorQ )
            {
                A_color_new[a_pos] = lc;
            }
            else
            {
                A_color_new[a_pos] = this->A_color[a];
            }
            
            if( !c_0_visitedQ )
            {
                C_state_new[c_0_pos] = this->C_state[c_0];
            }
            
            A_cross_new(a_pos,Tail) = c_0_pos;
            A_cross_new(a_pos,Head) = c_1_pos;
            
//            const ArcState_T a_state = this->A_state[a];
//            A_state_new[a_pos] = a_state;
//            C_arcs_new(c_0_pos,Out,a_state.Side(Tail)) = a_pos;
//            C_arcs_new(c_1_pos,In ,a_state.Side(Head)) = a_pos;
            
            A_state_new[a_pos] = ArcState_T::Active;
            C_arcs_new(c_0_pos,Out,ArcSide(a,Tail,c_0)) = a_pos;
            C_arcs_new(c_1_pos,In ,ArcSide(a,Head,c_1)) = a_pos;

        },
        []( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            (void)lc_begin;
            (void)lc_end;
        }
    );
    
    // `Traverse` computes `LinkComponentCount`.
    // That is definitely a useful quantity, even after relabeling.
    if( this->InCacheQ("LinkComponentCount") )
    {
        pd.template SetCache<false>("LinkComponentCount",LinkComponentCount());
    }
    
    return pd;
}

template<bool recolorQ = false>
void Compress()
{
    (*this) = this->template CreateCompressed<recolorQ>();
}

bool CompressedOrderQ()
{
    // An empty list is ordered, of course.
    if( !ValidQ() )
    {
        return true;
    }
    
    bool orderedQ = true;
    
    this->template Traverse<true>(
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
