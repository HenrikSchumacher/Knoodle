public:


struct PDCode_TArgs_T
{
    bool signQ     = true;
    bool colorQ    = true;
    bool farfalleQ = false;
};

friend constexpr std::string ToString( cref<PDCode_TArgs_T> args )
{
    return std::string("{ ")
        + "signQ = " + ToString(args.signQ)
        + ", colorQ = " + ToString(args.colorQ)
        + ", farfalleQ = " + ToString(args.farfalleQ)
        + " }";
}

constexpr Size_T PDCodeCrossingCount( bool farfalleQ ) const
{
    if ( farfalleQ )
    {
        if( InvalidQ() )
        {
            return 0;
        }
        if( AnelloQ() )
        {
            return 1;
        }
        else
        {
            return ToSize_T(CrossingCount());
        }
    }
    else
    {
        return ToSize_T(CrossingCount());
    }
}

static constexpr Size_T PDCodeWidth( bool signQ, bool colorQ )
{
    return 4 + signQ + 2 * colorQ;
}

/*!@brief Returns the pd-codes of the crossings as Tensor2 object.
 *
 *  The information for crossing `c` is given by a row in the output matrix. There 4-7 numbers stored in such a row,.
 *  The first 4 entries `PDCode()(c,0), PDCode()(c,1), PDCode()(c,2), PDCode()(c,3)` represent the arcs attached to crossing c. `PDCode()(c,0)` is the incoming arc that goes under. It is followed by the other arcs in counter-clockwise orientation. This should be compatible with Dror Bar-Natan's _KnotTheory_ package.
 *
 *  The crossing and arc labels  (i.e., a mapping from he old crossings and arcs to the crossings and arcs in the pd code) are generated. They can be retrieved from `CrossingScratchBuffer` and `ArcScratchBuffer` right after the call to `PDCode`. Beware that these internal buffers may be overwritten by other routines.
 *
 *  Depending on the template parameters, there might be further entries `PDCode()(c,4), PDCode()(c,5), PDCode()(c,6)`. Their meaning is explained below.
 *
 * @tparam T Integer type to be used for the returned code.
 *
 * @tparam targs Further options that control the kind of pd code to print:
 *
 * - `targs.signQ` controlas whether the code is signed. If yes, then `PDCode()(c,4)` represents the handedness of crossing `c`; `PDCode()(c,4) > 0` means right-handed, `PDCode()(c,4) <= 9` means left-handed.
 *
 * - `colorQ` controls whether the arc colors ought to be stored. If yes, then the last two entries of PDCode()(c,..) will be the color of the incoming underarc and the incoming over-arc, respectively.
 *

 *
 *  - `targs.faralleQ` If set to `true`, then unknots are represented as "farfalle", i.e., diagrams with a single crossing. This is to make this compatible with Regina.
 */

template<IntQ T = Int, PDCode_TArgs_T targs = PDCode_TArgs_T()>
Tensor2<T,Int> PDCode() const
{
    [[maybe_unused]] auto tag = []()
    {
        std::string s = MethodName("PDCode");
        s += "<";
        s += TypeName<T>;
        s += ",";
        s += ToString(targs);
        s += ">";
        return s;
    };
    
    TOOLS_PTIMER(timer,tag());
    
    Tensor2<T,Int> pd_code;
    
    // We do this to suppress a warning by `Traverse`.
    if( !ValidQ() ) { return pd_code; }
    
    if( std::cmp_greater( arc_count, std::numeric_limits<T>::max() ) )
    {
        error(tag() + ": Requested type " + TypeName<T> + " cannot store PD code for this diagram.");
        
        return pd_code;
    }
    
    pd_code = Tensor2<T,Int> (
        PDCodeCrossingCount(targs.farfalleQ),
        PDCodeWidth(targs.signQ,targs.colorQ)
    );

    this->WritePDCode<T,targs>(pd_code.data());
    
    return pd_code;
}

/*!@brief Writes the pd code to the output raw buffer `pd_code`. Size and meaning of the results are determined by the template parameters. See the documentation of `PDCode` for details.
 */

