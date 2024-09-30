Tensor2<Int,Int> ConnectedSumSplittingArcs()
{
    ptic(ClassName()+"::ConnectedSumSplittingArcs");
    
    RequireFaces();
    RequireComponents();
    
    Tensor1<Int,Int> F_face_idx ( F_arc_idx.Size() );
    Tensor1<Int,Int> perm       ( F_arc_idx.Size() );
    
    // Third entry is the distance
    Tensor2<Int,Int> arc_pairs  ( 2 * ArcCount(), 3 );
    
    Int pair_counter = 0;
    
    for( Int f = 0; f < FaceCount(); ++f )
    {
        const Int i_begin = F_arc_ptr[f  ];
        const Int i_end   = F_arc_ptr[f+1];
        
        for( Int i = i_begin; i < i_end; ++i )
        {
            const Int a = F_arc_idx[i];
            
            F_face_idx[i] = (A_faces(a,0) == f) ? A_faces(a,1) : A_faces(a,0);
        }
        
        Ordering( &F_face_idx[i_begin], &perm[i_begin], i_end - i_begin );
        
        for( Int i = i_begin; i < i_end - 1; ++i )
        {
            const Int p = i_begin + perm[i  ];
            const Int q = i_begin + perm[i+1];
            
            if( F_face_idx[p] == F_face_idx[q] )
            {
                const Int a = F_arc_idx[p];
                const Int b = F_arc_idx[q];
                
                const Int p = A_pos[a];
                const Int q = A_pos[b];
                
                const Int d = ArcDistance(a,b);
                
                PD_ASSERT( p != q );
                
                const bool pos = (Abs(p - q) == d) == (p < q);
                
//                dump(p);
//                dump(q);
//                dump(Abs(p - q));
//                dump(d);
                
                arc_pairs(pair_counter,!pos) = a;
                arc_pairs(pair_counter, pos) = b;
                arc_pairs(pair_counter,   2) = d;
                
                ++pair_counter;
            }
        }
    }
    
    arc_pairs.template Resize<true>( pair_counter, 3 );
    
    ptoc(ClassName()+"::ConnectedSumSplittingArcs");
    
    return arc_pairs;
}

bool SimplifySplit( std::vector<PlanarDiagram<Int>> & PD_list )
{
    ptic(ClassName()+"::SimplifySplit");
    
    bool globally_changed = false;
    bool changed = false;

    do
    {
        changed = Simplify4();
        
        Int splits = SplitConnectedSummands(PD_list);
        
        changed = changed || (splits > 0);
        
        globally_changed = globally_changed || changed;
    }
    while( changed );
    
    if( globally_changed )
    {
        *this = this->CreateCompressed();
    }
    
    ptoc(ClassName()+"::SimplifySplit");
    
    return globally_changed;
}

