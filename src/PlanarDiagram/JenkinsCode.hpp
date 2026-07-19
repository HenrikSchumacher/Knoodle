public:

template<SignedIntQ T>
Tensor1<T,Size_T> JenkinsCode() const
{
    Tensor1<T,Size_T> code ( JenkinsCodeSize() );

    WriteJenkinsCode( code.data() );

    return code;
}

Size_T JenkinsCodeSize() const
{
    return Size_T(1) + ToSize_T(LinkComponentCount()) + Size_T(6) * ToSize_T(CrossingCount());
}

template<SignedIntQ T>
void WriteJenkinsCode( mptr<T> code ) const
{
    Int lc_count = LinkComponentCount();
    
    code[0] = static_cast<T>(lc_count);
    
    Size_T idx = 1;
    Size_T comp_start = 1;
    Size_T c_start = Size_T(1) + static_cast<Size_T>(lc_count) + Size_T(4) * static_cast<Size_T>(CrossingCount());
    
    this->template Traverse<true,false>(
        [&idx,&comp_start]( const Int a, const Int lc, const Int lc_begin )
        {
            (void)a;
            (void)lc;
            (void)lc_begin;
            comp_start = idx;
            ++idx;
        },
        [&idx,c_start,code,this](
            const Int a,   const Int a_pos,   const Int  lc,
            const Int c_0, const Int c_0_pos, const bool c_0_visitedQ,
            const Int c_1, const Int c_1_pos, const bool c_1_visitedQ
        )
        {
            (void)a_pos;
            (void)lc;
            (void)c_0_visitedQ;
            (void)c_1;
            (void)c_1_pos;
            (void)c_1_visitedQ;
            
            {
                code[idx + Size_T(0)] = c_0_pos;
                code[idx + Size_T(1)] = ArcOverQ(a,Tail) ? T(1) : T(-1);
                idx += Int(2);
            }
            
            if( !c_0_visitedQ )
            {
                const Size_T pos  = c_start + Size_T(2) * static_cast<Size_T>(c_0_pos);
                code[pos + Size_T(0)] = c_0_pos;
                code[pos + Size_T(1)] = CrossingRightHandedQ(c_0) ? T(1) : T(-1);
            }
        },
        [&comp_start,code]( const Int lc, const Int lc_begin, const Int lc_end )
        {
            (void)lc;
            code[comp_start] = static_cast<T>(lc_end - lc_begin);
        }
    );
}


OutString ToJenkinsCodeString() const
{
    using T = ToSigned<Int>;
    
    auto jenkins_code = JenkinsCode<T>();

    OutString s ( jenkins_code.Size() * (ToChars<T>::char_count + Size_T(1)) );
    
    T lc_count = jenkins_code[0];
    
    s.Put(lc_count);
    s.PutChar('\n');
    
    T a_begin = 0;
    T a_end   = 0;
    Int idx = 1;
    
    Size_T a_count = 0;
    
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        T comp_size = jenkins_code[idx];
        a_count += comp_size;
        ++idx;

        a_begin = a_end;
        a_end   = a_begin + comp_size;
        
        s.Put(comp_size);
        
        for( T a = a_begin; a < a_end; ++a )
        {
            s.PutChar(' ');
            s.Put(jenkins_code[idx++]);
            s.PutChar(' ');
            s.Put(jenkins_code[idx++]);
        }
        s.PutChar('\n');
    }
    
    s.template PutVector<Format::Vector::Space>( &jenkins_code[idx], a_count );
    
    return s;
}

void ToJenkinsCodeFile( cref<std::filesystem::path> file ) const
{
    std::ofstream stream ( file );
    
    stream << ToJenkinsCodeString();
}

