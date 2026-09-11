public:

bool WriteToFile( cref<std::filesystem::path> file, const bool leading_kQ = true ) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out );

    if( !stream )
    {
        Msgr::eprint("WriteToFile", "Could not open file " + file.string() + ". Aborting.");
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
    
    const Size_T diagram_count = pd_list.size();
    
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

/*!@brief Export to `OutString`, using default options.*/
friend OutString & operator<<( OutString & s, const PDC_T & pdc )
{
    (void)pdc.WriteToOutString(s);
    return s;
}

/*!@brief Export to `std::basic_ostream`, using default options.*/
template<typename C, typename T>
friend std::basic_ostream<C,T> & operator<<(
    std::basic_ostream<C,T> & stream, const PDC_T & pdc
)
{
    OutString s;
    s << pdc;
    return stream << s;
}
