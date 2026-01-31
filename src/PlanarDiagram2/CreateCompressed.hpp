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
    // needs to know all member variables
    
    if( !ValidQ() ) { return InvalidDiagram(); }
    
    auto tag = [](){ return MethodName("CreateCompressed")+"<" + ToString(recolorQ) + ">"; };
    
    TOOLS_PTIMER(timer,tag());
    
    if( ProvenUnknotQ() )
    {
        if constexpr ( recolorQ )
        {
            return Unknot(Int(0));
        }
        else
        {
            return Unknot(last_color_deactivated);
        }
    }
    
//    constexpr bool debugQ = true;
    
    PD_T pd;
    
    if constexpr ( debugQ )
    {
        wprint(tag() + ": Debug mode active.");
        
        this->PrintInfo();
        
        if( this->CheckAll() ) { pd_eprint(tag() + ": this->CheckAll() failed."); }
        
        pd = PD_T( crossing_count );
    }
    else
    {
        // Skip filling buffers as they will be overwritten anyways.
        pd = PD_T( crossing_count, true );
    }
    
    // We assume that we start with a valid diagram.
    // So we do not have to compute `crossing_count` or `arc_count`.
    pd.crossing_count         = crossing_count;
    pd.arc_count              = arc_count;
    pd.proven_minimalQ        = proven_minimalQ;
    pd.last_color_deactivated = last_color_deactivated;
    
    ColorCounts_T color_arc_counts;
    
    mref<CrossingContainer_T> C_arcs_new  = pd.C_arcs;
    mptr<CrossingState_T>     C_state_new = pd.C_state.data();
    
    mref<ArcContainer_T>      A_cross_new = pd.A_cross;
    mptr<ArcState_T>          A_state_new = pd.A_state.data();
    mptr<Int>                 A_color_new = pd.A_color.data();
    
    this->template Traverse<true>(
        []( const Int lc, const Int lc_begin )
        {
            (void)lc;
            (void)lc_begin;
        },
        [&tag, &C_arcs_new, &C_state_new, &A_cross_new, &A_state_new, A_color_new, this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_1;
            (void)c_1_visitedQ;
            
            if constexpr ( debugQ )
            {
                if( !ValidIndexQ(a) )
                {
                    pd_eprint(tag()+": Index a = " + ToString(a) + " is invalid.");
                }
                if( !this->ArcActiveQ(a) )
                {
                    pd_eprint(tag()+": a = " + this->ArcString(a) + " is inactive.");
                }
                if( !ValidIndexQ(c_0) )
                {
                    pd_eprint(tag()+": Index c_0 = " + ToString(c_0) + " is invalid.");
                }
                if( !this->CrossingActiveQ(c_0) )
                {
                    pd_eprint(tag()+": c_0 = " + this->CrossingString(c_0) + " is inactive.");
                }
                if( !ValidIndexQ(c_1) )
                {
                    pd_eprint(tag()+": Index c_1 = " + ToString(c_1) + " is invalid.");
                }
                if( !this->CrossingActiveQ(c_1) )
                {
                    pd_eprint(tag()+": c_1 = " + this->CrossingString(c_1) + " is inactive.");
                }
                if( !ValidIndexQ(a_pos) )
                {
                    pd_eprint(tag()+": Index a_pos = " + ToString(a_pos) + " is invalid.");
                }
                if( !ValidIndexQ(c_0_pos) )
                {
                    pd_eprint(tag()+": Index c_0_pos = " + ToString(c_0_pos) + " is invalid.");
                }
                if( !ValidIndexQ(c_1_pos) )
                {
                    pd_eprint(tag()+": Index c_1_pos = " + ToString(c_1_pos) + " is invalid.");
                }
            }
            else
            {
                (void)tag;
            }

            if constexpr ( recolorQ )
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
            
            A_state_new[a_pos] = ArcState_T::Active;
            C_arcs_new(c_0_pos,Out,ArcSide(a,Tail,c_0)) = a_pos;
            C_arcs_new(c_1_pos,In ,ArcSide(a,Head,c_1)) = a_pos;
        },
        [&color_arc_counts]( const Int lc, const Int lc_begin, const Int lc_end )
        {
            if constexpr( recolorQ )
            {
                color_arc_counts[lc] = lc_end - lc_begin;
            }
            {
                (void)lc;
                (void)lc_begin;
                (void)lc_end;
                (void)color_arc_counts;
            }
        }
    );
    
    // `Traverse` computes `LinkComponentCount`.
    // That is definitely a useful quantity, even after relabeling.
    if( this->InCacheQ("LinkComponentCount") )
    {
        pd.template SetCache<false>("LinkComponentCount",LinkComponentCount());
    }
    if constexpr ( recolorQ )
    {
        pd.template SetCache<false>("ColorArcCounts",color_arc_counts);
    }
    
    if constexpr ( debugQ )
    {
        pd.PrintInfo();
        if( !pd.CheckAll() ) { pd_eprint(tag() + ": pd.CheckAll() failed."); }
    }
    
    return pd;
}

template<bool recolorQ = false>
void Compress()
{
    (*this) = this->template CreateCompressed<recolorQ>();
}

void ConditionalCompress()
{
//    //            +------ This avoids recompression of unknots.
//    //            V
//    if( arc_count < max_crossing_count )
//    {
//        Compress();
//    }
////    else
////    {
////        ClearCache();
////    }
    
    //                 +------ This avoids recompression of unknots.
    //                 V
    if( crossing_count < max_crossing_count )
    {
        Compress();
    }
//    else
//    {
//        ClearCache();
//    }
}

bool CompressedOrderQ()
{
    // An empty list is ordered, of course.
    if( !ValidQ() ) { return true; }
    
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
