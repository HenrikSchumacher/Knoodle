public:

template<IntQ T = UInt>
Tensor1<T,Int> LongMacLeodCode() const
{
    constexpr auto tag = MethodName("LongMacLeodCode<") + TypeName<T> + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(tag, ": This diagram has several link components. And, you know, there can be only one.>");
        return Tensor1<T,Int>();
    }
    
    if( !ValidQ() )
    {
        wprint(tag, ": Trying to compute long MacLeod code of invalid diagram. Returning empty vector.");
        return Tensor1<T,Int>();
    }
    
    this-> template CheckMacLeodReturnType<T>();
    
    Tensor1<T,Int> code( arc_count );
    
    this->WriteLongMacLeodCode<T>(code.data());
    
    return code;
}

template<IntQ T>
void WriteLongMacLeodCode( mptr<T> code ) const
{
    constexpr auto tag = MethodName("WriteLongMacLeodCode") + "<" + TypeName<T> + ">";
    
    TOOLS_PTIMER(timer,tag);
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(tag, ": Not defined for links with multiple components. Aborting.");
        
        return;
    }
    
    if( !ValidQ() )
    {
        wprint(tag, ": Trying to compute long MacLeod code of invalid diagram. Returning empty vector.");
        return;
    }
    
    this-> template CheckMacLeodReturnType<T>();

    const T m = static_cast<T>(ArcCount());

    Tensor1<Int,Int> workspace_buffer( CrossingCount() );
    mptr<Int> workspace = workspace_buffer.data();
    
    this->template Traverse<true,false>(
        [workspace,code,m,this](
            const Int arc, const Int arc_pos, const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)arc;
            (void)lc;
            (void)c_0;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            if( !c_0_visitedQ )
            {
                // Remember that arc_pos visited c_0_pos.
                workspace[c_0_pos] = arc_pos;
                
                code[arc_pos] = (static_cast<T>(ArcOverQ(arc,Tail)) << T(1))
                              |  static_cast<T>(ArcRightHandedQ(arc,Tail));
            }
            else
            {
                const T a = static_cast<T>(workspace[c_0_pos]);
                const T b = static_cast<T>(arc_pos);
                   
                const T a_leap = b - a;
                const T b_leap = (m + a) - b;
                
                code[a] |= (a_leap << T(2));
                
                code[b] = (static_cast<T>(ArcOverQ(arc,Tail)) << T(1))
                        |  static_cast<T>(ArcRightHandedQ(arc,Tail))
                        | (b_leap << T(2));
            }
        }
    );
    
    // Now `code` contains the unrotated  code.
    
    // Next, we are looking for the rotation that makes the code lexicographically maximal.
        
    Size_T counter = 0;
    
    auto greaterQ = [&counter,code,m]( T s, T t )
    {
        for( T i = 0; i < m; ++ i)
        {
            ++counter;
            
            const T s_i = (s + i < m) ? (s + i) : (s + i - m);
            const T t_i = (t + i < m) ? (t + i) : (t + i - m);
            
            const T g_s_i = code[s_i];
            const T g_t_i = code[t_i];
            
            if( g_s_i != g_t_i )
            {
                return (g_s_i > g_t_i);
            }
                
            if( g_s_i > g_t_i )
            {
                return true;
            }
            else if( g_s_i < g_t_i )
            {
                return false;
            }
        }
        
        return false;
    };
    
    T s = 0;
    
    for( T t = 0; t < m; ++t )
    {
        if( greaterQ(t,s) ) { s = t; }
    }
    
    this->template SetCache<false>("MacLeodComparisonCount", counter );
    
    rotate_buffer<Side::Left>(code,s,m);
}

Size_T MacLeodComparisonCount()
{
    if( !this->InCacheQ("MacLeodComparisonCount") )
    {
#ifdef PD_ALLOCATE_SCRATCH
        WriteLongMacLeodCode( reinterpret_cast<UInt *>(A_scratch.data()) );
#else
        Tensor1<UInt,Int> dummy ( max_arc_count );
        WriteLongMacLeodCode( dummy.data() );
#endif
    }
    return this->GetCache<Size_T>("MacLeodComparisonCount");
}

private:

/*!@brief Read a long MacLeod code into the current diagram. The diagram will be reallocated only, if the previously allocated memory is too small.
 *
 * @return Integer error code: `0` on success and other values so that downstream routines can generated appropriate error message.
 */
