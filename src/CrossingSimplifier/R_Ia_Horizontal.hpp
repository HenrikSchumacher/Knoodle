private:

bool Reidemeister_Ia_Horizontal( const Int c_0, const Int c_1, const bool side )
{
    PD_PRINT(ClassName() + "::Reidemeister_Ia_Horizontal("
        + ",\n\t" + CrossingString(c_0)
        + ",\n\t" + CrossingString(c_1)
        + ",\n\t" + ((side == Right) ? "Right" : "Left") +")");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_ASSERT(c_0 != c_1);
    
    auto C_0 = GetCrossing(c_0);
    auto C_1 = GetCrossing(c_1);
    
    // This are some central assumption here. (C_0 is the bottom crossing.)
    PD_ASSERT(SameHandednessQ(C_0,C_1));
    PD_ASSERT(C_0(Out,side) == C_1(In ,side));
    PD_ASSERT(C_0(In ,side) == C_1(Out,side));
    
// We assume this horizontal alignment in the case of side==Right.
// Both crossing's handedness can also be opposite (at the same time!).
//
//                  C_0(Out,side) = B = C_1(In ,side)
//
//           O----<----O       O---->----O       O----<----O
//               E_3    ^     ^     B     \     /    E_2
//                       \   /             \   /
//                        \ /               \ /
//                     C_0 /                 / C_1
//                        / \               / \
//                       /   \             /   \
//               E_0    /     \     A     v     v    E_1
//           O---->----O       O----<----O       O---->----O
//
//                  C_0(In ,side) = A = C_0(Out,side)
//
// In the case side == Left, we just flip everything around.
    
    
    // Since the two-crossing configuration of c_0 and c_1 is not destroyed, we can test both sides.
    bool changedQ = Reidemeister_Ia_Horizontal_impl(C_0,C_1,Out,side);
    
    pd.R_Ia_counter += changedQ;
    
    if( !changedQ )
    {
        changedQ = Reidemeister_Ia_Horizontal_impl(C_0,C_1,In ,side);
        
        pd.R_Ia_counter += changedQ;
    }
    
    return changedQ;
    
} // Reidemeister_Ia_Horizontal


