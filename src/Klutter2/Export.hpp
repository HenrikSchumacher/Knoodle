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
    
    Tensor1<Size_T,Size_T> bucket_sizes ( KnotTypeCount() );
    for( auto & subklutter : subklutters )
    {
        for( auto & [id,bucket] : subklutter.identified_buckets )
        {
            bucket_sizes[id] = bucket.size();
        }
    }
    
    for( ID_T id = 0; id < KnotTypeCount(); ++id )
    {
        stream << names[id] << '\t' << bucket_sizes[id] << '\n';
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
    
    Tensor1<Size_T,Size_T> knot_ptr ( KnotTypeCount() + Size_T(1) );
    knot_ptr[Size_T(0)] = Size_T(0);
    
    for( auto & subklutter : subklutters )
    {
        for( auto & [id,bucket] : subklutter.identified_buckets )
        {
            knot_ptr[id + ID_T(1)] = bucket.size();
        }
    }
    knot_ptr.Accumulate();
    
    
    Tensor1<Key_T,Size_T> all_keys ( knot_ptr.Last() );
    
    // TODO: Could be parallelized.
    for( auto & subklutter : subklutters )
    {
        for( auto & [id,bucket] : subklutter.identified_buckets )
        {
            const Size_T begin = knot_ptr[id        ];
            const Size_T end   = knot_ptr[id+ID_T(1)];
            
            Size_T ptr = begin;
            
            for( const Key_T & key : bucket )
            {
                all_keys[ptr++] = key;
            }
            
            // TODO: Sort keys.
            std::sort( all_keys.data(begin), all_keys.data(end) );
        }
    }
    
    for( Size_T i = 0; i < all_keys.Size(); ++i )
    {
        stream.write( reinterpret_cast<const char *>(all_keys.data(i)), crossing_count );
    }
    
//
//    for( ID_T id = 0; id < KnotTypeCount(); ++id )
//    {
//        for( const Key_T & key : buckets[id] )
//        {
//            stream.write( reinterpret_cast<const char *>(&key[0]), crossing_count );
//        }
//    }
    
    return file;
}
