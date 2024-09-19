public:

// TODO: This method finds triangles. So it could also try to detect trefoil connect summands.

// TODO: This method search among 4-crossing configurations. So we could also try to detect figure-8-knot connect summands.

// TODO: Introduce a flag as return that allows us to tell what happened, so that we can make guesses on where to search next.

// TODO: Some of the checks are rather expensive. Maybe we can store that in the CrossingState and ArcState somehow. Would be cool if CrossingView and AecView would be able to reset this info whenever they are modified. That would require separate getters and setters.

template<bool single_compQ>
bool Simplify_3( const Int a )
{
    if( !ArcActiveQ(a) )
    {
        return false;
    }
    
    auto A   = GetArc( a );
    auto C_0 = GetCrossing( A(Tail) );
    auto C_1 = GetCrossing( A(Head ) );
    
    // 1-Crossing moves
    if( C_0 == C_1 )
    {
        if( C_0(Out,Right) == A )
        {
            Perform_Reidemeister_I<Right>(C_0);
        }
        else
        {
            Perform_Reidemeister_I<Left >(C_0);
        }
        
        return true;
    }
    
    const bool upQ_0 = (C_0(Out,Right) == A);
    const bool upQ_1 = (C_1(In ,Left ) == A);
    
    // TODO: Are these checks really performed 4 times?
    // The four points of compass for C_0.
    const Int n_0 = upQ_0 ? C_0(Out,Left ) : C_0(In ,Left );
//    const Int e_0 = upQ_0 ? C_0(Out,Right) : C_0(Out,Left );  // We know that it's A.Idx().
    const Int s_0 = upQ_0 ? C_0(In ,Right) : C_0(Out,Right);
    const Int w_0 = upQ_0 ? C_0(In ,Left ) : C_0(In ,Right);
    // The four points of compass for C_1.
    const Int n_1 = upQ_1 ? C_1(Out,Left ) : C_1(In ,Left );
    const Int e_1 = upQ_1 ? C_1(Out,Right) : C_1(Out,Left );
    const Int s_1 = upQ_1 ? C_1(In ,Right) : C_1(Out,Right);
//    const Int w_1 = upQ_1 ? C_1(In ,Left ) : C_1(In ,Right);  // We know that it's A.Idx().
    
    
//              n_0           n_1
//               O             O
//               |      A      |
//       w_0 O---X-->O-----O-->X-->O e_1
//               |C_0          |C_1
//               O             O
//              s_0           s_1

//    PD_ASSERT( e_0 == w_1 );
    
    // We want to check for TwistMove _before_ Reidemeister I because the TwistMove can remove both crossings.
    
    // Check for twist move. If successful, A, C_0, and C_1 are deleted.
    if( n_0 == s_1 )
    {
        // We have one of these two situations:
        
        // Case A:
        //
        //
        //               +-------------------------+
        //               |                         |
        //               |            n_1          |
        //               O             O####       |
        //               |      A      |   #       |
        //       w_0 O---X-->O---->O---X-->O e_1   |
        //               |C_0          |C_1        |
        //               O             O           |
        //              s_0            |           |
        //                             +-----------+
        //
        // No matter what the crossing at C_1 is, we can reroute as follows:
        //
        //               +-------------------------+
        //               |         +----------+    |
        //               |         |  n_1     |    |
        //               O         |   O####  |    |
        //               |         |   |   #  |    |
        //       w_0 O---X---------+   |   O<-+    |
        //               |C_0          |           |
        //               O             O           |
        //              s_0            |           |
        //                             +-----------+
        //
        // Now, no matter what the crossing at C_0 is, we can reroute as follows:
        //
        //
        //                         +----------+
        //                         |  n_1     |
        //                         |   O####  |
        //                         |   |   #  |
        //       w_0 O-------------+   |   O<-+
        //                             |  e_1
        //               O-------------+
        //              s_0
        //
        // A, C_0, and C_1 are deleted.
        // w_0 and e_1 are fused.
        // s_0 and n_1 are fused.
        
        
        
        // Case B
        //
        //   +-----------+
        //   |           |
        //   |           |            n_1
        //   |           O             O
        //   |           |      A      |
        //   |   w_0 O---X-->O---->O---X-->O e_1
        //   |       #   |C_0          |C_1
        //   |       ####O             O
        //   |          s_0            |
        //   |                         |
        //   +-------------------------+
        //
        // No matter what the crossing at C_0 is, we can reroute as follows:
        //
        //   +-----------+
        //   |           |
        //   |           |            n_1
        //   |           O             O
        //   |      w_0  |             |
        //   |    +--O   |   +---------X-->O e_1
        //   |    |  #   |C_0|         |C_1
        //   |    |  ####O   |         O
        //   |    |     s_0  |         |
        //   |    +----------+         |
        //   +-------------------------+
        //
        // Now, no matter what the crossing at C_0 is, we can reroute as follows:
        //
        //
        //
        //                            n_1
        //               +-------------O
        //          w_0  |
        //        +--O   |   +------------>O e_1
        //        |  #   |   |
        //        |  ####O   |
        //        |     s_0  |
        //        +----------+
        //
        // A, C_0, and C_1 are deleted.
        // w_0 and e_1 are fused.
        // s_0 and n_1 are fused.
        
        
        Reconnect( w_0, Head               , e_1 );
        Reconnect( s_0, upQ_1 ? Head : Tail, n_1 );
        
        DeactivateCrossing(C_0);
        DeactivateCrossing(C_1);
        DeactivateArc(A);
        
        ++twist_counter;
        return true;
    }
    
    // Check for twist move. If successful, A, C_0, and C_1 are deleted.
    if( s_0 == n_1 )
    {
        // We have one of these two situations:
        //
        // Case A:
        //
        //                             +-----------+
        //                             |           |
        //              n_0            |           |
        //               O             O           |
        //               |      A      |           |
        //       w_0 O---X-->O---->O---X-->O e_1   |
        //               |C_0          |C_1#       |
        //               O             O####       |
        //               |            s_1          |
        //               |                         |
        //               +-------------------------+
        //
        // No matter what the crossing at C_1 is, we can reroute as follows:
        //
        //                             +-----------+
        //                             |           |
        //              n_0            |           |
        //               O             O           |
        //               |             |  e_1      |
        //       w_0 O---X---------+   |   O<-+    |
        //               |C_0      |   |   #  |    |
        //               O         |   O####  |    |
        //               |         |  s_1     |    |
        //               |         +----------+    |
        //               +-------------------------+
        //
        // Now, no matter what the crossing at C_0 is, we can reroute as follows:
        //
        //
        //
        //              n_0
        //               O-------------+
        //                             |  e_1
        //       w_0 O-------------+   |   O--+
        //                C_0      |   |   #  |
        //                         |   O####  |
        //                         |  s_1     |
        //                         +----------+
        //
        //
        // A, C_0, and C_1 are deleted.
        // w_0 and e_1 are fused.
        // n_0 and s_1 are fused.
        
        // Case B is fully analogous. I skip it.
                
        Reconnect( w_0, Head               , e_1 );
        Reconnect( n_0, upQ_0 ? Head : Tail, s_1 );
        
        DeactivateArc(A);
        DeactivateCrossing(C_0);
        DeactivateCrossing(C_1);
        
        ++twist_counter;
        return true;
    }
    
    // Next we check for Reidemeister_I at crossings C_0 and C_1.
    // This will also remove some unpleasant cases for the Reidemeister II and Ia moves.

    // Reidemeister_I at crossing C_0
    if( n_0 == w_0 )
    {
        if( s_0 != s_1 )
        {
            // Looks like this:
            //
            //
            //           +---+           |
            //           |   |     A     v
            //           +-->X---------->X-->
            //               |C_0        |C_1
            //               |           v
            
            Reconnect(A.Idx(),Tail,s_0);
        }
        
        else // if( s_0 == s_1 )
        {
            // A second Reidemeister I move can be performed.
            // Looks like this:
            //
            //
            //           +---+           |
            //           |   |     A     v
            //           +-->X---------->X-->
            //               |C_0        |C_1
            //               |           v
            //               +-----<-----+
            
            if( e_1 == n_1 )
            {
                DeactivateArc(e_1);
                
                ++unlink_count;
            }
            else
            {
                Reconnect(n_1,Head,e_1);
            }

            DeactivateArc(s_0);
            Deactivate(A  );
            Deactivate(C_1);
            ++R_I_counter;
        }
        
        // In any case, w_0 and C_0 are removed.
        DeactivateArc(w_0);
        Deactivate(C_0);
        ++R_I_counter;
        
        return true;
    }
    
    // Reidemeister_I at crossing C_0
    if( s_0 == w_0 )
    {
        if( n_0 != n_1 )
        {
            // A second Reidemeister I move can be performed.
            // Looks like this:
            //
            //               |           |
            //               |     A     |
            //           +-->X---------->X-->
            //           |   |C_0        ^C_1
            //           +<--+           |
            
            Reconnect(A.Idx(),Tail,n_0);
        }
        else // if( n_0 == n_1 )
        {
            // A second Reidemeister I move can be performed.
            // Looks like this:
            //
            //               +-----<-----+
            //               |           |
            //               |     A     |
            //           +-->X---------->X-->
            //           |   |C_0        ^C_1
            //           +<--+           |
            
            if( e_1 == s_1 )
            {
                DeactivateArc(e_1);
                
                ++unlink_count;
            }
            else
            {
                Reconnect(s_1,Head,e_1);
            }
            
            Deactivate(A  );
            DeactivateArc(n_0);
            Deactivate(C_1);
            ++R_I_counter;
        }
        
        // In any case, w_0 and C_0 are removed.
        DeactivateArc(w_0);
        Deactivate(C_0);
        ++R_I_counter;
        
        return true;
    }
    
    // Reidemeister_I at crossing C_1
    if( s_1 == e_1 )
    {
        if( n_0 != n_1 )
        {
            // Looks like this:
            //
            //               |           ^
            //               |     A     |
            //            -->X---------->X-->+
            //               |C_0        |   |
            //               v           +---+
            
            Reconnect(A.Idx(),Head,n_1);
        }
        else // if( n_0 == n_1 )
        {
            // A second Reidemeister I move can be performed.
            // Looks like this:
            //
            //               +-----<-----+
            //               |           ^
            //               |     A     |
            //            -->X---------->X-->+
            //               |C_0        |   |
            //               v           +---+
            
            if( w_0 == s_0 )
            {
                DeactivateArc(w_0);
                
                ++unlink_count;
            }
            else
            {
                Reconnect(w_0,Head,s_0);
            }
            
            DeactivateArc(n_1);
            Deactivate(A  );
            Deactivate(C_0);

            ++R_I_counter;
        }
        
        // In any case, e_1 and C_1 are removed.
        DeactivateArc(e_1);
        Deactivate(C_1);
        ++R_I_counter;
        
        return true;
    }

    // Reidemeister_I at crossing C_1
    if( n_1 == e_1 )
    {
        if( s_0 != s_1 )
        {
            // A second Reidemeister I move can be performed.
            // Looks like this:
            //
            //               ^           +---+
            //               |     A     |   |
            //            -->X---------->X-->+
            //               |C_0        |C_1
            //               |           v
            
            Reconnect(A.Idx(),Head,s_1);
            Deactivate(C_1);
        }
        else // if( s_0 == s_1 )
        {
            // Looks like this:
            //
            //               ^           +---+
            //               |     A     |   |
            //            -->X---------->X-->+
            //               |C_0        |C_1
            //               |           v
            //               +-----<-----+
            
            if( w_0 == n_0 )
            {
                DeactivateArc(w_0);
                
                ++unlink_count;
            }
            else
            {
                Reconnect(w_0,Head,n_0);
            }
            
            DeactivateArc(s_1);
            Deactivate(A  );
            Deactivate(C_0);
            ++R_I_counter;
        }
        
        // In any case, e_1 and C_1 are removed.
        DeactivateArc(e_1);
        Deactivate(C_1);
        ++R_I_counter;
        
        return true;
    }
    
    const bool overQ_0 = (upQ_0 == ( C_0.State() == CrossingState::LeftHanded ));
    const bool overQ_1 = (upQ_1 == ( C_1.State() == CrossingState::LeftHanded ));

    
    // Deal with the case that A is part of a loop of length 2.
    if constexpr( !single_compQ )
    {
        if( w_0 == e_1 )
        {
            if( overQ_0 == overQ_1 )
            {
                // We have a true over- or underloop
                // Looks like this:
                //
                //       +-------------------+              +-------------------+
                //       |                   |              |                   |
                //       |   O###########O   |              |   O###########O   |
                //       |   |     A     |   |              |   |     A     |   |
                //       +-->|---------->|-->+      or      +-->----------->--->+
                //           |C_0        |C_1                   |C_0        |C_1
                //           |           |                      |           |
                //
                //                 or                                 or
                //
                //           |           |                      |           |
                //           |     A     |                      |     A     |
                //       +-->|---------->|-->+      or      +-->----------->--->+
                //       |   |C_0        |C_1|              |   |C_0        |C_1|
                //       |   O###########O   |              |   O###########O   |
                //       |                   |              |                   |
                //       +-------------------+              +-------------------+
                
                wprint("Feature of detecting over- or underloops is not tested, yet.");
                
                Reconnect(s_0, upQ_0 ? Head : Tail, n_0);
                Reconnect(s_1, upQ_1 ? Head : Tail, n_1);
                DeactivateArc(w_0);
                DeactivateArc(e_1);
                Deactivate(A  );
                Deactivate(C_0);
                Deactivate(C_1);
                // TODO: Invent some counter here and increment it.
                ++unlink_count;
                
                return true;
            }
            else // if( overQ_0 != overQ_1 )
            {
                // Looks like this:
                //
                //       +-------------------+              +-------------------+
                //       |                   |              |                   |
                //       |   O###########O   |              |   O###########O   |
                //       |   |     A     |   |              |   |     A     |   |
                //       +-->|---------->--->+      or      +-->----------->|-->+
                //           |C_0        |C_1                   |C_0        |C_1
                //           |           |                      |           |
                //
                //                 or                                 or
                //
                //           |           |                      |           |
                //           |     A     |                      |     A     |
                //       +-->----------->|-->+      or      +-->|---------->--->+
                //       |   |C_0        |C_1|              |   |C_0        |C_1|
                //       |   O###########O   |              |   O###########O   |
                //       |                   |              |                   |
                //       +-------------------+              +-------------------+
                
                
                // TODO: If additionally the endpoints of n_0 and n_1 or the endpoints of s_0 and s_1 coincide, there are subtle ways to remove the crossing there:

                //       +-------------------+              +-------------------+
                //       |                   |              |   +--------------+|
                //       |                   |              |   |              ||
                //       |   O##TANGLE###O   |              |   O##TANGLE###O  ||
                //       |   |           |   |              |              /   ||
                //       |   |           |   |              |   +---------+    ||
                //       |   |     A     |   |              |   |     A     +--+|
                //       +-->|----------->---+     ==>      +-->------------|---+
                //           |C_0        |C_1                   |C_0        |C_1
                //           O           O                      O           O
                //            \         /                        \         /
                //             \       /                          \       /
                //              \     /                            \     /
                //               O   O                              +   +
                //                \ /                               |   |
                //                 \ C_2                            |   |2
                //                / \                               |   |
                //               O   O                              O   O
                //
                //       +-------------------+              +-------------------+
                //       |                   |              |                   |
                //       |   O##TANGLE###O   |              |   O##TANGLE###O   |
                //       |   |           |   |              |   |           |   |
                //       |   |     A     |   |   untwist    |   |     A     |   |
                //       +-->------------|---+     ==>      +-->------------|---+
                //           |C_0        |C_1                   |C_0        |C_1
                //           O           O                      O           O
                //            \         /                        \         /
                //             \       /                          \       /
                //              \     /                            \     /
                //               O   O                              +   +
                //                \ /                               |   |
                //                 \ C_2                            |   |2
                //                / \                               |   |
                //               O   O                              O   O
                
                // But this comes at the price of an additional crossing load, and it might not be very likely. Also, I am working only on connected diagrams, so I have no test code for this.
                
                return false;
            }
            
        }
    }
    

    if( overQ_0 == overQ_1 )
    {
//               |     A     |             |     A     |
//            -->|---------->|-->   or  -->----------->--->
//               |C_0        |C_1          |C_0        |C_1
        
        // Reidemeister II move.
        if( n_0 == n_1 )
        {
//               +-----------+             +-----------+
//               |           |             |           |
//               |     A     |             |     A     |
//            -->|---------->|-->   or  -->----------->--->
//               |C_0        |C_1          |C_0        |C_1
            
            // TODO: If the endpoints of s_0 and s_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
            // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
            // TODO: On the other hand: we need to reference to the two end point anyways to do the reconnecting right...
            
            
            // These case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
            PD_ASSERT( w_0 != e_1 );
            PD_ASSERT( w_0 != s_0 );
            PD_ASSERT( e_1 != s_1 );
            
            // .. so this is safe:
            Reconnect( w_0, Head, e_1);
            
            if constexpr( !single_compQ )
            {
                if( s_0 == s_1 )
                {
                    ++unlink_count;
                    DeactivateArc(s_0);
                }
                else
                {
                    Reconnect( s_0, upQ_0 ? Head : Tail, s_1 );
                }
            }
            else
            {
                Reconnect(s_0, upQ_0 ? Head : Tail, s_1 );
            }
            
            
            DeactivateArc(n_0);
            Deactivate(A  );
            Deactivate(C_0);
            Deactivate(C_1);
            ++R_II_counter;
            
            return true;
        }
        
        // Reidemeister II move.
        if( s_0 == s_1 )
        {
            // TODO: If the endpoints of n_0 and n_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
            // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
            // TODO: On the other hand: we need to reference to the two end point anyways to do the reconnecting right...
            
//
//               |     A     |             |     A     |
//            -->|---------->|-->   or  -->----------->--->
//               |C_0        |C_1          |C_0        |C_1
//               |           |             |           |
//               +-----------+             +-----------+
            
            // These case w_0 == e_1, w_0 == n_0, e_1 == n_1 are ruled out already...
            PD_ASSERT( w_0 != e_1 );
            PD_ASSERT( w_0 != n_0 );
            PD_ASSERT( e_1 != n_1 );
            
            // .. so this is safe:
            Reconnect(w_0,Head,e_1);
            
            // We have already checked this above.
            PD_ASSERT( n_0 != n_1 );
            Reconnect(n_0, upQ_0 ? Tail : Head, n_1 );
            DeactivateArc(s_0);
            Deactivate(A  );
            Deactivate(C_0);
            Deactivate(C_1);
            ++R_II_counter;

            return true;
        }
        
        
        // Try Reidemeister IIa moves.
        
        // TODO: If n_0 == n_1 and if the endpoints of s_0 and s_1 coincide, we can remove 3 crossings.
        // TODO: Price: Load 1 additional crossing for test; load 2 more crossings if successful
        
        // TODO: If s_0 == s_1 and if the endpoints of n_0 and n_1 coincide, we can remove 3 crossings.
        // TODO: Price: Load 1 additional crossing for test; load 2 more crossings if successful
        
        
        // TODO: If the endpoints of s_0 and s_1 coincide and if the endpoints of n_0 and n_1 coincide, we can remove 2 crossings by R_IIa move.
        // TODO: Price: Load 2 crossings for test. If successful, 4 arcs and 4 crossings have to be loaded.

        PD_ASSERT(s_0 != s_1);
        PD_ASSERT(n_0 != n_1);
        PD_ASSERT(w_0 != e_1);
        
        //               w_3     n_3             w_3     n_3
        //                  O   O                   O   O
        //                   \ /                     \ /
        //                    X C_3                   X C_3
        //                   / \                     / \
        //                  O   O                   O   O
        //             n_0 /     e_3           n_0 /     e_3
        //                /                       /
        //               O         O             O         O
        //               |    A    |             |    A    |
        //            O--|->O---O--|->O   or  O---->O---O---->O
        //               |C_0      |C_1          |C_0      |C_1
        //               O         O             O         O
        //                \                       \
        //             s_0 \                   s_0 \
        //                  O   O                   O   O
        //                   \ /                     \ /
        //                    X C_2                   X C_2
        //                   / \                     / \
        //                  O   O                   O   O
        //               w_2     s_2             w_2     s_2
        
        // Recall: Out == 0; In == 1; Left == 0; Right == 1;
        
        
        auto C_2 = Crossing( A_cross(s_0, upQ_0 ? Tail : Head ) );
        
        const bool b_2 = (s_0 != C_2( !upQ_0, Left ));
        
        PD_ASSERT( s_0 == C_2( !upQ_0, b_2 ) );
        
        const Int e_2 = C_2(  b_2  ,  upQ_0 );
        const Int s_2 = C_2(  upQ_0, !b_2   ); // opposite to n_2 == s_0.
        const Int w_2 = C_2( !b_2  , !upQ_0 ); // opposite to e_2
        
//        // Long version of the above:
//        const Int e_2 = upQ_0
//            ? ( s_0 == C_2(Out,Left ) ? C_2(Out,Right) : C_2(In ,Right) )
//            : ( s_0 == C_2(In ,Left ) ? C_2(Out,Left ) : C_2(In ,Left ) );

        auto C_3 = Crossing( A_cross(n_0, upQ_0 ? Head : Tail ) );
        
        const bool b_3 = (n_0 != C_3( upQ_0, Left ));
        
        PD_ASSERT( n_0 == C_3( upQ_0, b_3 ) );
        
        const Int e_3 = C_3(    b_3,  upQ_0 );
        const Int n_3 = C_3( !upQ_0, !b_3   ); // opposite to s_3 == n_0.
        const Int w_3 = C_3(   !b_3, !upQ_0 ); // opposite to e_3
        
//        // Long version of the above:
//        const Int e_3 = upQ_0
//            ? ( n_0 == C_3(In ,Left ) ? C_3(In ,Right) : C_3(Out,Right) )
//            : ( n_0 == C_3(Out,Left ) ? C_3(In ,Left ) : C_3(Out,Left ) );
        

        if( (e_2 == s_1) && (e_3 == n_1) )
        {
            //               w_3     n_3             w_3     n_3
            //                  O   O                   O   O
            //                   \ /                     \ /
            //                    X C_3                   X C_3
            //                   / \                     / \
            //                  O   O                   O   O
            //             n_0 /     \ n_1         n_0 /     \ n_1
            //                /       \               /       \
            //               O         O             O         O
            //               |    A    |             |    A    |
            //            O--|->O---O--|->O   or  O---->O---O---->O
            //               |C_0      |C_1          |C_0      |C_1
            //               O         O             O         O
            //                \       /               \       /
            //             s_0 \     / s_1         s_0 \     / s_1
            //                  O   O                   O   O
            //                   \ /                     \ /
            //                    X C_2                   X C_2
            //                   / \                     / \
            //                  O   O                   O   O
            //               w_2     s_2             w_2     s_2
            
            if( (upQ_0 == upQ_1) == OppositeHandednessQ(C_0,C_1) )
            {
                // R_IIa move can definitely be made.
                
                Reconnect( s_0, upQ_0 ? Tail : Head, w_2 );
                Reconnect( n_0, upQ_0 ? Head : Tail, w_3 );
                Reconnect( s_1, upQ_1 ? Tail : Head, s_2 );
                Reconnect( n_1, upQ_1 ? Head : Tail, n_3 );
                Deactivate(C_2);
                Deactivate(C_3);
                ++R_IIa_counter;
                return true;
            }
        }
        
    }
    else // ( overQ_0 != overQ_1 )
    {
//           |     A     |
//        -->|---------->--->
//           |C_0        |C_1
//
//           |     A     |
//        -->----------->|-->
//           |C_0        |C_1
        
        
        // Checking for Reidemeister Ia moves.
        
        if( n_0 == n_1 )
        {
            // TODO: Maybe Reidemeister Ia move.
            // TODO: We might also insert a check for a trefoil summand.
                        

//               +-----------+
//               |           |
//               |     A     |
//            -->|---------->--->
//               |C_0        |C_1
//
//               +-----------+
//               |           |
//               |     A     |
//            -->----------->|-->
//               |C_0        |C_1
            
            auto C_2 = Crossing( A_cross(s_0, upQ_0 ? Tail : Head ) );
                                
            // TODO: Check whether C_2 contains arc s_1.
            // TODO: return false, if not.
            
//           R_Ia cases:
//
//               +-----------+             +-----------+
//               |           |             |           |
//               |     A     |             |     A     |
//            -->----------->|-->       -->|---------->--->
//               |C_0        |C_1          |C_0        |C_1
//               O---O   O---O             O---O   O---O
//                    \ /                       \ /
//                     / C_2                     \  C_2
//                    / \                       / \
            
//           Potential trefoil cases:
//
//               +-----------+             +-----------+
//               |           |             |           |
//               |     A     |             |     A     |
//            -->----------->|-->       -->|---------->--->
//               |C_0        |C_1          |C_0        |C_1
//               O---O   O---O             O---O   O---O
//                    \ /                       \ /
//                     \ C_2                     /  C_2
//                    / \                       / \
            
//          TODO: Beware, two further interesting cases!
//          TODO: Beware, this can only happen, if the planar diagram has multiple components!
//
//               +-----------+             +-----------+
//               |           |             |           |
//            C_0|     A     |C_1       C_0|     A     |C_1
//            -->----------->|-->       -->|---------->--->
//               O   O###O   O             O   O###O   O
//               |    \ /    |             |    \ /    |
//           s_0 |     X C_2 | s_1         |     X C_2 | s_1
//               |    / \    |             |    / \    |
//               +---O   O---+             +---O   O---+
//
//          These can be rerouted to the following two situations:
//               +-----------+             +-----------+
//               |           |             |           |
//            C_0|     A     |C_1       C_0|     A     |C_1
//            -->----------->|-->       -->|---------->--->
//               O   +-------O             O   +-------O
//               |   |     s_1             |   |     s_1
//           s_0 |   O###O             s_0 |   O###O
//               |       |                 |       |
//               +-------+                 +-------+
//
//          No matter how C_2 is handed, we fuse s_0 and s_1 with their opposite arcs accross C_2.
//
//          C_2 can be deactivated.
//          This will probably happen seldomly,
//          but is has virtually no additional costs, once C_2 is loaded.

        }
        
        if( s_0 == s_1 )
        {
            // TODO: Maybe Reidemeister Ia move.

//               |     A     |
//            -->|---------->--->
//               |C_0        |C_1
//               |           |
//               +-----------+
//
//               |     A     |
//            -->----------->|-->
//               |C_0        |C_1
//               |           |
//               +-----------+
            
            auto C_3 = Crossing( A_cross(n_0, upQ_0 ? Head : Tail ) );
            
            // TODO: Check whether C_3 contains arc n_1.
            // TODO: return false, if not.
            
            // TODO: We might also insert a check for a trefoil summand.
        }

        auto C_2 = Crossing( A_cross(s_0, upQ_0 ? Tail : Head ) );
        auto C_3 = Crossing( A_cross(n_0, upQ_0 ? Head : Tail ) );
        
        // TODO: Check whether C_2 contains arc s_1.
        // TODO: Check whether C_3 contains arc n_1.
        
        // TODO: R_IIa possible if closed both above and below. (Caution: Contains bad cases!)
        
// The only two good cases here:
        
        
//              O   O                     O   O
//               \ /                       \ /
//                / C_3                     \ C_3
//               / \                       / \
//              O   O                     O   O
//             /     \                   /     \
//            /       \                 /       \
//           O         O               O         O
//           |    A    |               |    A    |
//        O--|->O-->O---->O         O---->O-->O--|->O
//           |C_0      |C_1            |C_0      |C_1
//           O         O               O         O
//            \       /                 \       /
//             \     /                   \     /
//              O   O                     O   O
//               \ /                       \ /
//                \ C_2                     / C_2
//               / \                       / \
//              O   O                     O   O
    }

}
