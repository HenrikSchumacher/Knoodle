public:

Path_T KeyFile() const
{
    return target_directory / ("Klut_Keys_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".bin");
}

Path_T ValueFile() const
{
    return target_directory / ("Klut_Values_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".tsv");
}

Path_T ExportValues() const
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ExportValues"); };
    
    if( !SucceededQ() )
    {
        eprint(tag() + ": Klutter did not succeed. Aborting to not overwrite data." );
        return Path_T();
    }
    
    Path_T file = ValueFile();
    
    std::ofstream stream ( file, std::ios::binary );
    if( !stream )
    {
        eprint(tag() + ": Could not open " + file.string() +". Aborting." );
        return Path_T();
    }
    
    for( ID_T id = 0; id < KnotTypeCount(); ++id )
    {
        stream << names[id] << "\t" << buckets[id].size() << "\n";
    }
    
    return file;
}

Path_T ExportKeys() const
{
    [[maybe_unused]] auto tag = [](){ return MethodName("ExportKeys"); };
    
    if( !SucceededQ() )
    {
        eprint(tag() + ": Klutter did not succeed. Aborting to not overwrite data." );
        return Path_T();
    }
    
    Path_T file = KeyFile();
    
    std::ofstream stream ( file, std::ios::binary );
    if( !stream )
    {
        eprint(tag() + ": Could not open " + file.string() + ". Aborting." );
        return Path_T();
    }
    
    for( ID_T id = 0; id < KnotTypeCount(); ++id )
    {
        for( const Key_T & key : buckets[id] )
        {
            stream.write( reinterpret_cast<const char *>(&key[0]), crossing_count );
        }
    }
    
    return file;
}
