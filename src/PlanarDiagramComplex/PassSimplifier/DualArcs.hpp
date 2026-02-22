private:


//        static constexpr Int tail_pos  = 0; // <-- Caution!
//        static constexpr Int mark_pos  = 1;
//        static constexpr Int head_pos  = 2; // <-- Caution!
//        static constexpr Int from_pos  = 3;


static constexpr bool mark_pos  = 0;
static constexpr bool from_pos  = 1;

void SetDualArc( const Int a, const bool direction, const bool forwardQ, const Int from )
{
    SetDualArc(a, direction, forwardQ, from, current_mark);
}

template<typename dummy = void>
void SetDualArc(
    const Int a, const bool direction, const bool forwardQ, const Int from, const Int time_stamp
)
{
    if constexpr ( vector_listQ )
    {
        D_data(a,mark_pos) = (time_stamp << Int(2)) | Int(forwardQ);
        D_data(a,from_pos) = (from << Int(1))       | Int(direction);
        // CAUTION: D_data(a,1) is not the darc we came from! It is the _arc_ we came from + the directions of da when we travelled through it!
    }
    else if constexpr ( hash_map2Q )
    {
        auto & v = D_data[a];
        v[mark_pos] = (time_stamp << Int(2)) | Int(forwardQ);
        v[from_pos] = (from << Int(1))       | Int(direction);
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
Int DualArcMark( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return (D_data(a,mark_pos) >> Int(2));
    }
    else if constexpr ( hash_map2Q )
    {
        if( D_data.contains(a) )
        {
            return (D_data[a][mark_pos] >> Int(2));
        }
        else
        {
            return 0;
        }
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
bool DualArcMarkedQ( const Int a, const Int dual_mark )
{
    if constexpr ( vector_listQ )
    {
        return ((D_data(a,mark_pos) >> Int(2)) == dual_mark);
    }
    else if constexpr ( hash_map2Q )
    {
        if( D_data.contains(a) )
        {
            return ((D_data[a][mark_pos] >> Int(2)) == dual_mark);
        }
        else
        {
            return false;
        }
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
bool DualArcMarkedQ( const Int a )
{
    return DualArcMarkedQ(a, current_mark);
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
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}


template<typename dummy = void>
bool DualArcDirection( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return static_cast<bool>(D_data(a,from_pos) & Int(1));
    }
    else if constexpr ( hash_map2Q )
    {
        return static_cast<bool>(D_data[a][from_pos] & Int(1));
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
void DualArcMarkAsVisitedTwice( const Int a )
{
    if constexpr ( vector_listQ )
    {
        D_data(a,mark_pos) = D_data(a,mark_pos) | Int(2);
    }
    else if constexpr ( hash_map2Q )
    {
        D_data[a][mark_pos] = D_data[a][mark_pos] | Int(2);
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}

template<typename dummy = void>
bool DualArcVisitedTwiceQ( const Int a )
{
    if constexpr ( vector_listQ )
    {
        return D_data(a,mark_pos) & Int(2);
    }
    else if constexpr ( hash_map2Q )
    {
        return D_data[a][mark_pos] & Int(2);
    }
    else
    {
        static_assert(DependentFalse<dummy>,"");
    }
}


bool DualArcLeftToRightQ( const Int a )
{
    return (DualArcForwardQ(a) == DualArcDirection(a));
}
