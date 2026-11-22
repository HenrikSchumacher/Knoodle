
private:

template<bool sortedQ, IntQ Int, typename V_T, IntQ Int2, IntQ Int3>
int Load_impl( cptr<Int> keys, cptr<V_T> vals, cptr<Int2> ordering, const Int3 key_count_ )
{
    const Size_T n = key_count_;

    Clear();
    values.reserve(n);
    
    int info = 0;
    Size_T j;
    
    if( Size_T(0) < n )
    {
        if constexpr ( sortedQ )
        {
            (void) ordering;
            j = Size_T(0);
        }
        else
        {
            j = static_cast<Size_T>(ordering[Size_T(0)]);
        }
        
        info = this->Append<true>( &keys[key_length * j], vals[j] );
        
        if( info )
        {
            failedQ = true;
            Msgr::eprint("Load", "Something went wrong. Error code = ", info, ".");
            return info;
        }
    }
    for( Size_T i = 1; i < ToSize_T(key_count_); ++i )
    {
        if constexpr ( sortedQ )
        {
            (void) ordering;
            j = i;
        }
        else
        {
            j = static_cast<Size_T>(ordering[i]);
        }
        
        info = this->Append<false>( &keys[key_length * j], vals[j] );
        
        if( info )
        {
            failedQ = true;
            Msgr::eprint("Load", "Something went wrong. Error code = ", info, ".");
            return info;
        }
    }
    
    Close();
    
    return 0;
}

private:

template<bool first_elementQ, IntQ Int, typename V_T>
int Append( cptr<Int> key, const V_T val )
{
    Key_T this_key;
    
    for( Size_T i = 0 ; i < key_length; ++i )
    {
        this_key[i] = static_cast<UInt>(key[i]);
    }
    
    Size_T i = 0;
    
    if constexpr ( !first_elementQ )
    {
        // Move forward until i is the most significant position where key and last_key differ.
        for( ; i < key_length; ++i )
        {
            // Don't allow strictly smaller keys.
            if( this_key[i] < last_key[i] )
            {
                TOOLS_DUMP(last_key);
                TOOLS_DUMP(i);
                TOOLS_DUMP(last_key[i]);
                TOOLS_DUMP(this_key[i]);
                valprint("current_key",OutString::FromVector(key,key_length));
                return 1;
            }
            
            if( this_key[i] > last_key[i] ) { break; }
        }
        
        // key and last_key coincide. Abort.
        if( i == key_length ) { return 2; }
    }
        
    // If key and last_key differ at the most significant position, we need to set prefix_ptr correctly.
    if( i == Size_T{0} )
    {
        UInt prefix = this_key[i];
        
        for( UInt j = last_key[Size_T(0)]; j < prefix; ++j )
        {
            prefix_ptr[j + UInt(1)] = buffer.size();
            prefix_pos[j + UInt(1)] = KeyCount();
        }
    }
    
    UInt dist_to_end = static_cast<UInt>(key_length - i);
    
    buffer.push_back(dist_to_end);
    
    for( ; i < key_length; ++i )
    {
        last_key[i] = this_key[i];
        buffer.push_back(this_key[i]);
    }
    values.push_back(val);
    
    return 0;
}

void Close()
{
    if( loadedQ || failedQ ) { return; }
    
    constexpr UInt max_j = 255;
    
    for( UInt j = last_key[Size_T(0)]; j < max_j; ++j )
    {
        prefix_ptr[j + UInt(1)] = buffer.size();
        prefix_pos[j + UInt(1)] = KeyCount();
    }
    
    loadedQ = true;
}

public:

template<IntQ Int, typename V_T, IntQ Int2>
int LoadSorted( cptr<Int> keys, cptr<V_T> vals, const Int2 key_count_ )
{
    Size_T * ordering = nullptr;
    return this->Load_impl<true>( keys, vals, ordering, key_count_ );
}

template<IntQ Int, typename V_T, IntQ Int2>
int LoadUnsorted( cptr<Int> keys, cptr<V_T> vals, const Int2 key_count_ )
{
    // TODO: We might use a needlessly big type for the sort.
    // TODO: We might want to load the keys into a sequence of Size_T to accelerate the comparison.
    
    const Size_T n = ToSize_T(key_count_);
    Tensor1<Size_T,Size_T> ordering (n);
    iota_buffer( ordering.data(), n );
    constexpr Size_T m = key_length;
    
    
    std::sort( ordering.begin(), ordering.end(),
         [keys]( const Size_T i, const Size_T j ) -> bool
         {
            if (i == j) { return false;}
        
            for( Size_T k = 0; k < m; ++k )
            {
                const UInt a_k = static_cast<UInt>(keys[m * i + k]);
                const UInt b_k = static_cast<UInt>(keys[m * j + k]);
                
                if( a_k < b_k ) { return true; }
                if( a_k > b_k ) { return false; }
            }
        
            return false;
         }
    );
    
    return this->Load_impl<false>( keys, vals, ordering.data(), n );
}
