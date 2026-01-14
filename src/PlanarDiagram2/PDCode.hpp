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
 */

template<typename T = Int, bool labelsQ = false>
Tensor2<T,Int> PDCode() const
{
    auto tag = []()
    {
        return MethodName("PDCode")
        + "<" + TypeName<T>
        + "," + (labelsQ ? "w/ labels" : "w/o labels")
        + ">";
    };
    
    TOOLS_PTIMER(timer,tag());
    
    static_assert( IntQ<T>, "" );
    
    Tensor2<T,Int> pd_code;
    
    // We do this to suppress a warning by `Traverse`.
    if( !ValidQ() )
    {
        return pd_code;
    }
    
    if( std::cmp_greater( arc_count, std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(tag() + ": Requested type " + TypeName<T> + " cannot store PD code for this diagram.");
        
        return pd_code;
    }
    
    pd_code = Tensor2<T,Int> ( crossing_count, Int(5) );

    this->WritePDCode<T,labelsQ>(pd_code.data());
    
    return pd_code;
}

template<typename T, bool labelsQ = false>
void WritePDCode( mptr<T> pd_code ) const
{
    static_assert( IntQ<T>, "" );
    
    TOOLS_PTIMER(timer,ClassName()+"::WritePDCode"
        + "<" + TypeName<T>
        + "," + (labelsQ ? "w/ labels" : "w/o labels")
        + ">");
    
    constexpr T T_LeftHanded  = SignedIntQ<T> ? T(-1) : T(0);
    constexpr T T_RightHanded = SignedIntQ<T> ? T( 1) : T(1);
    
    this->template Traverse<true,labelsQ>(
        [&pd_code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)lc;
            (void)c_0;
            (void)c_1;
            (void)c_0_visitedQ;
            (void)c_1_visitedQ;
            
            // Tell c_0_pos that arc a_pos goes out of it.
            {
                const bool side          = ArcSide(a,Tail,c_0);
                const bool right_handedQ = CrossingRightHandedQ(c_0);

                mptr<T> X = &pd_code[Int(5) * c_0_pos];

                if( right_handedQ )
                {
                    X[4] = T_RightHanded;
                    
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
                        
                        X[2] = static_cast<T>(a_pos);
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
                        
                        X[1] = static_cast<T>(a_pos);
                    }
                }
                else // if( !right_handedQ )
                {
                    X[4] = T_LeftHanded;
                    
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
                        
                        X[3] = static_cast<T>(a_pos);
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
                        
                        X[2] = static_cast<T>(a_pos);
                    }
                }
            }
            
            // Tell c_1_pos that arc a_pos goes into it.
            {
                const bool side          = ArcSide(a,Head,c_1);
                const bool right_handedQ = CrossingRightHandedQ(c_1);
                
                mptr<T> X = &pd_code[Int(5) * c_1_pos];

                if( right_handedQ )
                {
                    X[4] = T_RightHanded;
                    
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
                        
                        X[3] = static_cast<T>(a_pos);
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
                        
                        X[0] = static_cast<T>(a_pos);
                    }
                }
                else // if( !right_handedQ )
                {
                    X[4] = T_LeftHanded;
                    
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
                        
                        X[0] = static_cast<T>(a_pos);
                    }
                    else // if( side == Right )
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
                        
                        X[1] = static_cast<T>(a_pos);
                    }
                }
            }
        }
    );
}


