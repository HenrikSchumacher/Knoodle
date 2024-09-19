public:

template<bool mult_compQ = true>
bool SimplifyArc( const Int a_ )
{
    if( !ArcActiveQ(a_) ) return false;

    a = a_;
    
    PD_DPRINT( "SimplifyArc " + ArcString(a) );
    
    AssertArc(a);

    c_0 = A_cross(a,Tail);
    AssertCrossing(c_0);

    c_1 = A_cross(a,Head);
    AssertCrossing(c_1);
    
    load_c_0();

    if( a_is_loop() ) return true;
    
    /*              n_0           n_1
     *               O             O
     *               |      a      |
     *       w_0 O---X-->O-----O-->X-->O e_1
     *               |c_0          |c_1
     *               O             O
     *              s_0           s_1
     */
     
    load_c_1();
    
    // We want to check for twist move _before_ Reidemeister I because the twist move can remove both crossings.
    
    if( twist_at_a() ) return true;
    
    // TODO: Debug the following, too.
//
//    // Next we check for Reidemeister_I at crossings c_0 and c_1.
//    // This will also remove some unpleasant cases for the Reidemeister II and Ia moves.
//    
//    if( RI_at_c_0() ) return true;
//    
//    if( RI_at_c_1() ) return true;
//    
//    // Neglecting asserts, this is the only time we access C_state[c_0].
//    // Whether the vertical strand at c_0 goes over.
//    o_0 = (u_0 == ( C_state[c_0] == CrossingState::LeftHanded ));
//    
//    // Neglecting asserts, this is the only time we access C_state[c_1].
//    // Whether the vertical strand at c_1 goes over.
//    o_1 = (u_1 == ( C_state[c_1] == CrossingState::LeftHanded ));
//
//    // Deal with the case that a is part of a loop of length 2.
//    // This can only occur if  the diagram has a more than one component.
//    if constexpr( mult_compQ )
//    {
//        // Caution: This requires o_0 and o_1 to be defined already.
//        if( a_is_2loop() ) return true;
//    }
//    
//    if( o_0 == o_1 )
//    {
//        /*       |     a     |             |     a     |
//         *    -->|---------->|-->   or  -->----------->--->
//         *       |c_0        |c_1          |c_0        |c_1
//         */
//        
//        return strands_same_side<mult_compQ>();
//    }
//    else
//    {
//        /*       |     a     |             |     a     |
//         *    -->|---------->--->   or  -->----------->|-->
//         *       |c_0        |c_1          |c_0        |c_1
//         */
//        
//        return strands_opposite_sides();
//    }
    
    return false;
}

public:

void AssertArc( const Int a )
{
    PD_ASSERT(ArcActiveQ(a));
    PD_ASSERT(CheckArc  (a));
#ifndef PD_DEBUG
    (void)a;
#endif
}

void AssertCrossing( const Int c )
{
    PD_ASSERT(CrossingActiveQ(c));
    PD_ASSERT(CheckCrossing(c));
#ifndef PD_DEBUG
    (void)c;
#endif
}

private:
    
// Variables shared among simplification routines.
// These are used as local variables to the simplification and thus they are _excluded_ from value semantics.

    Int a;
    Int c_0;
    Int c_1;
    Int c_2;
    Int c_3;
    
// For crossing c_0
    Int n_0;
//        Int e_0; // always = a.
    Int s_0;
    Int w_0;

// For crossing c_1
    Int n_1;
    Int e_1;
    Int s_1;
//        Int w_1; // always = a.
    
    // For crossing c_2
//        Int n_2; // always = s_0.
    Int e_2;
    Int s_2;
    Int w_2;
    
// For crossing c_3
    Int n_3;
    Int e_3;
//        Int s_3; // always = n_0.
    Int w_3;
    
// Whether the vertical strand at corresponding crossing goes over.
    bool o_0;
    bool o_1;
    bool o_2;
    bool o_3;
    
// Whether the vertical strand at corresponding crossing goes up.
    bool u_0;
    bool u_1;

void load_c_0()
{
    /*              n_0
     *               O
     *               |
     *       w_0 O---X-->O a
     *               |c_0
     *               O
     *              s_0
     */
    
    // Whether the vertical strand at c_0 points upwards.
    u_0 = (C_arcs(c_0,Out,Right) == a);
    
    n_0 = C_arcs( c_0, !u_0,  Left  );
    s_0 = C_arcs( c_0,  u_0,  Right );
    w_0 = C_arcs( c_0,  In , !u_0   );
    
    AssertArc(n_0);
    AssertArc(s_0);
    AssertArc(w_0);
    
    // Remark: We are _not_ loading o_0 here, because this is not needed for the Reidemeister I moves. This may safe a cache miss.
}

void load_c_1()
{
    // Whether the vertical strand at c_1 points upwards.
    const bool u_1 = (C_arcs(c_1,In ,Left ) == a);
    
    n_1 = C_arcs(c_1,!u_1, Left );
    e_1 = C_arcs(c_1, Out, u_1  );
    s_1 = C_arcs(c_1, u_1, Right);
    
    AssertArc(n_1);
    AssertArc(e_1);
    AssertArc(s_1);
    
    // Remark: We are _not_ loading o_1 here, because this is not needed for the Reidemeister I moves. This may safe a cache miss.
}

void load_c_2()
{
    // TODO: Check this thoroughly!
    
    /*          n_0       n_1
     *           O         O
     *           |    a    |
     *    w_0 O--X->O-->O--X->O e_1
     *           |c_0      |c_1
     *           O         O
     *       s_0 |        s_1
     *           |
     *           O
     *           |
     *    w_2 O--X--O e_2
     *           |c_2
     *           O s_2
     */
     
    c_2 = A_cross(s_0,!u_0);
    
    AssertCrossing(c_2);
    
    const bool b_2 = (s_0 == C_arcs(c_2,!u_0,Right));
    
    PD_ASSERT( s_0 == C_arcs(c_2, !u_0, b_2 ) );
    
    e_2 = C_arcs(c_2, b_2, u_0);
    s_2 = C_arcs(c_2, u_0,!b_2); // opposite to n_2 == s_0.
    w_2 = C_arcs(c_2,!b_2,!u_0); // opposite to e_2
    
    // u_2 == u_0
    o_2 = (u_0 == ( C_state[c_2] == CrossingState::LeftHanded ));
    
    AssertArc(e_2);
    AssertArc(s_2);
    AssertArc(w_2);
}

