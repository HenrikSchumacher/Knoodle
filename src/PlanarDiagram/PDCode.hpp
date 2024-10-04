public:

/*! @brief Construction from PD codes and handedness of crossings.
 *
 *  @param pd_codes_ Integer array of length `5 * crossing_count_`.
 *  There is one 5-tuple for each crossing.
 *  The first 4 entries in each 5-tuple store the actual PD code.
 *  The last entry gives the handedness of the crossing:
 *    >  0 for a right-handed crossing
 *    <= 0 for a left-handed crossing
 *
 *  @param crossing_count_ Number of crossings in the diagram.
 *
 *  @param unlink_count_ Number of unlinks in the diagram. (This is necessary as PD codes cannot track trivial unlinks.
 *
 */

template<typename ExtInt, typename ExtInt2, typename ExtInt3>
PlanarDiagram(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const ExtInt3 unlink_count_
)
:   PlanarDiagram(
        int_cast<Int>(crossing_count_),
        int_cast<Int>(unlink_count_)
    )
{
    static_assert( IntQ<ExtInt>, "" );
    
    if( crossing_count_ <= 0 )
    {
        return ;
    }
    
    A_cross.Fill(-1);

    // The maximally allowed arc index.
    const Int max_a = 2 * crossing_count_ - 1;
    
    for( Int c = 0; c < crossing_count; ++c )
    {
        Int X [5];
        copy_buffer<5>( &pd_codes_[5*c], &X[0] );
        
        if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
        {
            eprint( ClassName()+"(): There is a pd code entry that is greater than 2 * number of crosssings - 1." );
            return;
        }
        
        bool righthandedQ = (X[4] > 0);
        
        if( righthandedQ )
        {
            C_state[c] = CrossingState::RightHanded;
            
            /*
             *    X[2]           X[1]
             *          ^     ^
             *           \   /
             *            \ /
             *             / <--- c
             *            ^ ^
             *           /   \
             *          /     \
             *    X[3]           X[0]
             */
            
            // Unless there is a wrap-around we have X[2] = X[0] + 1 and X[1] = X[3] + 1.
            // So A_cross(X[0],Head) and A_cross(X[2],Tail) will lie directly next to
            // each other as will A_cross(X[3],Head) and A_cross(X[1],Tail).
            // So this odd-appearing way of accessing A_cross is optimal.
            
            A_cross(X[0],Head) = c;
            A_cross(X[2],Tail) = c;
            A_cross(X[3],Head) = c;
            A_cross(X[1],Tail) = c;

            C_arcs(c,Out,Left ) = X[2];
            C_arcs(c,Out,Right) = X[1];
            C_arcs(c,In ,Left ) = X[3];
            C_arcs(c,In ,Right) = X[0];
        }
        else // left-handed
        {
            C_state[c] = CrossingState::LeftHanded;
            
            /*
             *    X[3]           X[2]
             *          ^     ^
             *           \   /
             *            \ /
             *             \ <--- c
             *            ^ ^
             *           /   \
             *          /     \
             *    X[0]           X[1]
             */
            
            // Unless there is a wrap-around we have X[2] = X[0] + 1 and X[3] = X[1] + 1.
            // So A_cross(X[0],Head) and A_cross(X[2],Tail) will lie directly next to
            // each other as will A_cross(X[1],Head) and A_cross(X[3],Tail).
            // So this odd-appearing way of accessing A_cross is optimal.
            
            A_cross(X[0],Head) = c;
            A_cross(X[2],Tail) = c;
            A_cross(X[1],Head) = c;
            A_cross(X[3],Tail) = c;
            
            C_arcs(c,Out,Left ) = X[3];
            C_arcs(c,Out,Right) = X[2];
            C_arcs(c,In ,Left ) = X[0];
            C_arcs(c,In ,Right) = X[1];
        }
    }
    
    fill_buffer( A_state.data(), ArcState::Active, arc_count );
}

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

                const bool side = (C_arcs(c_prev,Out,Right) == a);

                mptr<Int> pd = pdcode.data(c);
                
                if( RightHandedQ(state) )
                {
                    pd[4] = 1;
                    
                    if( side == Left )
                    {
                        /* a_counter
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
                         */
                        
                        pd[2] = a_counter;
                    }
                    else // if( side == Right )
                    {
                        /*                a_counter
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
                         */
                        
                        pd[1] = a_counter;
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pd[4] = -1;
                    
                    if( side == Left )
                    {
                        /* a_counter
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
                         */
                        
                        pd[3] = a_counter;
                    }
                    else // if( side == Right )
                    {
                        /*                a_counter
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
                         */
                        
                        pd[2] = a_counter;
                    }
                }
            }
            
            // Tell c_next that arc a_counter goes into it.
            {
                const CrossingState state = C_state[c_next];
                const Int           c     = C_labels[c_next];

                const bool side  = (C_arcs(c_next,In,Right)) == a;
                
                mptr<Int> pd = pdcode.data(c);
                
                if( RightHandedQ(state) )
                {
                    pd[4] = 1;
                    
                    if( side == Left )
                    {
                        /*    X[2]           X[1]
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
                         */
                        
                        pd[3] = a_counter;
                    }
                    else // if( side == Right )
                    {
                        /*    X[2]           X[1]
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
                         */
                        
                        pd[0] = a_counter;
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pd[4] = -1;
                    
                    if( side == Left )
                    {
                        /*    X[3]           X[2]
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
                         */
                        
                        pd[0] = a_counter;
                    }
                    else // if( lr == Right )
                    {
                        /*    X[3]           X[2]
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
                         */
                        
                        pd[1] = a_counter;
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