Int SplitConnectedSummands( std::vector<PlanarDiagram<Int>> & PD_list )
{
    ptic(ClassName()+"::SplitConnectedSummands");
    
    const Size_T PD_list_initial_size = PD_list.size();
    
    RequireFaces();
    RequireComponents();
    
    Tensor2<Int,Int> split_arcs = ConnectedSumSplittingArcs();
    
    Tensor1<Int,Int> perm ( split_arcs.Dimension(0) );
    
//    dump(split_arcs);
    
    // We split the large connected summands first to guarantee that shorter summands cannot contain deactivated arcs and crossings if their ending edges are still active.
    
    Ordering<VarSize,std::greater<>>(
        &split_arcs(0,2), 3, perm.data(), split_arcs.Dimension(0)
    );
    
//    dump(perm);
    
    for( Int i = 0; i < split_arcs.Dimension(0); ++i )
    {
        const Int j = perm[i];
    
        const Int a_0 = split_arcs(j,0);
        const Int a_1 = split_arcs(j,1);
        const Int d   = split_arcs(j,2);

        if( !ArcActiveQ(a_0) )
        {
//            wprint("Skipped because arc a_0 = " + ToString(a_0) + " is inactive.");
            continue;
        }
        
        if( !ArcActiveQ(a_1) )
        {
//            wprint("Skipped because arc a_1 = " + ToString(a_1) + " is inactive.");
            continue;
        }

        
//        dump(a_0);
//        dump(a_1);
//        dump(d);
        
        
        AssertArc(a_0);
        AssertArc(a_1);
        PD_ASSERT( A_comp[a_0] == A_comp[a_1] );
        
        const Int t_0 = A_cross(a_0,Tail);
        const Int h_0 = A_cross(a_0,Head);
        const Int t_1 = A_cross(a_1,Tail);
        const Int h_1 = A_cross(a_1,Head);
        
        AssertCrossing(t_0);
        AssertCrossing(h_0);
        AssertCrossing(t_1);
        AssertCrossing(h_1);
        
//        print(ArcString(a_0));
//        print(ArcString(a_1));
//        
//        print(CrossingString(t_0));
//        print(CrossingString(h_0));
//        print(CrossingString(t_1));
//        print(CrossingString(h_1));
        
        // Change this...
        //
        //         +-------------->+
        //         |               |
        //         |               |
        //         |               |
        //  +-------------->+      |
        //  ^      |        |      |
        //  |      |        |      |
        //  |      |h_0     |t_1   v
        //  +<-----|---------------+
        //         ^        |
        //         |        |
        //     a_0 |        | a_1
        //         |        |
        //         |        v
        //         X        X
        //      t_0      h_1

        // ... to this:
        //
        //         +-------------->+
        //         |               |
        //         |               |
        //         |               |
        //  +-------------->+      |
        //  ^      |        |      |
        //  |      |        |      |
        //  |      |h_0     |t_1   v
        //  +<-----|---------------+
        //         ^        .
        //         .        .
        //         ..........
        //
        //            a_0
        //         X.......>X
        
        
        if( h_0 != t_1 )
        {
            Int a = NextArc(a_0);
            
            PlanarDiagram<Int> pd (d/2,0);
            
            std::unordered_map<Int,Int> c_labels;
            std::unordered_map<Int,Int> a_labels;
            Int a_label = 0;
            Int c_counter = 0;
            
            do
            {
//                print(ArcString(a));
                
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
                
                PD_ASSERT( a_label < d );
                
                a = a_next;
            }
            while( a != a_1 );
            
            const Int h = h_0;
            const Int t = t_1;
            
            const Int t_label = c_labels[t];
            const Int h_label = c_labels[h];
            
            PD_ASSERT( (C_arcs(t,Out,Right) == a_1) || (C_arcs(t,Out,Left) == a_1) );
            PD_ASSERT( (C_arcs(h,In ,Right) == a_0) || (C_arcs(h,In ,Left) == a_0) );
            
            const bool t_side = (C_arcs(t,Out,Right) == a_1);
            const bool h_side = (C_arcs(h,In ,Right) == a_0);
            
            pd.C_arcs(t_label,Out,t_side) = a_label;
            pd.C_arcs(h_label,In ,h_side) = a_label;
            
            pd.A_cross(a_label,Tail) = t_label;
            pd.A_cross(a_label,Head) = h_label;
            pd.A_state[a_label] = ArcState::Active;

            // Caution: This might introduce a loop-arc.
            // We could try to immediately apply an Reidemeister I move, but
            // it could happen that we need to do further such moves.
            // It is easiert to risk introducing an arc-loop here and let the
            // simplification routines do their thing afterwards.
  
//            if( t_0 == h_1 )
//            {
//                wprint( ClassName()+"::SplitConnectSummands: t_0 = " + ToString(t_0) + " == h_1." );
//            }
            
            Reconnect<false>(a_0,Head,a_1);
            
            AssertArc(a_0);
            AssertArc(A_cross(a_0,Tail));
            AssertArc(A_cross(a_0,Head));
            if( a_label+1 != d )
            {
                eprint(ClassName()+"::SplitConnectSummands: split-off summand has " + ToString(a_label+1) + " arcs, but predicted were " + ToString(d) + " arcs." ) ;
            }
            
            PD_ASSERT( pd.CheckAll() );
            PD_ASSERT( CheckAll() );
            
            pd.SimplifySplit( PD_list );
            
            PD_list.push_back( std::move(pd) );
        }
    }
    
    if( split_arcs.Dimension(0) > 0 )
    {
        faces_initialized = false;
        comp_initialized  = false;
    }
    
    PD_ASSERT(CheckAll());
                            
    ptoc(ClassName()+"::SplitConnectedSummands");
    
    return static_cast<Int>(PD_list.size() - PD_list_initial_size);
}
