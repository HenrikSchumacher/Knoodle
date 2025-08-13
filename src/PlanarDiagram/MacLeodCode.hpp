public:

template<typename T = ToUnsigned<Int>>
Tensor1<T,Int> MacLeodCode()  const
{
    TOOLS_PTIMER(timer,ClassName()+"::MacLeodCode<"+TypeName<T>+">");
    
    static_assert( IntQ<T>, "" );
    
    Tensor1<T,Int> code;
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::MacLeodCode<"+TypeName<T>+">: This diagram has several link components. And, you know, there can be only one.>");
        return code;
    }
    
    if( !ValidQ() )
    {
        wprint(ClassName()+"::MacLeodCode<"+TypeName<T>+">: Trying to compute extended  code of invalid PlanarDiagram. Returning empty vector.");
        return code;
    }
    
    if( std::cmp_greater( Size_T(crossing_count) * Size_T(4) + Size_T(3) , std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::MacLeodCode<"+TypeName<T>+">: Requested type " + TypeName<T> + " cannot store extended  code for this diagram.");
    }
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::MacLeodCode<"+TypeName<T>+">: Not defined for links with multiple components.");
        
        return Tensor1<T,Int>();
    }
    
    code = Tensor1<T,Int>( arc_count );
    
    this->WriteMacLeodCode<T>(code.data());
    
    return code;
}


template<typename T>
void WriteMacLeodCode( mptr<T> code )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::WriteMacLeodCode<"+TypeName<T>+">");
    
    static_assert( IntQ<T>, "" );
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::WriteMacLeodCode<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
    }
    
    Tensor1<T,Int> buffer ( ArcCount(), static_cast<T>(Uninitialized) );

    this->template Traverse<true,false,0>(
        [&buffer,code,this](
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
            
            buffer[Int(2) * c_0_pos + c_0_visitedQ] = static_cast<T>(a_pos);
            
            code[a_pos] = (static_cast<T>(this->ArcOverQ<Tail>(a)) << T(1)) | static_cast<T>(this->CrossingRightHandedQ(c_0));
        }
    );
    
    const Int n = CrossingCount();
    const T   m = static_cast<T>(ArcCount());
    
    for( Int i = 0; i < n; ++i )
    {
        const T a = buffer[Int(2) * i + Tail];
        const T b = buffer[Int(2) * i + Head];

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
        if( greaterQ( t, s ) ) { s = t; }
    }
    
    this->template SetCache<false>("MacLeodComparisonCount", counter );
    
    rotate_buffer<Side::Left>( code, s, m );
}

Size_T MacLeodComparisonCount()
{
    if( !this->InCacheQ("MacLeodComparisonCount") )
    {
        MacLeodCode();
    }
    return this->GetCache<Size_T>("MacLeodComparisonCount");
}


