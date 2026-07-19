public:

static Key_T ToKey( cref<PD_T> pd )
{
    if( std::cmp_greater(pd.CrossingCount(),max_crossing_count) )
    {
        return InvalidKey;
    }
    
    Key_T key = {};
    mptr<CodeInt> ptr = reinterpret_cast<CodeInt *>(&key[0]);
    pd.WriteMacLeodCode( ptr );
    return key;
}

static PD_T FromKey( cref<Key_T> key )
{
    cptr<CodeInt> ptr = reinterpret_cast<const CodeInt *>(&key[0]);
    return PD_T::FromMacLeodCode( ptr, MacLeodCodeLength(key), Int(0) );
}

static Size_T MacLeodCodeLength( cref<Key_T> key )
{
    return Klut_T::MacLeodCodeLength(key);
}

static Int CrossingCount( cref<Key_T> key )
{
    return int_cast<Int>(Klut_T::CrossingCount(key));
}

template<IntQ T, IntQ ExtInt>
static Key_T MacLeodCodeToKey( cptr<T> s_mac_leod, ExtInt n )
{
    return Klut_T::MacLeodCodeToKey(s_mac_leod,n);
}

// Beware: Background of s_mac_leod should be filled by 0.
template<IntQ T>
static void KeyToMacLeodCode( cref<Key_T> key, mptr<T> s_mac_leod )
{
    Klut_T::KeyToMacLeodCode(key,s_mac_leod);
}
