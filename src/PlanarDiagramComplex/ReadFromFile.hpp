public:

static PDC_T ReadFromFile( cref<std::filesystem::path> file )
{
    Tools::InString s (file);
    
    return FromInString(s);
}

static PDC_T FromInString( mref<Tools::InString> s )
{
    PDC_T pdc;
    
    Int crossing_counter = 0;
    std::vector<Int> pd_buffer;
    UInt8 proven_minimalQ = 0;
    Int last_color_deactivated = PD_T::Uninitialized;
    Int x = 0;
   
    auto push_diagram = [&crossing_counter,&proven_minimalQ,&last_color_deactivated,&pd_buffer,&pdc]()
    {
        if( crossing_counter <= Int(0) ) { return; }
        
        PD_T pd = PD_T::template FromPDCode<true,true,true>(
            &pd_buffer[0], crossing_counter, static_cast<bool>(proven_minimalQ)
        );
        
        if( pd.ValidQ() ) { pdc.Push(std::move(pd)); }
        
        crossing_counter = 0;
        proven_minimalQ = 0;
        last_color_deactivated = PD_T::Uninitialized;
        pd_buffer.clear();
    };

    s.SkipWhiteSpace();
    if( s.CurrentChar() == 'k' ) { s.Skip(1); }
    s.SkipWhiteSpace();

    while( !s.EmptyQ() && !s.FailedQ() )
    {
        if( s.CurrentChar() == 'u' )
        {
            s.Skip(1);
            s.SkipWhiteSpace();
            s.Take(last_color_deactivated);
            s.SkipWhiteSpace();
            pdc.Push(PD_T::Unknot(last_color_deactivated));
            proven_minimalQ = 0;
            last_color_deactivated = PD_T::Uninitialized;
            continue;
        }
        else if( s.CurrentChar() == 's' )
        {
            s.Skip(1);
            s.SkipWhiteSpace();
            s.Take(proven_minimalQ);
            s.SkipWhiteSpace();
            last_color_deactivated = PD_T::Uninitialized;
            continue;
        }
        
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
        eprint(MethodName("Read") + ": Reading failed. Returning invalid object.");
        return PDC_T();
    }
    
    push_diagram();
    
    return pdc;
}
