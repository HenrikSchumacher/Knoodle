public:



/*! @brief This just splits off connected summands and appends them to the supplied `PD_list`.
 *
 * Caution: At the moment it does not track from which connected component it was split off. Hence this is useful only for knots, not for multi-component links.
 */

bool DisconnectSummands(
    mref<std::vector<PlanarDiagram<Int>>> PD_list,
    const Int max_dist = std::numeric_limits<Int>::max(),
    const bool compressQ = true,
    const Int  simplify3_level = 4,
    const Int  simplify3_max_iter = std::numeric_limits<Int>::max(),
    const bool strand_R_II_Q = true
)
{
    ptic(ClassName()+"::DisconnectSummands");
    
    // TODO: Introduce some tracking of from where the components are split off.
    
    TwoArraySort<Int,Int,Int> sort ( initial_arc_count );
    
    std::vector<Int> f_arcs;
    
    std::vector<Int> f_faces;

    cptr<Int> F_A_ptr  = FaceDirectedArcPointers().data();
    cptr<Int> F_A_idx  = FaceDirectedArcIndices().data();
    cptr<Int> A_face   = ArcFaces().data();
    
    bool changedQ = false;
    
    bool changed_at_least_onceQ = false;

    do
    {
        changedQ = false;
        
        for( Int f = 0; f < FaceCount(); ++f )
        {
            changedQ = changedQ || DisconnectSummand(
                f,PD_list,sort,f_arcs,f_faces,F_A_ptr,F_A_idx,A_face,
                max_dist,compressQ,simplify3_level,simplify3_max_iter,strand_R_II_Q
            );
        }
        
        changed_at_least_onceQ = changed_at_least_onceQ || changedQ;
    }
    while( changedQ );
    
    if( changed_at_least_onceQ )
    {
        this->ClearCache();
    }
    
    ptoc(ClassName()+"::DisconnectSummands");
    
    return changed_at_least_onceQ;
}


private:

/*! @brief Checks whether face `f` has a double face neighbor. If yes, it may split off connected summand(s) and pushes them to the supplied `std::vector` `PD_list`. It may however decide to perform some Reidemeister moves to remove crossings in the case that this would lead to an unknot. Returns `true` if any change has been made.
 *
 * If a nontrivial connect-sum decomposition is found, this routine splits off the smaller component, pushes it to `PD_list`, and then tries to simplify it further with `Simplify5` (which may push further connected summands to `PD_list`).
 *
 * Caution: At the moment it does not track from which connected component it was split off. Hence this is useful only for knots, not for multi-component links.
 */

bool DisconnectSummand(
    const Int f,
    mref<std::vector<PlanarDiagram<Int>>> PD_list,
    TwoArraySort<Int,Int,Int> & sort,
    std::vector<Int> & f_arcs,
    std::vector<Int> & f_faces,
    cptr<Int> F_A_ptr,
    cptr<Int> F_A_idx,
    cptr<Int> A_face,
    const Int max_dist,
    const bool compressQ,
    const Int  simplify3_level,
    const Int  simplify3_max_iter,
    const bool strand_R_II_Q = true
)
{
    // TODO: Introduce some tracking of from where the components are split off.
    
    const Int i_begin = F_A_ptr[f  ];
    const Int i_end   = F_A_ptr[f+1];

    f_arcs.clear();
    f_faces.clear();
    
    for( Int i = i_begin; i < i_end; ++i )
    {
        const Int A = F_A_idx[i];
        
        const Int a = (A >> 1);
        
        PD_ASSERT( f == A_face[A] );
        
        if( ArcActiveQ(a) )
        {
            f_arcs.push_back(A);
            
            // Tell `f` that it is neighbor of the face that belongs to the twin of `A`.
            f_faces.push_back( A_face[A ^ Int(1)] );
        }
    }
        
    const Int f_size = static_cast<Int>( f_arcs.size() );
    
    if( f_size == 1 )
    {
        // TODO: What to do here? This need not be a Reidemeister I move, since we ignore arcs on the current strand, right? So what is this?

        const Int a = (f_arcs[0] >> 1);
        
        if( A_cross(a,Tail) == A_cross(a,Head) )
        {
            Reidemeister_I(a);
            return true;
        }
    }
    
    sort( &f_faces[0], &f_arcs[0], f_size );
    
    auto push = [&]( PlanarDiagram<Int> && pd )
    {
        pd.Simplify5(
            PD_list,
            max_dist,
            compressQ,
            simplify3_level,
            simplify3_max_iter,
            strand_R_II_Q
        );
        
        PD_list.push_back( std::move(pd) );
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
                
                //   #  #  # #  #  #
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
                //
                
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
                    push( std::move( ExportSmallerComponent(a_prev,a_next) ) );
                    return true;
                }
            }
            else
            {
                //  #################
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
                
                const Int c = A_cross(a,Tail);
                
                const Int a_prev = NextArc<Tail>(a,c);
                const Int b_next = NextArc<Head>(b,c);
                
                Reconnect<Head>(a_prev,b_next);
                Reconnect<Tail>(a,b);
                DeactivateCrossing(c);

                push( std::move( ExportSmallerComponent(a_prev,a) ) );
                
                return true;
            }
        }
        else if( A_cross(b,Tail) == A_cross(a,Head) )
        {
            //  #################
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
            
            const Int c = A_cross(a,Head);

            const Int a_next = NextArc<Head>(a,c);
            const Int b_prev = NextArc<Tail>(b,c);
            
            Reconnect<Head>(a,b);
            Reconnect<Tail>(a_next,b_prev);
            DeactivateCrossing(c);
            
            push( std::move( ExportSmallerComponent(a,a_next) ) );
            
            // push( std::move( SplitSmallerDiagramComponent(a,a_next) ) );
                            
            return true;
        }
        
        // `a` and `b` do not have any crossing in common.
        //
        //    #####################
        //       ^             |
        //       |             |
        //     a |             | b
        //       |             |
        //       |             v
        //    #####################
        //

        const Int c_a = A_cross(a,Head);
        const Int c_b = A_cross(b,Head);
        
        A_cross(b,Head) = c_a;
        A_cross(a,Head) = c_b;
        
        SetMatchingPortTo<In>(c_a,a,b);
        SetMatchingPortTo<In>(c_b,b,a);
        
        push( std::move( ExportSmallerComponent(a,b) ) );
        
