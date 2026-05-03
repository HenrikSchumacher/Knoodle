private:

template<bool testQ = true, IntQ Int>
std::pair<Size_T,ID_T> FindID_impl( cref<Key_T> key, Int n )
{
    const Size_T c = static_cast<Size_T>(n);
    
    if constexpr( testQ )
    {
        if( std::cmp_less(n,3) || std::cmp_greater(c,CrossingCount()) )
        {
            return std::pair{c,not_found};
        }
    }
    const ID_T id = subtables[c].FindID(key);
    
    return std::pair{c,id};
}

public:

template<IntQ Int>
std::pair<Size_T,ID_T> FindID( cref<Key_T> key )
{
    return FindID_impl<true>(key,MacLeodCodeLength(key));
}


template<typename T, IntQ Int>
std::pair<Size_T,ID_T> FindID( cptr<T> s_mac_leod, Int n )
{
    const Size_T c = static_cast<Size_T>(n);
    
    if( std::cmp_less(n,3) || std::cmp_greater(c,CrossingCount()) )
    {
        return {c, not_found};
    }
    return FindID_impl<false>( MacLeodCodeToKey(s_mac_leod,c), c );
}

// Syntax sugar.

template<typename T, IntQ Int>
std::pair<Size_T,ID_T> FindID( cref<Tensor1<T,Int>> s_mac_leod )
{
    return FindID( &s_mac_leod[0], static_cast<Size_T>(s_mac_leod.Size()) );
}

template<typename T, IntQ Int>
std::pair<Size_T,ID_T> FindID( cref<std::vector<T,Int>> s_mac_leod )
{
    return FindID( &s_mac_leod[0], s_mac_leod.size() );
}

std::pair<Size_T,ID_T> FindID( std::string_view s_mac_leod )
{
    return FindID( &s_mac_leod[0], s_mac_leod.size() );
}

template<IntQ Int>
std::pair<Size_T,ID_T> FindID( cref<PlanarDiagram<Int>> pd )
{
    if( pd.InvalidQ() ) { return {invalid,invalid}; }
    
    Size_T c = static_cast<Size_T>(pd.CrossingCount());
    
    if( std::cmp_less(c,3) ||std::cmp_greater(c, CrossingCount()) ) { return {c,not_found}; }
    
    if( pd.LinkComponentCount() > Int(1) ) { return {c,not_found}; }
    
    pd.template WriteMacLeodCode<CodeInt>(s_mac_leod_buffer.data());
    
    return FindID( &s_mac_leod_buffer[0], c );
}