template<IntQ T, PDCode_TArgs_T targs>
void WritePDCode( mptr<T> pd_code, T offset = 0 ) const
{
    TOOLS_PTIMER(timer,MethodName("WritePDCode")
        + "<" + TypeName<T>
        + "," + ToString(targs)
        + ">");
    
    constexpr T T_LeftHanded  = SignedIntQ<T> ? T(-1) : T(0);
    constexpr T T_RightHanded = SignedIntQ<T> ? T( 1) : T(1);
    
    constexpr Int code_width = static_cast<Int>(PDCodeWidth(targs.signQ,targs.colorQ));
    constexpr Int under_pos = 4 + targs.signQ;
    constexpr Int over_pos  = 5 + targs.signQ;
    
    if constexpr ( targs.farfalleQ )
    {
        if( AnelloQ() )
        {
            pd_code[0] = offset;
            pd_code[1] = offset;
            pd_code[2] = offset + T(1);
            pd_code[3] = offset + T(1);
            
            if constexpr ( targs.signQ )
            {
                pd_code[4] = T_RightHanded;
            }
            
            if constexpr ( targs.colorQ )
            {
                const Int color = FirstColor();
                pd_code[under_pos] = color;
                pd_code[over_pos]  = color;
            }
            
            return;
        }
    }
    
    this->template Traverse<true,true>(
        [pd_code,offset,this](
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

                mptr<T> X = &pd_code[code_width * c_0_pos];

                if( right_handedQ )
                {
                    if constexpr ( targs.signQ ) { X[4] = T_RightHanded; }
                    
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
                        
                        X[2] = static_cast<T>(a_pos) + offset;
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
                        
                        X[1] = static_cast<T>(a_pos) + offset;
                    }
                }
                else // if( !right_handedQ )
                {
                    if constexpr ( targs.signQ ) { X[4] = T_LeftHanded; }
                    
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
                        
                        X[3] = static_cast<T>(a_pos) + offset;
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
                        
                        X[2] = static_cast<T>(a_pos) + offset;
                    }
                }
            }
            
            // Tell c_1_pos that arc a_pos goes into it.
            {
                const bool side          = ArcSide(a,Head,c_1);
                const bool right_handedQ = CrossingRightHandedQ(c_1);
                
                mptr<T> X = &pd_code[code_width * c_1_pos];

                if( right_handedQ )
                {
                    if constexpr ( targs.signQ ) { X[4] = T_RightHanded; }
                    
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
                        
                        X[3] = static_cast<T>(a_pos) + offset;

                        if constexpr ( targs.colorQ ) { X[over_pos] = A_color[a]; }
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
                        
                        X[0] = static_cast<T>(a_pos) + offset;
                        
                        if constexpr ( targs.colorQ ) { X[under_pos] = A_color[a]; }
                    }
                }
                else // if( !right_handedQ )
                {
                    if constexpr ( targs.signQ ) { X[4] = T_LeftHanded; }
                    
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
                        
                        X[0] = static_cast<T>(a_pos) + offset;
                        
                        
                        if constexpr ( targs.colorQ ) { X[under_pos] = A_color[a]; }
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
                        
                        X[1] = static_cast<T>(a_pos) + offset;
                        
                        if constexpr ( targs.colorQ ) { X[over_pos] = A_color[a]; }
                    }
                }
            }
        }
    );
}

public:

