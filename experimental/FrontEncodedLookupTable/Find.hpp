private:

static int MemCmp( const Byte_T * key_0, const Byte_T * key_1, const Int n )
{
    return std::memcmp(key_0,key_1,static_cast<Size_T>(n));
}

std::pair<Int,int> FindBlock() const
{
    const Int n = key_size;
    Int L = 0;
    Int R = block_count;
    
    int cmp = MemCmp(this_key.data(),block_keys.data(L),n);
    
    if( cmp  < 0 ) { return {L,3}; } // Mark as failed.
    if( cmp == 0 ) { return {L,1}; } // Mark as exact hit.
    
    cmp = MemCmp(this_key.data(),block_keys.data(R),n);
    
    if( cmp  > 0 ) { return {R,3}; } // Mark as failed.
    if( cmp == 0 ) { return {R,2}; } // Mark as exact hit on last entry.

    // Invariants from here on: entry L <= this_key < entry R
    // Starting binary search.
    while( L + Int(1) < R )
    {
        const Int C = R - (R-L)/Int{2};

        cmp = MemCmp(this_key.data(),block_keys.data(C),n);
        
        if( cmp < 0 )
        {
            R = C;
        }
        else if( cmp == 0 )
        {
            return {C,1};  // Mark as exact hit.
        }
        else // if( cmp >= 0 )
        {
            L = C;
        }
    }
    
    return {L,0};  // Mark as inexact hit.
}


public:

template<IntQ I>
FindResult_T Find( cptr<I> key ) const
{
    this_key.Read(key);
    
    auto [b,info] = FindBlock();

    Int k = b * block_size;
    
    switch( info )
    {
        case 0  : break; // Move on to search within the block.
        case 3  : return NotFound;
        case 1  : return {values[k], 0};
        case 2  : return {values[key_count - Int(1)],0};
        default : return NotFound;
    }
    
    last_key.Read(block_keys.data(b));
    
    if( MemCmp(this_key.data(),last_key.data(),key_size) == 0 )
    {
        return {values[k], 0};
    }
    
    ++k;
    Int ptr = block_ptr[b];
    
    const Int this_block_size = ThisBlockSize(b);
    
    for( Int idx = 1; idx < this_block_size; ++idx )
    {
        Int dist_to_end = buffer[ptr];

        Int pos = key_size-dist_to_end;
        ++ptr;
        
        copy_buffer( &buffer[ptr], &last_key[pos], dist_to_end);
        
        const int cmp = MemCmp(this_key.data(),last_key.data(),key_size);
        
        if( cmp == 0 ) { return {values[k], 0}; }
        if( cmp  < 0 ) { return NotFound; }
        
        ptr += dist_to_end;
        ++k;
    }
     
    return NotFound;
}
