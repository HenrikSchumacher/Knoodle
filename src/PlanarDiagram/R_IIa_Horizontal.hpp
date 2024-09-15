private:

bool Reidemeister_IIa_Horizontal( const Int c_0 )
{
    if( !CrossingActiveQ(c_0) )
    {
        return false;
    }
    
    PD_PRINT("Reidemeister_IIa_Horizontal");
    
    
    PD_ASSERT( (0 <= c_0) && (c_0 < C_arcs.Dimension(0)) );
    
    const Int e_1 = C_arcs(c_0,Out,Left );
    const Int f_3 = C_arcs(c_0,Out,Right);
    const Int f_2 = C_arcs(c_0,In ,Left );
    const Int e_0 = C_arcs(c_0,In ,Right);

    PD_ASSERT(ArcActiveQ(e_1));
    PD_ASSERT(ArcActiveQ(f_3));
    PD_ASSERT(ArcActiveQ(f_2));
    PD_ASSERT(ArcActiveQ(e_0));
    
    PD_ASSERT(CheckArc(e_1));
    PD_ASSERT(CheckArc(f_3));
    PD_ASSERT(CheckArc(f_2));
    PD_ASSERT(CheckArc(e_0));
    
    
    const Int c_1 = A_cross(e_1,Head);
    const Int e_2 = NextArc(e_1);
    const Int c_2 = A_cross(e_2,Head);
    
    PD_ASSERT(CrossingActiveQ(c_1));
    PD_ASSERT(CrossingActiveQ(c_2));
    
    PD_ASSERT(CheckCrossing(c_1));
    PD_ASSERT(CheckCrossing(c_2));
    
    PD_ASSERT(ArcActiveQ(e_2));
    PD_ASSERT(CheckArc(e_2));
    
    PD_ASSERT( (0 <= c_1) && (c_1 < C_arcs.Dimension(0)) );
    PD_ASSERT( (0 <= c_2) && (c_2 < C_arcs.Dimension(0)) );
    
    if( SameHandednessQ(c_0,c_2) )
    {
        // Not what we are looking for.
        return false;
    }
    
    if( (e_2 != C_arcs(c_2,In,Left))  )
    {
        // Rather look for twist move.
        return false;
    }
    
    const Int f_1 = C_arcs(c_2,Out,Left );
    const Int e_3 = C_arcs(c_2,Out,Right);
    const Int f_0 = C_arcs(c_2,In ,Right);
    
    PD_ASSERT(ArcActiveQ(f_1));
    PD_ASSERT(ArcActiveQ(e_3));
    PD_ASSERT(ArcActiveQ(f_0));
    
    PD_ASSERT(CheckArc(f_1));
    PD_ASSERT(CheckArc(e_3));
    PD_ASSERT(CheckArc(f_0));
    
    const Int c_3 = A_cross(f_1,Head);
    
    if( c_1 == c_3  )
    {
        // Not what we are looking for.
        return false;
    }
    
    if( SameHandednessQ(c_1,c_3) )
    {
        return false;
    }
    
    if( (c_3 != A_cross(f_2,Tail))  )
    {
        // Not what we are looking for.
        return false;
    }
    
//     c_0 Righthanded
//                             O
//                             |
//                             |c_1
//                       O<----X-----O
//                      /      |      ^
//                 e_2 /       |       \ e_1
//                    v        O        \
//       f_0 O       O         |         O       O f_3
//            \     /          |          ^     ^
//             \   /           |           \   /
//              \ /            |            \ /
//               \ c_2         |             / c_0
//              / \            |            / \
//             /   \           |           /   \
//            v     v          |          /     \
//       e_3 O       O         |         O       O e_0
//                    \        O        ^
//                 f_1 \       |       / f_2
//                      v      |c_3   /
//                       O-----X---->O
//                             |
//                             |
//                             O
    
    
    PD_ASSERT( OppositeHandednessQ(c_0,c_2) );
    
    PD_ASSERT( C_arcs(c_0,Out,Left ) == e_1 );
    PD_ASSERT( C_arcs(c_0,Out,Right) == f_3 );
    PD_ASSERT( C_arcs(c_0,In ,Left ) == f_2 );
    PD_ASSERT( C_arcs(c_0,In ,Right) == e_0 );
    
    
    PD_ASSERT( C_arcs(c_2,Out,Left ) == f_1 );
    PD_ASSERT( C_arcs(c_2,Out,Right) == e_3 );
    PD_ASSERT( C_arcs(c_2,In ,Left ) == e_2 );
    PD_ASSERT( C_arcs(c_2,In ,Right) == f_0 );
    
    
    PD_ASSERT( A_cross(e_0,Head) == c_0 );
    PD_ASSERT( A_cross(f_0,Head) == c_2 );
    
    PD_ASSERT( A_cross(e_1,Head) == c_1 );
    PD_ASSERT( A_cross(e_2,Tail) == c_1 );
    
    PD_ASSERT( A_cross(f_1,Head) == c_3 );
    PD_ASSERT( A_cross(f_2,Tail) == c_3 );
    
    PD_ASSERT( A_cross(e_3,Tail) == c_2 );
    PD_ASSERT( A_cross(f_3,Tail) == c_0 );
    
    
    PD_ASSERT( OppositeHandednessQ(c_1,c_3) );
    
    PD_PRINT("Incoming data");
    
    PD_VALPRINT("c_0",CrossingString(c_0));
    PD_VALPRINT("c_1",CrossingString(c_1));
    PD_VALPRINT("c_2",CrossingString(c_2));
    PD_VALPRINT("c_3",CrossingString(c_3));
    
    PD_VALPRINT("e_0",ArcString(e_0));
    PD_VALPRINT("e_1",ArcString(e_1));
    PD_VALPRINT("e_2",ArcString(e_2));
    PD_VALPRINT("e_3",ArcString(e_3));
    
    PD_VALPRINT("f_0",ArcString(f_0));
    PD_VALPRINT("f_1",ArcString(f_1));
    PD_VALPRINT("f_2",ArcString(f_2));
    PD_VALPRINT("f_3",ArcString(f_3));
    
    
    
    
    //
    //                         O
    //                         |
    //        f_0              |c_1           f_3
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
    //          \              |c_3           /
    //           +-------O<----X-----O<------+
    //        e_3              |              e_0
    //                         |
    //                         O


    // Reconnect c_1.
    const bool side_1 = (C_arcs(c_1,In,Right) == e_1) ? Right : Left;
    C_arcs(c_1,Out,!side_1) = C_arcs(c_1,Out, side_1);
    C_arcs(c_1,In , side_1) = C_arcs(c_1,In ,!side_1);
    C_arcs(c_1,Out, side_1) = f_3;
    C_arcs(c_1,In ,!side_1) = f_0;
    FlipHandedness(c_1);
    
// For details see the following ASCII art:
//
//    if( C_arcs(c_1,In,Right) == e_1 )
//    {
////
////                         O
////                         ^
////                         |c_1
////                   O<----X-----O
////                  /      |      ^
////             e_2 /       |       \ e_1
////                v        O        \
////   f_0 O       O         |         O       O f_3
////        \     /          |          ^     ^
////
//// This is what we want to get:
////
////                         O
////                         ^
////        f_0              |c_1           f_3
////           +------>O-----X---->O-------+
////          /              |              \
////         /               |               \
////        /                O                v
////       O                 |                 O
//        
//        C_arcs(c_1,Out,Left ) = C_arcs(c_1,Out,Right);
//        C_arcs(c_1,In ,Right) = C_arcs(c_1,In ,Left );
//        
//        C_arcs(c_1,Out,Right) = f_3;
//        C_arcs(c_1,In ,Left ) = f_0;
//        
//        FlipHandedness(c_1);
//    }
//    else // if( C_arcs(c_1,In,Left) == e_1 )
//    {
////
////                         O
////                         |
////                         |c_1
////                   O<----X-----O
////                  /      |      ^
////             e_2 /       v       \ e_1
////                v        O        \
////   f_0 O       O         |         O       O f_3
////        \     /          |          ^     ^
////
//// This is what we want to get:
////
////                         O
////                         |
////        f_0              |c_1           f_3
////           +------>O-----X---->O-------+
////          /              |              \
////         /               V               \
////        /                O                v
////       O                 |                 O
//        
//        C_arcs(c_1,Out,Right) = C_arcs(c_1,Out,Left );
//        C_arcs(c_1,In ,Left ) = C_arcs(c_1,In ,Right);
//        
//        C_arcs(c_1,Out,Left ) = f_3;
//        C_arcs(c_1,In ,Right) = f_0;
//        
//        FlipHandedness(c_1);
//    }
    
    
    
//    // Reconnect c_3.
    const bool side_3 = (C_arcs(c_3,In,Right) == f_1) ? Right : Left;
    C_arcs(c_3,Out,!side_3) = C_arcs(c_3,Out, side_3);
    C_arcs(c_3,In , side_3) = C_arcs(c_3,In ,!side_3);
    C_arcs(c_3,Out, side_3) = e_3;
    C_arcs(c_3,In ,!side_3) = e_0;
    FlipHandedness(c_3);
    
// For details see the following ASCII art:
    
//    if( C_arcs(c_3,In,Right) == f_1 )
//    {
////        v     v          |          /     \
////   e_3 O       O         |         O       O e_0
////                \        O        ^
////             f_1 \       |       / f_2
////                  v      |c_3   /
////                   O-----X---->O
////                         |
////                         v
////                         O
////        
//// This is what we want to get:
////
////       O                 |                 O
////        ^                O                /
////         \               |               /
////          \              |c_3           /
////           +-------O<----X-----O<------+
////        e_3              |              e_0
////                         v
////                         O
//
//        C_arcs(c_3,Out,Left ) = C_arcs(c_3,Out,Right);
//        C_arcs(c_3,In ,Right) = C_arcs(c_3,In ,Left );
//        C_arcs(c_3,Out,Right) = e_3;
//        C_arcs(c_3,In ,Left ) = e_0;
//        FlipHandedness(c_3);
//    }
//    else // if if( C_arcs(c_3,In,Left) == f_1 )
//    {
////        v     v          |          /     \
////   e_3 O       O         |         O       O e_0
////                \        O        ^
////             f_1 \       ^       / f_2
////                  v      |c_3   /
////                   O-----X---->O
////                         |
////                         |
////                         O
////
//// This is what we want to get:
////
////       O                 |                 O
////        ^                O                /
////         \               ^               /
////          \              |c_3           /
////           +-------O<----X-----O<------+
////        e_3              |              e_0
////                         |
////                         O
//        
//        C_arcs(c_3,Out,Right) = C_arcs(c_3,Out,Left );
//        C_arcs(c_3,In ,Left ) = C_arcs(c_3,In ,Right);
//        C_arcs(c_3,Out,Left ) = e_3;
//        C_arcs(c_3,In ,Right) = e_0;
//        FlipHandedness(c_3);
//    }

    
    A_cross(e_0,Head) = c_3;
    A_cross(e_3,Tail) = c_3;
    
    A_cross(f_0,Head) = c_1;
    A_cross(f_3,Tail) = c_1;
    
    DeactivateArc(e_1);
    DeactivateArc(e_2);
    DeactivateArc(f_1);
    DeactivateArc(f_2);
    DeactivateCrossing(c_0);
    DeactivateCrossing(c_2);
 
    
    PD_PRINT("Changed data");
    
    PD_VALPRINT("c_1",CrossingString(c_1));
    PD_VALPRINT("c_3",CrossingString(c_3));
    
    PD_VALPRINT("e_0",ArcString(e_0));
    PD_VALPRINT("e_3",ArcString(e_3));
    
    PD_VALPRINT("f_0",ArcString(f_0));
    PD_VALPRINT("f_3",ArcString(f_3));
    
    PD_ASSERT(!CrossingActiveQ(c_0) );
    PD_ASSERT(!CrossingActiveQ(c_2) );
    PD_ASSERT( CrossingActiveQ(c_1) );
    PD_ASSERT( CrossingActiveQ(c_3) );
    
    PD_ASSERT( CheckCrossing(c_1) );
    PD_ASSERT( CheckCrossing(c_3) );
    
    
    PD_ASSERT(!ArcActiveQ(e_1) );
    PD_ASSERT(!ArcActiveQ(f_1) );
    PD_ASSERT(!ArcActiveQ(e_2) );
    PD_ASSERT(!ArcActiveQ(f_2) );
    PD_ASSERT( ArcActiveQ(e_0) );
    PD_ASSERT( ArcActiveQ(f_0) );
    PD_ASSERT( ArcActiveQ(e_3) );
    PD_ASSERT( ArcActiveQ(f_3) );
    
    PD_ASSERT( CheckArc(e_0) );
    PD_ASSERT( CheckArc(f_0) );
    PD_ASSERT( CheckArc(e_3) );
    PD_ASSERT( CheckArc(f_3) );
    
    PD_ASSERT( OppositeHandednessQ(c_1,c_3) );
    
    PD_ASSERT( A_cross(e_0,Head) == c_3 );
    PD_ASSERT( A_cross(f_0,Head) == c_1 );

    PD_ASSERT( A_cross(e_3,Tail) == c_3 );
    PD_ASSERT( A_cross(f_3,Tail) == c_1 );
    
    
    PD_ASSERT( CheckArc(C_arcs(c_1,Out,Left ) ) );
    PD_ASSERT( CheckArc(C_arcs(c_1,Out,Right) ) );
    PD_ASSERT( CheckArc(C_arcs(c_1,In ,Left ) ) );
    PD_ASSERT( CheckArc(C_arcs(c_1,In ,Right) ) );
    
    PD_ASSERT( CheckArc(C_arcs(c_3,Out,Left ) ) );
    PD_ASSERT( CheckArc(C_arcs(c_3,Out,Right) ) );
    PD_ASSERT( CheckArc(C_arcs(c_3,In ,Left ) ) );
    PD_ASSERT( CheckArc(C_arcs(c_3,In ,Right) ) );
    
    
    PD_ASSERT( CheckCrossing(A_cross(e_0,Head) ) );
    PD_ASSERT( CheckCrossing(A_cross(e_0,Tail) ) );
    
    PD_ASSERT( CheckCrossing(A_cross(f_0,Head) ) );
    PD_ASSERT( CheckCrossing(A_cross(f_0,Tail) ) );
    
    PD_ASSERT( CheckCrossing(A_cross(e_3,Head) ) );
    PD_ASSERT( CheckCrossing(A_cross(e_3,Tail) ) );
    
    PD_ASSERT( CheckCrossing(A_cross(f_3,Head) ) );
    PD_ASSERT( CheckCrossing(A_cross(f_3,Tail) ) );
    
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
