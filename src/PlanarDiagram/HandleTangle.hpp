bool HandleTangle( const Int c_0, const Int c_1, const bool side )
{
    PD_print("\tHandleTangle( \n\t\tc_0 = "+CrossingString(c_0)+", \n\t\tc_1 = "+CrossingString(c_1)+", \n\t\tside = " + ((side==Left)? "left" : "right" ) + " \n\t)");
        
    // We can resolve boths crossings in any case of sign distribution.
    // See the commented-out code below for an explanation
    
    PD_assert( C_arcs(c_0,Out,side) == C_arcs(c_1,In ,!side) );
    
    const Int a   = C_arcs(c_0,In , side);
    const Int b   = C_arcs(c_0,Out, side);
    
    const Int e_0 = C_arcs(c_0,In ,!side);
    const Int e_3 = C_arcs(c_0,Out,!side);
    const Int e_1 = C_arcs(c_1,In , side);
    const Int e_2 = C_arcs(c_1,Out, side);

    Reconnect(a, Tip , e_3);
    Reconnect(a, Tail, e_1);
    Reconnect(b, Tip , e_2);
    Reconnect(b, Tail, e_0);

    DeactivateArc(e_0);
    DeactivateArc(e_1);
    DeactivateArc(e_2);
    DeactivateArc(e_3);
    DeactivateCrossing(c_0);
    DeactivateCrossing(c_1);

    ++tangle_move_counter;

    return true;
    
//                This alternative code with explanation shows why we do not have to distinguish the two cases of  OppositeCrossingSigns(c_0,c_1):
//
//                if( OppositeCrossingSigns(c_0,c_1) )
//                {
//// This horizontal alignment in the case of side == Right and positive crossing c_0
//// Cases side == Left and sign = -1 are analogous.
////
////                         C_arcs(c_0,Out,side) = b = C_arcs(c_1,In ,!side,c_1)
////                                        +------->-------+
////                                       /                 \
////                                      /                   \
////                   O----<----O       O  +------+ e_1       |
////                       e_3    ^     ^   |      O-->--O     O
////                               \   /    |      |      \   /
////                                \ /     |      |       \ /
////                             c_0 /      |tangle|        \ c_1
////                                / \     |      |       / \
////                               /   \    |      |      v   v
////                       e_0    /     \   |      O--<--O     O
////                   O---->----O       O  +------+ e_2       |
////                                      \                   /
////                                       \                 /
////                                        +-------<-------+
////                     C_arcs(c_0,In ,side,c_0) = a = C_arcs(c_1,Out,!side)
////
//// We have several possibilities to simplify this:
////
////  I) The arcs e_1 and e_2 share two edges. So we could do also an inverse connect-sum operation.
////  However, I suppose that this tangle is typically quite small. (Thus case happens only really rarely. So a brach is maybe not worth the effort.
//
////  II) Rotate the tangle by 360 degrees about the horizontal axis. This resolves _both_ crossings!
////                      +-------------<--------------+
////                     /              a               \
////                    v                                |
////                   O                    +------+    /
////                                        |      O>--+
////                                        |      |
////                                        |      |
////                                        |tangle|
////                                        |      |
////                                        |      |
////                                        |      O<--+
////                   O                    +------+    \
////                    \                                |
////                     v              b               /
////                      +------------->--------------+
//
//                    Reconnect(a, Tip , e_3);
//                    Reconnect(a, Tail, e_1);
//                    Reconnect(b, Tip , e_2);
//                    Reconnect(b, Tail, e_0);
//
//                    DeactivateArc(e_0);
//                    DeactivateArc(e_1);
//                    DeactivateArc(e_2);
//                    DeactivateArc(e_3);
//                    DeactivateCrossing(c_0);
//                    DeactivateCrossing(c_1);
//
//                    ++tangle_move_counter;
//
//                    return true;
//                }
//                else
//                {
//// This horizontal alignment in the case of side == Right and positive crossing c_0/
//// Note that this case differs from the one above only by the sign of the crossing c_1.
////
////                         C_arcs(c_0,Out,side) = b = C_arcs(c_1,In, !side)
////                                        +------->-------+
////                                       /                 \
////                                      /                   \
////                   O----<----O       O  +------+ e_1       |
////                       e_3    ^     ^   |      O-->--O     O
////                               \   /    |      |      \   /
////                                \ /     |      |       \ /
////                             c_0 /      |tangle|        / c_1
////                                / \     |      |       / \
////                               /   \    |      |      v   v
////                       e_0    /     \   |      O--<--O     O
////                   O---->----O       O  +------+ e_2       |
////                                      \                   /
////                                       \                 /
////                                        +-------<-------+
////                         C_arcs(c_0,In ,side) = a = C_arcs(c_1,Out,!side)
////
//// This can simply resolved by moving b over the tangle to the bottom of the diagram and a under the tangle to the top of the diagram. The new diagramm looks like this:
////
////                      +-------------<--------------+
////                     /              a               \
////                    v                                |
////                   O                    +------+    /
////                                        |      O>--+
////                                        |      |
////                                        |      |
////                                        |tangle|
////                                        |      |
////                                        |      |
////                                        |      O<--+
////                   O                    +------+    \
////                    \                                |
////                     v              b               /
////                      +------------->--------------+
//
//                    Reconnect(a, Tip , e_3);
//                    Reconnect(a, Tail, e_1);
//                    Reconnect(b, Tip , e_2);
//                    Reconnect(b, Tail, e_0);
//
//                    DeactivateArc(e_0);
//                    DeactivateArc(e_1);
//                    DeactivateArc(e_2);
//                    DeactivateArc(e_3);
//                    DeactivateCrossing(c_0);
//                    DeactivateCrossing(c_1);
//
//                    ++tangle_move_counter;
//                }
}
