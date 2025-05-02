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
 *  @param provably_minimalQ_ If this is set to `true`, then simplification routines may assume that this diagram is irreducible and terminate early. Caution: Set this to `true` only if you know what you are doing!
 */

template<typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromSignedPDCode(
    cptr<ExtInt> pd_codes,
    const ExtInt2 crossing_count,
    const ExtInt3 unlink_count,
    const bool    provably_minimalQ_ = false
)
{
    return PlanarDiagram<Int>::FromPDCode<true>(
        pd_codes, crossing_count, unlink_count, provably_minimalQ_
    );
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
 *  @param provably_minimalQ_ If this is set to `true`, then simplification routines may assume that this diagram is minimal and terminate early. Caution: Set this to `true` only if you know what you are doing!
 */

template<typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromUnsignedPDCode(
    cptr<ExtInt> pd_codes,
    const ExtInt2 crossing_count,
    const ExtInt3 unlink_count,
    const bool    provably_minimalQ_ = false
)
{
    return PlanarDiagram<Int>::FromPDCode<false>(
        pd_codes, crossing_count, unlink_count, provably_minimalQ_
    );
}

private:

template<bool PDsignedQ, typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromPDCode(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const ExtInt3 unlink_count_,
    const bool    provably_minimalQ_ = false
)
{
    PlanarDiagram<Int> pd (int_cast<Int>(crossing_count_),int_cast<Int>(unlink_count_));
    
    constexpr Int d = PDsignedQ ? 5 : 4;
    
    static_assert( IntQ<ExtInt>, "" );
    
    if( crossing_count_ <= 0 )
    {
        return pd;
    }
    
    // The maximally allowed arc index.
    const Int max_a = 2 * pd.max_crossing_count - 1;
    
    for( Int c = 0; c < pd.max_crossing_count; ++c )
    {
        Int X [d];
        copy_buffer<d>( &pd_codes_[d*c], &X[0] );
        
        if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
        {
            eprint( ClassName()+"(): There is a pd code entry that is greater than 2 * number of crosssings - 1. Returning invalid PlanarDiagram." );
            return PlanarDiagram<Int>();
        }
        
        if constexpr( PDsignedQ )
        {
            pd.C_state[c] = (X[4] > Int(0))
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
                
                pd.A_state(X[0]) = ArcState::Active;
                pd.A_state(X[1]) = ArcState::Active;
                pd.A_state(X[2]) = ArcState::Active;
                pd.A_state(X[3]) = ArcState::Active;
                
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
                
                pd.A_state(X[0]) = ArcState::Active;
                pd.A_state(X[1]) = ArcState::Active;
                pd.A_state(X[2]) = ArcState::Active;
                pd.A_state(X[3]) = ArcState::Active;
                
                break;
            }
            default:
            {
                // Do nothing;
                break;
            }
        }
    }
    
    pd.crossing_count    = crossing_count_;
    pd.arc_count         = pd.CountActiveArcs();
    pd.provably_minimalQ = provably_minimalQ_;
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(ClassName() + "FromPDCode: Input PD code is invalid because number of active arcs is not equal to twice the number of active crossings. Returning invalid PlanarDiagram.");
        
        return PlanarDiagram<Int>();
    }
    
    // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
    return pd.CreateCompressed();
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

template<typename T = Int>
Tensor2<T,Int> PDCode()
{
    TOOLS_PTIC(ClassName()+"::PDCode<" + TypeName<T> + ">" );
    
    static_assert( SignedIntQ<T>, "" );
    
    Tensor2<T,Int> pd_code;
    
    // We do this to suppress a warning by `Traverse`.
    if( !ValidQ() )
    {
        goto Exit;
    }
    
    if( std::cmp_greater( arc_count, std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName() + "::PDCode: Requested type " + TypeName<T> + " cannot store PD code for this diagram.");
        
        goto Exit;
    }
    
    pd_code = Tensor2<T,Int> ( crossing_count, Int(5) );
    
    Traverse(
        []( const Int lc, const Int lc_begin ){},
        [&pd_code,this](
            const Int a,   const Int a_label,
            const Int c_0, const Int c_0_label, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_label, const bool c_1_visitedQ
        )
        {
            // Tell c_0 that arc a_counter goes out of it.
            {
                const CrossingState state = C_state[c_0];
                const bool side = (C_arcs(c_0,Out,Right) == a);
                
                mptr<Int> pd = pd_code.data(c_0_label);
                
                if( RightHandedQ(state) )
                {
                    pd[4] = 1;
                    
                    if( side == Left )
                    {
                        /*  a_label
                         *     =
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_0_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         */
                        
                        pd[2] = static_cast<T>(a_label);
                    }
                    else // if( side == Right )
                    {
                        /*                 a_label
                         *                    =
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_0_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         */
                        
                        pd[1] = static_cast<T>(a_label);
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pd[4] = -1;
                    
                    if( side == Left )
                    {
                        /*  a_label
                         *     =
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_0_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         */
                        
                        pd[3] = static_cast<T>(a_label);
                    }
                    else // if( side == Right )
                    {
                        /*                 a_label
                         *                    =
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_0_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         */
                        
                        pd[2] = static_cast<T>(a_label);
                    }
                }
            }
            
            // Tell c_1 that arc a_counter goes into it.
            {
                const CrossingState state = C_state[c_1];
                const bool side  = (C_arcs(c_1,In,Right)) == a;
                
                mptr<Int> pd = pd_code.data(c_1_label);
                
                if( RightHandedQ(state) )
                {
                    pd[4] = 1;
                    
                    if( side == Left )
                    {
                        /*    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_1_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *     =
                         *  a_label
                         */
                        
                        pd[3] = static_cast<T>(a_label);
                    }
                    else // if( side == Right )
                    {
                        /*    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_1_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *                    =
                         *                 a_label
                         */
                        
                        pd[0] = static_cast<T>(a_label);
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
                         *             \ <--- c_1_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *     =
                         *  a_label
                         */
                        
                        pd[0] = static_cast<T>(a_label);
                    }
                    else // if( lr == Right )
                    {
                        /*    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_1_label
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *                    =
                         *                 a_label
                         */
                        
                        pd[1] = static_cast<T>(a_label);
                    }
                }
            }
        },
        []( const Int lc, const Int lc_begin, const Int lc_end ){}
    );
    
Exit:
    
    TOOLS_PTOC(ClassName()+"::PDCode<" + TypeName<T> + ">" );
    
    return pd_code;
}

std::tuple<Tensor2<Int,Int>,Tensor1<Int,Int>,Tensor1<Int,Int>> PDCodeWithLabels()
{
    Tensor2<Int,Int> pd_code  = PDCode();
    Tensor1<Int,Int> C_labels = C_scratch;
    Tensor1<Int,Int> A_labels = A_scratch;
    
    return std::tuple(pd_code,C_labels,A_labels);
}
