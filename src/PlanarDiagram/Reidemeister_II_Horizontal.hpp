void Reidemeister_II_Horizontal( const Int c_0, const Int c_1, const bool side )
{
    PD_print("\tReidemeister_II_Horizontal( "+ToString(c_0)+", "+ToString(c_1)+", "+ ToString(side) +" )");
    
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_assert( c_0 != c_1 );
    
    PD_assert( CheckCrossing(c_0) );
    PD_assert( CheckCrossing(c_1) );
    
    
    const Int a_0 = C_arcs[Out][side][c_0];
    const Int a_1 = C_arcs[In ][side][c_1];
    
    const Int b_0 = C_arcs[In ][side][c_0];
    const Int b_1 = C_arcs[Out][side][c_1];
    
    PD_assert( CheckArc(a_0) );
    PD_assert( CheckArc(a_1) );
    PD_assert( CheckArc(b_0) );
    PD_assert( CheckArc(b_1) );
    
    // These are the central assumptions here.
    PD_assert( ((a_0 == a_1) && (b_0 == b_1)) );
    
    if( !((a_0 == a_1) && (b_0 == b_1)))
    {
        if( side == Left )
        {
            print("left");
        }
        else
        {
            print("right");
        }
        valprint( "C_state[c_0]", SI(C_state[c_0]));
        valprint( "C_state[c_1]", SI(C_state[c_1]));
        print( CrossingString(c_0));
        print( CrossingString(c_1));
        
        print( ArcString(a_0));
        print( ArcString(a_1));
        print( ArcString(b_0));
        print( ArcString(b_1));
    }
    
    
    
// We assume this horizontal alignment in the case of side==Right.
//
//                C_arcs[Out][side][c_0] = b = C_arcs[In ][_side][c_1]
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
//               C_arcs[In ][_side][c_0] = a = C_arcs[Out][_side][c_1]
//
// In the case side == Left, we just flip everything around.
    
    
    const Int a = C_arcs[In ][side][c_0];
    const Int b = C_arcs[Out][side][c_0];
    
    const Int e_0 = C_arcs[In ][!side][c_0];
    const Int e_1 = C_arcs[Out][!side][c_1];
    const Int e_2 = C_arcs[In ][!side][c_1];
    const Int e_3 = C_arcs[Out][!side][c_0];
    
    if( e_0 == e_1 )
    {
        if( e_2 != e_3 )
        {
// This special situation with a loop over or under a strand (in the case of side==Right):
//
//               v_3 O----<----O       O---->----O       O----<----O v_2
//                       e_3    ^     ^     b     \     /    e_2
//                               \   /             \   /
//                                \ /               \ /
//           v_2 != v_3        c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                              /     \     a     v     v
//                             O       O----<----O       O
//                              \                       /
//                               \      e_0 = e_1      /
//                                +---------<---------+
            
            // We should arrive here because this is a vertical case.
            PD_assert(false);
            
            unlink_count++;
            
            PD_print("\tUnlink detectd.");
            
            Reconnect( a, Tail, e_2);
            Reconnect( a, Tip , e_3);
            
            DeactivateArc(b);
            DeactivateArc(e_0);
            DeactivateArc(e_2);
            DeactivateArc(e_3);
            
            goto exit;
        }
        else
        {
// This very,very special situation with two noninterlinked loops (in the case of side==Right):
//
//                                +--------->---------+
//                               /     e_3 = e_2       \
//                              /                       \
//                             O       O---->----O       O
//                              ^     ^     b     \     /
//                               \   /             \   /
//                                \ /               \ /
//                             c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                              /     \     a     v     v
//                             O       O----<----O       O
//                              \                       /
//                               \      e_0 = e_1      /
//                                +---------<---------+
            
            // We should arrive here because this is a vertical case.
            PD_assert(false);
            
            PD_print("\tTwo unlinks detectd.");
            
            unlink_count += 2;
            
            DeactivateArc(a);
            DeactivateArc(b);
            DeactivateArc(e_0);
            DeactivateArc(e_2);
            
            goto exit;
        }
        
    }
    else if( e_3 == e_2 )
    {
        // We don't have to check the case e_0 = e_1 here.

//                                +--------->---------+
//                               /     e_3 = e_2       \
//                              /                       \
//                             O       O---->----O       O
//                       e_3    ^     ^     b     \     /    e_2
//                               \   /             \   /
//                                \ /               \ /
//         v_0 != v_1          c_0 X                 X c_1
//                                / \               / \
//                               /   \             /   \
//                       e_0    /     \     a     v     v    e_1
//               v_0 O---->----O       O----<----O       O---->----O v_1
    
        // We should arrive here because this is a vertical case.
        PD_assert(false);
        
        unlink_count ++;
        
        PD_print("\tUnlink detectd.");
        
        Reconnect( b, Tail, e_0);
        Reconnect( b, Tip , e_1);
        
        DeactivateArc(a);
        DeactivateArc(e_0);
        DeactivateArc(e_1);
        DeactivateArc(e_2);
        
        goto exit;
    }
        
    if( e_0 == e_3 )
    {
        if( e_1 != e_2 )
        {
// This special case:
//                   +----<----O       O---->----O       O----<----O v_2
//                  /    e_3    ^     ^     b     \     /    e_2
//                 /             \   /             \   /
//         e_3    /               \ /               \ /
//          =    |             c_0 X                 X c_1        v_1 != v_2
//         e_0    \               / \               / \
//                 \             /   \             /   \
//                  \    e_0    /     \     a     v     v    e_1
//                   +---->----O       O----<----O       O---->----O v_1
//

            //  Should happen because we first check for a Reidemeister_I at c_0.
            PD_assert( false );
            
            Reconnect( e_0, Tail, e_2);
            Reconnect( e_0, Tip , e_1);
            
            DeactivateArc(a);
            DeactivateArc(b);
            
            PD_print("\t\tModified move 1.");
            
            goto exit;
        }
        else
        {
// This very, very special case:
//                   +----<----O       O---->----O       O----<----+
//                  /    e_3    ^     ^     b     \     /           \
//                 /             \   /             \   /             \
//         e_3    /               \ /               \ /               \    e_2
//          =    |             c_0 X                 X c_1             |    =
//         e_0    \               / \               / \               /    e_1
//                 \             /   \             /   \             /
//                  \    e_0    /     \     a     v     v           /
//                   +---->----O       O----<----O       O---->----+
//

            //  Should happen because we first check for a Reidemeister_I at c_0.
            PD_assert( false );

            ++unlink_count;
            
            PD_print("\t\tUnlink detected.");
            
            DeactivateArc(a);
            DeactivateArc(b);
            DeactivateArc(e_0);
            DeactivateArc(e_1);
            
            goto exit;
        }
    }
    else if( e_1 == e_2 )
    {
        // We do not have to check for e_0 == e_3 anymore.

// This special case:
//               v_3 O----<----O       O---->----O       O----<----+
//                       e_3    ^     ^     b     \     /    e_2    \
//                               \   /             \   /             \
//                                \ /               \ /               \    e_2
//         v_3 != v_0          c_0 X                 X c_1             |    =
//                                / \               / \               /    e_1
//                               /   \             /   \             /
//                       e_0    /     \     a     v     v    e_1    /
//               v_0 O---->----O       O----<----O       O---->----O
//

        
        //  Should happen be cause we first check for a Reidemeister_I at c_1.
        PD_assert( false );

        
        Reconnect( e_1, Tail, e_0);
        Reconnect( e_1, Tip , e_3);
        
        DeactivateArc(a);
        DeactivateArc(b);
        
        PD_print("\t\tModified move 2.");
        
        goto exit;
    }

    {
// Finally, the most common case/
// This is for side == Right. In the other case (side == Left), we just flip everything around.
//
  //              C_arcs[Out][Right][c_0] = b = C_arcs[In ][Right][c_1]
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
//                C_arcs[In ][Right][c_0] = a = C_arcs[Out][Right][c_1]
//
//
// State after the move:
//
//                     +----<---------------<---------------<----+
//                    /                     a                     \
//               v_3 O         P       O         O       O         O v_2
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
        
        PD_print(std::string("\t\tGeneric case (") + ((side==Left) ? "left)" : "right)") );
        Reconnect( a, Tail, e_2 );
        Reconnect( a, Tip , e_3 );
        Reconnect( b, Tail, e_0 );
        Reconnect( b, Tip , e_1 );
        
        DeactivateArc(e_0);
        DeactivateArc(e_1);
        DeactivateArc(e_2);
        DeactivateArc(e_3);
    
        goto exit;
    }
    
exit:
    
    // The two crossings are inactived in any case.
    DeactivateCrossing(c_0);
    DeactivateCrossing(c_1);
    
    ++R_II_counter;
//    CheckAll();

} // Reidemeister_II_Horizontal
