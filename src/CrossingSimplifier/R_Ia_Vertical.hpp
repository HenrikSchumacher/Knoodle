private:

bool Reidemeister_Ia_Vertical( const Int c_0, const Int c_1 )
{
    PD_PRINT(ClassName() + "::Reidemeister_Ia_Vertical("
        + ",\n\t" + CrossingString(c_0)
        + ",\n\t" + CrossingString(c_1)
        + ")");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT(c_0 != c_1);
    
    auto C_0 = GetCrossing(c_0);
    auto C_1 = GetCrossing(c_1);
    
    // This are some central assumption here. (c_0 is the bottom crossing.)
    PD_ASSERT(SameHandednessQ(C_0,C_1));
    PD_ASSERT(C_0(Out,Left ) == C_1(In ,Left ));
    PD_ASSERT(C_0(Out,Right) == C_1(In ,Right));
    
    
//                      O       O
//                       ^     ^
//                        \   /
//                         \ /
//                      C_1 \
//                         / \
//                        /   \
//                       /     \
//                      O       O
//                      ^       ^
//                      |       |
//                      |       |
//                      O       O
//                       ^     ^
//                        \   /
//                         \ /
//                      C_0 \
//                         / \
//                        /   \
//                       /     \
//                      O       O
    
    // Since the two-crossing configuration of c_0 and c_1 is not destroyed, we can test both sides.
    const bool leftQ  = Reidemeister_Ia_Vertical_impl(C_0,C_1,Left);
    
    pd.R_Ia_counter += leftQ;
    
    const bool rightQ = Reidemeister_Ia_Vertical_impl(C_0,C_1,Right);
    
    pd.R_Ia_counter += rightQ;
        
    return (leftQ || rightQ);
    
} // Reidemeister_Ia_Vertical



