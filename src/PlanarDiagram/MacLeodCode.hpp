public:

template<typename T = std::make_unsigned_t<Int>, int method = DefaultTraversalMethod>
Tensor1<T,Int> MacLeodCode()  const
{
    TOOLS_PTIMER(timer,ClassName()+"::MacLeodCode<"+TypeName<T>+","+ToString(method)+">");
    
    static_assert( IntQ<T>, "" );
    
    Tensor1<T,Int> code;
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::MacLeodCode<"+TypeName<T>+","+ToString(method)+": This diagram has several link components. And, you know, there can be only one.>");
        return code;
    }
    
    if( !ValidQ() )
    {
        wprint( ClassName()+"::MacLeodCode: Trying to compute extended  code of invalid PlanarDiagram. Returning empty vector.");
        return code;
    }
    
    if( std::cmp_greater( Size_T(crossing_count) * Size_T(4) + Size_T(3) , std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::MacLeodCode: Requested type " + TypeName<T> + " cannot store extended  code for this diagram.");
    }
    
    code = Tensor1<T,Int>( arc_count );
    
    this->WriteMacLeodCode<T,method>(code.data());
    
    return code;
}


template<typename T, int method = DefaultTraversalMethod>
void WriteMacLeodCode( mptr<T> code )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::WriteMacLeodCode<"+TypeName<T>+","+ToString(method)+">");
    
    static_assert( IntQ<T>, "" );
    
    Tensor1<Int,Int> buffer ( ArcCount() );

    this->template Traverse<true,false,0,method>(
        [&buffer](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)a;
            (void)lc;
            (void)c_0;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            buffer[T(2) * c_0_pos + c_0_visitedQ] = a_pos;
        }
    );
    
    const Int n = CrossingCount();
    const T   m = static_cast<T>(ArcCount());
    
    for( Int c = 0; c < n; ++c )
    {
        if( !CrossingActiveQ(c) ) { continue; }
        
        const T a = buffer[Int(2) * c + Tail];
        const T b = buffer[Int(2) * c + Head];
        
        const T a_leap = b - a;
        const T b_leap = (m + a) - b;
        
        const T right_handed_Q = T(CrossingRightHandedQ(c));
        const T a_overQ = ArcOverQ<Tail>(a);
        const T b_overQ = !a_overQ;
        
        code[a] = (a_leap << 2) | (a_overQ << 1) | right_handed_Q;
        code[b] = (b_leap << 2) | (b_overQ << 1) | right_handed_Q;
    }
    
    // Now `buffer` contains the unrotated  code.
    
    // Now e are looking for the rotation that makes the code lexicographical maximal.
        
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
        if( greaterQ( t, s ) )
        {
            s = t;
        }
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


template<typename T, typename ExtInt2, typename ExtInt3>
static PlanarDiagram<Int> FromMacLeodCode(
    cptr<T>       code,
    const ExtInt2 arc_count_,
    const ExtInt3 unlink_count_,
    const bool    compressQ = false,
    const bool    proven_minimalQ_ = false
)
{
    static_assert( IntQ<T>, "" );
    
    const Int n = int_cast<Int>(arc_count_/2);
    const Int m = int_cast<Int>(arc_count_);
    
    PlanarDiagram<Int> pd (n,int_cast<Int>(unlink_count_) );
    
    if( arc_count_ <= Int(0) )
    {
        pd.proven_minimalQ = true;
        return pd;
    }
    
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
            const bool a_right_handedQ = (v & T(1)) == T(1);
            const bool a_overQ         = (v & T(2)) == T(2);
            
            const Int  b               = a + a_leap;
            
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
                /* Situation in case of CrossingRightHandedQ(c) = overQ
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
                /* Situation in case of CrossingRightHandedQ(c) != overQ
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
    pd.arc_count      = int_cast<Int>(arc_count_);
    
    if( pd.arc_count != Int(2) * pd.crossing_count )
    {
        eprint(ClassName() + "FromPDCode: Input PD code is invalid because arc_count != 2 * crossing_count. Returning invalid PlanarDiagram.");
    }
    
    // TODO: Is this really meaningful?
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
