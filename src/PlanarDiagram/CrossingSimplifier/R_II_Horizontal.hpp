private:

void Reidemeister_II_Horizontal( const Int c_0, const Int c_1, const bool side )
{
    PD_PRINT("\n" + ClassName()+"::Reidemeister_II_Horizontal( c_0 = "+CrossingString(c_0)+", c_1 = "+CrossingString(c_1)+", "+ ToString(side) +" )");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT(c_0 != c_1);
    
    auto C_0 = GetCrossing(c_0);
    auto C_1 = GetCrossing(c_1);
    PD_ASSERT(OppositeHandednessQ(C_0,C_1));

    
#ifdef PD_DEBUG
    auto A_0 = GetArc(C_0(In ,side) );
    auto A_1 = GetArc(C_1(Out,side) );
    
    auto B_0 = GetArc(C_0(Out,side) );
    auto B_1 = GetArc(C_1(In ,side) );
    
    PD_ASSERT(pd.CheckArc(A_0.Idx()));
    PD_ASSERT(pd.CheckArc(A_1.Idx()));
    PD_ASSERT(pd.CheckArc(B_0.Idx()));
    PD_ASSERT(pd.CheckArc(B_1.Idx()));
        
    
    PD_ASSERT( A_0 == A_1 );
    PD_ASSERT( B_0 == B_1 );
    // These are the central assumptions here.
    if(!( (A_0 == A_1) && (B_0 == B_1) ))
    {
        PD_VALPRINT("C_0",C_0);
        PD_VALPRINT("C_1",C_1);
        
        PD_VALPRINT("A_0",A_0);
        PD_VALPRINT("A_1",A_1);
        PD_VALPRINT("B_0",B_0);
        PD_VALPRINT("B_1",B_1);
    }
#endif
    
    
    
/* We assume this horizontal alignment in the case of side==Right.
 *
 *              C_0(Out,side) = B = C_1(In ,side)
 *
 *       O----<----O       O---->----O       O----<----O
 *           E_3    ^     ^     B     \     /    E_2
 *                   \   /             \   /
 *                    \ /               \ /
 *                 C_0 X                 X C_1
 *                    / \               / \
 *                   /   \             /   \
 *           E_0    /     \     A     v     v    E_1
 *       O---->----O       O----<----O       O---->----O
 *
 *              C_0(In ,side) = A = C_0(Out,side)
 *
 * In the case side == Left, we just flip everything around.
 */
    
    auto A   = GetArc(C_0(In , side) );
    auto B   = GetArc(C_0(Out, side) );
    auto E_0 = GetArc(C_0(In ,!side) );
    auto E_1 = GetArc(C_1(Out,!side) );
    auto E_2 = GetArc(C_1(In ,!side) );
    auto E_3 = GetArc(C_0(Out,!side) );
    
    if( (E_0 == E_1) || (E_3 == E_2) )
    {
        eprint(ClassName()+"::Reidemeister_II_Horizontal: We should not arrive here because this is a vertical case.");
    }
    
//    if( E_0 == E_1 )
//    {
//        if( E_2 != E_3 )
//        {
    
/* This special situation with a loop over or under a strand (in the case of side==Right):
 *
 *               v_3 O----<----O       O---->----O       O----<----O v_2
 *                       E_3    ^     ^     B     \     /    E_2
 *                               \   /             \   /
 *                                \ /               \ /
 *           v_2 != v_3        C_0 X                 X C_1
 *                                / \               / \
 *                               /   \             /   \
 *                              /     \     A     v     v
 *                             O       O----<----O       O
 *                              \                       /
 *                               \      E_0 = E_1      /
 *                                +---------<---------+
 */

//
//            // We should not arrive here because this is a vertical case.
//            PD_ASSERT(false);
//            wprint("We should not arrive here because this is a vertical case. (1)");
//
//            unlink_count++;
//
//            PD_PRINT("\tUnlink detectd.");
//
//            Reconnect<Tail>(A,E_2);
//            Reconnect<Head>(A,E_3);
//
//            DeactivateArc(B.Idx());
//            DeactivateArc(E_0.Idx());
//
//            goto Exit;
//        }
//        else
//        {
    
/* This very,very special situation with two noninterlinked loops (in the case of side==Right):
 *
 *                                +--------->---------+
 *                               /     E_3 = E_2       \
 *                              /                       \
 *                             O       O---->----O       O
 *                              ^     ^     B     \     /
 *                               \   /             \   /
 *                                \ /               \ /
 *                             C_0 X                 X C_1
 *                                / \               / \
 *                               /   \             /   \
 *                              /     \     A     v     v
 *                             O       O----<----O       O
 *                              \                       /
 *                               \      E_0 = E_1      /
 *                                +---------<---------+
*/
    

//            // We should not arrive here because this is a vertical case.
//            PD_ASSERT(false);
//            wprint("We should not arrive here because this is a vertical case. (2)");
//
//            PD_PRINT("\tTwo unlinks detectd.");
//
//            unlink_count += 2;
//
//            DeactivateArc(A.Idx());
//            DeactivateArc(B.Idx());
//            DeactivateArc(E_0.Idx());
//            DeactivateArc(E_2.Idx());
//
//            goto Exit;
//        }
//    }
//    else if( E_3 == E_2 )
//    {
//        // We don't have to check the case E_0 = E_1 here.
//
    
/*                                +--------->---------+
 *                               /      E_3 = E_2      \
 *                              /                       \
 *                             O       O---->----O       O
 *                       E_3    ^     ^     B     \     /    E_2
 *                               \   /             \   /
 *                                \ /               \ /
 *         v_0 != v_1          C_0 X                 X c_1
 *                                / \               / \
 *                               /   \             /   \
 *                       E_0    /     \     A     v     v    E_1
 *               v_0 O---->----O       O----<----O       O---->----O v_1
 */
    
//
//        // We should not arrive here because this is a vertical case.
//        PD_ASSERT(false);
//
//        wprint("We should not arrive here because this is a vertical case. (3)");
//
//        unlink_count++;
//
//        PD_PRINT("\tUnlink detectd.");
//
//        Reconnect<Tail>(B,E_0);
//        Reconnect<Head>(B,E_1);
//
//        DeactivateArc(A.Idx());
//        DeactivateArc(E_2.Idx());
//
//        goto Exit;
//    }
    
    if( (E_0 == E_3) || (E_1 == E_2) )
    {
        eprint(ClassName()+"::Reidemeister_II_Horizontal: We should not arrive here because we first check for a Reidemeister_I.");
    }
    
//    if( E_0 == E_3 )
//    {
//        if( E_1 != E_2 )
//        {
    
/* This special case:
 *                   +----<----O       O---->----O       O----<----O v_2
 *                  /    E_3    ^     ^     B     \     /    E_2
 *                 /             \   /             \   /
 *         E_3    /               \ /               \ /
 *          =    |             C_0 X                 X C_1        v_1 != v_2
 *         E_0    \               / \               / \
 *                 \             /   \             /   \
 *                  \    E_0    /     \     A     v     v    E_1
 *                   +---->----O       O----<----O       O---->----O v_1
 */

//
//            // We should not arrive here because we first check for a Reidemeister_I at c_0.
//            PD_ASSERT( false );
//
//            wprint("We should not arrive here because we first check for a Reidemeister_I at c_0.");
//
//            Reconnect<Tail>(E_0,E_2);
//            Reconnect<Head>(E_0,E_1);
//
//            DeactivateArc(A);
//            DeactivateArc(B);
//
//            PD_PRINT("\t\tModified move 1.");
//
//            goto Exit;
//        }
//        else
//        {
    
/* This is a very, very special case:
 *                   +----<----O       O---->----O       O----<----+
 *                  /    E_3    ^     ^     B     \     /           \
 *                 /             \   /             \   /             \
 *         E_3    /               \ /               \ /               \    E_2
 *          =    |             C_0 X                 X C_1             |    =
 *         E_0    \               / \               / \               /    E_1
 *                 \             /   \             /   \             /
 *                  \    E_0    /     \     A     v     v           /
 *                   +---->----O       O----<----O       O---->----+
 */

//
//            //  We should not arrive here because we first check for a Reidemeister_I at C_0.
//            PD_ASSERT( false );
//            wprint("We should not arrive here because we first check for a Reidemeister_I at C_0. (2)");
//
//            ++pd.unlink_count;
//
//            PD_PRINT("\t\tUnlink detected.");
//
//            DeactivateArc(A.Idx());
//            DeactivateArc(B.Idx());
//            DeactivateArc(E_0.Idx());
//            DeactivateArc(E_1.Idx());
//
//            goto Exit;
//        }
//    }
//    else if( E_1 == E_2 )
//    {
//        // We do not have to check for E_0 == E_3 anymore.
//
    
/* This special case:
 *               v_3 O----<----O       O---->----O       O----<----+
 *                       E_3    ^     ^     B     \     /    E_2    \
 *                               \   /             \   /             \
 *                                \ /               \ /               \    E_2
 *         v_3 != v_0          C_0 X                 X C_1             |    =
 *                                / \               / \               /    E_1
 *                               /   \             /   \             /
 *                       E_0    /     \     A     v     v    E_1    /
 *               v_0 O---->----O       O----<----O       O---->----O
 */

//        //  We should not arrive here because we first check for a Reidemeister_I at C_1.
//        PD_ASSERT( false );
//        wprint("We should not arrive here because we first check for a Reidemeister_I at C_0. (3)");
//
//
//        Reconnect<Tail>(E_1,E_0);
//        Reconnect<Head>(E_1,E_3);
//
//        DeactivateArc(A.Idx());
//        DeactivateArc(B.Idx());
//
//        PD_PRINT("\t\tModified move 2.");
//
//        goto Exit;
//    }

    {
/* Finally, the most common case/
 * This is for side == Right. In the other case (side == Left), we just flip everything around.
 *
 *                         C_0(Out,Right) = B = C_1(In ,Right)
 *
 *               v_3 O----<----O       O---->----O       O----<----O v_2
 *                       E_3    ^     ^     B     \     /    E_2
 *                               \   /             \   /
 *                                \ /               \ /
 *                             C_0 X                 X C_1
 *                                / \               / \
 *                               /   \             /   \
 *                       E_0    /     \     A     v     v    E_1
 *               v_0 O---->----O       O----<----O       O---->----O v_1
 *
 *                         C_0(In ,Right) = A = C_1(Out,Right)
 *
 *
 * State after the move:
 *
 *                     +----<---------------<---------------<----+
 *                    /                     A                     \
 *                   O         O       O         O       O         O
 *                              ^     ^           \     /
 *                               \   /             \   /
 *                                \ /               \ /
 *                             C_0 X                 X C_1
 *                                / \               / \
 *                               /   \             /   \
 *                              /     \           v     v
 *                   O         O       O         O       O         O
 *                    \                     B                     /
 *                     +---->--------------->--------------->----+
 */
 
        PD_PRINT(std::string("\t\tGeneric case (") + ((side==Left) ? "left)" : "right)") );
        Reconnect<Tail>(A,E_2);
        Reconnect<Head>(A,E_3);
        Reconnect<Tail>(B,E_0);
        Reconnect<Head>(B,E_1);
    
        goto Exit;
    }
    
Exit:
    
    // The two crossings are inactivated in any case.
    DeactivateCrossing(C_0.Idx());
    DeactivateCrossing(C_1.Idx());
    
    ++pd.R_II_counter;

} // Reidemeister_II_Horizontal
