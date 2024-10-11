public:

/*! @brief Construction from PD codes and handedness of crossings.
 *
 *  @param pd_codes Integer array of length `5 * crossing_count_`.
 *  There is one 5-tuple for each crossing.
 *  The first 4 entries in each 5-tuple store the actual PD code.
 *  The last entry gives the handedness of the crossing:
 *    >  0 for a right-handed crossing
 *    <= 0 for a left-handed crossing
 *
 *  @param crossing_count Number of crossings in the diagram.
 *
 *  @param unlink_count Number of unlinks in the diagram. (This is necessary as pure PD codes cannot track trivial unlinks.
 *
 */

template<typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromSignedPDCode(
    cptr<ExtInt> pd_codes,
    const ExtInt2 crossing_count,
    const ExtInt3 unlink_count
)
{
    return PlanarDiagram<Int>::FromPDCode<true>(pd_codes,crossing_count,unlink_count);
}

/*! @brief Construction from PD codes of crossings.
 *
 *  The handedness of the crossing will be inferred from the PD codes. This does not always define a uniquely: A simple counterexample for uniqueness are the Hopf-links.
 *
 *  @param pd_codes Integer array of length `4 * crossing_count`.
 *  There has to be one 4-tuple for each crossing.
 *
 *  @param crossing_count Number of crossings in the diagram.
 *
 *  @param unlink_count Number of unlinks in the diagram. (This is necessary as pure PD codes cannot track trivial unlinks.
 *
 */

template<typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromUnsignedPDCode(
    cptr<ExtInt> pd_codes,
    const ExtInt2 crossing_count,
    const ExtInt3 unlink_count
)
{
    return PlanarDiagram<Int>::FromPDCode<false>(pd_codes,crossing_count,unlink_count);
}

private:

template<bool PDsignedQ, typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromPDCode(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const ExtInt3 unlink_count_
)
{
    PlanarDiagram<Int> pd (int_cast<Int>(crossing_count_),int_cast<Int>(unlink_count_));
    
    constexpr Int d = PDsignedQ ? 5 : 4;
    
    static_assert( IntQ<ExtInt>, "" );
    
    if( crossing_count_ <= 0 )
    {
        return pd;
    }
    
    pd.A_cross.Fill(-1);

    // The maximally allowed arc index.
    const Int max_a = 2 * pd.crossing_count - 1;
    
    for( Int c = 0; c < pd.crossing_count; ++c )
    {
        Int X [d];
        copy_buffer<d>( &pd_codes_[d*c], &X[0] );
        
        if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
        {
            eprint( ClassName()+"(): There is a pd code entry that is greater than 2 * number of crosssings - 1." );
            return PlanarDiagram<Int>();
        }
        
        if constexpr( PDsignedQ )
        {
            pd.C_state[c] = (X[4] > 0)
                          ? CrossingState::RightHanded
                          : CrossingState::LeftHanded;
        }
        else
        {
            pd.C_state[c] = PDCodeHandedness<Int>(&X[0]);
        }
        
        switch( pd.C_state[c] )
        {
            case CrossingState::RightHanded:
            {
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
                
                pd.A_cross(X[0],Head) = c;
                pd.A_cross(X[2],Tail) = c;
                pd.A_cross(X[3],Head) = c;
                pd.A_cross(X[1],Tail) = c;

                pd.C_arcs(c,Out,Left ) = X[2];
                pd.C_arcs(c,Out,Right) = X[1];
                pd.C_arcs(c,In ,Left ) = X[3];
                pd.C_arcs(c,In ,Right) = X[0];
                
                break;
            }
            case CrossingState::LeftHanded:
            {
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
                
                pd.A_cross(X[0],Head) = c;
                pd.A_cross(X[2],Tail) = c;
                pd.A_cross(X[1],Head) = c;
                pd.A_cross(X[3],Tail) = c;
                
                pd.C_arcs(c,Out,Left ) = X[3];
                pd.C_arcs(c,Out,Right) = X[2];
                pd.C_arcs(c,In ,Left ) = X[0];
                pd.C_arcs(c,In ,Right) = X[1];
                
                break;
            }
            default:
            {
                // Do nothing;
                break;
            }
        }
    }
    
    fill_buffer( pd.A_state.data(), ArcState::Active, pd.arc_count );
    
    return pd;
}

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

Tensor2<Int,Int> PDCode()
{
    ptic( ClassName()+"::PDCode" );
    
    const Int m = A_cross.Dimension(0);
    
    Tensor2<Int,Int> pdcode ( crossing_count, 5 );
    
    // We are using C_scratch to keep track of the new crossing's labels.
    C_scratch.Fill(-1);
    
    mptr<Int8> A_visited = reinterpret_cast<Int8 *>(A_scratch.data());
    fill_buffer(A_visited,Int8(0),m);
    
    Int a_counter = 0;
    Int c_counter = 0;
    Int a_ptr     = 0;
    Int a         = 0;
    
    while( a_ptr < m )
    {
        // Search for next arc that is active and has not yet been handled.
        while( ( a_ptr < m ) && ( A_visited[a_ptr]  || (!ArcActiveQ(a_ptr)) ) )
        {
            ++a_ptr;
        }
        
        if( a_ptr >= m )
        {
            break;
        }
        
        a = a_ptr;
        
        C_scratch[A_cross(a,Tail)] = c_counter++;
        
        // Cycle along all arcs in the link component, until we return where we started.
        do
        {
            const Int c_prev = A_cross(a,Tail);
            const Int c_next = A_cross(a,Head);
            
            A_visited[a] = true;
            
            if( C_scratch[c_next] < 0 )
            {
                C_scratch[c_next] = c_counter++;
            }
            
            // Tell c_prev that arc a_counter goes out of it.
            {
                const CrossingState state = C_state [c_prev];
                const Int           c     = C_scratch[c_prev];

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
                         *             / <--- c = C_scratch[c_prev]
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
                         *             / <--- c = C_scratch[c_prev]
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
                         *             \ <--- c = C_scratch[c_prev]
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
                         *             \ <--- c = C_scratch[c_prev]
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
                const CrossingState state = C_state [c_next];
                const Int           c     = C_scratch[c_next];

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
                         *             / <--- c = C_scratch[c_next]
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
                         *             / <--- c = C_scratch[c_next]
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
                         *             \ <--- c = C_scratch[c_next]
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
                         *             \ <--- c = C_scratch[c_next]
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
            
            a = NextArc<Head>(a,c_next);
            
            ++a_counter;
        }
        while( a != a_ptr );
        
        ++a_ptr;
    }
    
    ptoc( ClassName()+"::PDCode");
    
    return pdcode;
}


//
//CrossingState HandednessFromPDCode( cptr<Int> X )
//{
//    const Int i = X[0];
//    const Int j = X[1];
//    const Int k = X[2];
//    const Int l = X[3];
//    
//    if( (i == j) || (k == l) || (j - l == 1) || (l - j > 1) )
//    {
//        return CrossingState::Righthanded;
//    }
//    else if ( (i == l) || (j == k) || (l - j == 1) || (j - l > 1) )
//    {
//        return CrossingState::Lefthanded;
//    }
//    else
//    {
//        return CrossingState::Inactive;
//    }
//}