/*! @brief Construction from PD codes and handedness of crossings.
 *
 *  @param pd_code Integer array of length `5 * crossing_count_`.
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

template<bool checksQ = true, IntQ T, IntQ ExtInt>
static PD_T FromSignedPDCode(
    cptr<T>      pd_code,
    const ExtInt crossing_count,
    const bool   proven_minimalQ_ = false,
    const bool   compressQ = true
)
{
    return FromPDCode<{.signQ = true, .colorQ = false, .checksQ = checksQ}>(
        pd_code, crossing_count, proven_minimalQ_, compressQ
    );
}

/*! @brief Construction from PD codes of crossings.
 *
 *  The handedness of the crossing will be inferred from the PD codes. This does not always define a uniquely: A simple counterexample for uniqueness are the Hopf-links.
 *
 *  @param pd_code Integer array of length `4 * crossing_count`.
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

template<bool checksQ = true, IntQ T, IntQ ExtInt>
static PD_T FromUnsignedPDCode(
    cptr<T>      pd_code,
    const ExtInt crossing_count,
    const bool   proven_minimalQ_ = false,
    const bool   compressQ = true
    
)
{
    return FromPDCode<false,false,checksQ>(
        pd_code, crossing_count, proven_minimalQ_, compressQ
    );
}

public:

/*!@brief Deduces the handedness of a crossing from the entries `X[0]`, `X[1]`, `X[2]`, `X[3]` alone. This only works if every link component has more than two arcs and if all arcs in each component are numbered consecutively.
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

public:

struct FromPDCode_TArgs_T
{
    bool signQ     = true;
    bool colorQ    = true;
    bool checksQ   = true;
};

friend constexpr std::string ToString( cref<FromPDCode_TArgs_T> args )
{
    return std::string("{ ")
        + "signQ = " + ToString(args.signQ)
        + ", colorQ = " + ToString(args.colorQ)
        + ", checksQ = " + ToString(args.checksQ)
        + " }";
};

template<FromPDCode_TArgs_T targs, IntQ T, IntQ ExtInt>
static PD_T PDCodeSize(
                       )
{
}
                       

                       
/*!@brief Loads a PlanarDiagram from a pd code. Between 4 and 7 numbers are expected per crossing. The template parameters control how many and how they are interpreted.
 *
 * @tparam targs Options that govern the behavior:
 *
 * - `targs.signQ` Whether the code is signed. If yes, then the 4-th number per crossing represents the handedness of crossing `c`; ` 0` means right-handed, `<= 0` means left-handed.
 *
 * - `targs.colorQ` Whether the arc colors ought to be used. If yes, then the last two numbers per crossing are interpreted as the color of the incoming underarc and the incoming over-arc, respectively.
 *
 * - `targs.checksQ` If set to `true`, then a few consistency checks are performed.
 *
 * If no handedness is given, then an attempt will be made to guess these parameters. Note that that there are cases in which an unsigned pd code does not uniquely determine a link diagram.
 *
 * If color information is given, then the link components are colored in the "natural" order they a reversed by the `Traverse` routine.
 */