template<typename T = Int>
std::tuple<Tensor2<T,Int>,Tensor1<T,Int>,Tensor1<T,Int>> PDCodeWithLabels()
{
    Tensor2<T,Int> pd_code  = this->template PDCode<T,true,true>();
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
 *  @param compressQ If set to `true`, then the internal representation will be recompressed so that `CompressedOrderQ()` returns `true`. If this is set to `false`, then the output `CompressedOrderQ()` might be `true` or `false`.
 *
 *  @param proven_minimalQ_ If this is set to `true`, then simplification routines may assume that this diagram is irreducible and terminate early. Caution: Set this to `true` only if you know what you are doing!
 *
 *  @tparam checksQ Whether inputs shall be checked for correctness.
 */

template<bool checksQ = true, typename ExtInt, typename ExtInt2>
static PD_T FromSignedPDCode(
    cptr<ExtInt>  pd_codes,
    const ExtInt2 crossing_count,
    const bool    proven_minimalQ_ = false,
    const bool    compressQ = true
)
{
    return FromPDCode<true,checksQ>(
        pd_codes, crossing_count, proven_minimalQ_, compressQ
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
 *  @param compressQ If set to `true`, then the internal representation will be recompressed so that `CompressedOrderQ()` returns `true`. It this is set to `false`, then the output `CompressedOrderQ()` might be `true` or `false`.
 *
 *  @param proven_minimalQ_ If this is set to `true`, then simplification routines may assume that this diagram is minimal and terminate early. Caution: Set this to `true` only if you know what you are doing!
 *
 *  @tparam checksQ Whether inputs shall be checked for correctness.
 */

template<bool checksQ = true, typename ExtInt, typename ExtInt2>
static PD_T FromUnsignedPDCode(
    cptr<ExtInt>  pd_codes,
    const ExtInt2 crossing_count,
    const bool    proven_minimalQ_ = false,
    const bool    compressQ = true
    
)
{
    return FromPDCode<false,checksQ>(
        pd_codes, crossing_count, proven_minimalQ_, compressQ
    );
}

public:

/*!
 * Deduces the handedness of a crossing from the entries `X[0]`, `X[1]`, `X[2]`, `X[3]` alone. This only works if every link component has more than two arcs and if all arcs in each component are numbered consecutively.
 */

static constexpr CrossingState_T PDCodeHandedness( mptr<Int> X )
{
    const Int i = int_cast<Int>(X[0]);
    const Int j = int_cast<Int>(X[1]);
    const Int k = int_cast<Int>(X[2]);
    const Int l = int_cast<Int>(X[3]);
    
    /* This is what we know so far:
     *
     *      l \     ^ k
     *         \   /
     *          \ /
     *           \
     *          / \
     *         /   \
     *      i /     \ j
     */

    // Generically, we should have l = j + 1 or j = l + 1.
    // But of course, we also have to treat the edge case where
    // j and l differ by more than one due to the wrap-around at the end
    // of a connected component.
    
    // I "stole" this pretty neat code snippet from the KnotTheory Mathematica package by Dror Bar-Natan.
    
    if ( (i == j) || (k == l) || (j == l + 1) || (l > j + 1) )
    {
        /* These are right-handed:
         *
         *       O       O            O-------O
         *      l \     ^ k          l \     ^ k
         *         \   /                \   /
         *          \ /                  \ /
         *           \                    \
         *          / \                  / \
         *         /   \                /   \
         *      i /     v j          i /     v j
         *       O---<---O            O       O
         *
         *       O       O            O       O
         *      l \     ^ k     j + x  \     ^ k
         *         \   /                \   /
         *          \ /                  \ /
         *           \                    \
         *          / \                  / \
         *         /   \                /   \
         *      i /     v l + 1     i  /     v j
         *       O       O            O       O
         */
        
        return CrossingState_T::RightHanded;
    }
    else if ( (i == l) || (j == k) || (l == j + 1) || (j > l + 1) )
    {
        /* These are left-handed:
         *
         *       O       O            O       O
         *      l|^     ^ k          l ^     ^|k
         *       | \   /                \   / |
         *       |  \ /                  \ /  |
         *       |   \                    \   |
         *       |  / \                  / \  |
         *       | /   \                /   \ |
         *      i|/     \ j          i /     \|j
         *       O       O            O       O
         *
         *       O       O            O       O
         *    j+1 ^     ^ k         l  ^     ^ k
         *         \   /                \   /
         *          \ /                  \ /
         *           \                    \
         *          / \                  / \
         *         /   \                /   \
         *      i /     \ j         i  /     \ l + x
         *       O       O            O       O
         */
        return CrossingState_T::LeftHanded;
    }
    else
    {
        eprint( std::string("PDHandedness: Handedness of {") + ToString(i) + "," + ToString(i) + "," + ToString(j) + "," + ToString(k) + "," + ToString(l) + "} could not be determined. Make sure that consecutive arcs on each component have consecutive labels (except the wrap-around, of course).");
        
        return CrossingState_T::Inactive;
    }
}

private:

template<bool PDsignedQ, bool checksQ = true, typename ExtInt, typename ExtInt2>
static PD_T FromPDCode(
    cptr<ExtInt> pd_codes_,
    const ExtInt2 crossing_count_,
    const bool proven_minimalQ_ = false,
    const bool compressQ = false
)
{
    // needs to know all member variables
    
    static_assert( IntQ<ExtInt>, "" );
    static_assert( IntQ<ExtInt2>, "" );
    
    std::string tag = MethodName("FromPDCode") + "<" + ToString(PDsignedQ) + "," + ToString(checksQ) + ">";

    PD_T pd (int_cast<Int>(crossing_count_));
    
    constexpr Int d = PDsignedQ ? 5 : 4;
    
    if( crossing_count_ <= ExtInt2(0) )
    {
        return Unknot(Int(0));
    }
    
    pd.proven_minimalQ = proven_minimalQ_;

    // The maximally allowed arc index.
    const Int max_a = pd.max_arc_count - 1;
    
    for( Int c = 0; c < pd.max_crossing_count; ++c )
    {
        Int X [4];
        ExtInt state;
        
        copy_buffer<4>( &pd_codes_[d*c], &X[0] );
        
        if constexpr ( PDsignedQ )
        {
            state = pd_codes_[d*c + 4];
        }
        else
        {
            (void)state;
        }

        if( (X[0] < Int(0)) || (X[1] < Int(0)) || (X[2] < Int(0)) || (X[3] < Int(0)) )
        {
            eprint(tag + ": There is a PD code entry with negative entries. Returning invalid planar diagram.");
            valprint("Code of crossing " + ToString(c),ArrayToString(&X[0],{d}) );
            return InvalidDiagram();
        }
        
        if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
        {
            eprint(tag + ": There is a PD code entry that is greater than number of arcs - 1 = " + ToString(max_a) + ". Returning invalid planar diagram.");
            valprint("Code of crossing " + ToString(c),ArrayToString(&X[0],{d}) );
            return InvalidDiagram();
        }
        
        CrossingState_T c_state;
        
        if constexpr( PDsignedQ )
        {
            c_state = pd.C_state[c] = BooleanToCrossingState(state > ExtInt(0));
        }
        else
        {
            c_state = pd.C_state[c] = PDCodeHandedness(&X[0]);
        }
        
        if( RightHandedQ(c_state) )
        {
            
            /*
             *    X[2]           X[1]
             *          ^     ^
             *           \   /
             *            \ /
             *             / <--- c
             *            ^ ^
             *           /   \.
             *          /     \
             *    X[3]           X[0]
             */
            
            // Unless there is a wrap-around we have X[2] = X[0] + 1 and X[1] = X[3] + 1.
            // So A_cross(X[0],Head) and A_cross(X[2],Tail) will lie directly next to
            // each other as will A_cross(X[3],Head) and A_cross(X[1],Tail).
            // So this odd-appearing way of accessing A_cross is optimal.
            
            pd.C_arcs(c,Out,Left ) = X[2];
            pd.C_arcs(c,Out,Right) = X[1];
            pd.C_arcs(c,In ,Left ) = X[3];
            pd.C_arcs(c,In ,Right) = X[0];

            pd.A_cross(X[0],Head) = c;
            pd.A_cross(X[2],Tail) = c;
            pd.A_cross(X[3],Head) = c;
            pd.A_cross(X[1],Tail) = c;

            pd.A_state(X[0]) = ArcState_T::Active;
            pd.A_state(X[2]) = ArcState_T::Active;
            pd.A_state(X[3]) = ArcState_T::Active;
            pd.A_state(X[1]) = ArcState_T::Active;
        }
       else if( LeftHandedQ(c_state) )
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

           
            pd.C_arcs(c,Out,Left ) = X[3];
            pd.C_arcs(c,Out,Right) = X[2];
            pd.C_arcs(c,In ,Left ) = X[0];
            pd.C_arcs(c,In ,Right) = X[1];
           
            pd.A_cross(X[0],Head) = c;
            pd.A_cross(X[2],Tail) = c;
            pd.A_cross(X[1],Head) = c;
            pd.A_cross(X[3],Tail) = c;

            pd.A_state(X[0]) = ArcState_T::Active;
            pd.A_state(X[2]) = ArcState_T::Active;
            pd.A_state(X[3]) = ArcState_T::Active;
            pd.A_state(X[1]) = ArcState_T::Active;
        }
    }

    pd.crossing_count = pd.max_crossing_count;
    pd.arc_count      = pd.CountActiveArcs();
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(tag + ": Input PD code is invalid because number of active arcs (" + ToString(pd.arc_count)+ ") is not equal to twice the number of active crossings (" + ToString(pd.crossing_count)+ ") . Returning invalid planar diagram.");
        
        return InvalidDiagram();
    }
    
    if constexpr( checksQ )
    {
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
                eprint(tag + ": Input PD code is invalid because crossing " + ToString(c) + " has less than two incoming arcs.");
                
                valprint("crossing " + ToString(c), ArrayToString( pd.C_arcs.data(c), {2,2}) );
                
                all_crossings_initializedQ = false;
            }
            
            if( !out_okayQ )
            {
                eprint(tag + ": Input PD code is invalid because crossing " + ToString(c) + " has less than two outgoing arcs.");
                
                valprint("crossing " + ToString(c), ArrayToString( pd.C_arcs.data(c), {2,2}) );
                
                all_crossings_initializedQ = false;
            }
        }
        
        bool all_arcs_initializedQ = true;
        
        for( Int a = 0; a < pd.max_arc_count; ++a )
        {
            if( pd.A_cross(a,Tail) == Uninitialized )
            {
                eprint(tag + ": Input PD code is invalid because arc " + ToString(a) + " has no crossing assigned to its tail." + (( PDsignedQ ) ? "" : " This can easily happen with unsigned PD codes when the arc labels are not ordered correctly: The arcs in a valid input PD code must be numbered sequentially around each component of the link. For each crossing the arcs most be listed in counterclockwise order starting with the incoming underarc.\n Please check your input code. Or even better: use a signed PD code as the requirements for signed PD codes are less strict."));
                
                valprint("arc " + ToString(a), ArrayToString( pd.A_cross.data(a), {2}) );
                
                all_arcs_initializedQ = false;
            }
            
            if( pd.A_cross(a,Head) == Uninitialized )
            {
                eprint(tag + ": Input PD code is invalid because arc " + ToString(a) + " has no crossing assigned to its head." + (( PDsignedQ ) ? "" : " This can easily happen with unsigned PD codes when the arc labels are not ordered correctly: The arcs in a valid input PD code must be numbered sequentially around each component of the link. For each crossing the arcs most be listed in counterclockwise order starting with the incoming underarc.\n Please check your input code. Or even better: use a signed PD code as the requirements for signed PD codes are less strict.") );
                
                valprint("arc " + ToString(a), ArrayToString( pd.A_cross.data(a), {2}) );
                
                all_arcs_initializedQ = false;
            }
        }
        
        if( (!all_crossings_initializedQ) || (!all_arcs_initializedQ) )
        {
            eprint(tag + ": Input PD code is invalid. Returning invalid planar diagram.");
            
            return InvalidDiagram();
        }
    }
    
    // Compression could be meaningful because PD code stays valid under reordering.
    // But most of the tome PD codes are already compressed. That is somewhat their point.
    if( compressQ )
    {
        // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
        // This also applies the coloring to the arcs.
        return pd.template CreateCompressed<true>();
    }
    else
    {
        pd.ComputeArcColors();
        return pd;
    }
}