template<bool silentQ = false, IntQ T, IntQ ExtInt, IntQ ExtInt2>
int ReadLongMacLeodCode_Private(
    cptr<T>       code,
    const ExtInt  arc_count_,
    const ExtInt2 color,
    const bool    proven_minimalQ_ = false
)
{
    // needs to know all member variables
 
    constexpr auto tag = ct_string("ReadLongMacLeodCode_Private")
        + "<" + TypeName<T>
        + "," + TypeName<ExtInt>
        + "," + TypeName<ExtInt2>
        + ">";
    
//    TOOLS_PTIMER(timer,tag);
    
    if( std::cmp_greater( arc_count_, Scalar::Max<Int> / Int{2} ) )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Number of arcs requested is too large for data type" , TypeName<Int>, ". Invalidating diagram.");
            Reset_Private();
            return 3;
        }
    }
    
    // This cast is safe now.
    const Int m = static_cast<Int>(arc_count_);
    
    if( m < Int{0} )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Number of arcs is negative. Invalidating diagram.");
            Reset_Private();
            return 2;
        }
    }
    
    Int color_ = static_cast<Int>(color);

    if( !std::cmp_equal(color_,color) )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Requested color is out of bounds for type", TypeName<Int>, ".");
        }
        Reset_Private();
        return -4;
    }
    
    if( color_ == InvalidColor )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Requested color is the invalid color.");
        }
        Reset_Private();
        return -3;
    }
    
    if( m == Int(0) )
    {
        Reset_Private();
        // Make this an unknot of this color. Potentially wastes space if buffers were already allocated.
        last_color_deactivated = color_;
        proven_minimalQ        = true;
        return 0;
    }
    
    const Int n = m / Int(2);
    
    if( m != Int{2} * n )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Number of arcs requested is odd. Invalidating diagram.");
            Reset_Private();
            return 1;
        }
    }

    // Potential reallocation, but only if too small.
    RequireCrossingCount(n);
    // TODO: There are probably better ways to make sure that the trailing crossings and arcs are correctly recorded as inactive.
    Reset_Private();
    
#ifdef PD_ALLOCATE_SCRATCH
    mptr<bool> A_visitedQ = reinterpret_cast<bool *>(A_scratch.data());
    fill_buffer(A_visitedQ,false,m);
#else
    Tensor1<bool,Int> A_visitedQ_buffer( max_arc_count, false );
    mptr<bool> A_visitedQ = A_visitedQ_buffer.data();
