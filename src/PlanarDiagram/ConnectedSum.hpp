public:

/*! @brief This repeatedly applies `Simplify4` and attempts to split off connected summands with `SplitConnectedSummands`.
 */

bool SplitSimplify( std::vector<PlanarDiagram<Int>> & PD_list )
{
    ptic(ClassName()+"::SplitSimplify");

    bool globally_changed = false;
    bool changed = false;

    do
    {
        changed = Simplify4();
        
        if( changed )
        {
            // TODO: Can/should we skip this compression step?
            (*this) = this->CreateCompressed();
        }

        changed = changed || SplitConnectedSummands(PD_list);
        
        globally_changed = globally_changed || changed;
    }
    while( changed );

    if( globally_changed )
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

bool SplitConnectedSummands( std::vector<PlanarDiagram<Int>> & PD_list )
{
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
    std::vector<Int> & f_faces
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
//        Reidemeister_I(f_arcs[0]);
        
        return false;
    }
    
    S( &f_faces[0], &f_arcs[0], f_size );
    
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
                
                AssertCrossing(c_0);
                AssertCrossing(c_1);
                PD_ASSERT( c_0 != c_1 );
                
                const Int a_prev = PrevArc(a);
                const Int a_next = NextArc(a);
                const Int b_prev = PrevArc(b);
                const Int b_next = NextArc(b);
                
                AssertArc(a_prev);
                AssertArc(a_next);
                AssertArc(b_prev);
                AssertArc(b_next);
                
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
            
                if( (a_prev == b_next) and (b_prev == a_next) )
                {
                    ++unlink_count;
                    
                    return true;
                }
                else
                {
                    ExportSmallerComponent(a_prev,a_next,PD_list);
                    
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
                
                AssertCrossing(c);
                
                const Int a_prev = PrevArc(a);
                const Int b_next = NextArc(b);
                
                AssertArc(a_prev);
                AssertArc(b_next);
                
                Reconnect<Head>(a_prev,b_next);
                Reconnect<Tail>(a,b);
                DeactivateCrossing(c);

                ExportSmallerComponent(a_prev,a,PD_list);
                
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
            
            AssertCrossing(c);
            
            const Int a_next = NextArc(a);
            const Int b_prev = PrevArc(b);
            
            AssertArc(a_next);
            AssertArc(b_prev);
            
            Reconnect<Head>(a,b);
            Reconnect<Tail>(a_next,b_prev);
            DeactivateCrossing(c);
            
            ExportSmallerComponent(a,a_next,PD_list);
            
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
        
        const bool side_a = (C_arcs(c_a,In,Right) == a);
        const bool side_b = (C_arcs(c_b,In,Right) == b);
        
        A_cross(a,Head) = c_b;
        A_cross(b,Head) = c_a;

        C_arcs(c_a,In,side_a) = b;
        C_arcs(c_b,In,side_b) = a;
        
        ExportSmallerComponent(a,b,PD_list);
        
        return true;
    }
    
    return false;
}


/*! @brief Compute the length of the connected component of arc `a_0` by fully traversing the component. This is hence a rather costly operation.
 */

Int ConnectedComponentLength( const Int a_0 ) const
{
    Int a = a_0;
    
    Int arc_count_ = 0;
    
    do{
        ++arc_count_;
        a = NextArc(a);
    }
    while( a != a_0 );
    
    return arc_count_;
}

/*! @brief Removes the smaller of the connected components of arc `a` and `b`, creates a new diagram from it, and pushes it onto `PD_list`.
 */

void ExportSmallerComponent(
    const Int a, const Int b, std::vector<PlanarDiagram<Int>> & PD_list
)
{
//    ptic(ClassName()+"::ExportSmallerComponent");
    
    Int a_length = ConnectedComponentLength(a);
    Int b_length = ConnectedComponentLength(b);
    
    if( a_length <= b_length )
    {
        ExportComponent( a, a_length, PD_list );
    }
    else
    {
        ExportComponent( b, b_length, PD_list );
    }
    
//    ptoc(ClassName()+"::ExportSmallerComponent");
}

/*! @brief Removes the connected component of arc `a_0`, creates a new diagram from it, and pushes it onto `PD_list`.
 *
 * @param a_0 The index of the arc
 *
 * @param arc_count The number of arcs to remove. It is needed for creating the new `PlanarDiagram` instance with a sufficient number of arcs.
 *
 * @param PD_list As list to which we append the newly created `PlanarDiagram
 *
 */