bool Reidemeister_Ia_Vertical_impl( CrossingView & C_0, CrossingView & C_1, const bool side )
{
    PD_PRINT(std::string("Reidemeister_Ia_Vertical_impl(")
        + ",\n\t" + ToString(C_0)
        + ",\n\t" + ToString(C_1)
        + ",\n\t" + ((side == Right) ? "Right" : "Left") +")");

    
    auto E_0 = GetArc(C_0(In ,side));
    auto E_1 = GetArc(C_1(Out,side));
      
    auto C_2 = GetCrossing(E_1(Head));
    auto C_3 = GetCrossing(E_0(Tail));

    if(C_2 == C_3)
    {
        PD_ASSERT(C_0 != C_2);
        PD_ASSERT(C_1 != C_2);
        
        if(C_2(In,side) == E_1)
        {
            PD_ASSERT(C_2(Out,side) == E_0);
            
            if(OppositeHandednessQ(C_2,C_0))
            {
//                logprint("Reidemeister_Ia_Vertical_impl - normal move!");
//                logvalprint( "side", Right ? "Right" : "Left");
                
// This situation for side == Right (or all crossings flipped to other handedness):
//
//
//            O----<----O       O---->----+
//                       ^     ^    E_1    \
//                        \   /             \
//                         \ /               \             /
//                      C_1 \                 \           /
//                         / \                 v         v
//                        /   \                 O       O F_1
//                       /     \                 \     /
//                      O       O                 \   /
//                      ^       ^                  v v
//                    A |       | B                 / C_2
//                      |       |                  / \
//                      O       O                 /   \
//                       ^     ^                 v     v
//                        \   /                 O       O F_0
//                         \ /                 /         \
//                      C_0 \                 /           \
//                         / \               /             v
//                        /   \             /
//                       /     \    E_0    v
//            O---->----O       O----<----+
//
// We can change it to this:
//
//            O----<----O       O--------<--------+
//                       ^     /        F_1        ^
//                        \   /                     \
//                         \ /                       \
//                      C_1 /                         \
//                         / \                         \
//                        /   \
//                       v     \
//                      O       O
//                      |       ^
//                    A |       | B
//                      V       |
//                      O       O
//                       \     ^
//                        \   /
//                         \ /                         ^
//                      C_0 /                         /
//                         / \                       /
//                        /   \                     /
//                       /     v        F_0        /
//            O---->----O       O-------->--------+
                
                PD_PRINT("Incoming data.");
                
                PD_VALPRINT("C_0",C_0);
                PD_VALPRINT("C_1",C_1);
                PD_VALPRINT("C_2",C_2);
                
                // Reverse A.
                auto A = GetArc(C_0(Out,!side));

                PD_ASSERT(A(Tail) == C_0);
                PD_ASSERT(A(Head) == C_1);
                std::swap( A(Tail), A(Head));

                // Reconnect arc f_0 (the Reconnect routine does not work here!)
                auto F_0 = GetArc(C_2(Out,!side));
//                logvalprint("f_0",ArcString(f_0));
                PD_ASSERT(F_0 != E_0);
                PD_ASSERT(F_0 != E_1);
                PD_ASSERT(F_0(Tail) == C_2);
                F_0(Tail) = C_0.Idx();
                DeactivateArc(E_0.Idx());
                
                // Reconnect arc f_1 (the Reconnect routine does not work here!)
                auto F_1 = GetArc(C_2(In ,!side));
//                logvalprint("f_1",ArcString(f_1));
                PD_ASSERT(F_1 != E_0);
                PD_ASSERT(F_1 != E_1);
                PD_ASSERT(F_1(Head) == C_2);
                F_1(Head) = C_1.Idx();
                DeactivateArc(E_1.Idx());

                // Modify crossing C_0.
                C_0(In ,side) = F_0.Idx();
                RotateCrossing(C_0,side);
                
                // Modify crossing C_1.
                C_1(Out,side) = F_1.Idx();
                RotateCrossing(C_1,!side);

                // Finally, we can remove the crossing.
                DeactivateCrossing(C_2.Idx());
                
                PD_PRINT("Changed data.");
                PD_VALPRINT("C_0",C_0);
                PD_VALPRINT("C_1",C_1);
                PD_VALPRINT("F_0",F_0);
                PD_VALPRINT("F_1",F_1);
                PD_VALPRINT("A",A);
                
                PD_ASSERT(pd.CheckCrossing(C_0.Idx()));
                PD_ASSERT(pd.CheckCrossing(C_1.Idx()));
                PD_ASSERT(pd.CheckArc(F_0.Idx()));
                PD_ASSERT(pd.CheckArc(F_1.Idx()));
                PD_ASSERT(pd.CheckArc(A.Idx()));

                return true;
            }
            else
            {
// This situation:
//
//
//            O----<----O       O---->----+
//                       ^     ^    E_1    \
//                        \   /             \
//                         \ /               \
//                      C_1 \                 \           / F_1
//                         / \                 v         /
//                        /   \                 O       O
//                       /     \                 \     /
//                      O       O                 \   /
//                      ^       ^                  v v
//                    A |       | B                 \ C_2
//                      |       |                  / \
//                      O       O                 /   \
//                       ^     ^                 v     v
//                        \   /                 O       O
//                         \ /                 /         \
//                      C_0 \                 /           \ F_0
//                         / \               /
//                        /   \             /
//                       /     \    E_0    v
//            O---->----O       O----<----+
                    
                return false;
            }

        }
        else
        {
            PD_ASSERT(C_2(In ,!side) == E_1);
            PD_ASSERT(C_2(Out,!side) == E_0);
            
// This situation:
//
//            O----<----O       O-------->--------+
//                       ^     ^        E_1        \
//                        \   /                     \
//                         \ /                       \
//                      C_1 \                         \
//                         / \        +------+ F_1     v
//                        /   \       |      |->O       O
//                       /     \      |      |   \     /
//                      O       O     |      |    \   /
//                      ^       ^     |      |     v v
//                    A |       | B   |      |      X  C_2
//                      |       |     |      |     / \
//                      O       O     |      |    /   \
//                       ^     ^      |      |   v     v
//                        \   /       |      |<-O       O
//                         \ /        +------+ F_0     /
//                      C_0 \                         /
//                         / \                       /
//                        /   \                     /
//                       /     \        E_0        v
//            O---->----O       O--------<--------+
            
            
/* Changed to this:
 *
 *            O----<----O       O
 *                       ^     ^ \
 *                        \   /   \
 *                         \ /     v
 *                      C_1 \       +
 *                         / \      | +------+ F_1
 *                        /   \     | |      |->O
 *                       /     \    | |      |   \
 *                      O       O   | |      |    \
 *                      ^       ^   | |      |     \
 *                    A |       | B | |      |      \
 *                      |       |   | |      |       \
 *                      O       O   | |      |        \
 *                       ^     ^    | |      |         v
 *                        \   /     | |      |<-O       +
 *                         \ /      v +------+  |F_0   /
 *                      C_0 \       +-----------+     /
 *                         / \                       /
 *                        /   \                     /
 *                       /     \        E_0        v
 *           O---->----O       O--------<--------+
 */
            //TODO: Test this.
            
            auto F_0 = GetArc(C_2(In ,side ));
            auto F_1 = GetArc(C_2(Out,side ));
            
            Reconnect<Tail>(F_0,E_1);
            Reconnect<Head>(F_1,E_0);
            DeactivateCrossing(C_2.Idx());
              
            return true;
        }
    }
    
    return false;
}
