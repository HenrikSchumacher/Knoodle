private:

void Reidemeister_II_Vertical( const Int c_0, const Int c_1 )
{
    PD_PRINT("\n" + ClassName() + "::Reidemeister_II_Vertical  ( "+CrossingString(c_0)+", "+CrossingString(c_1)+" )");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT(c_0 != c_1);
    
    auto C_0 = GetCrossing(c_0);
    auto C_1 = GetCrossing(c_1);
    
    PD_ASSERT(OppositeHandednessQ(C_0,C_1));
    
    // This is the central assumption here. (C_0 is the bottom crossing.)
    PD_ASSERT(C_0(Out,Left ) == C_1(In,Left ));
    PD_ASSERT(C_0(Out,Right) == C_1(In,Right));
    
    auto A   = GetArc(C_0(Out,Left ) );
    auto B   = GetArc(C_0(Out,Right) );
    auto E_0 = GetArc(C_0(In ,Left ) );
    auto E_1 = GetArc(C_0(In ,Right) );
    
    auto E_3 = GetArc(C_1(Out,Left ) );
    auto E_2 = GetArc(C_1(Out,Right) );
    
    PD_ASSERT(E_0 != E_2); // Should be impossible because of topology.
    PD_ASSERT(E_1 != E_3); // Should be impossible because of topology.
    
    if(E_0 != E_3)
    {
        if(E_1 != E_2)
        {
// The generic case.

/*  E_0 != E_3 && E_1 != E_2
//
//
//                    O----<----O       O---->----O
//                        E_3    ^     ^    E_2
//                                \   /
//                                 \ /
//                              C_1 X
//                                 / \
//                                /   \
//                               /     \
//            C_1(In ,Left )    O       O    C_1(In ,Right)
//                          \\  ^       ^  //
//                            A |       | B
//                          //  |       |  \\
//            C_0(Out,Left )    O       O    C_0(Out,Right)
//                               ^     ^
//                                \   /
//                                 \ /
//                              C_0 X
//                                 / \
//                                /   \
//                        E_0    /     \    E_1
//                    O---->----O       O----<----O
*/
            Reconnect<Tail>(A,E_0);
            Reconnect<Head>(A,E_3);
            Reconnect<Tail>(B,E_1);
            Reconnect<Head>(B,E_2);
            
            goto Exit;
        }
        else
        {
/*  E_0 != E_3 && E_1 == E_2
//
//
//            O----<----O       O---->----+
//                E_3    ^     ^           \
//                        \   /             \
//                         \ /               \
//                      C_1 X                 |
//                         / \                |
//                        /   \               |
//                       /     \              |
//                      O       O             |
//                      ^       ^        E_2  |
//                    A |       | B       =   |
//                      |       |        E_1  |
//                      O       O             |
//                       ^     ^              |
//                        \   /               |
//                         \ /                |
//                      C_0 X                 |
//                         / \               /
//                        /   \             /
//                E_0    /     \           /
//            O---->----O       O----<----+
*/
        }
        
        Reconnect<Tail>(B,E_0);
        Reconnect<Head>(B,E_3);
        
        ++pd.unlink_count;
        
        DeactivateArc(A.Idx());
        DeactivateArc(E_1.Idx());
        
        goto Exit;
    }
    else if (E_1 != E_2)
    {
/*  E_0 == E_3 && E_1 != E_2
//
//
//            +----<----O       O---->----O
//           /           ^     ^    E_2
//          /             \   /
//         /               \ /
//        |             C_1 X
//        |                / \
//        |               /   \
//        |              /     \
//        |             O       O
//        | E_1         ^       ^
//        |  =        A |       | B
//        | E_0         |       |
//        |             O       O
//        |              ^     ^
//        |               \   /
//        |                \ /
//        |             C_0 X
//         \               / \
//          \             /   \
//           \           /     \    E_1
//            +---->----O       O----<----O
*/
        Reconnect<Tail>(A,E_1);
        Reconnect<Head>(A,E_2);
        
        ++pd.unlink_count;
        
        DeactivateArc(B.Idx());
        
        goto Exit;
    }
    else
    {
/*  E_0 == E_3 && E_1 == E_2
//
//
//            +----<----O       O---->----+
//           /           ^     ^           \
//          /             \   /             \
//         /               \ /               \
//        |             C_1 X                 |
//        |                / \                |
//        |               /   \               |
//        |              /     \              |
//        |             O       O             |
//        | E_1         ^       ^        E_2  |
//        |  =        A |       | B       =   |
//        | E_0         |       |        E_1  |
//        |             O       O             |
//        |              ^     ^              |
//        |               \   /               |
//        |                \ /                |
//        |             C_0 X                 |
//         \               / \               /
//          \             /   \             /
//           \           /     \           /
//            +---->----O       O----<----+
*/
        pd.unlink_count += Int(2);
        
        DeactivateArc(A.Idx());
        DeactivateArc(B.Idx());
        DeactivateArc(E_1.Idx());
        DeactivateArc(E_2.Idx());
        
        goto Exit;
    }
    
Exit:
    
    // The two crossings are inactivated in any case.
    
    DeactivateCrossing(C_0.Idx());
    DeactivateCrossing(C_1.Idx());
    
    ++pd.R_II_counter;

} // Reidemeister_II_Vertical
