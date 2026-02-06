private:


//        static constexpr Int tail_pos  = 0; // <-- Caution!
//        static constexpr Int mark_pos  = 1;
//        static constexpr Int head_pos  = 2; // <-- Caution!
//        static constexpr Int from_pos  = 3;


static constexpr bool mark_pos  = 0;
static constexpr bool from_pos  = 1;

template<typename dummy = void>
void SetDualArc( const Int a, const bool forwardQ, const Int from, const bool left_to_rightQ )
{
    
//    logprint(" a = " + ToString(a) + " set to {" + ToString(val_mark) +"," + ToString(val_from) +"}.");
    
    if constexpr ( vector_listQ )
    {
        D_data(a,mark_pos) = (current_mark << Int(1)) | Int(forwardQ);
        D_data(a,from_pos) = (from << Int(1))         | Int(left_to_rightQ);
        // CAUTION: D_data(a,1) is not the darc we came from! It is the _arc_ we came from + the directions of da when we travelled through it!
    }
    else if constexpr ( hash_map2Q )
    {
        auto & v = D_data[a];
        v[mark_pos] = (current_mark << Int(1)) | Int(forwardQ);
        v[from_pos] = (from << Int(1))         | Int(left_to_rightQ);
    }
    else if constexpr ( hash_map1Q )
    {
        D_data[a] = (from << Int(2)) | Int(left_to_rightQ) << Int(1) | Int(forwardQ);
    }
    else if constexpr ( hash_map_structQ )
    {
        D_data[a] = {from,left_to_rightQ,forwardQ};
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
bool DualArcMarkedQ( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return ((D_data(a,mark_pos) >> Int(1)) == current_mark);
    }
    else if constexpr ( hash_map2Q )
    {
        if( D_data.contains(a) )
        {
            return ((D_data[a][mark_pos] >> Int(1)) == current_mark);
        }
        else
        {
            return false;
        }
    }
    else if constexpr ( hash_map1Q || hash_map_structQ )
    {
        return D_data.contains(a);
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
bool DualArcForwardQ( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return static_cast<bool>(D_data(a,mark_pos) & Int(1));
    }
    else if constexpr ( hash_map2Q )
    {
        return static_cast<bool>(D_data[a][mark_pos] & Int(1));
    }
    else if constexpr ( hash_map1Q )
    {
        return static_cast<bool>(D_data[a] & Int(1));
    }
    else if constexpr ( hash_map_structQ )
    {
        return D_data[a].forwardQ;
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
Int DualArcFrom( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return (D_data(a,from_pos) >> Int(1));
    }
    else if constexpr ( hash_map2Q )
    {
        return (D_data[a][from_pos] >> Int(1));
    }
    else if constexpr ( hash_map1Q )
    {
        return (D_data[a] >> Int(2));
    }
    else if constexpr ( hash_map_structQ )
    {
        return D_data[a].from;
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}


template<typename dummy = void>
bool DualArcLeftToRightQ( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return static_cast<bool>(D_data(a,from_pos) & Int(1));
    }
    else if constexpr ( hash_map2Q )
    {
        return static_cast<bool>(D_data[a][from_pos] & Int(1));
    }
    else if constexpr ( hash_map1Q )
    {
        return static_cast<bool>(D_data[a] & Int(2));
    }
    else if constexpr ( hash_map_structQ )
    {
        return D_data[a].left_to_rightQ;
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}