template<FromPDCode_TArgs_T targs, IntQ T, IntQ ExtInt>
static PD_T FromPDCode(
    cptr<T>      pd_code,
    const ExtInt crossing_count_,
    const bool   proven_minimalQ_ = false,
    const bool   compressQ = false
)
{
    // needs to know all member variables
    [[maybe_unused]] auto tag = [](){
        std::string s = MethodName("FromPDCode");
        s += "<";
        s += ToString(targs);
        s += ",";
        s += TypeName<T>;
        s += ",";
        s += TypeName<ExtInt>;
        s += ">";
        return s;
    };
    
    TOOLS_PTIMER(timer,tag());

    PD_T pd (int_cast<Int>(crossing_count_));
    
    constexpr Int code_width = int_cast<Int>(PD_T::PDCodeWidth(targs.signQ,targs.colorQ));
    constexpr Int under_pos = 4 + targs.signQ;
    constexpr Int over_pos  = 5 + targs.signQ;
    
    if( crossing_count_ <= ExtInt(0) )
    {
        return Unknot(Int(0));
    }
    
    pd.proven_minimalQ = proven_minimalQ_;

    // The maximally allowed arc index.
    const Int max_a = pd.MaxArcCount() - Int(1);
    
    for( Int c = 0; c < pd.max_crossing_count; ++c )
    {
        Int X [code_width];
        T state;
        
        copy_buffer<code_width>( &pd_code[code_width * c], &X[0] );
        
        if constexpr ( targs.signQ )
        {
            state = X[4];
        }
        else
        {
            (void)state;
        }

        if( (X[0] < Int(0)) || (X[1] < Int(0)) || (X[2] < Int(0)) || (X[3] < Int(0)) )
        {
            eprint(tag() + ": There is a negative entry in the PD code. Returning invalid planar diagram.");
            valprint("Code of crossing " + ToString(c), OutString::FromVector(&X[0],code_width) );
            return InvalidDiagram();
        }
        
        if( (X[0] > max_a) || (X[1] > max_a) || (X[2] > max_a) || (X[3] > max_a) )
        {
            eprint(tag() + ": There is an entry greater than number of arcs - 1 = " + ToString(max_a) + " in the PD code. Returning invalid planar diagram.");
            valprint("Code of crossing " + ToString(c), OutString::FromVector(&X[0],code_width) );
            return InvalidDiagram();
        }
        
        if constexpr ( targs.colorQ )
        {
            if( (X[over_pos] < Int(0)) || (X[under_pos] < Int(0)) )
            {
                eprint(tag() + ": There is a negative color in the PD code. Returning invalid planar diagram.");
                valprint("Code of crossing " + ToString(c), OutString::FromVector(&X[0],code_width) );
                return InvalidDiagram();
            }
        }
        
        CrossingState_T c_state;
        
        if constexpr( targs.signQ )
        {
            c_state = pd.C_state[c] = BooleanToCrossingState(state > T(0));
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
//            pd.A_state(X[2]) = ArcState_T::Active;
            pd.A_state(X[3]) = ArcState_T::Active;
//            pd.A_state(X[1]) = ArcState_T::Active;
            
            if constexpr ( targs.colorQ )
            {
                pd.A_color(X[3]) = X[over_pos];
                pd.A_color(X[0]) = X[under_pos];
            }
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
//            pd.A_state(X[2]) = ArcState_T::Active;
//            pd.A_state(X[3]) = ArcState_T::Active;
            pd.A_state(X[1]) = ArcState_T::Active;
            
            if constexpr ( targs.colorQ )
            {
                pd.A_color(X[0]) = X[under_pos];
                pd.A_color(X[1]) = X[over_pos];
            }
        }
    }

    pd.crossing_count = pd.max_crossing_count;
    pd.arc_count      = pd.CountActiveArcs();
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(tag() + ": Input PD code is invalid because number of active arcs (" + ToString(pd.arc_count)+ ") is not equal to twice the number of active crossings (" + ToString(pd.crossing_count)+ "). Returning invalid planar diagram.");
        
        logvalprint(
            "problematic pd code",
            OutString::FromMatrix( pd_code, crossing_count_, code_width )
        );
        
        return InvalidDiagram();
    }
    
    if constexpr( targs.checksQ )
    {
        bool all_crossings_initializedQ  = true;
        
        for( Int c = 0; c < pd.max_crossing_count; ++c )
        {
            bool out_okayQ = (pd.C_arcs(c,Out,Left ) != Uninitialized) && (pd.C_arcs(c,Out,Right) != Uninitialized);
            
            bool in_okayQ  = (pd.C_arcs(c,In ,Left ) != Uninitialized) && (pd.C_arcs(c,In ,Right) != Uninitialized);
            
            
            if( !in_okayQ )
            {
                eprint(tag() + ": Input PD code is invalid because crossing " + ToString(c) + " has less than two incoming arcs.");
                
                valprint("crossing " + ToString(c), OutString::FromMatrix( pd.C_arcs.data(c), 2, 2 ) );
                
                all_crossings_initializedQ = false;
            }
            
            if( !out_okayQ )
            {
                eprint(tag() + ": Input PD code is invalid because crossing " + ToString(c) + " has less than two outgoing arcs.");
                
                valprint("crossing " + ToString(c), OutString::FromMatrix( pd.C_arcs.data(c), 2, 2 ) );
                
                all_crossings_initializedQ = false;
            }
        }
        
        bool all_arcs_initializedQ = true;
        
        for( Int a = 0; a < pd.MaxArcCount(); ++a )
        {
            if( pd.A_cross(a,Tail) == Uninitialized )
            {
                eprint(tag() + ": Input PD code is invalid because arc " + ToString(a) + " has no crossing assigned to its tail." + (( targs.signQ ) ? "" : " This can easily happen with unsigned PD codes when the arc labels are not ordered correctly: The arcs in a valid input PD code must be numbered sequentially around each component of the link. For each crossing the arcs most be listed in counterclockwise order starting with the incoming underarc.\n Please check your input code. Or even better: use a signed PD code as the requirements for signed PD codes are less strict."));
                
                valprint("arc " + ToString(a), OutString::FromVector( pd.A_cross.data(a), 2 ) );
                
                all_arcs_initializedQ = false;
            }
            
            if( pd.A_cross(a,Head) == Uninitialized )
            {
                eprint(tag() + ": Input PD code is invalid because arc " + ToString(a) + " has no crossing assigned to its head." + (( targs.signQ ) ? "" : " This can easily happen with unsigned PD codes when the arc labels are not ordered correctly: The arcs in a valid input PD code must be numbered sequentially around each component of the link. For each crossing the arcs most be listed in counterclockwise order starting with the incoming underarc.\n Please check your input code. Or even better: use a signed PD code as the requirements for signed PD codes are less strict.") );
                
                valprint("arc " + ToString(a), OutString::FromVector( pd.A_cross.data(a), 2 ) );
                
                all_arcs_initializedQ = false;
            }
        }
        
        if( (!all_crossings_initializedQ) || (!all_arcs_initializedQ) )
        {
            eprint(tag() + ": Input PD code is invalid. Returning invalid planar diagram.");
            
            return InvalidDiagram();
        }
    }
    
    // Compression could be meaningful because PD code stays valid under reordering.
    // But most of the tome PD codes are already compressed. That is somewhat their point.
    if( compressQ )
    {
        // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
        // This also applies a coloring to the arcs of `colorQ == true`.
        return pd.template CreateCompressed<!targs.colorQ>();
    }
    else
    {
        if constexpr ( !targs.colorQ )
        {
            pd.ComputeArcColors();
        }
        return pd;
    }
}


