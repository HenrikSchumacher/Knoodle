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
 *  The fifth entry stores the handedness of the crossing: if `T` is a signed integer type, then this is encoded as follows:
 *    `+1` for a right-handed crossing,
 *    `-1` for a left-handed crossing.
 *  If `T` is an unsigned integer type, then we use
 *    `+1` for a right-handed crossing,
 *     `0` for a left-handed crossing.
 *
 * @tparam T Integer type to be used for the returned code.
 *
 * @tparam one_basedQ Whether the generated code should be 1-indexed (`true`) or 0-indexed (`false`).
 */
    
template<typename T = Int, bool one_basedQ = false, int method = DefaultTraversalMethod>
Tensor2<T,Int> PDCode()
{
    TOOLS_PTIMER(timer,ClassName()+"::PDCode<" + TypeName<T> + "," + ToString(method) +  ">" );
    
    static_assert( IntQ<T>, "" );
    
    Tensor2<T,Int> pd_code;
    
    // We do this to suppress a warning by `Traverse`.
    if( !ValidQ() )
    {
        return pd_code;
    }
    
    if( std::cmp_greater( arc_count, std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::PDCode: Requested type " + TypeName<T> + " cannot store PD code for this diagram.");
        
        return pd_code;
    }
    
    pd_code = Tensor2<T,Int> ( crossing_count, Int(5) );

    this->WritePDCode<T,one_basedQ,method>(pd_code.data());
    
    return pd_code;
}

template<typename T, bool one_basedQ = false, int method = DefaultTraversalMethod>
void WritePDCode( mptr<T> pd_code )
{
    static_assert( IntQ<T>, "" );
    
    TOOLS_PTIMER(timer,ClassName()+"::WritePDCode"
        + "<" + TypeName<T>
        + "," + ToString(one_basedQ)
        + "," + ToString(method)
        + ">");
    
    constexpr bool crossingsQ = true;
    
    constexpr T T_LeftHanded  = SignedIntQ<T> ? T(-1) : T(0);
    constexpr T T_RightHanded = SignedIntQ<T> ? T( 1) : T(1);
    
    this->template Traverse<crossingsQ,true,-1,method>(
        [&pd_code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0_visitedQ;
            (void)c_1_visitedQ;
            
//            // DEBUGGING
//            if( !ValidIndexQ(a  ) ) { eprint("!ValidIndexQ(a)"); }
//            
//            // DEBUGGING
//            if( !ValidIndexQ(c_0) ) { eprint("!ValidIndexQ(c_0)"); }
//            if( !CrossingActiveQ(c_0) ) { eprint("Inactive crossing!"); }
//            // DEBUGGING
//            if( !ValidIndexQ(c_1) ) { eprint("!ValidIndexQ(c_1)"); }
//            if( !CrossingActiveQ(c_1) ) { eprint("Inactive crossing!"); }
            
            // Tell c_0 that arc a_counter goes out of it.
            {
                const CrossingState state = C_state[c_0];
                
                const bool side = (C_arcs(c_0,Out,Right) == a);
                
                mptr<Int> pd = &pd_code[Int(5) * c_0_pos];
                
                if( RightHandedQ(state) )
                {
                    pd[4] = T_RightHanded;
                    
                    if( side == Left )
                    {
                        /*   a_pos
                         *     =
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_0_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         */
                        
                        pd[2] = static_cast<T>(a_pos);
                    }
                    else // if( side == Right )
                    {
                        /*                  a_pos
                         *                    =
                         *    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_0_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         */
                        
                        pd[1] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pd[4] = T_LeftHanded;
                    
                    if( side == Left )
                    {
                        /*   a_pos
                         *     =
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_0_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         */
                        
                        pd[3] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                    else // if( side == Right )
                    {
                        /*                  a_pos
                         *                    =
                         *    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_0_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         */
                        
                        pd[2] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                }
            }
            
            // Tell c_1 that arc a_counter goes into it.
            {
                const CrossingState state = C_state[c_1];
                const bool side  = (C_arcs(c_1,In,Right)) == a;
                
                mptr<Int> pd = &pd_code[Int(5) * c_1_pos];
                
                if( RightHandedQ(state) )
                {
                    pd[4] = T_RightHanded;
                    
                    if( side == Left )
                    {
                        /*    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_1_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *     =
                         *   a_pos
                         */
                        
                        pd[3] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                    else // if( side == Right )
                    {
                        /*    X[2]           X[1]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             / <--- c_1_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[3]           X[0]
                         *                    =
                         *                  a_pos
                         */
                        
                        pd[0] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                }
                else if( LeftHandedQ(state) )
                {
                    pd[4] = T_LeftHanded;
                    
                    if( side == Left )
                    {
                        /*    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_1_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *     =
                         *   a_pos
                         */
                        
                        pd[0] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                    else // if( lr == Right )
                    {
                        /*    X[3]           X[2]
                         *          ^     ^
                         *           \   /
                         *            \ /
                         *             \ <--- c_1_pos
                         *            ^ ^
                         *           /   \
                         *          /     \
                         *    X[0]           X[1]
                         *                    =
                         *                  a_pos
                         */
                        
                        pd[1] = static_cast<T>(a_pos) + T(one_basedQ);
                    }
                }
            }
        }
    );
}


template<typename T = Int, bool  one_basedQ = false, int method = DefaultTraversalMethod>
std::tuple<Tensor2<T,Int>,Tensor1<T,Int>,Tensor1<T,Int>> PDCodeWithLabels()
{
    Tensor2<T,Int> pd_code  = this->template PDCode<T,true,method>();
    Tensor1<T,Int> C_pos    = C_scratch;
    Tensor1<T,Int> A_pos    = A_scratch;
    
    return std::tuple(pd_code,C_pos,A_pos);
}




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
 *  @param canonicalizeQ If set to `true`, then the internal representation will be canonicalized so that `CanonicallyOrderedQ()` returns `true`. It this is set to `false`, then the output `CanonicallyOrderedQ()` might be `true` or `false`.
 *
 *  @param proven_minimalQ_ If this is set to `true`, then simplification routines may assume that this diagram is irreducible and terminate early. Caution: Set this to `true` only if you know what you are doing!
 *
 *  @tparam one_basedQ Whether the input PD code is be to be interpreted as 1-indexed (`true`) or 0-indexed (`false`).
 */

template<bool one_basedQ = false, typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromSignedPDCode(
    cptr<ExtInt> pd_codes,
    const ExtInt2 crossing_count,
    const ExtInt3 unlink_count,
    const bool    canonicalizeQ = true,
    const bool    proven_minimalQ_ = false
)
{
    return PlanarDiagram<Int>::FromPDCode<true,one_basedQ>(
        pd_codes, crossing_count, unlink_count, canonicalizeQ, proven_minimalQ_
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
 *  @param canonicalizeQ If set to `true`, then the internal representation will be canonicalized so that `CanonicallyOrderedQ()` returns `true`. It this is set to `false`, then the output `CanonicallyOrderedQ()` might be `true` or `false`.
 *
 *  @param proven_minimalQ_ If this is set to `true`, then simplification routines may assume that this diagram is minimal and terminate early. Caution: Set this to `true` only if you know what you are doing!
 *
 *  @tparam one_basedQ Whether the input PD code is be to be interpreted as 1-indexed (`true`) or 0-indexed (`false`).
 */

template<bool one_basedQ = false, typename ExtInt, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromUnsignedPDCode(
    cptr<ExtInt> pd_codes,
    const ExtInt2 crossing_count,
    const ExtInt3 unlink_count,
    const bool    canonicalizeQ = true,
    const bool    proven_minimalQ_ = false
)
{
    return PlanarDiagram<Int>::FromPDCode<false,one_basedQ>(
        pd_codes, crossing_count, unlink_count, canonicalizeQ, proven_minimalQ_
    );
}

private:

template<
    bool PDsignedQ, bool one_basedQ = false,
    typename ExtInt, typename ExtInt2, typename ExtInt3
>
static PlanarDiagram<Int> FromPDCode(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const ExtInt3 unlink_count_,
    const bool canonicalizeQ,
    const bool proven_minimalQ_
)
{
    // TODO: Handle over/under in ArcState.
//    using F_T = Underlying_T<ArcState>;
//    constexpr F_T TailUnder = F_T(1) | ( F_T(0) >> 1);
//    constexpr F_T TailOver  = F_T(1) | ( F_T(1) >> 1);
//    constexpr F_T HeadUnder = F_T(1) | ( F_T(0) >> 2);
//    constexpr F_T HeadOver  = F_T(1) | ( F_T(1) >> 2);
        
    PlanarDiagram<Int> pd (int_cast<Int>(crossing_count_),int_cast<Int>(unlink_count_));
    
    constexpr Int d = PDsignedQ ? 5 : 4;
    
    constexpr Int s = one_basedQ ? Int(1) : Int(0);
    
    static_assert( IntQ<ExtInt>, "" );
    static_assert( IntQ<ExtInt2>, "" );
    static_assert( IntQ<ExtInt3>, "" );
    
    if( crossing_count_ <= Int(0) )
    {
        pd.proven_minimalQ = true;
        return pd;
    }
    
    pd.proven_minimalQ = proven_minimalQ_;

    // The maximally allowed arc index.
    const Int max_a = pd.max_arc_count - 1;
    
    for( Int c = 0; c < pd.max_crossing_count; ++c )
    {
        Int X [d];
        
        
        if constexpr ( one_basedQ )
        {
            X[0] = pd_codes_[d*c + Int(0)] - s;
            X[1] = pd_codes_[d*c + Int(1)] - s;
            X[2] = pd_codes_[d*c + Int(2)] - s;
            X[3] = pd_codes_[d*c + Int(3)] - s;
            
            if constexpr ( PDsignedQ )
            {
                X[4] = pd_codes_[d*c + Int(4)];
            }
        }
        else
        {
            copy_buffer<d>( &pd_codes_[d*c], &X[0] );
        }

        if( (X[0] < Int(0)) || (X[1] < Int(0)) || (X[2] < Int(0)) || (X[3] < Int(0)) )
        {
            eprint( ClassName()+"FromPDCode(): There is a PD code entry with negative entries. Returning invalid PlanarDiagram." );
            valprint("Code of crossing " + ToString(c),ArrayToString(&X[0],{d}) );
            return PlanarDiagram<Int>();
        }
        
        if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
        {
            eprint( ClassName()+"FromPDCode(): There is a PD code entry that is greater than number of arcs - 1 = " + ToString(max_a) + ". Returning invalid PlanarDiagram." );
            valprint("Code of crossing " + ToString(c),ArrayToString(&X[0],{d}) );
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

                // TODO: Handle over/under in ArcState.
                pd.A_state(X[0]) = ArcState::Active;
                pd.A_state(X[1]) = ArcState::Active;
                pd.A_state(X[2]) = ArcState::Active;
                pd.A_state(X[3]) = ArcState::Active;
                
//                pd.A_state(X[0]) |= HeadUnder;
//                pd.A_state(X[1]) |= TailOver;
//                pd.A_state(X[2]) |= TailUnder;
//                pd.A_state(X[3]) |= HeadOver;
                
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
                
                // TODO: Handle over/under in ArcState.
                pd.A_state(X[0]) = ArcState::Active;
                pd.A_state(X[1]) = ArcState::Active;
                pd.A_state(X[2]) = ArcState::Active;
                pd.A_state(X[3]) = ArcState::Active;
                
//                pd.A_state(X[0]) |= HeadUnder;
//                pd.A_state(X[1]) |= HeadOver;
//                pd.A_state(X[2]) |= TailUnder;
//                pd.A_state(X[3]) |= TailOver;
                
                break;
            }
            default:
            {
                eprint(ClassName()+"::FromPDCode: Unclear crossing state.");
                TOOLS_DUMP(pd.C_state[c]);
                // Do nothing;
                break;
            }
        }
    }

    pd.crossing_count = int_cast<Int>(crossing_count_);
    pd.arc_count      = pd.CountActiveArcs();
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(ClassName()+"::FromPDCode: Input PD code is invalid because number of active arcs is not equal to twice the number of active crossings. Returning invalid PlanarDiagram.");
        
        return PlanarDiagram<Int>();
    }
    
    bool all_crossings_initializedQ  = true;
    
    for( Int c = 0; c < pd.max_crossing_count; ++c )
    {
        bool out_okayQ =
            (pd.C_arcs(c,Out,Left ) != Uninitialized)
            &&
            (pd.C_arcs(c,Out,Right) != Uninitialized);
        
        bool in_okayQ =
            (pd.C_arcs(c,In ,Left ) != Uninitialized)
            &&
            (pd.C_arcs(c,In ,Right) != Uninitialized);
        
        
        if( !in_okayQ )
        {
            eprint(ClassName()+"::FromPDCode: Input PD code is invalid because crossing " + ToString(c) + " has less than two incoming arcs.");
            
            valprint("crossing " + ToString(c), ArrayToString( pd.C_arcs.data(c), {2,2}) );
            
            all_crossings_initializedQ = false;
        }
        
        if( !out_okayQ )
        {
            eprint(ClassName()+"::FromPDCode: Input PD code is invalid because crossing " + ToString(c) + " has less than two outgoing arcs.");
            
            valprint("crossing " + ToString(c), ArrayToString( pd.C_arcs.data(c), {2,2}) );
            
            all_crossings_initializedQ = false;
        }
    }
    
    bool all_arcs_initializedQ = true;

    for( Int a = 0; a < pd.max_arc_count; ++a )
    {
        if( pd.A_cross(a,Tail) == Uninitialized )
        {
            eprint(ClassName()+"::FromPDCode: Input PD code is invalid because arc " + ToString(a) + " has no crossing assigned to its tail." + (( PDsignedQ ) ? "" : " This can easily happen with unsigned PD codes when the arc labels are not ordered correctly: The arcs in a valid input PD code must be numbered sequentially around each component of the link. For each crossing the arcs most be listed in counterclockwise order starting with the incoming underarc.\n Please check your input code. Or even better: use a signed PD code as the requirements for signed PD codes are less strict.") );
            
            valprint("arc " + ToString(a), ArrayToString( pd.A_cross.data(a), {2}) );
            
            all_arcs_initializedQ = false;
        }

        if( pd.A_cross(a,Head) == Uninitialized )
        {
            eprint(ClassName()+"::FromPDCode: Input PD code is invalid because arc " + ToString(a) + " has no crossing assigned to its head." + (( PDsignedQ ) ? "" : " This can easily happen with unsigned PD codes when the arc labels are not ordered correctly: The arcs in a valid input PD code must be numbered sequentially around each component of the link. For each crossing the arcs most be listed in counterclockwise order starting with the incoming underarc.\n Please check your input code. Or even better: use a signed PD code as the requirements for signed PD codes are less strict.") );
            
            valprint("arc " + ToString(a), ArrayToString( pd.A_cross.data(a), {2}) );
            
            all_arcs_initializedQ = false;
        }
    }
    
    if( (!all_crossings_initializedQ) || (!all_arcs_initializedQ) )
    {
        eprint(ClassName()+"::FromPDCode: Input PD code is invalid. Returning invalid PlanarDiagram.");
        
        return PlanarDiagram<Int>();
    }
    
    if( canonicalizeQ )
    {
        // We finally call `Canonicalize` to get the ordering of crossings and arcs consistent.
        return pd.Canonicalize();
    }
    else
    {
        return pd;
    }
}