bool Reidemeister_Ia_Horizontal_impl( CrossingView & C_0, CrossingView & C_1, const bool io, const bool side )
{
    PD_PRINT(std::string("Reidemeister_Ia_Horizontal_impl(")
        + ",\n\t" + ToString(C_0)
        + ",\n\t" + ToString(C_1)
        + ",\n\t" + ((io == Out) ? "Out" : "In") + "," + ((side == Right) ? "Right" : "Left") +")");
    PD_ASSERT(C_0(Out,side) == C_1(In ,side));
    PD_ASSERT(C_0(In ,side) == C_1(Out,side));
    
    auto E_0 = GetArc(C_0( io,!side));
    auto E_1 = GetArc(C_1(!io,!side));
    
    PD_ASSERT(E_0( (io == Out) ? Tail : Head ) == C_0);
    PD_ASSERT(E_1( (io == Out) ? Head : Tail ) == C_1);

    auto C_2 = GetCrossing(E_0( (io == Out) ? Head : Tail ));
    auto C_3 = GetCrossing(E_1( (io == Out) ? Tail : Head ));
    
    if( C_2 == C_3 )
    {
        PD_ASSERT(C_0 != C_2);
        PD_ASSERT(C_1 != C_2);
        
        if( C_2(!io, side) == E_0 )
        {
            if( SameHandednessQ(C_0,C_2) )
            {
                PD_PRINT("Reidemeister_Ia_Horizontal_impl normal move.");
                
                PD_VALPRINT("io", io == In ? "In" : "Out");
                PD_VALPRINT("side", side == Left ? "Left" : "Right");

                
// We have this horizontal alignment in the case of io = Out and side == Right.
// (In the case side == Left or io == In, we just have to reflect everything accordingly.)
// All crossing's handedness can also be switched (at the same time!).
//
//                         F_0  O       O  F_1
//                               \     ^
//                                \   /
//                                 \ /
//                                  \ C_2
//                                 / \
//                                /   \
//                               /     v
//                              O       O
//                             ^         \
//                            /           \
//                           /             \
//                          /               \
//                    E_0  /                 \  E_1
//                        /                   \
//                       /                     \
//                      /                       v
//                     O       O---->----O       O
//                      ^     ^     B     \     /
//                       \   /             \   /
//                        \ /               \ /
//                     C_0 /                 / C_1
//                        / \               / \
//                       /   \             /   \
//                      /     \     A     v     v
//           O---->----O       O----<----O       O---->----O
//
                
//                PD_ASSERT(io   == Out  );
//                PD_ASSERT(side == Right);
                
                auto A   = GetArc(C_0(!io, side));
                auto F_0 = GetArc(C_2(!io,!side));
                auto F_1 = GetArc(C_2( io,!side));
                
                
                PD_PRINT("incoming data");
                PD_VALPRINT("C_0",C_0);
                PD_VALPRINT("C_1",C_1);
                PD_VALPRINT("C_2",C_2);

                PD_VALPRINT("E_0",E_0);
                PD_VALPRINT("E_1",E_1);

                PD_VALPRINT("F_0",F_0);
                PD_VALPRINT("F_1",F_1);

                PD_VALPRINT("A",A);
                
                PD_ASSERT(C_0 != C_1);
                PD_ASSERT(C_0 != C_2);
                PD_ASSERT(C_1 != C_2);
                PD_ASSERT(SameHandednessQ(C_0,C_1));
                PD_ASSERT(SameHandednessQ(C_0,C_2));
                PD_ASSERT(SameHandednessQ(C_1,C_2));
                
                PD_ASSERT(E_0 != E_1);
                PD_ASSERT(F_0 != F_1);
                PD_ASSERT(F_0 != E_0);
                PD_ASSERT(F_0 != E_1);
                PD_ASSERT(F_1 != E_0);
                PD_ASSERT(F_1 != E_1);

                
//                PD_ASSERT(A_cross(e_0,Head) == c_2); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(e_0,Tail) == c_0); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(e_1,Head) == c_1); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(e_1,Tail) == c_2); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(f_0,Head) == c_2); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(f_1,Tail) == c_2); // Only for io == Out and side == Right.
//
//                PD_ASSERT(A_cross(a  ,Head) == c_0); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(a  ,Tail) == c_1); // Only for io == Out and side == Right.
//
//                PD_ASSERT(C_arcs(c_2,In ,Right) == e_0); // Only for io == Out and side == Right.
//                PD_ASSERT(C_arcs(c_2,Out,Right) == e_1); // Only for io == Out and side == Right.
//                PD_ASSERT(C_arcs(c_2,In ,Left ) == f_0); // Only for io == Out and side == Right.
//                PD_ASSERT(C_arcs(c_2,Out,Left ) == f_1); // Only for io == Out and side == Right.

//
// In the case side == Left or io == In, we just have to reflect everything accordingly.
//
//
// Change to this:
//
//                              |       ^
//                              |       |
//                              |       |
//                              |       |
//                              |       |
//                              |       |
//                              v       |
//                              +       O
//                             /         ^
//                            /           \
//                           /             \
//                          /               \
//                    F_0  /                 \  F_1
//                        /                   \
//                       /                     \
//                      v                       \
//                     O       O---->----O       O
//                      \     ^     B     \     ^
//                       \   /             \   /
//                        \ /               \ /
//                     C_0 \                 \ C_1
//                        / \               / \
//                       /   \             /   \
//                      /     v     A     /     v
//           O---->----O       O---->----O       O---->----O
                
                
                // Reverse a.
                std::swap( A(Tail), A(Head));
                
                // Reconnect arc f_0 (the Reconnect routine does not work!)
                PD_ASSERT(F_0( io == Out ? Head : Tail ) == C_2);
                F_0(io == Out ? Head : Tail) = C_0.Idx();
                DeactivateArc(E_0.Idx());
                
                // Reconnect arc F_1 (the Reconnect routine does not work!)
                PD_ASSERT(F_1( io == Out ? Tail : Head) == C_2);
                F_1( io == Out ? Tail : Head ) = C_1.Idx();
                DeactivateArc(E_1.Idx());
                
                // Modify crossing C_0.
                C_0( io,!side) = F_0.Idx();
                RotateCrossing(C_0,io == Out ? side : !side); // ??
                
                // Modify crossing C_1.
                C_1(!io,!side) = F_1.Idx();
                RotateCrossing(C_1,io == Out ? !side : side); // ??
                
                // Finally, we can remove the crossing.
                DeactivateCrossing(C_2.Idx());
                
                PD_PRINT("changedQ data");
                PD_VALPRINT("C_0",C_0);
                PD_VALPRINT("C_1",C_1);
                PD_VALPRINT("F_0",F_0);
                PD_VALPRINT("F_1",F_1);
                PD_VALPRINT("A  ",A );
                
                PD_ASSERT( C_0.ActiveQ());
                PD_ASSERT( C_1.ActiveQ());
                PD_ASSERT(!C_2.ActiveQ());
                
                PD_ASSERT(!E_0.ActiveQ());
                PD_ASSERT(!E_1.ActiveQ());
                PD_ASSERT( F_0.ActiveQ());
                PD_ASSERT( F_1.ActiveQ());
                PD_ASSERT( A.ActiveQ());
                
                PD_ASSERT(pd.CheckCrossing(C_0.Idx()));
                PD_ASSERT(pd.CheckCrossing(C_1.Idx()));
                PD_ASSERT(pd.CheckArc(F_0.Idx()));
                PD_ASSERT(pd.CheckArc(F_1.Idx()));
                PD_ASSERT(pd.CheckArc(A.Idx()));
                

//                PD_ASSERT(A_cross(f_0,Head) == c_0); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(f_1,Tail) == c_1); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(a  ,Head) == c_1); // Only for io == Out and side == Right.
//                PD_ASSERT(A_cross(a  ,Tail) == c_0); // Only for io == Out and side == Right.
                
                PD_PRINT("Reidemeister_Ia_Horizontal_impl normal move done.");
                
                return true;
            }
            else
            {

// We have this horizontal alignment in the case of io = Out and side == Right.
// (In the case side == Left or io == In, we just have to reflect everything accordingly.)
// All crossing's handedness can also be switched (at the same time!).
//
//                         F_0  O       O  F_1
//                               \     ^
//                                \   /
//                                 \ /
//                                  / C_2
//                                 / \
//                                /   \
//                               /     v
//                              O       O
//                             ^         \
//                            /           \
//                           /             \
//                          /               \
//                    E_0  /                 \  E_1
//                        /                   \
//                       /                     \
//                      /                       v
//                     O       O---->----O       O
//                      ^     ^     B     \     /
//                       \   /             \   /
//                        \ /               \ /
//                     C_0 /                 / C_1
//                        / \               / \
//                       /   \             /   \
//                      /     \     A     v     v
//           O---->----O       O----<----O       O---->----O

// So we cannot do anything meaningful.
                return false;
            }
        }
        else
        {
            // This is a very seldom (impossible?) case.
            // TODO: Actually, this is an indicator for a connect sum.
            
            PD_PRINT("Reidemeister_Ia_Horizontal_impl - twist move");

            
            PD_ASSERT(C_0(Out,side) == C_1(In ,side));
            PD_ASSERT(C_0(In ,side) == C_1(Out,side));
            
            PD_VALPRINT( "io", io == In ? "In" : "Out");
            PD_VALPRINT( "side", side == Left ? "Left" : "Right");
            
            
//            ArcView F_0;
//            ArcView F_1;
//
//            if( io == Out )
//            {
//                if( side == Left )
//                {
//// We have this horizontal alignment in the case of io = Out and side == Left.
//// All crossing's handedness can also be switched (at the same time!).
////
////                              O       O
////                             / ^     / ^
////                            /   \   /   \
////                           /     \ /     \
////                          /       X C_2   \
////                    E_1  /       / \       \  E_0
////                        /       /   \       \
////                       /       v     \       \
////                      v       O       O       \
////                     +        |       ^        +
////                     |    F_0 v       | F_1    ^
////                     |      +-----------+      |
////                     |      |  Tangle   |      |
////                     |      +-----------+      |
////                     v                         |
////                     O       O----<----O       O
////                      \     /           ^     ^
////                       \   /             \   /
////                        \ /               \ /
////                     C_1 /                 / C_0
////                        / \               / \
////                       /   \             /   \
////                      v     v           /     \
////           O----<----O       O---->----O       O----<----O
////
//                    F_0 = GetArc(C_2( Out, Left ));
//                    F_1 = GetArc(C_2( In , Left ));
//                }
//                else // if( side == Right )
//                {
//// We have this horizontal alignment in the case of io = Out and side == Right.
//// (In the case side == Left or io == In, we just have to reflect everything accordingly.)
//// All crossing's handedness can also be switched (at the same time!).
////
////                              O       O
////                             ^ \     ^ \
////                            /   \   /   \
////                           /     \ /     \
////                          /       X C_2   \
////                    E_0  /       / \       \  E_1
////                        /       /   \       \
////                       /       /     v       \
////                      /       O       O       v
////                     +        ^       |        +
////                     ^    F_1 |       v F_0    |
////                     |      +-----------+      |
////                     |      |  Tangle   |      |
////                     |      +-----------+      |
////                     |                         v
////                     O       O---->----O       O
////                      ^     ^           \     /
////                       \   /             \   /
////                        \ /               \ /
////                     C_0 /                 / C_1
////                        / \               / \
////                       /   \             /   \
////                      /     \           v     v
////           O---->----O       O----<----O       O---->----O
////
////
//                    F_0 = GetArc(C_2( Out, Right));
//                    F_1 = GetArc(C_2( In , Right));
//                }
//            }
//            else // if( io == In )
//            {
//                if( side == Left )
//                {
//// We have this horizontal alignment in the case of io = In and side == Left.
//// All crossing's handedness can also be switched (at the same time!).
////
////           O---->----O       O----<----O       O---->----O
////                      \     /           ^     ^
////                       \   /             \   /
////                        \ /               \ /
////                     C_1 /                 / C_0
////                        / \               / \
////                       /   \             /   \
////                      v     v           /     \
////                     O       O---->----O       O
////                     |                         ^
////                     |      +-----------+      |
////                     |      |  Tangle   |      |
////                     |      +-----------+      |
////                     v    F_0 |       ^ F_1    |
////                     +        v       |        +
////                      \       O       O       ^
////                       \       \     ^       /
////                        \       \   /       /
////                    E_1  \       \ /       /  E_0
////                          \       X C_2   /
////                           \     / \     /
////                            \   /   \   /
////                             v /     v /
////                              O       O
//
//                    F_0 = GetArc(C_2( In , Left ));
//                    F_1 = GetArc(C_2( Out, Left ));
//                }
//                else // if( side == Right )
//                {
//// We have this horizontal alignment in the case of io = In and side == Right.
//// All crossing's handedness can also be switched (at the same time!).
////
////           O---->----O       O---->----O       O---->----O
////                      ^     ^           \     /
////                       \   /             \   /
////                        \ /               \ /
////                     C_0 /                 / C_1
////                        / \               / \
////                       /   \             /   \
////                      /     \           v     v
////                     O       O----<----O       O
////                     ^                         |
////                     |      +-----------+      |
////                     |      |  Tangle   |      |
////                     |      +-----------+      |
////                     |    F_1 ^       | F_0    v
////                     +        |       v        +
////                      ^       O       O       /
////                       \       ^     /       /
////                        \       \   /       /
////                    E_0  \       \ /       /  E_1
////                          \       X C_2   /
////                           \     / \     /
////                            \   /   \   /
////                             \ v     \ v
////                              O       O
//
//                    F_0 = GetArc(C_2( In , Right));
//                    F_1 = GetArc(C_2( Out, Right));
//                }
//            }
            
            auto F_0 = GetArc(C_2( io, side ));
            auto F_1 = GetArc(C_2(!io, side ));

            PD_ASSERT(E_0 != F_1);
            PD_ASSERT(E_0 != F_0);
            PD_ASSERT(E_1 != F_1);
            PD_ASSERT(E_1 != F_0);

            PD_PRINT("Incoming data.");
            
            PD_VALPRINT("C_0",C_0.String());
            PD_VALPRINT("C_1",C_1.String());
            PD_VALPRINT("C_2",C_2.String());
            
            PD_VALPRINT("E_0",E_0.String());
            PD_VALPRINT("E_1",E_1.String());
            PD_VALPRINT("F_0",F_0.String());
            PD_VALPRINT("F_0",F_0.String());
            
            PD_ASSERT(C_0(Out,side) == C_1(In ,side));
            PD_ASSERT(C_0(In ,side) == C_1(Out,side));
            
            if( io != Out || side !=Left )
            {
                PD_PRINT("Aborted because of io != Out || side !=Left");
                return false;
            }
            
            PD_ASSERT(E_0(Head) == C_2); // Only for io == Out, side == Left.
            PD_ASSERT(E_0(Tail) == C_0); // Only for io == Out, side == Left.
            
            PD_ASSERT(E_1(Head) == C_1); // Only for io == Out, side == Left.
            PD_ASSERT(E_1(Tail) == C_2); // Only for io == Out, side == Left.
            
            PD_ASSERT(F_1(Head) == C_2); // Only for io == Out, side == Left.
            PD_ASSERT(F_0(Tail) == C_2); // Only for io == Out, side == Left.

            
// We have this horizontal alignment in the case of io = Out and side == Left.
// All crossing's handedness can also be switched (at the same time!).
//
//                              O       O
//                             / ^     / ^
//                            /   \   /   \
//                           /     \ /     \
//                          /       X C_2   \
//                    E_1  /       / \       \  E_0
//                        /       /   \       \
//                       /       v     \       \
//                      v       O       O       \
//                     +        |       ^        +
//                     |    F_0 v       | F_1    ^
//                     |      +-----------+      |
//                     |      |  Tangle   |      |
//                     |      +-----------+      |
//                     v                         |
//                     O       O----<----O       O
//                      \     /           ^     ^
//                       \   /             \   /
//                        \ /               \ /
//                     C_1 /                 / C_0
//                        / \               / \
//                       /   \             /   \
//                      v     v           /     \
//           O----<----O       O---->----O       O----<----O

            // DEBUGGING
            wprint(ClassName()+"::Reidemeister_Ia_Horizontal_impl - twist move!");
            //TODO: Test this.
                
            PD_ASSERT(F_0( io == Out ? Tail : Head ) == C_2);
            PD_ASSERT(F_1( io == Out ? Head : Tail ) == C_2);


//            Reconnect( F_0, io == Out ? Tail : Head, E_0);
//            Reconnect( F_1, io == Out ? Head : Tail, E_1);
            
            if( io )
            {
                Reconnect<Head>(F_0,E_0);
                Reconnect<Tail>(F_1,E_1);
            }
            else
            {
                Reconnect<Tail>(F_0,E_0);
                Reconnect<Head>(F_1,E_1);
            }
            
            DeactivateCrossing(C_2.Idx());
            
// We we want to change it into this in the case of io = Out and side == Left.
// All crossing's handedness can also be switched (at the same time!).
//
//                                      +
//                                     / ^
//                                    /   \
//                                   /     \
//                                  /       \  F_0
//                                 /         \
//                                /           \
//                               /             \
//                              +       O-----+ \
//                              |       ^     |  +
//                              v       | F_1 |  ^
//                            +-----------+   |  |
//                            |  Tangle   |   |  |
//                            +-----------+   |  |
//                     v----------------------+  |
//                     O       O----<----O       O
//                      \     /           ^     ^
//                       \   /             \   /
//                        \ /               \ /
//                     C_1 /                 / C_0
//                        / \               / \
//                       /   \             /   \
//                      v     v           /     \
//           O----<----O       O---->----O       O----<----O

            
            
            PD_PRINT("Changed data.");
            
            PD_VALPRINT("C_0",C_0.String());
            PD_VALPRINT("C_1",C_1.String());
            PD_VALPRINT("F_0",F_0.String());
            PD_VALPRINT("F_1",F_1.String());

            PD_PRINT("Horizontal reroute checks.");
            
            PD_ASSERT( C_0.ActiveQ());
            PD_ASSERT( C_1.ActiveQ());
            PD_ASSERT(!C_2.ActiveQ());
            
            PD_ASSERT(pd.CheckCrossing(C_0.Idx()));
            PD_ASSERT(pd.CheckCrossing(C_1.Idx()));

            PD_ASSERT(!E_0.ActiveQ());
            PD_ASSERT(!E_1.ActiveQ());
            PD_ASSERT( F_0.ActiveQ());
            PD_ASSERT( F_1.ActiveQ());
            PD_ASSERT(pd.CheckArc(F_0.Idx()));
            PD_ASSERT(pd.CheckArc(F_1.Idx()));
            
            
            PD_ASSERT(F_0(Tail) == C_0); // Only for io == Out, side == Left.
            PD_ASSERT(F_1(Head) == C_1); // Only for io == Out, side == Left.
            

            PD_PRINT("Horizontal reroute checks done.");
            
            PD_PRINT("Reidemeister_Ia_Horizontal_impl - twist move done.");
            
            return true;
            
//            wprint(ClassName()+"::Reidemeister_Ia_Horizontal_impl: Twist move possible.");
//
//            return false;
        }
    }
    
    return false;
}