template<typename S, typename ExtInt>
static Tensor1<ToSigned<Int>,Int> MacLeodCodeToExtendedGaussCode(
    cptr<S> code,
    const ExtInt arc_count_
)
{
    using T = ToSigned<Int>;
    
    const Int m = int_cast<Int>(arc_count_);

    Tensor1<T,Int> gauss ( m );

    Tensor1<bool,Int> visitedQ (m,false);
    
    // We need 1-based integers for extended Gauss code.
    T counter = 1;
    
    for( Int i = 0; i < m; ++i )
    {
        if( !visitedQ[i] )
        {
            const T    v               = code[i];
            const Int  i_leap          = static_cast<Int>(v >> T(2));
            const bool i_overQ         = get_bit(v,T(1));
            const bool i_right_handedQ = get_bit(v,T(0));
//            const bool i_overQ         = (v & T(2)) == T(2);
//            const bool i_right_handedQ = (v & T(1)) == T(1);
            
            Int  j = i + i_leap;
            if( j >= m ) { j-=m; };
            
            if( !InIntervalQ(i,Int(0),m) )
            {
                eprint("!InIntervalQ(i,Int(0),m)");
            }
            
            const T    w               = code[j];
            const Int  j_leap          = static_cast<Int>(w >> T(2));
            const bool j_overQ         = get_bit(w,T(1));
            const bool j_right_handedQ = get_bit(w,T(0));
//            const bool j_overQ         = (w & T(2)) == T(2);
//            const bool j_right_handedQ = (w & T(1)) == T(1);
            
            if( i_overQ == j_overQ )
            {
                eprint("i_overQ == j_overQ");
            }
            
            if( i_right_handedQ != j_right_handedQ )
            {
                eprint("i_right_handedQ != j_right_handedQ");
            }
            
            if( i_leap + j_leap != m )
            {
                eprint("i_leap + j_leap != m");
            }
            
            if( i >= j )
            {
                eprint("i >= j");
            }
            
            visitedQ[i] = true;
            visitedQ[j] = true;
            
            TOOLS_LOGDUMP(i);
            TOOLS_LOGDUMP(j);
            TOOLS_LOGDUMP(counter);
            if( i < j )
            {
                logprint("A");
                gauss[i] = (i_overQ         ? counter : -counter );
                gauss[j] = (j_right_handedQ ? counter : -counter );
            }
            else
            {
                logprint("B");
                gauss[j] = (j_overQ         ? counter : -counter );
                gauss[i] = (i_right_handedQ ? counter : -counter );
            }
            
            ++counter;
        }
    }
    
    return gauss;
}

template<typename T, typename ExtInt2, typename ExtInt3>
static PlanarDiagram FromMacLeodCode(
    cptr<T>       code,
    const ExtInt2 arc_count_,
    const ExtInt3 unlink_count_,
    const bool    compressQ = false,
    const bool    proven_minimalQ_ = false
)
{
    static_assert( IntQ<T>, "" );

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
        eprint(ClassName() + "FromPDCode: Input PD code is invalid because arc_count != 2 * crossing_count. Returning invalid PlanarDiagram.");
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
static PlanarDiagram FromMacLeodCode( cref<Tensor1<T,ExtInt>> code )
{
    static_assert(UnsignedIntQ<T>,"");
    static_assert(IntQ<ExtInt>,"");
    return FromMacLeodCode( code.data(), code.Size(), Int(0), false, false );
}






template<typename Int>
static Size_T MacLeod_DigitCountFromStringLength( Int string_length )
{
    static_assert(IntQ<Int>);
    
    Size_T n   = ToSize_T(string_length);
    Size_T d   = 1;
    Size_T pow = 8;
    
    while( n > pow )
    {
        ++d;
        pow = (pow << Binarizer::digit_bit_count);
    }
    
    return d;
}

std::string MacLeodString() const
{
    TOOLS_PTIMER(timer,MethodName("MacLeodString"));
    
    auto code = this->template MacLeodCode<UInt>();
    
    const UInt   m = int_cast<UInt>(ArcCount());
    const Size_T d = Binarizer::DigitCountFromMaxNumber(UInt(4) * m);
    
    auto s = Binarizer::ToString( code.data(), m, d );
    
    return s;
}

static PlanarDiagram FromMacLeodString( cref<std::string> s )
{
    TOOLS_PTIMER(timer,MethodName("FromMacLeodString"));
    
    Size_T L = ToSize_T(s.size());
    Size_T d = 1;   // current digit count that we try
    Size_T m = 16;  // the maximal arc count possible with current d.
    
    while( L > d * m )
    {
        ++d;
        m = (m << Binarizer::digit_bit_count);
    }

    auto code = Binarizer::template FromString<UInt>(s,d);
    
    return FromMacLeodCode(code);
}

//Tensor1<Binarizer::Char,Int> BinaryMacLeodCode() const
//{
//    TOOLS_PTIMER(timer,MethodName("BinaryMacLeodCode"));
//    
//    auto code = this->template MacLeodCode<UInt>();
//    
//    const auto digit_count = Binarizer::DigitCountFromMaxNumber(code.Size() * 4);
//    
//    return Binarizer::ToCharSequence(
//        code.data(), code.Size(), digit_count
//    );
//}
