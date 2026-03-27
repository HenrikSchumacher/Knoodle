public:


template<bool farfalleQ  = true>
Size_T PDCodeCrossingCount() const
{
    Size_T c_count = 0;
    
    for( PD_T & pd : pd_list )
    {
        c_count += pd.PDCodeCrossingCount(farfalleQ);
    }
    
    return c_count;
}

template<IntQ T, PDCode_TArgs_T targs, bool color_warningQ = true>
void WritePDCode( mptr<T> pd_code ) const
{
    static_assert( targs.farfalleQ == true, "This won't work correctly without farfalle." );
    
    [[maybe_unused]] auto tag = []()
    {
        return MethodName("WritePDCode")+
            + "<" + TypeName<T>
            + "," + ToString(targs)
            + ">";
    };
    
    TOOLS_PTIMER(timer,tag());

    if constexpr ( !targs.signQ )
    {
        wprint(tag() + ": Attempting to write unsigned pd code. Beware that this may lead to loss of information.");
    }
    
    if constexpr ( color_warningQ && !targs.colorQ )
    {
        wprint(tag() + ": Attempting to write uncolored pd code. Beware that this may lead to the loss of information on connected summands and thus to a different link class.");
    }

    constexpr Int code_width = static_cast<Int>(PD_T::PDCodeWidth(targs.signQ,targs.colorQ));
    
    Size_T pos = 0;
    T offset   = 0;
    
    for( PD_T & pd : pd_list )
    {
        if( pd.InvalidQ() ) { continue; }
        
        pd.template WritePDCode<T,targs>( &pd_code[pos], offset );
        
        if( pd.AnelloQ() )
        {
            pos    += code_width;
            offset += T(2);
        }
        else
        {
            offset += int_cast<T>(pd.ArcCount());
            pos    += code_width * ToSize_T(pd.CrossingCount());
        }
    }
}

template<IntQ T, PDCode_TArgs_T targs = {.signQ = true, .colorQ = true, .farfalleQ = true}>
Tensor2<T,Int> PDCode() const
{
    [[maybe_unused]] auto tag = []()
    {
        return MethodName("PDCode")+
            + "<" + TypeName<T>
            + "," + ToString(targs)
            + ">";
    };
    
    TOOLS_PTIMER(timer,tag());
    
    Tensor2<T,Int> pd_code ( PDCodeCrossingCount(), PD_T::PDCodeWidth(targs.signQ,targs.colorQ) );
    
    if constexpr ( targs.colorQ )
    {
        WritePDCode<T,targs>( pd_code.data() );
    }
    else
    {
        // Without color information we have to connect the pieces, first.
        this->ConnectedSum().template WritePDCode<T,targs,false>( pd_code.data() );
    }
    
    return pd_code;
}

template<PDCode_TArgs_T targs = {.signQ = true, .colorQ = true, .farfalleQ = true}>
OutString ToPDCodeString() const
{
    auto pd_code = PDCode<Int,targs>();

    return OutString::FromArray(
         pd_code.ReadAccess(),
         pd_code.Dim(0), "", "\n", "",
         pd_code.Dim(1), "", "\t", ""
    );
}

template<PDCode_TArgs_T targs = {.signQ = true, .colorQ = true, .farfalleQ = true}>
void ToPDCodeFile( cref<std::filesystem::path> file ) const
{
    std::ofstream stream ( file );
    
    stream << ToPDCodeString<targs>();
}

template<FromPDCode_TArgs_T targs = {.signQ = true, .colorQ = true}, IntQ T, IntNotBoolQ ExtInt>
static PDC_T FromPDCode(
    cptr<T> pd_code,
    const ExtInt crossing_count,
    const bool splitQ = true
)
{
    PDC_T pdc ( PD_T::template FromPDCode<targs>( pd_code, crossing_count, false, false ) );
    
    if( splitQ ) { pdc.Split(); }
    
    return pdc;
}

template<IntQ T, IntNotBoolQ ExtInt, IntNotBoolQ ExtInt2>
static PDC_T FromPDCode(
    cptr<T> pd_code,
    const ExtInt crossing_count,
    const ExtInt2 code_width,
    const bool splitQ = true
)
{
    PDC_T pdc ( PD_T::FromPDCode( pd_code, crossing_count, code_width, false, false ) );
    
    if( splitQ ) { pdc.Split(); }
    
    return pdc;
}

static PDC_T FromPDCodeString( mref<InString> s, const bool splitQ = true )
{
    PDC_T pdc ( PD_T::FromPDCodeString( s, false, false ) );
    
    if( splitQ ) { pdc.Split(); }
    
    return pdc;
}

static PDC_T FromPDCodeFile( cref<std::filesystem::path> file, const bool splitQ = true )
{
    InString s ( file );
    
    return FromPDCodeString( s, splitQ );
}
