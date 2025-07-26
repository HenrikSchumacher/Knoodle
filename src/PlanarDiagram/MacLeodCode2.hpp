public:

template<typename T = ToUnsigned<Int>>
Tensor1<T,Int> MacLeodCode2( const Int a_0 )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::MacLeodCode2<"+TypeName<T>+">");
    
    static_assert( IntQ<T>, "" );
    
    Tensor1<T,Int> code;
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::MacLeodCode2<"+TypeName<T>+">: This diagram has several link components. And, you know, there can be only one.>");
        return code;
    }
    
    if( !ValidQ() )
    {
        wprint(ClassName()+"::MacLeodCode2<"+TypeName<T>+">: Trying to compute extended  code of invalid PlanarDiagram. Returning empty vector.");
        return code;
    }
    
    if( std::cmp_greater( Size_T(crossing_count) * Size_T(4) + Size_T(3) , std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error(ClassName()+"::MacLeodCode2<"+TypeName<T>+">: Requested type " + TypeName<T> + " cannot store extended  code for this diagram.");
    }
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::MacLeodCode2<"+TypeName<T>+">: Not defined for links with multiple components.");
        
        return Tensor1<T,Int>();
    }
    
    code = Tensor1<T,Int>( arc_count );
    
    this->WriteMacLeodCode2<T>(a_0,code.data());
    
    return code;
}


template<typename T>
void WriteMacLeodCode2( const Int a_0, mptr<T> code )  const
{
    TOOLS_PTIMER(timer,ClassName()+"::WriteMacLeodCode2<"+TypeName<T>+">");
    
    static_assert( IntQ<T>, "" );
    
    if( LinkComponentCount() > Int(1) )
    {
        eprint(ClassName()+"::WriteMacLeodCode2<"+TypeName<T>+">: Not defined for links with multiple components. Aborting.");
    }
    
    Tensor1<Int,Int> buffer ( ArcCount(), Uninitialized );

    C_scratch.Fill(Uninitialized);
    
    mptr<Int>  c_i = C_scratch.data();
//    mptr<bool> A_visitedQ = reinterpret_cast<bool *>(A_scratch.data());
//    fill_buffer(A_visitedQ,false,A_cross.Dim(0));
    
    Int i_counter = 0;
    Int a_counter = 0;
    
    TraverseComponent(
        a_0,
        [code,&buffer,&i_counter,&a_counter,c_i,this]
        ( const Int a )
        {
            const Int c = this->A_cross(a,Tail);
            
            Int i = c_i[c];
            
            bool visitedQ = ( i != Uninitialized );
            
            if( !visitedQ )
            {
                i = c_i[c] = i_counter;
                ++i_counter;
            }

            code[a_counter] = (this->ArcOverQ<Tail>(a) << T(1)) | this->CrossingRightHandedQ(c);
            
            buffer[Int(2) * i + visitedQ] = a_counter;
            ++a_counter;
        }
    );
    
    
    TOOLS_LOGDUMP(buffer);
    
//    const Int n = CrossingCount();
    const T   m = static_cast<T>(ArcCount());
    
//    //DEBUGGING;
//    Tensor1<Int,Int> gauss ( ArcCount() );
    
    for( Int i = 0; i < i_counter; ++i )
    {
        const T a = buffer[Int(2) * i + Tail];
        const T b = buffer[Int(2) * i + Head];
        
        // DEBUGGING
        if( !InIntervalQ(a,T(0),m) ) { eprint("!InIntervalQ(a,T(0),m)"); };
        if( !InIntervalQ(b,T(0),m) ) { eprint("!InIntervalQ(b,T(0),m)"); };
        
        if( b < a ) { eprint("b < a"); };
        
        const T a_leap =  b - a;
        const T b_leap = (m + a) - b;
        
        if( a + a_leap != b     ) { eprint("a + a_leap != b    "); }
        if( b + b_leap != a + m ) { eprint("b + b_leap != a + m"); }
        
        code[a] |= (a_leap << T(2));
        code[b] |= (b_leap << T(2));
    }
    
//    //DEBUGGING;
//    {
//        auto extended_gauss = ExtendedGaussCode();
//        bool failedQ = false;
//        for( T i = 0; i < m; ++i )
//        {
//            if( std::cmp_not_equal(gauss[i],Abs(extended_gauss[i]) - 1) )
//            {
//                failedQ = true;
//                break;
//            }
//        }
//        
//        logvalprint("code",ArrayToString(code,{m}));
//        TOOLS_LOGDUMP(gauss);
//        TOOLS_LOGDUMP(ExtendedGaussCode());
//        TOOLS_LOGDUMP(ExtendedGaussCode2(Int(0)));
//        
//        if( failedQ )
//        {
//            eprint(ClassName()+"::WriteMacLeodCode2<"+TypeName<T>+">: gauss != extended_gauss");
//            logvalprint("code",ArrayToString(code,{ArcCount()}));
//            TOOLS_LOGDUMP(gauss);
//            TOOLS_LOGDUMP(ExtendedGaussCode());
//        }
//    }
//    
//    // Now `buffer` contains the unrotated  code.
//    
//    // Now we are looking for the rotation that makes the code lexicographically maximal.
//        
//    Size_T counter = 0;
//    
//    auto greaterQ = [&counter,code,m]( T s, T t )
//    {
//        for( T i = 0; i < m; ++ i)
//        {
//            ++counter;
//            
//            const T s_i = (s + i < m) ? (s + i) : (s + i - m);
//            const T t_i = (t + i < m) ? (t + i) : (t + i - m);
//            
//            const T g_s_i = code[s_i];
//            const T g_t_i = code[t_i];
//            
//            if( g_s_i > g_t_i )
//            {
//                return true;
//            }
//            else if( g_s_i < g_t_i )
//            {
//                return false;
//            }
//        }
//        
//        return false;
//    };
//    
//    T s = 0;
//    
//    for( T t = 0; t < m; ++t )
//    {
//        if( greaterQ( t, s ) )
//        {
//            s = t;
//        }
//    }
//    
//    this->template SetCache<false>("MacLeodComparisonCount2", counter );
//    
//    rotate_buffer<Side::Left>( code, s, m );
}
