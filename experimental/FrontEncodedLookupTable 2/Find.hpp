public:


template<IntQ Int>
FindResult_T Find( cptr<Int> key ) const
{
//    print("\nFind");
    
    if( !loadedQ ) { return NotLoaded; }
    if(  failedQ ) { return Failed; }

    Key_T query_key;
    for( Size_T i = 0; i < key_length; ++i )
    {
        query_key[i] = static_cast<UInt>(key[i]);
    }
    
    const UInt prefix = query_key[Size_T(0)];
    
    Size_T ptr = prefix_ptr[prefix];
    Size_T pos = prefix_pos[prefix];
    
    UInt dist_to_end = buffer[ptr];
    ++ptr;
    
    if( static_cast<Size_T>(dist_to_end) != key_length )
    {
//        eprint("dist_to_end != key_length");
        return Failed;
    }
    
    Key_T curr_key;
    
    for( Size_T i = 0; i < key_length; ++i )
    {
        curr_key[i] = buffer[ptr + i];
    }
    
    if( curr_key == query_key ) { return {values[pos],0}; }
    
    do
    {
        ptr += dist_to_end;
        ++pos;
        dist_to_end = buffer[ptr];
        ++ptr;
//        TOOLS_DUMP(ptr);
//        TOOLS_DUMP(pos);
//        TOOLS_DUMP(key_length - dist_to_end);
        
        const Size_T offset = key_length - dist_to_end;
        
        for( Size_T i = 0; i < dist_to_end; ++i )
        {
//            TOOLS_DUMP(i);
            curr_key[offset + i] = buffer[ptr + i];
        }
        

//        TOOLS_DUMP(curr_key);

        if( curr_key == query_key ) { return {values[pos],0}; }
    }
    while( dist_to_end != key_length );
    
//    TOOLS_DUMP(dist_to_end);
//    TOOLS_DUMP(key_length);
//    TOOLS_DUMP(ptr);
//    TOOLS_DUMP(pos);
//    TOOLS_DUMP(curr_key);
    
    return NotFound;
}
