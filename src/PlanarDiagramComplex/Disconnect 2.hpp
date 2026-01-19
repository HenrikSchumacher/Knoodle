public:

Size_T DisconnectDiagrams()
{
    Size_T counter = 0;
    
    for( PD_T & pd : pd_list )
    {
        counter += Disconnect( pd, pd_list );
    }
    
    return counter;
}

/*!@brief This attempts to disconnects summands by using the information on the diagram's faces. Not all connected summands might be visible, though. This does one scan through all the faces of the diagram. Later simplification and further scans might be neccessary.
 
 * Only the local surgery is performed so that all found connected summands will reside as split subdiagrams within the current planar diagram.
 *
 *  The return value is the number of disconnection moves made.
 */

Size_T Disconnect( PD_T & pd, mref<PDC_T::PDList_T> pd_output )
{
    TOOLS_PTIMER(timer,MethodName("Disconnect"));
    
    TwoArraySort<Int,Int,Int> sorter ( pd.max_arc_count );
    
    const Int f_max_size = pd.MaxFaceSize();
    
    Aggregator<Int,Int> f_dA (f_max_size);
    Aggregator<Int,Int> f_F  (f_max_size);
    
    Size_T changes     = 0;
    Size_T changes_old = 0;
    
    Size_T iter = 0;
    do
    {
        TOOLS_LOGDUMP(changes);
        
        const auto & F_dA  = pd.FaceDarcs();
        cptr<Int> F_dA_ptr = F_dA.Pointers().data();
        cptr<Int> F_dA_idx = F_dA.Elements().data();
        cptr<Int> dA_F     = pd.ArcFaces().data();
        
        const Int face_count = F_dA.SublistCount();
        
        changes_old = changes;
        
        // DEBUGGING
        pd.PrintInfo();
        
        for( Int f = 0; f < face_count; ++f )
        {
            changes += Disconnect(pd,pd_output,f,sorter,f_dA,f_F,F_dA_ptr,F_dA_idx,dA_F);
            
            // Problem: I can get an infinite loop otherwise because one pair of arcs can revert what the reverse pair did!
            if( changes > changes_old)
            {
                pd.ClearCache();
                continue;
            }
        }
        ++iter;
    }
    while( (changes > changes_old) && (iter < 5) );
    
    if( changes > Size_T(0) ) { this->ClearCache(); }
    
    return (iter < 5) ? changes : Size_T(0);
}



private:

/*! @brief Checks whether face `f` has a double face neighbor. If yes, it may do the surgery to splitt of a one of more connected summands. It may however decide to perform some Reidemeister moves to remove crossings in the case that this would lead to an unknot. Returns the number of changes made.
 */