template<IntQ T>
static std::pair<PD_T,Tensor1<Int,Int>> FromJenkinsCode( cptr<T> jenkins_code )
{
    if( jenkins_code == nullptr )
    {
        return {InvalidDiagram(),Tensor1<Int,Int>()};
    }
    
    Int lc_count = int_cast<Int>(jenkins_code[0]);
    
    if( lc_count == Int(0) )
    {
        return {InvalidDiagram(),Tensor1<Int,Int>()};
    }
    
    Int c_count = 0;
    Int idx = 1;
    
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        Int comp_size = jenkins_code[idx];
        c_count += comp_size;
        idx += Int(2) * comp_size + Int(1);
    }
    
    const Int c_start = idx;
    c_count /= Int(2);

    PD_T pd ( c_count, true );

    pd.crossing_count  = pd.max_crossing_count;
    pd.arc_count       = pd.max_arc_count;
    pd.proven_minimalQ = false;
    
    for( Int i = 0; i < c_count; ++i )
    {
        const Int pos = c_start + Int(2) * i;
        const Int c = static_cast<Int>(jenkins_code[pos + Int(0)]);
        pd.C_state[c] = BooleanToCrossingState(jenkins_code[pos + Int(1)] > T(0));
    }
    
    ColorCounts_T color_arc_counts;
    Aggregator<Int,Int> anello_colors;

    Int a_begin = 0;
    Int a_end   = 0;
    idx = 1;
    
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        Int comp_size = static_cast<Int>(jenkins_code[idx]);
        ++idx;

        if( comp_size == Int(0) )
        {
            anello_colors.Push(lc);
            continue;
        }
        
        a_begin = a_end;
        a_end   = a_begin + comp_size;
        color_arc_counts[lc] = comp_size;
        
        {
            const Int a              = a_begin;
            const Int c              = static_cast<Int>(jenkins_code[idx++]);
            const Int a_prev         = a_end - Int(1);
            
            const bool overQ         = jenkins_code[idx++] > T(0);
            const bool right_handedQ = pd.CrossingRightHandedQ(c);
            const bool side          = overQ != right_handedQ;
            
            pd.C_arcs(c,In , side)   = a_prev;
            pd.C_arcs(c,Out,!side)   = a;
            
            pd.A_cross(a_prev,Head)  = c;
            pd.A_cross(a     ,Tail)  = c;
            pd.A_state[a]            = ArcState_T::Active;
            pd.A_color[a]            = lc;
        }
        
        for( Int a = a_begin + Int(1); a < a_end; ++a )
        {
            const Int  c             = static_cast<Int>(jenkins_code[idx++]);
            const Int  a_prev        = a - Int(1);
            
            const bool overQ         = jenkins_code[idx++] > T(0);
            const bool right_handedQ = pd.CrossingRightHandedQ(c);
            const bool side          = overQ != right_handedQ;

            pd.C_arcs(c,In , side)   = a_prev;
            pd.C_arcs(c,Out,!side)   = a;
            
            pd.A_cross(a_prev,Head)  = c;
            pd.A_cross(a     ,Tail)  = c;
            pd.A_state[a]            = ArcState_T::Active;
            pd.A_color[a]            = lc;
        }
    }
    
    pd.template SetCache<false>("LinkComponentCount",lc_count - anello_colors.Size());
    
    pd.template SetCache<false>("ColorArcCounts",std::move(color_arc_counts));
    
    return {pd,anello_colors.Disband()};
}


static std::pair<PD_T,Tensor1<Int,Int>> FromJenkinsCodeString( mref<InString> s )
{
    using T = ToSigned<Int>;
    
    T x = 0;
    Aggregator<Int,Int> agg ( 128 );
    
    if( s.EmptyQ() )
    {
        return {PD_T::InvalidDiagram(),Tensor1<Int,Int>()};
    }
    
    s.SkipWhiteSpace();
    while( !s.EmptyQ() && !s.FailedQ() )
    {
        s.Take(x);
        agg.Push(x);
        s.SkipWhiteSpace();
    }
    
    if( s.FailedQ() )
    {
        eprint(MethodName("FromJenkinsCodeString") + ": Reading failed.");
        
        return {PD_T::InvalidDiagram(),Tensor1<Int,Int>()};
    }
    
    return FromJenkinsCode( agg.data() );
}

static std::pair<PD_T,Tensor1<Int,Int>> FromJenkinsCodeFile( cref<std::filesystem::path> file )
{
    InString s ( file );

    if( s.FailedQ() || s.EmptyQ() )
    {
        eprint(MethodName("FromJenkinsCodeFile") + ": Reading failed.");
        return {PD_T::InvalidDiagram(),Tensor1<Int,Int>()};
    }
    
    return FromJenkinsCodeString( s );
}
