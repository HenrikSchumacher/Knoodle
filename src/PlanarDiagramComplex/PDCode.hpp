public:

template<IntQ T = Int>
Tensor2<T,Int> PDCode() const
{
    TOOLS_PTIMER(timer,ClassName("PDCode") + "<" + TypeName<T> + ">");
    
    Size_T c_count = 0;
    
    for( PD_T & pd : pd_list )
    {
        if( pd.InvalidQ() ) { continue; }
        
        if( pd.AnelloQ() )
        {
            ++c_count;
        }
        else
        {
            c_count += ToSize_T(pd.CrossingCount());
        }
    }
    
    Tensor2<T,Int> pd_code ( c_count, Size_T(7) );
    
    WritePDCode( pd_code.data() );
    
    return pd_code;
}

template<IntQ T>
void WritePDCode( mptr<T> pd_code ) const
{
    TOOLS_PTIMER(timer,MethodName("WritePDCode") + "<" + TypeName<T> + ">");
    
    constexpr Size_T step = 7;
    
    Size_T pos = 0;
    T offset   = 0;
    
    for( PD_T & pd : pd_list )
    {
        if( pd.InvalidQ() ) { continue; }
        
        pd.template WritePDCode<T,true,true,false,true>( &pd_code[pos], offset );
        
        if( pd.AnelloQ() )
        {
            pos    += step;
            offset += T(2);
        }
        else
        {
            offset += int_cast<T>(pd.ArcCount());
            pos    += step * ToSize_T(pd.CrossingCount());
        }
    }
}


OutString ToPDCodeString() const
{
    auto pd_code = PDCode();

    return OutString::FromArray(
         pd_code.ReadAccess(),
         pd_code.Dim(0), "", "\n", "",
         pd_code.Dim(1), "", "\t", ""
    );
}

void ToPDCodeFile( cref<std::filesystem::path> file ) const
{
    std::ofstream stream ( file );
    
    stream << ToPDCodeString();
}

static PDC_T FromPDCodeString( mref<InString> s )
{
    Int crossing_counter = 0;
    std::vector<Int> pd_buffer;
    Int x = 0;
    
    s.SkipWhiteSpace();
    
    while( !s.EmptyQ() && !s.FailedQ() )
    {
        // We have to be careful here, because the last line may easily end with an '\n'.
        for( Int i = 0; i < Int(6); ++i )
        {
            s.Take(x);
            pd_buffer.push_back(x);
            s.SkipWhiteSpace();
        }
        s.Take(x);
        pd_buffer.push_back(x);
        
        ++crossing_counter;
        
        if( s.EmptyQ() ) { break; }
        
        s.SkipWhiteSpace();
    }
    
    if( s.FailedQ() )
    {
        eprint(MethodName("FromPDCodeString") + ": Reading from InString failed. Returning invalid object.");
        return PDC_T();
    }
    
    return PDC_T::template FromPDCode<true,true,true>( &pd_buffer[0], crossing_counter, false );
}

static PDC_T FromPDCodeFile( cref<std::filesystem::path> file )
{
    InString s ( file );
    
    return FromPDCodeString(s);
}
