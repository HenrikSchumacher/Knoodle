private:

bool Reidemeister_IIa_Horizontal( const Int c_0 )
{
    if( !CrossingActiveQ(c_0) )
    {
        return false;
    }
    
    PD_PRINT("Reidemeister_IIa_Horizontal");
    
    auto C_0 = GetCrossing( c_0 );

    auto A_1 = GetArc( C_0(Out,Left ) );
    auto B_3 = GetArc( C_0(Out,Right) );
    auto B_2 = GetArc( C_0(In ,Left ) );
    auto A_0 = GetArc( C_0(In ,Right) );
    auto C_1 = GetCrossing( A_1(Head) );
    auto A_2 = NextArc<Head>(A_1);
    auto C_2 = GetCrossing( A_2(Head) );
    
    if( SameHandednessQ(C_0,C_2) )
    {
        // Not what we are looking for.
        return false;
    }
    
    if( A_2 != C_2(In,Left)  )
    {
        // Rather look for twist move.
        return false;
    }
    
    auto B_1 = GetArc( C_2(Out,Left ) );
    auto A_3 = GetArc( C_2(Out,Right) );
    auto B_0 = GetArc( C_2(In ,Right) );
    auto C_3 = GetCrossing( B_1(Head) );
    
    if( C_1 == C_3  )
    {
        // Not what we are looking for.
        return false;
    }
    
    if( SameHandednessQ(C_1,C_3) )
    {
        return false;
    }
    
    if( C_3 != B_2(Tail) )
    {
        // Not what we are looking for.
        return false;
    }
    
//     C_0 Righthanded
//                             O
//                             |
//                             |C_1
//                       O<----X-----O
//                      /      |      ^
//                 A_2 /       |       \ A_1
//                    v        O        \
//       B_0 O       O         |         O       O B_3
//            \     /          |          ^     ^
//             \   /           |           \   /
//              \ /            |            \ /
//               \ C_2         |             / C_0
//              / \            |            / \
//             /   \           |           /   \
//            v     v          |          /     \
//       A_3 O       O         |         O       O A_0
//                    \        O        ^
//                 B_1 \       |       / B_2
//                      v      |C_3   /
//                       O-----X---->O
//                             |
//                             |
//                             O
    
    
    PD_ASSERT( OppositeHandednessQ(C_0,C_2) );
    
    PD_ASSERT( C_0(Out,Left ) == A_1 );
    PD_ASSERT( C_0(Out,Right) == B_3 );
    PD_ASSERT( C_0(In ,Left ) == B_2 );
    PD_ASSERT( C_0(In ,Right) == A_0 );
    
    PD_ASSERT( C_2(Out,Left ) == B_1 );
    PD_ASSERT( C_2(Out,Right) == A_3 );
    PD_ASSERT( C_2(In ,Left ) == A_2 );
    PD_ASSERT( C_2(In ,Right) == B_0 );
    
    
    PD_ASSERT( HeadQ(A_0,C_0) );
    PD_ASSERT( HeadQ(B_0,C_2) );
    
    PD_ASSERT( HeadQ(A_1,C_1) );
    PD_ASSERT( TailQ(A_2,C_1) );
    
    PD_ASSERT( HeadQ(B_1,C_3) );
    PD_ASSERT( TailQ(B_2,C_3) );
    
    PD_ASSERT( TailQ(A_3,C_2) );
    PD_ASSERT( TailQ(B_3,C_0) );
    
    PD_ASSERT( OppositeHandednessQ(C_1,C_3) );
    
    PD_PRINT("Incoming data");
    
    PD_VALPRINT("C_0",C_0);
    PD_VALPRINT("C_1",C_1);
    PD_VALPRINT("C_2",C_2);
    PD_VALPRINT("C_3",C_3);
    
    PD_VALPRINT("A_0",A_0);
    PD_VALPRINT("A_1",A_1);
    PD_VALPRINT("A_2",A_2);
    PD_VALPRINT("A_3",A_3);
    
    PD_VALPRINT("B_0",B_0);
    PD_VALPRINT("B_1",B_1);
    PD_VALPRINT("B_2",B_2);
    PD_VALPRINT("B_3",B_3);
    
    //
    //                         O
    //                         |
    //        B_0              |C_1           B_3
    //           +------>O-----X---->O-------+
    //          /              |              \
    //         /               |               \
    //        /                O                v
    //       O                 |                 O
    //                         |
    //                         |
    //                         |
    //                         |
    //                         |
    //                         |
    //                         |
    //       O                 |                 O
    //        ^                O                /
    //         \               |               /
    //          \              |C_3           /
    //           +-------O<----X-----O<------+
    //        A_3              |              A_0
    //                         |
    //                         O


    // Rewire C_1. (Reconnect does not work here.)
    const bool side_1 = (C_1(In,Right) == A_1) ? Right : Left;
    C_1(Out,!side_1) = C_1(Out, side_1);
    C_1(In , side_1) = C_1(In ,!side_1);
    C_1(Out, side_1) = B_3.Idx();
    C_1(In ,!side_1) = B_0.Idx();
    FlipHandedness(C_1.Idx());
    A_0(Head) = C_3.Idx();
    A_3(Tail) = C_3.Idx();

    
    // Rewire C_3. (Reconnect does not work here.)
    const bool side_3 = (C_3(In,Right) == B_1) ? Right : Left;
    C_3(Out,!side_3) = C_3(Out, side_3);
    C_3(In , side_3) = C_3(In ,!side_3);
    C_3(Out, side_3) = A_3.Idx();
    C_3(In ,!side_3) = A_0.Idx();
    FlipHandedness(C_3.Idx());
    B_0(Head) = C_1.Idx();
    B_3(Tail) = C_1.Idx();
    
    DeactivateArc(A_1.Idx());
    DeactivateArc(A_2.Idx());
    DeactivateArc(B_1.Idx());
    DeactivateArc(B_2.Idx());
    DeactivateCrossing(C_0.Idx());
    DeactivateCrossing(C_2.Idx());
 
    PD_PRINT("Changed data");
    
    PD_VALPRINT("C_1",C_1);
    PD_VALPRINT("C_3",C_3);
    
    PD_VALPRINT("A_0",A_0);
    PD_VALPRINT("A_3",A_3);
    
    PD_VALPRINT("B_0",B_0);
    PD_VALPRINT("B_3",B_3);
    
    
    PD_ASSERT(!C_0.ActiveQ() );
    PD_ASSERT(!C_2.ActiveQ() );
    PD_ASSERT( C_1.ActiveQ() );
    PD_ASSERT( C_3.ActiveQ() );
    
    PD_ASSERT( CheckCrossing(C_1.Idx()) );
    PD_ASSERT( CheckCrossing(C_3.Idx()) );
    
    PD_ASSERT(!A_1.ActiveQ() );
    PD_ASSERT(!B_1.ActiveQ() );
    PD_ASSERT(!A_2.ActiveQ() );
    PD_ASSERT(!B_2.ActiveQ() );
    PD_ASSERT( A_0.ActiveQ() );
    PD_ASSERT( B_0.ActiveQ() );
    PD_ASSERT( A_3.ActiveQ() );
    PD_ASSERT( B_3.ActiveQ() );
    
    PD_ASSERT( CheckArc(A_0.Idx()) );
    PD_ASSERT( CheckArc(B_0.Idx()) );
    PD_ASSERT( CheckArc(A_3.Idx()) );
    PD_ASSERT( CheckArc(B_3.Idx()) );
    
    PD_ASSERT( OppositeHandednessQ(C_1,C_3) );
    
    PD_ASSERT( A_0(Head) == C_3 );
    PD_ASSERT( B_0(Head) == C_1 );

    PD_ASSERT( A_3(Tail) == C_3 );
    PD_ASSERT( B_3(Tail) == C_1 );
    
    PD_ASSERT( CheckArc( C_1(Out,Left ) ) );
    PD_ASSERT( CheckArc( C_1(Out,Right) ) );
    PD_ASSERT( CheckArc( C_1(In ,Left ) ) );
    PD_ASSERT( CheckArc( C_1(In ,Right) ) );
    
    PD_ASSERT( CheckArc( C_3(Out,Left ) ) );
    PD_ASSERT( CheckArc( C_3(Out,Right) ) );
    PD_ASSERT( CheckArc( C_3(In ,Left ) ) );
    PD_ASSERT( CheckArc( C_3(In ,Right) ) );
    
    PD_ASSERT( CheckCrossing( A_0(Head) ) );
    PD_ASSERT( CheckCrossing( A_0(Tail) ) );
    
    PD_ASSERT( CheckCrossing( B_0(Head) ) );
    PD_ASSERT( CheckCrossing( B_0(Tail) ) );
    
    PD_ASSERT( CheckCrossing( A_3(Head) ) );
    PD_ASSERT( CheckCrossing( A_3(Tail) ) );
    
    PD_ASSERT( CheckCrossing( B_3(Head) ) );
    PD_ASSERT( CheckCrossing( B_3(Tail) ) );
    
    ++R_IIa_counter;
    
    return true;
}

// Done:
//            |
//      \   +-|-+   /
//       \ /  |  \ /
//        /   |   \
//       / \  |  / \
//      /   +-|-+   \
//            |
//
// Done:
//            |
//      \   +---+   /
//       \ /  |  \ /
//        /   |   \
//       / \  |  / \
//      /   +---+   \
//            |
//
// TODO: Do also this case:
//            |
//      \   +---+   /
//       \ /  |  \ /
//        /   |   \
//       / \  |  / \
//      /   +-|-+   \
//            |
