public:

/*!
 * @brief Returns the pd-codes of the crossing as Tensor2 object.
 *
 *  The pd-code of crossing `c` is given by
 *
 *    `{ PDCode()(c,0), PDCode()(c,1), PDCode()(c,2), PDCode()(c,3), PDCode()(c,4) }`
 *
 *  The first 4 entries are the arcs attached to crossing c.
 *  `PDCode()(c,0)` is the incoming arc that goes under.
 *  This should be compatible with Dror Bar-Natan's _KnotTheory_ package.
 *
 *  The last entry stores the handedness of the crossing:
 *    +1 for a right-handed crossing,
 *    -1 for a left-handed crossing.
 */

Tensor2<Int,Int> PDCode() const
{
    ptic( ClassName()+"::PDCode" );
    
    const Int m = A_cross.Dimension(0);
    
    Tensor2<Int ,Int> pdcode     ( crossing_count, 5 );
//            Tensor1<Int ,Int> C_labels   ( A_cross.Max()+1, -1 );
    Tensor1<Int ,Int> C_labels   ( m/2, -1 );
    Tensor1<char,Int> A_visisted ( m, false           );
    
    Int a_counter = 0;
    Int c_counter = 0;
    Int a_ptr     = 0;
    Int a         = 0;
    
//            logdump( C_arcs  );
//            logdump( C_state );
//            logdump( A_cross );
//            logdump( A_state );
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( A_visisted[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        a = a_ptr;
        
        
        C_labels[A_cross(a,Tail)] = c_counter++;
        
        
        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            const Int c_prev = A_cross(a,Tail);
            const Int c_next = A_cross(a,Head);
            
            A_visisted[a] = true;
            
            if( C_labels[c_next] < 0 )
            {
                C_labels[c_next] = c_counter++;
            }
            
            // Tell c_prev that arc a_counter goes out of it.
            {
                const CrossingState state = C_state[c_prev];
                const Int           c     = C_labels[c_prev];

//                const bool side = (C_arcs(c_prev,Out,Right) == a) ? Right : Left;
                
                const bool side = (C_arcs(c_prev,Out,Right) == a);

                if( RightHandedQ(state) )
                {
                    pdcode( c, 4 ) = 1;
                    
                    if( side == Left )
                    {
                        /*
                         *
                         * a_counter
                         *     =
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c = C_labels[c_prev]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *
                         */
                        
                        pdcode( c, 2 ) = a_counter;
                    }
                    else // if( side == Right )
                    {
                        /*
                         *
                         *                a_counter
                         *                    =
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c = C_labels[c_prev]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *
                         */
                        
                        pdcode( c, 1 ) = a_counter;
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pdcode( c, 4 ) = -1;
                    
                    if( side == Left )
                    {
                        /*
                         *
                         * a_counter
                         *     =
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c = C_labels[c_prev]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *
                         */
                        
                        pdcode( c, 3 ) = a_counter;
                    }
                    else // if( side == Right )
                    {
                        /*
                         *
                         *                a_counter
                         *                    =
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c = C_labels[c_prev]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *
                         */
                        
                        pdcode( c, 2 ) = a_counter;
                    }
                }
                
            }
            
            // Tell c_next that arc a_counter goes into it.
            {
                const CrossingState state = C_state[c_next];
                const Int           c     = C_labels[c_next];
                
//                const bool side  = (C_arcs(c_next,In,Right)) == a ? Right : Left;
                const bool side  = (C_arcs(c_next,In,Right)) == a;
                
                if( RightHandedQ(state) )
                {
                    pdcode( c, 4 ) = 1;
                    
                    if( side == Left )
                    {
                        /*
                         *
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c = C_labels[c_next]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *     =
                         * a_counter
                         *
                         */
                        
                        pdcode( c, 3 ) = a_counter;
                    }
                    else // if( side == Right )
                    {
                        /*
                         *
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c = C_labels[c_next]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *                    =
                         *                a_counter
                         *
                         */
                        
                        pdcode( c, 0 ) = a_counter;
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pdcode( c, 4 ) = -1;
                    
                    if( side == Left )
                    {
                        /*
                         *
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c = C_labels[c_next]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *     =
                         * a_counter
                         *
                         */
                        
                        pdcode( c, 0 ) = a_counter;
                    }
                    else // if( lr == Right )
                    {
                        /*
                         *
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c = C_labels[c_next]
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *                    =
                         *                a_counter
                         *
                         */
                        
                        pdcode( c, 1 ) = a_counter;
                    }
                }
            }
            
            a = NextArc(a);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    ptoc( ClassName()+"::PDCode");
    
    return pdcode;
}
