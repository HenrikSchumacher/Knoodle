public:

// TODO: We could build the check of being alternating directly into the construction loop.

// Caution: Split is allowed to push minimal diagrams to pd_done.

Size_T Split( PD_T && pd, mref<PDC_T::PDList_T> pd_output, const bool proven_reducedQ = false )
{
    TOOLS_PTIMER(timer,MethodName("Split")+ "(pd,pd_output," + ToString(proven_reducedQ) + ")");
    
//    constexpr bool debugQ = true;
    
    if constexpr ( debugQ )
    {
        wprint(MethodName("Split")+": Debug mode active.");
    }
    
    if( pd.InvalidQ() ) { return Size_T(0); }
    
    if( pd.proven_minimalQ )
    {
        if constexpr ( debugQ )
        {
            if( !pd.CheckAll() ) { pd_eprint("pd.CheckAll() failed when pushed to pd_output."); };
        }
        
        pd_done.push_back( std::move(pd) );
        pd = PD_T::InvalidDiagram();
        return Size_T(1);
    }
    
    if( pd.crossing_count <= Int(1) )
    {
        CreateUnlink(pd.last_color_deactivated);
        pd = PD_T::InvalidDiagram();
        return Size_T(1);
    }
    
    if( proven_reducedQ )
    {
        PD_ASSERT(pd.ReducedQ());
    }
    
    auto push = [&pd_output,this,proven_reducedQ]( PD_T && pd_ )
    {
        if( proven_reducedQ && pd_.AlternatingQ() )
        {
            PD_ASSERT(pd_.CheckProvenMinimalQ());
            pd_.proven_minimalQ = true;
            pd_done.push_back( std::move(pd_) );
        }
        else
        {
            if( pd_.ValidQ() )
            {
                pd_output.push_back( std::move(pd_) );
            }
        }
    };
    
    // dc = diagram component
    // lc = link component
    
    const Int lc_count = pd.LinkComponentCount();
    
    if( lc_count == Int(1) )
    {
        push( std::move(pd) );
        pd = PD_T::InvalidDiagram();
        return Size_T(1);
    }

    const Int dc_count = pd.DiagramComponentCount();
    
    if( dc_count == Int(1) )
    {
        push( std::move(pd) );
        pd = PD_T::InvalidDiagram();
        return Size_T(1);
    }

    cref<typename PD_T::ComponentMatrix_T> A = pd.DiagramComponentLinkComponentMatrix();

    const auto & lc_arcs = pd.LinkComponentArcs();
    
    pd.C_scratch.Fill(Uninitialized);
    
#if defined(TENSORS_BOUND_CHECKS)
    // Use the containers to enable automatic bound checks.
    auto & dc_lc_ptr = A.Outer();
    auto & dc_lc_idx = A.Inner();
    
    auto & lc_arc_ptr = lc_arcs.Pointers();
    auto & lc_arc_idx = lc_arcs.Elements();
    
    auto & C_labels = pd.C_scratch;
#else
    // Use pointers to make this potentially faster. (Well, the restrict keyword would not work anyways, as we do not funnel this through a function call.)
    cptr<Int> dc_lc_ptr = A.Outer().data();
    cptr<Int> dc_lc_idx = A.Inner().data();
    
    cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
    cptr<Int> lc_arc_idx = lc_arcs.Elements().data();
    
    mptr<Int> C_labels = pd.C_scratch.data();
#endif
    
    for( Int dc = 0; dc < dc_count; ++dc  )
    {
        const Int i_begin = dc_lc_ptr[dc    ];
        const Int i_end   = dc_lc_ptr[dc + 1];
        
        Int dc_arc_count = 0;
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int lc = dc_lc_idx[i];
            
            const Int j_begin = lc_arc_ptr[lc    ];
            const Int j_end   = lc_arc_ptr[lc + 1];
            
            dc_arc_count += j_end - j_begin;
        }

        PD_T pd_new (dc_arc_count/Int(2),true);
        Int a_counter = 0;
        Int c_counter = 0;
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int lc = dc_lc_idx[i];
            
            const Int j_begin = lc_arc_ptr[lc    ];
            const Int j_end   = lc_arc_ptr[lc + 1];
            
            for( Int j = j_begin; j < j_end; ++j )
            {
                // Traversing link component `j`.
                const Int a = lc_arc_idx[j];

                if constexpr (debugQ)
                {
                    if( a_counter >= pd_new.max_arc_count )
                    {
                        TOOLS_LOGDUMP(a_counter);
                        TOOLS_LOGDUMP(pd_new.max_arc_count);
                        pd_eprint("a_counter >= pd_new.max_arc_count");
                    }
                }
                
                pd_new.A_state[a_counter] = pd.A_state[a];
                pd_new.A_color[a_counter] = pd.A_color[a];
                
                const Int  c_0    = pd.A_cross(a,Tail);
                const bool side_0 = pd.ArcSide(a,Tail);
                const Int  c_1    = pd.A_cross(a,Head);
                const bool side_1 = pd.ArcSide(a,Head);
                
                pd.DeactivateArc(a);
                
                Int c_0_label;
                Int c_1_label;
                
                // TODO: I think we can get rid of this check.
                if( !PD_T::ValidIndexQ(C_labels[c_0]) )
                {
                    c_0_label = C_labels[c_0] = c_counter;
                    pd_new.C_state[c_0_label] = pd.C_state[c_0];
                    ++c_counter;
                }
                else
                {
                    c_0_label = C_labels[c_0];
                    pd.template DeactivateCrossing<false>(c_0);
                }

                if constexpr (debugQ)
                {
                    if( c_0_label >= pd_new.max_crossing_count )
                    {
                        TOOLS_LOGDUMP(c_0_label);
                        TOOLS_LOGDUMP(pd_new.max_crossing_count);
                        pd_eprint("c_0_label >= pd_new.max_crossing_count");
                    }
                }
                
                pd_new.C_arcs(c_0_label,Out,side_0) = a_counter;
                pd_new.A_cross(a_counter,Tail) = c_0_label;
                
                if( !PD_T::ValidIndexQ(C_labels[c_1]) )
                {
                    c_1_label = C_labels[c_1] = c_counter;
                    pd_new.C_state[c_1_label] = pd.C_state[c_1];
                    ++c_counter;
                }
                else
                {
                    c_1_label = C_labels[c_1];
                    pd.template DeactivateCrossing<false>(c_1);
                }
                
                if constexpr (debugQ)
                {
                    if( c_1_label >= pd_new.max_crossing_count )
                    {
                        TOOLS_LOGDUMP(c_1_label);
                        TOOLS_LOGDUMP(pd_new.max_crossing_count);
                        pd_eprint("c_1_label >= pd_new.max_crossing_count");
                    }
                }
                pd_new.C_arcs(c_1_label,In,side_1) = a_counter;
                pd_new.A_cross(a_counter,Head) = c_1_label;
                ++a_counter;
            }
        }
        
        pd_new.crossing_count = c_counter;
        pd_new.arc_count      = a_counter;
            
        pd_new.SetCache("LinkComponentCount",i_end - i_begin);
        
        if constexpr (debugQ)
        {
            if( pd_new.crossing_count <= Int(0) )
            {
                TOOLS_LOGDUMP(pd_new.crossing_count);
                pd_eprint("pd_new.crossing_count <= Int(0)");
            }
            
            if( pd_new.crossing_count != pd_new.max_crossing_count )
            {
                TOOLS_LOGDUMP(pd_new.crossing_count);
                TOOLS_LOGDUMP(pd_new.max_crossing_count);
                pd_eprint("pd_new.crossing_count != pd_new.max_crossing_count");
            }
            
            if( pd_new.arc_count != pd_new.max_arc_count )
            {
                TOOLS_LOGDUMP(pd_new.arc_count);
                TOOLS_LOGDUMP(pd_new.max_arc_count);
                pd_eprint("pd_new.arc_count != pd_new.max_arc_count");
            }
            
            if( pd_new.arc_count != Int(2) * pd_new.crossing_count )
            {
                TOOLS_LOGDUMP(pd_new.arc_count);
                TOOLS_LOGDUMP(2 * pd_new.crossing_count);
                pd_eprint("pd_new.arc_count != Int(2) * pd_new.crossing_count");
            }
            
            if( !pd_new.CheckAll() )
            {
                pd_eprint("pd_new.CheckAll() failed.");
            }
        }
        
        PD_PRINT(MethodName("Split") + ": Split off a diagram with " + ToString(pd_new.crossing_count) + " crossings.");
        
        PD_ASSERT( pd_new.crossing_count > Int(0) );
        
        if( pd_new.ValidQ() )
        {
            if constexpr ( debugQ )
            {
                if( !pd_new.CheckAll() ) { pd_eprint("!pd_new.CheckAll() when pushed to pd_output."); };
            }
            
            push( std::move(pd_new) );
        }
        
    } // for( Int dc = 0; dc < dc_count; ++dc )
    
    pd = PD_T::InvalidDiagram(); // Communicate upstream that this is empty now.
    
    return ToSize_T(dc_count);
}

Size_T Split()
{
    TOOLS_PTIMER(timer,MethodName("Split"));
    PD_ASSERT(pd_done.empty());
    
    pd_done.clear();
    
    Size_T split_count = 0;
    
    for( PD_T & pd : pd_list )
    {
        split_count += Split( std::move(pd), pd_done );
    }
    
    using std::swap;
    
    swap(pd_list,pd_done);
    
    pd_done.clear();
    
    this->ClearCache();
    
    return split_count;
}