#endif
    
    proven_minimalQ = proven_minimalQ_;
    
    Int crossing_counter = 0;
    
    bool invalid_leapQ = false;
    
    for( Int a = 0; a < m; ++a )
    {
        if( !A_visitedQ[a] )
        {
            const T    v               = code[a];
            const T    v_leap          = v >> 2;
            const bool a_right_handedQ = get_bit(v,0);
            const bool a_overQ         = get_bit(v,1);
            
            if( v_leap < T(1) ) { invalid_leapQ = true; break; }
            
            if constexpr( std::cmp_greater(Scalar::Max<T>, Scalar::Max<Int>) )
            {
                if( v_leap > static_cast<T>(m) ) { invalid_leapQ = true; break; }
            }
            
            // This cast is safe now.
            const Int a_leap = static_cast<Int>(v_leap);
            
            if constexpr( !std::cmp_greater(Scalar::Max<T>, Scalar::Max<Int>) )
            {
                if( a_leap >= m ) { invalid_leapQ = true; break; }
            }
                
            Int b = a + a_leap;
            if( b >= m ) { b -= m; } //  Here we employ the guarantee that 0 <= leap < m.
            
            A_visitedQ[a] = true;
            A_visitedQ[b] = true;
            
            const Int c = crossing_counter;
            
            const CrossingState_T c_state = BooleanToCrossingState(a_right_handedQ);
            C_state[c] = c_state;
            
            const Int a_prev = (a > Int(0)) ? (a - Int(1)) : (m - Int(1));
            A_cross(a_prev,Head) = c;
            A_cross(a     ,Tail) = c;

            const Int b_prev = (b > Int(0)) ? (b - Int(1)) : (m - Int(1));
            A_cross(b_prev,Head) = c;
            A_cross(b     ,Tail) = c;
            
            A_color[a] = color_;
            A_color[b] = color_;
            
            A_state[a] = ArcState_T::Active;
            A_state[b] = ArcState_T::Active;

            if( a_overQ == a_right_handedQ )
            {
                /* Situation in case of a_overQ == a_right_handedQ
                 *
                 *         b       a          b       a
                 *          ^     ^            ^     ^
                 *           \   /              \   /
                 *            \ /                \ /
                 *             / <--- c  or       \ <--- c
                 *            ^ ^                ^ ^
                 *           /   \              /   \
                 *          /     \            /     \
                 *       a_prev  b_prev    a_prev   b_prev
                 */
                
                C_arcs(c,Out,Left ) = b;
                C_arcs(c,Out,Right) = a;
                C_arcs(c,In ,Left ) = a_prev;
                C_arcs(c,In ,Right) = b_prev;
            }
            else
            {
                /* Situation in case of a_overQ != a_right_handedQ
                 *
                 *         a       b          a       b
                 *          ^     ^            ^     ^
                 *           \   /              \   /
                 *            \ /                \ /
                 *             / <--- c  or       \ <--- c
                 *            ^ ^                ^ ^
                 *           /   \              /   \
                 *          /     \            /     \
                 *      b_prev   a_prev    b_prev   a_prev
                 */
                
                C_arcs(c,Out,Left ) = a;
                C_arcs(c,Out,Right) = b;
                C_arcs(c,In ,Left ) = b_prev;
                C_arcs(c,In ,Right) = a_prev;
            }
            ++crossing_counter;
        }
    }

    if( invalid_leapQ )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Input long MacLeod code is invalid because it contains a leap greater than the arc count. Invalidating diagram.");
        }
        // Explicitly invalidation might be superflous if downstream code perfers to return a PD_T::InvalidDiagram. Nonetheless, we invalidate because it is actually not that expensive and it happens only on the unhappy path (when we have more severe problems).
        Reset_Private();
        return -1;
    }
    
    crossing_count = crossing_counter;
    arc_count      = CountActiveArcs();
    
    if( !std::cmp_equal(arc_count, arc_count_) )
    {
        if constexpr( !silentQ )
        {
            Msgr::eprint(tag, "Input long MacLeod code is invalid because arc_count != 2 * crossing_count. Invalidating diagram.");
        }
        // Explicitly invalidation might be superflous if downstream code perfers to return a PD_T::InvalidDiagram. Nonetheless, we invalidate because it is actually not that expensive and it happens only on the unhappy path (when we have more severe problems).
        Reset_Private();
        return -2;
    }
    
    return 0;
    
} // ReadLongMacLeodCode_Private

public:

/*!@brief **UNSAFE.** Read a long MacLeod code into the current diagram. The diagram will be reallocated only, if the previously allocated memory is too small.
 *
 * @return Integer error code: `0` on success and other values so that downstream routines can generated appropriate error message.
 */
template<bool silentQ = false, IntQ T, IntQ ExtInt, IntQ ExtInt2>
int ReadLongMacLeodCode(
    cptr<T>       code,
    const ExtInt  arc_count_,
    const ExtInt2 color,
    const bool    proven_minimalQ_ = false
)
{
    constexpr auto tag = ct_string("ReadLongMacLeodCode")
    + "<" + TypeName<T>
    + "," + TypeName<ExtInt>
    + "," + TypeName<ExtInt2>
    + ">";
    
    if( LockedQ() ) { LockMessage(tag); return 5; }
    
    return ReadLongMacLeodCode_Private(code, arc_count_, color, proven_minimalQ_ );
}

template<IntQ T, IntQ ExtInt, IntQ ExtInt2>
static PD_T FromLongMacLeodCode(
    cptr<T>       code,
    const ExtInt  arc_count_,
    const ExtInt2 color,
    const bool    proven_minimalQ_ = false
)
{
    constexpr auto tag = ct_string("FromLongMacLeodCode")
        + "<" + TypeName<T>
        + "," + TypeName<ExtInt>
        + "," + TypeName<ExtInt2>
        + ">";
    
//    TOOLS_PTIMER(timer,MethodName(tag));
    
    PD_T pd;
    
    const int info = pd.ReadLongMacLeodCode_Private(code, arc_count_, color, proven_minimalQ_);
    
    if( info )
    {
        Msgr::eprint(tag, "Failed due to an error. Returning invalid diagram.");
        return PD_T::InvalidDiagram();
    }
    
    return pd;
}

template<IntQ T, IntQ ExtInt, IntQ ExtInt2>
static PD_T FromLongMacLeodCode(
    cref<Tensor1<T,ExtInt>> code, const ExtInt2 color, const bool proven_minimalQ_ = false
)
{
    return FromLongMacLeodCode(code.data(), code.Size(), color, proven_minimalQ_);
}
