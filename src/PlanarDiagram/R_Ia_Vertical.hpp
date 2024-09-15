private:

bool Reidemeister_Ia_Vertical( const Int c_0, const Int c_1 )
{
    PD_PRINT("\nReidemeister_Ia_Vertical  ( "+CrossingString(c_0)+", "+CrossingString(c_1)+" )");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT( c_0 != c_1 );
    
    auto C_0 = Crossing( c_0 );
    auto C_1 = Crossing( c_1 );
    
    // This are some central assumption here. (c_0 is the bottom crossing.)
    PD_ASSERT( SameHandednessQ(C_0,C_1) );
    PD_ASSERT( C_0(Out,Left ) == C_1(In ,Left ) );
    PD_ASSERT( C_0(Out,Right) == C_1(In ,Right) );
    
    
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
    const bool leftQ  = Reidemeister_Ia_Vertical_impl(C_0,C_1,Left );
    
    R_Ia_counter += leftQ;
    
    const bool rightQ = Reidemeister_Ia_Vertical_impl(C_0,C_1,Right);
    
    R_Ia_counter += rightQ;
        
    return (leftQ || rightQ);
    
} // Reidemeister_Ia_Vertical



bool Reidemeister_Ia_Vertical_impl( CrossingView & C_0, CrossingView & C_1, const bool side )
{
    PD_PRINT("\nReidemeister_Ia_Vertical_impl  ( "+ToString(C_0)+", "+ToString(C_1)+"," side == Right ? "Right" : "Left" + " )");
    
    auto E_0 = Arc( C_0(In ,side) );
    auto E_1 = Arc( C_1(Out,side) );
      
    auto C_2 = Crossing( E_1(Head) );
    auto C_3 = Crossing( E_0(Tail) );

    if( C_2 == C_3 )
    {
        PD_ASSERT( C_0 != C_2 );
        PD_ASSERT( C_1 != C_2 );
        
        if( C_2(In,side) == E_1 )
        {
            PD_ASSERT( C_2(Out,side) == E_0 );
            
            if( OppositeHandednessQ(C_2,C_0) )
            {
//                logprint("Reidemeister_Ia_Vertical_impl - normal move!");
//                logvalprint( "side", Right ? "Right" : "Left" );
                
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
                
                PD_VALPRINT("C_0",C_0);
                PD_VALPRINT("C_1",C_1);
                PD_VALPRINT("C_2",C_2);
                
                // Reverse A.
                auto A = Arc( C_0(Out,!side) );

                PD_ASSERT( A(Tail) == C_0 );
                PD_ASSERT( A(Head) == C_1 );
                std::swap( A(Tail), A(Head) );

                // Reconnect arc f_0 (the Reconnect routine does not work here!)
                auto F_0 = Arc( C_2(Out,!side) );
//                logvalprint("f_0",ArcString(f_0));
                PD_ASSERT( F_0 != E_0 );
                PD_ASSERT( F_0 != E_1 );
                PD_ASSERT( F_0(Tail) == C_2 );
                F_0(Tail) = C_0.Idx();
                DeactivateArc(E_0.Idx());
                
                // Reconnect arc f_1 (the Reconnect routine does not work here!)
                auto F_1 = Arc( C_2(In ,!side) );
//                logvalprint("f_1",ArcString(f_1));
                PD_ASSERT( F_1 != E_0 );
                PD_ASSERT( F_1 != E_1 );
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
                
//                logprint("changed data");
//                logvalprint("c_0",CrossingString(c_0));
//                logvalprint("c_1",CrossingString(c_1));
//                logvalprint("f_0",ArcString(f_0));
//                logvalprint("f_1",ArcString(f_1));
//                logvalprint("a",ArcString(a));
                
                PD_ASSERT( CheckCrossing(C_0.Idx()) );
                PD_ASSERT( CheckCrossing(C_1.Idx()) );
                PD_ASSERT( CheckArc(F_0.Idx()) );
                PD_ASSERT( CheckArc(F_1.Idx()) );
                PD_ASSERT( CheckArc(A.Idx()) );
                
                ++R_Ia_vertical_counter;
                
//                logprint("\nReidemeister_Ia_Vertical_impl  ( "+ToString(C_0)+", "+ToString(C_1)+"," + ToString(side) + " )");
                
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
            PD_ASSERT( C_2(In ,!side) == E_1 );
            PD_ASSERT( C_2(Out,!side) == E_0 );
            
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
            
            // DEBUGGING
            wprint(ClassName()+"::Reidemeister_Ia_Horizontal_impl - twist move!");
            
            auto F_0 = Arc( C_2(In ,side ) );
            auto F_1 = Arc( C_2(Out,side ) );
            
            Reconnect(F_0,Tail,E_1);
            Reconnect(F_1,Head,E_0);
            DeactivateCrossing(C_2.Idx());
            
            ++R_Ia_vertical_counter;

            return true;
        }
    }
    
    return false;
}



bool Reidemeister_Ia_Vertical_B( const Int c_0, const Int c_1 )
{
    PD_PRINT("\nReidemeister_Ia_Vertical  ( "+CrossingString(c_0)+", "+CrossingString(c_1)+" )");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT( c_0 != c_1 );
    
    // This are some central assumption here. (c_0 is the bottom crossing.)
    PD_ASSERT( SameHandednessQ(c_0,c_1) );
    PD_ASSERT( C_arcs(c_0,Out,Left ) == C_arcs(c_1,In,Left ) );
    PD_ASSERT( C_arcs(c_0,Out,Right) == C_arcs(c_1,In,Right) );
    
    
//                      O       O
//                       ^     ^
//                        \   /
//                         \ /
//                      c_1 \
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
//                      c_0 \
//                         / \
//                        /   \
//                       /     \
//                      O       O
    
    // Since the two-crossing configuration of c_0 and c_1 is not destroyed, we can test both sides.
    const bool leftQ = Reidemeister_Ia_Vertical_impl(c_0,c_1,Left);
    
    R_Ia_counter += leftQ;
    
    const bool rightQ = Reidemeister_Ia_Vertical_impl(c_0,c_1,Right);
    
    R_Ia_counter += rightQ;
        
    return (leftQ || rightQ);
    
} // Reidemeister_Ia_Vertical

bool Reidemeister_Ia_Vertical_impl_B( const Int c_0, const Int c_1, const bool side )
{
    PD_PRINT("\nReidemeister_Ia_Vertical_impl  ( "+CrossingString(c_0)+", "+CrossingString(c_1)+"," side == Right ? "Right" : "Left" + " )");
    
    const Int e_0 = C_arcs(c_0,In ,side);
    const Int e_1 = C_arcs(c_1,Out,side);
      
    const Int c_2 = A_cross(e_1,Head);
    const Int c_3 = A_cross(e_0,Tail);

    if( c_2 == c_3 )
    {
        PD_ASSERT( c_0 != c_2 );
        PD_ASSERT( c_1 != c_2 );
        
        if( C_arcs(c_2,In,side) == e_1 )
        {
            PD_ASSERT( C_arcs(c_2,Out,side) == e_0 );
            
            if( OppositeHandednessQ(c_2,c_0) )
            {   
//                logprint("Reidemeister_Ia_Vertical_impl - normal move!");
//                logvalprint( "side", Right ? "Right" : "Left" );
                
// This situation for side == Right (or all crossings flipped to other handedness):
//
//
//            O----<----O       O---->----+
//                       ^     ^    e_1    \
//                        \   /             \
//                         \ /               \             /
//                      c_1 \                 \           /
//                         / \                 v         v
//                        /   \                 O       O f_1
//                       /     \                 \     /
//                      O       O                 \   /
//                      ^       ^                  v v
//                    a |       | b                 / c_2
//                      |       |                  / \
//                      O       O                 /   \
//                       ^     ^                 v     v
//                        \   /                 O       O f_0
//                         \ /                 /         \
//                      c_0 \                 /           \
//                         / \               /             v
//                        /   \             /
//                       /     \    e_0    v
//            O---->----O       O----<----+
//
// We can change it to this:
//
//            O----<----O       O--------<--------+
//                       ^     /        f_1        ^
//                        \   /                     \
//                         \ /                       \
//                      c_1 /                         \
//                         / \                         \
//                        /   \
//                       v     \
//                      O       O
//                      |       ^
//                    a |       | b
//                      V       |
//                      O       O
//                       \     ^
//                        \   /
//                         \ /                         ^
//                      c_0 /                         /
//                         / \                       /
//                        /   \                     /
//                       /     v        f_0        /
//            O---->----O       O-------->--------+
                
//                logvalprint("c_0",CrossingString(c_0));
//                logvalprint("c_1",CrossingString(c_1));
//                logvalprint("c_2",CrossingString(c_2));
                
                // Reverse a.
                const Int a = C_arcs(c_0,Out,!side);
                PD_ASSERT( CheckArc(a) );
                PD_ASSERT( A_cross(a,Tail) == c_0 );
                PD_ASSERT( A_cross(a,Head) == c_1 );
                std::swap( A_cross(a,Tail), A_cross(a,Head) );

                // Reconnect arc f_0 (the Reconnect routine does not work here!)
                const Int f_0 = C_arcs(c_2,Out,!side);
//                logvalprint("f_0",ArcString(f_0));
                PD_ASSERT(ArcActiveQ(f_0));
                PD_ASSERT( f_0 != e_0 );
                PD_ASSERT( f_0 != e_1 );
                PD_ASSERT(A_cross(f_0,Tail) == c_2);
                A_cross(f_0,Tail) = c_0;
                DeactivateArc(e_0);
                
                // Reconnect arc f_1 (the Reconnect routine does not work here!)
                const Int f_1 = C_arcs(c_2,In ,!side);
//                logvalprint("f_1",ArcString(f_1));
                PD_ASSERT(ArcActiveQ(f_1));
                PD_ASSERT( f_1 != e_0 );
                PD_ASSERT( f_1 != e_1 );
                PD_ASSERT(A_cross(f_1,Head) == c_2);
                A_cross(f_1,Head) = c_1;
                DeactivateArc(e_1);

                // Modify crossing c_0.
                C_arcs(c_0,In ,side) = f_0;
                RotateCrossing(c_0,side);
                
                // Modify crossing c_1.
                C_arcs(c_1,Out,side) = f_1;
                RotateCrossing(c_1,!side);

                // Finally, we can remove the crossing.
                DeactivateCrossing(c_2);
                
//                logprint("changed data");
//                logvalprint("c_0",CrossingString(c_0));
//                logvalprint("c_1",CrossingString(c_1));
//                logvalprint("f_0",ArcString(f_0));
//                logvalprint("f_1",ArcString(f_1));
//                logvalprint("a",ArcString(a));
                
                PD_ASSERT( CheckCrossing(c_0) );
                PD_ASSERT( CheckCrossing(c_1) );
                PD_ASSERT( CheckArc(f_0) );
                PD_ASSERT( CheckArc(f_1) );
                PD_ASSERT( CheckArc(a) );
                
                ++R_Ia_vertical_counter;
                
//                logprint("\nReidemeister_Ia_Vertical_impl  ( "+CrossingString(c_0)+", "+CrossingString(c_1)+"," + ToString(side) + " )");
                
                return true;
            }
            else
            {
// This situation:
//
//
//            O----<----O       O---->----+
//                       ^     ^    e_1    \
//                        \   /             \
//                         \ /               \
//                      c_1 \                 \           / f_1
//                         / \                 v         /
//                        /   \                 O       O
//                       /     \                 \     /
//                      O       O                 \   /
//                      ^       ^                  v v
//                    a |       | b                 \ c_2
//                      |       |                  / \
//                      O       O                 /   \
//                       ^     ^                 v     v
//                        \   /                 O       O
//                         \ /                 /         \
//                      c_0 \                 /           \ f_0
//                         / \               /
//                        /   \             /
//                       /     \    e_0    v
//            O---->----O       O----<----+
                    
                return false;
            }

        }
        else
        {
            PD_ASSERT( C_arcs(c_2,In ,!side) == e_1 );
            PD_ASSERT( C_arcs(c_2,Out,!side) == e_0 );
            
// This situation:
//
//            O----<----O       O-------->--------+
//                       ^     ^        e_1        \
//                        \   /                     \
//                         \ /                       \
//                      c_1 \                         \
//                         / \        +------+ f_1     v
//                        /   \       |      |->O       O
//                       /     \      |      |   \     /
//                      O       O     |      |    \   /
//                      ^       ^     |      |     v v
//                    a |       | b   |      |      X  c_2
//                      |       |     |      |     / \
//                      O       O     |      |    /   \
//                       ^     ^      |      |   v     v
//                        \   /       |      |<-O       O
//                         \ /        +------+ f_0     /
//                      c_0 \                         /
//                         / \                       /
//                        /   \                     /
//                       /     \        e_0        v
//            O---->----O       O--------<--------+
            
            
/* Changed to this:
 *
 *            O----<----O       O
 *                       ^     ^ \
 *                        \   /   \
 *                         \ /     v
 *                      c_1 \       +
 *                         / \      | +------+ f_1
 *                        /   \     | |      |->O
 *                       /     \    | |      |   \
 *                      O       O   | |      |    \
 *                      ^       ^   | |      |     \
 *                    a |       | b | |      |      \
 *                      |       |   | |      |       \
 *                      O       O   | |      |        \
 *                       ^     ^    | |      |         v
 *                        \   /     | |      |<-O       +
 *                         \ /      v +------+  |f_0   /
 *                      c_0 \       +-----------+     /
 *                         / \                       /
 *                        /   \                     /
 *                       /     \        e_0        v
 *           O---->----O       O--------<--------+
 */
            
            // DEBUGGING
            print("Reidemeister_Ia_Vertical_impl - twist move!");
            
            const Int f_0 = C_arcs(c_2,In ,side );
            const Int f_1 = C_arcs(c_2,Out,side );
            
            Reconnect(f_0,Tail,e_1);
            Reconnect(f_1,Head,e_0);
            DeactivateCrossing(c_2);

            return true;
        }
    }
    
    return false;
}
