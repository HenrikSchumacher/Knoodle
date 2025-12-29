public:



/*! @brief This just splits off connected summands and appends them to the supplied `pd_list`.
 *
 * Caution: At the moment it does not track from which connected component it was split off. Hence, this is useful only for knots, not for multi-component links.
 */

bool DisconnectSummands(
    mref<PD_List_T> pd_list,
    const Int  min_dist           = 6,
    const Int  max_dist           = std::numeric_limits<Int>::max(),
    const bool compressQ          = true,
    const Int  simplify3_level    = 4,
    const Int  simplify3_max_iter = std::numeric_limits<Int>::max(),
    const bool strand_R_II_Q      = true
)
{
    TOOLS_PTIMER(timer,MethodName("DisconnectSummands"));
    
    // TODO: Introduce some tracking of from where the components are split off.
    
    // TODO: Funelling the whole simplification pipeline through this is a bit awkward.
    
    TwoArraySort<Int,Int,Int> sort ( max_arc_count );
    
    const Int f_max_size = MaxFaceSize();
    
    Aggregator<Int,Int> f_arcs  (f_max_size);
    Aggregator<Int,Int> f_faces (f_max_size);
    
    const auto & F_dA  = FaceDarcs();
    cptr<Int> F_A_ptr  = F_dA.Pointers().data();
    cptr<Int> F_A_idx  = F_dA.Elements().data();
    cptr<Int> A_face   = ArcFaces().data();
    
    const Int face_count = F_dA.SublistCount();
    
    bool changedQ = false;
    
    bool changed_at_least_onceQ = false;

    do
    {
        changedQ = false;
        
        for( Int f = 0; f < face_count; ++f )
        {
            changedQ = changedQ || DisconnectSummand(
                f,pd_list,sort,f_arcs,f_faces,F_A_ptr,F_A_idx,A_face,
                min_dist,max_dist,
                compressQ,simplify3_level,simplify3_max_iter,strand_R_II_Q
            );
        }
        
        changed_at_least_onceQ = changed_at_least_onceQ || changedQ;
    }
    while( changedQ );
    
    if( changed_at_least_onceQ )
    {
        this->ClearCache();
    }
    
    return changed_at_least_onceQ;
}


private:

/*! @brief Checks whether face `f` has a double face neighbor. If yes, it may split off connected summand(s) and pushes them to the supplied `std::vector` `pd_list`. It may however decide to perform some Reidemeister moves to remove crossings in the case that this would lead to an unknot. Returns `true` if any change has been made.
 *
 * If a nontrivial connect-sum decomposition is found, this routine splits off the smaller component, pushes it to `pd_list`, and then tries to simplify it further with `Simplify5` (which may push further connected summands to `pd_list`).
 *
 * Caution: At the moment it does not track from which connected component it was split off. Hence, this is useful only for knots, not for multi-component links.
 */

