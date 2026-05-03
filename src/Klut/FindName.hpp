private:

template<bool testQ = true, IntQ Int>
std::string FindName_impl( cref<Key_T> key, Int n  )
{
    const Size_T c = static_cast<Size_T>(n);
    
    if constexpr( testQ )
    {
        if( std::cmp_less(n,3) || std::cmp_greater(c,CrossingCount()) )
        {
            return "NotFound";
        }
    }
    return subtables[c].FindName(key);
}

public:

template<IntQ Int>
std::string FindName( cref<Key_T> key, Int n  )
{
    const Size_T c = static_cast<Size_T>(n);
    
    if( std::cmp_less(n,3) || std::cmp_greater(c,CrossingCount()) )
    {
        return "NotFound";
    }
    return FindName_impl<false>(key,c);
}

std::string FindName( cref<Key_T> key )
{
    const Size_T c = static_cast<Size_T>(CrossingCount(key));
    
    if( std::cmp_less(c,3) || std::cmp_greater(c,CrossingCount()) )
    {
        return "NotFound";
    }
    return FindName_impl<false>(key,c);
}


template<typename T, IntQ Int>
std::string FindName( cptr<T> s_mac_leod, Int n )
{
    const Size_T c = static_cast<Size_T>(n);
    
    if( std::cmp_less(n,3) || std::cmp_greater(c,CrossingCount()) )
    {
        return "NotFound";
    }
    return FindName_impl<false>( MacLeodCodeToKey(s_mac_leod,c), c );
}

template<typename T, IntQ Int>
std::string FindName( cref<Tensor1<T,Int>> s_mac_leod )
{
    return FindName( &s_mac_leod[0], static_cast<Size_T>(s_mac_leod.Size()) );
}


template<typename T, IntQ Int>
std::string FindName( cref<std::vector<T,Int>> s_mac_leod )
{
    return FindName( &s_mac_leod[0], s_mac_leod.size() );
}

std::string FindName( std::string_view s_mac_leod )
{
    return FindName( &s_mac_leod[0], s_mac_leod.size() );
}

template<IntQ Int>
std::string FindName( cref<PlanarDiagram<Int>> pd )
{
    if( pd.InvalidQ() ) { return "Invalid"; }
    
    const Size_T c = static_cast<Size_T>(pd.CrossingCount());
    
    if( std::cmp_less(c,3) || std::cmp_greater(c, CrossingCount()) ) { return "NotFound"; }
    
    if( pd.LinkComponentCount() > Int(1) ) { return "NotFound"; }
    
    pd.template WriteMacLeodCode<CodeInt>(s_mac_leod_buffer.data());
    
    return FindName( &s_mac_leod_buffer[0], c );
}


std::string FindName( cref<std::pair<Size_T,ID_T>> id )
{
    const Size_T c = id.first;
    const ID_T   i = id.second;
    
    if( i == invalid )
    {
        return "Invalid";
    }
    else if( i == error )
    {
        return "Error";
    }
    else if( i == not_found )
    {
        return "NotFound";
    }
    else if( std::cmp_less(c,3) || std::cmp_greater(c,CrossingCount()) )
    {
        return "NotFound";
    }
    else
    {
        return subtables[c].knot_names[i];
    }
}
