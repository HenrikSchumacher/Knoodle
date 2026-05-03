public:

static Size_T MacLeodCodeLength( cref<Key_T> key )
{
    cptr<CodeInt> ptr = reinterpret_cast<const CodeInt *>(&key[0]);
    Size_T n = 0;
    // Find the first nonzero position in key.
    while( (n < max_crossing_count) && (ptr[n] != CodeInt(0)) ) { ++n; }
    
    return n;
}

static Size_T CrossingCount( cref<Key_T> key )
{
    return MacLeodCodeLength(key);
}

// Beware: Background of s_mac_leod should be filled by 0.
template<IntQ T>
static void KeyToMacLeodCode( cref<Key_T> key, mptr<T> s_mac_leod )
{
    cptr<CodeInt> ptr = reinterpret_cast<const CodeInt *>(&key[0]);
    copy_buffer( ptr, s_mac_leod, MacLeodCodeLength(key) );
}




template<IntQ T, IntQ ExtInt>
static Key_T MacLeodCodeToKey( cptr<T> s_mac_leod, ExtInt n )
{
    Key_T key = {};
    mptr<CodeInt> ptr = reinterpret_cast<CodeInt *>(&key[0]);
    copy_buffer(s_mac_leod,ptr,n);
    return key;
}

template<typename T, IntQ Int>
static Key_T MacLeodCodeToKey( cref<Tensor1<T,Int>> s_mac_leod )
{
    return MacLeodCodeToKey( &s_mac_leod[0], s_mac_leod.Size() );
}

template<typename T>
static Key_T MacLeodCodeToKey( cref<std::vector<T>> s_mac_leod )
{
    return MacLeodCodeToKey( &s_mac_leod[0], s_mac_leod.size() );
}