bool DisconnectSummand(
    const Int f,
    mref<PD_List_T> pd_list,
    TwoArraySort<Int,Int,Int> & sort,
    mref<Aggregator<Int,Int>> f_arcs,
    mref<Aggregator<Int,Int>> & f_faces,
    cptr<Int> F_dA_ptr,
    cptr<Int> F_dA_idx,
    cptr<Int> dA_face,
    const Int min_dist,
    const Int max_dist,
    const bool compressQ,
    const Int  simplify3_level,
    const Int  simplify3_max_iter,
    const bool strand_R_II_Q = true
)
{
    // TODO: If the diagram is disconnected, then this might raise problems here. We need to prevent that!
    
    // TODO: Introduce some tracking so that we can tell where the components are split off.
    
    const Int i_begin = F_dA_ptr[f  ];
    const Int i_end   = F_dA_ptr[f+1];

    f_arcs.Clear();
    f_faces.Clear();
    
    for( Int i = i_begin; i < i_end; ++i )
    {
        const Int da = F_dA_idx[i];
        
        auto [a,d] = FromDarc(da);
        
        PD_ASSERT( f == dA_face[da] );
        
        if( ArcActiveQ(a) )
        {
            f_arcs.Push(da);
            
            // Tell `f` that it is neighbor of the face that belongs to the twin of `A`.
            f_faces.Push( dA_face[FlipDarc(da)] );
        }
    }
        
    const Int f_size = f_arcs.Size();
    
    if( f_size == Int(1) ) [[unlikely]]
    {
        const Int a = (f_arcs[0] >> 1);

        // Warning: This alters the diagram but preserves the Cache -- which is important to not invalidate `FaceDarcs()`, etc. I don't think that this will ever happen because `DisconnectSummand` is called only in a very controlled context when all possible Reidemeister I moves have been performed already.
        
        if( Private_Reidemeister_I<true,true>(a) )
        {
            wprint(ClassName()+"::DisconnectSummand: Found a face with just one arc around it. Tried to call Private_Reidemeister_I to remove. But maybe the face information is violated. Check your results thoroughly.");
            return true;
        }
        else
        {
            // TODO: Can we get here? What to do here? This need not be a Reidemeister I move, since we ignore arcs on the current strand, right? So what is this?

            eprint(ClassName()+"::DisconnectSummand: Face with one arc detected that is not a loop arc. Something must have gone very wrong here.");
            return false;
        }
    }
    
    sort( &f_faces[0], &f_arcs[0], f_size );
    
    auto conditional_push = [&]( PlanarDiagram && pd )
    {
        pd.Simplify5(
            pd_list,
            min_dist, max_dist,
            compressQ,
            simplify3_level,
            simplify3_max_iter,
            strand_R_II_Q
        );
        
        if( pd.NonTrivialQ() )
        {
            pd_list.push_back( std::move(pd) );
        }
    };
    
    Int i = 0;
    
    while( i+1 < f_size )
    {
        if( f_faces[i] != f_faces[i+1] )
        {
            ++i;
            continue;
        }
        
        const Int a = (f_arcs[i  ] >> 1);
        const Int b = (f_arcs[i+1] >> 1);
        
        AssertArc(a);
        AssertArc(b);
        
        if( A_cross(a,Tail) == A_cross(b,Head) )
        {
            if( A_cross(a,Head) == A_cross(b,Tail) )
            {
                // We split a bigon.
                
                /*   #  #  # #  #  #
                //   ## # ## ## # ##
                //    ##X##   ##X##
                //       \     ^
                // b_prev \   / a_next
                //         v /
                //          X c_1
                //         ^ \
                //        /   \
                //       /     \
                //    a +       +
                //      |       |
                // .....|.......|..... <<-- We cut here.
                //      |       |
                //      +       + b
                //       \     /
                //        \   /
                //         \ v
                //          X  c_0
                //         / ^
                // b_next /   \ a_prev
                //       v     \
                //      X       X
                */
                
                const Int c_0 = A_cross(a,Tail);
                const Int c_1 = A_cross(a,Head);
                
                PD_ASSERT( c_0 != c_1 );
                
                const Int a_prev = NextArc<Tail>(a,c_0);
                const Int b_next = NextArc<Head>(b,c_0);
                const Int a_next = NextArc<Head>(a,c_1);
                const Int b_prev = NextArc<Tail>(b,c_1);
                
                if( a_prev != b_next )
                {
                    Reconnect<Head>(a_prev,b_next);
                }
                
                if( b_prev != a_next )
                {
                    Reconnect<Tail>(a_next,b_prev);
                }
                
                DeactivateArc(a);
                DeactivateArc(b);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
            
                if( (a_prev == b_next) && (b_prev == a_next) )
                {
                    ++unlink_count;
                    return true;
                }
                else
                {
                    conditional_push(
                        std::move(ExportSmallerComponent(a_prev,a_next))
                    );
                    return true;
                }
            }
            else
            {
                /*  #################
                //  #               #
                //  #               #
                //  #################
                //     ^         /
                //      \       /
                //     a \     / b
                //        \   /
                //         \ v
                //          X c
                //         / ^
                // b_next /   \ a_prev
                //       /     \
                //      /       \
                //     v         \
                //
                // Beware of Reidemeister I move here!
                */
                
                const Int c = A_cross(a,Tail);
                
                const Int a_prev = NextArc<Tail>(a,c);
                const Int b_next = NextArc<Head>(b,c);
                
                Reconnect<Head>(a_prev,b_next);
                Reconnect<Tail>(a,b);
                DeactivateCrossing(c);

                conditional_push( std::move( ExportSmallerComponent(a_prev,a) ) );
                return true;
            }
        }
        else if( A_cross(b,Tail) == A_cross(a,Head) )
        {
            /*  #################
            //  #               #
            //  #               #
            //  #################
            //     \         ^
            //      \       /
            //     a \     / b
            //        \   /
            //         v /
            //          X c
            //         ^ \
            // p_prev /   \ a_next
            //       /     \
            //      /       \
            //     /         v
            //
            // Beware of Reidemeister I move here!
            */
            
            const Int c = A_cross(a,Head);

            const Int a_next = NextArc<Head>(a,c);
            const Int b_prev = NextArc<Tail>(b,c);
            
            Reconnect<Head>(a,b);
            Reconnect<Tail>(a_next,b_prev);
            DeactivateCrossing(c);
            
            conditional_push( std::move(ExportSmallerComponent(a,a_next)) );
            return true;
        }
        
        /* `a` and `b` do not have any crossing in common.
        //
        //    #####################
        //       ^             |
        //       |             |
        //     a |             | b
        //       |             |
        //       |             v
        //    #####################
        */

        const Int c_a = A_cross(a,Head);
        const Int c_b = A_cross(b,Head);
        
        A_cross(b,Head) = c_a;
        A_cross(a,Head) = c_b;
        
        SetMatchingPortTo<In>(c_a,a,b);
        SetMatchingPortTo<In>(c_b,b,a);
        
        conditional_push( std::move( ExportSmallerComponent(a,b) ) );
        return true;
    }

    return false;
}


