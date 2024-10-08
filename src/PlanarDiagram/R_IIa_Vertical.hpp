private:

bool Reidemeister_IIa_Vertical( const Int c_0 )
{
    if( !CrossingActiveQ(c_0) )
    {
        return false;
    }
    
    PD_PRINT("Reidemeister_IIa_Vertical");

#ifdef PD_COUNTERS
    ++R_IIa_check_counter;
#endif
    
    auto C_0 = GetCrossing( c_0 );
    
    auto B_1 = GetArc( C_0(Out,Left ) );
    auto A_1 = GetArc( C_0(Out,Right) );
    auto A_0 = GetArc( C_0(In ,Left ) );
    auto B_0 = GetArc( C_0(In ,Right) );
    auto C_1 = GetCrossing( A_1(Head) );
    auto A_2 = NextArc(A_1);
    auto C_2 = GetCrossing( A_2(Head) );
    
    if( SameHandednessQ(C_0,C_2) )
    {
        // Not what we are looking for.
        return false;
    }
    else
    {
        auto A_3 = GetArc( C_2(Out,Left ) );
        auto B_3 = GetArc( C_2(Out,Right) );
        auto B_2 = GetArc( C_2(In ,Left ) );
        auto C_3 = GetCrossing( B_1(Head) );
        
        if( C_1 == C_3  )
        {
            // Not what we are looking for.
            return false;
        }
        
        if( OppositeHandednessQ(C_1,C_3) )
        {
            return false;
        }
        
        if( C_3 != B_2(Tail) )
        {
            // Not what we are looking for.
            return false;
        }

// Now C_1 and C_3 should share an arc and the situation should look like this.
// (The handedness of crossings c_0 and c_1 might be flipped.)
//
//          A_3 O       O B_3
//               ^     ^
//                \   /
//                 \ /
//                  \ C_2
//                 / \
//                /   \
//               /     \
//              O       O
//         B_2 ^         ^ A_2
//            /           \
//           O             O
//           ^             ^
//           |C_3          |C_1
//    ---O---X---O-----O---X---O---
//           |             |
//           |             |
//           O             O
//        B_1 ^           ^ A_1
//             \         /
//              O       O
//               ^     ^
//                \   /
//                 \ /
//                  / C_0
//                 / \
//                /   \
//               /     \
//          A_0 O       O B_0
//
//
        
        
        PD_ASSERT( C_0(In ,Left ) == A_0 );
        PD_ASSERT( C_0(Out,Right) == A_1 );
        
        PD_ASSERT( C_0(In ,Right) == B_0 );
        PD_ASSERT( C_0(Out,Left ) == B_1 );

        PD_ASSERT( C_2(In ,Right) == A_2 );
        PD_ASSERT( C_2(Out,Left ) == A_3 );
        
        PD_ASSERT( C_2(In ,Left ) == B_2 );
        PD_ASSERT( C_2(Out,Right) == B_3 );
        
        PD_ASSERT( A_0(Head) == C_0 );
        PD_ASSERT( B_0(Head) == C_0 );
        
        PD_ASSERT( A_1(Head) == C_1 );
        PD_ASSERT( A_2(Tail) == C_1 );
        
        PD_ASSERT( B_1(Head) == C_3 );
        PD_ASSERT( B_2(Tail) == C_3 );
        
        PD_ASSERT( A_3(Tail) == C_2 );
        PD_ASSERT( B_3(Tail) == C_2 );
        
        PD_ASSERT( OppositeHandednessQ(C_0,C_2) );
        PD_ASSERT( SameHandednessQ(C_1,C_3) );
        
        
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
        
        Reconnect<Head>(A_0,B_1);
        Reconnect<Head>(B_0,A_1);
        
        Reconnect<Tail>(A_3,B_2);
        Reconnect<Tail>(B_3,A_2);
        
        DeactivateCrossing(C_0.Idx());
        DeactivateCrossing(C_2.Idx());
        
        // Now c_1 and c_3 should share an arc and the situation should look like this.
        // (The handedness of crossings c_0 and c_1 might be flipped.)
        //
        //           ^             ^
        //           |             |
        //       E_3 |             | B_3
        //           |             |
        //           |             |
        //           O             O
        //           ^             ^
        //           |C_3          |C_1
        //    ---O---X---O-----O---X---O---
        //           |             |
        //           |             |
        //           O             O
        //           ^             ^
        //           |             |
        //           |             |
        //       E_0 |             | B_0
        //           |             |
        //
        //
        
        
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
        
        PD_ASSERT( SameHandednessQ(C_1,C_3) );
        
        PD_ASSERT( A_0(Head) == C_3 );
        PD_ASSERT( B_0(Head) == C_1 );
        
        PD_ASSERT( A_3(Tail) == C_3 );
        PD_ASSERT( B_3(Tail) == C_1 );
        
        ++R_IIa_counter;
        
        return true;
    }
}