void load_c_3()
{
    // TODO: Check this thoroughly!
    
    /*           O n_3
     *           |
     *    w_3 O--X--O e_3
     *           |c_3
     *           O
     *           |
     *       n_0 |       n_1
     *           O         O
     *           |    a    |
     *    w_0 O--X->O-->O--X->O e_1
     *           |c_0      |c_1
     *           O         O
     *          s_0       s_1
     */
     
    c_3 = A_cross(n_0,u_0);
    
    AssertCrossing(c_3);
    
    const bool b_3 = (n_0 == C_arcs(c_3,u_0,Right));
    
    PD_ASSERT( n_0 == C_arcs(c_3, u_0, b_3) );
    
    e_3 = C_arcs(c_3, b_3, u_0);
    n_3 = C_arcs(c_3,!u_0,!b_3); // opposite to s_3 == n_0.
    w_3 = C_arcs(c_3,!b_3,!u_0); // opposite to e_3

    // u_3 == u_0
    o_3 = (u_0 == ( C_state[c_3] == CrossingState::LeftHanded ));
    
    AssertArc(e_3);
    AssertArc(n_3);
    AssertArc(w_3);
}



    
bool a_is_loop()
{
    PD_DPRINT( "a_is_loop()" );
    
    if( c_0 == c_1 )
    {
        PD_DPRINT( "\tc_0 == c_1" );
        
        if( s_0 == a )
        {
            PD_DPRINT( "\t\ts_0 == a" );
           // This implies s_0 == a

            if( w_0 != n_0 )
            {
                PD_DPRINT( "\t\tw_0 != n_0" );
                
                /*              n_0
                 *               O
                 *               ^
                 *               |
                 *      w_0 O----X--->O
                 *               |    |
                 *               |c_0 |
                 *               O<---+  a
                 */
                
                Reconnect(w_0,Head,n_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                ++R_I_counter;
                
                AssertArc(w_0);
                
                return true;
            }
            else // if( w_0 == n_0 )
            {
                PD_DPRINT( "\t\tw_0 == n_0" );
                
                /*          +----O
                 *          |    ^
                 *          |    |
                 *      w_0 O----X--->O
                 *               |    |
                 *               |c_0 |
                 *               O<---+  a
                 *              s_0
                 */
                
                ++unlink_count;
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                
                R_I_counter += 2; // We make two R_I moves instead of one.
                
                return true;
            }
        }
        else
        {
            // This implies n_0 == a.
            
            PD_DPRINT( "\t\tn_0 == a" );
            PD_ASSERT( "n_0 == a" );
            
            if( w_0 != s_0 )
            {
                PD_DPRINT( "\t\t\tw_0 != s_0" );
                
                /*               O----+
                 *               |    |
                 *               |    | a
                 *      w_0 O----X--->O
                 *               |c_0
                 *               v
                 *               O
                 *              s_0
                 */
            
                Reconnect(w_0,Head,s_0);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                ++R_I_counter;
                
                AssertArc(w_0);
                
                return true;
            }
            else // if( w_0 == s_0 )
            {
                PD_DPRINT( "\t\t\tw_0 == s_0" );
                
                /*               O----+
                 *               |    |
                 *               |    | a
                 *      w_0 O----X--->O
                 *          |    |c_0
                 *          |    v
                 *          +----O
                 */
                
                ++unlink_count;
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                
                R_I_counter += 2;
                
                return true;
            }
        }
    }
    else
    {
        return false;
    }
}


bool twist_at_a()
{
    PD_DPRINT( "twist_at_a()" );
    
    AssertArc(a);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
    
    AssertArc(w_0);
    AssertArc(n_0);
    AssertArc(s_0);
    
    AssertArc(n_1);
    AssertArc(e_1);
    AssertArc(s_1);
    
    // Check for twist move. If successful, a, c_0, and c_1 are deleted.
    if( n_0 == s_1 )
    {
        PD_DPRINT( "\tn_0 == s_1" );
        
        if( w_0 == s_0 )
        {
            PD_DPRINT( "\t\tw_0 == s_0" );
            
           /*               +-------------------------+
            *               |                         |
            *               |            n_1          |
            *               O             O####       |
            *               |      a      |   #       |
            *       w_0 O---X-->O---->O---X-->O e_1   |
            *           |   |c_0          |c_1        |
            *           +---O             O           |
            *              s_0            |           |
            *                             +-----------+
            */
            
            if( n_1 != e_1 )
            {
                PD_DPRINT( "\t\t\tn_1 != e_1" );
                
                Reconnect(e_1,Tail,n_1);
                DeactivateArc(w_0);
                DeactivateArc(a );
                DeactivateArc(n_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                ++twist_counter;
                
                AssertArc(e_1);
                
                return true;
            }
            else
            {
                PD_DPRINT( "\t\t\tn_1 == e_1" );
                
                /*               +-------------------------+
                 *               |                         |
                 *               |            n_1          |
                 *               O             O---+       |
                 *               |      a      |   |       |
                 *       w_0 O---X-->O---->O---X-->O e_1   |
                 *           |   |c_0          |c_1        |
                 *           +---O             O           |
                 *              s_0            |           |
                 *                             +-----------+
                 */
                
                ++unlink_count;
                DeactivateArc(w_0);
                DeactivateArc(e_1);
                DeactivateArc(a );
                DeactivateArc(n_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                R_I_counter += 2;
                
                return true;
            }
        }
        
        if( n_1 == e_1 )
        {
            PD_DPRINT( "\t\tn_1 == e_1" );
            
            PD_ASSERT( w_0 != s_0 );
            
           /*               +-------------------------+
            *               |                         |
            *               |            n_1          |
            *               O             O---+       |
            *               |      a      |   |       |
            *       w_0 O---X-->O---->O---X-->O e_1   |
            *               |c_0          |c_1        |
            *               O             O           |
            *              s_0            |           |
            *                             +-----------+
            */
            
            Reconnect(w_0,Head,s_0);
            DeactivateArc(n_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            R_I_counter += 2;
            
            return true;
        }
        
        PD_DPRINT( "\t\t(w_0 != s_0) && (n_1 != e_1)." );
        
        // We have one of these two situations:
        
        /* Case A:
         *
         *
         *               +-------------------------+
         *               |                         |
         *               |            n_1          |
         *               O             O####       |
         *               |      a      |   #       |
         *       w_0 O---X-->O---->O---X-->O e_1   |
         *               |c_0          |c_1        |
         *               O             O           |
         *              s_0            |           |
         *                             +-----------+
         *
         * No matter what the crossing at c_1 is, we can reroute as follows:
         *
         *               +-------------------------+
         *               |         +----------+    |
         *               |         |  n_1     |    |
         *               O         |   O####  |    |
         *               |         |   |   #  |    |
         *       w_0 O---X---------+   |   O<-+    |
         *               |c_0          |           |
         *               O             O           |
         *              s_0            |           |
         *                             +-----------+
         *
         * Now, no matter what the crossing at c_0 is, we can reroute as follows:
         *
         *
         *                         +----------+
         *                         |  n_1     |
         *                         |   O####  |
         *                         |   |   #  |
         *       w_0 O-------------+   |   O<-+
         *                             |  e_1
         *               O-------------+
         *              s_0
         *
         * a, c_0, and c_1 are deleted.
         * w_0 and e_1 are fused.
         * s_0 and n_1 are fused.
         */
        
        
        /* Case B
         *
         *   +-----------+
         *   |           |
         *   |           |            n_1
         *   |           O             O
         *   |           |      a      |
         *   |   w_0 O---X-->O---->O---X-->O e_1
         *   |       #   |c_0          |c_1
         *   |       ####O             O
         *   |          s_0            |
         *   |                         |
         *   +-------------------------+
         *
         * No matter what the crossing at c_0 is, we can reroute as follows:
         *
         *   +-----------+
         *   |           |
         *   |           |            n_1
         *   |           O             O
         *   |      w_0  |             |
         *   |    +--O   |   +---------X-->O e_1
         *   |    |  #   |c_0|         |c_1
         *   |    |  ####O   |         O
         *   |    |     s_0  |         |
         *   |    +----------+         |
         *   +-------------------------+
         *
         * Now, no matter what the crossing at c_0 is, we can reroute as follows:
         *
         *
         *
         *                            n_1
         *               +-------------O
         *          w_0  |
         *        +--O   |   +------------>O e_1
         *        |  #   |   |
         *        |  ####O   |
         *        |     s_0  |
         *        +----------+
         *
         * a, c_0, and c_1 are deleted.
         * w_0 and e_1 are fused.
         * s_0 and n_1 are fused.
         */
        
        
        Reconnect(w_0,Head,e_1);
        Reconnect(s_0,u_0 ,n_1);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        DeactivateArc(a);
        ++twist_counter;
        
        AssertArc(w_0);
        AssertArc(s_0);
        
        return true;
    }
    
    // Check for twist move. If successful, a, c_0, and c_1 are deleted.
    if( s_0 == n_1 )
    {
        PD_DPRINT( "\ts_0 == n_1" );
        
        if( w_0 == n_0 )
        {
            PD_DPRINT( "\t\tw_0 == n_0" );
            
            if( e_1 != s_1 )
            {
                PD_DPRINT( "\t\t\te_1 != s_1" );
                
               /*                             +-----------+
                *                             |           |
                *              n_0            |           |
                *           +---O             O           |
                *           |   |      a      |           |
                *       w_0 O---X-->O---->O---X-->O e_1   |
                *               |c_0          |c_1#       |
                *               O             O####       |
                *               |            s_1          |
                *               |                         |
                *               +-------------------------+
                */
                
                Reconnect(e_1,Tail,s_1);
                
                DeactivateArc(a  );
                DeactivateArc(w_0);
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                R_I_counter += 2;
                
                AssertArc(e_1);
                
                return true;
            }
            else
            {
                PD_DPRINT( "\t\t\te_1 == s_1" );
                
                /*                             +-----------+
                 *                             |           |
                 *              n_0            |           |
                 *           +---O             O           |
                 *           |   |      a      |           |
                 *       w_0 O---X-->O---->O---X-->O e_1   |
                 *               |c_0          |c_1|       |
                 *               O             O---+       |
                 *               |            s_1          |
                 *               |                         |
                 *               +-------------------------+
                 */
                
                ++unlink_count;
                DeactivateArc(w_0);
                DeactivateArc(a  );
                DeactivateArc(e_1);
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                R_I_counter += 2;
                
                return true;
            }
        }
        
        if( e_1 == s_1 )
        {
            PD_ASSERT( w_0 != n_0 );
            
            PD_DPRINT( "\t\te_1 == s_1" );
            
            /*                             +-----------+
             *                             |           |
             *              n_0            |           |
             *               O             O           |
             *               |      a      |           |
             *       w_0 O---X-->O---->O---X-->O e_1   |
             *               |c_0          |c_1|       |
             *               O             O---+       |
             *               |            s_1          |
             *               |                         |
             *               +-------------------------+
             */
            
            Reconnect(w_0,Head,n_0);
            DeactivateArc(a  );
            DeactivateArc(e_1);
            DeactivateArc(s_0);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            
            R_I_counter += 2;
            
            AssertArc(w_0);
            
            return true;
        }
        
        PD_DPRINT( "\t\t(w_0 != n_0) && (s_1 != e_1)." );
        
        /* We have one of these two situations:
         *
         * Case A:
         *
         *                             +-----------+
         *                             |           |
         *              n_0            |           |
         *               O             O           |
         *               |      a      |           |
         *       w_0 O---X-->O---->O---X-->O e_1   |
         *               |c_0          |c_1#       |
         *               O             O####       |
         *               |            s_1          |
         *               |                         |
         *               +-------------------------+
         *
         * No matter what the crossing at c_1 is, we can reroute as follows:
         *
         *                             +-----------+
         *                             |           |
         *              n_0            |           |
         *               O             O           |
         *               |             |  e_1      |
         *       w_0 O---X---------+   |   O<-+    |
         *               |c_0      |   |   #  |    |
         *               O         |   O####  |    |
         *               |         |  s_1     |    |
         *               |         +----------+    |
         *               +-------------------------+
         *
         * Now, no matter what the crossing at c_0 is, we can reroute as follows:
         *
         *
         *
         *              n_0
         *               O-------------+
         *                             |  e_1
         *       w_0 O-------------+   |   O--+
         *                c_0      |   |   #  |
         *                         |   O####  |
         *                         |  s_1     |
         *                         +----------+
         *
         *
         * a, c_0, and c_1 are deleted.
         * w_0 and e_1 are fused.
         * n_0 and s_1 are fused.
         */
         
        // Case B is fully analogous. I skip it.
                
        Reconnect(w_0,Head,e_1);
        Reconnect(n_0,!u_0,s_1);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++twist_counter;
        
        AssertArc(w_0);
        AssertArc(n_0);
        
        return true;
    }
    
    return false;
}

bool RI_at_c_0()
{
    PD_DPRINT( "RI_at_c_0()" );
    
    AssertArc(a);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
    
    if( n_0 == w_0 )
    {
        PD_DPRINT( "\tn_0 == w_0" );
        
        if( s_0 != s_1 )
        {
            PD_DPRINT( "\t\ts_0 != s_1" );
            
            /* Looks like this:
             *
             *                            n_1
             *           +---+           |
             *           |   |     a     v
             *       w_0 +-->X---------->X--> e_1
             *               |c_0        |c_1
             *               |           v
             *                s_0         s_1
             */
            
            Reconnect(a,Tail,s_0);
            DeactivateArc(w_0);
            DeactivateCrossing(c_0);
            ++R_I_counter;
            
            AssertArc(a  );
            AssertArc(n_1);
            AssertArc(e_1);
            AssertArc(s_1);
            AssertCrossing(c_1);
            
            return true;
        }
        else // if( s_0 == s_1 )
        {
            PD_DPRINT( "t\ts_0 == s_1" );
             
            if( e_1 != n_1 )
            {
                /* A second Reidemeister I move can be performed.
                 *
                 *                            n_1
                 *           +---+           |
                 *           |   |     a     v
                 *       w_0 +-->X---------->X--> e_1
                 *               |c_0        |c_1
                 *               |           v
                 *               +-----<-----+
                 8                    s_0
                 */
                
                Reconnect(e_1,Tail,n_1);
                DeactivateArc(w_0);
                DeactivateArc(s_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                R_I_counter += 2;
                
                AssertArc(e_1);
                
                return true;
            }
            else
            {
                /* A second Reidemeister I move can be performed.
                 *
                 *
                 *           +---+           +---+
                 *           |   |     a     v   |
                 *       w_0 +-->X---------->X-->+ e_1
                 *               |c_0        |c_1
                 *               |           v
                 *               +-----<-----+
                 *                     s_0
                 */
                
                DeactivateArc(w_0);
                DeactivateArc(e_1);
                DeactivateArc(s_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                R_I_counter += 2;
                
                ++unlink_count;
                
                return true;
            }
        }
    }
    
    // Reidemeister_I at crossing c_0
    if( s_0 == w_0 )
    {
        PD_DPRINT( "\ts_0 == w_0" );
        
        if( n_0 != n_1 )
        {
            PD_DPRINT( "\t\tn_0 != n_1" );
            
            /* A second Reidemeister I move can be performed.
             *
             *                n_0         n_1
             *               |           |
             *               |     a     |
             *       w_0 +-->X---------->X--> e_1
             *           |   |c_0        ^c_1
             *           +<--+           |
             *                            s_1
             */
            
            Reconnect(a,Tail,n_0);
            DeactivateArc(w_0);
            DeactivateCrossing(c_0);
            ++R_I_counter;
            
            AssertArc(a  );
            AssertArc(n_1);
            AssertArc(e_1);
            AssertArc(s_1);
            AssertCrossing(c_1);

            return true;
        }
        else // if( n_0 == n_1 )
        {
            PD_DPRINT( "\t\tn_0 == n_1" );

            if( e_1 != s_1 )
            {
                PD_DPRINT( "\t\t\te_1 != s_1" );
                
                /* A second Reidemeister I move can be performed.
                 *
                 *                    n_0
                 *               +-----<-----+
                 *               |           |
                 *               |     a     |
                 *       w_0 +-->X---------->X--> e_1
                 *           |   |c_0        ^c_1
                 *           +<--+           |
                 *                            s_1
                 */
                
                Reconnect(e_1,Tail,s_1);
                DeactivateArc(w_0);
                DeactivateArc(n_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                R_I_counter += 2;
                
                AssertArc(e_1);
                
                return true;
            }
            else
            {
                PD_DPRINT( "\t\t\te_1 == s_1" );
                
                
                /* A second Reidemeister I move can be performed.
                 *
                 *                    n_0
                 *               +-----<-----+
                 *               |           |
                 *               |     a     |
                 *       w_0 +-->X---------->X-->+e_1
                 *           |   |c_0        ^c_1|
                 *           +<--+           +---+
                 *
                 */
                
                ++unlink_count;
                
                DeactivateArc(e_1);
                DeactivateArc(w_0);
                DeactivateArc(n_0);
                DeactivateArc(a  );
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                R_I_counter += 2;
                
                return true;
            }
        }
    }
    
    return false;
}

bool RI_at_c_1()
{
    PD_DPRINT( "RI_at_c_1()" );
    
    AssertArc(a);
    AssertCrossing(c_0);
    AssertCrossing(c_1);
    
    // Reidemeister_I at crossing c_1
    if( s_1 == e_1 )
    {
        PD_DPRINT( "\ts_1 == e_1" );
        
        if( n_0 != n_1 )
        {
            PD_DPRINT( "\t\tn_0 != n_1" );
            
            /*                n_0         n_1
             *               |           ^
             *               |     a     |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |   |
             *               v           +---+
             *                s_0
             */
            
            Reconnect(a,Head,n_1);
            DeactivateArc(e_1);
            DeactivateCrossing(c_1);
            ++R_I_counter;
            
            AssertCrossing(c_0);
            AssertArc(a);
            AssertArc(w_0);
            AssertArc(n_0);
            AssertArc(s_0);
            
            return true;
        }
        else // if( n_0 == n_1 )
        {
            PD_DPRINT( "\t\tn_0 == n_1" );
            
            if( w_0 != s_0 )
            {
                PD_DPRINT( "\t\t\tw_0 != s_0" );
                
                /* A second Reidemeister I move can be performed.
                 *
                 *               +-----<-----+
                 *               |           ^
                 *               |     a     |
                 *        w_0 -->X---------->X-->+ e_1
                 *               |c_0        |   |
                 *               v           +---+
                 *                s_0
                 */
                
                Reconnect(w_0,Head,s_0);
                DeactivateArc(n_1);
                DeactivateArc(e_1);
                DeactivateArc(a);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                R_I_counter += 2;
                
                AssertArc(w_0);
                
                return true;
            }
            else
            {
                PD_DPRINT( "\t\t\tw_0 == s_0" );
                
                /* We detected an unlink
                 *
                 *               +-----<-----+
                 *               |           ^
                 *               |     a     |
                 *       w_0 +-->X---------->X-->+ e_1
                 *           |   |c_0        |   |
                 *           +---v           +---+
                 */
                
                DeactivateArc(w_0);
                DeactivateArc(e_1);
                DeactivateArc(n_1);
                DeactivateArc(a);
                
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);

                R_I_counter += 2;
                
                ++unlink_count;
                
                return true;
            }
        }
    }

    // Reidemeister_I at crossing c_1
    if( n_1 == e_1 )
    {
        PD_DPRINT( "\tn_1 == e_1" );
        
        if( s_0 != s_1 )
        {
            PD_DPRINT( "\t\ts_0 != s_1" );
            
            /* A second Reidemeister I move can be performed.
             *
             *                 n_0
             *               ^           +---+
             *               |     a     |   |
             *        w_0 -->X---------->X-->+ e_1
             *               |c_0        |c_1
             *               |           v
             *                 s_0
             */
             
            Reconnect(a,Head,s_1);
            DeactivateArc(e_1);
            DeactivateCrossing(c_1);
            ++R_I_counter;
            
            AssertCrossing(c_0);
            AssertArc(a  );
            AssertArc(n_0);
            AssertArc(s_0);
            AssertArc(w_0);
            
            return true;
        }
        else // if( s_0 == s_1 )
        {
            PD_DPRINT( "\t\ts_0 == s_1" );
             
            if( w_0 != n_0 )
            {
                PD_DPRINT( "\t\t\tw_0 != n_0" );
                
                /*                n_0
                 *               ^           +---+
                 *               |     a     |   |
                 *        w_0 -->X---------->X-->+ e_1
                 *               |c_0        |c_1
                 *               |           v
                 *               +-----<-----+
                 *                    s_0
                 */
                
                Reconnect(w_0,Head,n_0);
                
                DeactivateArc(a  );
                DeactivateArc(e_1);
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                R_I_counter += 2;
                
                AssertArc(w_0);
                
                return true;
            }
            else
            {
                PD_DPRINT( "\t\t\tw_0 == n_0" );
                
                /*
                 *           +---+           +---+
                 *           |   |     a     |   |
                 *       w_0 +-->X---------->X-->+ e_1
                 *               |c_0        |c_1
                 *               |           v
                 *               +-----<-----+
                 *                    s_0
                 */
                
                DeactivateArc(w_0);
                DeactivateArc(e_1);
                DeactivateArc(a  );
                DeactivateArc(s_0);
                DeactivateCrossing(c_0);
                DeactivateCrossing(c_1);
                
                ++unlink_count;
                
                R_I_counter += 2;
                
                return true;
            }
        }
    }
    
    return false;
}


bool a_is_2loop()
{
    PD_DPRINT( "a_is_2loop()" );
    
    if( w_0 == e_1 )
    {
        PD_DPRINT( "\tw_0 == e_1" );
        
        if( o_0 == o_1 )
        {
            PD_DPRINT( "\t\to_0 == o_1" );
            
            /* We have a true over- or underloop
             * Looks like this:
             *
             *       +-------------------+              +-------------------+
             *       |                   |              |                   |
             *       |   O###########O   |              |   O###########O   |
             *       |   |     a     |   |              |   |     a     |   |
             *       +-->|---------->|-->+      or      +-->----------->--->+
             *           |c_0        |c_1                   |c_0        |c_1
             *           |           |                      |           |
             *
             *                 or                                 or
             *
             *           |           |                      |           |
             *           |     a     |                      |     a     |
             *       +-->|---------->|-->+      or      +-->----------->--->+
             *       |   |c_0        |c_1|              |   |c_0        |c_1|
             *       |   O###########O   |              |   O###########O   |
             *       |                   |              |                   |
             *       +-------------------+              +-------------------+
             */
            
            wprint("Feature of detecting over- or underloops is not tested, yet.");
            
            Reconnect(s_0,u_0,n_0);
            Reconnect(s_1,u_1,n_1);
            DeactivateArc(w_0);
            DeactivateArc(e_1);
            DeactivateArc(a);
            DeactivateCrossing(c_0);
            DeactivateCrossing(c_1);
            // TODO: Invent some counter here and increment it.
            ++unlink_count;
            
            AssertArc(s_0);
            AssertArc(s_1);
            
            return true;
        }
        else // if( o_0 != o_1 )
        {
            PD_DPRINT( "\t\to_0 != o_1" );
            
            /* Looks like this:
             *
             *       +-------------------+              +-------------------+
             *       |                   |              |                   |
             *       |   O###########O   |              |   O###########O   |
             *       |   |     a     |   |              |   |     a     |   |
             *       +-->|---------->--->+      or      +-->----------->|-->+
             *           |c_0        |c_1                   |c_0        |c_1
             *           |           |                      |           |
             *
             *                 or                                 or
             *
             *           |           |                      |           |
             *           |     a     |                      |     a     |
             *       +-->----------->|-->+      or      +-->|---------->--->+
             *       |   |c_0        |c_1|              |   |c_0        |c_1|
             *       |   O###########O   |              |   O###########O   |
             *       |                   |              |                   |
             *       +-------------------+              +-------------------+
             */
            
            // TODO: If additionally the endpoints of n_0 and n_1 or the endpoints of s_0 and s_1 coincide, there are subtle ways to remove the crossing there:

            /*       +-------------------+              +-------------------+
             *       |                   |              |   +--------------+|
             *       |                   |              |   |              ||
             *       |   O##TANGLE###O   |              |   O##TANGLE###O  ||
             *       |   |           |   |              |              /   ||
             *       |   |           |   |              |   +---------+    ||
             *       |   |     a     |   |              |   |     a     +--+|
             *       +-->|----------->---+     ==>      +-->------------|---+
             *           |c_0        |c_1                   |c_0        |c_1
             *           O           O                      O           O
             *            \         /                        \         /
             *             \       /                          \       /
             *              \     /                            \     /
             *               O   O                              +   +
             *                \ /                               |   |
             *                 \ c_2                            |   |2
             *                / \                               |   |
             *               O   O                              O   O
             *
             *       +-------------------+              +-------------------+
             *       |                   |              |                   |
             *       |   O##TANGLE###O   |              |   O##TANGLE###O   |
             *       |   |           |   |              |   |           |   |
             *       |   |     a     |   |   untwist    |   |     a     |   |
             *       +-->------------|---+     ==>      +-->------------|---+
             *           |c_0        |c_1                   |c_0        |c_1
             *           O           O                      O           O
             *            \         /                        \         /
             *             \       /                          \       /
             *              \     /                            \     /
             *               O   O                              +   +
             *                \ /                               |   |
             *                 \ c_2                            |   |2
             *                / \                               |   |
             *               O   O                              O   O
             */
            
            // But this comes at the price of an additional crossing load, and it might not be very likely. Also, I am working only on connected diagrams, so I have no test code for this.
            
            return false;
        }
    }
    
    return false;
}


template<bool mult_compQ = true>
bool strands_same_side()
{
    PD_DPRINT( "strands_same_side()" );
    
    /*               |     a     |             |     a     |
     *            -->|---------->|-->   or  -->----------->--->
     *               |c_0        |c_1          |c_0        |c_1
     */
    
    // Check for Reidemeister II move.
    if( n_0 == n_1 )
    {
        PD_DPRINT( "\tn_0 == n_1" );
        
        /*               +-----------+             +-----------+
         *               |           |             |           |
         *               |     a     |             |     a     |
         *            -->|---------->|-->   or  -->----------->--->
         *               |c_0        |c_1          |c_0        |c_1
         */
        
        // TODO: If the endpoints of s_0 and s_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
        // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
        // TODO: On the other hand: we need to reference to the two end point anyways to do the reconnecting right...
        
        
        // These case w_0 == e_1, w_0 == s_0, e_1 == s_1 are ruled out already...
        PD_ASSERT( w_0 != e_1 );
        PD_ASSERT( w_0 != s_0 );
        PD_ASSERT( e_1 != s_1 );
        
        // .. so this is safe:
        Reconnect(w_0,Head,e_1);
        
        if constexpr( mult_compQ )
        {
            if( s_0 != s_1 )
            {
                PD_DPRINT( "\t\ts_0 != s_1" );
                
                Reconnect(s_0,u_0,s_1);
            }
            else
            {
                PD_DPRINT( "\t\ts_0 == s_1" );
                
                ++unlink_count;
                DeactivateArc(s_0);
            }
        }
        else
        {
            Reconnect(s_0,u_0,s_1);
        }
        
        DeactivateArc(n_0);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++R_II_counter;
        
        AssertArc(s_0);
        AssertArc(w_0);

        
        return true;
    }
    
    // Check for Reidemeister II move.
    if( s_0 == s_1 )
    {
        PD_DPRINT( "\ts_0 == s_1" );
        
        logprint("Incoming data");
        logvalprint("c_0",CrossingString(c_0));
        logvalprint("c_1",CrossingString(c_1));
        
        logvalprint("a  ",ArcString(a  ));
        
        logvalprint("n_0",ArcString(n_0));
        logvalprint("w_0",ArcString(w_0));
        logvalprint("s_0",ArcString(s_0));
        
        logvalprint("n_1",ArcString(n_1));
        logvalprint("e_1",ArcString(e_1));
        logvalprint("s_1",ArcString(s_1));
        
        // TODO: If the endpoints of n_0 and n_1 coincide, we can remove even 3 crossings by doing the unlocked Reidemeister I move.
        // TODO: Price: Load 1 additional crossing for test. Since a later R_I would remove the crossing anyways, this does not seem to be overly attractive.
        // TODO: On the other hand: we need to reference to the two end point anyways to do the reconnecting right...
        
        /*                n_0         n_1                    n_0         n_1
         *               |     a     |                      |     a     |
         *        w_0 -->|---------->|--> e_1   or   w_0 -->----------->---> e_1
         *               |c_0        |c_1                   |c_0        |c_1
         *               |           |                      |           |
         *               +-----------+                      +-----------+
         *                    s_0                                s_0
         */
        
        // These case w_0 == e_1, w_0 == n_0, e_1 == n_1 are ruled out already...
        PD_ASSERT( w_0 != e_1 );
        PD_ASSERT( w_0 != n_0 );
        PD_ASSERT( e_1 != n_1 );
        
        // .. so this is safe:
        Reconnect(w_0,Head,e_1);
        
        // We have already checked this above.
        PD_ASSERT( n_0 != n_1 );
        Reconnect(n_0,!u_0,n_1);
        DeactivateArc(s_0);
        DeactivateArc(a);
        DeactivateCrossing(c_0);
        DeactivateCrossing(c_1);
        ++R_II_counter;
        
        AssertArc(n_0);
        AssertArc(w_0);
        
        return true;
    }
    
    
    // Try Reidemeister IIa move.
    
    // TODO: If n_0 == n_1 and if the endpoints of s_0 and s_1 coincide, we can remove 3 crossings.
    // TODO: Price: Load 1 additional crossing for test; load 2 more crossings if successful
    
    // TODO: If s_0 == s_1 and if the endpoints of n_0 and n_1 coincide, we can remove 3 crossings.
    // TODO: Price: Load 1 additional crossing for test; load 2 more crossings if successful
    
    PD_ASSERT(s_0 != s_1);
    PD_ASSERT(n_0 != n_1);
    PD_ASSERT(w_0 != e_1);
    
    load_c_2();
    load_c_3();
    
    /*       w_3     n_3             w_3     n_3
     *          O   O                   O   O
     *           \ /                     \ /
     *            X c_3                   X c_3
     *           / \                     / \
     *          O   O                   O   O e_3
     *     n_0 /     e_3           n_0 /
     *        /                       /
     *       O         O n_1         O         O n_1
     *       |    a    |             |    a    |
     *    O--|->O---O--|->O   or  O---->O---O---->O
     *       |c_0   c_1|             |c_0   c_1|
     *       O         O s_1         O         O s_1
     *        \                       \
     *     s_0 \                   s_0 \
     *          O   O e_2               O   O e_2
     *           \ /                     \ /
     *            X c_2                   X c_2
     *           / \                     / \
     *          O   O                   O   O
     *       w_2     s_2             w_2     s_2
     */
    
    //Check for Reidemeister IIa move.
    if( (e_2 == s_1) && (e_3 == n_1) && (o_2 == o_3) )
    {
        PD_DPRINT( "\t(e_2 == s_1) && (e_3 == n_1) && (o_2 == o_3)" );
        
        /*       w_3     n_3             w_3     n_3
         *          O   O                   O   O
         *           \ /                     \ /
         *            / c_3                   / c_3
         *           / \                     / \
         *          O   O                   O   O
         *     n_0 /     \ n_1         n_0 /     \ n_1
         *        /       \               /       \
         *       O         O             O         O
         *       |    a    |             |    a    |
         *    O--|->O---O--|->O   or  O---->O---O---->O
         *       |c_0   c_1|             |c_0   c_1|
         *       O         O             O         O
         *        \       /               \       /
         *     s_0 \     / s_1         s_0 \     / s_1
         *          O   O                   O   O
         *           \ /                     \ /
         *            \ c_2                   \ c_2
         *           / \                     / \
         *          O   O                   O   O
         *       w_2     s_2             w_2     s_2
         *
         *            or                      or
         *
         *       w_3     n_3             w_3     n_3
         *          O   O                   O   O
         *           \ /                     \ /
         *            \ c_3                   \ c_3
         *           / \                     / \
         *          O   O                   O   O
         *     n_0 /     \ n_1         n_0 /     \ n_1
         *        /       \               /       \
         *       O         O             O         O
         *       |    a    |             |    a    |
         *    O--|->O---O--|->O   or  O---->O---O---->O
         *       |c_0   c_1|             |c_0   c_1|
         *       O         O             O         O
         *        \       /               \       /
         *     s_0 \     / s_1         s_0 \     / s_1
         *          O   O                   O   O
         *           \ /                     \ /
         *            / c_2                   / c_2
         *           / \                     / \
         *          O   O                   O   O
         *       w_2     s_2             w_2     s_2
         */
        
        // Reidemeister IIa move
        
        Reconnect(n_0, u_0,w_3);
        Reconnect(s_0,!u_0,w_2);
        Reconnect(n_1, u_1,n_3);
        Reconnect(s_1,!u_1,s_2);
        DeactivateCrossing(c_2);
        DeactivateCrossing(c_3);
        ++R_IIa_counter;
        
        AssertArc(a);
        AssertArc(n_0);
        AssertArc(n_1);
        AssertArc(s_0);
        AssertArc(s_1);
        AssertArc(w_0);
        AssertArc(e_1);
        AssertCrossing(c_0);
        AssertCrossing(c_1);
        
        return true;
    }
    
    return false;
}

template<bool mult_compQ = true>
bool strands_opposite_sides()
{
    PD_DPRINT("strands_opposite_sides()");
    
    /*       |     a     |             |     a     |
     *    -->|---------->--->   or  -->----------->|-->
     *       |c_0        |c_1          |c_0        |c_1
     */
    
    //Check for Reidemeister Ia move.
    if( n_0 == n_1 )
    {
        PD_DPRINT( "\tn_0 == n_1" );
        
        /*       +-----------+             +-----------+
         *       |     a     |             |     a     |
         *    -->----------->|-->   or  -->|---------->--->
         *       |c_0        |c_1          |c_0        |c_1
         *       |           |             |           |
         */
        
        load_c_2();
                            
        if( e_2 == s_1 )
        {
            PD_DPRINT( "\t\te_2 == s_1" );
            
            if( o_0 == o_2 )
            {
                PD_DPRINT( "\t\to_0 == o_2" );
                
                /*  R_Ia move.
                 *
                 *           +-----------+             +-----------+
                 *           |           |             |           |
                 *           |     a     |             |     a     |
                 *        -->----------->|-->   or  -->|---------->--->
                 *           |c_0        |c_1          |c_0        |c_1
                 *           O---O   O---O             O---O   O---O
                 *                \ /                       \ /
                 *                 / c_2                     \  c_2
                 *                / \                       / \
                 *               O   O s_2                 O   O s_2
                 */
                
                // TODO: Do the R_Ia surgery.
                // I skip this for now, because it does not have _that_ much impact.
                
//                DeactivateCrossing(c_2);
//                ++R_Ia_counter;
//                return true;
            }
            else
            {
                PD_DPRINT( "\t\to_0 != o_2" );
                
                /*
                 *           +-----------+             +-----------+
                 *           |           |             |           |
                 *           |     a     |             |     a     |
                 *        -->----------->|-->       -->|---------->--->
                 *           |c_0        |c_1          |c_0        |c_1
                 *           O---O   O---O             O---O   O---O
                 *                \ /                       \ /
                 *                 \ c_2                     /  c_2
                 *                / \                       / \
                 */
                
                if( (w_0 == w_2) || (e_1 == s_2) )
                {
                    PD_DPRINT( "\t\t\t(w_0 == w_2) || (e_1 == s_2)" );
                    
                    // TODO: trefoil as connect summand detected
                    // How to store this info?
                }
            }
            
            return false;
        }
        
        // This can only happen, if the planar diagram has multiple components!
        if constexpr( mult_compQ )
        {
            if( w_2 == s_1 )
            {
                PD_DPRINT( "\t\tw_2 == s_1" );
                
                /* Two further interesting cases.
                 *
                 *           +-----------+             +-----------+
                 *           |           |             |           |
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *        -->----------->|-->       -->|---------->--->
                 *           O   O###O   O             O   O###O   O
                 *           |    \ /    |             |    \ /    |
                 *       s_0 |     X c_2 | s_1         |     X c_2 | s_1
                 *           |    / \    |             |    / \    |
                 *           +---O   O---+             +---O   O---+
                 *
                 *      These can be rerouted to the following two situations:
                 *           +-----------+             +-----------+
                 *           |           |             |           |
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *        -->----------->|-->       -->|---------->--->
                 *           O   +-------O             O   +-------O
                 *           |   |     s_1             |   |     s_1
                 *       s_0 |   O###O             s_0 |   O###O
                 *           |       |                 |       |
                 *           +-------+                 +-------+
                 *
                 *      No matter how c_2 is handed, we fuse s_0 and s_1 with their opposite arcs accross c_2 and deactivate c_2.
                 *
                 *      This will probably happen seldomly.
                 */
                
                Reconnect(s_0,!u_0,s_2);
                Reconnect(s_1,!u_1,e_2);
                DeactivateCrossing(c_2);
                ++twist_counter;
                
                AssertArc(a);
                AssertArc(n_0);
                AssertArc(n_1);
                AssertArc(s_0);
                AssertArc(s_1);
                AssertArc(w_0);
                AssertArc(e_1);
                AssertCrossing(c_0);
                AssertCrossing(c_1);
                
                return true;
            }
        }

        return false;
    }
    
    //Check for Reidemeister Ia move.
    if( s_0 == s_1 )
    {
        PD_DPRINT( "\ts_0 == s_1" );
        
        /*       |     a     |
         *    -->----------->|-->
         *       |c_0        |c_1
         *       |           |
         *       +-----------+
         */
        
        load_c_3();
        
        if( e_3 == n_1 )
        {
            PD_DPRINT( "\t\te_3 == n_1" );
            
            if( o_0 == o_3 )
            {
                PD_DPRINT( "\t\t\to_0 == o_3" );
                
                /*  R_Ia move.
                 *
                 *           w_3 O   O n_3             w_3 O   O n_3
                 *                \ /                       \ /
                 *                 \ c_3                     / c_3
                 *                / \                       / \
                 *           O---O   O---O             O---O   O---O
                 *           |           |             |           |
                 *           |     a     |             |     a     |
                 *        -->----------->|-->       -->|---------->--->
                 *           |c_0        |c_1          |c_0        |c_1
                 *           +-----------+             +-----------+
                 */
                
                // TODO: Do the R_Ia surgery.
                // I skip this for now, because it does not have _that_ much impact.
                
//                DeactivateCrossing(c_3);
//                ++R_Ia_counter;
//
//                return true;
            }
            else
            {
                PD_DPRINT( "\t\t\to_0 != o_3" );
                
                /* Potential trefoil cases:
                 *
                 *       w_3 O   O n_3                w_3 O   O n_3
                 *            \ /                          \ /
                 *             /                            \
                 *            / \                          / \
                 *       O---O   O---O                O---O   O---O
                 *       |           |                |           |
                 *   w_0 |     a     |  e_1      w_0  |     a     |  e_1
                 *   O-->----------->|-->O   or   O-->|---------->--->O
                 *       |c_0        |c_1             |c_0        |c_1
                 *       +-----------+                +-----------+
                 */
                 
                if( (w_0 == w_3) || (n_3 == e_1) )
                {
                    // TODO: trefoil as connect summand detected
                    // How to store this info?
                }
                
            }
            
        } // if( e_3 == n_1 )
        
        // This can only happen, if the planar diagram has multiple components!
        if constexpr( mult_compQ )
        {
            if( w_3 == n_1 )
            {
                PD_DPRINT( "\t\tw_3 == n_1" );
                
                /* Two further interesting cases.
                 *
                 *           +---O   O---+             +---O   O---+
                 *           |    \ /    |             |    \ /    |
                 *       n_0 |     X c_3 | n_1     n_0 |     X c_3 | n_1
                 *           |    / \    |             |    / \    |
                 *           |   O###O   |             |   O###O   |
                 *           |           |             |           |
                 *           O           O             O           O
                 *        -->----------->|-->       -->|---------->--->
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *           |           |             |           |
                 *           +-----------+             +-----------+
                 *
                 *      These can be rerouted to the following two situations:
                 *           +-------+                 +-------+
                 *           |       |                 |       |
                 *           |       |                 |       |
                 *           |       |                 |       |
                 *       n_0 |   O###O             n_0 |   O###O
                 *           |   |     n_1             |   |     n_1
                 *           O   +-------O             O   +-------O
                 *        -->----------->|-->       -->|---------->--->
                 *        c_0|     a     |c_1       c_0|     a     |c_1
                 *           |           |             |           |
                 *           +-----------+             +-----------+
                 *
                 *      No matter how c_3 is handed, we fuse n_0 and n_1 with their opposite arcs accross c_3 and deactivate c_3.
                 *
                 *      This will probably happen seldomly.
                 */
                
                Reconnect(n_0, u_0,e_3);
                Reconnect(n_1, u_1,w_3);
                DeactivateCrossing(c_2);
                ++twist_counter;
                
                AssertArc(a);
                AssertArc(n_0);
                AssertArc(n_1);
                AssertArc(s_0);
                AssertArc(s_1);
                AssertArc(w_0);
                AssertArc(e_1);
                AssertCrossing(c_0);
                AssertCrossing(c_1);
                
                return true;
            }
            
        } // if constexpr( mult_compQ )

        return false;
    }

    load_c_2();
    load_c_3();
    
    //Check for Reidemeister IIa move.
    if( (e_3 == n_1) && (e_2 == s_1) && (o_2 == o_3) && ( (o_2 == o_0) || (o_2 != o_1) ) )
    {
        PD_DPRINT( "\t(e_3 == n_1) && (e_2 == s_1) && (o_2 == o_3) && ( (o_2 == o_0) || (o_2 != o_1) )" );
        
        /*          w_3 O   O n_3             w_3 O   O n_3
         *               \ /                       \ /
         *                / c_3                     \ c_3
         *               / \                       / \
         *              O   O                     O   O
         *             /     \                   /     \
         *        n_0 /       \ n_1         n_0 /       \ n_1
         *           O         O               O         O
         *           |    a    |               |    a    |
         *        O--|->O-->O---->O         O---->O-->O--|->O
         *           |c_0      |c_1            |c_0      |c_1
         *           O         O               O         O
         *        s_0 \       / s_1         s_0 \       / s_1
         *             \     /                   \     /
         *              O   O                     O   O
         *               \ /                       \ /
         *                \ c_2                     / c_2
         *               / \                       / \
         *          w_2 O   O s_2             w_2 O   O s_2
         */
         
        // Reidemeister IIa move
        
        Reconnect(s_0,!u_0,w_2);
        Reconnect(n_0, u_0,w_3);
        Reconnect(s_1,!u_1,s_2);
        Reconnect(n_1, u_1,n_3);
        DeactivateCrossing(c_2);
        DeactivateCrossing(c_3);
        ++R_IIa_counter;
        
        AssertArc(a);
        AssertArc(s_0);
        AssertArc(n_0);
        AssertArc(s_1);
        AssertArc(n_1);
        AssertCrossing(c_0);
        AssertCrossing(c_1);

        return true;
    }

    return false;
}
