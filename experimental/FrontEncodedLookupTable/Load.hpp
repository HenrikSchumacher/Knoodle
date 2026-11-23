
private:

Int ThisBlockSize( Int b ) const
{
    return ((b + Int{1}) == block_count) ? last_block_size : block_size;
}

template<bool sortedQ, IntQ B_T, typename V_T, IntQ I_0>
int Load_impl(
    cptr<B_T> keys,
    cptr<V_T> vals,
    cptr<I_0> ordering
)
{
    Int ptr = 0;
    Int k = 0;
    Int l = 0;
    
    for( Int b = 0; b < block_count; ++b )
    {
        block_ptr[b] = ptr;
        const Int this_block_size = ThisBlockSize(b);
        
        if constexpr ( sortedQ )
        {
            l = k;
        }
        else
        {
            l = static_cast<Int>(ordering[k]);
        }
        values[k] = static_cast<Value_T>(vals[l]);
        last_key.Read(&keys[key_size * l]);
        last_key.Write(block_keys.data(b));
        ++k;
        
        // We write the first key of each block to block_keys.
        // No need to store it in buffer.
        
        for( Int idx = 1; idx < this_block_size; ++idx )
        {
//            TOOLS_DUMP(idx);
            
            if constexpr ( sortedQ )
            {
                l = k;
            }
            else
            {
                l = static_cast<Int>(ordering[k]);
            }
            values[k] = static_cast<Value_T>(vals[l]);
            this_key.Read(&keys[key_size * l]);
            
            Int i = 0;
            
            for( ; i < key_size; ++i )
            {
                // Don't allow strictly smaller keys.
                if( this_key[i] < last_key[i] )
                {
                    TOOLS_DUMP(last_key);
                    TOOLS_DUMP(this_key);
                    TOOLS_DUMP(i);
                    TOOLS_DUMP(last_key[i]);
                    TOOLS_DUMP(this_key[i]);
                    return 4;
                }
                
                if( this_key[i] > last_key[i] ) { break; }
            }
            
            // key and last_key coincide. Abort.
            if( i == key_size )
            {
                Msgr::eprint("Load_impl","this_key and last_key coincide. Inputs keys contain duplicates.");
                TOOLS_DUMP(this_key);
                TOOLS_DUMP(last_key);
                return 10;
            }

            Int dist_to_end = key_size - i;
            buffer[ptr] = static_cast<Byte_T>(dist_to_end);
            ++ptr;
            copy_buffer( &this_key[i], &last_key[i], dist_to_end);
            copy_buffer( &this_key[i], &buffer[ptr], dist_to_end);
            ptr += dist_to_end;
            ++k;
        }
    }
    
    block_ptr[block_count] = ptr;
    last_key.Write(block_keys.data(block_count));
    buffer.template Resize<true>(ptr);
    loadedQ = true;
    
    return 0;
}

public:

template<IntQ B_T, typename V_T, IntQ I_1, IntQ I_2, IntQ I_3>
int LoadSorted(
    cptr<B_T> keys,
    cptr<V_T> vals,
    const I_1 key_count_,
    const I_2 key_size_,
    const I_3 block_size_ = default_block_size
)
{
    Int * ordering = nullptr;
    return this->Load_impl<true>( keys, vals, ordering, key_count_, key_size_, block_size_ );
}

template<IntQ B_T, typename V_T, IntQ I_1, IntQ I_2, IntQ I_3>
int LoadUnsorted(
    cptr<B_T> keys,
    cptr<V_T> vals,
    const I_1 key_count_,
    const I_2 key_size_,
    const I_3 block_size_ = default_block_size
)
{
    // TODO: We might use a needlessly big type for the sort here. Shall we copy to a Byte_T-array, first?
    int info = Reset( key_count_, key_size_, block_size_ );
    
    if( info )
    {
        Msgr::eprint("LoadUnsorted","Error raised by function Reset. Error code = ", info,".");
        return info;
    }
    
    Tensor1<Int,Int> ordering (key_count);
    iota_buffer( ordering.data(), key_count );
    
    const Int m = key_size;
    
    std::sort(
         ordering.begin(),
         ordering.end(),
         [keys,m]( const Int i, const Int j ) -> bool
         {
            return (MemCmp(&keys[m * i],&keys[m * j],m) < 0);
         }
    );
    
    return this->Load_impl<false>( keys, vals, ordering.data() );
}
