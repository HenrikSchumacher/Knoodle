public:

// TODO: It lies in the nature of the way we detect connected summands that the local simplifications can only changes something around the seam. We should really do a local search that is sensitive to this.

/*! @brief This repeatedly applies `Simplify4` and attempts to split off connected summands with `SplitConnectedSummands`.
 */

bool SplitSimplify(
    std::vector<PlanarDiagram<Int>> & PD_list,
    const Int max_dist_checked = std::numeric_limits<Int>::max(),
    const bool compressQ = true,
    bool simplify3_exhaustiveQ = true
)
{
    ptic(ClassName()+"::SplitSimplify");

    bool globally_changed = false;
    bool changed = false;

    do
    {
        changed = Simplify4(max_dist_checked,compressQ,simplify3_exhaustiveQ);

        changed = changed || SplitConnectedSummands(PD_list);
        
        globally_changed = globally_changed || changed;
    }
    while( changed );
    
    if( compressQ && globally_changed )
    {
        // TODO: Can/should we skip this compression step?
        *this = this->CreateCompressed();
    }
    
    ptoc(ClassName()+"::SplitSimplify");

    return globally_changed;
}

public:



/*! @brief This just splits off connected summands and appends them to the supplied `PD_list`.
 *
 * Caution: At the moment it does not track from which connected component it was split off. Hence this is useful only for knots, not for multi-component links.
 */

bool SplitConnectedSummands(
    std::vector<PlanarDiagram<Int>> & PD_list,
    const Int max_dist_checked = std::numeric_limits<Int>::max(),
    const bool compressQ = true,
    bool simplify3_exhaustiveQ = true
)
{
    ptic(ClassName()+"::SplitConnectedSummands");
    // TODO: Introduce some tracking of from where the components are split off.
    
    RequireFaces();
    
    TwoArraySort<Int,Int,Int> S ( initial_arc_count );
    
    std::vector<Int> f_arcs;
    
    std::vector<Int> f_faces;
    
    bool changedQ = false;
    
    bool changed_at_least_onceQ = false;

    do
    {
        changedQ = false;
        
        for( Int f = 0; f < FaceCount(); ++f )
        {
            changedQ = changedQ || SplitConnectedSummand(f,PD_list,S,f_arcs,f_faces);
        }
        
        changed_at_least_onceQ = changed_at_least_onceQ || changedQ;
    }
    while( changedQ );
    
    ptoc(ClassName()+"::SplitConnectedSummands");
    
    return changed_at_least_onceQ;
}


private:

/*! @brief Checks whether face `f` has a double face neighbor. If yes, it may split off connected summand(s) and pushes them to the supplied `std::vector` `PD_list`. It may however decide to perform some Reidemeister moves to remove crossings in the case that this would lead to an unknot. Returns `true` if any change has been made.
 *
 * If a nontrivial connect-sum decomposition is found, this routine splits off the smaller component, pushes it to `PD_list`, and then tries to simplify it further with `SplitSimplify` (which may push further connected summands to `PD_list`).
 *
 * Caution: At the moment it does not track from which connected component it was split off. Hence this is useful only for knots, not for multi-component links.
 */

bool SplitConnectedSummand(
    const Int f, std::vector<PlanarDiagram<Int>> & PD_list,
    TwoArraySort<Int,Int,Int> & S,
    std::vector<Int> & f_arcs,
    std::vector<Int> & f_faces,
    const Int max_dist_checked = std::numeric_limits<Int>::max(),
    const bool compressQ = true,
    bool simplify3_exhaustiveQ = true
)
{
    // TODO: Introduce some tracking of from where the components are split off.
    
    const Int i_begin = F_arc_ptr[f  ];
    const Int i_end   = F_arc_ptr[f+1];

    f_arcs.clear();
    f_faces.clear();
    
    for( Int i = i_begin; i < i_end; ++i )
    {
        const Int a = F_arc_idx[i];
        
        if( ArcActiveQ(a) )
        {
            f_arcs.push_back(a);
            
            const Int g = A_faces(a,0);
            
            f_faces.push_back( (g == f) ? A_faces(a,1) : g );
        }
    }
        
    const Int f_size = static_cast<Int>( f_arcs.size() );
    
    if( f_size == 1 )
    {
        // TODO: What to do here? This need not be a Reidemeister I move, since we ignore arcs on the current strand, right? So what is this?

//        
//        wprint("f_size == 1");
//        
//        for( auto a : f_arcs )
//        {
//            logprint( ArcString(a) );
//        }
        
        
        Reidemeister_I(f_arcs[0]);
        
        return true;
    }
    
    S( &f_faces[0], &f_arcs[0], f_size );
    
    auto push = [&]( PlanarDiagram<Int> && pd )
    {
        pd.SplitSimplify( PD_list, max_dist_checked, compressQ, simplify3_exhaustiveQ );

        PD_ASSERT( pd.CheckAll() );

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
        
        const Int a = f_arcs[i  ];
        const Int b = f_arcs[i+1];
        
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
        else if( A_cross(a,Head) == A_cross(b,Tail) )
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

        const Int c_a = A_cross(a, Head);
        const Int c_b = A_cross(b, Head);
        
        A_cross(b,Head) = c_a;
        A_cross(a,Head) = c_b;
        
        SetMatchingPortTo<In>(c_a,a,b);
        SetMatchingPortTo<In>(c_b,b,a);
        
        push( std::move( ExportSmallerComponent(a,b) ) );
        
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
 *
 * Before this is called, it is essential that at least the positions in `C_scratch` that will be traversed are filled by negative numbers! For example, `ExportSmallerComponent` does this.
 *
 * @param a_0 The index of the arc
 *
 * @param arc_count The number of arcs to remove. It is needed for creating the new `PlanarDiagram` instance with a sufficient number of arcs.
 *
 */

PlanarDiagram<Int> ExportComponent( const Int  a_0, const Int arc_count )
{
    ptic(ClassName()+"::ExportComponent");
    
    PlanarDiagram<Int> pd (arc_count/2,0);
    
    Int a_label = 0;
    Int c_counter = 0;
    
    // Using C_scratch to keep track of the new labels of crossings.
    
    Int a = a_0;
    
    do
    {
        const Int t = A_cross(a,Tail);
        const Int h = A_cross(a,Head);
        
        Int t_label;
        Int h_label;
        
        if( C_scratch[t] < 0 )
        {
            C_scratch[t] = t_label = c_counter;
            pd.C_state[t_label] = C_state[t];
            ++c_counter;
        }
        else
        {
            t_label = C_scratch[t];
        }
        
        if( C_scratch[h] < 0 )
        {
            C_scratch[h] = h_label = c_counter;
            pd.C_state[h_label] = C_state[h];
            ++c_counter;
        }
        else
        {
            h_label = C_scratch[h];
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
