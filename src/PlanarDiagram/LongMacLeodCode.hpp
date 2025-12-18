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
    TOOLS_PTIMER(timer,ClassName()+"::WriteLongMacLeodCode<"+TypeName<T>+">");
    
    static_assert(IntQ<T>,"");
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::WriteLongMacLeodCode<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
        
        return;
    }
    
    if( !ValidQ() )
    {
        wprint(ClassName()+"::WriteLongMacLeodCode<"+TypeName<T>+">: Trying to compute long MacLeod code of invalid PlanarDiagram. Returning empty vector.");
        return;
    }
    
    this-> template CheckMacLeodReturnType<T>();
    
    Tensor1<Int,Int> workspace ( arc_count );

    this->template Traverse<true,false,0>(
        [&workspace,code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)a;
            (void)lc;
            (void)c_0;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            workspace[Int(2) * c_0_pos + c_0_visitedQ] = a_pos;
            
            code[a_pos] = (
                static_cast<T>(this->ArcOverQ<Tail>(a)) << T(1))
                | static_cast<T>(this->CrossingRightHandedQ(c_0)
            );
        }
    );
    
    const Int n = CrossingCount();
    const T   m = static_cast<T>(ArcCount());
    
    for( Int i = 0; i < n; ++i )
    {
        const T a = static_cast<T>(workspace[Int(2) * i + Tail]);
        const T b = static_cast<T>(workspace[Int(2) * i + Head]);

        const T a_leap = b - a;
        const T b_leap = (m + a) - b;
        
        code[a] |= (a_leap << T(2));
        code[b] |= (b_leap << T(2));
    }
    
    // Now `buffer` contains the unrotated  code.
    
    // Now we are looking for the rotation that makes the code lexicographically maximal.
        
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


// This was used only for debugging.
//template<typename S, typename ExtInt>
//static Tensor1<ToSigned<Int>,Int> LongMacLeodCode_to_ExtendedGaussCode(
//    cptr<S> code,
//    const ExtInt arc_count_
//)
//{
//    using T = ToSigned<Int>;
//    
//    const Int m = int_cast<Int>(arc_count_);
//
//    Tensor1<T,Int> gauss (m);
//
//    Tensor1<bool,Int> visitedQ (m,false);
//    
//    // We need 1-based integers for extended Gauss code.
//    T counter = 1;
//    
//    for( Int i = 0; i < m; ++i )
//    {
//        if( !visitedQ[i] )
//        {
//            const T    v               = code[i];
//            const Int  i_leap          = static_cast<Int>(v >> T(2));
//            const bool i_overQ         = get_bit(v,T(1));
//            const bool i_right_handedQ = get_bit(v,T(0));
////            const bool i_overQ         = (v & T(2)) == T(2);
////            const bool i_right_handedQ = (v & T(1)) == T(1);
//            
//            Int  j = i + i_leap;
//            if( j >= m ) { j-=m; };
//            
//            if( !InIntervalQ(i,Int(0),m) )
//            {
//                eprint("!InIntervalQ(i,Int(0),m)");
//            }
//            
//            const T    w               = code[j];
//            const Int  j_leap          = static_cast<Int>(w >> T(2));
//            const bool j_overQ         = get_bit(w,T(1));
//            const bool j_right_handedQ = get_bit(w,T(0));
////            const bool j_overQ         = (w & T(2)) == T(2);
////            const bool j_right_handedQ = (w & T(1)) == T(1);
//            
//            if( i_overQ == j_overQ )
//            {
//                eprint("i_overQ == j_overQ");
//            }
//            
//            if( i_right_handedQ != j_right_handedQ )
//            {
//                eprint("i_right_handedQ != j_right_handedQ");
//            }
//            
//            if( i_leap + j_leap != m )
//            {
//                eprint("i_leap + j_leap != m");
//            }
//            
//            if( i >= j )
//            {
//                eprint("i >= j");
//            }
//            
//            visitedQ[i] = true;
//            visitedQ[j] = true;
//            
//            if( i < j )
//            {
//                gauss[i] = (i_overQ         ? counter : -counter );
//                gauss[j] = (j_right_handedQ ? counter : -counter );
//            }
//            else
//            {
//                gauss[j] = (j_overQ         ? counter : -counter );
//                gauss[i] = (i_right_handedQ ? counter : -counter );
//            }
//            
//            ++counter;
//        }
//    }
//    
//    return gauss;
//}

template<typename T, typename ExtInt2, typename ExtInt3>
static PlanarDiagram FromLongMacLeodCode(
    cptr<T>       code,
    const ExtInt2 arc_count_,
    const ExtInt3 unlink_count_,
    const bool    compressQ = false,
    const bool    proven_minimalQ_ = false
)
{
    TOOLS_PTIMER(timer,MethodName("FromLongMacLeodCode")
        + "<" + TypeName<T>
        + "," + TypeName<ExtInt2>
        + "," + TypeName<ExtInt3>
        + ">");
    
    static_assert(IntQ<T>,"");
    
    // TODO: We should check whether 2 * arc_count_ fits into Int.

    if( arc_count_ <= ExtInt2(0) )
    {
        PlanarDiagram pd( Int(0), int_cast<Int>(unlink_count_) );
        pd.proven_minimalQ = true;
        return pd;
    }
    
    const Int m = int_cast<Int>(arc_count_);
    const Int n = m / Int(2);

    PlanarDiagram pd ( n, int_cast<Int>(unlink_count_) );
    
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
            
//            TOOLS_LOGDUMP(v);
//            TOOLS_LOGDUMP(a_leap);
//            TOOLS_LOGDUMP(a_right_handedQ);
//            TOOLS_LOGDUMP(a_overQ);
//            TOOLS_LOGDUMP(b);
            
            A_visitedQ[a] = true;
            A_visitedQ[b] = true;
            
            const Int c = crossing_counter;
            
            pd.C_state[c] = a_right_handedQ
                          ? CrossingState::RightHanded
                          : CrossingState::LeftHanded;
            
            const Int a_prev = (a > Int(0)) ? (a - Int(1)) : (m - Int(1));
            pd.A_cross(a_prev,Head) = c;
            pd.A_cross(a     ,Tail) = c;

            const Int b_prev = (b > Int(0)) ? (b - Int(1)) : (m - Int(1));
            pd.A_cross(b_prev,Head) = c;
            pd.A_cross(b     ,Tail) = c;
            
            pd.A_state[a] = ArcState::Active;
            pd.A_state[b] = ArcState::Active;
            
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
    
    // Compression is not really meaningful because the traversal ordering is crucial for the MacLeod code.
    if(compressQ)
    {
        // We finally call `CreateCompressed` to get the ordering of crossings and arcs consistent.
        return pd.CreateCompressed();
    }
    else
    {
        return pd;
    }
}

template<typename T, typename ExtInt>
static PlanarDiagram FromLongMacLeodCode( cref<Tensor1<T,ExtInt>> code )
{
    static_assert(IntQ<T>,"");
    static_assert(IntQ<ExtInt>,"");
    return FromLongMacLeodCode( code.data(), code.Size(), Int(0), false, false );
}
