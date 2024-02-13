void Reidemeister_II_Vertical( const Int c_0, const Int c_1 )
{
    PD_print("Reidemeister_II_Vertical  ( "+ToString(c_0)+", "+ToString(c_1)+" )");
    
    // c_0 == c_1  should be made impossible by the way we call this function.
    PD_assert( c_0 != c_1 );
    
    // This is the central assumption here.
    PD_assert( C_arcs[In ][Left ][c_1] == C_arcs[Out][Left ][c_0] );
    PD_assert( C_arcs[In ][Right][c_1] == C_arcs[Out][Right][c_0] );
    
    const Int a = C_arcs[Out][Left ][c_0];
    const Int b = C_arcs[Out][Right][c_0];
    
    const Int e_0 = C_arcs[In ][Left ][c_0];
    const Int e_1 = C_arcs[In ][Right][c_0];
    const Int e_2 = C_arcs[Out][Right][c_1];
    const Int e_3 = C_arcs[Out][Left ][c_1];
    
    PD_assert( e_0 != e_2 );    // Should be impossible because of topology.
    PD_assert( e_1 != e_3 );    // Should be impossible because of topology.
    
    if( e_0 != e_3 )
    {
        if( e_1 != e_2 )
        {
            // The generic case.
            
            //  e_0 != e_3 && e_1 != e_2
            //
            //
            //               d_3  O----<----O       O---->----O  d_2
            //                        e_3    ^     ^    e_2
            //                                \   /
            //                                 \ /
            //                              c_1 X
            //                                 / \
            //                                /   \
            //                               /     \
            //  C_arcs[In ][Left ][c_1]     O       O    C_arcs[In ][Right][c_1]
            //                          \\  ^       ^  //
            //                            a |       | b
            //                          //  |       |  \\
            //  C_arcs[Out][Left ][c_0]     O       O    C_arcs[Out][Right][c_0]
            //                               ^     ^
            //                                \   /
            //                                 \ /
            //                              c_0 X
            //                                 / \
            //                                /   \
            //                        e_0    /     \    e_1
            //               d_0  O---->----O       O----<----O  d_1
            
            Reconnect( a, Tail, e_0 );
            Reconnect( a, Tip , e_3 );
            Reconnect( b, Tail, e_1 );
            Reconnect( b, Tip , e_2 );

            DeactivateArc(e_0);
            DeactivateArc(e_1);
            DeactivateArc(e_2);
            DeactivateArc(e_3);
            
            goto exit;
        }
        else
        {
            //  e_0 != e_3 && e_1 == e_2
            //
            //
            //            O----<----O       O---->----+
            //                e_3    ^     ^           \
            //                        \   /             \
            //                         \ /               \
            //                      c_1 X                 |
            //                         / \                |
            //                        /   \               |
            //                       /     \              |
            //                      O       O             |
            //                      ^       ^        e_2  |
            //                    a |       | b       =   |
            //                      |       |        e_1  |
            //                      O       O             |
            //                       ^     ^              |
            //                        \   /               |
            //                         \ /                |
            //                      c_0 X                 |
            //                         / \               /
            //                        /   \             /
            //                e_0    /     \           /
            //            O---->----O       O----<----+
        }
        
        Reconnect(b, Tail, e_0);
        Reconnect(b, Tip , e_3);
        
        ++unlink_count;
        
        DeactivateArc(a);
        DeactivateArc(e_1);
        
        goto exit;
    }
    else if (e_1 != e_2)
    {
        //  e_0 == e_3 && e_1 != e_2
        //
        //
        //            +----<----O       O---->----O
        //           /           ^     ^    e_2
        //          /             \   /
        //         /               \ /
        //        |             c_1 X
        //        |                / \
        //        |               /   \
        //        |              /     \
        //        |             O       O
        //        | e_1         ^       ^
        //        |  =        a |       | b
        //        | e_0         |       |
        //        |             O       O
        //        |              ^     ^
        //        |               \   /
        //        |                \ /
        //        |             c_0 X
        //         \                / \
        //          \             /   \
        //           \           /     \    e_1
        //            +---->----O       O----<----O
        
        Reconnect(a, Tail, e_1);
        Reconnect(a, Tip , e_2);
        
        ++unlink_count;
        
        DeactivateArc(b);
        DeactivateArc(e_0);
        
        goto exit;
    }
    else
    {
        //  e_0 == e_3 && e_1 == e_2
        //
        //
        //            +----<----O       O---->----+
        //           /           ^     ^           \
        //          /             \   /             \
        //         /               \ /               \
        //        |             c_1 X                 |
        //        |                / \                |
        //        |               /   \               |
        //        |              /     \              |
        //        |             O       O             |
        //        | e_1         ^       ^        e_2  |
        //        |  =        a |       | b       =   |
        //        | e_0         |       |        e_1  |
        //        |             O       O             |
        //        |              ^     ^              |
        //        |               \   /               |
        //        |                \ /                |
        //        |             c_0 X                 |
        //         \               / \               /
        //          \             /   \             /
        //           \           /     \           /
        //            +---->----O       O----<----+
                
        unlink_count += Int(2);
        
        DeactivateArc(a);
        DeactivateArc(b);
        DeactivateArc(e_1);
        DeactivateArc(e_2);
        
        goto exit;
    }
 
exit:
    
    DeactivateCrossing(c_0);
    DeactivateCrossing(c_1);
    
    ++R_II_counter;
    
//    CheckAll();

} // Reidemeister_II_Vertical