Size_T Disconnect(
    mref<PD_T> pd,
    [[maybe_unused]] mref<PDList_T> pd_output,
    const Int f,
    TwoArraySort<Int,Int,Int> & sorter,
    mref<Aggregator<Int,Int>> f_dA,
    mref<Aggregator<Int,Int>> f_F,
    cptr<Int>    F_dA_ptr,
    cptr<Int>    F_dA_idx,
    cptr<Int>    dA_F
)
{
    PD_PRINT("Disconnect(" + ToString(f) +")");
    // TODO: If the diagram is disconnected, then this might raise problems here. We need to prevent that!
    
    // TODO: Introduce some tracking so that we can tell where the components are split off.
    
    const Int i_begin = F_dA_ptr[f  ];
    const Int i_end   = F_dA_ptr[f+1];

    f_dA.Clear();
    f_F.Clear();
    
    for( Int i = i_begin; i < i_end; ++i )
    {
        const Int da = F_dA_idx[i];
        
        auto [a,d] = PD_T::FromDarc(da);
        
        PD_ASSERT( f == dA_F[da] );
        
        if( pd.ArcActiveQ(a) )
        {
            f_dA.Push(da);
            
            // Tell `f` that it is neighbor of the face that belongs to the twin of `A`.
            f_F.Push( dA_F[PD_T::FlipDarc(da)] );
        }
    }

        
    const Int f_size = f_dA.Size();
    
    if( f_size == Int(1) ) [[unlikely]]
    {
        eprint("Face with only one arc detected.");
        throw std::runtime_error("Face with only one arc detected.");
        
        auto [a,d] = PD_T::FromDarc(f_dA[0]);

        // Warning: This alters the diagram but preserves the Cache -- which is important to not invalidate `FaceDarcs()`, etc. I don't think that this will ever happen because `DisconnectSummand` is called only in a very controlled context when all possible Reidemeister I moves have been performed already.
        
        if( pd.A_cross(a,Tail) == pd.A_cross(a,Head) )
        {
            // TODO: Repair this.
            eprint("Loop detected.");
            throw std::runtime_error("Loop detected.");
            
            return 1;
        }
//        if( Private_Reidemeister_I<true,true>(a) )
//        {
//            wprint(ClassName()+"::DisconnectSummand: Found a face with just one arc around it. Tried to call Private_Reidemeister_I to remove. But maybe the face information is violated. Check your results thoroughly.");
//            return true;
//        }
//        else
//        {
//            // TODO: Can we get here? What to do here? This need not be a Reidemeister I move, since we ignore arcs on the current strand, right? So what is this?
//
//            eprint(ClassName()+"::DisconnectSummand: Face with one arc detected that is not a loop arc. Something must have gone very wrong here.");
//            return false;
//        }
    }
    
    sorter( &f_F[0], &f_dA[0], f_size );
    
    
    TOOLS_LOGDUMP(f_dA);
    TOOLS_LOGDUMP(f_F );
    
    Int i = 0;
    
    while( i+1 < f_size )
    {
        if( f_F[i] != f_F[i+1] )
        {
            ++i;
            continue;
        }
        
        PD_ASSERT(i+1 < f_size);
        PD_ASSERT(f_F[i] == f_F[i+1]);
        
        const Int g = f_F[i];
        
        auto [a,d_a] = PD_T::FromDarc(f_dA[i  ]);
        auto [b,d_b] = PD_T::FromDarc(f_dA[i+1]);
        
        PD_ASSERT(a != b);
        
        TOOLS_LOGDUMP(a);
        TOOLS_LOGDUMP(d_a);
        TOOLS_LOGDUMP(b);
        TOOLS_LOGDUMP(d_b);
        
        pd.AssertArc(a);
        pd.AssertArc(b);
        
        PD_ASSERT( pd.ArcActiveQ(a) );
        PD_ASSERT( pd.ArcActiveQ(b) );
        PD_ASSERT( pd.A_color[a] == pd.A_color[b] );
        
        if( pd.A_cross(a,Tail) == pd.A_cross(b,Head) )
        {
            PD_PRINT("A_cross(a,Tail) == A_cross(b,Head)");
            if( pd.A_cross(a,Head) == pd.A_cross(b,Tail) )
            {
                PD_PRINT("A_cross(a,Head) == A_cross(b,Tail)");
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
                
                const Int c_0 = pd.A_cross(a,Tail);
                const Int c_1 = pd.A_cross(a,Head);
                
                PD_ASSERT( c_0 != c_1 );
                
                const Int a_prev = pd.NextArc(a,Tail,c_0);
                const Int b_next = pd.NextArc(b,Head,c_0);
                const Int a_next = pd.NextArc(a,Head,c_1);
                const Int b_prev = pd.NextArc(b,Tail,c_1);
                
                if( a_prev != b_next )
                {
                    pd.template Reconnect<Head>(a_prev,b_next);
                }
                
                if( b_prev != a_next )
                {
                    pd.template Reconnect<Tail>(a_next,b_prev);
                }
                
                pd.DeactivateArc(a);
                pd.DeactivateArc(b);
                pd.DeactivateCrossing(c_0);
                pd.DeactivateCrossing(c_1);
            
                if( (a_prev == b_next) && (b_prev == a_next) )
                {
                    CreateUnlinkFromArc(pd,a);
                }
                
                return 1;
            }
            else
            {
                PD_PRINT("A_cross(a,Head) != A_cross(b,Tail)");
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
                
                const Int c = pd.A_cross(a,Tail);
                
                const Int a_prev = pd.NextArc(a,Tail,c);
                const Int b_next = pd.NextArc(b,Head,c);
                
                pd.template Reconnect<Head>(a_prev,b_next);
                pd.template Reconnect<Tail>(a,b);
                pd.DeactivateCrossing(c);

                return 1;
            }
        }
        else
        {
            PD_PRINT("A_cross(a,Tail) != A_cross(b,Head)");
            if( pd.A_cross(b,Tail) == pd.A_cross(a,Head) )
            {
                PD_PRINT("A_cross(b,Tail) == pd.A_cross(a,Head)");
                
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
                
                const Int c = pd.A_cross(a,Head);

                const Int a_next = pd.NextArc(a,Head,c);
                const Int b_prev = pd.NextArc(b,Tail,c);
                
                pd.template Reconnect<Head>(a,b);
                pd.template Reconnect<Tail>(a_next,b_prev);
                pd.DeactivateCrossing(c);
            
                return 1;
            }
            else
            {
                PD_PRINT("A_cross(b,Tail) != A_cross(a,Head)");
                
//                PD_ASSERT(d_a == d_b);
                
                /* `a` and `b` do not have any crossing in common.
                 *  Assuming that d_a == d_b == Head.
                 *
                 *                   g
                 *
                 *         #######<------#######
                 *            |             ^
                 *            |             |
                 *            |             |
                 *     g    b |      f      | a    g
                 *            |             |
                 *            |             |
                 *            v             |
                 *         #######------>#######
                 *
                 *                   g
                 */
                
                /* `a` and `b` do not have any crossing in common.
                 *  Assuming that d_a == d_b == Head.
                 *
                 *                   g
                 *                         c_a
                 *         #######<------###X###
                 *            |             ^
                 *            |     f_0     |
                 *     g    b +-------------+      g
                 *
                 *            +-------------+ a
                 *            |     f_1     |
                 *            v             |
                 *         ###X###------>#######
                 *           c_b
                 *                   g
                 */
                
                pd.template AssertArc<1>(a);
                pd.template AssertArc<1>(b);
                
                const Int c_a = pd.A_cross(a,Head);
                const Int c_b = pd.A_cross(b,Head);
                
                pd.template AssertCrossing<1>(c_a);
                pd.template AssertCrossing<1>(c_b);
                
                PD_VALPRINT("c_a", pd.CrossingString(c_a));
                PD_VALPRINT("c_b", pd.CrossingString(c_b));
                
                PD_VALPRINT("a", pd.ArcString(a));
                PD_VALPRINT("b", pd.ArcString(b));
                
                pd.A_cross(b,Head) = c_a;
                pd.A_cross(a,Head) = c_b;
                
                pd.template SetMatchingPortTo<In>(c_a,a,b);
                pd.template SetMatchingPortTo<In>(c_b,b,a);
                
                PD_VALPRINT("c_a", pd.CrossingString(c_a));
                PD_VALPRINT("c_b", pd.CrossingString(c_b));
                
                PD_VALPRINT("a", pd.ArcString(a));
                PD_VALPRINT("b", pd.ArcString(b));
                
                return 1;
            }
        }
        
    } // while( i+1 < f_size )
    
    return 0;
}

