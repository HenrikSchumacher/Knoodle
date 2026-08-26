public:

/*!@brief Search the node whose data filed equals `data`. Return the node index if such a node is found. Return `NIL` otherwise.
 */
Int FindNode( const Data_T & data )
{
    Find(data);
    return Current();
}

/*!@brief Check whether a node whose data filed equals `data` is contained in the tree.
 */
bool ContainsQ( const Data_T & data  )
{
    return Find(data);
}

/*!@brief Search the node whose data filed equals `data` and tracks the path from the root in an internal stack. If such a node is found, then make it available through `Current()` and return `true`. Otherwise, make `Current()` return the `NIL`, make `Parent()` the leave node where the search was terminated, and return false.
 */
template<bool verboseQ = false>
bool Find( const Data_T & data )
{
    if constexpr ( verboseQ )
    {
        logprint(MethodName("Find(" + ToString(data)+ ")"));
    }
    
    auto target_val = std::invoke(f, data);
    ResetPath();
    
    while( Current() != NIL )
    {
        auto val = std::invoke(f, GetData(Current()));
        if( val == target_val ) { return true; }
        const bool side = std::invoke(cmp, val, target_val);
        PushPath(side);
    }
    
    return false;
}


void FindMin()
{
    ResetPath();
    WalkToEnd(Left);
}

Int MinNode()
{
    FindMin();
    return Current();
}

void FindMax()
{
    ResetPath();
    WalkToEnd(Right);
}

Int MaxNode()
{
    FindMax();
    return Current();
}