template<IntQ T, IntNotBoolQ ExtInt, IntNotBoolQ ExtInt2>
static PD_T FromPDCode(
    cptr<T>       pd_code,
    const ExtInt  crossing_count_,
    const ExtInt2 code_width,
    const bool    proven_minimalQ_ = false,
    const bool    compressQ = false
)
{
    if( crossing_count_ == ExtInt(0) )
    {
        return PD_T();
    }
    else if( code_width == ExtInt2(4) )
    {
        return FromPDCode<{.signQ = 0, .colorQ = 0}>(
            pd_code, crossing_count_, proven_minimalQ_, compressQ
        );
    }
    else if( code_width == ExtInt2(5) )
    {
        return FromPDCode<{.signQ = 1, .colorQ = 0}>(
             pd_code, crossing_count_, proven_minimalQ_, compressQ
        );
    }
    else if( code_width == ExtInt2(6) )
    {
        return FromPDCode<{.signQ = 0, .colorQ = 1}>(
            pd_code, crossing_count_, proven_minimalQ_, compressQ
        );
    }
    else if( code_width == ExtInt2(7) )
    {
        return FromPDCode<{.signQ = 1, .colorQ = 1}>(
            pd_code, crossing_count_, proven_minimalQ_, compressQ
        );
    }
    else
    {
        eprint(MethodName("FromPDCode") +": Code width is not equal to 4, 5, 6, or 7. Returning invalid diagram.");
        return PD_T();
    }
}

static PD_T FromPDCodeString(
    mref<InString> s,
    const bool proven_minimalQ_ = false,
    const bool compressQ = false
)
{
    Int crossing_counter = 0;
    std::vector<Int> pd_buffer;
    Int x = 0;
    Size_T code_width = 1;

    // Read the first line and figure out code_width.
    for( Size_T i = 0; i < Size_T(8); ++i )
    {
        s.SkipWhiteSpace();
        s.Take(x);
        pd_buffer.push_back(x);
        
        if( s.CurrentChar() == '\n' )
        {
            break;
        }
        else
        {
            ++code_width;
        }
    }
    
    ++crossing_counter;
    
    if( code_width < Size_T(4) )
    {
        eprint(MethodName("FromPDCodeString") + ": Input has less than 4 tokens per line. This violates the assumptions for an unsigned and uncolored PD code. Returning invalid diagram.");
        return InvalidDiagram();
    }
    
    if( code_width > Size_T(7) )
    {
        eprint(MethodName("FromPDCodeString") + ": Input has more than 7 tokens per line. This violates the assumptions for a signed and colored PD code. Returning invalid diagram.");
        
        return InvalidDiagram();
    }
    
    while( !s.EmptyQ() && !s.FailedQ() )
    {
        // We have to be careful here, because the last line may easily end with an '\n'.
        for( Size_T i = 0; i < code_width; ++i )
        {
            s.SkipWhiteSpace();
            s.Take(x);
            pd_buffer.push_back(x);
        }
        
        ++crossing_counter;
        
        if( s.EmptyQ() ) { break; }
    }
    
    if( s.FailedQ() )
    {
        eprint(MethodName("FromPDCodeString") + ": Reading failed. Returning invalid diagram.");
        return InvalidDiagram();
    }
    
    PD_T pd = FromPDCode( &pd_buffer[0], crossing_counter, code_width, proven_minimalQ_, compressQ );

    return pd;
}

static PD_T FromPDCodeFile(
    cref<std::filesystem::path> file,
    const bool proven_minimalQ_ = false,
    const bool compressQ = false
)
{
    InString s ( file );
    
    return FromPDCodeString( s, proven_minimalQ_, compressQ );
}
