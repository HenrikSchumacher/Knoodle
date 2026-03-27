public:

bool WriteToFile( cref<std::filesystem::path> file, bool leading_kQ = true ) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out );

    if( !stream )
    {
        eprint(MethodName("WriteToFile") + ": Could not open file " + file.string() + ". Aborting.");
        return false;
    }
    
    if( leading_kQ )
    {
        stream << 'k' << '\n';
    }
    
    OutString s;
    
    const Size_T diagram_count = DiagramCount();
    
    for( Size_T i = 0; i < diagram_count; ++i )
    {
        mref<PD_T> pd = pd_list[i];
        
        if( pd.InvalidQ() ) { continue; }
        
        if( pd.AnelloQ() )
        {
            stream << "u " << pd.LastColorDeactivated() << "\n";
            continue;
        }

        stream << "s " << pd.ProvenMinimalQ() << "\n";

        auto pd_code = pd.template PDCode<Int,{.signQ = true, .colorQ = true, .farfalleQ = false}>();
        
        s.Clear();

        s.PutArray(
            pd_code.ReadAccess(), true,
            pd.CrossingCount(), "", "\n", "\n",
            Int(7), "", "\t", ""
        );
        
        stream << s;
    }
    
    return true;
}