void ExportComponent(
    const Int a_0, const Int arc_count, std::vector<PlanarDiagram<Int>> & PD_list
)
{
//    ptic(ClassName()+"::ExportComponent");
    
    PlanarDiagram<Int> pd (arc_count/2,0);
    
    // TODO: Might be replaced by one global array (with some appropriate rolling labelling.)
    std::unordered_map<Int,Int> c_labels;
    
    Int a_label = 0;
    Int c_counter = 0;
    
    Int a = a_0;
    
    do
    {
        const Int t = A_cross(a,Tail);
        const Int h = A_cross(a,Head);
        
        Int t_label;
        Int h_label;
        
        if( c_labels.count(t) == 0 )
        {
            c_labels[t] = t_label = c_counter;
            pd.C_state[t_label] = C_state[t];
            ++c_counter;
        }
        else
        {
            t_label = c_labels[t];
        }
        
        if( c_labels.count(h) == 0 )
        {
            c_labels[h] = h_label = c_counter;
            pd.C_state[h_label] = C_state[h];
            ++c_counter;
        }
        else
        {
            h_label = c_labels[h];
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
    
    pd.SplitSimplify( PD_list );

    PD_ASSERT( pd.CheckAll() );
    
    PD_list.push_back( std::move(pd) );
    
//    ptoc(ClassName()+"::ExportComponent");
}

//public:
//
//template<bool mult_compQ = true>
//Tensor2<Int,Int> ConnectedSumSplittingArcs()
//{
//    ptic(ClassName()+"::ConnectedSumSplittingArcs");
//    
//    RequireFaces();
//    RequireComponents();
//    
//    Tensor1<Int,Int> F_face_idx ( F_arc_idx.Size() );
//    Tensor1<Int,Int> perm       ( F_arc_idx.Size() );
//    
//    // Third entry is the distance.
//    // Fourth entry is the face.
//    // TODO: We don't need the face information in production code.
//    Tensor2<Int,Int> arc_pairs  ( 2 * ArcCount(), 4 );
//    
//    Int pair_counter = 0;
//    
//    for( Int f = 0; f < FaceCount(); ++f )
//    {
////        logprint("=======");
////        logdump(f);
//        
//        const Int i_begin = F_arc_ptr[f  ];
//        const Int i_end   = F_arc_ptr[f+1];
//        
//        for( Int i = i_begin; i < i_end; ++i )
//        {
//            const Int a = F_arc_idx[i];
//            
//            F_face_idx[i] = (A_faces(a,0) == f) ? A_faces(a,1) : A_faces(a,0);
//        }
//        
//        // Now [F_face_idx[i_begin],...,F_face_idx[i_end][ contains all the indices of all the neighboring faces
//        
//        // We order this so that we can easily delete duplicates.
//        Ordering( &F_face_idx[i_begin], &perm[i_begin], i_end - i_begin );
//        
////        logvalprint( "f-f indices", ArrayToString( &F_face_idx[i_begin], {i_end - i_begin} ) );
////        logvalprint( "ordering", ArrayToString( &perm[i_begin], {i_end - i_begin} ) );
////        
////        std::stringstream s;
////        
////        s << "f-f indices reordered = {";
////        for( Int i = i_begin; i < i_end-1; ++i )
////        {
////            s << ToString( F_face_idx[ i_begin + perm[i] ] ) << ",";
////        }
////        s << ToString( F_face_idx[ i_begin + perm[i_end-1] ] );
////        
////        s << "}";
////        
////        logprint( s.str() );
//        
//        Int p;
//        Int g;
//        Int p_next = i_begin + perm[i_begin];
//        Int g_next = F_face_idx[p_next];
//        
//        Int i = i_begin;
//        
//        while( i < i_end - 1 )
//        {
//            p = p_next;
//            g = g_next;
//            
//            p_next = i_begin + perm[i+1];
//            g_next = F_face_idx[p_next];
//            
////            logprint( "{p,p_next} = {" + ToString(p) + "," + ToString(p_next) + "}  :  {g,g_next} = {" + ToString(g) + "," + ToString(g_next) + "}");
//            
//            if( g != g_next )
//            {
//                ++i;
//                continue;
//            }
//            
//            // g is a duplicate face.
//            
//            // Now there might be many arcs that are common to f and g.
//            // If there are k such arcs, then we get (k choose 2) pairs of edges.
//            // Since we invested some time in computing the faces, we want to split off as many summands as possible. So we want to split off the smallest possible summands.
//            // We do so by collecting for given `a` only pairs of edges `{a,b}` that have minimal distance `ArcDistance(a,b)`.
//            
//            Int j_begin = i;
//            Int j_end   = i+1;
//            
//            do
//            {
//                ++j_end;
//            }
//            while( (j_end < i_end) && ( F_face_idx[i_begin + perm[j_end]] == g ) );
//            
//            const Int a = F_arc_idx[p];
//            
//            // `k` is the position of `a` within its connected component.
//            const Int k = A_pos[a];
//            
//            // The minimal distance from `a` found so far amoung all arcs `b` from `f` to `g`.
//            Int d = std::numeric_limits<Int>::max();
//            
//            // There might be two arcs b that minimize the connected component.
//            // We only record the one b that is closest to b within face f in counter-clockwise direction.
//            // The other minimizer will be recorded when we traverse face g.
//            // If the minimizer b is unique (which is probably the generic case), then we will get the pair {a,b} twice: once from f and once from g.
//            
//            for( Int j = j_begin; j < j_end; ++j )
//            {
//                const Int q = i_begin + perm[j];
//                const Int b = F_arc_idx[q];
//                
//                // We don't want to collect arcs from bigons, because if they can be used to cut, then there will be nearby arc pairs that make for less confusing cuts:
//                //
//                // Example: We could collect {b,e}, but both {a,f} and {c,d} make better cuts.
//                //
//                //
//                //                    #
//                //                    #
//                //                    |
//                //                  f |
//                //            e       |
//                //              +-----X------###
//                //              |     |  a
//                //           d  |     |
//                //     ###------X-----+
//                //              |       b
//                //              | c
//                //              |
//                //              #
//                //              #
//                
//                
//                if( ArcsFormBigonQ(a,b) ) { continue; }
//                
//                // If the two arcs do not lie in the same connected component, we can discard them as well.
//                
//                // Example: The Hopf link:
//                //
//                //  +-------+
//                //  |       |
//                //  |   +-------+
//                //  |   |   |   |
//                //  +-------+   |
//                //      |       |
//                //      +-------+
//                
//                if constexpr (mult_compQ)
//                {
//                    if( A_comp[a] != A_comp[b] ) { continue; }
//                }
//                
//                const Int d_ab = ArcDistance(a,b);
//                
//                // If we have already found a pair that involves a and that is at least as good as this one, we can discard this one.
//                // Since we walk around faces in counter-clockwise order, in the case of a tie, we pick the b closest to a along the face f in counter-clockwise order.
//                
//                if( d_ab >= d ) { continue; }
//                
//                d = d_ab;
//                
////                logprint( "{a,b} = {" + ToString(a) + "," + ToString(b) + "}" );
//                
//                // l is the position `b` within its connected components.
//
//                const Int l = A_pos[b];
//                
//                PD_ASSERT( k != l );
//                
//                const bool a_comes_firstQ = (Abs(l - k) == d) == (k < l);
//                
//                PD_ASSERT( pair_counter < arc_pairs.Dimension(0) );
//                
//                arc_pairs(pair_counter,!a_comes_firstQ) = a;
//                arc_pairs(pair_counter, a_comes_firstQ) = b;
//                arc_pairs(pair_counter,              2) = d;
//                arc_pairs(pair_counter,              3) = f;
//                ++pair_counter;
//            }
//    
//            i = j_end;
//            
//        } // while( i < i_end - 1 )
//        
//    } // for( Int f = 0; f < FaceCount(); ++f )
//    
//    // Now we have many redundent arc pairs. For one, we get each pair twice from two different faces. These duplicates are easy to ignore, though.
//    // But there are worse cases,
//    // For example, already for the four trefoil summands, and assuming that a = 0, we get the (4 choose 2) = 6 pairs
//    //  {a,b}, {b,c}, {c,d}, {d,a}, {a,c}, {b,d}.
//    //
//    //
//    //              +-----+     +-----+
//    //              |     |     |     |
//    //              |     |  d  |     |
//    //        +-----------|-----------|-----+
//    //        |     |     |     |     |     |
//    //        |     |     |     |     |     |
//    //        +-----|-----+     +-----------+
//    //              |                 |
//    //            a |                 | c
//    //              |                 |
//    //        +-----------+     +-----|-----+
//    //        |     |     |     |     |     |
//    //        |     |     |  b  |     |     |
//    //        +---->|---------->|---->----->+
//    //              |     |     |     |
//    //              |     |     |     |
//    //              +-----+     +-----+
//    //
//    // Since we invested some time in computing the faces, we want to split off as many summands as possible. So we want to split off the smallest possible summands and return only
//    //  {a,b}, {b,c}, {c,d}, {d,a}.
//    //
//    // But things can become even more complicated if some arc pairs have a common crossing:
//    //
//    //                                #
//    //                                #
//    //                                X
//    //                              f ^
//    //                                |
//    //                          +<----|-----X###
//    //                        e |     |  a
//    //                  d       |     |
//    //              +-----------|-----+
//    //              |           |  b
//    //              |           |
//    //        +-----------+     |
//    //        |     |     |     |  c
//    //        |     |     |     |
//    //        +---->|-----------+
//    //              |     |
//    //              |     |
//    //              +-----+
//    //
//    // Here, if `a` is close to arc number 0, we would get the pairs
//    //
//    // {a,f},{b,e},{c,d},{a,e},{b,f}
//    
//    arc_pairs.template Resize<true>( pair_counter, 4 );
//    
//    ptoc(ClassName()+"::ConnectedSumSplittingArcs");
//    
//    return arc_pairs;
//}
//

//
//Int SplitConnectedSummands( std::vector<PlanarDiagram<Int>> & PD_list )
//{
//    ptic(ClassName()+"::SplitConnectedSummands");
//    
//    const Size_T PD_list_initial_size = PD_list.size();
//    
//    RequireFaces();
//    RequireComponents();
//    
//    Tensor2<Int,Int> split_arcs = ConnectedSumSplittingArcs();
//    
//    Tensor1<Int,Int> perm ( split_arcs.Dimension(0) );
//    
////    dump(split_arcs);
//    
//    // We split the large connected summands first to guarantee that shorter summands cannot contain deactivated arcs and crossings if both their ending edges are still active.
//    
//    Ordering<VarSize,std::greater<>>(
//        &split_arcs(0,2), 3, perm.data(), split_arcs.Dimension(0)
//    );
//    
////    dump(perm);
//    
//    for( Int i = 0; i < split_arcs.Dimension(0); ++i )
//    {
//        const Int j = perm[i];
//    
//        const Int a_0 = split_arcs(j,0);
//        const Int a_1 = split_arcs(j,1);
//        const Int d   = split_arcs(j,2);
//
//        if( !ArcActiveQ(a_0) )
//        {
////            wprint("Skipped because arc a_0 = " + ToString(a_0) + " is inactive.");
//            continue;
//        }
//        
//        if( !ArcActiveQ(a_1) )
//        {
////            wprint("Skipped because arc a_1 = " + ToString(a_1) + " is inactive.");
//            continue;
//        }
//
//        
////        dump(a_0);
////        dump(a_1);
////        dump(d);
//        
//        
//        AssertArc(a_0);
//        AssertArc(a_1);
//        PD_ASSERT( A_comp[a_0] == A_comp[a_1] );
//        
//        const Int t_0 = A_cross(a_0,Tail);
//        const Int h_0 = A_cross(a_0,Head);
//        const Int t_1 = A_cross(a_1,Tail);
//        const Int h_1 = A_cross(a_1,Head);
//        
//        AssertCrossing(t_0);
//        AssertCrossing(h_0);
//        AssertCrossing(t_1);
//        AssertCrossing(h_1);
//        
////        print(ArcString(a_0));
////        print(ArcString(a_1));
////        
////        print(CrossingString(t_0));
////        print(CrossingString(h_0));
////        print(CrossingString(t_1));
////        print(CrossingString(h_1));
//        
//        // Change this...
//        //
//        //         +-------------->+
//        //         |               |
//        //         |               |
//        //         |               |
//        //  +-------------->+      |
//        //  ^      |        |      |
//        //  |      |        |      |
//        //  |      |h_0     |t_1   v
//        //  +<-----|---------------+
//        //         ^        |
//        //         |        |
//        //     a_0 |        | a_1
//        //         |        |
//        //         |        v
//        //         X        X
//        //      t_0      h_1
//
//        // ... to this:
//        //
//        //         +-------------->+
//        //         |               |
//        //         |               |
//        //         |               |
//        //  +-------------->+      |
//        //  ^      |        |      |
//        //  |      |        |      |
//        //  |      |h_0     |t_1   v
//        //  +<-----|---------------+
//        //         ^        .
//        //         .        .
//        //         ..........
//        //
//        //            a_0
//        //         X.......>X
//        
//        
//        if( h_0 != t_1 )
//        {
//            Int a = NextArc(a_0);
//            
//            PlanarDiagram<Int> pd (d/2,0);
//            
//            std::unordered_map<Int,Int> c_labels;
//            std::unordered_map<Int,Int> a_labels;
//            Int a_label = 0;
//            Int c_counter = 0;
//            
//            do
//            {
////                print(ArcString(a));
//                
//                const Int t = A_cross(a,Tail);
//                const Int h = A_cross(a,Head);
//                
//                Int t_label;
//                Int h_label;
//                
//                if( c_labels.count(t) == 0 )
//                {
//                    c_labels[t] = t_label = c_counter;
//                    pd.C_state[t_label] = C_state[t];
//                    ++c_counter;
//                }
//                else
//                {
//                    t_label = c_labels[t];
//                }
//                
//                if( c_labels.count(h) == 0 )
//                {
//                    c_labels[h] = h_label = c_counter;
//                    pd.C_state[h_label] = C_state[h];
//                    ++c_counter;
//                }
//                else
//                {
//                    h_label = c_labels[h];
//                }
//                
//                const bool t_side = (C_arcs(t,Out,Right) == a);
//                const bool h_side = (C_arcs(h,In ,Right) == a);
//                
//                pd.C_arcs(t_label,Out,t_side) = a_label;
//                pd.C_arcs(h_label,In ,h_side) = a_label;
//                
//                pd.A_cross(a_label,Tail) = t_label;
//                pd.A_cross(a_label,Head) = h_label;
//                pd.A_state[a_label] = ArcState::Active;
//                
//                const Int a_next = C_arcs(h,Out,!h_side);
//                
//                DeactivateArc(a);
//                DeactivateCrossing<false>(h);
//                
//                ++a_label;
//                
//                PD_ASSERT( a_label < d );
//                
//                a = a_next;
//            }
//            while( a != a_1 );
//            
//            const Int h = h_0;
//            const Int t = t_1;
//            
//            const Int t_label = c_labels[t];
//            const Int h_label = c_labels[h];
//            
//            PD_ASSERT( (C_arcs(t,Out,Right) == a_1) or (C_arcs(t,Out,Left) == a_1) );
//            PD_ASSERT( (C_arcs(h,In ,Right) == a_0) or (C_arcs(h,In ,Left) == a_0) );
//            
//            const bool t_side = (C_arcs(t,Out,Right) == a_1);
//            const bool h_side = (C_arcs(h,In ,Right) == a_0);
//            
//            pd.C_arcs(t_label,Out,t_side) = a_label;
//            pd.C_arcs(h_label,In ,h_side) = a_label;
//            
//            pd.A_cross(a_label,Tail) = t_label;
//            pd.A_cross(a_label,Head) = h_label;
//            pd.A_state[a_label] = ArcState::Active;
//
//            // Caution: This might introduce a loop-arc.
//            // We could try to immediately apply an Reidemeister I move, but
//            // it could happen that we need to do further such moves.
//            // It is easier to risk introducing an arc-loop here and let the
//            // simplification routines do their job afterwards.
//  
////            if( t_0 == h_1 )
////            {
////                wprint( ClassName()+"::SplitConnectSummands: t_0 = " + ToString(t_0) + " == h_1." );
////            }
//            
//            Reconnect<false>(a_0,Head,a_1);
//            
//            AssertArc(a_0);
//            AssertArc(A_cross(a_0,Tail));
//            AssertArc(A_cross(a_0,Head));
//            if( a_label+1 != d )
//            {
//                eprint(ClassName()+"::SplitConnectSummands: split-off summand has " + ToString(a_label+1) + " arcs, but predicted were " + ToString(d) + " arcs." ) ;
//            }
//            
//            PD_ASSERT( pd.CheckAll() );
//            PD_ASSERT( CheckAll() );
//            
//            pd.SplitSimplify( PD_list );
//            
//            PD_list.push_back( std::move(pd) );
//        }
//    }
//    
//    if( split_arcs.Dimension(0) > 0 )
//    {
//        faces_initialized = false;
//        comp_initialized  = false;
//    }
//    
//    PD_ASSERT(CheckAll());
//                            
//    ptoc(ClassName()+"::SplitConnectedSummands");
//    
//    return static_cast<Int>(PD_list.size() - PD_list_initial_size);
//}
//
//
//
//bool ArcsFormBigonQ(const Int a, const Int b) const
//{
//    cptr<Int> A = A_cross.data(a);
//    cptr<Int> B = A_cross.data(b);
//    
//    return (A[0] == B[0] and A[1] == B[1]) or (A[0] == B[1] and A[1] == B[0]);
//}


//Int SplitConnectedSummands( std::vector<PlanarDiagram<Int>> & PD_list )
//{
//    ptic(ClassName()+"::SplitConnectedSummands");
//    
//    const Size_T PD_list_initial_size = PD_list.size();
//    
//    RequireFaces();
//    RequireComponents();
//    
//    Tensor2<Int,Int> split_arcs = ConnectedSumSplittingArcs();
//    
//    Tensor1<Int,Int> A_labels ( A_cross.Dimension(0) );
//    
//    A_labels.iota();
//    
//    for( Int i = 0; i < split_arcs.Dimension(0); ++i )
//    {
//        const Int a_0 = A_labels[split_arcs(i,0)];
//        const Int a_1 = A_labels[split_arcs(i,1)];
////        const Int d   = split_arcs(i,2);
//
//        if( !ArcActiveQ(a_0) )
//        {
//            wprint("Skipped because arc a_0 = " + ToString(a_0) + " is inactive.");
//            continue;
//        }
//        
//        if( !ArcActiveQ(a_1) )
//        {
//            wprint("Skipped because arc a_1 = " + ToString(a_1) + " is inactive.");
//            continue;
//        }
//
//
//        AssertArc(a_0);
//        AssertArc(a_1);
//        PD_ASSERT( A_comp[a_0] == A_comp[a_1] );
//        
//        const Int t_0 = A_cross(a_0,Tail);
//        const Int h_0 = A_cross(a_0,Head);
//        const Int t_1 = A_cross(a_1,Tail);
//        const Int h_1 = A_cross(a_1,Head);
//        
//        AssertCrossing(t_0);
//        AssertCrossing(h_0);
//        AssertCrossing(t_1);
//        AssertCrossing(h_1);
//        
////        print(ArcString(a_0));
////        print(ArcString(a_1));
////
////        print(CrossingString(t_0));
////        print(CrossingString(h_0));
////        print(CrossingString(t_1));
////        print(CrossingString(h_1));
//        
//        // Change this...
//        //
//        //         +-------------->+
//        //         |               |
//        //         |               |
//        //         |               |
//        //  +-------------->+      |
//        //  ^      |        |      |
//        //  |      |        |      |
//        //  |      |h_0     |t_1   v
//        //  +<-----|---------------+
//        //         ^        |
//        //         |        |
//        //     a_0 |        | a_1
//        //         |        |
//        //         |        v
//        //         X        X
//        //      t_0      h_1
//
//        // ... to this:
//        //
//        //         +-------------->+
//        //         |               |
//        //         |               |
//        //         |               |
//        //  +-------------->+      |
//        //  ^      |        |      |
//        //  |      |        |      |
//        //  |      |h_0     |t_1   v
//        //  +<-----|---------------+
//        //         ^        .
//        //         .        .
//        //         ..........
//        //
//        //            a_0
//        //         X.......>X
//        
//        
//        if( h_0 != t_1 )
//        {
//            Int a = NextArc(a_0);
//            
//            // TODO: Computing the distance is not for free as the traversal contains a lot of indirection. We have to traverse the arc range anyways, so this does not change the asymptotic complexity. Nevertheless, there might be a way to save this time.
//            
//            const Int d = ArcRangeLength(a_0,a_1);
//            
//            if( d < 6 )
//            {
//                logprint("Discarded split arc pair {" + ToString(a_0) + "," + ToString(a_1) + " } because the distance has become too small.");
//                         
//                continue;
//            }
//            
////            PlanarDiagram<Int> pd (d/2,0);
//            PlanarDiagram<Int> pd (d,0);
//            
//            std::unordered_map<Int,Int> c_labels;
//            Int a_label = 0;
//            Int c_counter = 0;
//            
//            do
//            {
//                const Int t = A_cross(a,Tail);
//                const Int h = A_cross(a,Head);
//                
//                Int t_label;
//                Int h_label;
//                
//                if( c_labels.count(t) == 0 )
//                {
//                    c_labels[t] = t_label = c_counter;
//                    pd.C_state[t_label] = C_state[t];
//                    ++c_counter;
//                }
//                else
//                {
//                    t_label = c_labels[t];
//                }
//                
//                if( c_labels.count(h) == 0 )
//                {
//                    c_labels[h] = h_label = c_counter;
//                    pd.C_state[h_label] = C_state[h];
//                    ++c_counter;
//                }
//                else
//                {
//                    h_label = c_labels[h];
//                }
//                
//                const bool t_side = (C_arcs(t,Out,Right) == a);
//                const bool h_side = (C_arcs(h,In ,Right) == a);
//                
//                pd.C_arcs(t_label,Out,t_side) = a_label;
//                pd.C_arcs(h_label,In ,h_side) = a_label;
//                
//                pd.A_cross(a_label,Tail) = t_label;
//                pd.A_cross(a_label,Head) = h_label;
//                pd.A_state[a_label] = ArcState::Active;
//                
//                const Int a_next = C_arcs(h,Out,!h_side);
//                
//                DeactivateArc(a);
//                DeactivateCrossing<false>(h);
//                
//                ++a_label;
//                
//                PD_ASSERT( a_label < d );
//                
//                a = a_next;
//            }
//            while( a != a_1 );
//            
//            const Int h = h_0;
//            const Int t = t_1;
//            
//            const Int t_label = c_labels[t];
//            const Int h_label = c_labels[h];
//            
//            PD_ASSERT( (C_arcs(t,Out,Right) == a_1) or (C_arcs(t,Out,Left) == a_1) );
//            PD_ASSERT( (C_arcs(h,In ,Right) == a_0) or (C_arcs(h,In ,Left) == a_0) );
//            
//            const bool t_side = (C_arcs(t,Out,Right) == a_1);
//            const bool h_side = (C_arcs(h,In ,Right) == a_0);
//            
//            pd.C_arcs(t_label,Out,t_side) = a_label;
//            pd.C_arcs(h_label,In ,h_side) = a_label;
//            
//            pd.A_cross(a_label,Tail) = t_label;
//            pd.A_cross(a_label,Head) = h_label;
//            pd.A_state[a_label] = ArcState::Active;
//
//            // Caution: This might introduce a loop-arc.
//            // We could try to immediately apply an Reidemeister I move, but
//            // it could happen that we need to do further such moves.
//            // It is easier to risk introducing an arc-loop here and let the
//            // simplification routines do their job afterwards.
//  
////            if( t_0 == h_1 )
////            {
////                wprint( ClassName()+"::SplitConnectSummands: t_0 = " + ToString(t_0) + " == h_1." );
////            }
//            
//            Reconnect<false>(a_0,Head,a_1);
//            
//            A_labels[a_1] = a_0;
//            
//            AssertArc(a_0);
//            AssertArc(A_cross(a_0,Tail));
//            AssertArc(A_cross(a_0,Head));
//            if( a_label+1 != d )
//            {
//                eprint(ClassName()+"::SplitConnectSummands: split-off summand has " + ToString(a_label+1) + " arcs, but predicted were " + ToString(d) + " arcs." ) ;
//            }
//            
//            PD_ASSERT( pd.CheckAll() );
//            PD_ASSERT( CheckAll() );
//            
//            pd.SplitSimplify( PD_list );
//            
//            PD_list.push_back( std::move(pd) );
//        }
//    }
//    
//    if( split_arcs.Dimension(0) > 0 )
//    {
//        faces_initialized = false;
//        comp_initialized  = false;
//    }
//    
//    PD_ASSERT(CheckAll());
//                            
//    ptoc(ClassName()+"::SplitConnectedSummands");
//    
//    return static_cast<Int>(PD_list.size() - PD_list_initial_size);
//}


Int ArcRangeLength( const Int a_begin, const Int a_end ) const
{
    if( a_end == a_begin )
    {
        return 0;
    }

    Int a = a_begin;
    Int d = 0;
    
    do{
        ++d;
        Int a = NextArc(a);
    }
    while( (a != a_begin) and (a != a_end) );
    
    if( a == a_begin )
    {
        wprint(ClassName()+"::ArcRangeLength: " + ArcString(a_begin) + " and  " + ArcString(a_end) + " do not belong to the same connected component. Returning -1.");
        
        dump(d);
        
        return -1;
    }
    else
    {
        return d;
    }
}

