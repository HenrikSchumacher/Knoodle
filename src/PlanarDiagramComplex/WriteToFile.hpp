public:

bool WriteToFile( cref<std::filesystem::path> file, const bool leading_kQ = true ) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out );

    if( !stream )
    {
        eprint(MethodName("WriteToFile") + ": Could not open file " + file.string() + ". Aborting.");
        return false;
    }
    
    OutString s;
    bool succeededQ = WriteToOutString(s,leading_kQ);
    stream << s;
    
    return succeededQ;
}


bool WriteToOutString( mref<Tools::OutString> s, const bool leading_kQ = true ) const
{
    if( leading_kQ ) { s.PutChars("k\n"); }
    
    const Size_T diagram_count = DiagramCount();
    
    constexpr Int code_width = PD_T::PDCodeWidth(true,true);
    
    // One buffer to be reused for all diagrams.
    Tensor2<Int,Int> pd_code ( HighestCrossingCount(), code_width );
    
    for( Size_T i = 0; i < diagram_count; ++i )
    {
        mref<PD_T> pd = pd_list[i];
        
        if( pd.InvalidQ() ) { continue; }
        
        if( pd.AnelloQ() )
        {
            s.PutWithPrefixAndSuffix("u ",pd.FirstColor(),"\n");
            continue;
        }
        
        s.PutWithPrefixAndSuffix("s ",int(pd.ProvenMinimalQ()),"\n");
        
        pd.template WritePDCode<Int,{.signQ = true, .colorQ = true, .farfalleQ = false}>(pd_code.data());

        s.PutArray(
            pd_code.ReadAccess(), true,
            pd.CrossingCount(), "", "\n", "\n",
            code_width, "", "\t", ""
        );
    }
    
    return true;
}