//Size_T Disconnect(
//    mref<PD_T> pd,
//    [[maybe_unused]] mref<PDList_T> pd_output,
//    const Int f,
//    TwoArraySort<Int,Int,Int> & sorter,
//    mref<Aggregator<Int,Int>> f_dA,
//    mref<Aggregator<Int,Int>> f_F,
//    cptr<Int>    F_dA_ptr,
//    cptr<Int>    F_dA_idx,
//    cptr<Int>    dA_F
//)
//{
//    PD_PRINT(MethodName("Disconnect") + "(" + ToString(f) + ")");
//    // TODO: If the diagram is disconnected, then this might raise problems here. We need to prevent that!
//    
//    // TODO: This disconnects only one pair of arcs per face. Can we improve this?
//    
//    const Int i_begin = F_dA_ptr[f  ];
//    const Int i_end   = F_dA_ptr[f+1];
//
//    f_dA.Clear();
//    f_F.Clear();
//    
//    for( Int i = i_begin; i < i_end; ++i )
//    {
//        const Int da = F_dA_idx[i];
//        
//        auto [a,d] = PD_T::FromDarc(da);
//        
//        PD_ASSERT( f == dA_F[da] );
//        
//        if( pd.ArcActiveQ(a) )
//        {
//            f_dA.Push(da);
//            
//            // Tell `f` that it is neighbor of the face that belongs to the twin of `da`.
//            f_F.Push( dA_F[PD_T::FlipDarc(da)] );
//        }
//    }
//        
//    const Int f_size = f_dA.Size();
//    
//    if( f_size == Int(1) ) [[unlikely]]
//    {
//        auto [a,d] = PD_T::FromDarc(f_dA[0]);
//        
//        if( pd.A_cross(a,Tail) == pd.A_cross(a,Head) )
//        {
//            // Warning: This alters the diagram but preserves the Cache -- which is important to not invalidate the references to `FaceDarcs()`, etc. I don't think that this will ever happen because `Disconnect` is typically called only in a very controlled context when all possible Reidemeister I moves have been performed already.
//            eprint(MethodName("Disconnect")+": Face with one arc detected that is not a loop arc. Something must have gone very wrong here.");
//        }
////        if( Private_Reidemeister_I<true,true>(a) )
////        {
////            wprint(MethodName("Disconnect")+": Found a face with just one arc around it. Tried to call Private_Reidemeister_I to remove it. But maybe the face information is violated. Check your results thoroughly.");
////            return 1;
////        }
////        else
////        {
////            // TODO: Can we get here? What to do here? This need not be a Reidemeister I move, since we ignore arcs on the current strand, right? So what is this?
////
////            eprint(MethodName("Disconnect")+": Face with one arc detected that is not a loop arc. Something must have gone very wrong here.");
////            return 0;
////        }
//    }
//    
//    sorter( &f_F[0], &f_dA[0], f_size );
//    
//    Int i = 0;
//    
//    while( i+1 < f_size )
//    {
//        if( f_F[i] != f_F[i+1] )
//        {
//            ++i;
//            continue;
//        }
//        
//        // Now f_F[i] == f_F[i+1]); find the corresponding arcs.
//        
//        const Int a = (f_dA[i  ] >> 1);
//        const Int b = (f_dA[i+1] >> 1);
//        
////        // It is not quite clear to me why this is guaranteed.
////        AssertArc<1>(a);
////        AssertArc<1>(b);
////        if( !pd.ArcActiveQ(a) || !pd.ArcActiveQ(b) )
////        {
////            continue;
////        }
//        PD_ASSERT(pd.A_color[a] == pd.A_color[b]);
//        
//        if( pd.A_cross(a,Tail) == pd.A_cross(b,Head) )
//        {
//            if( pd.A_cross(a,Head) == pd.A_cross(b,Tail) )
//            {
//                PD_ASSERT( pd.A_cross(a,Tail) == pd.A_cross(b,Head) );
//                PD_ASSERT( pd.A_cross(a,Head) == pd.A_cross(b,Tail) );
//                
//                // We split a bigon.
//                
//                /*   #  #  # #  #  #
//                //   ## # ## ## # ##
//                //    ##X##   ##X##
//                //       \     ^
//                // b_prev \   / a_next
//                //         v /
//                //          X c_1
//                //         ^ \
//                //        /   \
//                //       /     \
//                //    a +       +
//                //      |       |
//                // .....|.......|..... <<-- We cut here.
//                //      |       |
//                //      +       + b
//                //       \     /
//                //        \   /
//                //         \ v
//                //          X  c_0
//                //         / ^
//                // b_next /   \ a_prev
//                //       v     \
//                //      X       X
//                */
//                
//                const Int c_0 = pd.A_cross(a,Tail);
//                const Int c_1 = pd.A_cross(a,Head);
//                
//                PD_ASSERT( c_0 != c_1 );
//                
//                const Int a_prev = pd.NextArc(a,Tail,c_0);
//                const Int b_next = pd.NextArc(b,Head,c_0);
//                const Int a_next = pd.NextArc(a,Head,c_1);
//                const Int b_prev = pd.NextArc(b,Tail,c_1);
//                
//                // Should not happen if we have dealt with loops already.
//                if( a_prev != b_next )
//                {
//                    pd.template Reconnect<Head>(a_prev,b_next);
//                }
//                
//                // Should not happen if we have dealt with loops already.
//                if( b_prev != a_next )
//                {
//                    pd.template Reconnect<Tail>(a_next,b_prev);
//                }
//                
//                pd.DeactivateArc(a);
//                pd.DeactivateArc(b);
//                pd.DeactivateCrossing(c_0);
//                pd.DeactivateCrossing(c_1);
//            
//                // Should not happen if have run local ArcSimplifier or StrandSimplifier already.
//                if( (a_prev == b_next) && (b_prev == a_next) )
//                {
//                    wprint(MethodName("Disconnect")+": (a_prev == b_next) && (b_prev == a_next). We should not get here.");
//                    CreateUnlinkFromArc(pd,a);
//                    return 1;
//                }
//                else
//                {
//                    return 1;
//                }
//            }
//            else
//            {
//                PD_ASSERT( pd.A_cross(a,Tail) == pd.A_cross(b,Head) );
//                PD_ASSERT( pd.A_cross(a,Head) != pd.A_cross(b,Tail) );
//                
//                /*  #################
//                //  #               #
//                //  #               #
//                //  #################
//                //     ^         /
//                //      \       /
//                //     a \     / b
//                //        \   /
//                //         \ v
//                //          X c
//                //         / ^
//                // b_next /   \ a_prev
//                //       /     \
//                //      /       \
//                //     v         \
//                //
//                // Beware of Reidemeister I move here!
//                */
//                
//                const Int c = pd.A_cross(a,Tail);
//                
//                const Int a_prev = pd.NextArc(a,Tail,c);
//                const Int b_next = pd.NextArc(b,Head,c);
//
//                // It is crucial that at least one of a or b these gets deactivated.
//                pd.template Reconnect<Tail>(a,b);
//                
//                if( a_prev == b_next )
//                {
//                    wprint(MethodName("DisconnectSumman")+": a_prev == b_next. We should not get here.");
//                    // Should not happen if we have dealt with loops already, no?
//                    pd.DeactivateArc(a_prev);
//                    pd.DeactivateCrossing(c);
//                    return 1;
//                }
//                else
//                {
//                    pd.template Reconnect<Head>(a_prev,b_next);
//                    pd.DeactivateCrossing(c);
//                    return 1;
//                }
//            }
//        }
//        else if( pd.A_cross(a,Head) == pd.A_cross(b,Tail) )
//        {
//            PD_ASSERT( pd.A_cross(a,Tail) != pd.A_cross(b,Head) );
//            PD_ASSERT( pd.A_cross(a,Head) == pd.A_cross(b,Tail) );
//            
//            /*  #################
//            //  #               #
//            //  #               #
//            //  #################
//            //     \         ^
//            //      \       /
//            //     a \     / b
//            //        \   /
//            //         v /
//            //          X c
//            //         ^ \
//            // b_prev /   \ a_next
//            //       /     \
//            //      /       \
//            //     /         v
//            //
//            // Beware of Reidemeister I move here!
//            */
//            
//            const Int c = pd.A_cross(a,Head);
//
//            const Int a_next = pd.NextArc(a,Head,c);
//            const Int b_prev = pd.NextArc(b,Tail,c);
//            
//            pd.template Reconnect<Head>(a,b);
//            
//            if( b_prev == a_next )
//            {
//                wprint(MethodName("Disconnect")+": b_prev == a_next. We should not get here.");
//                
//                // Should not happen if we have dealt with loops already, no?
//                pd.DeactivateArc(b_prev);
//                pd.DeactivateCrossing(c);
//                return 1;
//            }
//            else
//            {
//                pd.template Reconnect<Head>(b_prev,a_next);
//                pd.DeactivateCrossing(c);
//                return 1;
//            }
//        }
//        
//        /* `a` and `b` do not have any crossing in common.
//        //
//        //    #####################
//        //       ^             |
//        //       |             |
//        //     a |             | b
//        //       |             |
//        //       |             v
//        //    #####################
//        */
//        
//        PD_ASSERT( pd.A_cross(a,Tail) != pd.A_cross(b,Head) );
//        PD_ASSERT( pd.A_cross(a,Head) != pd.A_cross(b,Tail) );
//
//        const Int c_a = pd.A_cross(a,Head);
//        const Int c_b = pd.A_cross(b,Head);
//        
//        pd.A_cross(b,Head) = c_a;
//        pd.A_cross(a,Head) = c_b;
//        
//        // Both a and b stay active!s
//        pd.template SetMatchingPortTo<In>(c_a,a,b);
//        pd.template SetMatchingPortTo<In>(c_b,b,a);
//        
//        return 1;
//    }
//
//    return false;
//}