//        push( std::move( SplitSmallerDiagramComponent(a,b) ) );
        
        return true;
    }

    return false;
}





private:

/*! @brief Removes the smaller of the connected components of arc `a` and `b` and creates a new diagram from it.
 */

PlanarDiagram<Int> ExportSmallerComponent( const Int a_0, const Int b_0 )
{
    ptic(ClassName()+"::ExportSmallerComponent");
    
    Int length = 0;
    Int a = a_0;
    Int b = b_0;
    
    do
    {
        const Int c_a = A_cross(a,Head);
        const Int c_b = A_cross(b,Head);
        
        C_scratch[c_a] = -1;
        C_scratch[c_b] = -1;
        
        ++length;
        
        a = NextArc<Head>(a,c_a);
        b = NextArc<Head>(b,c_b);
    }
    while( (a != a_0) && (b != b_0) );
    
    PlanarDiagram<Int> pd = ExportComponent( (a == a_0) ? a_0 : b_0, length );
    
    ptoc(ClassName()+"::ExportSmallerComponent");
    
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

PlanarDiagram<Int> ExportComponent( const Int a_0, const Int comp_size )
{
    ptic(ClassName()+"::ExportComponent");
    
    PlanarDiagram<Int> pd (comp_size/2,0);
    
    Int a_label = 0;
    Int c_counter = 0;
    
    // Using C_scratch to keep track of the new labels of crossings.
    
    mptr<Int> C_color = C_scratch.data();
    
    Int a = a_0;
    
    do
    {
        const Int t = A_cross(a,Tail);
        const Int h = A_cross(a,Head);
        
        Int t_label;
        Int h_label;
        
        if( C_color[t] < 0 )
        {
            C_color[t] = t_label = c_counter;
            pd.C_state[t_label] = C_state[t];
            ++c_counter;
        }
        else
        {
            t_label = C_color[t];
        }
        
        if( C_color[h] < 0 )
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
        
        pd.C_arcs(t_label,Out,t_side) = a_label;
        pd.C_arcs(h_label,In ,h_side) = a_label;
        
        pd.A_cross(a_label,Tail) = t_label;
        pd.A_cross(a_label,Head) = h_label;
        pd.A_state[a_label] = ArcState::Active;
        
        const Int a_next = C_arcs(h,Out,!h_side);
        
        DeactivateArc(a);
        DeactivateCrossing<false>(h);
        
        ++a_label;
        
        a = a_next;
    }
    while( a != a_0 );
    
    PD_ASSERT( pd.CheckAll() );
    
    ptoc(ClassName()+"::ExportComponent");
    
    return pd;
}

//Int ArcRangeLength( const Int a_begin, const Int a_end ) const
//{
//    if( a_end == a_begin )
//    {
//        return 0;
//    }
//
//    Int a = a_begin;
//    Int d = 0;
//    
//    do{
//        ++d;
//        Int a = NextArc<Head>(a);
//    }
//    while( (a != a_begin) and (a != a_end) );
//    
//    if( a == a_begin )
//    {
//        wprint(ClassName()+"::ArcRangeLength: " + ArcString(a_begin) + " and  " + ArcString(a_end) + " do not belong to the same connected component. Returning -1.");
//        
//        dump(d);
//        
//        return -1;
//    }
//    else
//    {
//        return d;
//    }
//}


private:


void PrintFace( const Int a_0, const bool headtail )
{
    Int a = a_0;
    
    bool dir = headtail;
    
    do
    {
        logprint( ArcString(a) );

        std::tie(a,dir) = NextLeftArc(a,dir);
    }
    while( a != a_0 );
}
