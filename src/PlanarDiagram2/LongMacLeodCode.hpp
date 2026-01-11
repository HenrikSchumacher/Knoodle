public:

template<typename T = UInt>
Tensor1<T,Int> LongMacLeodCode() const
{
    TOOLS_PTIMER(timer,ClassName()+"::LongMacLeodCode<"+TypeName<T>+">");
    
    static_assert(IntQ<T>,"");
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::LongMacLeodCode<"+TypeName<T>+">: This diagram has several link components. And, you know, there can be only one.>");
        return Tensor1<T,Int>();
    }
    
    if( !ValidQ() )
    {
        wprint(ClassName()+"::LongMacLeodCode<"+TypeName<T>+">: Trying to compute long MacLeod code of invalid PlanarDiagram. Returning empty vector.");
        return Tensor1<T,Int>();
    }
    
    this-> template CheckMacLeodReturnType<T>();
    
    Tensor1<T,Int> code( arc_count );
    
    this->WriteLongMacLeodCode<T>(code.data());
    
    return code;
}

template<typename T>
void WriteLongMacLeodCode( mptr<T> code ) const
{
    TOOLS_PTIMER(timer,MethodName("WriteLongMacLeodCode")+"<"+TypeName<T>+">");
    
    static_assert(IntQ<T>,"");
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(MethodName("WriteLongMacLeodCode")+"<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
        
        return;
    }
    
    if( !ValidQ() )
    {
        wprint(MethodName("WriteLongMacLeodCode")+"<"+TypeName<T>+">: Trying to compute long MacLeod code of invalid PlanarDiagram. Returning empty vector.");
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
        WriteLongMacLeodCode( reinterpret_cast<UInt *>(A_scratch.data()) );
    }
    return this->GetCache<Size_T>("MacLeodComparisonCount");
}

template<typename T, typename ExtInt>
static PD_T FromLongMacLeodCode(
    cptr<T>       code,
    const ExtInt  arc_count_,
    const bool    compressQ = false,
    const bool    proven_minimalQ_ = false
)
{
    TOOLS_PTIMER(timer,MethodName("FromLongMacLeodCode")
        + "<" + TypeName<T>
        + "," + TypeName<ExtInt>
        + ">");
    
    static_assert(IntQ<T>,"");
    
    // TODO: We should check whether 2 * arc_count_ fits into Int.

    if( arc_count_ <= ExtInt(0) )
    {
        PD_T pd( Int(0) );
        pd.proven_minimalQ = true;
        return pd;
    }
    
    const Int m = int_cast<Int>(arc_count_);
    const Int n = m / Int(2);

    PD_T pd ( n );
    
    mptr<bool> A_visitedQ = reinterpret_cast<bool *>(pd.A_scratch.data());
    fill_buffer(A_visitedQ,false,m);
    
    pd.proven_minimalQ = proven_minimalQ_;
    
    Int crossing_counter = 0;
    
    for( Int a = 0; a < m; ++a )
    {
        if( !A_visitedQ[a] )
        {
            const T    v               = code[a];
            const Int  a_leap          = static_cast<Int>(v >> 2);
            const bool a_right_handedQ = get_bit(v,T(0));
            const bool a_overQ         = get_bit(v,T(1));
            
            Int b = a + a_leap;
            if( b >= m ) { b -= m; }
            
            A_visitedQ[a] = true;
            A_visitedQ[b] = true;
            
            const Int c = crossing_counter;
            
            const CrossingState_T c_state = a_right_handedQ
                                          ? CrossingState_T::RightHanded
                                          : CrossingState_T::LeftHanded;
            pd.C_state[c] = c_state;
            
            const Int a_prev = (a > Int(0)) ? (a - Int(1)) : (m - Int(1));
            pd.A_cross(a_prev,Head) = c;
            pd.A_cross(a     ,Tail) = c;

            const Int b_prev = (b > Int(0)) ? (b - Int(1)) : (m - Int(1));
            pd.A_cross(b_prev,Head) = c;
            pd.A_cross(b     ,Tail) = c;
            
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
                
                pd.C_arcs(c,Out,Left ) = b;
                pd.C_arcs(c,Out,Right) = a;
                pd.C_arcs(c,In ,Left ) = a_prev;
                pd.C_arcs(c,In ,Right) = b_prev;

                pd.A_state[a_prev].Set(Head,Left ,c_state);
                pd.A_state[a     ].Set(Tail,Right,c_state);
                pd.A_state[b_prev].Set(Head,Right,c_state);
                pd.A_state[b     ].Set(Tail,Left ,c_state);
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
                
                pd.C_arcs(c,Out,Left ) = a;
                pd.C_arcs(c,Out,Right) = b;
                pd.C_arcs(c,In ,Left ) = b_prev;
                pd.C_arcs(c,In ,Right) = a_prev;

                pd.A_state[a_prev].Set(Head,Right,c_state);
                pd.A_state[a     ].Set(Tail,Left ,c_state);
                pd.A_state[b_prev].Set(Head,Left ,c_state);
                pd.A_state[b     ].Set(Tail,Right,c_state);
            }
        
            ++crossing_counter;
        }
    }

    pd.crossing_count = crossing_counter;
    pd.arc_count      = pd.CountActiveArcs();
    
    if( !std::cmp_equal(pd.arc_count, arc_count_) )
    {
        eprint(MethodName("FromLongMacLeodCode") + ": Input long MacLeod code code is invalid because arc_count != 2 * crossing_count. Returning invalid PlanarDiagram.");
    }
    
    // TODO: Computing the arc colors is actually redundant and diagrams from Gauss code can only be knots.
    
    // Compression is not really meaningful because the traversal ordering is unique for the MacLeod code.
    if(compressQ)
    {
        // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
        return pd.template CreateCompressed<true>();
    }
    else
    {
        pd.ComputeArcColors();
        return pd;
    }
}

template<typename T, typename ExtInt>
static PD_T FromLongMacLeodCode( cref<Tensor1<T,ExtInt>> code )
{
    static_assert(IntQ<T>,"");
    static_assert(IntQ<ExtInt>,"");
    return FromLongMacLeodCode( code.data(), code.Size(), false, false );
}
