private:

void Reidemeister_II_Horizontal( const Int c_0, const Int c_1, const bool side )
{
    PD_PRINT("\nReidemeister_II_Horizontal( c_0 = "+CrossingString(c_0)+", c_1 = "+CrossingString(c_1)+", "+ ToString(side) +" )");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT(c_0 != c_1);
    PD_ASSERT(OppositeHandednessQ(c_0,c_1));
    
    PD_ASSERT(CheckCrossing(c_0));
    PD_ASSERT(CheckCrossing(c_1));
    
#ifdef PD_DEBUG
    const Int a_0 = C_arcs(c_0,In ,side);
    const Int a_1 = C_arcs(c_1,Out,side);
    
    const Int b_0 = C_arcs(c_0,Out,side);
    const Int b_1 = C_arcs(c_1,In ,side);
    
    PD_ASSERT(CheckArc(a_0));
    PD_ASSERT(CheckArc(a_1));
    PD_ASSERT(CheckArc(b_0));
    PD_ASSERT(CheckArc(b_1));
        
    
    PD_ASSERT( a_0 == a_1 );
    PD_ASSERT( b_0 == b_1 );
    // These are the central assumptions here.
    if(!( (a_0 == a_1) && (b_0 == b_1) ))
    {
        PD_VALPRINT("C_state[c_0]",ToUnderlying(C_state[c_0]));
        PD_VALPRINT("C_state[c_1]",ToUnderlying(C_state[c_1]));
        PD_PRINT(CrossingString(c_0));
        PD_PRINT(CrossingString(c_1));
        
        PD_PRINT(ArcString(a_0));
        PD_PRINT(ArcString(a_1));
        PD_PRINT(ArcString(b_0));
        PD_PRINT(ArcString(b_1));
    }
#endif
    
    
    
// We assume this horizontal alignment in the case of side==Right.
//
//                   C_arcs(c_0,Out,side) = b = C_arcs(c_1,In ,side)
//
//               v_3 O----<----O       O---->----O       O----<----O v_2
//                       e_3    ^     ^     b     \     /    e_2
//                               \   /             \   /
//                                \ /               \ /
//                             c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                       e_0    /     \     a     v     v    e_1
//               v_0 O---->----O       O----<----O       O---->----O v_1
//
//                   C_arcs(c_0,In ,side) = a = C_arcs(c_0,Out,side)
//
// In the case side == Left, we just flip everything around.
    
    
    const Int a = C_arcs(c_0,In ,side);
    const Int b = C_arcs(c_0,Out,side);
    
    const Int e_0 = C_arcs(c_0,In ,!side);
    const Int e_1 = C_arcs(c_1,Out,!side);
    const Int e_2 = C_arcs(c_1,In ,!side);
    const Int e_3 = C_arcs(c_0,Out,!side);
    
    if( (e_0 == e_1) || (e_3 == e_2) )
    {
        eprint(ClassName()+"::Reidemeister_II_Horizontal: We should not arrive here because this is a vertical case.");
    }
    
//    if( e_0 == e_1 )
//    {
//        if( e_2 != e_3 )
//        {
//// This special situation with a loop over or under a strand (in the case of side==Right):
////
////               v_3 O----<----O       O---->----O       O----<----O v_2
////                       e_3    ^     ^     b     \     /    e_2
////                               \   /             \   /
////                                \ /               \ /
////           v_2 != v_3        c_0 X                 X c_1
////                                / \               / \
////                               /   \             /   \
////                              /     \     a     v     v
////                             O       O----<----O       O
////                              \                       /
////                               \      e_0 = e_1      /
////                                +---------<---------+
//            
//            // We should not arrive here because this is a vertical case.
////            PD_ASSERT(false);
//            wprint("We should not arrive here because this is a vertical case. (1)");
//            
//            unlink_count++;
//            
//            PD_PRINT("\tUnlink detectd.");
//            
//            Reconnect(a,Tail,e_2);
//            Reconnect(a,Tip ,e_3);
//            
//            DeactivateArc(b);
//            DeactivateArc(e_0);
//            
//            goto exit;
//        }
//        else
//        {
//// This very,very special situation with two noninterlinked loops (in the case of side==Right):
////
////                                +--------->---------+
////                               /     e_3 = e_2       \
////                              /                       \
////                             O       O---->----O       O
////                              ^     ^     b     \     /
////                               \   /             \   /
////                                \ /               \ /
////                             c_0 X                 X c_1
////                                / \               / \
////                               /   \             /   \
////                              /     \     a     v     v
////                             O       O----<----O       O
////                              \                       /
////                               \      e_0 = e_1      /
////                                +---------<---------+
//            
//            // We should not arrive here because this is a vertical case.
////            PD_ASSERT(false);
//            wprint("We should not arrive here because this is a vertical case. (2)");
//            
//            PD_PRINT("\tTwo unlinks detectd.");
//            
//            unlink_count += 2;
//            
//            DeactivateArc(a);
//            DeactivateArc(b);
//            DeactivateArc(e_0);
//            DeactivateArc(e_2);
//            
//            goto exit;
//        }
//    }
//    else if( e_3 == e_2 )
//    {
//        // We don't have to check the case e_0 = e_1 here.
//
////                                +--------->---------+
////                               /     e_3 = e_2       \
////                              /                       \
////                             O       O---->----O       O
////                       e_3    ^     ^     b     \     /    e_2
////                               \   /             \   /
////                                \ /               \ /
////         v_0 != v_1          c_0 X                 X c_1
////                                / \               / \
////                               /   \             /   \
////                       e_0    /     \     a     v     v    e_1
////               v_0 O---->----O       O----<----O       O---->----O v_1
//    
//        // We should not arrive here because this is a vertical case.
////        PD_ASSERT(false);
//        
//        wprint("We should not arrive here because this is a vertical case. (3)");
//        
//        unlink_count++;
//        
//        PD_PRINT("\tUnlink detectd.");
//        
//        Reconnect(b,Tail,e_0);
//        Reconnect(b,Tip ,e_1);
//        
//        DeactivateArc(a);
//        DeactivateArc(e_2);
//        
//        goto exit;
//    }
    
    if( (e_0 == e_3) || ( e_1 == e_2 ) )
    {
        eprint(ClassName()+"::Reidemeister_II_Horizontal: We should not arrive here because we first check for a Reidemeister_I.");
    }
    
//    if( e_0 == e_3 )
//    {
//        if( e_1 != e_2 )
//        {
//// This special case:
////                   +----<----O       O---->----O       O----<----O v_2
////                  /    e_3    ^     ^     b     \     /    e_2
////                 /             \   /             \   /
////         e_3    /               \ /               \ /
////          =    |             c_0 X                 X c_1        v_1 != v_2
////         e_0    \               / \               / \
////                 \             /   \             /   \
////                  \    e_0    /     \     a     v     v    e_1
////                   +---->----O       O----<----O       O---->----O v_1
////
//
//            // We should not arrive here because we first check for a Reidemeister_I at c_0.
////            PD_ASSERT( false );
//            
//            wprint("We should not arrive here because we first check for a Reidemeister_I at c_0.");
//            
//            Reconnect( e_0, Tail, e_2);
//            Reconnect( e_0, Tip , e_1);
//            
//            DeactivateArc(a);
//            DeactivateArc(b);
//            
//            PD_PRINT("\t\tModified move 1.");
//            
//            goto exit;
//        }
//        else
//        {
//// This is a very, very special case:
////                   +----<----O       O---->----O       O----<----+
////                  /    e_3    ^     ^     b     \     /           \
////                 /             \   /             \   /             \
////         e_3    /               \ /               \ /               \    e_2
////          =    |             c_0 X                 X c_1             |    =
////         e_0    \               / \               / \               /    e_1
////                 \             /   \             /   \             /
////                  \    e_0    /     \     a     v     v           /
////                   +---->----O       O----<----O       O---->----+
////
//
//            //  We should not arrive here because we first check for a Reidemeister_I at c_0.
////            PD_ASSERT( false );
//            wprint("We should not arrive here because we first check for a Reidemeister_I at c_0. (2)");
//
//            ++unlink_count;
//            
//            PD_PRINT("\t\tUnlink detected.");
//            
//            DeactivateArc(a);
//            DeactivateArc(b);
//            DeactivateArc(e_0);
//            DeactivateArc(e_1);
//            
//            goto exit;
//        }
//    }
//    else if( e_1 == e_2 )
//    {
//        // We do not have to check for e_0 == e_3 anymore.
//
//// This special case:
////               v_3 O----<----O       O---->----O       O----<----+
////                       e_3    ^     ^     b     \     /    e_2    \
////                               \   /             \   /             \
////                                \ /               \ /               \    e_2
////         v_3 != v_0          c_0 X                 X c_1             |    =
////                                / \               / \               /    e_1
////                               /   \             /   \             /
////                       e_0    /     \     a     v     v    e_1    /
////               v_0 O---->----O       O----<----O       O---->----O
////
//
//        
//        //  We should not arrive here because we first check for a Reidemeister_I at c_1.
////        PD_ASSERT( false );
//        wprint("We should not arrive here because we first check for a Reidemeister_I at c_0. (3)");
//
//        
//        Reconnect(e_1,Tail,e_0);
//        Reconnect(e_1,Tip ,e_3);
//        
//        DeactivateArc(a);
//        DeactivateArc(b);
//        
//        PD_PRINT("\t\tModified move 2.");
//        
//        goto exit;
//    }

    {
// Finally, the most common case/
// This is for side == Right. In the other case (side == Left), we just flip everything around.
//
//                  C_arcs(c_0,Out,Right) = b = C_arcs(c_1,In ,Right)
//
//               v_3 O----<----O       O---->----O       O----<----O v_2
//                       e_3    ^     ^     b     \     /    e_2
//                               \   /             \   /
//                                \ /               \ /
//                             c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                       e_0    /     \     a     v     v    e_1
//               v_0 O---->----O       O----<----O       O---->----O v_1
//
//                  C_arcs(c_0,In ,Right) = a = C_arcs(c_1,Out,Right)
//
//
// State after the move:
//
//                     +----<---------------<---------------<----+
//                    /                     a                     \
//               v_3 O         O       O         O       O         O v_2
//                              ^     ^           \     /
//                               \   /             \   /
//                                \ /               \ /
//                             c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                              /     \           v     v
//               v_0 O         O       O         O       O         O v_1
//                    \                     b                     /
//                     +---->--------------->--------------->----+
        
        PD_PRINT(std::string("\t\tGeneric case (") + ((side==Left) ? "left)" : "right)") );
        Reconnect(a,Tail,e_2);
        Reconnect(a,Head,e_3);
        Reconnect(b,Tail,e_0);
        Reconnect(b,Head,e_1);
    
        goto exit;
    }
    
exit:
    
    // The two crossings are inactivated in any case.
    DeactivateCrossing(c_0);
    DeactivateCrossing(c_1);
    
    ++R_II_counter;
    ++R_II_horizontal_counter;

} // Reidemeister_II_Horizontal