private:

/*! @brief Removes the smaller of the connected components of arc `a` and `b` and creates a new diagram from it.
 */

PlanarDiagram ExportSmallerComponent( const Int a_0, const Int b_0 )
{
    TOOLS_PTIMER(timer,MethodName("ExportSmallerComponent"));
    
    Int length = 0;
    Int a = a_0;
    Int b = b_0;
    
    do
    {
        const Int c_a = A_cross(a,Head);
        const Int c_b = A_cross(b,Head);
        
        C_scratch[c_a] = Uninitialized;
        C_scratch[c_b] = Uninitialized;
        
        ++length;
        
        a = NextArc<Head>(a,c_a);
        b = NextArc<Head>(b,c_b);
    }
    while( (a != a_0) && (b != b_0) );
    
    PlanarDiagram pd = ExportComponent( (a == a_0) ? a_0 : b_0, length );
    
    return pd;
}

private:

/*! @brief Removes the connected component of arc `a_0`, creates a new diagram from it, and returns it.
 *
 * Before this is called, it is essential that at least the positions in `C_scratch` that will be traversed are filled by negative numbers! For example, `ExportSmallerComponent` does this.
 *
 * @param a_0 The index of the arc
 *
 * @param comp_size The number of arcs to remove. It is needed for creating the new `PlanarDiagram` instance with a sufficient number of arcs.
 *
 */

PlanarDiagram ExportComponent( const Int a_0, const Int comp_size )
{
    TOOLS_PTIMER(timer,MethodName("ExportComponent"));
    
    PlanarDiagram pd (comp_size/Int(2),Int(0));
    
    Int a_counter = 0;
    Int c_counter = 0;
    
    // Using C_scratch to keep track of the new labels of crossings.
    mptr<Int> C_color = C_scratch.data();
    
    Int a = a_0;
    
    print("ExportComponent");
    do
    {
        const Int t = A_cross(a,Tail);
        const Int h = A_cross(a,Head);
        
        Int t_label;
        Int h_label;
        
        if( !ValidIndexQ(C_color[t]) )
        {
            C_color[t] = t_label = c_counter;
            pd.C_state[t_label] = C_state[t];
            ++c_counter;
        }
        else
        {
            t_label = C_color[t];
        }
        
        if( !ValidIndexQ(C_color[h]) )
        {
            C_color[h] = h_label = c_counter;
            pd.C_state[h_label] = C_state[h];
            ++c_counter;
        }
        else
        {
            h_label = C_color[h];
        }
        
        const bool t_side = (C_arcs(t,Out,Right) == a);
        const bool h_side = (C_arcs(h,In ,Right) == a);
        
        pd.C_arcs(t_label,Out,t_side) = a_counter;
        pd.C_arcs(h_label,In ,h_side) = a_counter;
        
        pd.A_cross(a_counter,Tail) = t_label;
        pd.A_cross(a_counter,Head) = h_label;
        // TODO: Handle over/under in ArcState.
        pd.A_state[a_counter] = ArcState::Active;
//        pd.A_state[a_counter] = ??;
        
        const Int a_next = C_arcs(h,Out,!h_side);
        
        DeactivateArc(a);
        DeactivateCrossing<false>(h);
        
        ++a_counter;
        
        a = a_next;
    }
    while( a != a_0 );
    
    // This might fail if the diagram has more than one connected component.
    if( pd.max_crossing_count != c_counter )
    {
        wprint(MethodName("ExportComponent") + ": pd.max_crossing_count != c_counter.");
        TOOLS_LOGDUMP(pd.max_crossing_count);
        TOOLS_LOGDUMP(c_counter);
    }
    if( pd.max_arc_count != a_counter )
    {
        TOOLS_LOGDUMP(pd.max_arc_count);
        TOOLS_LOGDUMP(a_counter);
        wprint(MethodName("ExportComponent") + ": pd.max_arc_count != a_counter.");
    }
    
    pd.crossing_count = c_counter;
    pd.arc_count      = a_counter;
    
    // TODO: Should we recompress here?
    // TODO: Compute crossing_count and arc_count!
    
    PD_ASSERT( pd.CheckAll() );
    
    return pd;
}
