public:


template<typename T>
void CheckMacLeodReturnType() const
{
    if( std::cmp_greater( Size_T(crossing_count) * Size_T(4) + Size_T(3) , std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::CheckMacLeodReturnType<"+TypeName<T>+">: Requested type " + TypeName<T> + " cannot store MacLeod code for this diagram.");
    }
}

template<typename S, typename T = UInt>
static void LongMacLeodCode_to_MacLeodCode(
    cptr<S> l_mac_leod, mptr<T> s_mac_leod, Int c_count
)
{
    TOOLS_PTIMER(timer,MethodName("LongMacLeodCode_to_MacLeodCode")
        + "<" + TypeName<S>
        + "," + TypeName<T>
        + ">");
    
    static_assert(IntQ<S>,"");
    static_assert(IntQ<T>,"");
    
    
    const T n = static_cast<T>(c_count);
    const T m = T(2) * n;
    
    T j = 0; // where to write in s_mac_leod
    
    for( T a = 0; a < m; ++a )
    {
        T code = static_cast<S>(l_mac_leod[a]); // TODO: Use int_cast?
        T leap = (code >> 2);
        if( a + leap < m )
        {
            s_mac_leod[j] = code;
            ++j;
        }
    }
}

template<typename S, typename T = UInt>
static void MacLeodCode_to_LongMacLeodCode(
    cptr<S> s_mac_leod, mptr<T> l_mac_leod, Int c_count
)
{
    TOOLS_PTIMER(timer,MethodName("MacLeodCode_to_LongMacLeodCode")
        + "<" + TypeName<S>
        + "," + TypeName<T>
        + ">");
    
    static_assert(IntQ<S>,"");
    static_assert(IntQ<T>,"");
    
    const T n = static_cast<T>(c_count);
    const T m = T(2) * n;
    
    constexpr T undefined = 0;
    
    // 0 cannot occur in a MacLeod code, so we can use it to signal "undefined".
    fill_buffer( l_mac_leod, undefined, m );
    
    T a = 0; // where to write in mac_leod
    
    for( T j = 0; j < n; ++j )
    {
        // Move to next empty position.
        while( l_mac_leod[a] != undefined ) { ++a; }
            
        const T    a_code   = static_cast<T>(s_mac_leod[j]); // TODO: Use int_cast?
        const T    a_leap   = (a_code >> 2);
        const bool a_overQ  = (a_code & T(2));
        const bool a_rightQ = (a_code & T(1));

        const T    b        = a + a_leap;
        const T    b_leap   = m - a_leap;
        const bool b_overQ  = !a_overQ;
        const bool b_rightQ = a_rightQ;
        const T    b_code   = (b_leap << 2) | (T(b_overQ) << 2) | T(b_rightQ);
        
        l_mac_leod[a] = a_code;
        l_mac_leod[b] = b_code;
        ++a;
    }
}



template<typename T = UInt>
void WriteMacLeodCode( mptr<T> s_mac_leod ) const
{
    TOOLS_PTIMER(timer,ClassName()+"::WriteMacLeodCode<"+TypeName<T>+">");
    
    static_assert(IntQ<T>,"");
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::WriteMacLeodCode<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
        return;
    }

    if( !ValidQ() )
    {
        wprint(ClassName()+"::WriteMacLeodCode<"+TypeName<T>+">: Trying to compute MacLeod code of invalid planar diagram. Returning empty vector.");
        return;
    }

    this-> template CheckMacLeodReturnType<T>();
    
    auto l_mac_leod = LongMacLeodCode();
    
    LongMacLeodCode_to_MacLeodCode( l_mac_leod.data(), s_mac_leod, crossing_count );
}

template<typename T = UInt>
Tensor1<T,Int> MacLeodCode() const
{
    TOOLS_PTIMER(timer,MethodName("MacLeodCode")+"<"+TypeName<T>+">");
    
    static_assert(IntQ<T>,"");
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(MethodName("MacLeodCode")+"<"+TypeName<T>+">: This diagram has several link components. And, you know, there can be only one.>");
        return Tensor1<T,Int>();
    }
    
    if( !ValidQ() )
    {
        wprint(MethodName("MacLeodCode")+"<"+TypeName<T>+">: Trying to compute MacLeod code of invalid planar diagram. Returning empty vector.");
        return Tensor1<T,Int>();
    }
    
    Tensor1<T,Int> s_mac_leod ( crossing_count );
    WriteMacLeodCode( s_mac_leod.data() );
    
    return s_mac_leod;
}

template<typename T, typename ExtInt>
static PD_T FromMacLeodCode(
    cptr<T>       s_mac_leod,
    const ExtInt  crossing_count_,
    const bool    proven_minimalQ_ = false,
    const bool    compressQ = false
)
{
    TOOLS_PTIMER(timer,MethodName("FromMacLeodCode")
        + "<" + TypeName<T>
        + "," + TypeName<ExtInt>
        + ">");
    
    static_assert(IntQ<T>,"");
    static_assert(IntQ<ExtInt>,"");
    
    Int c_count = int_cast<Int>(crossing_count_);
    Int a_count = Int(2) * c_count;
    
    Tensor1<T,Int> l_mac_leod ( a_count );
    
    // TODO: Can we out this step and reimplement FromMacLeodCode directly?
    MacLeodCode_to_LongMacLeodCode( s_mac_leod, l_mac_leod.data(), c_count );

    return FromLongMacLeodCode( l_mac_leod.data(), a_count, proven_minimalQ_, compressQ );
}


template<typename T, typename ExtInt>
static PD_T FromMacLeodCode( cref<Tensor1<T,ExtInt>> s_mac_leod )
{
    static_assert(IntQ<T>,"");
    static_assert(IntQ<ExtInt>,"");
    return FromMacLeodCode( s_mac_leod.data(), s_mac_leod.Size(), false, false );
}







//template<typename Int>
//static Size_T MacLeod_DigitCountFromStringLength( Int string_length )
//{
//    static_assert(IntQ<Int>);
//
//    Size_T L   = ToSize_T(2) * ToSize_T(string_length);
//    Size_T d   = 1;
//    Size_T pow = 8;
//
//    while( L > pow )
//    {
//        ++d;
//        pow = (pow << Binarizer::digit_bit_count);
//    }
//
//    return d;
//}


std::string MacLeodString() const
{
    TOOLS_PTIMER(timer,MethodName("MacLeodString"));
    
    auto code = this->template MacLeodCode<UInt>();
    
    const UInt   n = int_cast<UInt>(CrossingCount());
    const Size_T d = Binarizer::DigitCountFromMaxNumber(UInt(8) * n);
    
    auto s = Binarizer::ToString( code.data(), n, d );
    
    return s;
}

static PD_T FromMacLeodString( cref<std::string> s )
{
    TOOLS_PTIMER(timer,MethodName("FromMacLeodString"));
    
    Size_T L = ToSize_T(s.size());
    Size_T d = 1;   // current digit count that we try
    
    // The maximal crossing count possible with current d.
    // We lose three bits:
    // One for over/under; one for handedness, one because max leap can be 2 * n.
    Size_T n = Size_T(1) << (Binarizer::digit_bit_count - Size_T(3));

    while( L > d * n )
    {
        ++d;
        n = (n << Binarizer::digit_bit_count);
    }

    auto code = Binarizer::template FromString<UInt>(s,d);
    
    return FromMacLeodCode(code);
}
