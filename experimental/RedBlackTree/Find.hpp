public:

/*!@brief Search the node whose data filed equals `data`. Return the node index if such a node is found. Return `NIL` otherwise.
 */
template<typename F = std::identity, typename C = std::less<>>
Int FindNode( const Data_T & data, F && f = F(), C && cmp = C() )
{
    Find(data, std::forward<F>(f), std::forward<C>(cmp) );
    return Current();
}

/*!@brief Check whether a node whose data filed equals `data` is contained in the tree.
 */
template<typename F = std::identity, typename C = std::less<>>
bool ContainsQ( const Data_T & data, F && f = F(), C && cmp = C() )
{
    return Find(data, std::forward<F>(f), std::forward<C>(cmp) );
}

/*!@brief Search the node whose data filed equals `data` and tracks the path from the root in an internal stack. If such a node is found, then make it available through `Current()` and return `true`. Otherwise, make `Current()` return the `NIL`, make `Parent()` the leave node where the search was terminated, and return false.
 */
template<typename F = std::identity, typename C = std::less<>>
bool Find( const Data_T & data, F && f = F(), C && cmp = C() )
{
    auto target_val = std::invoke(f, data);
    ResetPath();
    
    while( Current() != NIL )
    {
        auto val = std::invoke(f, Data(Current()));
        if( val == target_val ) { return true; }
        const bool side = std::invoke(cmp, val, target_val);
        PushPath(side);
    }
    
    return false;
}


void FindMin()
{
    ResetPath();
    WalkToEnd<Left>();
}

Int MinNode()
{
    FindMin();
    return Current();
}

void FindMax()
{
    ResetPath();
    WalkToEnd<Right>();
}

Int MaxNode()
{
    FindMax();
    return Current();
}
